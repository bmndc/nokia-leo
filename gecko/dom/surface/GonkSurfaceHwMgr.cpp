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

#include "GonkSurfaceHwMgr.h"

#ifdef MOZ_WIDGET_GONK
#include <binder/IPCThreadState.h>
#include <sys/system_properties.h>
#include "GonkNativeWindow.h"
#endif

#include "base/basictypes.h"
#include "nsDebug.h"
#include "mozilla/layers/TextureClient.h"
#include "mozilla/RefPtr.h"
#if defined(MOZ_WIDGET_GONK) && ANDROID_VERSION >= 21
#include "GonkBufferQueueProducer.h"
#if ANDROID_VERSION >= 23
#include <binder/IServiceManager.h>
#endif
#endif
#include "GonkSurfaceControl.h"

using namespace mozilla;
using namespace mozilla::layers;
using namespace android;

#ifndef MOZ_WIDGET_GONK
NS_IMPL_ISUPPORTS0(GonkSurfaceHardware);
NS_IMPL_ISUPPORTS0(android::Surface);
#endif

GonkSurfaceHardware::GonkSurfaceHardware(mozilla::nsGonkSurfaceControl* aTarget)
  : mClosing(false)
  , mTarget(aTarget)
{
}

#ifdef MOZ_WIDGET_GONK
void
GonkSurfaceHardware::OnNewFrame() {
  if (mClosing) {
    return;
  }
  RefPtr<TextureClient> buffer = mNativeWindow->getCurrentBuffer();
  if (!buffer) {
    DOM_CAMERA_LOGE("received null frame");
    return;
  }
  OnNewPreviewFrame(mTarget, buffer);
}
#endif

nsresult
GonkSurfaceHardware::Init()
{

#ifdef MOZ_WIDGET_GONK
#if ANDROID_VERSION >= 21
  sp<IGraphicBufferProducer> producer;
  sp<IGonkGraphicBufferConsumer> consumer;
  GonkBufferQueue::createBufferQueue(&producer, &consumer);
  static_cast<GonkBufferQueueProducer*>(producer.get())->setSynchronousMode(false);
  mNativeWindow = new GonkNativeWindow(consumer, GonkSurfaceHardware::MIN_UNDEQUEUED_BUFFERS);

  mGraphicBufferProducer = producer;
#else
  mNativeWindow = new GonkNativeWindow();
#endif
  mNativeWindow->setNewFrameCallback(this);

#endif

  return NS_OK;
}

sp<GonkSurfaceHardware>
GonkSurfaceHardware::Connect(mozilla::nsGonkSurfaceControl* aTarget)
{
  sp<GonkSurfaceHardware> surfaceHardware;
  surfaceHardware = new GonkSurfaceHardware(aTarget);

  nsresult rv = surfaceHardware->Init();
  if (NS_FAILED(rv)) {
    DOM_CAMERA_LOGE("Failed to initialize camera hardware (0x%X)\n", rv);
    surfaceHardware->Close();
    return nullptr;
  }

  return surfaceHardware;
}

void
GonkSurfaceHardware::Close()
{
  if (mClosing) {
    return;
  }

  mClosing = true;
  mGraphicBufferProducer.clear();
#ifdef MOZ_WIDGET_GONK
  if (mNativeWindow.get()) {
    mNativeWindow->abandon();
  }
  mNativeWindow.clear();

  // Ensure that ICamera's destructor is actually executed
  IPCThreadState::self()->flushCommands();
#endif
}

sp<IGraphicBufferProducer> GonkSurfaceHardware::GetGraphicBufferProducer()
{
  return mGraphicBufferProducer;
}

GonkSurfaceHardware::~GonkSurfaceHardware()
{
  mGraphicBufferProducer.clear();
#ifdef MOZ_WIDGET_GONK
  mNativeWindow.clear();
#endif
}