/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DOM_SURFACE_DOMSURFACECONTROLCALLBACK_H
#define DOM_SURFACE_DOMSURFACECONTROLCALLBACK_H

#include <gui/IGraphicBufferProducer.h>
#include <utils/StrongPointer.h>

namespace mozilla {

class IDOMSurfaceControlCallback {
public:
  virtual void OnProducerCreated(android::sp<android::IGraphicBufferProducer> aProducer) = 0;
  virtual void OnProducerDestroyed() = 0;

protected:
  virtual ~IDOMSurfaceControlCallback() {}
};

} // namespace mozilla

#endif // DOM_SURFACE_DOMSURFACECONTROLCALLBACK_H
