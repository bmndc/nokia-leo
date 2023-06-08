/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "WebrtcOMXVP8VideoCodec.h"

#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>

#include "WebrtcMediaCodecWrapper.h"

using namespace android;

namespace mozilla {

// Decoder.
WebrtcOMXVP8VideoDecoder::WebrtcOMXVP8VideoDecoder()
  : mCallback(nullptr)
  , mOMX(nullptr)
{
  mReservation = new OMXCodecReservation(false);
  CODEC_LOGD("WebrtcOMXVP8VideoDecoder:%p will be constructed", this);
}

int32_t
WebrtcOMXVP8VideoDecoder::InitDecode(const webrtc::VideoCodec* aCodecSettings,
                                      int32_t aNumOfCores)
{
  CODEC_LOGD("WebrtcOMXVP8VideoDecoder:%p init OMX:%p", this, mOMX.get());

  if (!mReservation->ReserveOMXCodec()) {
    CODEC_LOGD("WebrtcOMXVP8VideoDecoder:%p Decoder in use", this);
    return WEBRTC_VIDEO_CODEC_ERROR;
  }

  // Defer configuration until first frame are received.

  CODEC_LOGD("WebrtcOMXVP8VideoDecoder:%p OMX Decoder reserved", this);
  return WEBRTC_VIDEO_CODEC_OK;
}

/* static */ int32_t
WebrtcOMXVP8VideoDecoder::ExtractPicDimensions(uint8_t* aData, size_t aSize,
                                               int32_t* aWidth, int32_t* aHeight)
{
  if (aSize < 10) {
    return WEBRTC_VIDEO_CODEC_ERROR;
  }
  // Cannot get dimension from delta frames
  if (aData[0] & 0x01) {
    return WEBRTC_VIDEO_CODEC_ERROR;
  }
  *aWidth = ((aData[7] << 8) + aData[6]) & 0x3fff;
  *aHeight = ((aData[9] << 8) + aData[8]) & 0x3fff;
  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t
WebrtcOMXVP8VideoDecoder::Decode(const webrtc::EncodedImage& aInputImage,
                                 bool aMissingFrames,
                                 const webrtc::RTPFragmentationHeader* aFragmentation,
                                 const webrtc::CodecSpecificInfo* aCodecSpecificInfo,
                                 int64_t aRenderTimeMs)
{
  if (aInputImage._length== 0 || !aInputImage._buffer) {
    return WEBRTC_VIDEO_CODEC_ERROR;
  }

  bool configured = !!mOMX;
  if (!configured) {
    int32_t width;
    int32_t height;
    int32_t result = ExtractPicDimensions(aInputImage._buffer,
                                          aInputImage._length,
                                          &width, &height);
    if (result != WEBRTC_VIDEO_CODEC_OK) {
      // Cannot config decoder because key frame hasn't been seen.
      CODEC_LOGI("WebrtcOMXVP8VideoDecoder:%p skipping delta frame", this);
      return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }
    RefPtr<WebrtcOMXDecoder> omx = new WebrtcOMXDecoder(MEDIA_MIMETYPE_VIDEO_VP8,
                                                        mCallback);
    result = omx->ConfigureWithPicDimensions(width, height);
    if (NS_WARN_IF(result != OK)) {
      return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }
    CODEC_LOGD("WebrtcOMXVP8VideoDecoder:%p start OMX", this);
    mOMX = omx;
  }

  status_t err = mOMX->FillInput(aInputImage, false, aRenderTimeMs);
  if (NS_WARN_IF(err != OK)) {
    return WEBRTC_VIDEO_CODEC_ERROR;
  }
  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t
WebrtcOMXVP8VideoDecoder::RegisterDecodeCompleteCallback(webrtc::DecodedImageCallback* aCallback)
{
  CODEC_LOGD("WebrtcOMXVP8VideoDecoder:%p set callback:%p", this, aCallback);
  MOZ_ASSERT(aCallback);
  mCallback = aCallback;

  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t
WebrtcOMXVP8VideoDecoder::Release()
{
  CODEC_LOGD("WebrtcOMXVP8VideoDecoder:%p will be released", this);

  mOMX = nullptr; // calls Stop()
  mReservation->ReleaseOMXCodec();

  return WEBRTC_VIDEO_CODEC_OK;
}

WebrtcOMXVP8VideoDecoder::~WebrtcOMXVP8VideoDecoder()
{
  CODEC_LOGD("WebrtcOMXVP8VideoDecoder:%p will be destructed", this);
  Release();
}

int32_t
WebrtcOMXVP8VideoDecoder::Reset()
{
  CODEC_LOGW("WebrtcOMXVP8VideoDecoder::Reset() will NOT reset decoder");
  return WEBRTC_VIDEO_CODEC_OK;
}

} // namespace mozilla
