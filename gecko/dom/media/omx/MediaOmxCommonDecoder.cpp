/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "MediaOmxCommonDecoder.h"

#include <stagefright/MediaSource.h>

#include "MediaOffloadPlayerBase.h"
#include "MediaDecoderStateMachine.h"
#include "MediaOmxCommonReader.h"

#ifdef MOZ_AUDIO_OFFLOAD
#include "AudioOffloadPlayer.h"
#endif
#include "VideoOffloadPlayer.h"
using namespace android;

namespace mozilla {

extern LazyLogModule gMediaDecoderLog;
#define DECODER_LOG(type, msg) MOZ_LOG(gMediaDecoderLog, type, msg)

MediaOmxCommonDecoder::MediaOmxCommonDecoder(MediaDecoderOwner* aOwner)
  : MediaDecoder(aOwner)
  , mReader(nullptr)
  , mCanOffloadAudio(false)
  , mCanOffloadVideo(false)
  , mFallbackToStateMachine(false)
  , mIsCaptured(false)
  , mFD(0)
  , mNoVideoOffloadDueToNoBlobSource(true)
{
  mDormantSupported = true;
}

MediaOmxCommonDecoder::~MediaOmxCommonDecoder() {}

void 
MediaOmxCommonDecoder::SetPlatformCanOffloadMedia(OffloadMediaType aOffloadMediaType,
                                                  bool aCanOffloadMedia)
{
  MOZ_ASSERT((aOffloadMediaType == OFFLOAD_TYPE_AUDIO) || 
             (aOffloadMediaType == OFFLOAD_TYPE_VIDEO));

  if (!aCanOffloadMedia) {
    return;
  }

  // Stop MDSM from playing to avoid startup glitch (bug 1053186).
  GetStateMachine()->DispatchMediaOffloading(true);

  // Modify mCanOffloadAudio or mCanOffloadVideo in the main thread.
  RefPtr<MediaOmxCommonDecoder> self = this;
  nsCOMPtr<nsIRunnable> r = NS_NewRunnableFunction([=] () {
    if (aOffloadMediaType == OFFLOAD_TYPE_AUDIO) {
      self->mCanOffloadAudio = true;
    } else {
      self->mCanOffloadVideo = true;
    }
  });
  AbstractThread::MainThread()->Dispatch(r.forget());
}

void
MediaOmxCommonDecoder::DisableStateMachineMediaOffloading()
{
  MOZ_ASSERT(NS_IsMainThread());
  if (mCanOffloadAudio || mCanOffloadVideo) {
    // mCanOffloadAudio or mCanOffloadVideo is true implies we've called
    // |GetStateMachine()->DispatchMediaOffloading(true)| in
    // SetPlatformCanOffloadMedia(). We need to turn off media offloading
    // for MDSM so it can start playback.
    GetStateMachine()->DispatchMediaOffloading(false);
  }
}

bool
MediaOmxCommonDecoder::CheckDecoderCanOffloadMedia(OffloadMediaType aOffloadMediaType)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT((aOffloadMediaType == OFFLOAD_TYPE_AUDIO) || 
             (aOffloadMediaType == OFFLOAD_TYPE_VIDEO));

  return (((aOffloadMediaType == OFFLOAD_TYPE_AUDIO) ? mCanOffloadAudio : mCanOffloadVideo) && 
          !mFallbackToStateMachine &&
          !mIsCaptured && 
          ((aOffloadMediaType == OFFLOAD_TYPE_AUDIO) ? (mPlaybackRate == 1.0) : true));
}

void
MediaOmxCommonDecoder::FirstFrameLoaded(nsAutoPtr<MediaInfo> aInfo,
                                        MediaDecoderEventVisibility aEventVisibility)
{
  MOZ_ASSERT(NS_IsMainThread());

  MediaDecoder::FirstFrameLoaded(aInfo, aEventVisibility);

  bool offloadVideo = CheckDecoderCanOffloadMedia(OFFLOAD_TYPE_VIDEO);

  if (!CheckDecoderCanOffloadMedia(OFFLOAD_TYPE_AUDIO) && !offloadVideo) {
    DECODER_LOG(LogLevel::Debug, ("In %s Offload check failed",
        __PRETTY_FUNCTION__));
    DisableStateMachineMediaOffloading();
    return;
  }

  if (!offloadVideo) {
  #ifdef MOZ_AUDIO_OFFLOAD
    mMediaOffloadPlayer = new AudioOffloadPlayer(this);
  #endif
  } else {
    mMediaOffloadPlayer = new VideoOffloadPlayer(this);
  }

  if (!mMediaOffloadPlayer) {
    DisableStateMachineMediaOffloading();
    return;
  }

  if (!offloadVideo) {
    mMediaOffloadPlayer->SetSource(mReader->GetAudioOffloadTrack());
  } else {
    mMediaOffloadPlayer->SetGraphicBufferProducer(mProducer);
    mMediaOffloadPlayer->SetFD(mFD);
  }

  status_t err = mMediaOffloadPlayer->Start(false);
  if (err != OK) {
    mMediaOffloadPlayer = nullptr;
    mFallbackToStateMachine = true;
    DECODER_LOG(LogLevel::Debug, ("In %s Unable to start offload audio %d."
      "Switching to normal mode", __PRETTY_FUNCTION__, err));
    DisableStateMachineMediaOffloading();
    return;
  }
  
  PauseStateMachine();
  if (mLogicallySeeking) {
    SeekTarget target = SeekTarget(media::TimeUnit::FromSeconds(mLogicalPosition).ToMicroseconds(),
                                   SeekTarget::Accurate,
                                   MediaDecoderEventVisibility::Observable);
    mSeekRequest.DisconnectIfExists();
    mSeekRequest.Begin(mMediaOffloadPlayer->Seek(target)
      ->Then(AbstractThread::MainThread(), __func__, static_cast<MediaDecoder*>(this),
             &MediaDecoder::OnSeekResolved, &MediaDecoder::OnSeekRejected));
  }
  // Call ChangeState() to run AudioOffloadPlayer/VideoOffloadPlayer since offload state enabled
  ChangeState(mPlayState);
}

void
MediaOmxCommonDecoder::PauseStateMachine()
{
  MOZ_ASSERT(NS_IsMainThread());
  DECODER_LOG(LogLevel::Debug, ("%s", __PRETTY_FUNCTION__));

  MOZ_ASSERT(GetStateMachine());
  // enter dormant state
  GetStateMachine()->DispatchSetDormant(true);
}

void
MediaOmxCommonDecoder::ResumeStateMachine()
{
  MOZ_ASSERT(NS_IsMainThread());
  DECODER_LOG(LogLevel::Debug, ("%s current time %f", __PRETTY_FUNCTION__, mLogicalPosition));

  if (mShuttingDown) {
    return;
  }

  if (!GetStateMachine()) {
    return;
  }

  GetStateMachine()->DispatchMediaOffloading(false);

  mFallbackToStateMachine = true;
  mMediaOffloadPlayer = nullptr;
  SeekTarget target = SeekTarget(media::TimeUnit::FromSeconds(mLogicalPosition).ToMicroseconds(),
                                 SeekTarget::Accurate,
                                 MediaDecoderEventVisibility::Suppressed);
  // Call Seek of MediaDecoderStateMachine to suppress seek events.
  GetStateMachine()->InvokeSeek(target);

  mNextState = mPlayState;
  ChangeState(PLAY_STATE_LOADING);
  // exit dormant state
  GetStateMachine()->DispatchSetDormant(false);
  UpdateLogicalPosition();
}

void
MediaOmxCommonDecoder::MediaOffloadTearDown()
{
  MOZ_ASSERT(NS_IsMainThread());
  DECODER_LOG(LogLevel::Debug, ("%s", __PRETTY_FUNCTION__));

  // mMediaOffloadPlayer can be null here if ResumeStateMachine was called
  // just before because of some other error.
  if (mMediaOffloadPlayer) {
    ResumeStateMachine();
  }
}

void
MediaOmxCommonDecoder::AddOutputStream(ProcessedMediaStream* aStream,
                                       bool aFinishWhenEnded)
{
  MOZ_ASSERT(NS_IsMainThread());

  mIsCaptured = true;

  if (mMediaOffloadPlayer) {
    ResumeStateMachine();
  }

  MediaDecoder::AddOutputStream(aStream, aFinishWhenEnded);
}

void
MediaOmxCommonDecoder::SetPlaybackRate(double aPlaybackRate)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (mMediaOffloadPlayer &&
      ((aPlaybackRate != 0.0) || (aPlaybackRate != 1.0))) {
    ResumeStateMachine();
  }

  if (mCanOffloadVideo && 
      mMediaOffloadPlayer && 
      (aPlaybackRate != 0.0)) {
    mMediaOffloadPlayer->SetPlaybackRate(aPlaybackRate);
  }

  MediaDecoder::SetPlaybackRate(aPlaybackRate);
}

void
MediaOmxCommonDecoder::ChangeState(PlayState aState)
{
  MOZ_ASSERT(NS_IsMainThread());
  // Keep MediaDecoder state in sync with MediaElement irrespective of offload
  // playback so it will continue to work in normal mode when offloading fails
  // in between
  MediaDecoder::ChangeState(aState);

  if (!mMediaOffloadPlayer) {
    return;
  }

  status_t err = mMediaOffloadPlayer->ChangeState(aState);
  if (err != OK) {
    ResumeStateMachine();
    return;
  }
}

void
MediaOmxCommonDecoder::CallSeek(const SeekTarget& aTarget)
{
  if (!mMediaOffloadPlayer) {
    MediaDecoder::CallSeek(aTarget);
    return;
  }

  mSeekRequest.DisconnectIfExists();
  mSeekRequest.Begin(mMediaOffloadPlayer->Seek(aTarget)
    ->Then(AbstractThread::MainThread(), __func__, static_cast<MediaDecoder*>(this),
            &MediaDecoder::OnSeekResolved, &MediaDecoder::OnSeekRejected));
}

int64_t
MediaOmxCommonDecoder::CurrentPosition()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (mMediaOffloadPlayer) {
    return mMediaOffloadPlayer->GetMediaTimeUs();
  } 

  return MediaDecoder::CurrentPosition();
}

void
MediaOmxCommonDecoder::SetElementVisibility(bool aIsVisible)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (mMediaOffloadPlayer) {
    mMediaOffloadPlayer->SetElementVisibility(aIsVisible);
  }
}

MediaDecoderOwner::NextFrameStatus
MediaOmxCommonDecoder::NextFrameStatus()
{
  MOZ_ASSERT(NS_IsMainThread());
  if (mMediaOffloadPlayer) {
    return mMediaOffloadPlayer->GetNextFrameStatus();
  }

  return MediaDecoder::NextFrameStatus();
}

void
MediaOmxCommonDecoder::SetVolume(double aVolume)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (!mMediaOffloadPlayer) {
    MediaDecoder::SetVolume(aVolume);
    return;
  }

  mMediaOffloadPlayer->SetVolume(aVolume);
}

MediaDecoderStateMachine*
MediaOmxCommonDecoder::CreateStateMachine()
{
  mReader = CreateReader();
  if (mReader != nullptr) {
    mReader->SetAudioChannel(GetAudioChannel());
  }
  return CreateStateMachineFromReader(mReader);
}

void 
MediaOmxCommonDecoder::SetFD(int aFD)
{
  mFD = aFD;
  if (mCanOffloadVideo && mMediaOffloadPlayer) {
    mMediaOffloadPlayer->SetFD(mFD);
  }
}

int 
MediaOmxCommonDecoder::GetFD() const
{
  return mFD;
}

void 
MediaOmxCommonDecoder::SetGraphicBufferProducer(android::sp<android::IGraphicBufferProducer>& aProducer)
{
  mProducer = aProducer;
  if (mCanOffloadVideo && mMediaOffloadPlayer) {
    mMediaOffloadPlayer->SetGraphicBufferProducer(mProducer);
  }
}

void 
MediaOmxCommonDecoder::SetNoVideoOffloadDueToNoBlobSource(bool aNoBlobSource) 
{
  mNoVideoOffloadDueToNoBlobSource = aNoBlobSource;
}
} // namespace mozilla
