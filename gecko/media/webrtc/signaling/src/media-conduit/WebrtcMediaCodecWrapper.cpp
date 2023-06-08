/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "WebrtcMediaCodecWrapper.h"

// Android/Stagefright
#include <binder/ProcessState.h>
#include <foundation/ABuffer.h>
#include <foundation/AMessage.h>
#include <gui/Surface.h>
#include <media/ICrypto.h>
#include <media/stagefright/MediaCodec.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MetaData.h>
#include <OMX_Component.h>

// Gecko
#if defined(MOZ_WIDGET_GONK) && ANDROID_VERSION >= 21
#include "GonkBufferQueueProducer.h"
#endif
#include "GonkNativeWindow.h"
#include "GrallocImages.h"
#include "mozilla/Mutex.h"
#include "nsThreadUtils.h"
#include "TextureClient.h"

using namespace android;

namespace mozilla {

#define WEBRTC_OMX_MIN_DECODE_BUFFERS 10
#define DEQUEUE_BUFFER_TIMEOUT_US (100 * 1000ll) // 100ms.
#define DRAIN_THREAD_TIMEOUT_US  (1000 * 1000ll) // 1s.

class ImageNativeHandle : public webrtc::NativeHandle
{
public:
  ImageNativeHandle(layers::Image* aImage)
    : mImage(aImage)
  {}

  virtual void* GetHandle() override { return mImage.get(); }

private:
  RefPtr<layers::Image> mImage;
};

static void
ShutdownThread(nsCOMPtr<nsIThread>& aThread)
{
  aThread->Shutdown();
}

void
OMXOutputDrain::Start()
{
  CODEC_LOGD("OMXOutputDrain starting");
  MonitorAutoLock lock(mMonitor);
  if (mThread == nullptr) {
    NS_NewNamedThread("OMXOutputDrain", getter_AddRefs(mThread));
  }
  CODEC_LOGD("OMXOutputDrain started");
  mEnding = false;
  mThread->Dispatch(this, NS_DISPATCH_NORMAL);
}

void
OMXOutputDrain::Stop()
{
  CODEC_LOGD("OMXOutputDrain stopping");
  MonitorAutoLock lock(mMonitor);
  mEnding = true;
  lock.NotifyAll(); // In case Run() is waiting.

  if (mThread != nullptr) {
    MonitorAutoUnlock unlock(mMonitor);
    CODEC_LOGD("OMXOutputDrain thread shutdown");
    NS_DispatchToMainThread(
      WrapRunnableNM<decltype(&ShutdownThread),
                     nsCOMPtr<nsIThread> >(&ShutdownThread, mThread));
    mThread = nullptr;
  }
  CODEC_LOGD("OMXOutputDrain stopped");
}

void
OMXOutputDrain::QueueInput(const EncodedFrame& aFrame)
{
  MonitorAutoLock lock(mMonitor);

  MOZ_ASSERT(mThread);

  mInputFrames.push(aFrame);
  // Notify Run() about queued input and it can start working.
  lock.NotifyAll();
}

NS_IMETHODIMP
OMXOutputDrain::Run()
{
  MonitorAutoLock lock(mMonitor);
  if (mEnding) {
    return NS_OK;
  }
  MOZ_ASSERT(mThread);

  while (true) {
    if (mInputFrames.empty()) {
      // Wait for new input.
      lock.Wait();
    }

    if (mEnding) {
      CODEC_LOGD("OMXOutputDrain Run() ending");
      // Stop draining.
      break;
    }

    MOZ_ASSERT(!mInputFrames.empty());
    {
      // Release monitor while draining because it's blocking.
      MonitorAutoUnlock unlock(mMonitor);
      DrainOutput();
    }
  }

  CODEC_LOGD("OMXOutputDrain Ended");
  return NS_OK;
}

WebrtcOMXDecoder::~WebrtcOMXDecoder()
{
  CODEC_LOGD("WebrtcOMXDecoder:%p OMX destructor", this);
  if (mStarted) {
    Stop();
  }
  if (mCodec != nullptr) {
    mCodec->release();
    mCodec.clear();
  }
  mLooper.clear();
}

WebrtcOMXDecoder::WebrtcOMXDecoder(const char* aMimeType,
                                   webrtc::DecodedImageCallback* aCallback)
  : mWidth(0)
  , mHeight(0)
  , mStarted(false)
  , mEnding(false)
  , mMimeType(aMimeType)
  , mCallback(aCallback)
  , mDecodedFrameLock("WebRTC decoded frame lock")
{
  // Create binder thread pool required by stagefright.
  android::ProcessState::self()->startThreadPool();

  mLooper = new ALooper;
  mLooper->start();
  CODEC_LOGD("WebrtcOMXDecoder:%p creating decoder", this);
  mCodec = MediaCodec::CreateByType(mLooper, aMimeType, false /* encoder */);
  CODEC_LOGD("WebrtcOMXDecoder:%p OMX created", this);
}

status_t
WebrtcOMXDecoder::ConfigureWithPicDimensions(int32_t aWidth, int32_t aHeight)
{
  MOZ_ASSERT(mCodec != nullptr);
  if (mCodec == nullptr) {
    return INVALID_OPERATION;
  }

  CODEC_LOGD("OMX:%p decoder width:%d height:%d", this, aWidth, aHeight);

  sp<AMessage> config = new AMessage();
  config->setString("mime", mMimeType);
  config->setInt32("width", aWidth);
  config->setInt32("height", aHeight);
  mWidth = aWidth;
  mHeight = aHeight;

  sp<Surface> surface = nullptr;
#if defined(MOZ_WIDGET_GONK) && ANDROID_VERSION >= 21
  sp<IGraphicBufferProducer> producer;
  sp<IGonkGraphicBufferConsumer> consumer;
  GonkBufferQueue::createBufferQueue(&producer, &consumer);
  mNativeWindow = new GonkNativeWindow(consumer);
#else
  mNativeWindow = new GonkNativeWindow();
#endif
  if (mNativeWindow.get()) {
    // listen to buffers queued by MediaCodec::RenderOutputBufferAndRelease().
    mNativeWindow->setNewFrameCallback(this);
    // XXX remove buffer changes after a better solution lands - bug 1009420
#if defined(MOZ_WIDGET_GONK) && ANDROID_VERSION >= 21
    static_cast<GonkBufferQueueProducer*>(producer.get())->setSynchronousMode(false);
    // More spare buffers to avoid OMX decoder waiting for native window
    consumer->setMaxAcquiredBufferCount(WEBRTC_OMX_MIN_DECODE_BUFFERS);
    surface = new Surface(producer);
#else
    sp<GonkBufferQueue> bq = mNativeWindow->getBufferQueue();
    bq->setSynchronousMode(false);
    // More spare buffers to avoid OMX decoder waiting for native window
    bq->setMaxAcquiredBufferCount(WEBRTC_OMX_MIN_DECODE_BUFFERS);
    surface = new Surface(bq);
#endif
  }
  status_t result = mCodec->configure(config, surface, nullptr, 0);
  if (result == OK) {
    CODEC_LOGD("OMX:%p decoder configured", this);
    result = Start();
  }
  return result;
}

status_t
WebrtcOMXDecoder::FillInput(const webrtc::EncodedImage& aEncoded,
                            bool aIsCodecConfig,
                            int64_t aRenderTimeMs)
{
  if (mCodec == nullptr || !aEncoded._buffer || aEncoded._length == 0) {
    return INVALID_OPERATION;
  }

  size_t index;
  status_t err = mCodec->dequeueInputBuffer(&index, DEQUEUE_BUFFER_TIMEOUT_US);
  if (err != OK) {
    if (err != -EAGAIN) {
      CODEC_LOGE("decode dequeue input buffer error:%d", err);
    } else {
      CODEC_LOGE("decode dequeue 100ms without a buffer (EAGAIN)");
    }
    return err;
  }

  const sp<ABuffer>& omxIn = mInputBuffers.itemAt(index);
  MOZ_ASSERT(omxIn->capacity() >= aEncoded._length);
  omxIn->setRange(0, aEncoded._length);
  // Copying is needed because MediaCodec API doesn't support externally
  // allocated buffer as input.
  uint8_t* dst = omxIn->data();
  memcpy(dst, aEncoded._buffer, aEncoded._length);
  int64_t inputTimeUs = (aEncoded._timeStamp * 1000ll) / 90; // 90kHz -> us.
  // Assign input flags according to frame header
  uint32_t flags = 0;
  if (aIsCodecConfig) {
    flags = MediaCodec::BUFFER_FLAG_CODECCONFIG;
  }

  err = mCodec->queueInputBuffer(index, 0, aEncoded._length, inputTimeUs, flags);
  if (err == OK && !aIsCodecConfig) {
    if (mOutputDrain == nullptr) {
      mOutputDrain = new OutputDrain(this);
      mOutputDrain->Start();
    }
    EncodedFrame frame;
    frame.mWidth = mWidth;
    frame.mHeight = mHeight;
    frame.mTimestamp = aEncoded._timeStamp;
    frame.mRenderTimeMs = aRenderTimeMs;
    mOutputDrain->QueueInput(frame);
  }

  return err;
}

status_t
WebrtcOMXDecoder::DrainOutput(std::queue<EncodedFrame>& aInputFrames, Monitor& aMonitor)
{
  MOZ_ASSERT(mCodec != nullptr);
  if (mCodec == nullptr) {
    return INVALID_OPERATION;
  }

  size_t index = 0;
  size_t outOffset = 0;
  size_t outSize = 0;
  int64_t outTime = -1ll;
  uint32_t outFlags = 0;
  status_t err = mCodec->dequeueOutputBuffer(&index, &outOffset, &outSize,
                                             &outTime, &outFlags,
                                             DRAIN_THREAD_TIMEOUT_US);
  switch (err) {
    case OK:
      break;
    case -EAGAIN:
      // Not an error: output not available yet. Try later.
      CODEC_LOGI("decode dequeue OMX output buffer timed out. Try later.");
      return err;
    case INFO_FORMAT_CHANGED:
      // Not an error: will get this value when OMX output buffer is enabled,
      // or when input size changed.
      CODEC_LOGD("decode dequeue OMX output buffer format change");
      return err;
    case INFO_OUTPUT_BUFFERS_CHANGED:
      // Not an error: will get this value when OMX output buffer changed
      // (probably because of input size change).
      CODEC_LOGD("decode dequeue OMX output buffer change");
      err = mCodec->getOutputBuffers(&mOutputBuffers);
      MOZ_ASSERT(err == OK);
      return INFO_OUTPUT_BUFFERS_CHANGED;
    default:
      CODEC_LOGE("decode dequeue OMX output buffer error:%d", err);
      // Return OK to instruct OutputDrain to drop input from queue.
      MonitorAutoLock lock(aMonitor);
      aInputFrames.pop();
      return OK;
  }

  if (mCallback) {
    EncodedFrame frame;
    {
      MonitorAutoLock lock(aMonitor);
      frame = aInputFrames.front();
      aInputFrames.pop();
    }
    {
      // Store info of this frame. OnNewFrame() will need the timestamp later.
      MutexAutoLock lock(mDecodedFrameLock);
      if (mEnding) {
        mCodec->releaseOutputBuffer(index);
        return err;
      }
      mDecodedFrames.push(frame);
    }
    // Ask codec to queue buffer back to native window. OnNewFrame() will be
    // called.
    mCodec->renderOutputBufferAndRelease(index);
    // Once consumed, buffer will be queued back to GonkNativeWindow for codec
    // to dequeue/use.
  } else {
    mCodec->releaseOutputBuffer(index);
  }

  return err;
}

void
WebrtcOMXDecoder::OnNewFrame()
{
  RefPtr<layers::TextureClient> buffer = mNativeWindow->getCurrentBuffer();
  if (!buffer) {
    CODEC_LOGE("Decoder NewFrame: Get null buffer");
    return;
  }

  gfx::IntSize picSize(buffer->GetSize());
  nsAutoPtr<layers::GrallocImage> grallocImage(new layers::GrallocImage());
  grallocImage->AdoptData(buffer, picSize);

  // Get timestamp of the frame about to render.
  int64_t timestamp = -1;
  int64_t renderTimeMs = -1;
  {
    MutexAutoLock lock(mDecodedFrameLock);
    if (mDecodedFrames.empty()) {
      return;
    }
    EncodedFrame decoded = mDecodedFrames.front();
    timestamp = decoded.mTimestamp;
    renderTimeMs = decoded.mRenderTimeMs;
    mDecodedFrames.pop();
  }
  MOZ_ASSERT(timestamp >= 0 && renderTimeMs >= 0);

  nsAutoPtr<webrtc::I420VideoFrame> videoFrame(new webrtc::I420VideoFrame(
    new rtc::RefCountedObject<ImageNativeHandle>(grallocImage.forget()),
    picSize.width,
    picSize.height,
    timestamp,
    renderTimeMs));
  if (videoFrame != nullptr) {
    mCallback->Decoded(*videoFrame);
  }
}

status_t
WebrtcOMXDecoder::Start()
{
  MOZ_ASSERT(!mStarted);
  if (mStarted) {
    return OK;
  }

  {
    MutexAutoLock lock(mDecodedFrameLock);
    mEnding = false;
  }
  status_t err = mCodec->start();
  if (err == OK) {
    mStarted = true;
    mCodec->getInputBuffers(&mInputBuffers);
    mCodec->getOutputBuffers(&mOutputBuffers);
  }

  return err;
}

status_t
WebrtcOMXDecoder::Stop()
{
  MOZ_ASSERT(mStarted);
  if (!mStarted) {
    return OK;
  }

  CODEC_LOGD("OMXOutputDrain decoder stopping");
  // Drop all 'pending to render' frames.
  {
    MutexAutoLock lock(mDecodedFrameLock);
    mEnding = true;
    while (!mDecodedFrames.empty()) {
      mDecodedFrames.pop();
    }
  }

  if (mOutputDrain != nullptr) {
    CODEC_LOGD("decoder's OutputDrain stopping");
    mOutputDrain->Stop();
    mOutputDrain = nullptr;
  }

  status_t err = mCodec->stop();
  if (err == OK) {
    mInputBuffers.clear();
    mOutputBuffers.clear();
    mStarted = false;
  } else {
    MOZ_ASSERT(false);
  }
  CODEC_LOGD("OMXOutputDrain decoder stopped");
  return err;
}

} // namespace mozilla
