/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DOM_SURFACE_DOMSURFACECONTROLLISTENER_H
#define DOM_SURFACE_DOMSURFACECONTROLLISTENER_H

#include "nsProxyRelease.h"
#include "SurfaceControlListener.h"

namespace mozilla {

class nsDOMSurfaceControl;
class CameraPreviewMediaStream;

class DOMSurfaceControlListener : public SurfaceControlListener
{
public:
  DOMSurfaceControlListener(nsDOMSurfaceControl* aDOMSurfaceControl, CameraPreviewMediaStream* aStream);

  virtual void OnSurfaceStateChange(SurfaceState aState,
                                    nsresult aReason,
                                    android::sp<android::IGraphicBufferProducer> aProducer) override;
  virtual void OnPreviewStateChange(PreviewState aState) override;
  virtual bool OnNewPreviewFrame(layers::Image* aImage, uint32_t aWidth, uint32_t aHeight) override;
  virtual void OnUserError(UserContext aContext, nsresult aError) override;

protected:
  virtual ~DOMSurfaceControlListener();

  nsMainThreadPtrHandle<nsISupports> mDOMSurfaceControl;
  CameraPreviewMediaStream* mStream;

  class DOMCallback;

private:
  uint64_t mFrameNum;

  DOMSurfaceControlListener(const DOMSurfaceControlListener&) = delete;
  DOMSurfaceControlListener& operator=(const DOMSurfaceControlListener&) = delete;
};

} // namespace mozilla

#endif // DOM_SURFACE_DOMSURFACECONTROLLISTENER_H
