/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DOM_SURFACE_ISURFACECONTROL_H
#define DOM_SURFACE_ISURFACECONTROL_H

#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsAutoPtr.h"
#include "nsISupportsImpl.h"
#include "base/basictypes.h"
#include <gui/IGraphicBufferProducer.h>
struct DeviceStorageFileDescriptor;

namespace mozilla {

class SurfaceControlListener;

enum {
  // current settings
  SURFACE_PARAM_PREVIEWSIZE,
};

class ISurfaceControl
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(ISurfaceControl)

  static already_AddRefed<ISurfaceControl> Create();

  virtual void AddListener(SurfaceControlListener* aListener) = 0;
  virtual void RemoveListener(SurfaceControlListener* aListener) = 0;

  struct Size {
    uint32_t  width;
    uint32_t  height;

    Size() 
      : width(0)
      , height(0)
    {
    }

    Size(const Size& aSize) 
      : width(aSize.width)
      , height(aSize.height)
    {
    }

    bool Equals(const Size& aSize) const
    {
      return width == aSize.width && height == aSize.height;
    }
  };

  struct Configuration {
    Size      mPreviewSize;
  };

  // Surface control methods.
  //
  // Return values:
  //  - NS_OK on success (if the method requires an asynchronous process,
  //      this value indicates that the process has begun successfully);
  //  - NS_ERROR_INVALID_ARG if one or more arguments is invalid;
  //  - NS_ERROR_FAILURE if an asynchronous method could not be dispatched.
  virtual nsresult SetDataSourceSize(const ISurfaceControl::Size& aSize) = 0;
  virtual nsresult Start(const Configuration* aInitialConfig = nullptr) = 0;
  virtual nsresult Stop() = 0;
  virtual nsresult StartPreview() = 0;

protected:
  virtual ~ISurfaceControl() { }
};
} // namespace mozilla

#endif // DOM_SURFACE_ISURFACECONTROL_H
