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

#ifndef DOM_SURFACE_GONKSURFACECONTROL_H
#define DOM_SURFACE_GONKSURFACECONTROL_H

#include "base/basictypes.h"
#include "nsRefPtrHashtable.h"
#include "mozilla/ReentrantMonitor.h"
#include "SurfaceControlImpl.h"
#include "GonkSurfaceHwMgr.h"
#include <system/window.h>
#include <utils/StrongPointer.h>

class nsITimer;

namespace mozilla {

namespace layers {
  class TextureClient;
  class ImageContainer;
  class Image;
}

class nsGonkSurfaceControl : public SurfaceControlImpl
{
public:
  nsGonkSurfaceControl();

  void OnNewPreviewFrame(layers::TextureClient* aBuffer);


protected:
  ~nsGonkSurfaceControl();

  using SurfaceControlImpl::OnNewPreviewFrame;

  nsresult Initialize();
  nsresult SetConfigurationInternal(const Configuration& aConfig);

  nsresult SetDataSourceSizeInternal(const ISurfaceControl::Size& aSize);
  nsresult StartInternal(const Configuration* aInitialConfig);
  nsresult StopInternal();
  nsresult StartPreviewInternal();

  // See SurfaceControlImpl.h for these methods' return values.
  virtual nsresult SetDataSourceSizeImpl(const ISurfaceControl::Size& aSize) override;
  virtual nsresult StartImpl(const Configuration* aInitialConfig = nullptr) override;
  virtual nsresult StopImpl() override;
  virtual nsresult StartPreviewImpl() override;

  android::sp<android::GonkSurfaceHardware> GetSurfaceHw();

  android::sp<android::GonkSurfaceHardware> mSurfaceHw;

  RefPtr<mozilla::layers::ImageContainer> mImageContainer;

private:
  nsGonkSurfaceControl(const nsGonkSurfaceControl&) = delete;
  nsGonkSurfaceControl& operator=(const nsGonkSurfaceControl&) = delete;
};

// camera driver callbacks
void OnNewPreviewFrame(nsGonkSurfaceControl* gc, layers::TextureClient* aBuffer);
} // namespace mozilla

#endif // DOM_SURFACE_GONKSURFACECONTROL_H
