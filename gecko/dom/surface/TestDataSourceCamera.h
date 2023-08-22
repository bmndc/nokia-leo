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

#ifndef DOM_SURFACE_TESTDATASOURCECAMERA_H
#define DOM_SURFACE_TESTDATASOURCECAMERA_H

#include "ITestDataSource.h"
#include "nsIVideoCallProvider.h"
#include <binder/IMemory.h>
#include <utils/threads.h>
#include <camera/Camera.h>
#include <media/stagefright/foundation/AHandler.h>
#include <utils/StrongPointer.h>
#include <media/stagefright/foundation/AMessage.h>

using namespace android;

class FakeImageFeeder;
class TestDataSourceCamera : public ITestDataSource
{
  friend FakeImageFeeder;
public:
  TestDataSourceCamera(ITestDataSourceResolutionResultListener* aResolutionResultListener);
  virtual ~TestDataSourceCamera();

  virtual void SetPreviewSurface(android::sp<android::IGraphicBufferProducer>& aProducer,
                                 uint32_t aPreferWidth,
                                 uint32_t aPreferHeight);
  virtual void SetDisplaySurface(android::sp<android::IGraphicBufferProducer>& aProducer,
                                 uint32_t aPreferWidth,
                                 uint32_t aPreferHeight);
  virtual void Stop();

private:
  TestDataSourceCamera(const TestDataSourceCamera&) = delete;
  TestDataSourceCamera& operator=(const TestDataSourceCamera&) = delete;


  ITestDataSourceResolutionResultListener* mTestDataSourceResolutionResultListener;

  //Preview surface
  void SetCameraParameters();

  android::sp<android::Camera> mCamera;
  uint32_t mPreferPreviewWidth;
  uint32_t mPreferPreviewHeight;
  uint32_t mResultPreviewWidth;
  uint32_t mResultPreviewHeight;
  bool mPreviewStarted;

  //Display surface

  nsresult StartFakeImage();//Test function to feed image data to the created surface.
  sp<ANativeWindow> mTestANativeWindow;
  unsigned char* mTestImage;
  unsigned char* mTestImage2;
  int mTestImageIndex;
  bool mIsTestRunning;
  sp<IGraphicBufferProducer> mFakeImageProducer;
  uint32_t mPreferDisplayWidth;
  uint32_t mPreferDisplayHeight;
  uint32_t mResultDisplayWidth;
  uint32_t mResultDisplayHeight;
  sp<ALooper> mFakeImageFeederLooper;
  sp<FakeImageFeeder> mFakeImageFeeder;
  bool mDisplayStarted;
};

class FakeImageFeeder : public AHandler
{
public:
  enum {
    kWhatSendFakeImage
  };

  FakeImageFeeder(TestDataSourceCamera* aTestDataSourceCamera);
  virtual ~FakeImageFeeder();
  virtual void onMessageReceived(const sp<AMessage> &msg);
  void SendFakeImage(TestDataSourceCamera* aTestDataSourceCamera);
private:
  TestDataSourceCamera* mTestDataSourceCamera;
};

#endif // DOM_SURFACE_TESTDATASOURCECAMERA_H
