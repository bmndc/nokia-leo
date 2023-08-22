/* -*- Mode: C++; tab-width: 20; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "VsyncSource.h"
#include "nsThreadUtils.h"
#include "nsXULAppAPI.h"
#include "mozilla/VsyncDispatcher.h"
#include "MainThreadUtils.h"
#include "RefreshDriverTimerImpl.h"

namespace mozilla {
namespace gfx {

void
VsyncSource::AddCompositorVsyncDispatcher(CompositorVsyncDispatcher* aCompositorVsyncDispatcher)
{
  MOZ_ASSERT(XRE_IsParentProcess());
  MOZ_ASSERT(NS_IsMainThread());
  // Just use the global display until we have enough information to get the
  // corresponding display for compositor.
  GetGlobalDisplay().AddCompositorVsyncDispatcher(aCompositorVsyncDispatcher);
}

void
VsyncSource::RemoveCompositorVsyncDispatcher(CompositorVsyncDispatcher* aCompositorVsyncDispatcher)
{
  MOZ_ASSERT(XRE_IsParentProcess());
  MOZ_ASSERT(NS_IsMainThread());
  // See also AddCompositorVsyncDispatcher().
  GetGlobalDisplay().RemoveCompositorVsyncDispatcher(aCompositorVsyncDispatcher);
}

VsyncSource::Display::Display()
  : mDispatcherLock("display dispatcher lock")
  , mRefreshTimerNeedsVsync(false)
{
  MOZ_ASSERT(NS_IsMainThread());
  mRefreshTimerVsyncDispatcher = new RefreshTimerVsyncDispatcher(this);
}

VsyncSource::Display::~Display()
{
  MOZ_ASSERT(NS_IsMainThread());
  {
    MutexAutoLock lock(mDispatcherLock);
    mRefreshTimerVsyncDispatcher->ClearDisplay();
    mRefreshTimerVsyncDispatcher = nullptr;
    mCompositorVsyncDispatchers.Clear();
  }
  // No need to protect mRefreshDriverTimer by mDispatcherLock since it is
  // accessed from main thread only, otherwise, a deadlock will occur in
  // VsyncSource::Display::UpdateVsyncStatus() when destructing
  // VsyncRefreshDriverTimer.
  // More important, ClearDisplay() of mRefreshTimerVsyncDispatcher must called
  // before releasing mRefreshDriverTimer.
}

void
VsyncSource::Display::NotifyVsync(TimeStamp aVsyncTimestamp)
{
  // Called on the vsync thread
  MutexAutoLock lock(mDispatcherLock);

  for (size_t i = 0; i < mCompositorVsyncDispatchers.Length(); i++) {
    mCompositorVsyncDispatchers[i]->NotifyVsync(aVsyncTimestamp);
  }

  mRefreshTimerVsyncDispatcher->NotifyVsync(aVsyncTimestamp);
}

TimeDuration
VsyncSource::Display::GetVsyncRate()
{
  // If hardware queries fail / are unsupported, we have to just guess.
  return TimeDuration::FromMilliseconds(1000.0 / 60.0);
}

void
VsyncSource::Display::AddCompositorVsyncDispatcher(CompositorVsyncDispatcher* aCompositorVsyncDispatcher)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aCompositorVsyncDispatcher);
  { // scope lock
    MutexAutoLock lock(mDispatcherLock);
    if (!mCompositorVsyncDispatchers.Contains(aCompositorVsyncDispatcher)) {
      mCompositorVsyncDispatchers.AppendElement(aCompositorVsyncDispatcher);
    }
  }
  UpdateVsyncStatus();
}

void
VsyncSource::Display::RemoveCompositorVsyncDispatcher(CompositorVsyncDispatcher* aCompositorVsyncDispatcher)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aCompositorVsyncDispatcher);
  { // Scope lock
    MutexAutoLock lock(mDispatcherLock);
    if (mCompositorVsyncDispatchers.Contains(aCompositorVsyncDispatcher)) {
      mCompositorVsyncDispatchers.RemoveElement(aCompositorVsyncDispatcher);
    }
  }
  UpdateVsyncStatus();
}

void
VsyncSource::Display::NotifyRefreshTimerVsyncStatus(bool aEnable)
{
  MOZ_ASSERT(NS_IsMainThread());
  mRefreshTimerNeedsVsync = aEnable;
  UpdateVsyncStatus();
}

RefreshDriverTimer*
VsyncSource::Display::GetRefreshDriverTimer()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!mRefreshDriverTimer) {
    mRefreshDriverTimer =
      new VsyncRefreshDriverTimer(mRefreshTimerVsyncDispatcher);
  }

  return mRefreshDriverTimer;
}

void
VsyncSource::Display::UpdateVsyncStatus()
{
  MOZ_ASSERT(NS_IsMainThread());
  // WARNING: This function SHOULD NOT BE CALLED WHILE HOLDING LOCKS
  // NotifyVsync grabs a lock to dispatch vsync events
  // When disabling vsync, we wait for the underlying thread to stop on some platforms
  // We can deadlock if we wait for the underlying vsync thread to stop
  // while the vsync thread is in NotifyVsync.
  bool enableVsync = false;
  { // scope lock
    MutexAutoLock lock(mDispatcherLock);
    enableVsync = !mCompositorVsyncDispatchers.IsEmpty() || mRefreshTimerNeedsVsync;
  }

  if (enableVsync) {
    EnableVsync();
  } else {
    DisableVsync();
  }

  if (IsVsyncEnabled() != enableVsync) {
    NS_WARNING("Vsync status did not change.");
  }
}

RefPtr<RefreshTimerVsyncDispatcher>
VsyncSource::Display::GetRefreshTimerVsyncDispatcher()
{
  return mRefreshTimerVsyncDispatcher;
}

VsyncSource::Display&
VsyncSource::GetDisplayById(uint32_t aScreenId)
{
  // Vsync events occur on a specific Display Object.
  // Ideally each screen with independent vsync rate has a
  // corresponding Display Object which provides vsync events for ticking.
  // And a Global Display synchronizes across all Displays, providing
  // vsync events for compositing. Each platform should implement at least
  // Global Display, and then we can fallback to it when specific Display
  // doesn't exist.
  return GetGlobalDisplay();
};

} //namespace gfx
} //namespace mozilla
