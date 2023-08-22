/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=4 ts=8 et tw=80 : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_layers_CompositorScheduler_h
#define mozilla_layers_CompositorScheduler_h

#include "Layers.h"
#include "mozilla/TimeStamp.h"
#include "ThreadSafeRefcountingWithMainThreadDestruction.h"

namespace mozilla {

namespace gfx {
class DrawTarget;
} // namespace gfx

namespace layers {

class CompositorBridgeParent;

class CompositorScheduler
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(CompositorScheduler)
  explicit CompositorScheduler(CompositorBridgeParent* aCompositorBridgeParent);

  virtual void ScheduleComposition();
  virtual void CancelCurrentCompositeTask();
  virtual bool NeedsComposite();
  virtual void Composite(TimeStamp aTimestamp);
  virtual void ScheduleTask(CancelableTask*, int);
  virtual void ResumeComposition();
  virtual void ForceComposeToTarget(gfx::DrawTarget* aTarget,
                                    const gfx::IntRect* aRect);
  virtual void ComposeToTarget(gfx::DrawTarget* aTarget,
                              const gfx::IntRect* aRect = nullptr);
  virtual void Destroy();

  static already_AddRefed<CompositorScheduler>
    Create(CompositorBridgeParent* aCompositorBridgeParent, nsIWidget* aWidget);

  const TimeStamp& GetLastComposeTime()
  {
    return mLastCompose;
  }

#ifdef COMPOSITOR_PERFORMANCE_WARNING
  const TimeStamp& GetExpectedComposeStartTime()
  {
    return mExpectedComposeStartTime;
  }
#endif

protected:
  virtual ~CompositorScheduler();

  CompositorBridgeParent* mCompositorBridgeParent;
  TimeStamp mLastCompose;
  CancelableTask* mCurrentCompositeTask;

#ifdef COMPOSITOR_PERFORMANCE_WARNING
  TimeStamp mExpectedComposeStartTime;
#endif
};

class CompositorSoftwareTimerScheduler final : public CompositorScheduler
{
public:
  explicit CompositorSoftwareTimerScheduler(
    CompositorBridgeParent* aCompositorBridgeParent);

  // from CompositorScheduler
  virtual void ScheduleComposition() override;
  virtual bool NeedsComposite() override;
  virtual void Composite(TimeStamp aTimestamp) override;

  void CallComposite();
private:
  ~CompositorSoftwareTimerScheduler();
};

} // namespace layers
} // namespace mozilla

#endif // mozilla_layers_CompositorSoftwareTimerScheduler_h
