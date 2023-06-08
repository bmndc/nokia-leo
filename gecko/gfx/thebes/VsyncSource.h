/* -*- Mode: C++; tab-width: 20; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef GFX_VSYNCSOURCE_H
#define GFX_VSYNCSOURCE_H

#include "nsTArray.h"
#include "mozilla/RefPtr.h"
#include "mozilla/Mutex.h"
#include "mozilla/TimeStamp.h"
#include "nsISupportsImpl.h"

class SoftwareDisplay;

namespace mozilla {
class RefreshTimerVsyncDispatcher;
class CompositorVsyncDispatcher;
class RefreshDriverTimer;

namespace gfx {

// Controls how and when to enable/disable vsync. Lives as long as the
// gfxPlatform does on the parent process
class VsyncSource
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(VsyncSource)

  typedef mozilla::RefreshTimerVsyncDispatcher RefreshTimerVsyncDispatcher;
  typedef mozilla::CompositorVsyncDispatcher CompositorVsyncDispatcher;

  enum VsyncType {
    HARDWARE_VYSNC,
    SORTWARE_VSYNC
  };

public:
  // Controls vsync unique to each display and unique on each platform
  class Display {
 #ifdef MOZ_WIDGET_GONK
     NS_INLINE_DECL_THREADSAFE_REFCOUNTING(Display)
 #endif
    public:
      Display();

      // Notified when this display's vsync occurs, on the vsync thread
      // The aVsyncTimestamp should normalize to the Vsync time that just occured
      // However, different platforms give different vsync notification times.
      // b2g - The vsync timestamp of the previous frame that was just displayed
      // OSX - The vsync timestamp of the upcoming frame, in the future
      // Windows: It's messy, see gfxWindowsPlatform.
      // Android: TODO
      // All platforms should normalize to the vsync that just occured.
      // Large parts of Gecko assume TimeStamps should not be in the future such as animations
      virtual void NotifyVsync(TimeStamp aVsyncTimestamp);

      RefPtr<RefreshTimerVsyncDispatcher> GetRefreshTimerVsyncDispatcher();

      void AddCompositorVsyncDispatcher(CompositorVsyncDispatcher* aCompositorVsyncDispatcher);
      void RemoveCompositorVsyncDispatcher(CompositorVsyncDispatcher* aCompositorVsyncDispatcher);
      void NotifyRefreshTimerVsyncStatus(bool aEnable);
      virtual TimeDuration GetVsyncRate();
      RefreshDriverTimer* GetRefreshDriverTimer();

      // These should all only be called on the main thread
      virtual void EnableVsync() = 0;
      virtual void DisableVsync() = 0;
      virtual bool IsVsyncEnabled() = 0;

      virtual SoftwareDisplay* AsSoftwareDisplay() { return nullptr; }

    protected:
      virtual ~Display();

    private:
      void UpdateVsyncStatus();

      Mutex mDispatcherLock;
      bool mRefreshTimerNeedsVsync;
      nsTArray<RefPtr<CompositorVsyncDispatcher>> mCompositorVsyncDispatchers;
      RefPtr<RefreshTimerVsyncDispatcher> mRefreshTimerVsyncDispatcher;
      // mRefreshDriverTimer is accessed from main thread only, no need to
      // protect it by mDispatcherLock.
      RefPtr<RefreshDriverTimer> mRefreshDriverTimer;
  };

  void AddCompositorVsyncDispatcher(CompositorVsyncDispatcher* aCompositorVsyncDispatcher);
  void RemoveCompositorVsyncDispatcher(CompositorVsyncDispatcher* aCompositorVsyncDispatcher);

  virtual Display& GetGlobalDisplay() = 0; // Works across all displays
  virtual Display& GetDisplayById(uint32_t aScreenId);
  virtual nsresult AddDisplay(uint32_t aScreenId, VsyncType aVsyncType) {
    return NS_OK;
  }
  virtual nsresult RemoveDisplay(uint32_t aScreenId) { return NS_OK; }

protected:
  virtual ~VsyncSource() {}
};

} // namespace gfx
} // namespace mozilla

#endif /* GFX_VSYNCSOURCE_H */
