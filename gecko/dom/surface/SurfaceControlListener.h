/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DOM_SURFACE_SURFACECONTROLLISTENER_H
#define DOM_SURFACE_SURFACECONTROLLISTENER_H

#include <stdint.h>
#include "ISurfaceControl.h"
#include <gui/IGraphicBufferProducer.h>
#include <utils/StrongPointer.h>

namespace mozilla {

namespace dom {
  class BlobImpl;
} // namespace dom

namespace layers {
  class Image;
} // namespace layers

class SurfaceControlListener
{
public:
  SurfaceControlListener()
  {
    MOZ_COUNT_CTOR(SurfaceControlListener);
  }


  class SurfaceListenerConfiguration : public ISurfaceControl::Configuration
  {
  };

protected:
  // Protected destructor, to discourage deletion outside of Release():
  virtual ~SurfaceControlListener()
  {
    MOZ_COUNT_DTOR(SurfaceControlListener);
  }

public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(SurfaceControlListener);

  enum SurfaceState
  {
    kSurfaceUninitialized,
    kSurfaceCreate,
    kSurfaceCreateFailed,
    kSurfaceDestroyed
  };

  // aReason:
  //    NS_OK : state change was expected and normal;
  //    NS_ERROR_FAILURE : one or more system-level components failed;
  virtual void OnSurfaceStateChange(SurfaceState aState,
                                    nsresult aReason,
                                    android::sp<android::IGraphicBufferProducer> aProducer) { }

  enum PreviewState
  {
    kPreviewStopped,
    kPreviewPaused,
    kPreviewStarted
  };
  virtual void OnPreviewStateChange(PreviewState aState) { }

  virtual bool OnNewPreviewFrame(layers::Image* aFrame, uint32_t aWidth, uint32_t aHeight)
  {
    return false;
  }

  enum UserContext
  {
    kInCreateSurface,
    kInSetDataSourceSize,
    kInDestroySurface,
    kInStartPreview,
    kInGetGraphicBufferProducer,
    kInUnspecified
  };

  // Error handler for problems arising due to user-initiated actions.
  virtual void OnUserError(UserContext aContext, nsresult aError) { }

  enum SystemContext
  {
    kSystemService
  };
  // Error handler for problems arising due to system failures, not triggered
  // by something the SurfaceControl API user did.
  virtual void OnSystemError(SystemContext aContext, nsresult aError) { }
};

} // namespace mozilla

#endif // DOM_SURFACE_SURFACECONTROLLISTENER_H
