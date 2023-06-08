/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DOM_SURFACE_SurfaceControlImpl_H
#define DOM_SURFACE_SurfaceControlImpl_H

#include "nsTArray.h"
#include "nsWeakPtr.h"
#include "mozilla/Attributes.h"
#include "mozilla/ReentrantMonitor.h"
#include "mozilla/Mutex.h"
#include "nsIFile.h"
#include "nsProxyRelease.h"
#include "ISurfaceControl.h"
#include "DeviceStorage.h"
#include "DeviceStorageFileDescriptor.h"
#include "SurfaceControlListener.h"

namespace mozilla {

namespace dom {
  class BlobImpl;
} // namespace dom

namespace layers {
  class Image;
} // namespace layers

class SurfaceControlImpl : public ISurfaceControl
{
public:
  explicit SurfaceControlImpl();
  virtual void AddListener(SurfaceControlListener* aListener) override;
  virtual void RemoveListener(SurfaceControlListener* aListener) override;

  // See ISurfaceControl.h for these methods' return values.
  virtual nsresult SetDataSourceSize(const ISurfaceControl::Size& aSize) override;
  virtual nsresult Start(const Configuration* aConfig = nullptr) override;
  virtual nsresult Stop() override;
  virtual nsresult StartPreview() override;

protected:
  // Event handlers.
  bool OnNewPreviewFrame(layers::Image* aImage, uint32_t aWidth, uint32_t aHeight);
  void OnPreviewStateChange(SurfaceControlListener::PreviewState aState);
  void OnSurfaceStateChange(SurfaceControlListener::SurfaceState aState,
                            nsresult aReason,
                            android::sp<android::IGraphicBufferProducer> aProducer);

  // When we create a new SurfaceThread, we keep a static reference to it so
  // that multiple SurfaceControl instances can find and reuse it; but we
  // don't want that reference to keep the thread object around unnecessarily,
  // so we make it a weak reference. The strong dynamic references will keep
  // the thread object alive as needed.
  static StaticRefPtr<nsIThread> sSurfaceThread;
  nsCOMPtr<nsIThread> mSurfaceThread;

  virtual ~SurfaceControlImpl();

  // Manage surface event listeners.
  void AddListenerImpl(already_AddRefed<SurfaceControlListener> aListener);
  void RemoveListenerImpl(SurfaceControlListener* aListener);
  nsTArray<RefPtr<SurfaceControlListener> > mListeners;
  mutable Mutex mListenerLock;

  class ControlMessage;
  class ListenerMessage;

  nsresult Dispatch(ControlMessage* aMessage);

  // Asynchronous method implementations, invoked on the Surface Thread.
  //
  // Return values:
  //  - NS_OK on success;
  //  - NS_ERROR_INVALID_ARG if one or more arguments is invalid;
  //  - NS_ERROR_NOT_INITIALIZED if the underlying hardware is not initialized,
  //      failed to initialize (in the case of StartImpl()), or has been stopped;
  //      for StartRecordingImpl(), this indicates that no recorder has been
  //      configured (either by calling StartImpl() or SetConfigurationImpl());
  //  - NS_ERROR_ALREADY_INITIALIZED if the underlying hardware is already
  //      initialized;
  //  - NS_ERROR_NOT_IMPLEMENTED if the method is not implemented;
  //  - NS_ERROR_FAILURE on general failures.
  virtual nsresult SetDataSourceSizeImpl(const ISurfaceControl::Size& aSize) = 0;
  virtual nsresult StartImpl(const Configuration* aConfig = nullptr) = 0;
  virtual nsresult StopImpl() = 0;
  virtual nsresult StartPreviewImpl() = 0;

  SurfaceControlListener::SurfaceListenerConfiguration mCurrentConfiguration;

  SurfaceControlListener::PreviewState   mPreviewState;
  SurfaceControlListener::SurfaceState  mSurfaceState;
private:
  SurfaceControlImpl(const SurfaceControlImpl&) = delete;
  SurfaceControlImpl& operator=(const SurfaceControlImpl&) = delete;
};

} // namespace mozilla

#endif // DOM_SURFACE_SurfaceControlImpl_H
