/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* (c) 2018 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#ifndef MEDIA_OFFLOAD_PLAYER_BASE_H_
#define MEDIA_OFFLOAD_PLAYER_BASE_H_

#include <gui/IGraphicBufferProducer.h>
#include <media/stagefright/MediaSource.h>
#include <utils/threads.h>
#include <utils/RefBase.h>

#include "mozilla/dom/WakeLock.h"
#include "MediaDecoder.h"
#include "MediaDecoderOwner.h"
#include "MediaOmxCommonDecoder.h"

namespace mozilla {

namespace dom {
class WakeLock;
}

/**
 * MediaOffloadPlayerBase class provide the command base of offload players.
 * Such as AudioOffloadPlayer and VideoOffloadPlayer.
 */
class MediaOffloadPlayerBase
{
  typedef android::Mutex Mutex;
  typedef android::status_t status_t;
  typedef android::MediaSource MediaSource;

public:
  MediaOffloadPlayerBase(MediaOmxCommonDecoder* aObserver);
  virtual ~MediaOffloadPlayerBase() {};

  // Caller retains ownership of "aSource".
  virtual void SetSource(const android::sp<MediaSource> &aSource) {}

  // For Audio:
  // Start the source if it's not already started and open the AudioSink to
  // create an offloaded audio track
  // For Video:
  // Create and prepare the Android MediaPlayer.
  virtual status_t Start(bool aSourceAlreadyStarted = false)
  {
    return android::NO_INIT;
  }

  virtual status_t ChangeState(MediaDecoder::PlayState aState)
  {
    return android::NO_INIT;
  }

  virtual void SetVolume(double aVolume) {}

  virtual int64_t GetMediaTimeUs() { return 0; }

  // To update progress bar when the element is visible
  virtual void SetElementVisibility(bool aIsVisible);

  // Update ready state based on current play state.
  virtual MediaDecoderOwner::NextFrameStatus GetNextFrameStatus()
  {
    return MediaDecoderOwner::NEXT_FRAME_UNAVAILABLE;
  }

  virtual RefPtr<MediaDecoder::SeekPromise> Seek(SeekTarget aTarget) = 0;

  void TimeUpdate();

  // Caller retains ownership of "fd".
  virtual void SetFD(int aFD) {}
  virtual void SetGraphicBufferProducer(android::sp<android::IGraphicBufferProducer>& aProducer) {}
  virtual void SetPlaybackRate(double aPlaybackRate) {};

protected:

  // Audio: 
  // When audio is offloaded, application processor wakes up less frequently
  // (>1sec) But when Player UI is visible we need to update progress bar
  // atleast once in 250ms. Start a timer when player UI becomes visible or
  // audio starts playing to send UpdateLogicalPosition events once in 250ms.
  // Stop the timer when UI goes invisible or play state is not playing.
  // Also make sure timer functions are always called from main thread
  //
  // Video:
  // Start a timer when player UI becomes visible or
  // MediaPlayer starts playing to send UpdateLogicalPosition events once in 250ms.
  // Stop the timer when UI goes invisible or play state is not playing.
  // Also make sure timer functions are always called from main thread
  nsresult StartTimeUpdate();
  nsresult StopTimeUpdate();

  void WakeLockCreate();
  void WakeLockRelease();

  // Notify position changed event by sending UpdateLogicalPosition event to
  // observer
  void NotifyPositionChanged();

  // Protect accessing variables between main thread and
  // callback thread
  Mutex mLock;

  // Set when the HTML Media Element is visible to the user.
  // Used only in main thread
  bool mIsElementVisible;

  //Audio:
  //MediaOmxCommonDecoder object used mainly to notify the audio sink status
  //Video: 
  //MediaOmxCommonDecoder object used mainly to notify the MediaPlayer status
  MediaOmxCommonDecoder* mObserver;

  // State obtained from MediaOmxCommonDecoder. Used only in main thread
  MediaDecoder::PlayState mPlayState;

  TimeStamp mLastFireUpdateTime;
  // Timer to trigger position changed events
  nsCOMPtr<nsITimer> mTimeUpdateTimer;
  // To avoid device suspend when mResetTimer is going to be triggered.
  // Used only from main thread so no lock is needed.
  RefPtr<mozilla::dom::WakeLock> mWakeLock;
};

} // namespace mozilla

#endif // MEDIA_OFFLOAD_PLAYER_BASE_H_
