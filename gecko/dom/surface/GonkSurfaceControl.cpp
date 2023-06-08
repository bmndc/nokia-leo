/*
 * Copyright (C) 2012-2015 Mozilla Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "GonkSurfaceControl.h"
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <libgen.h>
#include "base/basictypes.h"
#include "Layers.h"
#ifdef MOZ_WIDGET_GONK
#include "GrallocImages.h"
#include "imgIEncoder.h"
#include "libyuv.h"
#include "nsNetUtil.h" // for NS_ReadInputStreamToBuffer
#endif
#include "nsNetCID.h" // for NS_STREAMTRANSPORTSERVICE_CONTRACTID
#include "nsAutoPtr.h" // for nsAutoArrayPtr
#include "nsCOMPtr.h"
#include "nsMemory.h"
#include "nsThread.h"
#include "nsITimer.h"
#include "mozilla/FileUtils.h"
#include "mozilla/Services.h"
#include "mozilla/unused.h"
#include "mozilla/ipc/FileDescriptorUtils.h"
#include "nsAlgorithm.h"
#include "nsPrintfCString.h"
#include "GonkCameraHwMgr.h"
#include <gui/IGraphicBufferProducer.h>
#include <gui/Surface.h>
#include <android/native_window.h>
#include <ui/GraphicBufferMapper.h>
#include <system/graphics.h>
#include <cutils/properties.h>

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::layers;
using namespace mozilla::gfx;
using namespace mozilla::ipc;
using namespace android;

static const unsigned long kAutoFocusCompleteTimeoutMs = 1000;
static const int32_t kAutoFocusCompleteTimeoutLimit = 3;

// Construct nsGonkSurfaceControl on the main thread.
nsGonkSurfaceControl::nsGonkSurfaceControl()
{
  // Constructor runs on the main thread...
  mImageContainer = LayerManager::CreateImageContainer();
}

nsresult
nsGonkSurfaceControl::Initialize()
{
  if (mSurfaceHw.get()) {
    DOM_CAMERA_LOGI("Surface already connected (this=%p)\n", this);
    return NS_ERROR_ALREADY_INITIALIZED;
  }

  mSurfaceHw = GonkSurfaceHardware::Connect(this);
  if (!mSurfaceHw.get()) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  return NS_OK;
}

nsGonkSurfaceControl::~nsGonkSurfaceControl()
{
}

nsresult
nsGonkSurfaceControl::SetConfigurationInternal(const Configuration& aConfig)
{
  mCurrentConfiguration.mPreviewSize.width = aConfig.mPreviewSize.width;
  mCurrentConfiguration.mPreviewSize.height = aConfig.mPreviewSize.height;

  return NS_OK;
}

nsresult
nsGonkSurfaceControl::SetDataSourceSizeImpl(const ISurfaceControl::Size& aSize)
{
  MOZ_ASSERT(NS_GetCurrentThread() == mSurfaceThread);

  return SetDataSourceSizeInternal(aSize);
}

nsresult
nsGonkSurfaceControl::SetDataSourceSizeInternal(const ISurfaceControl::Size& aSize)
{
  //Update display configuration.
  mCurrentConfiguration.mPreviewSize.width = aSize.width;
  mCurrentConfiguration.mPreviewSize.height = aSize.height;
  return NS_OK;
}

nsresult
nsGonkSurfaceControl::StartImpl(const Configuration* aInitialConfig)
{
  MOZ_ASSERT(NS_GetCurrentThread() == mSurfaceThread);

  nsresult rv = StartInternal(aInitialConfig);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    android::sp<android::IGraphicBufferProducer> empty;
    OnSurfaceStateChange(SurfaceControlListener::kSurfaceCreateFailed,
                         NS_ERROR_NOT_AVAILABLE, empty);
  }
  return rv;
}

nsresult
nsGonkSurfaceControl::StartInternal(const Configuration* aInitialConfig)
{
  nsresult rv = Initialize();
  switch (rv) {
    case NS_ERROR_ALREADY_INITIALIZED:
    case NS_OK:
      break;

    default:
      return rv;
  }

  if (aInitialConfig) {
    rv = SetConfigurationInternal(*aInitialConfig);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      // The initial configuration failed, close up the hardware
      StopInternal();
      return rv;
    }
  }
  android::sp<IGraphicBufferProducer> producer = mSurfaceHw->GetGraphicBufferProducer();
  OnSurfaceStateChange(SurfaceControlListener::kSurfaceCreate, NS_OK, producer);

  return NS_OK;
}

nsresult
nsGonkSurfaceControl::StopInternal()
{
  // release the surface handle
  if (mSurfaceHw.get()){
     mSurfaceHw->Close();
     mSurfaceHw.clear();
  }

  return NS_OK;
}

nsresult
nsGonkSurfaceControl::StopImpl()
{

  nsresult rv = StopInternal();
  if (rv != NS_ERROR_NOT_INITIALIZED) {
    rv = NS_OK;
  }
  if (NS_SUCCEEDED(rv)) {
    android::sp<android::IGraphicBufferProducer> empty;
    OnSurfaceStateChange(SurfaceControlListener::kSurfaceDestroyed, NS_OK, empty);
  }
  return rv;
}

nsresult
nsGonkSurfaceControl::StartPreviewInternal()
{
  MOZ_ASSERT(NS_GetCurrentThread() == mSurfaceThread);

//  ReentrantMonitorAutoEnter mon(mReentrantMonitor);

  if (mPreviewState == SurfaceControlListener::kPreviewStarted) {
    return NS_OK;
  }

  return NS_OK;
}

nsresult
nsGonkSurfaceControl::StartPreviewImpl()
{
  nsresult rv = StartPreviewInternal();
  if (NS_SUCCEEDED(rv)) {
    OnPreviewStateChange(SurfaceControlListener::kPreviewStarted);
  }
  return rv;
}

void
nsGonkSurfaceControl::OnNewPreviewFrame(layers::TextureClient* aBuffer)
{
#ifdef MOZ_WIDGET_GONK
  RefPtr<GrallocImage> frame = new GrallocImage();

  IntSize picSize(mCurrentConfiguration.mPreviewSize.width,
                  mCurrentConfiguration.mPreviewSize.height);
  frame->AdoptData(aBuffer, picSize);

  OnNewPreviewFrame(frame, mCurrentConfiguration.mPreviewSize.width,
                    mCurrentConfiguration.mPreviewSize.height);
#endif
}

// Gonk callback handlers.
namespace mozilla {

void
OnNewPreviewFrame(nsGonkSurfaceControl* gc, layers::TextureClient* aBuffer)
{
  gc->OnNewPreviewFrame(aBuffer);
}

} // namespace mozilla