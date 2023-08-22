/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* (c) 2018 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */


#ifndef VIDEO_OFFLOAD_PLAYER_H_
#define VIDEO_OFFLOAD_PLAYER_H_

#include <stagefright/TimeSource.h>
#include <media/IMediaPlayerService.h>

#include <stagefright/Utils.h>

#include "MediaOffloadPlayerBase.h"
#include "MediaDecoderOwner.h"

namespace mozilla {

namespace dom {
class WakeLock;
}

class VideoOffloadPlayer;
//The definition of msg, ext1 and ext2 are in KaiOS/frameworks/av/include/media/mediaplayer.h
class AndroidMediaPlayerListener : public android::MediaPlayerListener
{
  public:
    explicit AndroidMediaPlayerListener(VideoOffloadPlayer* aOwener);
    virtual void notify(int msg, int ext1, int ext2, const android::Parcel *obj);
  private:
    const char* MediaEventTypeToStr(int aEventType) const;

    AndroidMediaPlayerListener();
    VideoOffloadPlayer* mVideoOffloadPlayer;
};

/**
 * VideoOffloadPlayer adds support for video playback which using Android MediaPlayer,
 * in order to use less battery.
 *
 * Current VideoOffloadPlayer supports only local video playback with file descriptor.
 *
 *
 * It acts as a bridge between MediaOmxCommonDecoder and Android MediaPlayer during
 * offload playback
 */

class VideoOffloadPlayer : public MediaOffloadPlayerBase
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(VideoOffloadPlayer)

  typedef android::MetaData MetaData;
  typedef android::status_t status_t;

  friend class AndroidMediaPlayerListener;

public:
  enum {
    REACHED_EOS,
    SEEK_COMPLETE
  };

  VideoOffloadPlayer(MediaOmxCommonDecoder* aDecoder = nullptr);

  // Caller retains ownership of "aFD".
  void SetFD(int aFD) override;
  void SetGraphicBufferProducer(android::sp<android::IGraphicBufferProducer>& aProducer) override;

  // Create and prepare the Android MediaPlayer.
  status_t Start(bool aSourceAlreadyStarted = false) override;

  status_t ChangeState(MediaDecoder::PlayState aState) override;

  void SetVolume(double aVolume) override;

  int64_t GetMediaTimeUs() override;

  // Update ready state based on current play state.
  MediaDecoderOwner::NextFrameStatus GetNextFrameStatus() override;

  RefPtr<MediaDecoder::SeekPromise> Seek(SeekTarget aTarget) override;

  // Reset Android MediaPlayer, stop time updates
  void Reset();

  virtual void SetPlaybackRate(double aPlaybackRate);

private:
  // Set when Android MediaPlayer is prepared.
  bool mStarted;

  // Set when Android MediaPlayer is started. i.e. playback started
  bool mPlaying;

  // Indicate the playback end.
  bool mReachedEOS;

  // The target of current seek when there is a request to seek
  // Used in main thread protected by Mutex
  // mLock
  SeekTarget mSeekTarget;

  // MozPromise of current seek.
  MozPromiseHolder<MediaDecoder::SeekPromise> mSeekPromise;

  // Positions obtained from offlaoded tracks
  // Used in main thread and offload callback thread, protected by Mutex
  // mLock
  int64_t mPositionTimeMediaUs;

  int mFD;

  // Provide the playback position in microseconds.
  int64_t GetOutputPlayPositionUs_l() const;

  bool IsSeeking();

  // Set mSeekTarget to the given position and ask MediaPlayer to seek.
  status_t DoSeek();

  // Start/Resume the Android MediaPlayer.
  status_t Play();

  // Pause the MediaPlayer in case we should stop playing immediately
  void Pause();

  // Notify end of stream by sending PlaybackEnded event to observer
  // (i.e.MediaDecoder)
  void NotifyVideoEOS();

  // Offloaded video is invalidated due to errors. Notify
  // MediaDecoder to re-evaluate offloading options
  void NotifyVideoTearDown();

  status_t PrepareMediaPlayer();
  android::sp<android::IGraphicBufferProducer> mProducer;
  android::sp<android::IMediaPlayer> mIMediaPlayer;
  android::sp<android::MediaPlayer> mMediaPlayer;
  android::sp<android::MediaPlayerListener> mMediaPlayerListener;

  VideoOffloadPlayer(const VideoOffloadPlayer &);
  VideoOffloadPlayer &operator=(const VideoOffloadPlayer &);

  ~VideoOffloadPlayer();
};

} // namespace mozilla

#endif  // VIDEO_OFFLOAD_PLAYER_H_
