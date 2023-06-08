/* Copyright (C) 2015 Acadine Technologies. All rights reserved. */
/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set shiftwidth=2 tabstop=8 autoindent cindent expandtab: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef RefreshDriverTimerImpl_h_
#define RefreshDriverTimerImpl_h_

#ifdef XP_WIN
#include <windows.h>
// mmsystem isn't part of WIN32_LEAN_AND_MEAN, so we have
// to manually include it
#include <mmsystem.h>
#include "WinUtils.h"
#endif

#include "mozilla/Telemetry.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/VsyncDispatcher.h"
#include "mozilla/layout/VsyncChild.h"
#include "nsITimer.h"
#include "nsRefreshDriver.h"
#include "nsTArray.h"
#include "nsPresContext.h"
#include "VsyncSource.h"

using namespace mozilla::layout;

static mozilla::LazyLogModule sRefreshDriverLog("nsRefreshDriver");
#define LOG(...) MOZ_LOG(sRefreshDriverLog, mozilla::LogLevel::Debug, (__VA_ARGS__))

namespace {
  // `true` if we are currently in jank-critical mode.
  //
  // In jank-critical mode, any iteration of the event loop that takes
  // more than 16ms to compute will cause an ongoing animation to miss
  // frames.
  //
  // For simplicity, the current implementation assumes that we are in
  // jank-critical mode if and only if at least one vsync driver has
  // at least one observer.
  static uint64_t sActiveVsyncTimers = 0;

  // The latest value of process-wide jank levels.
  //
  // For each i, sJankLevels[i] counts the number of times delivery of
  // vsync to the main thread has been delayed by at least 2^i ms. Use
  // GetJankLevels to grab a copy of this array.
  uint64_t sJankLevels[12];
}

namespace mozilla {

/*
 * The base class for all global refresh driver timers.  It takes care
 * of managing the list of refresh drivers attached to them and
 * provides interfaces for querying/setting the rate and actually
 * running a timer 'Tick'.  Subclasses must implement StartTimer(),
 * StopTimer(), and ScheduleNextTick() -- the first two just
 * start/stop whatever timer mechanism is in use, and ScheduleNextTick
 * is called at the start of the Tick() implementation to set a time
 * for the next tick.
 */
class RefreshDriverTimer {
  NS_INLINE_DECL_REFCOUNTING(RefreshDriverTimer)
public:
  RefreshDriverTimer()
    : mLastFireEpoch(0)
  {
  }

  virtual void AddRefreshDriver(nsRefreshDriver* aDriver)
  {
    LOG("[%p] AddRefreshDriver %p", this, aDriver);

    bool startTimer = mContentRefreshDrivers.IsEmpty() && mRootRefreshDrivers.IsEmpty();
    if (IsRootRefreshDriver(aDriver)) {
      NS_ASSERTION(!mRootRefreshDrivers.Contains(aDriver), "Adding a duplicate root refresh driver!");
      mRootRefreshDrivers.AppendElement(aDriver);
    } else {
      NS_ASSERTION(!mContentRefreshDrivers.Contains(aDriver), "Adding a duplicate content refresh driver!");
      mContentRefreshDrivers.AppendElement(aDriver);
    }

    if (startTimer) {
      StartTimer();
    }
  }

  virtual void RemoveRefreshDriver(nsRefreshDriver* aDriver)
  {
    LOG("[%p] RemoveRefreshDriver %p", this, aDriver);

    if (IsRootRefreshDriver(aDriver)) {
      NS_ASSERTION(mRootRefreshDrivers.Contains(aDriver), "RemoveRefreshDriver for a refresh driver that's not in the root refresh list!");
      mRootRefreshDrivers.RemoveElement(aDriver);
    } else {
      nsPresContext* rootContext = aDriver->PresContext()->GetRootPresContext();
      // During PresContext shutdown, we can't accurately detect
      // if a root refresh driver exists or not. Therefore, we have to
      // search and find out which list this driver exists in.
      if (!rootContext) {
        if (mRootRefreshDrivers.Contains(aDriver)) {
          mRootRefreshDrivers.RemoveElement(aDriver);
        } else {
          NS_ASSERTION(mContentRefreshDrivers.Contains(aDriver),
                       "RemoveRefreshDriver without a display root for a driver that is not in the content refresh list");
          mContentRefreshDrivers.RemoveElement(aDriver);
        }
      } else {
        NS_ASSERTION(mContentRefreshDrivers.Contains(aDriver), "RemoveRefreshDriver for a driver that is not in the content refresh list");
        mContentRefreshDrivers.RemoveElement(aDriver);
      }
    }

    bool stopTimer = mContentRefreshDrivers.IsEmpty() && mRootRefreshDrivers.IsEmpty();
    if (stopTimer) {
      StopTimer();
    }
  }

  TimeStamp MostRecentRefresh() const { return mLastFireTime; }
  int64_t MostRecentRefreshEpochTime() const { return mLastFireEpoch; }

  void SwapRefreshDrivers(RefreshDriverTimer* aNewTimer)
  {
    MOZ_ASSERT(NS_IsMainThread());

    for (nsRefreshDriver* driver : mContentRefreshDrivers) {
      aNewTimer->AddRefreshDriver(driver);
      driver->mActiveTimer = aNewTimer;
    }
    mContentRefreshDrivers.Clear();

    for (nsRefreshDriver* driver : mRootRefreshDrivers) {
      aNewTimer->AddRefreshDriver(driver);
      driver->mActiveTimer = aNewTimer;
    }
    mRootRefreshDrivers.Clear();

    aNewTimer->mLastFireEpoch = mLastFireEpoch;
    aNewTimer->mLastFireTime = mLastFireTime;
  }

protected:
  friend nsRefreshDriver;
  virtual ~RefreshDriverTimer()
  {
    MOZ_ASSERT(mContentRefreshDrivers.Length() == 0, "Should have removed all content refresh drivers from here by now!");
    MOZ_ASSERT(mRootRefreshDrivers.Length() == 0, "Should have removed all root refresh drivers from here by now!");
  }

  virtual void StartTimer() = 0;
  virtual void StopTimer() = 0;
  virtual void ScheduleNextTick(TimeStamp aNowTime) = 0;

  bool IsRootRefreshDriver(nsRefreshDriver* aDriver)
  {
    nsPresContext* rootContext = aDriver->PresContext()->GetRootPresContext();
    if (!rootContext) {
      return false;
    }

    return aDriver == rootContext->RefreshDriver();
  }

  /*
   * Actually runs a tick, poking all the attached RefreshDrivers.
   * Grabs the "now" time via JS_Now and TimeStamp::Now().
   */
  void Tick()
  {
    int64_t jsnow = JS_Now();
    TimeStamp now = TimeStamp::Now();
    Tick(jsnow, now);
  }

  void TickRefreshDrivers(int64_t aJsNow, TimeStamp aNow, nsTArray<RefPtr<nsRefreshDriver>>& aDrivers)
  {
    if (aDrivers.IsEmpty()) {
      return;
    }

    nsTArray<RefPtr<nsRefreshDriver> > drivers(aDrivers);
    for (nsRefreshDriver* driver : drivers) {
      // don't poke this driver if it's in test mode
      if (driver->IsTestControllingRefreshesEnabled()) {
        continue;
      }

      TickDriver(driver, aJsNow, aNow);
    }
  }

  /*
   * Tick the refresh drivers based on the given timestamp.
   */
  void Tick(int64_t jsnow, TimeStamp now)
  {
    ScheduleNextTick(now);

    mLastFireEpoch = jsnow;
    mLastFireTime = now;

    LOG("[%p] ticking drivers...", this);
    // RD is short for RefreshDriver
    profiler_tracing("Paint", "RD", TRACING_INTERVAL_START);

    TickRefreshDrivers(jsnow, now, mContentRefreshDrivers);
    TickRefreshDrivers(jsnow, now, mRootRefreshDrivers);

    profiler_tracing("Paint", "RD", TRACING_INTERVAL_END);
    LOG("[%p] done.", this);
  }

  static void TickDriver(nsRefreshDriver* driver, int64_t jsnow, TimeStamp now)
  {
    LOG(">> TickDriver: %p (jsnow: %lld)", driver, jsnow);
    driver->Tick(jsnow, now);
  }

  int64_t mLastFireEpoch;
  TimeStamp mLastFireTime;
  TimeStamp mTargetTime;

  nsTArray<RefPtr<nsRefreshDriver> > mContentRefreshDrivers;
  nsTArray<RefPtr<nsRefreshDriver> > mRootRefreshDrivers;

  // useful callback for nsITimer-based derived classes, here
  // bacause of c++ protected shenanigans
  static void TimerTick(nsITimer* aTimer, void* aClosure)
  {
    RefreshDriverTimer *timer = static_cast<RefreshDriverTimer*>(aClosure);
    timer->Tick();
  }
};

/*
 * A RefreshDriverTimer that uses a nsITimer as the underlying timer.  Note that
 * this is a ONE_SHOT timer, not a repeating one!  Subclasses are expected to
 * implement ScheduleNextTick and intelligently calculate the next time to tick,
 * and to reset mTimer.  Using a repeating nsITimer gets us into a lot of pain
 * with its attempt at intelligent slack removal and such, so we don't do it.
 */
class SimpleTimerBasedRefreshDriverTimer :
    public RefreshDriverTimer
{
public:
  /*
   * aRate -- the delay, in milliseconds, requested between timer firings
   */
  explicit SimpleTimerBasedRefreshDriverTimer(double aRate)
  {
    SetRate(aRate);
    mTimer = do_CreateInstance(NS_TIMER_CONTRACTID);
  }

  virtual ~SimpleTimerBasedRefreshDriverTimer()
  {
    StopTimer();
  }

  // will take effect at next timer tick
  virtual void SetRate(double aNewRate)
  {
    mRateMilliseconds = aNewRate;
    mRateDuration = TimeDuration::FromMilliseconds(mRateMilliseconds);
  }

  double GetRate() const
  {
    return mRateMilliseconds;
  }

protected:

  virtual void StartTimer()
  {
    // pretend we just fired, and we schedule the next tick normally
    mLastFireEpoch = JS_Now();
    mLastFireTime = TimeStamp::Now();

    mTargetTime = mLastFireTime + mRateDuration;

    uint32_t delay = static_cast<uint32_t>(mRateMilliseconds);
    mTimer->InitWithFuncCallback(TimerTick, this, delay, nsITimer::TYPE_ONE_SHOT);
  }

  virtual void StopTimer()
  {
    mTimer->Cancel();
  }

  double mRateMilliseconds;
  TimeDuration mRateDuration;
  RefPtr<nsITimer> mTimer;
};

/*
 * A refresh driver that listens to vsync events and ticks the refresh driver
 * on vsync intervals. We throttle the refresh driver if we get too many
 * vsync events and wait to catch up again.
 */
class VsyncRefreshDriverTimer : public RefreshDriverTimer
{
public:
  VsyncRefreshDriverTimer(RefreshTimerVsyncDispatcher* aVsyncDispatcher)
    : mVsyncDispatcher(aVsyncDispatcher)
    , mVsyncChild(nullptr)
  {
    MOZ_ASSERT(XRE_IsParentProcess());
    MOZ_ASSERT(NS_IsMainThread());
    mVsyncObserver = new RefreshDriverVsyncObserver(this);
    mVsyncDispatcher->SetParentRefreshTimer(mVsyncObserver);
  }

  explicit VsyncRefreshDriverTimer(VsyncChild* aVsyncChild)
    : mVsyncChild(aVsyncChild)
  {
    MOZ_ASSERT(!XRE_IsParentProcess());
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(mVsyncChild);
    mVsyncObserver = new RefreshDriverVsyncObserver(this);
    mVsyncChild->SetVsyncObserver(mVsyncObserver);
  }

private:
  // Since VsyncObservers are refCounted, but the RefreshDriverTimer are
  // explicitly shutdown. We create an inner class that has the VsyncObserver
  // and is shutdown when the RefreshDriverTimer is deleted. The alternative is
  // to (a) make all RefreshDriverTimer RefCounted or (b) use different
  // VsyncObserver types.
  class RefreshDriverVsyncObserver final : public VsyncObserver
  {
  public:
    explicit RefreshDriverVsyncObserver(VsyncRefreshDriverTimer* aVsyncRefreshDriverTimer)
      : mVsyncRefreshDriverTimer(aVsyncRefreshDriverTimer)
      , mRefreshTickLock("RefreshTickLock")
      , mRecentVsync(TimeStamp::Now())
      , mLastChildTick(TimeStamp::Now())
      , mVsyncRate(TimeDuration::Forever())
      , mProcessedVsync(true)
    {
      MOZ_ASSERT(NS_IsMainThread());
    }

    virtual bool NotifyVsync(TimeStamp aVsyncTimestamp) override
    {
      if (!NS_IsMainThread()) {
        MOZ_ASSERT(XRE_IsParentProcess());
        // Compress vsync notifications such that only 1 may run at a time
        // This is so that we don't flood the refresh driver with vsync messages
        // if the main thread is blocked for long periods of time
        { // scope lock
          MonitorAutoLock lock(mRefreshTickLock);
          mRecentVsync = aVsyncTimestamp;
          if (!mProcessedVsync) {
            return true;
          }
          mProcessedVsync = false;
        }

        nsCOMPtr<nsIRunnable> vsyncEvent =
             NS_NewRunnableMethodWithArg<TimeStamp>(this,
                                                    &RefreshDriverVsyncObserver::TickRefreshDriver,
                                                    aVsyncTimestamp);
        NS_DispatchToMainThread(vsyncEvent);
      } else {
        TickRefreshDriver(aVsyncTimestamp);
      }

      return true;
    }

    void Shutdown()
    {
      MOZ_ASSERT(NS_IsMainThread());
      mVsyncRefreshDriverTimer = nullptr;
    }

    void OnTimerStart()
    {
      if (!XRE_IsParentProcess()) {
        mLastChildTick = TimeStamp::Now();
      }
    }

  private:
    virtual ~RefreshDriverVsyncObserver() {}

    void RecordTelemetryProbes(TimeStamp aVsyncTimestamp)
    {
      MOZ_ASSERT(NS_IsMainThread());
    #ifndef ANDROID  /* bug 1142079 */
      if (XRE_IsParentProcess()) {
        TimeDuration vsyncLatency = TimeStamp::Now() - aVsyncTimestamp;
        uint32_t sample = (uint32_t)vsyncLatency.ToMilliseconds();
        Telemetry::Accumulate(Telemetry::FX_REFRESH_DRIVER_CHROME_FRAME_DELAY_MS,
                              sample);
        Telemetry::Accumulate(Telemetry::FX_REFRESH_DRIVER_SYNC_SCROLL_FRAME_DELAY_MS,
                              sample);
        RecordJank(sample);
      } else if (mVsyncRate != TimeDuration::Forever()) {
        TimeDuration contentDelay = (TimeStamp::Now() - mLastChildTick) - mVsyncRate;
        if (contentDelay.ToMilliseconds() < 0 ){
          // Vsyncs are noisy and some can come at a rate quicker than
          // the reported hardware rate. In those cases, consider that we have 0 delay.
          contentDelay = TimeDuration::FromMilliseconds(0);
        }
        uint32_t sample = (uint32_t)contentDelay.ToMilliseconds();
        Telemetry::Accumulate(Telemetry::FX_REFRESH_DRIVER_CONTENT_FRAME_DELAY_MS,
                              sample);
        Telemetry::Accumulate(Telemetry::FX_REFRESH_DRIVER_SYNC_SCROLL_FRAME_DELAY_MS,
                              sample);
        RecordJank(sample);
      } else {
        // Request the vsync rate from the parent process. Might be a few vsyncs
        // until the parent responds.
        mVsyncRate = mVsyncRefreshDriverTimer->mVsyncChild->GetVsyncRate();
      }
    #endif
    }

    void RecordJank(uint32_t aJankMS)
    {
      uint32_t duration = 1 /* ms */;
      for (size_t i = 0;
           i < mozilla::ArrayLength(sJankLevels) && duration < aJankMS;
           ++i, duration *= 2) {
        sJankLevels[i]++;
      }
    }

    void TickRefreshDriver(TimeStamp aVsyncTimestamp)
    {
      MOZ_ASSERT(NS_IsMainThread());

      RecordTelemetryProbes(aVsyncTimestamp);
      if (XRE_IsParentProcess()) {
        MonitorAutoLock lock(mRefreshTickLock);
        aVsyncTimestamp = mRecentVsync;
        mProcessedVsync = true;
      } else {
        mLastChildTick = TimeStamp::Now();
      }
      MOZ_ASSERT(aVsyncTimestamp <= TimeStamp::Now());

      // We might have a problem that we call ~VsyncRefreshDriverTimer() before
      // the scheduled TickRefreshDriver() runs. Check mVsyncRefreshDriverTimer
      // before use.
      if (mVsyncRefreshDriverTimer) {
        mVsyncRefreshDriverTimer->RunRefreshDrivers(aVsyncTimestamp);
      }
    }

    // VsyncRefreshDriverTimer holds this RefreshDriverVsyncObserver and it will
    // be always available before Shutdown(). We can just use the raw pointer
    // here.
    VsyncRefreshDriverTimer* mVsyncRefreshDriverTimer;
    Monitor mRefreshTickLock;
    TimeStamp mRecentVsync;
    TimeStamp mLastChildTick;
    TimeDuration mVsyncRate;
    bool mProcessedVsync;
  }; // RefreshDriverVsyncObserver

  virtual ~VsyncRefreshDriverTimer()
  {
    if (XRE_IsParentProcess()) {
      mVsyncDispatcher->SetParentRefreshTimer(nullptr);
      mVsyncDispatcher = nullptr;
    } else {
      // Since the PVsyncChild actors live through the life of the process, just
      // send the unobserveVsync message to disable vsync event. We don't need
      // to handle the cleanup stuff of this actor. PVsyncChild::ActorDestroy()
      // will be called and clean up this actor.
      Unused << mVsyncChild->SendUnobserve();
      mVsyncChild->SetVsyncObserver(nullptr);
      mVsyncChild = nullptr;
    }

    // Detach current vsync timer from this VsyncObserver. The observer will no
    // longer tick this timer.
    mVsyncObserver->Shutdown();
    mVsyncObserver = nullptr;
  }

  virtual void StartTimer() override
  {
    // Protect updates to `sActiveVsyncTimers`.
    MOZ_ASSERT(NS_IsMainThread());

    mLastFireEpoch = JS_Now();
    mLastFireTime = TimeStamp::Now();

    if (XRE_IsParentProcess()) {
      mVsyncDispatcher->SetParentRefreshTimer(mVsyncObserver);
    } else {
      Unused << mVsyncChild->SendObserve();
      mVsyncObserver->OnTimerStart();
    }

    ++sActiveVsyncTimers;
  }

  virtual void StopTimer() override
  {
    // Protect updates to `sActiveVsyncTimers`.
    MOZ_ASSERT(NS_IsMainThread());

    if (XRE_IsParentProcess()) {
      mVsyncDispatcher->SetParentRefreshTimer(nullptr);
    } else {
      Unused << mVsyncChild->SendUnobserve();
    }

    MOZ_ASSERT(sActiveVsyncTimers > 0);
    --sActiveVsyncTimers;
  }

  virtual void ScheduleNextTick(TimeStamp aNowTime) override
  {
    // Do nothing since we just wait for the next vsync from
    // RefreshDriverVsyncObserver.
  }

  void RunRefreshDrivers(TimeStamp aTimeStamp)
  {
    int64_t jsnow = JS_Now();
    TimeDuration diff = TimeStamp::Now() - aTimeStamp;
    int64_t vsyncJsNow = jsnow - diff.ToMicroseconds();
    Tick(vsyncJsNow, aTimeStamp);
  }

  RefPtr<RefreshDriverVsyncObserver> mVsyncObserver;
  // Used for parent process.
  RefPtr<RefreshTimerVsyncDispatcher> mVsyncDispatcher;
  // Used for child process.
  // The mVsyncChild will be always available before VsncChild::ActorDestroy().
  // After ActorDestroy(), StartTimer() and StopTimer() calls will be non-op.
  RefPtr<VsyncChild> mVsyncChild;
}; // VsyncRefreshDriverTimer

/**
 * Since the content process takes some time to setup
 * the vsync IPC connection, this timer is used
 * during the intial startup process.
 * During initial startup, the refresh drivers
 * are ticked off this timer, and are swapped out once content
 * vsync IPC connection is established.
 */
class StartupRefreshDriverTimer :
    public SimpleTimerBasedRefreshDriverTimer
{
public:
  explicit StartupRefreshDriverTimer(double aRate)
    : SimpleTimerBasedRefreshDriverTimer(aRate)
  {
  }

protected:
  virtual void ScheduleNextTick(TimeStamp aNowTime)
  {
    // Since this is only used for startup, it isn't super critical
    // that we tick at consistent intervals.
    TimeStamp newTarget = aNowTime + mRateDuration;
    uint32_t delay = static_cast<uint32_t>((newTarget - aNowTime).ToMilliseconds());
    mTimer->InitWithFuncCallback(TimerTick, this, delay, nsITimer::TYPE_ONE_SHOT);
    mTargetTime = newTarget;
  }
};

/*
 * A RefreshDriverTimer for inactive documents.  When a new refresh driver is
 * added, the rate is reset to the base (normally 1s/1fps).  Every time
 * it ticks, a single refresh driver is poked.  Once they have all been poked,
 * the duration between ticks doubles, up to mDisableAfterMilliseconds.  At that point,
 * the timer is quiet and doesn't tick (until something is added to it again).
 *
 * When a timer is removed, there is a possibility of another timer
 * being skipped for one cycle.  We could avoid this by adjusting
 * mNextDriverIndex in RemoveRefreshDriver, but there's little need to
 * add that complexity.  All we want is for inactive drivers to tick
 * at some point, but we don't care too much about how often.
 */
class InactiveRefreshDriverTimer final :
    public SimpleTimerBasedRefreshDriverTimer
{
public:
  explicit InactiveRefreshDriverTimer(double aRate)
    : SimpleTimerBasedRefreshDriverTimer(aRate),
      mNextTickDuration(aRate),
      mDisableAfterMilliseconds(-1.0),
      mNextDriverIndex(0)
  {
  }

  InactiveRefreshDriverTimer(double aRate, double aDisableAfterMilliseconds)
    : SimpleTimerBasedRefreshDriverTimer(aRate),
      mNextTickDuration(aRate),
      mDisableAfterMilliseconds(aDisableAfterMilliseconds),
      mNextDriverIndex(0)
  {
  }

  virtual void AddRefreshDriver(nsRefreshDriver* aDriver)
  {
    RefreshDriverTimer::AddRefreshDriver(aDriver);

    LOG("[%p] inactive timer got new refresh driver %p, resetting rate",
        this, aDriver);

    // reset the timer, and start with the newly added one next time.
    mNextTickDuration = mRateMilliseconds;

    // we don't really have to start with the newly added one, but we may as well
    // not tick the old ones at the fastest rate any more than we need to.
    mNextDriverIndex = GetRefreshDriverCount() - 1;

    StopTimer();
    StartTimer();
  }

protected:
  uint32_t GetRefreshDriverCount()
  {
    return mContentRefreshDrivers.Length() + mRootRefreshDrivers.Length();
  }

  virtual void StartTimer()
  {
    mLastFireEpoch = JS_Now();
    mLastFireTime = TimeStamp::Now();

    mTargetTime = mLastFireTime + mRateDuration;

    uint32_t delay = static_cast<uint32_t>(mRateMilliseconds);
    mTimer->InitWithFuncCallback(TimerTickOne, this, delay, nsITimer::TYPE_ONE_SHOT);
  }

  virtual void StopTimer()
  {
    mTimer->Cancel();
  }

  virtual void ScheduleNextTick(TimeStamp aNowTime)
  {
    if (mDisableAfterMilliseconds > 0.0 &&
        mNextTickDuration > mDisableAfterMilliseconds)
    {
      // We hit the time after which we should disable
      // inactive window refreshes; don't schedule anything
      // until we get kicked by an AddRefreshDriver call.
      return;
    }

    // double the next tick time if we've already gone through all of them once
    if (mNextDriverIndex >= GetRefreshDriverCount()) {
      mNextTickDuration *= 2.0;
      mNextDriverIndex = 0;
    }

    // this doesn't need to be precise; do a simple schedule
    uint32_t delay = static_cast<uint32_t>(mNextTickDuration);
    mTimer->InitWithFuncCallback(TimerTickOne, this, delay, nsITimer::TYPE_ONE_SHOT);

    LOG("[%p] inactive timer next tick in %f ms [index %d/%d]", this, mNextTickDuration,
        mNextDriverIndex, GetRefreshDriverCount());
  }

  /* Runs just one driver's tick. */
  void TickOne()
  {
    int64_t jsnow = JS_Now();
    TimeStamp now = TimeStamp::Now();

    ScheduleNextTick(now);

    mLastFireEpoch = jsnow;
    mLastFireTime = now;

    nsTArray<RefPtr<nsRefreshDriver> > drivers(mContentRefreshDrivers);
    drivers.AppendElements(mRootRefreshDrivers);

    if (mNextDriverIndex < drivers.Length() &&
        !drivers[mNextDriverIndex]->IsTestControllingRefreshesEnabled())
    {
      TickDriver(drivers[mNextDriverIndex], jsnow, now);
    }

    mNextDriverIndex++;
  }

  static void TimerTickOne(nsITimer* aTimer, void* aClosure)
  {
    InactiveRefreshDriverTimer *timer = static_cast<InactiveRefreshDriverTimer*>(aClosure);
    timer->TickOne();
  }

  double mNextTickDuration;
  double mDisableAfterMilliseconds;
  uint32_t mNextDriverIndex;
};

} //namespace mozilla

#undef LOG

#endif /* !defined(RefreshDriverTimerImpl_h_) */
