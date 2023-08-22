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

#ifndef DOM_SURFACE_ITESTDATASOURCE_H
#define DOM_SURFACE_ITESTDATASOURCE_H

#include <gui/IGraphicBufferProducer.h>
#include "DOMSurfaceControl.h"



class ITestDataSourceResolutionResultListener
{
public:
  virtual void SetPreviewSurfaceControl(mozilla::nsDOMSurfaceControl* aPreviewSurfaceControl) = 0;

  virtual void SetDisplaySurfaceControl(mozilla::nsDOMSurfaceControl* aDisplaySurfaceControl) = 0;

  virtual void onChangeCameraCapabilities(unsigned int  aResultWidth,
                                          unsigned int  aResultHeight) = 0;

  virtual void onChangePeerDimensions(unsigned int  aResultWidth,
                                      unsigned int  aResultHeight) = 0;
  virtual ~ITestDataSourceResolutionResultListener() {}                 
};

class ITestDataSource
{
public:
  virtual void SetPreviewSurface(android::sp<android::IGraphicBufferProducer>& aProducer,
                         uint32_t aPreferWidth,
                         uint32_t aPreferHeight) = 0;
  virtual void SetDisplaySurface(android::sp<android::IGraphicBufferProducer>& aProducer,
                         uint32_t aPreferWidth,
                         uint32_t aPreferHeight) = 0;                       
  virtual void Stop() = 0;

  ITestDataSource() {}
  virtual ~ITestDataSource() {}
private:
  ITestDataSource(const ITestDataSource&) = delete;
  ITestDataSource& operator=(const ITestDataSource&) = delete;
};

#endif // DOM_SURFACE_ITESTDATASOURCE_H
