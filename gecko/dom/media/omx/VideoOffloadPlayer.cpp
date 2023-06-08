/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* (c) 2018 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#include "VideoOffloadPlayer.h"
#include "nsComponentManagerUtils.h"
#include "nsITimer.h"
#include "mozilla/dom/HTMLMediaElement.h"
#include "VideoUtils.h"
#include "mozilla/dom/power/PowerManagerService.h"
#include "mozilla/dom/WakeLock.h"

#include <binder/IPCThreadState.h>
#include <stagefright/foundation/ADebug.h>
#include <stagefright/foundation/ALooper.h>
#include <stagefright/MediaDefs.h>
#include <stagefright/MediaErrors.h>
#include <stagefright/MetaData.h>
#include <stagefright/Utils.h>

#include <binder/IServiceManager.h>
#include <media/IStreamSource.h>
#include <fcntl.h>
#if ANDROID_VERSION == 19
#include <binder/Parcel.h>
#endif

using namespace android;

namespace mozilla {

LazyLogModule gVideoOffloadPlayerLog("VideoOffloadPlayer");
#define VIDEO_OFFLOAD_LOG(type, msg) \
  MOZ_LOG(gVideoOffloadPlayerLog, type, msg)

static const char* const gAndroidMediaEventTypeStr[] = {
    "MEDIA_NOP",              // 0
    "MEDIA_PREPARED",         // 1
    "MEDIA_PLAYBACK_COMPLETE",// 2
    "MEDIA_BUFFERING_UPDATE", // 3
    "MEDIA_SEEK_COMPLETE",    // 4
    "MEDIA_SET_VIDEO_SIZE",   // 5
    "MEDIA_STARTED",          // 6
    "MEDIA_PAUSED",           // 7
    "MEDIA_STOPPED",          // 8
    "MEDIA_SKIPPED",          // 9
    "MEDIA_TIMED_TEXT",       //10
    "MEDIA_ERROR",            //11
    "MEDIA_INFO",             //12
    "MEDIA_SUBTITLE_DATA",    //13
    "MEDIA_META_DATA",        //14
    "MEDIA_UNKNOWN",          //15
  };

const char* AndroidMediaPlayerListener::MediaEventTypeToStr(int aEventType) const
{
  switch(aEventType) {
    case android::MEDIA_NOP:
      return gAndroidMediaEventTypeStr[0];
    case android::MEDIA_PREPARED:
      return gAndroidMediaEventTypeStr[1];
    case android::MEDIA_PLAYBACK_COMPLETE:
      return gAndroidMediaEventTypeStr[2];
    case android::MEDIA_BUFFERING_UPDATE:
      return gAndroidMediaEventTypeStr[3];
    case android::MEDIA_SEEK_COMPLETE:
      return gAndroidMediaEventTypeStr[4];
    case android::MEDIA_SET_VIDEO_SIZE:
      return gAndroidMediaEventTypeStr[5];
    case android::MEDIA_STARTED:
      return gAndroidMediaEventTypeStr[6];
    case android::MEDIA_PAUSED:
      return gAndroidMediaEventTypeStr[7];
    case android::MEDIA_STOPPED:
      return gAndroidMediaEventTypeStr[8];
    case android::MEDIA_SKIPPED:
      return gAndroidMediaEventTypeStr[9];
    case android::MEDIA_TIMED_TEXT:
      return gAndroidMediaEventTypeStr[10];
    case android::MEDIA_ERROR:
      return gAndroidMediaEventTypeStr[11];
    case android::MEDIA_INFO:
      return gAndroidMediaEventTypeStr[12];
    case android::MEDIA_SUBTITLE_DATA:
      return gAndroidMediaEventTypeStr[13];
#if ANDROID_VERSION >= 23
    case android::MEDIA_META_DATA:
      return gAndroidMediaEventTypeStr[14];
#endif
    default:
      return gAndroidMediaEventTypeStr[15];
  }
}

AndroidMediaPlayerListener::AndroidMediaPlayerListener(VideoOffloadPlayer* aOwener)
{
  mVideoOffloadPlayer = aOwener;
}

void
AndroidMediaPlayerListener::notify(int msg, int ext1, int ext2, const Parcel *obj)
{
  VIDEO_OFFLOAD_LOG(LogLevel::Debug, ("%s, %s", __func__, MediaEventTypeToStr(msg)));
  switch(msg) {
    case android::MEDIA_PLAYBACK_COMPLETE:
      mVideoOffloadPlayer->mReachedEOS = true;
      mVideoOffloadPlayer->NotifyVideoEOS();
      break;
    case android::MEDIA_SEEK_COMPLETE:
      if (!mVideoOffloadPlayer->mSeekPromise.IsEmpty()) {
        VIDEO_OFFLOAD_LOG(LogLevel::Debug, ("Seek complete"));
        android::Mutex::Autolock autoLock(mVideoOffloadPlayer->mLock);
        mVideoOffloadPlayer->mSeekTarget.Reset();
        MediaDecoder::SeekResolveValue val(mVideoOffloadPlayer->mReachedEOS, mVideoOffloadPlayer->mSeekTarget.mEventVisibility);
        mVideoOffloadPlayer->mSeekPromise.Resolve(val, __func__);
      }
      break;
    case android::MEDIA_ERROR:
      VIDEO_OFFLOAD_LOG(LogLevel::Debug, ("%s, line:%d MEDIA_ERROR, type:%d, code:%d, Notify Tear down event", __func__, __LINE__, ext1, ext2));
      mVideoOffloadPlayer->NotifyVideoTearDown();
      break;
    default:
      VIDEO_OFFLOAD_LOG(LogLevel::Debug, ("%s, line:%d, unhandled mediaplayer event:%d", __func__, __LINE__, msg));
  }
}

VideoOffloadPlayer::VideoOffloadPlayer(MediaOmxCommonDecoder* aObserver) :
  MediaOffloadPlayerBase(aObserver),
  mStarted(false),
  mPlaying(false),
  mReachedEOS(false),
  mPositionTimeMediaUs(-1),
  mFD(0)
{
  MOZ_ASSERT(NS_IsMainThread());

  CHECK(aObserver);
}

VideoOffloadPlayer::~VideoOffloadPlayer()
{
  VIDEO_OFFLOAD_LOG(LogLevel::Debug, ("%s, line:%d", __func__, __LINE__));
  Reset();
}

void VideoOffloadPlayer::SetFD(int aFD)
{
  mFD = aFD;
}

void VideoOffloadPlayer::SetGraphicBufferProducer(android::sp<android::IGraphicBufferProducer>& aProducer)
{
  VIDEO_OFFLOAD_LOG(LogLevel::Debug, ("%s, Producer:%p, line:%d", __func__, aProducer.get(), __LINE__));
  mProducer = aProducer;
  
  if (mMediaPlayer != nullptr) {
    mMediaPlayer->setVideoSurfaceTexture(mProducer);
  }
}

status_t VideoOffloadPlayer::Start(bool aSourceAlreadyStarted)
{
  VIDEO_OFFLOAD_LOG(LogLevel::Debug, ("%s, line:%d", __func__, __LINE__));
  MOZ_ASSERT(NS_IsMainThread());
  CHECK(!mStarted);

  status_t err = OK;

  err = PrepareMediaPlayer();

  if (err == OK) {
    mStarted = true;
    mPlaying = false;
  }

  return err;
}

status_t VideoOffloadPlayer::ChangeState(MediaDecoder::PlayState aState)
{
  MOZ_ASSERT(NS_IsMainThread());
  mPlayState = aState;
  VIDEO_OFFLOAD_LOG(LogLevel::Debug, ("%s, state:%d", __func__, mPlayState));
  
  switch (mPlayState) {
    case MediaDecoder::PLAY_STATE_PLAYING: {
      status_t err = Play();
      if (err != OK) {
        return err;
      }
      StartTimeUpdate();
    } 
      break;

    case MediaDecoder::PLAY_STATE_PAUSED:
    case MediaDecoder::PLAY_STATE_SHUTDOWN:
    case MediaDecoder::PLAY_STATE_ENDED:
      Pause();
      break;

    default:
      break;
  }
  return OK;
}

void VideoOffloadPlayer::Pause()
{
  VIDEO_OFFLOAD_LOG(LogLevel::Debug, ("%s, line:%d", __func__, __LINE__));
  MOZ_ASSERT(NS_IsMainThread());

  if (mPlaying) {
    CHECK(mMediaPlayer.get());
    WakeLockCreate();
    VIDEO_OFFLOAD_LOG(LogLevel::Debug, ("%s, line:%d, Pause", __func__, __LINE__));
    mMediaPlayer->pause();
    mPlaying = false;
  }
}

status_t VideoOffloadPlayer::Play()
{
  VIDEO_OFFLOAD_LOG(LogLevel::Debug, ("%s, line:%d", __func__, __LINE__));
  MOZ_ASSERT(NS_IsMainThread());

  status_t err = OK;

  if (!mStarted) {
    err = Start();
    if (err != OK) {
      return err;
    }
    // Seek to last play position only when there was no seek during last pause
    android::Mutex::Autolock autoLock(mLock);
    if (!mSeekTarget.IsValid()) {
      mSeekTarget = SeekTarget(mPositionTimeMediaUs,
                               SeekTarget::Accurate,
                               MediaDecoderEventVisibility::Suppressed);
      DoSeek();
    }
  }

  if (!mPlaying) {
    CHECK(mMediaPlayer.get());
    err = mMediaPlayer->start();
    if (err == OK) {
      mPlaying = true;
    }
  }

  return err;
}

void VideoOffloadPlayer::Reset()
{
  MOZ_ASSERT(NS_IsMainThread());
  if (!mStarted) {
    return;
  }

  CHECK(mMediaPlayer.get());

  VIDEO_OFFLOAD_LOG(LogLevel::Debug, ("reset: mPlaying=%d mReachedEOS=%d",
      mPlaying, mReachedEOS));

  //Detach Surface from MediaPlayer to keep last frame in Surface.
  mMediaPlayer->setVideoSurfaceTexture(nullptr);

  mMediaPlayer->reset();

  IPCThreadState::self()->flushCommands();
  StopTimeUpdate();

  mReachedEOS = false;
  mStarted = false;
  mPlaying = false;

  WakeLockRelease();
}

RefPtr<MediaDecoder::SeekPromise> VideoOffloadPlayer::Seek(SeekTarget aTarget)
{
  MOZ_ASSERT(NS_IsMainThread());
  android::Mutex::Autolock autoLock(mLock);

  mSeekPromise.RejectIfExists(true, __func__);
  mSeekTarget = aTarget;
  RefPtr<MediaDecoder::SeekPromise> p = mSeekPromise.Ensure(__func__);
  DoSeek();
  return p;
}

status_t VideoOffloadPlayer::DoSeek()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mSeekTarget.IsValid());
  CHECK(mMediaPlayer.get());

  VIDEO_OFFLOAD_LOG(LogLevel::Debug,
                    ("DoSeek ( %lld )", mSeekTarget.GetTime().ToMicroseconds()));

  mReachedEOS = false;
  mPositionTimeMediaUs = -1;

  if (!mSeekPromise.IsEmpty()) {
    nsCOMPtr<nsIRunnable> nsEvent =
      NS_NewRunnableMethodWithArg<MediaDecoderEventVisibility>(
        mObserver,
        &MediaDecoder::SeekingStarted,
        mSeekTarget.mEventVisibility);
    NS_DispatchToCurrentThread(nsEvent);
  }

  mMediaPlayer->seekTo(((int)mSeekTarget.GetTime().ToMicroseconds())/1000);

  return OK;
}

int64_t VideoOffloadPlayer::GetMediaTimeUs()
{
  android::Mutex::Autolock autoLock(mLock);

  int64_t playPosition = 0;
  if (mSeekTarget.IsValid()) {
    return mSeekTarget.GetTime().ToMicroseconds();
  }
  if (!mStarted) {
    return mPositionTimeMediaUs;
  }

  playPosition = GetOutputPlayPositionUs_l();
  if (!mReachedEOS) {
    mPositionTimeMediaUs = playPosition;
  } else {
    int durationInMs = 0;
    mMediaPlayer->getDuration(&durationInMs);
    mPositionTimeMediaUs = durationInMs*1000;
  }

  return mPositionTimeMediaUs;
}

int64_t VideoOffloadPlayer::GetOutputPlayPositionUs_l() const
{
  CHECK(mMediaPlayer.get());
  int currentPosition = 0;
  mMediaPlayer->getCurrentPosition(&currentPosition);
  int64_t result = (int64_t)currentPosition * 1000;
  return result;
}

void VideoOffloadPlayer::NotifyVideoEOS()
{
  android::Mutex::Autolock autoLock(mLock);
  // We do not reset mSeekTarget here.
  if (!mSeekPromise.IsEmpty()) {
    MediaDecoder::SeekResolveValue val(mReachedEOS, mSeekTarget.mEventVisibility);
    mSeekPromise.Resolve(val, __func__);
  }
  nsCOMPtr<nsIRunnable> nsEvent = NS_NewRunnableMethod(mObserver,
      &MediaDecoder::PlaybackEnded);
  NS_DispatchToMainThread(nsEvent);
}

void VideoOffloadPlayer::NotifyVideoTearDown()
{
  // Fallback to state machine.
  // state machine's seeks will be done with
  // MediaDecoderEventVisibility::Suppressed.
  android::Mutex::Autolock autoLock(mLock);
  // We do not reset mSeekTarget here.
  if (!mSeekPromise.IsEmpty()) {
    MediaDecoder::SeekResolveValue val(mReachedEOS, mSeekTarget.mEventVisibility);
    mSeekPromise.Resolve(val, __func__);
  }
  nsCOMPtr<nsIRunnable> nsEvent = NS_NewRunnableMethod(mObserver,
      &MediaOmxCommonDecoder::MediaOffloadTearDown);
  NS_DispatchToMainThread(nsEvent);
}

MediaDecoderOwner::NextFrameStatus VideoOffloadPlayer::GetNextFrameStatus()
{
  MOZ_ASSERT(NS_IsMainThread());
  if (mSeekTarget.IsValid()) {
    return MediaDecoderOwner::NEXT_FRAME_UNAVAILABLE_SEEKING;
  } else if (mPlayState == MediaDecoder::PLAY_STATE_ENDED) {
    return MediaDecoderOwner::NEXT_FRAME_UNAVAILABLE;
  } else {
    return MediaDecoderOwner::NEXT_FRAME_AVAILABLE;
  }
}

void VideoOffloadPlayer::SetVolume(double aVolume)
{
  MOZ_ASSERT(NS_IsMainThread());
  CHECK(mMediaPlayer.get());
  mMediaPlayer->setVolume((float) aVolume, (float) aVolume);
}

status_t VideoOffloadPlayer::PrepareMediaPlayer()
{
  VIDEO_OFFLOAD_LOG(LogLevel::Info, ("%s, Using VideoOffloadPlayer.", __func__));

  status_t err = OK;

  if (mFD > 0) {
    VIDEO_OFFLOAD_LOG(LogLevel::Debug, ("%s, FD from HTMLMediaElement, mFD:%d, line:%d", __func__, mFD, __LINE__));
  } else {
    VIDEO_OFFLOAD_LOG(LogLevel::Error, ("Can't use MediaPlayer without FD."));
    err = UNKNOWN_ERROR;
    return err;
  }

  size_t size = lseek(mFD, 0, SEEK_END);
  lseek(mFD, 0, SEEK_SET);

  if (mMediaPlayer.get() == nullptr) {
    sp<IServiceManager> sm = defaultServiceManager();
    sp<IBinder> binder = sm->getService(String16("media.player"));
    sp<IMediaPlayerService> service = interface_cast<IMediaPlayerService>(binder);
  
    CHECK(service.get() != nullptr);
   
    mMediaPlayer = new MediaPlayer();
    mMediaPlayerListener = new AndroidMediaPlayerListener(this);
    mMediaPlayer->setListener(mMediaPlayerListener);
    mIMediaPlayer =
#if ANDROID_VERSION == 19
        service->create(mMediaPlayer, 0);
#else
        service->create(mMediaPlayer, AUDIO_SESSION_ALLOCATE);
#endif

  }

  if (mMediaPlayer == nullptr)
    VIDEO_OFFLOAD_LOG(LogLevel::Error, ("mMediaPlayer == nullptr"));
  if (mProducer == nullptr)
    VIDEO_OFFLOAD_LOG(LogLevel::Error, ("mProducer == nullptr"));

  if (mMediaPlayer != nullptr && 
      mMediaPlayer->setDataSource(mFD, 0, size) == NO_ERROR &&
      mProducer != nullptr) {
    err = mMediaPlayer->setVideoSurfaceTexture(mProducer);
    VIDEO_OFFLOAD_LOG(LogLevel::Debug, ("%s, MediaPlayer->setVideoSurfaceTexture line:%d, err:%d", __func__, __LINE__, err));
    err = mMediaPlayer->prepare();
    VIDEO_OFFLOAD_LOG(LogLevel::Debug, ("%s, MediaPlayer->prepare() line:%d, err:%d", __func__, __LINE__, err));
  } else {
    VIDEO_OFFLOAD_LOG(LogLevel::Error, ("failed to instantiate player."));
    err = UNKNOWN_ERROR;
  }
  return err;
}

void VideoOffloadPlayer::SetPlaybackRate(double aPlaybackRate)
{
  VIDEO_OFFLOAD_LOG(LogLevel::Debug, ("%s, line:%d, aPlaybackRate:%lf", __func__, __LINE__, aPlaybackRate));
  if (mMediaPlayer != nullptr) {
#if ANDROID_VERSION == 19
    Parcel rate;
    rate.writeDouble(aPlaybackRate);
    mMediaPlayer->setParameter(KEY_PARAMETER_PLAYBACK_RATE_PERMILLE, rate);
#else
    AudioPlaybackRate rate;
    mMediaPlayer->getPlaybackSettings(&rate);
    rate.mSpeed = aPlaybackRate;
    mMediaPlayer->setPlaybackSettings(rate);
#endif
  }
}

} // namespace mozilla
