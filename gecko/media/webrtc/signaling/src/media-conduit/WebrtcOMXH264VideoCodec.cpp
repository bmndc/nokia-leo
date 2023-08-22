/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "WebrtcOMXH264VideoCodec.h"

#include <avc_utils.h>
#include <foundation/ABuffer.h>
#include <foundation/AMessage.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <OMX_Component.h>

#include "WebrtcMediaCodecWrapper.h"

using namespace android;

#define DRAIN_THREAD_TIMEOUT_US  (1000 * 1000ll) // 1s.

namespace mozilla {

static const uint8_t kNALStartCode[] = { 0x00, 0x00, 0x00, 0x01 };
enum {
  kNALTypeIDR = 5,
  kNALTypeSPS = 7,
  kNALTypePPS = 8,
};

// Assumption: SPS is first paramset or is not present
static bool IsParamSets(uint8_t* aData, size_t aSize)
{
  MOZ_ASSERT(aData && aSize > sizeof(kNALStartCode));
  return (aData[sizeof(kNALStartCode)] & 0x1f) == kNALTypeSPS;
}

// get the length of any pre-pended SPS/PPS's
static size_t ParamSetLength(uint8_t* aData, size_t aSize)
{
  const uint8_t* data = aData;
  size_t size = aSize;
  const uint8_t* nalStart = nullptr;
  size_t nalSize = 0;
  while (getNextNALUnit(&data, &size, &nalStart, &nalSize, true) == OK) {
    if ((*nalStart & 0x1f) != kNALTypeSPS &&
        (*nalStart & 0x1f) != kNALTypePPS) {
      MOZ_ASSERT(nalStart - sizeof(kNALStartCode) >= aData);
      return (nalStart - sizeof(kNALStartCode)) - aData; // SPS/PPS/iframe
    }
  }
  return aSize; // it's only SPS/PPS
}

class EncOutputDrain : public OMXOutputDrain
{
public:
  EncOutputDrain(OMXVideoEncoder* aOMX, webrtc::EncodedImageCallback* aCallback)
    : OMXOutputDrain()
    , mOMX(aOMX)
    , mCallback(aCallback)
    , mIsPrevFrameParamSets(false)
  {}

protected:
  virtual bool DrainOutput() override
  {
    nsTArray<uint8_t> output;
    int64_t timeUs = -1ll;
    int flags = 0;
    nsresult rv = mOMX->GetNextEncodedFrame(&output, &timeUs, &flags,
                                            DRAIN_THREAD_TIMEOUT_US);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      // Fail to get encoded frame. The corresponding input frame should be
      // removed.
      // We'll treat this like a skipped frame
      return true;
    }

    if (output.Length() == 0) {
      // No encoded data yet. Try later.
      CODEC_LOGD("OMX: (encode no output available this time)");
      return false;
    }

    // Conversion to us rounds down, so we need to round up for us->90KHz
    uint32_t target_timestamp = (timeUs * 90ll + 999) / 1000; // us -> 90KHz
    // 8x10 v2.0 encoder doesn't set this reliably:
    //bool isParamSets = (flags & MediaCodec::BUFFER_FLAG_CODECCONFIG);
    // Assume that SPS/PPS will be at the start of any buffer
    // Assume PPS will not be in a separate buffer - SPS/PPS or SPS/PPS/iframe
    bool isParamSets = IsParamSets(output.Elements(), output.Length());
    bool isIFrame = (flags & MediaCodec::BUFFER_FLAG_SYNCFRAME);
    CODEC_LOGD("OMX: encoded frame (%d): time %lld (%u), flags x%x",
               output.Length(), timeUs, target_timestamp, flags);
    // Should not be parameter sets and I-frame at the same time.
    // Except that it is possible, apparently, after an encoder re-config (bug 1063883)
    // MOZ_ASSERT(!(isParamSets && isIFrame));

    if (mCallback) {
      // Implementation here assumes encoder output to be a buffer containing
      // parameter sets(SPS + PPS) followed by a series of buffers, each for
      // one input frame.
      // TODO: handle output violating this assumpton in bug 997110.
      webrtc::EncodedImage encoded(output.Elements(), output.Length(),
                                   output.Capacity());
      encoded._frameType = (isParamSets || isIFrame) ?
                           webrtc::kKeyFrame : webrtc::kDeltaFrame;
      EncodedFrame input_frame;
      {
        MonitorAutoLock lock(mMonitor);
        // will sps/pps have the same timestamp as their iframe? Initial one on 8x10 has
        // 0 timestamp.
        if (isParamSets) {
          // Let's assume it was the first item in the queue, but leave it there since an
          // IDR will follow
          input_frame = mInputFrames.front();
        } else {
          do {
            if (mInputFrames.empty()) {
              // Let's assume it was the last item in the queue, but leave it there
              mInputFrames.push(input_frame);
              CODEC_LOGE("OMX: encoded timestamp %u which doesn't match input queue!! (head %u)",
                         target_timestamp, input_frame.mTimestamp);
              break;
            }

            input_frame = mInputFrames.front();
            mInputFrames.pop();
            if (input_frame.mTimestamp != target_timestamp) {
              CODEC_LOGD("OMX: encoder skipped frame timestamp %u", input_frame.mTimestamp);
            }
          } while (input_frame.mTimestamp != target_timestamp);
        }
      }

      encoded._encodedWidth = input_frame.mWidth;
      encoded._encodedHeight = input_frame.mHeight;
      encoded._timeStamp = input_frame.mTimestamp;
      encoded.capture_time_ms_ = input_frame.mRenderTimeMs;
      encoded._completeFrame = true;

      CODEC_LOGD("Encoded frame: %d bytes, %dx%d, is_param %d, is_iframe %d, timestamp %u, captureTimeMs %" PRIu64,
                 encoded._length, encoded._encodedWidth, encoded._encodedHeight,
                 isParamSets, isIFrame, encoded._timeStamp, encoded.capture_time_ms_);
      // Prepend SPS/PPS to I-frames unless they were sent last time.
      SendEncodedDataToCallback(encoded, isIFrame && !mIsPrevFrameParamSets && !isParamSets);
      // This will be true only for the frame following a paramset block!  So if we're
      // working with a correct encoder that generates SPS/PPS then iframe always, we
      // won't try to insert.  (also, don't set if we get SPS/PPS/iframe in one buffer)
      mIsPrevFrameParamSets = isParamSets && !isIFrame;
      if (isParamSets) {
        // copy off the param sets for inserting later
        mParamSets.Clear();
        // since we may have SPS/PPS or SPS/PPS/iframe
        size_t length = ParamSetLength(encoded._buffer, encoded._length);
        MOZ_ASSERT(length > 0);
        mParamSets.AppendElements(encoded._buffer, length);
      }
    }

    return !isParamSets; // not really needed anymore
  }

private:
  // Send encoded data to callback.The data will be broken into individual NALUs
  // if necessary and sent to callback one by one. This function can also insert
  // SPS/PPS NALUs in front of input data if requested.
  void SendEncodedDataToCallback(webrtc::EncodedImage& aEncodedImage,
                                 bool aPrependParamSets)
  {
    if (aPrependParamSets) {
      webrtc::EncodedImage prepend(aEncodedImage);
      // Insert current parameter sets in front of the input encoded data.
      MOZ_ASSERT(mParamSets.Length() > sizeof(kNALStartCode)); // Start code + ...
      prepend._length = mParamSets.Length();
      prepend._buffer = mParamSets.Elements();
      // Break into NALUs and send.
      CODEC_LOGD("Prepending SPS/PPS: %d bytes, timestamp %u, captureTimeMs %" PRIu64,
                 prepend._length, prepend._timeStamp, prepend.capture_time_ms_);
      SendEncodedDataToCallback(prepend, false);
    }

    struct nal_entry {
      uint32_t offset;
      uint32_t size;
    };
    AutoTArray<nal_entry, 1> nals;

    // Break input encoded data into NALUs and send each one to callback.
    const uint8_t* data = aEncodedImage._buffer;
    size_t size = aEncodedImage._length;
    const uint8_t* nalStart = nullptr;
    size_t nalSize = 0;
    while (getNextNALUnit(&data, &size, &nalStart, &nalSize, true) == OK) {
      // XXX optimize by making buffer an offset
      nal_entry nal = {((uint32_t) (nalStart - aEncodedImage._buffer)), (uint32_t) nalSize};
      nals.AppendElement(nal);
    }

    size_t num_nals = nals.Length();
    if (num_nals > 0) {
      webrtc::RTPFragmentationHeader fragmentation;
      fragmentation.VerifyAndAllocateFragmentationHeader(num_nals);
      for (size_t i = 0; i < num_nals; i++) {
        fragmentation.fragmentationOffset[i] = nals[i].offset;
        fragmentation.fragmentationLength[i] = nals[i].size;
      }
      webrtc::EncodedImage unit(aEncodedImage);
      unit._completeFrame = true;

      mCallback->Encoded(unit, nullptr, &fragmentation);
    }
  }

  OMXVideoEncoder* mOMX;
  webrtc::EncodedImageCallback* mCallback;
  bool mIsPrevFrameParamSets;
  nsTArray<uint8_t> mParamSets;
};

// Encoder.
WebrtcOMXH264VideoEncoder::WebrtcOMXH264VideoEncoder()
  : mOMX(nullptr)
  , mCallback(nullptr)
  , mWidth(0)
  , mHeight(0)
  , mFrameRate(0)
  , mBitRateKbps(0)
#ifdef OMX_IDR_NEEDED_FOR_BITRATE
  , mBitRateAtLastIDR(0)
#endif
  , mOMXConfigured(false)
  , mOMXReconfigure(false)
{
  mReservation = new OMXCodecReservation(true);
  CODEC_LOGD("WebrtcOMXH264VideoEncoder:%p constructed", this);
}

int32_t
WebrtcOMXH264VideoEncoder::InitEncode(const webrtc::VideoCodec* aCodecSettings,
                                      int32_t aNumOfCores,
                                      size_t aMaxPayloadSize)
{
  CODEC_LOGD("WebrtcOMXH264VideoEncoder:%p init", this);

  if (mOMX == nullptr) {
    nsAutoPtr<OMXVideoEncoder> omx(OMXCodecWrapper::CreateAVCEncoder());
    if (NS_WARN_IF(omx == nullptr)) {
      return WEBRTC_VIDEO_CODEC_ERROR;
    }
    mOMX = omx.forget();
    CODEC_LOGD("WebrtcOMXH264VideoEncoder:%p OMX created", this);
  }

  if (!mReservation->ReserveOMXCodec()) {
    CODEC_LOGD("WebrtcOMXH264VideoEncoder:%p Encoder in use", this);
    mOMX = nullptr;
    return WEBRTC_VIDEO_CODEC_ERROR;
  }

  // Defer configuration until 1st frame is received because this function will
  // be called more than once, and unfortunately with incorrect setting values
  // at first.
  mWidth = aCodecSettings->width;
  mHeight = aCodecSettings->height;
  mFrameRate = aCodecSettings->maxFramerate;
  mBitRateKbps = aCodecSettings->startBitrate;
  // XXX handle maxpayloadsize (aka mode 0/1)

  CODEC_LOGD("WebrtcOMXH264VideoEncoder:%p OMX Encoder reserved", this);
  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t
WebrtcOMXH264VideoEncoder::Encode(const webrtc::I420VideoFrame& aInputImage,
                                  const webrtc::CodecSpecificInfo* aCodecSpecificInfo,
                                  const std::vector<webrtc::VideoFrameType>* aFrameTypes)
{
  MOZ_ASSERT(mOMX != nullptr);
  if (mOMX == nullptr) {
    return WEBRTC_VIDEO_CODEC_ERROR;
  }

  // Have to reconfigure for resolution or framerate changes :-(
  // ~220ms initial configure on 8x10, 50-100ms for re-configure it appears
  // XXX drop frames while this is happening?
  if (aInputImage.width() < 0 || (uint32_t)aInputImage.width() != mWidth ||
      aInputImage.height() < 0 || (uint32_t)aInputImage.height() != mHeight) {
    mWidth = aInputImage.width();
    mHeight = aInputImage.height();
    mOMXReconfigure = true;
  }

  if (!mOMXConfigured || mOMXReconfigure) {
    if (mOMXConfigured) {
      CODEC_LOGD("WebrtcOMXH264VideoEncoder:%p reconfiguring encoder %dx%d @ %u fps",
                 this, mWidth, mHeight, mFrameRate);
      mOMXConfigured = false;
    }
    mOMXReconfigure = false;
    // XXX This can take time.  Encode() likely assumes encodes are queued "quickly" and
    // don't block the input too long.  Frames may build up.

    // XXX take from negotiated SDP in codecSpecific data
    OMX_VIDEO_AVCLEVELTYPE level = OMX_VIDEO_AVCLevel3;
    // OMX_Video_ControlRateConstant is not supported on QC 8x10
    OMX_VIDEO_CONTROLRATETYPE bitrateMode = OMX_Video_ControlRateConstantSkipFrames;

    // Set up configuration parameters for AVC/H.264 encoder.
    sp<AMessage> format = new AMessage;
    // Fixed values
    format->setString("mime", MEDIA_MIMETYPE_VIDEO_AVC);
    // XXX We should only set to < infinity if we're not using any recovery RTCP options
    // However, we MUST set it to a lower value because the 8x10 rate controller
    // only changes rate at GOP boundaries.... but it also changes rate on requested GOPs

    // Too long and we have very low bitrates for the first second or two... plus
    // bug 1014921 means we have to force them every ~3 seconds or less.
    format->setInt32("i-frame-interval", 4 /* seconds */);
    // See mozilla::layers::GrallocImage, supports YUV 4:2:0, CbCr width and
    // height is half that of Y
    format->setInt32("color-format", OMX_COLOR_FormatYUV420SemiPlanar);
    format->setInt32("profile", OMX_VIDEO_AVCProfileBaseline);
    format->setInt32("level", level);
    format->setInt32("bitrate-mode", bitrateMode);
    format->setInt32("store-metadata-in-buffers", 0);
    // XXX Unfortunately, 8x10 doesn't support this, but ask anyways
    format->setInt32("prepend-sps-pps-to-idr-frames", 1);
    // Input values.
    format->setInt32("width", mWidth);
    format->setInt32("height", mHeight);
    format->setInt32("stride", mWidth);
    format->setInt32("slice-height", mHeight);
    format->setInt32("frame-rate", mFrameRate);
    format->setInt32("bitrate", mBitRateKbps*1000);

    CODEC_LOGD("WebrtcOMXH264VideoEncoder:%p configuring encoder %dx%d @ %d fps, rate %d kbps",
               this, mWidth, mHeight, mFrameRate, mBitRateKbps);
    nsresult rv = mOMX->ConfigureDirect(format,
                                        OMXVideoEncoder::BlobFormat::AVC_NAL);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      CODEC_LOGE("WebrtcOMXH264VideoEncoder:%p FAILED configuring encoder %d", this, int(rv));
      return WEBRTC_VIDEO_CODEC_ERROR;
    }
    mOMXConfigured = true;
#ifdef OMX_IDR_NEEDED_FOR_BITRATE
    mLastIDRTime = TimeStamp::Now();
    mBitRateAtLastIDR = mBitRateKbps;
#endif
  }

  if (aFrameTypes && aFrameTypes->size() &&
      ((*aFrameTypes)[0] == webrtc::kKeyFrame)) {
    mOMX->RequestIDRFrame();
#ifdef OMX_IDR_NEEDED_FOR_BITRATE
    mLastIDRTime = TimeStamp::Now();
    mBitRateAtLastIDR = mBitRateKbps;
  } else if (mBitRateKbps != mBitRateAtLastIDR) {
    // 8x10 OMX codec requires a keyframe to shift bitrates!
    TimeStamp now = TimeStamp::Now();
    if (mLastIDRTime.IsNull()) {
      // paranoia
      mLastIDRTime = now;
    }
    int32_t timeSinceLastIDR = (now - mLastIDRTime).ToMilliseconds();

    // Balance asking for IDRs too often against direction and amount of bitrate change.

    // HACK for bug 1014921: 8x10 has encode/decode mismatches that build up errors
    // if you go too long without an IDR.  In normal use, bitrate will change often
    // enough to never hit this time limit.
    if ((timeSinceLastIDR > 3000) ||
        (mBitRateKbps < (mBitRateAtLastIDR * 8)/10) ||
        (timeSinceLastIDR < 300 && mBitRateKbps < (mBitRateAtLastIDR * 9)/10) ||
        (timeSinceLastIDR < 1000 && mBitRateKbps < (mBitRateAtLastIDR * 97)/100) ||
        (timeSinceLastIDR >= 1000 && mBitRateKbps < mBitRateAtLastIDR) ||
        (mBitRateKbps > (mBitRateAtLastIDR * 15)/10) ||
        (timeSinceLastIDR < 500 && mBitRateKbps > (mBitRateAtLastIDR * 13)/10) ||
        (timeSinceLastIDR < 1000 && mBitRateKbps > (mBitRateAtLastIDR * 11)/10) ||
        (timeSinceLastIDR >= 1000 && mBitRateKbps > mBitRateAtLastIDR)) {
      CODEC_LOGD("Requesting IDR for bitrate change from %u to %u (time since last idr %dms)",
                 mBitRateAtLastIDR, mBitRateKbps, timeSinceLastIDR);

      mOMX->RequestIDRFrame();
      mLastIDRTime = now;
      mBitRateAtLastIDR = mBitRateKbps;
    }
#endif
  }

  // Wrap I420VideoFrame input with PlanarYCbCrImage for OMXVideoEncoder.
  layers::PlanarYCbCrData yuvData;
  yuvData.mYChannel = const_cast<uint8_t*>(aInputImage.buffer(webrtc::kYPlane));
  yuvData.mYSize = gfx::IntSize(aInputImage.width(), aInputImage.height());
  yuvData.mYStride = aInputImage.stride(webrtc::kYPlane);
  MOZ_ASSERT(aInputImage.stride(webrtc::kUPlane) == aInputImage.stride(webrtc::kVPlane));
  yuvData.mCbCrStride = aInputImage.stride(webrtc::kUPlane);
  yuvData.mCbChannel = const_cast<uint8_t*>(aInputImage.buffer(webrtc::kUPlane));
  yuvData.mCrChannel = const_cast<uint8_t*>(aInputImage.buffer(webrtc::kVPlane));
  yuvData.mCbCrSize = gfx::IntSize((yuvData.mYSize.width + 1) / 2,
                                   (yuvData.mYSize.height + 1) / 2);
  yuvData.mPicSize = yuvData.mYSize;
  yuvData.mStereoMode = StereoMode::MONO;
  layers::RecyclingPlanarYCbCrImage img(nullptr);
  // AdoptData() doesn't need AllocateAndGetNewBuffer(); OMXVideoEncoder is ok with this
  img.AdoptData(yuvData);

  CODEC_LOGD("Encode frame: %dx%d, timestamp %u (%lld), renderTimeMs %" PRIu64,
             aInputImage.width(), aInputImage.height(),
             aInputImage.timestamp(), aInputImage.timestamp() * 1000ll / 90,
             aInputImage.render_time_ms());

  nsresult rv = mOMX->Encode(&img,
                             yuvData.mYSize.width,
                             yuvData.mYSize.height,
                             aInputImage.timestamp() * 1000ll / 90, // 90kHz -> us.
                             0);
  if (rv == NS_OK) {
    if (mOutputDrain == nullptr) {
      mOutputDrain = new EncOutputDrain(mOMX, mCallback);
      mOutputDrain->Start();
    }
    EncodedFrame frame;
    frame.mWidth = mWidth;
    frame.mHeight = mHeight;
    frame.mTimestamp = aInputImage.timestamp();
    frame.mRenderTimeMs = aInputImage.render_time_ms();
    mOutputDrain->QueueInput(frame);
  }

  return (rv == NS_OK) ? WEBRTC_VIDEO_CODEC_OK : WEBRTC_VIDEO_CODEC_ERROR;
}

int32_t
WebrtcOMXH264VideoEncoder::RegisterEncodeCompleteCallback(
    webrtc::EncodedImageCallback* aCallback)
{
  CODEC_LOGD("WebrtcOMXH264VideoEncoder:%p set callback:%p", this, aCallback);
  MOZ_ASSERT(aCallback);
  mCallback = aCallback;

  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t
WebrtcOMXH264VideoEncoder::Release()
{
  CODEC_LOGD("WebrtcOMXH264VideoEncoder:%p will be released", this);

  if (mOutputDrain != nullptr) {
    mOutputDrain->Stop();
    mOutputDrain = nullptr;
  }
  mOMXConfigured = false;
  bool hadOMX = !!mOMX;
  mOMX = nullptr;
  if (hadOMX) {
    mReservation->ReleaseOMXCodec();
  }
  CODEC_LOGD("WebrtcOMXH264VideoEncoder:%p released", this);

  return WEBRTC_VIDEO_CODEC_OK;
}

WebrtcOMXH264VideoEncoder::~WebrtcOMXH264VideoEncoder()
{
  CODEC_LOGD("WebrtcOMXH264VideoEncoder:%p will be destructed", this);

  Release();
}

// Inform the encoder of the new packet loss rate and the round-trip time of
// the network. aPacketLossRate is fraction lost and can be 0~255
// (255 means 100% lost).
// Note: stagefright doesn't handle these parameters.
int32_t
WebrtcOMXH264VideoEncoder::SetChannelParameters(uint32_t aPacketLossRate,
                                                int64_t aRoundTripTimeMs)
{
  CODEC_LOGD("WebrtcOMXH264VideoEncoder:%p set channel packet loss:%u, rtt:%" PRIi64,
             this, aPacketLossRate, aRoundTripTimeMs);

  return WEBRTC_VIDEO_CODEC_OK;
}

// TODO: Bug 997567. Find the way to support frame rate change.
int32_t
WebrtcOMXH264VideoEncoder::SetRates(uint32_t aBitRateKbps, uint32_t aFrameRate)
{
  CODEC_LOGE("WebrtcOMXH264VideoEncoder:%p set bitrate:%u, frame rate:%u (%u))",
             this, aBitRateKbps, aFrameRate, mFrameRate);
  MOZ_ASSERT(mOMX != nullptr);
  if (mOMX == nullptr) {
    return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
  }

  // XXX Should use StageFright framerate change, perhaps only on major changes of framerate.

  // Without Stagefright support, Algorithm should be:
  // if (frameRate < 50% of configured) {
  //   drop framerate to next step down that includes current framerate within 50%
  // } else if (frameRate > configured) {
  //   change config to next step up that includes current framerate
  // }
#if !defined(TEST_OMX_FRAMERATE_CHANGES)
  if (aFrameRate > mFrameRate ||
      aFrameRate < mFrameRate/2) {
    uint32_t old_rate = mFrameRate;
    if (aFrameRate >= 15) {
      mFrameRate = 30;
    } else if (aFrameRate >= 10) {
      mFrameRate = 20;
    } else if (aFrameRate >= 8) {
      mFrameRate = 15;
    } else /* if (aFrameRate >= 5)*/ {
      // don't go lower; encoder may not be stable
      mFrameRate = 10;
    }
    if (mFrameRate < aFrameRate) { // safety
      mFrameRate = aFrameRate;
    }
    if (old_rate != mFrameRate) {
      mOMXReconfigure = true;  // force re-configure on next frame
    }
  }
#else
  // XXX for testing, be wild!
  if (aFrameRate != mFrameRate) {
    mFrameRate = aFrameRate;
    mOMXReconfigure = true;  // force re-configure on next frame
  }
#endif

  // XXX Limit bitrate for 8x10 devices to a specific level depending on fps and resolution
  // mBitRateKbps = LimitBitrate8x10(mWidth, mHeight, mFrameRate, aBitRateKbps);
  // Rely on global single setting (~720 kbps for HVGA@30fps) for now
  if (aBitRateKbps > 700) {
    aBitRateKbps = 700;
  }
  mBitRateKbps = aBitRateKbps;
  nsresult rv = mOMX->SetBitrate(mBitRateKbps);
  NS_WARN_IF(NS_FAILED(rv));
  return NS_FAILED(rv) ? WEBRTC_VIDEO_CODEC_OK : WEBRTC_VIDEO_CODEC_ERROR;
}

// Decoder.
WebrtcOMXH264VideoDecoder::WebrtcOMXH264VideoDecoder()
  : mCallback(nullptr)
  , mOMX(nullptr)
{
  mReservation = new OMXCodecReservation(false);
  CODEC_LOGD("WebrtcOMXH264VideoDecoder:%p will be constructed", this);
}

int32_t
WebrtcOMXH264VideoDecoder::InitDecode(const webrtc::VideoCodec* aCodecSettings,
                                      int32_t aNumOfCores)
{
  CODEC_LOGD("WebrtcOMXH264VideoDecoder:%p init OMX:%p", this, mOMX.get());

  if (!mReservation->ReserveOMXCodec()) {
    CODEC_LOGD("WebrtcOMXH264VideoDecoder:%p Decoder in use", this);
    return WEBRTC_VIDEO_CODEC_ERROR;
  }

  // Defer configuration until SPS/PPS NALUs (where actual decoder config
  // values can be extracted) are received.

  CODEC_LOGD("WebrtcOMXH264VideoDecoder:%p OMX Decoder reserved", this);
  return WEBRTC_VIDEO_CODEC_OK;
}

// Find SPS in input data and extract picture width and height if found.
/* static */ int32_t
WebrtcOMXH264VideoDecoder::ExtractPicDimensions(uint8_t* aData, size_t aSize,
                                                int32_t* aWidth, int32_t* aHeight)
{
  MOZ_ASSERT(aData && aSize > sizeof(kNALStartCode));
  if ((aData[sizeof(kNALStartCode)] & 0x1f) != kNALTypeSPS) {
    return WEBRTC_VIDEO_CODEC_ERROR;
  }
  sp<ABuffer> sps = new ABuffer(&aData[sizeof(kNALStartCode)], aSize - sizeof(kNALStartCode));
  FindAVCDimensions(sps, aWidth, aHeight);
  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t
WebrtcOMXH264VideoDecoder::Decode(const webrtc::EncodedImage& aInputImage,
                                  bool aMissingFrames,
                                  const webrtc::RTPFragmentationHeader* aFragmentation,
                                  const webrtc::CodecSpecificInfo* aCodecSpecificInfo,
                                  int64_t aRenderTimeMs)
{
  if (aInputImage._length== 0 || !aInputImage._buffer) {
    return WEBRTC_VIDEO_CODEC_ERROR;
  }

  if (!mOMX) {
    // Search for SPS NALU in input to get width/height config.
    int32_t width;
    int32_t height;
    int32_t result = ExtractPicDimensions(aInputImage._buffer,
                                          aInputImage._length,
                                          &width, &height);
    if (result != WEBRTC_VIDEO_CODEC_OK) {
      // Cannot config decoder because SPS haven't been seen.
      CODEC_LOGI("WebrtcOMXH264VideoDecoder:%p missing SPS in input (nal 0x%02x, len %d)",
                 this, aInputImage._buffer[sizeof(kNALStartCode)] & 0x1f, aInputImage._length);
      return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }
    RefPtr<WebrtcOMXDecoder> omx = new WebrtcOMXDecoder(MEDIA_MIMETYPE_VIDEO_AVC,
                                                        mCallback);
    status_t err = omx->ConfigureWithPicDimensions(width, height);
    if (NS_WARN_IF(err != OK)) {
      return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }
    CODEC_LOGD("WebrtcOMXH264VideoDecoder:%p start OMX", this);
    mOMX = omx;
  }

  // Break input encoded data into NALUs and send each one to decode.
  // 8x10 decoder doesn't allow picture coding NALs to be in the same buffer
  // with SPS/PPS (BUFFER_FLAG_CODECCONFIG) per QC
  const uint8_t* data = aInputImage._buffer;
  size_t size = aInputImage._length;
  const uint8_t* nalStart = nullptr;
  size_t nalSize = 0;

  // this returns a pointer to the NAL byte (after the StartCode)
  while (getNextNALUnit(&data, &size, &nalStart, &nalSize, true) == OK) {
    // Individual NALU inherits metadata from input encoded data.
    webrtc::EncodedImage nalu(aInputImage);

    nalu._buffer = const_cast<uint8_t*>(nalStart) - sizeof(kNALStartCode);
    MOZ_ASSERT(nalu._buffer >= aInputImage._buffer);
    nalu._length = nalSize + sizeof(kNALStartCode);
    MOZ_ASSERT(nalu._buffer + nalu._length <= aInputImage._buffer + aInputImage._length);

    int nalType = nalStart[0] & 0x1f;
    bool isCodecConfig = false;
    if (nalType == kNALTypeSPS || nalType == kNALTypePPS) {
      isCodecConfig = true;
    }
    status_t err = mOMX->FillInput(nalu, isCodecConfig, aRenderTimeMs);
    if (NS_WARN_IF(err != OK)) {
      return WEBRTC_VIDEO_CODEC_ERROR;
    }
  }
  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t
WebrtcOMXH264VideoDecoder::RegisterDecodeCompleteCallback(webrtc::DecodedImageCallback* aCallback)
{
  CODEC_LOGD("WebrtcOMXH264VideoDecoder:%p set callback:%p", this, aCallback);
  MOZ_ASSERT(aCallback);
  mCallback = aCallback;

  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t
WebrtcOMXH264VideoDecoder::Release()
{
  CODEC_LOGD("WebrtcOMXH264VideoDecoder:%p will be released", this);

  mOMX = nullptr; // calls Stop()
  mReservation->ReleaseOMXCodec();

  return WEBRTC_VIDEO_CODEC_OK;
}

WebrtcOMXH264VideoDecoder::~WebrtcOMXH264VideoDecoder()
{
  CODEC_LOGD("WebrtcOMXH264VideoDecoder:%p will be destructed", this);
  Release();
}

int32_t
WebrtcOMXH264VideoDecoder::Reset()
{
  CODEC_LOGW("WebrtcOMXH264VideoDecoder::Reset() will NOT reset decoder");
  return WEBRTC_VIDEO_CODEC_OK;
}

} // namespace mozilla
