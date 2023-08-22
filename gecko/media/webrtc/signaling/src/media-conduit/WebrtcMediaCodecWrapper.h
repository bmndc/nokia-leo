/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef WEBRTC_MEDIA_CODEC_WRAPPER_H_
#define WEBRTC_MEDIA_CODEC_WRAPPER_H_

#include <utils/RefBase.h>
#include "webrtc/video_decoder.h"
#include "CSFLog.h"
#include "OMXCodecWrapper.h"
#include "runnable_utils.h"

#define CODEC_LOG_TAG "OMX"
#define CODEC_LOGV(...) CSFLogInfo(CODEC_LOG_TAG, __VA_ARGS__)
#define CODEC_LOGD(...) CSFLogDebug(CODEC_LOG_TAG, __VA_ARGS__)
#define CODEC_LOGI(...) CSFLogInfo(CODEC_LOG_TAG, __VA_ARGS__)
#define CODEC_LOGW(...) CSFLogWarn(CODEC_LOG_TAG, __VA_ARGS__)
#define CODEC_LOGE(...) CSFLogError(CODEC_LOG_TAG, __VA_ARGS__)

namespace mozilla {

struct EncodedFrame
{
  uint32_t mWidth;
  uint32_t mHeight;
  uint32_t mTimestamp;
  int64_t mRenderTimeMs;
};

// Base runnable class to repeatly pull OMX output buffers in seperate thread.
// How to use:
// - implementing DrainOutput() to get output. Remember to return false to tell
//   drain not to pop input queue.
// - call QueueInput() to schedule a run to drain output. The input, aFrame,
//   should contains corresponding info such as image size and timestamps for
//   DrainOutput() implementation to construct data needed by encoded/decoded
//   callbacks.
// TODO: Bug 997110 - Revisit queue/drain logic. Current design assumes that
//       encoder only generate one output buffer per input frame and won't work
//       if encoder drops frames or generates multiple output per input.
class OMXOutputDrain : public nsRunnable
{
public:
  void Start();

  void Stop();

  void QueueInput(const EncodedFrame& aFrame);

  NS_IMETHODIMP Run() override;

protected:
  OMXOutputDrain()
    : mMonitor("OMXOutputDrain monitor")
    , mEnding(false)
  {}

  // Drain output buffer for input frame queue mInputFrames.
  // mInputFrames contains info such as size and time of the input frames.
  // We have to give a queue to handle encoder frame skips - we can input 10
  // frames and get one back.  NOTE: any access of aInputFrames MUST be preceded
  // locking mMonitor!

  // Blocks waiting for decoded buffers, but for a limited period because
  // we need to check for shutdown.
  virtual bool DrainOutput() = 0;

protected:
  // This monitor protects all things below it, and is also used to
  // wait/notify queued input.
  Monitor mMonitor;
  std::queue<EncodedFrame> mInputFrames;

private:
  // also protected by mMonitor
  nsCOMPtr<nsIThread> mThread;
  bool mEnding;
};

// Generic decoder using stagefright.
// It implements gonk native window callback to receive buffers from
// MediaCodec::RenderOutputBufferAndRelease().
class WebrtcOMXDecoder final : public android::GonkNativeWindowNewFrameCallback
{
  typedef android::status_t status_t;
  typedef android::ALooper ALooper;
  typedef android::ABuffer ABuffer;
  typedef android::MediaCodec MediaCodec;
  typedef android::GonkNativeWindow GonkNativeWindow;

public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(WebrtcOMXDecoder)

  WebrtcOMXDecoder(const char* aMimeType,
                   webrtc::DecodedImageCallback* aCallback);

  // Configure decoder using image width/height.
  status_t ConfigureWithPicDimensions(int32_t aWidth, int32_t aHeight);

  status_t FillInput(const webrtc::EncodedImage& aEncoded,
                     bool aIsCodecConfig,
                     int64_t aRenderTimeMs);

  status_t DrainOutput(std::queue<EncodedFrame>& aInputFrames, Monitor& aMonitor);

  // Will be called when MediaCodec::RenderOutputBufferAndRelease() returns
  // buffers back to native window for rendering.
  void OnNewFrame() override;

private:
  class OutputDrain : public OMXOutputDrain
  {
  public:
    OutputDrain(WebrtcOMXDecoder* aOMX)
      : OMXOutputDrain()
      , mOMX(aOMX)
    {}

  protected:
    virtual bool DrainOutput() override
    {
      return (mOMX->DrainOutput(mInputFrames, mMonitor) == android::OK);
    }

  private:
    WebrtcOMXDecoder* mOMX;
  };

  virtual ~WebrtcOMXDecoder();
  status_t Start();
  status_t Stop();

  int mWidth;
  int mHeight;
  bool mStarted;
  bool mEnding;
  const char* mMimeType;
  webrtc::DecodedImageCallback* mCallback;

  android::sp<ALooper> mLooper;
  android::sp<MediaCodec> mCodec;
  android::Vector<android::sp<ABuffer> > mInputBuffers;
  android::Vector<android::sp<ABuffer> > mOutputBuffers;
  android::sp<GonkNativeWindow> mNativeWindow;

  RefPtr<OutputDrain> mOutputDrain;
  Mutex mDecodedFrameLock; // To protect mDecodedFrames and mEnding
  std::queue<EncodedFrame> mDecodedFrames;
};

} // namespace mozilla

#endif // WEBRTC_MEDIA_CODEC_WRAPPER_H_
