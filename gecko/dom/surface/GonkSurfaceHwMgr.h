/*
 * Copyright (C) 2012-2014 Mozilla Foundation
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

#ifndef DOM_SURFACE_GONKSURFACEHWMGR_H
#define DOM_SURFACE_GONKSURFACEHWMGR_H

#include "GonkSurfaceControl.h"
#include "mozilla/ReentrantMonitor.h"

#include <binder/IMemory.h>
#include <gui/IGraphicBufferProducer.h>
#include <utils/threads.h>
#include "GonkNativeWindow.h"

namespace mozilla {
  class nsGonkSurfaceControl;
}

namespace android {

class GonkSurfaceHardware
#ifdef MOZ_WIDGET_GONK
  : public GonkNativeWindowNewFrameCallback
#else
  : public nsISupports
#endif
  , virtual public RefBase
{
#ifndef MOZ_WIDGET_GONK
  NS_DECL_ISUPPORTS
#endif

protected:
  GonkSurfaceHardware(mozilla::nsGonkSurfaceControl* aTarget);
  virtual ~GonkSurfaceHardware();

  // Initialize the AOSP camera interface.
  //
  // Return values:
  //  - NS_OK on success;
  //  - NS_ERROR_NOT_INITIALIZED if the interface could not be initialized.
  virtual nsresult Init();

public:
  static sp<GonkSurfaceHardware> Connect(mozilla::nsGonkSurfaceControl* aTarget);
  virtual void Close();
  virtual sp<IGraphicBufferProducer> GetGraphicBufferProducer();

#ifdef MOZ_WIDGET_GONK
  // derived from GonkNativeWindowNewFrameCallback
  virtual void OnNewFrame() override;
#endif

  /**
   * MIN_UNDEQUEUED_BUFFERS has increased to 4 since Android JB. For FFOS, more
   * than 3 gralloc buffers are necessary between ImageHost and GonkBufferQueue
   * for consuming preview stream. To keep the stability for older platform, we
   * set MIN_UNDEQUEUED_BUFFERS to 4 only in Android KK base.
   * See also bug 988704.
   */
  enum { MIN_UNDEQUEUED_BUFFERS = 4};

protected:
  bool                           mClosing;
  sp<IGraphicBufferProducer>     mGraphicBufferProducer;
  mozilla::nsGonkSurfaceControl* mTarget;
#ifdef MOZ_WIDGET_GONK
  sp<GonkNativeWindow>          mNativeWindow;
#endif

private:
  GonkSurfaceHardware(const GonkSurfaceHardware&) = delete;
  GonkSurfaceHardware& operator=(const GonkSurfaceHardware&) = delete;
};

} // namespace android

#endif // GONK_IMPL_HW_MGR_H
