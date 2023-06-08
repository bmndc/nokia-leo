/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/HangMonitor.h"

#include "mozilla/Atomics.h"
#include "mozilla/BackgroundHangMonitor.h"
#include "mozilla/Monitor.h"
#include "mozilla/Preferences.h"
#include "mozilla/ProcessedStack.h"
#include "mozilla/Telemetry.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/UniquePtr.h"
#include "mozilla/StackWalk.h"
#include "mozilla/dom/TabChild.h"
#include "nsIObserverService.h"
#include "nsReadableUtils.h"
#include "nsThreadUtils.h"
#include "nsXULAppAPI.h"

#ifdef MOZ_CRASHREPORTER
#include "nsExceptionHandler.h"
#endif

#ifdef MOZ_NUWA_PROCESS
#include "ipc/Nuwa.h"
#endif

#ifdef XP_WIN
#include <windows.h>
#endif

#if defined(MOZ_ENABLE_PROFILER_SPS) && defined(MOZ_PROFILING) && defined(XP_WIN)
  #define REPORT_CHROME_HANGS
#endif

namespace mozilla {
namespace HangMonitor {

/**
 * A flag which may be set from within a debugger to disable the hang
 * monitor.
 */
volatile bool gDebugDisableHangMonitor = false;

const char kHangMonitorPrefName[] = "hangmonitor.timeout";
const char kHangMonitorLogLevelPrefName[] = "hangmonitor.log.level";

#ifdef REPORT_CHROME_HANGS
const char kTelemetryPrefName[] = "toolkit.telemetry.enabled";
#endif

// Monitor protects gShutdown and gTimeout, but not gTimestamp which rely on
// being atomically set by the processor; synchronization doesn't really matter
// in this use case.
Monitor* gMonitor;

// The timeout preference, in seconds.
int32_t gTimeout;

PRThread* gThread;

// Set when shutdown begins to signal the thread to exit immediately.
bool gShutdown;

// The timestamp of the last event notification, or PR_INTERVAL_NO_WAIT if
// we're currently not processing events.
Atomic<PRIntervalTime> gTimestamp(PR_INTERVAL_NO_WAIT);

static int sLogLevel = 1;

#ifdef MOZ_WIDGET_GONK
#include <android/log.h>
#define HMLOG(level, args...) \
  do { \
    if (level <= sLogLevel) { \
      __android_log_print(ANDROID_LOG_INFO, "HangMonitor", ## args); \
    } \
  } while(0)
#else
#define HMLOG(level, args...)
#endif

#ifdef REPORT_CHROME_HANGS
// Main thread ID used in reporting chrome hangs under Windows
static HANDLE winMainThreadHandle = nullptr;

// Default timeout for reporting chrome hangs to Telemetry (5 seconds)
static const int32_t DEFAULT_CHROME_HANG_INTERVAL = 5;

// Maximum number of PCs to gather from the stack
static const int32_t MAX_CALL_STACK_PCS = 400;
#endif

// PrefChangedFunc
void
PrefChanged(const char*, void*)
{
  int32_t newval = Preferences::GetInt(kHangMonitorPrefName);
  sLogLevel = Preferences::GetInt(kHangMonitorLogLevelPrefName, 1);

  HMLOG(1, "PrefChanged, timeout: %d, log level: %d\n", newval, sLogLevel);
#ifdef REPORT_CHROME_HANGS
  // Monitor chrome hangs on the profiling branch if Telemetry enabled
  if (newval == 0) {
    bool telemetryEnabled = Preferences::GetBool(kTelemetryPrefName);
    if (telemetryEnabled) {
      newval = DEFAULT_CHROME_HANG_INTERVAL;
    }
  }
#endif
  MonitorAutoLock lock(*gMonitor);
  if (newval != gTimeout) {
    gTimeout = newval;
    lock.Notify();
  }
}

void
Crash()
{
  if (gDebugDisableHangMonitor) {
    return;
  }

#ifdef XP_WIN
  if (::IsDebuggerPresent()) {
    return;
  }
#endif

#ifdef MOZ_CRASHREPORTER
  // If you change this, you must also deal with the threadsafety of AnnotateCrashReport in
  // non-chrome processes!
  if (GeckoProcessType_Default == XRE_GetProcessType()) {
    CrashReporter::AnnotateCrashReport(NS_LITERAL_CSTRING("Hang"),
                                       NS_LITERAL_CSTRING("1"));
  }
#endif

  NS_RUNTIMEABORT("HangMonitor triggered");
}

#ifdef REPORT_CHROME_HANGS

static void
ChromeStackWalker(uint32_t aFrameNumber, void* aPC, void* aSP, void* aClosure)
{
  MOZ_ASSERT(aClosure);
  std::vector<uintptr_t>* stack =
    static_cast<std::vector<uintptr_t>*>(aClosure);
  if (stack->size() == MAX_CALL_STACK_PCS) {
    return;
  }
  MOZ_ASSERT(stack->size() < MAX_CALL_STACK_PCS);
  stack->push_back(reinterpret_cast<uintptr_t>(aPC));
}

static void
GetChromeHangReport(Telemetry::ProcessedStack& aStack,
                    int32_t& aSystemUptime,
                    int32_t& aFirefoxUptime)
{
  MOZ_ASSERT(winMainThreadHandle);

  // The thread we're about to suspend might have the alloc lock
  // so allocate ahead of time
  std::vector<uintptr_t> rawStack;
  rawStack.reserve(MAX_CALL_STACK_PCS);
  DWORD ret = ::SuspendThread(winMainThreadHandle);
  if (ret == -1) {
    return;
  }
  MozStackWalk(ChromeStackWalker, /* skipFrames */ 0, /* maxFrames */ 0,
               reinterpret_cast<void*>(&rawStack),
               reinterpret_cast<uintptr_t>(winMainThreadHandle), nullptr);
  ret = ::ResumeThread(winMainThreadHandle);
  if (ret == -1) {
    return;
  }
  aStack = Telemetry::GetStackAndModules(rawStack);

  // Record system uptime (in minutes) at the time of the hang
  aSystemUptime = ((GetTickCount() / 1000) - (gTimeout * 2)) / 60;

  // Record Firefox uptime (in minutes) at the time of the hang
  bool error;
  TimeStamp processCreation = TimeStamp::ProcessCreation(error);
  if (!error) {
    TimeDuration td = TimeStamp::Now() - processCreation;
    aFirefoxUptime = (static_cast<int32_t>(td.ToSeconds()) - (gTimeout * 2)) / 60;
  } else {
    aFirefoxUptime = -1;
  }
}

#endif

void
ThreadMain(void*)
{
  PR_SetCurrentThreadName("Hang Monitor");

#ifdef MOZ_NUWA_PROCESS
  if (IsNuwaProcess()) {
    NuwaMarkCurrentThread(nullptr, nullptr);
  }
#endif

  MonitorAutoLock lock(*gMonitor);

  // In order to avoid issues with the hang monitor incorrectly triggering
  // during a general system stop such as sleeping, the monitor thread must
  // run twice to trigger hang protection.
  PRIntervalTime lastTimestamp = 0;
  int waitCount = 0;

#ifdef REPORT_CHROME_HANGS
  Telemetry::ProcessedStack stack;
  int32_t systemUptime = -1;
  int32_t firefoxUptime = -1;
  UniquePtr<HangAnnotations> annotations;
#endif

  while (true) {
    if (gShutdown) {
      return; // Exit the thread
    }

    // avoid rereading the volatile value in this loop
    PRIntervalTime timestamp = gTimestamp;

    PRIntervalTime now = PR_IntervalNow();

    if (timestamp != PR_INTERVAL_NO_WAIT &&
        now < timestamp) {
      // 32-bit overflow, reset for another waiting period
      timestamp = 1; // lowest legal PRInterval value
    }

    if (timestamp != PR_INTERVAL_NO_WAIT &&
        timestamp == lastTimestamp &&
        gTimeout > 0) {
      ++waitCount;
#ifdef REPORT_CHROME_HANGS
      // Capture the chrome-hang stack + Firefox & system uptimes after
      // the minimum hang duration has been reached (not when the hang ends)
      if (waitCount == 2) {
        GetChromeHangReport(stack, systemUptime, firefoxUptime);
        annotations = ChromeHangAnnotatorCallout();
      }
#else
      // This is the crash-on-hang feature.
      // See bug 867313 for the quirk in the waitCount comparison
      if (waitCount >= 2) {
        int32_t delay =
          int32_t(PR_IntervalToSeconds(now - timestamp));
        if (delay >= gTimeout) {
          MonitorAutoUnlock unlock(*gMonitor);
          HMLOG(1, "Hang is detected\n");
          Action(NOTIFY_HANG);
        }
      }
#endif
    } else {
#ifdef REPORT_CHROME_HANGS
      if (waitCount >= 2) {
        uint32_t hangDuration = PR_IntervalToSeconds(now - lastTimestamp);
        Telemetry::RecordChromeHang(hangDuration, stack, systemUptime,
                                    firefoxUptime, Move(annotations));
        stack.Clear();
      }
#endif
      lastTimestamp = timestamp;
      waitCount = 0;
    }

    PRIntervalTime timeout;
    if (gTimeout <= 0) {
      timeout = PR_INTERVAL_NO_TIMEOUT;
    } else {
      timeout = PR_MillisecondsToInterval(gTimeout * 500);
    }
    HMLOG(2, "Hang monitor is working with timeout, %d ms\n", timeout);
    lock.Wait(timeout);
  }
}

void
Startup()
{
  if (GeckoProcessType_Default != XRE_GetProcessType() &&
      GeckoProcessType_Content != XRE_GetProcessType()) {
    return;
  }

  MOZ_ASSERT(!gMonitor, "Hang monitor already initialized");

  HMLOG(2, "Hang monitor starts up\n");
  gMonitor = new Monitor("HangMonitor");

  Preferences::RegisterCallback(PrefChanged, kHangMonitorPrefName, nullptr);
  Preferences::RegisterCallback(PrefChanged, kHangMonitorLogLevelPrefName,
                                nullptr);
  PrefChanged(nullptr, nullptr);

#ifdef REPORT_CHROME_HANGS
  Preferences::RegisterCallback(PrefChanged, kTelemetryPrefName, nullptr);
  winMainThreadHandle =
    OpenThread(THREAD_ALL_ACCESS, FALSE, GetCurrentThreadId());
  if (!winMainThreadHandle) {
    return;
  }
#endif

  // Don't actually start measuring hangs until we hit the main event loop.
  // This potentially misses a small class of really early startup hangs,
  // but avoids dealing with some xpcshell tests and other situations which
  // start XPCOM but don't ever start the event loop.
  Suspend();

  gThread = PR_CreateThread(PR_USER_THREAD,
                            ThreadMain,
                            nullptr, PR_PRIORITY_LOW, PR_GLOBAL_THREAD,
                            PR_JOINABLE_THREAD, 0);
}

void
Shutdown()
{
  if (GeckoProcessType_Default != XRE_GetProcessType() &&
      GeckoProcessType_Content != XRE_GetProcessType()) {
    return;
  }

  MOZ_ASSERT(gMonitor, "Hang monitor not started");

  {
    // Scope the lock we're going to delete later
    MonitorAutoLock lock(*gMonitor);
    gShutdown = true;
    lock.Notify();
  }

  // thread creation could theoretically fail
  if (gThread) {
    PR_JoinThread(gThread);
    gThread = nullptr;
  }

  HMLOG(2, "Hang monitor shutdowns\n");
  delete gMonitor;
  gMonitor = nullptr;
}

static bool
IsUIMessageWaiting()
{
#ifndef XP_WIN
  return false;
#else
#define NS_WM_IMEFIRST WM_IME_SETCONTEXT
#define NS_WM_IMELAST  WM_IME_KEYUP
  BOOL haveUIMessageWaiting = FALSE;
  MSG msg;
  haveUIMessageWaiting |= ::PeekMessageW(&msg, nullptr, WM_KEYFIRST,
                                         WM_IME_KEYLAST, PM_NOREMOVE);
  haveUIMessageWaiting |= ::PeekMessageW(&msg, nullptr, NS_WM_IMEFIRST,
                                         NS_WM_IMELAST, PM_NOREMOVE);
  haveUIMessageWaiting |= ::PeekMessageW(&msg, nullptr, WM_MOUSEFIRST,
                                         WM_MOUSELAST, PM_NOREMOVE);
  return haveUIMessageWaiting;
#endif
}

void
NotifyActivity(ActivityType aActivityType)
{
  MOZ_ASSERT(NS_IsMainThread(),
             "HangMonitor::Notify called from off the main thread.");

  // Determine the activity type more specifically
  if (aActivityType == kGeneralActivity) {
    aActivityType = IsUIMessageWaiting() ? kActivityUIAVail :
                                           kActivityNoUIAVail;
  }

  // Calculate the cumulative amount of lag time since the last UI message
  static uint32_t cumulativeUILagMS = 0;
  switch (aActivityType) {
    case kActivityNoUIAVail:
      cumulativeUILagMS = 0;
      break;
    case kActivityUIAVail:
    case kUIActivity:
      if (gTimestamp != PR_INTERVAL_NO_WAIT) {
        cumulativeUILagMS += PR_IntervalToMilliseconds(PR_IntervalNow() -
                                                       gTimestamp);
      }
      break;
    default:
      break;
  }

  // This is not a locked activity because PRTimeStamp is a 32-bit quantity
  // which can be read/written atomically, and we don't want to pay locking
  // penalties here.
  gTimestamp = PR_IntervalNow();

  // If we have UI activity we should reset the timer and report it
  if (aActivityType == kUIActivity) {
    mozilla::Telemetry::Accumulate(mozilla::Telemetry::EVENTLOOP_UI_ACTIVITY_EXP_MS,
                                     cumulativeUILagMS);
    cumulativeUILagMS = 0;
  }

  if (gThread && !gShutdown) {
    mozilla::BackgroundHangMonitor().NotifyActivity();
  }
}

void
Suspend()
{
  MOZ_ASSERT(NS_IsMainThread(),
             "HangMonitor::Suspend called from off the main thread.");

  // Because gTimestamp changes this resets the wait count.
  gTimestamp = PR_INTERVAL_NO_WAIT;

  if (gThread && !gShutdown) {
    mozilla::BackgroundHangMonitor().NotifyWait();
  }
}

namespace {
class HangDetectedEvent : public nsRunnable {
public:
  HangDetectedEvent()
  {
  }

  NS_IMETHOD Run()
  {
    if (XRE_IsParentProcess()) {
      HangMonitor::Notify();
    } else {
      nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
      if (obs) {
        obs->NotifyObservers(nullptr, "content-process-no-response", nullptr);
      }
    }
    return NS_OK;
  }
};
}

bool
Notify()
{
  MOZ_ASSERT(NS_IsMainThread());

  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  if (obs) {
    obs->NotifyObservers(nullptr, "process-no-response", nullptr);
  }

  return true;
}

void
Action(HangActionType aAction)
{
  if (aAction == CRASH) {
    Crash();
  } else if (aAction == NOTIFY_HANG) {
    NS_DispatchToMainThread(new HangDetectedEvent());
  } else {
    MOZ_ASSERT(false, "HangMonitor::Unknown action type!");
  }
}

} // namespace HangMonitor
} // namespace mozilla
