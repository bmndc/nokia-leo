/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et ft=cpp : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "Hal.h"

#include "HalImpl.h"
#include "HalLog.h"
#include "HalSandbox.h"
#include "nsIDOMDocument.h"
#include "nsIDOMWindow.h"
#include "nsIDocument.h"
#include "nsIDocShell.h"
#include "nsITabChild.h"
#include "nsIWebNavigation.h"
#include "nsThreadUtils.h"
#include "nsXULAppAPI.h"
#include "nsPIDOMWindow.h"
#include "nsJSUtils.h"
#include "mozilla/ClearOnShutdown.h"
#include "mozilla/Observer.h"
#include "mozilla/Services.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/dom/ContentChild.h"
#include "mozilla/dom/ContentParent.h"
#include "mozilla/dom/ScreenOrientation.h"
#include "WindowIdentifier.h"

#ifdef XP_WIN
#include <process.h>
#define getpid _getpid
#endif

using namespace mozilla::services;
using namespace mozilla::dom;

#define PROXY_IF_SANDBOXED(_call)                 \
  do {                                            \
    if (InSandbox()) {                            \
      if (!hal_sandbox::HalChildDestroyed()) {    \
        hal_sandbox::_call;                       \
      }                                           \
    } else {                                      \
      hal_impl::_call;                            \
    }                                             \
  } while (0)

#define RETURN_PROXY_IF_SANDBOXED(_call, defValue)\
  do {                                            \
    if (InSandbox()) {                            \
      if (hal_sandbox::HalChildDestroyed()) {     \
        return defValue;                          \
      }                                           \
      return hal_sandbox::_call;                  \
    } else {                                      \
      return hal_impl::_call;                     \
    }                                             \
  } while (0)

namespace mozilla {
namespace hal {

mozilla::LogModule *
GetHalLog()
{
  static mozilla::LazyLogModule sHalLog("hal");
  return sHalLog;
}

namespace {

void
AssertMainThread()
{
  MOZ_ASSERT(NS_IsMainThread());
}

bool
InSandbox()
{
  return GeckoProcessType_Content == XRE_GetProcessType();
}

void
AssertMainProcess()
{
  MOZ_ASSERT(GeckoProcessType_Default == XRE_GetProcessType());
}

bool
WindowIsActive(nsPIDOMWindowInner* aWindow)
{
  nsIDocument* document = aWindow->GetDoc();
  NS_ENSURE_TRUE(document, false);

  return !document->Hidden();
}

StaticAutoPtr<WindowIdentifier::IDArrayType> gLastIDToVibrate;

void InitLastIDToVibrate()
{
  gLastIDToVibrate = new WindowIdentifier::IDArrayType();
  ClearOnShutdown(&gLastIDToVibrate);
}

uint16_t GetAppStatusByWindow(nsPIDOMWindowInner* aWindow)
{
  uint16_t status = nsIPrincipal::APP_STATUS_NOT_INSTALLED;
  nsIDocument* document = aWindow->GetDoc();
  NS_ENSURE_TRUE(document, status);

  nsIPrincipal* principal = document->NodePrincipal();
  if (!principal || NS_FAILED(principal->GetAppStatus(&status))) {
    NS_WARNING("principal is a nullptr or GetAppStatus is failed");
    return status;
  }

  return status;
}

static bool sScreenOrientationHolderEnbled = false;
} // anonymous namespace

void
Vibrate(const nsTArray<uint32_t>& pattern, nsPIDOMWindowInner* window)
{
  Vibrate(pattern, WindowIdentifier(window));
}

void
Vibrate(const nsTArray<uint32_t>& pattern, const WindowIdentifier &id)
{
  AssertMainThread();

  // Only active windows may start vibrations.  If |id| hasn't gone
  // through the IPC layer -- that is, if our caller is the outside
  // world, not hal_proxy -- check whether the window is active.  If
  // |id| has gone through IPC, don't check the window's visibility;
  // only the window corresponding to the bottommost process has its
  // visibility state set correctly.
  if (!id.HasTraveledThroughIPC() && !WindowIsActive(id.GetWindow()) &&
      GetAppStatusByWindow(id.GetWindow()) <
      nsIPrincipal::APP_STATUS_CERTIFIED) {
    HAL_LOG("Vibrate: Window is inactive, dropping vibrate.");
    return;
  }

  if (!InSandbox()) {
    if (!gLastIDToVibrate) {
      InitLastIDToVibrate();
    }
    *gLastIDToVibrate = id.AsArray();
  }

  // Don't forward our ID if we are not in the sandbox, because hal_impl
  // doesn't need it, and we don't want it to be tempted to read it.  The
  // empty identifier will assert if it's used.
  PROXY_IF_SANDBOXED(Vibrate(pattern, InSandbox() ? id : WindowIdentifier()));
}

void
CancelVibrate(nsPIDOMWindowInner* window)
{
  CancelVibrate(WindowIdentifier(window));
}

void
CancelVibrate(const WindowIdentifier &id)
{
  AssertMainThread();

  // Although only active windows may start vibrations, a window may
  // cancel its own vibration even if it's no longer active.
  //
  // After a window is marked as inactive, it sends a CancelVibrate
  // request.  We want this request to cancel a playing vibration
  // started by that window, so we certainly don't want to reject the
  // cancellation request because the window is now inactive.
  //
  // But it could be the case that, after this window became inactive,
  // some other window came along and started a vibration.  We don't
  // want this window's cancellation request to cancel that window's
  // actively-playing vibration!
  //
  // To solve this problem, we keep track of the id of the last window
  // to start a vibration, and only accepts cancellation requests from
  // the same window.  All other cancellation requests are ignored.

  if (InSandbox() || (gLastIDToVibrate && *gLastIDToVibrate == id.AsArray())) {
    // Don't forward our ID if we are not in the sandbox, because hal_impl
    // doesn't need it, and we don't want it to be tempted to read it.  The
    // empty identifier will assert if it's used.
    PROXY_IF_SANDBOXED(CancelVibrate(InSandbox() ? id : WindowIdentifier()));
  }
}

template <class InfoType>
class ObserversManager
{
public:
  void AddObserver(Observer<InfoType>* aObserver) {
    if (!mObservers) {
      mObservers = new mozilla::ObserverList<InfoType>();
    }

    mObservers->AddObserver(aObserver);

    if (mObservers->Length() == 1) {
      EnableNotifications();
    }
  }

  void RemoveObserver(Observer<InfoType>* aObserver) {
    bool removed = mObservers && mObservers->RemoveObserver(aObserver);
    if (!removed) {
      return;
    }

    if (mObservers->Length() == 0) {
      DisableNotifications();

      OnNotificationsDisabled();

      delete mObservers;
      mObservers = nullptr;
    }
  }

  void BroadcastInformation(const InfoType& aInfo) {
    // It is possible for mObservers to be nullptr here on some platforms,
    // because a call to BroadcastInformation gets queued up asynchronously
    // while RemoveObserver is running (and before the notifications are
    // disabled). The queued call can then get run after mObservers has
    // been nulled out. See bug 757025.
    if (!mObservers) {
      return;
    }
    mObservers->Broadcast(aInfo);
  }

  bool IsNotificationsEnabled() {
    return mObservers && mObservers->Length() != 0;
  }

protected:
  virtual void EnableNotifications() = 0;
  virtual void DisableNotifications() = 0;
  virtual void OnNotificationsDisabled() {}

private:
  mozilla::ObserverList<InfoType>* mObservers;
};

template <class InfoType>
class CachingObserversManager : public ObserversManager<InfoType>
{
public:
  InfoType GetCurrentInformation() {
    if (mHasValidCache) {
      return mInfo;
    }
    GetCurrentInformationInternal(&mInfo);

    // If there are no observers, the notification is disabled by ObserversManager.
    // Once the sensor data changed, we will cache and return old data.
    mHasValidCache = this->IsNotificationsEnabled();
    return mInfo;
  }

  void CacheInformation(const InfoType& aInfo) {
    mHasValidCache = this->IsNotificationsEnabled();
    mInfo = aInfo;
  }

  void BroadcastCachedInformation() {
    this->BroadcastInformation(mInfo);
  }

protected:
  virtual void GetCurrentInformationInternal(InfoType*) = 0;

  virtual void OnNotificationsDisabled() {
    mHasValidCache = false;
  }

private:
  InfoType                mInfo;
  bool                    mHasValidCache;
};

class BatteryObserversManager : public CachingObserversManager<BatteryInformation>
{
protected:
  void EnableNotifications() {
    PROXY_IF_SANDBOXED(EnableBatteryNotifications());
  }

  void DisableNotifications() {
    PROXY_IF_SANDBOXED(DisableBatteryNotifications());
  }

  void GetCurrentInformationInternal(BatteryInformation* aInfo) {
    PROXY_IF_SANDBOXED(GetCurrentBatteryInformation(aInfo));
  }
};

static BatteryObserversManager&
BatteryObservers()
{
  static BatteryObserversManager sBatteryObservers;
  AssertMainThread();
  return sBatteryObservers;
}

class PowerSupplyObserversManager : public CachingObserversManager<PowerSupplyStatus>
{
protected:
  void EnableNotifications() {
    PROXY_IF_SANDBOXED(EnablePowerSupplyNotifications());
  }

  void DisableNotifications() {
    PROXY_IF_SANDBOXED(DisablePowerSupplyNotifications());
  }

  void GetCurrentInformationInternal(PowerSupplyStatus* aStatus) {
    PROXY_IF_SANDBOXED(GetCurrentPowerSupplyStatus(aStatus));
  }
};

static PowerSupplyObserversManager sPowerSupplyObservers;

class UsbObserversManager : public CachingObserversManager<UsbStatus>
{
protected:
  void EnableNotifications() {
    PROXY_IF_SANDBOXED(EnableUsbNotifications());
  }

  void DisableNotifications() {
    PROXY_IF_SANDBOXED(DisableUsbNotifications());
  }

  void GetCurrentInformationInternal(UsbStatus* aStatus) {
    PROXY_IF_SANDBOXED(GetCurrentUsbStatus(aStatus));
  }
};

static UsbObserversManager sUsbObservers;

class NetworkObserversManager : public CachingObserversManager<NetworkInformation>
{
protected:
  void EnableNotifications() {
    PROXY_IF_SANDBOXED(EnableNetworkNotifications());
  }

  void DisableNotifications() {
    PROXY_IF_SANDBOXED(DisableNetworkNotifications());
  }

  void GetCurrentInformationInternal(NetworkInformation* aInfo) {
    PROXY_IF_SANDBOXED(GetCurrentNetworkInformation(aInfo));
  }
};

static NetworkObserversManager&
NetworkObservers()
{
  static NetworkObserversManager sNetworkObservers;
  AssertMainThread();
  return sNetworkObservers;
}

class WakeLockObserversManager : public ObserversManager<WakeLockInformation>
{
protected:
  void EnableNotifications() {
    PROXY_IF_SANDBOXED(EnableWakeLockNotifications());
  }

  void DisableNotifications() {
    PROXY_IF_SANDBOXED(DisableWakeLockNotifications());
  }
};

static WakeLockObserversManager&
WakeLockObservers()
{
  static WakeLockObserversManager sWakeLockObservers;
  AssertMainThread();
  return sWakeLockObservers;
}

class ScreenConfigurationObserversManager : public CachingObserversManager<ScreenConfiguration>
{
protected:
  void EnableNotifications() {
    PROXY_IF_SANDBOXED(EnableScreenConfigurationNotifications());
  }

  void DisableNotifications() {
    PROXY_IF_SANDBOXED(DisableScreenConfigurationNotifications());
  }

  void GetCurrentInformationInternal(ScreenConfiguration* aInfo) {
    PROXY_IF_SANDBOXED(GetCurrentScreenConfiguration(aInfo));
  }
};

static ScreenConfigurationObserversManager&
ScreenConfigurationObservers();

class ScreenConfigurationObserverHolder final : public Observer<ScreenConfiguration>
{
public:
  ScreenConfigurationObserverHolder() {
    ScreenConfigurationObservers().AddObserver(this);
  }

  ~ScreenConfigurationObserverHolder() {
    ScreenConfigurationObservers().RemoveObserver(this);
  }

  void Notify(const ScreenConfiguration& aInfo) {}
};

static ScreenConfigurationObserversManager&
ScreenConfigurationObservers()
{
  AssertMainThread();
  static ScreenConfigurationObserversManager sScreenConfigurationObservers;
  return sScreenConfigurationObservers;
}

void
RegisterBatteryObserver(BatteryObserver* aObserver)
{
  AssertMainThread();
  BatteryObservers().AddObserver(aObserver);
}

void
UnregisterBatteryObserver(BatteryObserver* aObserver)
{
  AssertMainThread();
  BatteryObservers().RemoveObserver(aObserver);
}

void
GetCurrentBatteryInformation(BatteryInformation* aInfo)
{
  AssertMainThread();
  *aInfo = BatteryObservers().GetCurrentInformation();
}

void
NotifyBatteryChange(const BatteryInformation& aInfo)
{
  AssertMainThread();
  BatteryObservers().CacheInformation(aInfo);
  BatteryObservers().BroadcastCachedInformation();
}

#ifdef HAS_KOOST_MODULES
class FlipObserversManager : public ObserversManager<bool>
{
protected:
  void EnableNotifications() {
    PROXY_IF_SANDBOXED(EnableFlipNotifications());
  }

  void DisableNotifications() {
    PROXY_IF_SANDBOXED(DisableFlipNotifications());
  }
};

static FlipObserversManager sFlipObservers;

void
RegisterFlipObserver(FlipObserver* aObserver)
{
  AssertMainThread();
  sFlipObservers.AddObserver(aObserver);
}

void
UnregisterFlipObserver(FlipObserver* aObserver)
{
  AssertMainThread();
  sFlipObservers.RemoveObserver(aObserver);
}

void
UpdateFlipState(const bool& aFlipState)
{
  AssertMainThread();
  sFlipObservers.BroadcastInformation(aFlipState);
}

void
NotifyFlipStateFromInputDevice(bool aFlipState)
{
  AssertMainThread();
  PROXY_IF_SANDBOXED(NotifyFlipStateFromInputDevice(aFlipState));
}

void
RequestCurrentFlipState()
{
  AssertMainThread();
  PROXY_IF_SANDBOXED(RequestCurrentFlipState());
}

class FlashlightObserversManager : public ObserversManager<FlashlightInformation>
{
protected:
  void EnableNotifications() {
    PROXY_IF_SANDBOXED(EnableFlashlightNotifications());
  }

  void DisableNotifications() {
    PROXY_IF_SANDBOXED(DisableFlashlightNotifications());
  }
};

static FlashlightObserversManager sFlashlightObservers;

void
RegisterFlashlightObserver(FlashlightObserver* aObserver)
{
  AssertMainThread();
  sFlashlightObservers.AddObserver(aObserver);
}

void
UnregisterFlashlightObserver(FlashlightObserver* aObserver)
{
  AssertMainThread();
  sFlashlightObservers.RemoveObserver(aObserver);
}

void
UpdateFlashlightState(const FlashlightInformation& aFlashlightState)
{
  AssertMainThread();
  sFlashlightObservers.BroadcastInformation(aFlashlightState);
}

void
RequestCurrentFlashlightState()
{
  AssertMainThread();
  PROXY_IF_SANDBOXED(RequestCurrentFlashlightState());
}

bool
GetFlashlightEnabled()
{
  AssertMainThread();
  RETURN_PROXY_IF_SANDBOXED(GetFlashlightEnabled(), true);
}

void
SetFlashlightEnabled(bool aEnabled)
{
  AssertMainThread();
  PROXY_IF_SANDBOXED(SetFlashlightEnabled(aEnabled));
}
#endif

void
RegisterPowerSupplyObserver(PowerSupplyObserver* aPowerSupplyObserver)
{
  AssertMainThread();
  sPowerSupplyObservers.AddObserver(aPowerSupplyObserver);
}

void
UnregisterPowerSupplyObserver(PowerSupplyObserver* aPowerSupplyObserver)
{
  AssertMainThread();
  sPowerSupplyObservers.RemoveObserver(aPowerSupplyObserver);
}

void
GetCurrentPowerSupplyStatus(PowerSupplyStatus* aPowerSupplyStatus)
{
  AssertMainThread();
  *aPowerSupplyStatus = sPowerSupplyObservers.GetCurrentInformation();
}

void
NotifyPowerSupplyStatus(const PowerSupplyStatus& aPowerSupplyStatus)
{
  AssertMainThread();
  sPowerSupplyObservers.CacheInformation(aPowerSupplyStatus);
  sPowerSupplyObservers.BroadcastCachedInformation();
}

void
RegisterUsbObserver(UsbObserver* aUsbObserver)
{
  AssertMainThread();
  sUsbObservers.AddObserver(aUsbObserver);
}

void
UnregisterUsbObserver(UsbObserver* aUsbObserver)
{
  AssertMainThread();
  sUsbObservers.RemoveObserver(aUsbObserver);
}

void
GetCurrentUsbStatus(UsbStatus* aUsbStatus)
{
  AssertMainThread();
  *aUsbStatus = sUsbObservers.GetCurrentInformation();
}

void
NotifyUsbStatus(const UsbStatus& aUsbStatus)
{
  AssertMainThread();
  sUsbObservers.CacheInformation(aUsbStatus);
  sUsbObservers.BroadcastCachedInformation();
}

bool GetScreenEnabled()
{
  AssertMainThread();
  RETURN_PROXY_IF_SANDBOXED(GetScreenEnabled(), false);
}

void SetScreenEnabled(bool aEnabled)
{
  AssertMainThread();
  PROXY_IF_SANDBOXED(SetScreenEnabled(aEnabled));
}

bool GetExtScreenEnabled()
{
  AssertMainThread();
  RETURN_PROXY_IF_SANDBOXED(GetExtScreenEnabled(), false);
}

void SetExtScreenEnabled(bool aEnabled)
{
  AssertMainThread();
  PROXY_IF_SANDBOXED(SetExtScreenEnabled(aEnabled));
}

bool GetKeyLightEnabled()
{
  AssertMainThread();
  RETURN_PROXY_IF_SANDBOXED(GetKeyLightEnabled(), false);
}

void SetKeyLightEnabled(bool aEnabled)
{
  AssertMainThread();
  PROXY_IF_SANDBOXED(SetKeyLightEnabled(aEnabled));
}

bool GetCpuSleepAllowed()
{
  // Generally for interfaces that are accessible by normal web content
  // we should cache the result and be notified on state changes, like
  // what the battery API does. But since this is only used by
  // privileged interface, the synchronous getter is OK here.
  AssertMainThread();
  RETURN_PROXY_IF_SANDBOXED(GetCpuSleepAllowed(), true);
}

void SetCpuSleepAllowed(bool aAllowed)
{
  AssertMainThread();
  PROXY_IF_SANDBOXED(SetCpuSleepAllowed(aAllowed));
}

double GetScreenBrightness()
{
  AssertMainThread();
  RETURN_PROXY_IF_SANDBOXED(GetScreenBrightness(), 0);
}

void SetScreenBrightness(double aBrightness)
{
  AssertMainThread();
  PROXY_IF_SANDBOXED(SetScreenBrightness(clamped(aBrightness, 0.0, 1.0)));
}

double GetExtScreenBrightness()
{
  AssertMainThread();
  RETURN_PROXY_IF_SANDBOXED(GetExtScreenBrightness(), 0);
}

void SetExtScreenBrightness(double aBrightness)
{
  AssertMainThread();
  PROXY_IF_SANDBOXED(SetExtScreenBrightness(clamped(aBrightness, 0.0, 1.0)));
}

double GetKeyLightBrightness()
{
  AssertMainThread();
  RETURN_PROXY_IF_SANDBOXED(GetKeyLightBrightness(), 0);
}

void SetKeyLightBrightness(double aBrightness)
{
  AssertMainThread();
  PROXY_IF_SANDBOXED(SetKeyLightBrightness(clamped(aBrightness, 0.0, 1.0)));
}

class SystemClockChangeObserversManager : public ObserversManager<int64_t>
{
protected:
  void EnableNotifications() {
    PROXY_IF_SANDBOXED(EnableSystemClockChangeNotifications());
  }

  void DisableNotifications() {
    PROXY_IF_SANDBOXED(DisableSystemClockChangeNotifications());
  }
};

static SystemClockChangeObserversManager&
SystemClockChangeObservers()
{
  static SystemClockChangeObserversManager sSystemClockChangeObservers;
  AssertMainThread();
  return sSystemClockChangeObservers;
}

void
RegisterSystemClockChangeObserver(SystemClockChangeObserver* aObserver)
{
  AssertMainThread();
  SystemClockChangeObservers().AddObserver(aObserver);
}

void
UnregisterSystemClockChangeObserver(SystemClockChangeObserver* aObserver)
{
  AssertMainThread();
  SystemClockChangeObservers().RemoveObserver(aObserver);
}

void
NotifySystemClockChange(const int64_t& aClockDeltaMS)
{
  SystemClockChangeObservers().BroadcastInformation(aClockDeltaMS);
}

class SystemTimezoneChangeObserversManager : public ObserversManager<SystemTimezoneChangeInformation>
{
protected:
  void EnableNotifications() {
    PROXY_IF_SANDBOXED(EnableSystemTimezoneChangeNotifications());
  }

  void DisableNotifications() {
    PROXY_IF_SANDBOXED(DisableSystemTimezoneChangeNotifications());
  }
};

static SystemTimezoneChangeObserversManager&
SystemTimezoneChangeObservers()
{
  static SystemTimezoneChangeObserversManager sSystemTimezoneChangeObservers;
  return sSystemTimezoneChangeObservers;
}

void
RegisterSystemTimezoneChangeObserver(SystemTimezoneChangeObserver* aObserver)
{
  AssertMainThread();
  SystemTimezoneChangeObservers().AddObserver(aObserver);
}

void
UnregisterSystemTimezoneChangeObserver(SystemTimezoneChangeObserver* aObserver)
{
  AssertMainThread();
  SystemTimezoneChangeObservers().RemoveObserver(aObserver);
}

void
NotifySystemTimezoneChange(const SystemTimezoneChangeInformation& aSystemTimezoneChangeInfo)
{
  // Call tzset() here to ensure the timezone value is updated before calling localtime_r().
  // In main process, GonkHal already calls tzset() in mozilla::hal::SetTimezone.
  if (GeckoProcessType_Default != XRE_GetProcessType()) {
    tzset();
  }
  nsJSUtils::ResetTimeZone();
  SystemTimezoneChangeObservers().BroadcastInformation(aSystemTimezoneChangeInfo);
}

void
AdjustSystemClock(int64_t aDeltaMilliseconds)
{
  AssertMainThread();
  PROXY_IF_SANDBOXED(AdjustSystemClock(aDeltaMilliseconds));
}

void
SetTimezone(const nsCString& aTimezoneSpec)
{
  AssertMainThread();
  PROXY_IF_SANDBOXED(SetTimezone(aTimezoneSpec));
}

int32_t
GetTimezoneOffset()
{
  AssertMainThread();
  RETURN_PROXY_IF_SANDBOXED(GetTimezoneOffset(), 0);
}

nsCString
GetTimezone()
{
  AssertMainThread();
  RETURN_PROXY_IF_SANDBOXED(GetTimezone(), nsCString(""));
}

void
EnableSensorNotifications(SensorType aSensor) {
  AssertMainThread();
  PROXY_IF_SANDBOXED(EnableSensorNotifications(aSensor));
}

void
DisableSensorNotifications(SensorType aSensor) {
  AssertMainThread();
  PROXY_IF_SANDBOXED(DisableSensorNotifications(aSensor));
}

typedef mozilla::ObserverList<SensorData> SensorObserverList;
static SensorObserverList* gSensorObservers = nullptr;

static SensorObserverList &
GetSensorObservers(SensorType sensor_type) {
  MOZ_ASSERT(sensor_type < NUM_SENSOR_TYPE);

  if(!gSensorObservers) {
    gSensorObservers = new SensorObserverList[NUM_SENSOR_TYPE];
  }
  return gSensorObservers[sensor_type];
}

void
RegisterSensorObserver(SensorType aSensor, ISensorObserver *aObserver) {
  SensorObserverList &observers = GetSensorObservers(aSensor);

  AssertMainThread();

  observers.AddObserver(aObserver);
  if(observers.Length() == 1) {
    EnableSensorNotifications(aSensor);
  }
}

void
UnregisterSensorObserver(SensorType aSensor, ISensorObserver *aObserver) {
  AssertMainThread();

  if (!gSensorObservers) {
    return;
  }

  SensorObserverList &observers = GetSensorObservers(aSensor);
  if (!observers.RemoveObserver(aObserver) || observers.Length() > 0) {
    return;
  }
  DisableSensorNotifications(aSensor);

  // Destroy sSensorObservers only if all observer lists are empty.
  for (int i = 0; i < NUM_SENSOR_TYPE; i++) {
    if (gSensorObservers[i].Length() > 0) {
      return;
    }
  }
  delete [] gSensorObservers;
  gSensorObservers = nullptr;
}

void
NotifySensorChange(const SensorData &aSensorData) {
  SensorObserverList &observers = GetSensorObservers(aSensorData.sensor());

  AssertMainThread();

  observers.Broadcast(aSensorData);
}

void
RegisterNetworkObserver(NetworkObserver* aObserver)
{
  AssertMainThread();
  NetworkObservers().AddObserver(aObserver);
}

void
UnregisterNetworkObserver(NetworkObserver* aObserver)
{
  AssertMainThread();
  NetworkObservers().RemoveObserver(aObserver);
}

void
GetCurrentNetworkInformation(NetworkInformation* aInfo)
{
  AssertMainThread();
  *aInfo = NetworkObservers().GetCurrentInformation();
}

void
NotifyNetworkChange(const NetworkInformation& aInfo)
{
  NetworkObservers().CacheInformation(aInfo);
  NetworkObservers().BroadcastCachedInformation();
}

void SetNetworkType(int32_t aType)
{
  AssertMainThread();
  PROXY_IF_SANDBOXED(SetNetworkType(aType));
}

void Reboot()
{
  AssertMainProcess();
  AssertMainThread();
  PROXY_IF_SANDBOXED(Reboot());
}

void PowerOff()
{
  AssertMainProcess();
  AssertMainThread();
  PROXY_IF_SANDBOXED(PowerOff());
}

void StartForceQuitWatchdog(ShutdownMode aMode, int32_t aTimeoutSecs)
{
  AssertMainProcess();
  AssertMainThread();
  PROXY_IF_SANDBOXED(StartForceQuitWatchdog(aMode, aTimeoutSecs));
}

void
RegisterWakeLockObserver(WakeLockObserver* aObserver)
{
  AssertMainThread();
  WakeLockObservers().AddObserver(aObserver);
}

void
UnregisterWakeLockObserver(WakeLockObserver* aObserver)
{
  AssertMainThread();
  WakeLockObservers().RemoveObserver(aObserver);
}

void
ModifyWakeLock(const nsAString& aTopic,
               WakeLockControl aLockAdjust,
               WakeLockControl aHiddenAdjust,
               uint64_t aProcessID /* = CONTENT_PROCESS_ID_UNKNOWN */)
{
  AssertMainThread();

  if (aProcessID == CONTENT_PROCESS_ID_UNKNOWN) {
    aProcessID = InSandbox() ? ContentChild::GetSingleton()->GetID() :
                               CONTENT_PROCESS_ID_MAIN;
  }

  PROXY_IF_SANDBOXED(ModifyWakeLock(aTopic, aLockAdjust,
                                    aHiddenAdjust, aProcessID));
}

void
GetWakeLockInfo(const nsAString& aTopic, WakeLockInformation* aWakeLockInfo)
{
  AssertMainThread();
  PROXY_IF_SANDBOXED(GetWakeLockInfo(aTopic, aWakeLockInfo));
}

void
NotifyWakeLockChange(const WakeLockInformation& aInfo)
{
  AssertMainThread();
  WakeLockObservers().BroadcastInformation(aInfo);
}

void
RegisterScreenConfigurationObserver(ScreenConfigurationObserver* aObserver)
{
  AssertMainThread();
  ScreenConfigurationObservers().AddObserver(aObserver);
}

void
UnregisterScreenConfigurationObserver(ScreenConfigurationObserver* aObserver)
{
  AssertMainThread();
  ScreenConfigurationObservers().RemoveObserver(aObserver);
}

void
EnableScreenOrientationHolder()
{
  AssertMainThread();
  sScreenOrientationHolderEnbled = true;
}

void
GetCurrentScreenConfiguration(ScreenConfiguration* aScreenConfiguration)
{
  AssertMainThread();
  if (sScreenOrientationHolderEnbled) {
    // Using a static dummy observer to keep the registration of notification to keep
    // the cached data synchronized. Fetching status of a CachingObserversManager w/o
    // registering notification will fetch status through IPC and may introduce performance
    // issues if it's called frequently.
    static ScreenConfigurationObserverHolder sScreenConfigurationObserverHolder;
    // Silence the warning about unused variable.
    (void)sScreenConfigurationObserverHolder;
  }
  *aScreenConfiguration = ScreenConfigurationObservers().GetCurrentInformation();
}

void
NotifyScreenConfigurationChange(const ScreenConfiguration& aScreenConfiguration)
{
  ScreenConfigurationObservers().CacheInformation(aScreenConfiguration);
  ScreenConfigurationObservers().BroadcastCachedInformation();
}

bool
LockScreenOrientation(const dom::ScreenOrientationInternal& aOrientation)
{
  AssertMainThread();
  RETURN_PROXY_IF_SANDBOXED(LockScreenOrientation(aOrientation), false);
}

void
UnlockScreenOrientation()
{
  AssertMainThread();
  PROXY_IF_SANDBOXED(UnlockScreenOrientation());
}

void
EnableSwitchNotifications(SwitchDevice aDevice) {
  AssertMainThread();
  PROXY_IF_SANDBOXED(EnableSwitchNotifications(aDevice));
}

void
DisableSwitchNotifications(SwitchDevice aDevice) {
  AssertMainThread();
  PROXY_IF_SANDBOXED(DisableSwitchNotifications(aDevice));
}

SwitchState GetCurrentSwitchState(SwitchDevice aDevice)
{
  AssertMainThread();
  RETURN_PROXY_IF_SANDBOXED(GetCurrentSwitchState(aDevice), SWITCH_STATE_UNKNOWN);
}

void NotifySwitchStateFromInputDevice(SwitchDevice aDevice, SwitchState aState)
{
  AssertMainThread();
  PROXY_IF_SANDBOXED(NotifySwitchStateFromInputDevice(aDevice, aState));
}

typedef mozilla::ObserverList<SwitchEvent> SwitchObserverList;

static SwitchObserverList *sSwitchObserverLists = nullptr;

static SwitchObserverList&
GetSwitchObserverList(SwitchDevice aDevice) {
  MOZ_ASSERT(0 <= aDevice && aDevice < NUM_SWITCH_DEVICE);
  if (sSwitchObserverLists == nullptr) {
    sSwitchObserverLists = new SwitchObserverList[NUM_SWITCH_DEVICE];
  }
  return sSwitchObserverLists[aDevice];
}

static void
ReleaseObserversIfNeeded() {
  for (int i = 0; i < NUM_SWITCH_DEVICE; i++) {
    if (sSwitchObserverLists[i].Length() != 0)
      return;
  }

  //The length of every list is 0, no observer in the list.
  delete [] sSwitchObserverLists;
  sSwitchObserverLists = nullptr;
}

void
RegisterSwitchObserver(SwitchDevice aDevice, SwitchObserver *aObserver)
{
  AssertMainThread();
  SwitchObserverList& observer = GetSwitchObserverList(aDevice);
  observer.AddObserver(aObserver);
  if (observer.Length() == 1) {
    EnableSwitchNotifications(aDevice);
  }
}

void
UnregisterSwitchObserver(SwitchDevice aDevice, SwitchObserver *aObserver)
{
  AssertMainThread();

  if (!sSwitchObserverLists) {
    return;
  }

  SwitchObserverList& observer = GetSwitchObserverList(aDevice);
  if (!observer.RemoveObserver(aObserver) || observer.Length() > 0) {
    return;
  }

  DisableSwitchNotifications(aDevice);
  ReleaseObserversIfNeeded();
}

void
NotifySwitchChange(const SwitchEvent& aEvent)
{
  // When callback this notification, main thread may call unregister function
  // first. We should check if this pointer is valid.
  if (!sSwitchObserverLists)
    return;

  SwitchObserverList& observer = GetSwitchObserverList(aEvent.device());
  observer.Broadcast(aEvent);
}

static AlarmObserver* sAlarmObserver;

bool
RegisterTheOneAlarmObserver(AlarmObserver* aObserver)
{
  MOZ_ASSERT(!InSandbox());
  MOZ_ASSERT(!sAlarmObserver);

  sAlarmObserver = aObserver;
  RETURN_PROXY_IF_SANDBOXED(EnableAlarm(), false);
}

void
UnregisterTheOneAlarmObserver()
{
  if (sAlarmObserver) {
    sAlarmObserver = nullptr;
    PROXY_IF_SANDBOXED(DisableAlarm());
  }
}

void
NotifyAlarmFired()
{
  if (sAlarmObserver) {
    sAlarmObserver->Notify(void_t());
  }
}

bool
SetAlarm(int32_t aSeconds, int32_t aNanoseconds)
{
  // It's pointless to program an alarm nothing is going to observe ...
  MOZ_ASSERT(sAlarmObserver);
  RETURN_PROXY_IF_SANDBOXED(SetAlarm(aSeconds, aNanoseconds), false);
}

void
SetProcessPriority(int aPid, ProcessPriority aPriority, uint32_t aLRU)
{
  // n.b. The sandboxed implementation crashes; SetProcessPriority works only
  // from the main process.
  PROXY_IF_SANDBOXED(SetProcessPriority(aPid, aPriority, aLRU));
}

void
SetCurrentThreadPriority(hal::ThreadPriority aThreadPriority)
{
  PROXY_IF_SANDBOXED(SetCurrentThreadPriority(aThreadPriority));
}

void
SetThreadPriority(PlatformThreadId aThreadId,
                  hal::ThreadPriority aThreadPriority)
{
  PROXY_IF_SANDBOXED(SetThreadPriority(aThreadId, aThreadPriority));
}

// From HalTypes.h.
const char*
ProcessPriorityToString(ProcessPriority aPriority)
{
  switch (aPriority) {
  case PROCESS_PRIORITY_MASTER:
    return "MASTER";
  case PROCESS_PRIORITY_PREALLOC:
    return "PREALLOC";
  case PROCESS_PRIORITY_FOREGROUND_HIGH:
    return "FOREGROUND_HIGH";
  case PROCESS_PRIORITY_FOREGROUND:
    return "FOREGROUND";
  case PROCESS_PRIORITY_FOREGROUND_KEYBOARD:
    return "FOREGROUND_KEYBOARD";
  case PROCESS_PRIORITY_BACKGROUND_PERCEIVABLE:
    return "BACKGROUND_PERCEIVABLE";
  case PROCESS_PRIORITY_BACKGROUND:
    return "BACKGROUND";
  case PROCESS_PRIORITY_UNKNOWN:
    return "UNKNOWN";
  default:
    MOZ_ASSERT(false);
    return "???";
  }
}

const char *
ThreadPriorityToString(ThreadPriority aPriority)
{
  switch (aPriority) {
    case THREAD_PRIORITY_COMPOSITOR:
      return "COMPOSITOR";
    default:
      MOZ_ASSERT(false);
      return "???";
  }
}

static StaticAutoPtr<ObserverList<FMRadioOperationInformation> > sFMRadioObservers;
static StaticAutoPtr<ObserverList<FMRadioRDSGroup> > sFMRadioRDSObservers;

static void
InitializeFMRadioObserver()
{
  if (!sFMRadioObservers) {
    sFMRadioObservers = new ObserverList<FMRadioOperationInformation>;
    sFMRadioRDSObservers = new ObserverList<FMRadioRDSGroup>;
    ClearOnShutdown(&sFMRadioRDSObservers);
    ClearOnShutdown(&sFMRadioObservers);
  }
}

void
RegisterFMRadioObserver(FMRadioObserver* aFMRadioObserver) {
  AssertMainThread();
  InitializeFMRadioObserver();
  sFMRadioObservers->AddObserver(aFMRadioObserver);
}

void
UnregisterFMRadioObserver(FMRadioObserver* aFMRadioObserver) {
  AssertMainThread();
  InitializeFMRadioObserver();
  sFMRadioObservers->RemoveObserver(aFMRadioObserver);
}

void
NotifyFMRadioStatus(const FMRadioOperationInformation& aFMRadioState) {
  InitializeFMRadioObserver();
  sFMRadioObservers->Broadcast(aFMRadioState);
}

void
RegisterFMRadioRDSObserver(FMRadioRDSObserver* aFMRadioRDSObserver) {
  AssertMainThread();
  InitializeFMRadioObserver();
  sFMRadioRDSObservers->AddObserver(aFMRadioRDSObserver);
}

void
UnregisterFMRadioRDSObserver(FMRadioRDSObserver* aFMRadioRDSObserver) {
  AssertMainThread();
  InitializeFMRadioObserver();
  sFMRadioRDSObservers->RemoveObserver(aFMRadioRDSObserver);
}


void
NotifyFMRadioRDSGroup(const FMRadioRDSGroup& aRDSGroup) {
  InitializeFMRadioObserver();
  sFMRadioRDSObservers->Broadcast(aRDSGroup);
}

void
EnableFMRadio(const FMRadioSettings& aInfo) {
  AssertMainThread();
  PROXY_IF_SANDBOXED(EnableFMRadio(aInfo));
}

void
DisableFMRadio() {
  AssertMainThread();
  PROXY_IF_SANDBOXED(DisableFMRadio());
}

#if defined(PRODUCT_MANUFACTURER_SPRD) || defined(PRODUCT_MANUFACTURER_MTK)
void
SetFMRadioAntenna(const int32_t aStatus) {
  AssertMainThread();
  PROXY_IF_SANDBOXED(SetFMRadioAntenna(aStatus));
}
#endif

void
FMRadioSeek(const FMRadioSeekDirection& aDirection) {
  PROXY_IF_SANDBOXED(FMRadioSeek(aDirection));
}

void
GetFMRadioSettings(FMRadioSettings* aInfo) {
  AssertMainThread();
  PROXY_IF_SANDBOXED(GetFMRadioSettings(aInfo));
}

void
SetFMRadioFrequency(const uint32_t aFrequency) {
  PROXY_IF_SANDBOXED(SetFMRadioFrequency(aFrequency));
}

uint32_t
GetFMRadioFrequency() {
  AssertMainThread();
  RETURN_PROXY_IF_SANDBOXED(GetFMRadioFrequency(), 0);
}

bool
IsFMRadioOn() {
  AssertMainThread();
  RETURN_PROXY_IF_SANDBOXED(IsFMRadioOn(), false);
}

uint32_t
GetFMRadioSignalStrength() {
  AssertMainThread();
  RETURN_PROXY_IF_SANDBOXED(GetFMRadioSignalStrength(), 0);
}

void
CancelFMRadioSeek() {
  AssertMainThread();
  PROXY_IF_SANDBOXED(CancelFMRadioSeek());
}

bool
EnableRDS(uint32_t aMask) {
  AssertMainThread();
  RETURN_PROXY_IF_SANDBOXED(EnableRDS(aMask), false);
}

void
DisableRDS() {
  AssertMainThread();
  PROXY_IF_SANDBOXED(DisableRDS());
}

FMRadioSettings
GetFMBandSettings(FMRadioCountry aCountry) {
  FMRadioSettings settings;

  switch (aCountry) {
    case FM_RADIO_COUNTRY_US:
    case FM_RADIO_COUNTRY_EU:
      settings.upperLimit() = 108000;
      settings.lowerLimit() = 87800;
      settings.spaceType() = 200;
      settings.preEmphasis() = 75;
      break;
    case FM_RADIO_COUNTRY_JP_STANDARD:
      settings.upperLimit() = 76000;
      settings.lowerLimit() = 90000;
      settings.spaceType() = 100;
      settings.preEmphasis() = 50;
      break;
    case FM_RADIO_COUNTRY_CY:
    case FM_RADIO_COUNTRY_DE:
    case FM_RADIO_COUNTRY_DK:
    case FM_RADIO_COUNTRY_ES:
    case FM_RADIO_COUNTRY_FI:
    case FM_RADIO_COUNTRY_FR:
    case FM_RADIO_COUNTRY_HU:
    case FM_RADIO_COUNTRY_IR:
    case FM_RADIO_COUNTRY_IT:
    case FM_RADIO_COUNTRY_KW:
    case FM_RADIO_COUNTRY_LT:
    case FM_RADIO_COUNTRY_ML:
    case FM_RADIO_COUNTRY_NO:
    case FM_RADIO_COUNTRY_OM:
    case FM_RADIO_COUNTRY_PG:
    case FM_RADIO_COUNTRY_NL:
    case FM_RADIO_COUNTRY_CZ:
    case FM_RADIO_COUNTRY_UK:
    case FM_RADIO_COUNTRY_RW:
    case FM_RADIO_COUNTRY_SN:
    case FM_RADIO_COUNTRY_SI:
    case FM_RADIO_COUNTRY_ZA:
    case FM_RADIO_COUNTRY_SE:
    case FM_RADIO_COUNTRY_CH:
    case FM_RADIO_COUNTRY_TW:
    case FM_RADIO_COUNTRY_UA:
      settings.upperLimit() = 108000;
      settings.lowerLimit() = 87500;
      settings.spaceType() = 100;
      settings.preEmphasis() = 50;
      break;
    case FM_RADIO_COUNTRY_VA:
    case FM_RADIO_COUNTRY_MA:
    case FM_RADIO_COUNTRY_TR:
      settings.upperLimit() = 10800;
      settings.lowerLimit() = 87500;
      settings.spaceType() = 100;
      settings.preEmphasis() = 75;
      break;
    case FM_RADIO_COUNTRY_AU:
    case FM_RADIO_COUNTRY_BD:
      settings.upperLimit() = 108000;
      settings.lowerLimit() = 87500;
      settings.spaceType() = 200;
      settings.preEmphasis() = 75;
      break;
    case FM_RADIO_COUNTRY_AW:
    case FM_RADIO_COUNTRY_BS:
    case FM_RADIO_COUNTRY_CO:
    case FM_RADIO_COUNTRY_KR:
      settings.upperLimit() = 108000;
      settings.lowerLimit() = 88000;
      settings.spaceType() = 200;
      settings.preEmphasis() = 75;
      break;
    case FM_RADIO_COUNTRY_EC:
      settings.upperLimit() = 108000;
      settings.lowerLimit() = 88000;
      settings.spaceType() = 200;
      settings.preEmphasis() = 0;
      break;
    case FM_RADIO_COUNTRY_GM:
      settings.upperLimit() = 108000;
      settings.lowerLimit() = 88000;
      settings.spaceType() = 0;
      settings.preEmphasis() = 75;
      break;
    case FM_RADIO_COUNTRY_QA:
      settings.upperLimit() = 108000;
      settings.lowerLimit() = 88000;
      settings.spaceType() = 200;
      settings.preEmphasis() = 50;
      break;
    case FM_RADIO_COUNTRY_SG:
      settings.upperLimit() = 108000;
      settings.lowerLimit() = 88000;
      settings.spaceType() = 200;
      settings.preEmphasis() = 50;
      break;
    case FM_RADIO_COUNTRY_IN:
      settings.upperLimit() = 100000;
      settings.lowerLimit() = 108000;
      settings.spaceType() = 100;
      settings.preEmphasis() = 50;
      break;
    case FM_RADIO_COUNTRY_NZ:
      settings.upperLimit() = 100000;
      settings.lowerLimit() = 88000;
      settings.spaceType() = 50;
      settings.preEmphasis() = 50;
      break;
    case FM_RADIO_COUNTRY_USER_DEFINED:
      break;
    default:
      MOZ_ASSERT(0);
      break;
    }
    return settings;
}

void FactoryReset(mozilla::dom::FactoryResetReason& aReason)
{
  AssertMainThread();
  PROXY_IF_SANDBOXED(FactoryReset(aReason));
}

void
StartDiskSpaceWatcher()
{
  AssertMainProcess();
  AssertMainThread();
  PROXY_IF_SANDBOXED(StartDiskSpaceWatcher());
}

void
StopDiskSpaceWatcher()
{
  AssertMainProcess();
  AssertMainThread();
  PROXY_IF_SANDBOXED(StopDiskSpaceWatcher());
}

uint32_t
GetTotalSystemMemory()
{
  return hal_impl::GetTotalSystemMemory();
}

uint32_t
GetTotalSystemMemoryLevel()
{
  return hal_impl::GetTotalSystemMemoryLevel();
}

uint32_t
GetProcessMemory(pid_t aPid)
{
  return hal_impl::GetProcessMemory(aPid);
}

bool IsHeadphoneEventFromInputDev()
{
  AssertMainThread();
  RETURN_PROXY_IF_SANDBOXED(IsHeadphoneEventFromInputDev(), false);
}

nsresult StartSystemService(const char* aSvcName, const char* aArgs)
{
  AssertMainThread();
  RETURN_PROXY_IF_SANDBOXED(StartSystemService(aSvcName, aArgs), NS_ERROR_FAILURE);
}

void StopSystemService(const char* aSvcName)
{
  AssertMainThread();
  PROXY_IF_SANDBOXED(StopSystemService(aSvcName));
}

bool SystemServiceIsRunning(const char* aSvcName)
{
  AssertMainThread();
  RETURN_PROXY_IF_SANDBOXED(SystemServiceIsRunning(aSvcName), false);
}

bool SystemServiceIsStopped(const char* aSvcName)
{
  AssertMainThread();
  RETURN_PROXY_IF_SANDBOXED(SystemServiceIsStopped(aSvcName), false);
}

bool IsFlipOpened()
{
  AssertMainThread();
  RETURN_PROXY_IF_SANDBOXED(IsFlipOpened(), true);
}

double GetBatteryTemperature()
{
  AssertMainThread();
  RETURN_PROXY_IF_SANDBOXED(GetBatteryTemperature(), 20.0);
}

bool IsBatteryPresent()
{
  AssertMainThread();
  RETURN_PROXY_IF_SANDBOXED(IsBatteryPresent(), true);

}

} // namespace hal
} // namespace mozilla
