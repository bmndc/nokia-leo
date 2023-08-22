/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "MediaOmxCommonReader.h"

#include <stagefright/MediaSource.h>

#include "AbstractMediaDecoder.h"
#include "AudioChannelService.h"
#include "MediaStreamSource.h"
#include "gfxPrefs.h"

#ifdef MOZ_AUDIO_OFFLOAD
#include <stagefright/Utils.h>
#include <cutils/properties.h>
#endif

#include "DecoderTraits.h"
#include "mozilla/dom/HTMLMediaElement.h"
#include "MediaOmxCommonDecoder.h"

using namespace android;

namespace mozilla {

extern LazyLogModule gMediaDecoderLog;
#define DECODER_LOG(type, msg) MOZ_LOG(gMediaDecoderLog, type, msg)

MediaOmxCommonReader::MediaOmxCommonReader(AbstractMediaDecoder *aDecoder)
  : MediaDecoderReader(aDecoder)
  , mStreamSource(nullptr)
{
  mAudioChannel = dom::AudioChannelService::GetDefaultAudioChannel();
}

bool MediaOmxCommonReader::IsMonoAudioEnabled()
{
  return gfxPrefs::MonoAudio();
}

#ifdef MOZ_AUDIO_OFFLOAD
void MediaOmxCommonReader::CheckAudioOffload()
{
  MOZ_ASSERT(OnTaskQueue());

  char offloadProp[128];
  property_get("audio.offload.disable", offloadProp, "0");
  bool offloadDisable =  atoi(offloadProp) != 0;
  if (offloadDisable) {
    return;
  }

  sp<MediaSource> audioOffloadTrack = GetAudioOffloadTrack();
  sp<MetaData> meta = audioOffloadTrack.get()
      ? audioOffloadTrack->getFormat() : nullptr;

  if (!meta.get()) {
    return;
  }

  // Supporting audio offload only when there is no video, no streaming
  bool hasVideo = HasVideo();
  bool isStreaming
      = !mDecoder->GetResource()->IsDataCachedToEndOfResource(0);

  if (hasVideo || isStreaming) {
    return;
  }

  // Not much benefit in trying to offload other channel types. Most of them
  // aren't supported and also duration would be less than a minute
  bool isTypeMusic = mAudioChannel == dom::AudioChannel::Content;
  if (!isTypeMusic) {
    return;
  }

  //Check if we need to adjust VolumeBalance.
  if (gfxPrefs::VolumeBalance() != 50) {
    return;
  }

  if (IsMonoAudioEnabled()) {
    return;
  }

  if (!canOffloadStream(meta, false, false, AUDIO_STREAM_MUSIC)) {
    return;
  }

  DECODER_LOG(LogLevel::Debug, ("Can offload this audio stream"));
  mDecoder->SetPlatformCanOffloadMedia(AbstractMediaDecoder::OFFLOAD_TYPE_AUDIO, true);
}
#endif

bool MediaOmxCommonReader::CheckVideoOffload()
{
  MOZ_ASSERT(OnTaskQueue());

  char offloadProp[128];
  property_get("media.disableVideoOffload.on", offloadProp, "0");
  bool offloadDisable =  atoi(offloadProp) != 0;
  if (offloadDisable) {
    return false;
  }


  MediaOmxCommonDecoder* mediaplayerDecoder = static_cast<MediaOmxCommonDecoder*>(mDecoder);

  if (!mediaplayerDecoder) {
    return false;
  }

  if (mediaplayerDecoder->GetFD() <= 0) {
    DECODER_LOG(LogLevel::Debug, ("Can't offload this video without FD"));
    return false;
  }

  if(mediaplayerDecoder->IsNoVideoOffloadDueToNoBlobSource()) {
    DECODER_LOG(LogLevel::Debug, ("Can't offload this streaming video stream"));
    return false;
  }

  //Check video container mime type.
  sp<MetaData> meta =  GetContainerMetaData();

  if (meta != nullptr) {
    const char *mime;
    meta->findCString(kKeyMIMEType, &mime);
    nsCString nsMime(mime);
    if (!DecoderTraits::IsVideoOffloadPlayerSupportedType(nsMime)) {
      return false;
    }
  } else {
    return false;
  }

  DECODER_LOG(LogLevel::Debug, ("Can offload this video stream"));
  mDecoder->SetPlatformCanOffloadMedia(AbstractMediaDecoder::OFFLOAD_TYPE_VIDEO, true);
  return true;
}
} // namespace mozilla
