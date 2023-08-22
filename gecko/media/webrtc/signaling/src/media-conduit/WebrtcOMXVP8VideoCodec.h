/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef WEBRTC_OMX_VP8_VIDEO_CODEC_H_
#define WEBRTC_OMX_VP8_VIDEO_CODEC_H_

#include <utils/RefBase.h>
#include "OMXCodecWrapper.h"
#include "VideoConduit.h"

namespace mozilla {

class WebrtcOMXDecoder;

class WebrtcOMXVP8VideoDecoder : public WebrtcVideoDecoder
{
public:
  WebrtcOMXVP8VideoDecoder();

  virtual ~WebrtcOMXVP8VideoDecoder();

  // Implement VideoDecoder interface.
  virtual uint64_t PluginID() const override { return 0; }

  virtual int32_t InitDecode(const webrtc::VideoCodec* aCodecSettings,
                             int32_t aNumOfCores) override;
  virtual int32_t Decode(const webrtc::EncodedImage& aInputImage,
                         bool aMissingFrames,
                         const webrtc::RTPFragmentationHeader* aFragmentation,
                         const webrtc::CodecSpecificInfo* aCodecSpecificInfo = nullptr,
                         int64_t aRenderTimeMs = -1) override;
  virtual int32_t RegisterDecodeCompleteCallback(webrtc::DecodedImageCallback* callback) override;

  virtual int32_t Release() override;

  virtual int32_t Reset() override;

private:
  static int32_t ExtractPicDimensions(uint8_t* aData, size_t aSize,
                                      int32_t* aWidth, int32_t* aHeight);

  webrtc::DecodedImageCallback* mCallback;
  RefPtr<WebrtcOMXDecoder> mOMX;
  android::sp<android::OMXCodecReservation> mReservation;
};

} // namespace mozilla

#endif // WEBRTC_OMX_VP8_VIDEO_CODEC_H_
