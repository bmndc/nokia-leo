/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MEDIA_OMX_COMMON_DECODER_H
#define MEDIA_OMX_COMMON_DECODER_H

#include "MediaDecoder.h"
#include <gui/IGraphicBufferProducer.h>

namespace android {
struct MOZ_EXPORT MediaSource;
} // namespace android

namespace mozilla {

class MediaOffloadPlayerBase;

class MediaOmxCommonReader;

class MediaOmxCommonDecoder : public MediaDecoder
{
public:
  explicit MediaOmxCommonDecoder(MediaDecoderOwner* aOwner);

  void FirstFrameLoaded(nsAutoPtr<MediaInfo> aInfo,
                        MediaDecoderEventVisibility aEventVisibility) override;
  void ChangeState(PlayState aState) override;
  void CallSeek(const SeekTarget& aTarget) override;
  void SetVolume(double aVolume) override;
  int64_t CurrentPosition() override;
  MediaDecoderOwner::NextFrameStatus NextFrameStatus() override;
  void SetElementVisibility(bool aIsVisible) override;

  void SetPlatformCanOffloadMedia(OffloadMediaType aOffloadMediaType, 
                                  bool aCanOffloadMedia) override;
  void AddOutputStream(ProcessedMediaStream* aStream,
                       bool aFinishWhenEnded) override;
  void SetPlaybackRate(double aPlaybackRate) override;

  void MediaOffloadTearDown();

  MediaDecoderStateMachine* CreateStateMachine() override;

  virtual MediaOmxCommonReader* CreateReader() = 0;
  virtual MediaDecoderStateMachine* CreateStateMachineFromReader(MediaOmxCommonReader* aReader) = 0;

  void NotifyOffloadPlayerPositionChanged() { UpdateLogicalPosition(); }
  //For VideoOffloadPlayer
  void SetFD(int aFD);
  int GetFD() const;
  void SetGraphicBufferProducer(android::sp<android::IGraphicBufferProducer>& aProducer);
  void SetNoVideoOffloadDueToNoBlobSource(bool aNoBlobSource);
  bool IsNoVideoOffloadDueToNoBlobSource() { return mNoVideoOffloadDueToNoBlobSource;}
protected:
  virtual ~MediaOmxCommonDecoder();
  void PauseStateMachine();
  void ResumeStateMachine();
  bool CheckDecoderCanOffloadMedia(OffloadMediaType aOffloadType);
  void DisableStateMachineMediaOffloading();

  MediaOmxCommonReader* mReader;

  // Offloaded audio track
  android::sp<android::MediaSource> mAudioTrack;

  nsAutoPtr<MediaOffloadPlayerBase> mMediaOffloadPlayer;

  // Set by Media*Reader to denote current track can be offloaded
  bool mCanOffloadAudio;
  // Set by Media*Reader to denote current track can be offloaded
  bool mCanOffloadVideo;
  // Set when offload playback of current track fails in the middle and need to
  // fallback to state machine
  bool mFallbackToStateMachine;

  // True if the media element is captured.
  bool mIsCaptured;
  int mFD;
  android::sp<android::IGraphicBufferProducer> mProducer;
  bool mNoVideoOffloadDueToNoBlobSource;
};

} // namespace mozilla

#endif // MEDIA_OMX_COMMON_DECODER_H
