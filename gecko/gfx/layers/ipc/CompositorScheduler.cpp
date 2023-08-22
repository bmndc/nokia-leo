/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=2 et tw=80 : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/layers/CompositorScheduler.h"
#include "mozilla/layers/CompositorBridgeParent.h"
#include "gfxPrefs.h"

namespace mozilla {
namespace layers {

/* static */ already_AddRefed<CompositorScheduler>
CompositorScheduler::Create(CompositorBridgeParent* aCompositorBridgeParent,
                            nsIWidget* aWidget)
{
  RefPtr<CompositorScheduler> scheduler;
  if (aWidget->IsVsyncSupported()) {
    scheduler = new CompositorVsyncScheduler(aCompositorBridgeParent, aWidget);
  } else {
    scheduler = new CompositorSoftwareTimerScheduler(aCompositorBridgeParent);
  }
  return scheduler.forget();
}

CompositorScheduler::CompositorScheduler(
    CompositorBridgeParent* aCompositorBridgeParent)
  : mCompositorBridgeParent(aCompositorBridgeParent)
  , mLastCompose(TimeStamp::Now())
  , mCurrentCompositeTask(nullptr)
{
}

CompositorScheduler::~CompositorScheduler()
{
  MOZ_ASSERT(!mCompositorBridgeParent);
}

void
CompositorScheduler::ScheduleComposition()
{
}

bool
CompositorScheduler::NeedsComposite()
{
  return false;
}

void
CompositorScheduler::Composite(TimeStamp aTimestamp)
{
}

void
CompositorScheduler::CancelCurrentCompositeTask()
{
  if (mCurrentCompositeTask) {
    mCurrentCompositeTask->Cancel();
    mCurrentCompositeTask = nullptr;
  }
}

void
CompositorScheduler::ScheduleTask(CancelableTask* aTask, int aTime)
{
  MOZ_ASSERT(CompositorBridgeParent::CompositorLoop());
  MOZ_ASSERT(aTime >= 0);
  CompositorBridgeParent::CompositorLoop()->PostDelayedTask(
    FROM_HERE, aTask, aTime);
}

void
CompositorScheduler::ResumeComposition()
{
  mLastCompose = TimeStamp::Now();
  ComposeToTarget(nullptr);
}

void
CompositorScheduler::ForceComposeToTarget(gfx::DrawTarget* aTarget,
                                          const gfx::IntRect* aRect)
{
  mLastCompose = TimeStamp::Now();
  ComposeToTarget(aTarget, aRect);
}

void
CompositorScheduler::ComposeToTarget(gfx::DrawTarget* aTarget,
                                     const gfx::IntRect* aRect)
{
  MOZ_ASSERT(CompositorBridgeParent::IsInCompositorThread());
  MOZ_ASSERT(mCompositorBridgeParent);
  mCompositorBridgeParent->CompositeToTarget(aTarget, aRect);
}

void
CompositorScheduler::Destroy()
{
  MOZ_ASSERT(CompositorBridgeParent::IsInCompositorThread());
  CancelCurrentCompositeTask();
  mCompositorBridgeParent = nullptr;
}

CompositorSoftwareTimerScheduler::CompositorSoftwareTimerScheduler(
    CompositorBridgeParent* aCompositorBridgeParent)
  : CompositorScheduler(aCompositorBridgeParent)
{
}

CompositorSoftwareTimerScheduler::~CompositorSoftwareTimerScheduler()
{
  MOZ_ASSERT(!mCurrentCompositeTask);
}

// Used when layout.frame_rate is -1. Needs to be kept in sync with
// DEFAULT_FRAME_RATE in nsRefreshDriver.cpp.
static const int32_t kDefaultFrameRate = 60;

static int32_t
CalculateCompositionFrameRate()
{
  int32_t compositionFrameRatePref = gfxPrefs::LayersCompositionFrameRate();
  if (compositionFrameRatePref < 0) {
    // Use the same frame rate for composition as for layout.
    int32_t layoutFrameRatePref = gfxPrefs::LayoutFrameRate();
    if (layoutFrameRatePref < 0) {
      // TODO: The main thread frame scheduling code consults the actual
      // monitor refresh rate in this case. We should do the same.
      return kDefaultFrameRate;
    }
    return layoutFrameRatePref;
  }
  return compositionFrameRatePref;
}

void
CompositorSoftwareTimerScheduler::ScheduleComposition()
{
  if (mCurrentCompositeTask) {
    return;
  }

  bool initialComposition = mLastCompose.IsNull();
  TimeDuration delta;
  if (!initialComposition) {
    delta = TimeStamp::Now() - mLastCompose;
  }

  int32_t rate = CalculateCompositionFrameRate();

  // If rate == 0 (ASAP mode), minFrameDelta must be 0 so there's no delay.
  TimeDuration minFrameDelta = TimeDuration::FromMilliseconds(
    rate == 0 ? 0.0 : std::max(0.0, 1000.0 / rate));

  mCurrentCompositeTask =
    NewRunnableMethod(this, &CompositorSoftwareTimerScheduler::CallComposite);

  if (!initialComposition && delta < minFrameDelta) {
    TimeDuration delay = minFrameDelta - delta;
#ifdef COMPOSITOR_PERFORMANCE_WARNING
    mExpectedComposeStartTime = TimeStamp::Now() + delay;
#endif
    ScheduleTask(mCurrentCompositeTask, delay.ToMilliseconds());
  } else {
#ifdef COMPOSITOR_PERFORMANCE_WARNING
    mExpectedComposeStartTime = TimeStamp::Now();
#endif
    ScheduleTask(mCurrentCompositeTask, 0);
  }
}

bool
CompositorSoftwareTimerScheduler::NeedsComposite()
{
  return mCurrentCompositeTask ? true : false;
}

void
CompositorSoftwareTimerScheduler::CallComposite()
{
  Composite(TimeStamp::Now());
}

void
CompositorSoftwareTimerScheduler::Composite(TimeStamp aTimestamp)
{
  mCurrentCompositeTask = nullptr;
  mLastCompose = aTimestamp;
  ComposeToTarget(nullptr);
}

} // namespace layers
} // namespace mozilla
