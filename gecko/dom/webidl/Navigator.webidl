/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * The origin of this IDL file is
 * http://www.whatwg.org/specs/web-apps/current-work/#the-navigator-object
 * http://www.w3.org/TR/tracking-dnt/
 * http://www.w3.org/TR/geolocation-API/#geolocation_interface
 * http://www.w3.org/TR/battery-status/#navigatorbattery-interface
 * http://www.w3.org/TR/vibration/#vibration-interface
 * http://www.w3.org/2012/sysapps/runtime/#extension-to-the-navigator-interface-1
 * https://dvcs.w3.org/hg/gamepad/raw-file/default/gamepad.html#navigator-interface-extension
 * http://www.w3.org/TR/beacon/#sec-beacon-method
 * https://html.spec.whatwg.org/#navigatorconcurrenthardware
 *
 * © Copyright 2004-2011 Apple Computer, Inc., Mozilla Foundation, and
 * Opera Software ASA. You are granted a license to use, reproduce
 * and create derivative works of this document.
 */

// http://www.whatwg.org/specs/web-apps/current-work/#the-navigator-object
[HeaderFile="Navigator.h"]
interface Navigator {
  // objects implementing this interface also implement the interfaces given below
};
Navigator implements NavigatorID;
Navigator implements NavigatorLanguage;
Navigator implements NavigatorOnLine;
Navigator implements NavigatorContentUtils;
Navigator implements NavigatorStorageUtils;
Navigator implements NavigatorFeatures;
Navigator implements NavigatorConcurrentHardware;

[NoInterfaceObject, Exposed=(Window,Worker)]
interface NavigatorID {
  // WebKit/Blink/Trident/Presto support this (hardcoded "Mozilla").
  [Constant, Cached]
  readonly attribute DOMString appCodeName; // constant "Mozilla"
  [Constant, Cached]
  readonly attribute DOMString appName;
  [Constant, Cached]
  readonly attribute DOMString appVersion;
  [Constant, Cached]
  readonly attribute DOMString platform;
  [Pure, Cached, Throws]
  readonly attribute DOMString userAgent;
  [Constant, Cached]
  readonly attribute DOMString product; // constant "Gecko"

  // Everyone but WebKit/Blink supports this.  See bug 679971.
  [Exposed=Window]
  boolean taintEnabled(); // constant false
};

[NoInterfaceObject, Exposed=(Window,Worker)]
interface NavigatorLanguage {

  // These two attributes are cached because this interface is also implemented
  // by Workernavigator and this way we don't have to go back to the
  // main-thread from the worker thread anytime we need to retrieve them. They
  // are updated when pref intl.accept_languages is changed.

  [Pure, Cached]
  readonly attribute DOMString? language;
  [Pure, Cached, Frozen]
  readonly attribute sequence<DOMString> languages;
};

[NoInterfaceObject, Exposed=(Window,Worker)]
interface NavigatorOnLine {
  readonly attribute boolean onLine;
};

[NoInterfaceObject]
interface NavigatorContentUtils {
  // content handler registration
  [Throws]
  void registerProtocolHandler(DOMString scheme, DOMString url, DOMString title);
  [Throws]
  void registerContentHandler(DOMString mimeType, DOMString url, DOMString title);
  // NOT IMPLEMENTED
  //DOMString isProtocolHandlerRegistered(DOMString scheme, DOMString url);
  //DOMString isContentHandlerRegistered(DOMString mimeType, DOMString url);
  //void unregisterProtocolHandler(DOMString scheme, DOMString url);
  //void unregisterContentHandler(DOMString mimeType, DOMString url);
};

[NoInterfaceObject]
interface NavigatorStorageUtils {
  // NOT IMPLEMENTED
  //void yieldForStorageUpdates();
};

[NoInterfaceObject]
interface NavigatorFeatures {
  [CheckAnyPermissions="feature-detection", Throws]
  Promise<any> getFeature(DOMString name);

  [CheckAnyPermissions="feature-detection", Throws]
  Promise<any> hasFeature(DOMString name);
};

partial interface Navigator {
  [Throws]
  readonly attribute Permissions permissions;
};

// Things that definitely need to be in the spec and and are not for some
// reason.  See https://www.w3.org/Bugs/Public/show_bug.cgi?id=22406
partial interface Navigator {
  [Throws]
  readonly attribute MimeTypeArray mimeTypes;
  [Throws]
  readonly attribute PluginArray plugins;
};

// http://www.w3.org/TR/tracking-dnt/ sort of
partial interface Navigator {
  readonly attribute DOMString doNotTrack;
};

// http://www.w3.org/TR/geolocation-API/#geolocation_interface
[NoInterfaceObject]
interface NavigatorGeolocation {
  [Throws, Pref="geo.enabled"]
  readonly attribute Geolocation geolocation;
};
Navigator implements NavigatorGeolocation;

// http://www.w3.org/TR/battery-status/#navigatorbattery-interface
partial interface Navigator {
  [Throws, Pref="dom.battery.enabled"]
  Promise<BatteryManager> getBattery();
  // Deprecated. Use getBattery() instead.
  // XXXbz Per spec this should be non-nullable, but we return null in
  // torn-down windows.  See bug 884925.
  [Throws, Pref="dom.battery.enabled", BinaryName="deprecatedBattery"]
  readonly attribute BatteryManager? battery;
};

#ifdef HAS_KOOST_MODULES
partial interface Navigator {
  [Throws, Pref="dom.flip.enabled", CheckAnyPermissions="flip", AvailableIn=CertifiedApps]
  Promise<FlipManager> getFlipManager();
};

partial interface Navigator {
  [Throws, Pref="dom.flashlight.enabled", CheckAnyPermissions="flashlight", AvailableIn=CertifiedApps]
  Promise<FlashlightManager> getFlashlightManager();
};

partial interface Navigator {
  [Throws, CheckAnyPermissions="embed-apps"]
  readonly attribute MemoryCleanerManager memoryCleanerManager;
};
#endif

[NoInterfaceObject]
interface NavigatorUsb {
    [Throws, Pref="dom.usb.enabled"]
    readonly attribute UsbManager? usb;
};
Navigator implements NavigatorUsb;

[NoInterfaceObject]
interface NavigatorPowerSupply {
    [Throws, Pref="dom.powersupply.enabled"]
    readonly attribute PowerSupplyManager? powersupply;
};
Navigator implements NavigatorPowerSupply;

// https://wiki.mozilla.org/WebAPI/DataStore
[NoInterfaceObject,
 Exposed=(Window,Worker)]
interface NavigatorDataStore {
    [NewObject, Func="Navigator::HasDataStoreSupport"]
    Promise<sequence<DataStore>> getDataStores(DOMString name,
                                               optional DOMString? owner = null);
};
Navigator implements NavigatorDataStore;

// http://www.w3.org/TR/vibration/#vibration-interface
partial interface Navigator {
    // We don't support sequences in unions yet
    //boolean vibrate ((unsigned long or sequence<unsigned long>) pattern);
    boolean vibrate(unsigned long duration);
    boolean vibrate(sequence<unsigned long> pattern);
};

// http://www.w3.org/TR/pointerevents/#extensions-to-the-navigator-interface
partial interface Navigator {
    [Pref="dom.w3c_pointer_events.enabled"]
    readonly attribute long maxTouchPoints;
};

// Mozilla-specific extensions

callback interface MozIdleObserver {
  // Time is in seconds and is read only when idle observers are added
  // and removed.
  readonly attribute unsigned long time;
  void onidle();
  void onactive();
};

#ifdef MOZ_B2G
dictionary MobileIdOptions {
  boolean forceSelection = false;
};

[NoInterfaceObject]
interface NavigatorMobileId {
    // Ideally we would use [CheckAnyPermissions] here, but the "mobileid"
    // permission is set to PROMPT_ACTION and [CheckAnyPermissions] only checks
    // for ALLOW_ACTION.
    // XXXbz what is this promise resolved with?
    [NewObject, Func="Navigator::HasMobileIdSupport"]
    Promise<any> getMobileIdAssertion(optional MobileIdOptions options);
};
Navigator implements NavigatorMobileId;
#endif // MOZ_B2G

// nsIDOMNavigator
partial interface Navigator {
  [Throws, Constant, Cached]
  readonly attribute DOMString oscpu;
  // WebKit/Blink support this; Trident/Presto do not.
  readonly attribute DOMString vendor;
  // WebKit/Blink supports this (hardcoded ""); Trident/Presto do not.
  readonly attribute DOMString vendorSub;
  // WebKit/Blink supports this (hardcoded "20030107"); Trident/Presto don't
  readonly attribute DOMString productSub;
  // WebKit/Blink/Trident/Presto support this.
  readonly attribute boolean cookieEnabled;
  [Throws, Constant, Cached]
  readonly attribute DOMString buildID;
  [Throws, CheckAnyPermissions="power", UnsafeInPrerendering]
  readonly attribute MozPowerManager mozPower;

  // WebKit/Blink/Trident/Presto support this.
  [Throws]
  boolean javaEnabled();

  /**
   * Navigator requests to add an idle observer to the existing window.
   */
  [Throws, CheckAnyPermissions="idle"]
  void addIdleObserver(MozIdleObserver aIdleObserver);

  /**
   * Navigator requests to remove an idle observer from the existing window.
   */
  [Throws, CheckAnyPermissions="idle"]
  void removeIdleObserver(MozIdleObserver aIdleObserver);

  /**
   * Request a wake lock for a resource.
   *
   * A page holds a wake lock to request that a resource not be turned
   * off (or otherwise made unavailable).
   *
   * The topic is the name of a resource that might be made unavailable for
   * various reasons. For example, on a mobile device the power manager might
   * decide to turn off the screen after a period of idle time to save power.
   *
   * The resource manager checks the lock state of a topic before turning off
   * the associated resource. For example, a page could hold a lock on the
   * "screen" topic to prevent the screensaver from appearing or the screen
   * from turning off.
   *
   * The resource manager defines what each topic means and sets policy.  For
   * example, the resource manager might decide to ignore 'screen' wake locks
   * held by pages which are not visible.
   *
   * One topic can be locked multiple times; it is considered released only when
   * all locks on the topic have been released.
   *
   * The returned MozWakeLock object is a token of the lock.  You can
   * unlock the lock via the object's |unlock| method.  The lock is released
   * automatically when its associated window is unloaded.
   *
   * @param aTopic resource name
   */
  [Throws, Pref="dom.wakelock.enabled", Func="Navigator::HasWakeLockSupport", UnsafeInPrerendering]
  MozWakeLock requestWakeLock(DOMString aTopic);

  [Throws]
  void minimizeMemoryUsage();
};

partial interface Navigator {
  [Throws, Pref="device.storage.enabled"]
  readonly attribute DeviceStorageAreaListener deviceStorageAreaListener;
};

// nsIDOMNavigatorDeviceStorage
partial interface Navigator {
  [Throws, Pref="device.storage.enabled"]
  DeviceStorage? getDeviceStorage(DOMString type);
  [Throws, Pref="device.storage.enabled"]
  sequence<DeviceStorage> getDeviceStorages(DOMString type);
  [Throws, Pref="device.storage.enabled"]
  DeviceStorage? getDeviceStorageByNameAndType(DOMString name, DOMString type);
};

// nsIDOMNavigatorDesktopNotification
partial interface Navigator {
  [Throws, Pref="notification.feature.enabled", UnsafeInPrerendering]
  readonly attribute DesktopNotificationCenter mozNotification;
};

#ifdef MOZ_WEBSMS_BACKEND
partial interface Navigator {
  [CheckAnyPermissions="sms", Pref="dom.sms.enabled", AvailableIn="CertifiedApps"]
  readonly attribute MozMobileMessageManager? mozMobileMessage;
};
#endif

// NetworkInformation
partial interface Navigator {
  [Throws, Pref="dom.netinfo.enabled"]
  readonly attribute NetworkInformation connection;
};

// nsIDOMNavigatorCamera
partial interface Navigator {
  [Throws, Pref="dm.camera.enabled", Func="Navigator::HasCameraSupport", UnsafeInPrerendering]
  readonly attribute CameraManager mozCameras;
};

// nsIDOMNavigatorSystemMessages and sort of maybe
// http://www.w3.org/2012/sysapps/runtime/#extension-to-the-navigator-interface-1
callback systemMessageCallback = void (optional object message);
partial interface Navigator {
  [Throws, Pref="dom.sysmsg.enabled"]
  void    mozSetMessageHandler (DOMString type, systemMessageCallback? callback);
  [Throws, Pref="dom.sysmsg.enabled"]
  boolean mozHasPendingMessage (DOMString type);

  // This method can be called only from the systeMessageCallback function and
  // it allows the page to set a promise to keep alive the app until the
  // current operation is not fully completed.
  [Throws, Pref="dom.sysmsg.enabled"]
  void mozSetMessageHandlerPromise (Promise<any> promise);
};

#ifdef MOZ_B2G_RIL
partial interface Navigator {
  [Throws, Pref="dom.mobileconnection.enabled", CheckAnyPermissions="mobileconnection mobilenetwork", UnsafeInPrerendering]
  readonly attribute MozMobileConnectionArray mozMobileConnections;
};

partial interface Navigator {
  [Throws, Pref="dom.cellbroadcast.enabled", CheckAnyPermissions="cellbroadcast",
   AvailableIn="CertifiedApps", UnsafeInPrerendering]
  readonly attribute MozCellBroadcast mozCellBroadcast;
};

partial interface Navigator {
  [Throws, Pref="dom.voicemail.enabled", CheckAnyPermissions="voicemail",
   AvailableIn="CertifiedApps", UnsafeInPrerendering]
  readonly attribute MozVoicemail mozVoicemail;
};

partial interface Navigator {
  [Throws, Pref="dom.icc.enabled", CheckAnyPermissions="mobileconnection",
   AvailableIn="CertifiedApps", UnsafeInPrerendering]
  readonly attribute MozIccManager? mozIccManager;
};

partial interface Navigator {
  [Throws, Pref="dom.telephony.enabled", CheckAnyPermissions="telephony", UnsafeInPrerendering]
  readonly attribute Telephony? mozTelephony;
};

partial interface Navigator {
  [Throws, Pref="dom.subsidylock.enabled", CheckAnyPermissions="mobileconnection",
   AvailableIn="CertifiedApps", UnsafeInPrerendering]
  readonly attribute SubsidyLockManager? subsidyLockManager;
};
#endif // MOZ_B2G_RIL

#ifdef MOZ_GAMEPAD
// https://dvcs.w3.org/hg/gamepad/raw-file/default/gamepad.html#navigator-interface-extension
partial interface Navigator {
  [Throws, Pref="dom.gamepad.enabled"]
  sequence<Gamepad?> getGamepads();
};
#endif // MOZ_GAMEPAD

partial interface Navigator {
  [Throws, Pref="dom.vr.enabled"]
  Promise<sequence<VRDevice>> getVRDevices();
};

#ifdef MOZ_B2G_BT
partial interface Navigator {
  [Throws, CheckAnyPermissions="bluetooth", UnsafeInPrerendering]
  readonly attribute BluetoothManager mozBluetooth;
};
#endif // MOZ_B2G_BT

#ifdef MOZ_B2G_FM
partial interface Navigator {
  [Throws, CheckAnyPermissions="fmradio", UnsafeInPrerendering]
  readonly attribute FMRadio mozFMRadio;
};
#endif // MOZ_B2G_FM

#ifdef MOZ_TIME_MANAGER
// nsIDOMMozNavigatorTime
partial interface Navigator {
  [Throws, CheckAnyPermissions="time", UnsafeInPrerendering]
  readonly attribute MozTimeManager mozTime;
};
#endif // MOZ_TIME_MANAGER

#ifdef MOZ_AUDIO_CHANNEL_MANAGER
// nsIMozNavigatorAudioChannelManager
partial interface Navigator {
  [Throws]
  readonly attribute AudioChannelManager mozAudioChannelManager;
};
#endif // MOZ_AUDIO_CHANNEL_MANAGER

callback NavigatorUserMediaSuccessCallback = void (MediaStream stream);
callback NavigatorUserMediaErrorCallback = void (MediaStreamError error);

partial interface Navigator {
  [Throws, Func="Navigator::HasUserMediaSupport"]
  readonly attribute MediaDevices mediaDevices;

  // Deprecated. Use mediaDevices.getUserMedia instead.
  [Deprecated="NavigatorGetUserMedia", Throws,
   Func="Navigator::HasUserMediaSupport", UnsafeInPrerendering]
  void mozGetUserMedia(MediaStreamConstraints constraints,
                       NavigatorUserMediaSuccessCallback successCallback,
                       NavigatorUserMediaErrorCallback errorCallback);
};

// nsINavigatorUserMedia
callback MozGetUserMediaDevicesSuccessCallback = void (nsIVariant? devices);
partial interface Navigator {
  [Throws, ChromeOnly]
  void mozGetUserMediaDevices(MediaStreamConstraints constraints,
                              MozGetUserMediaDevicesSuccessCallback onsuccess,
                              NavigatorUserMediaErrorCallback onerror,
                              // The originating innerWindowID is needed to
                              // avoid calling the callbacks if the window has
                              // navigated away. It is optional only as legacy.
                              optional unsigned long long innerWindowID = 0,
                              // The callID is needed in case of multiple
                              // concurrent requests to find the right one.
                              // It is optional only as legacy.
                              // TODO: Rewrite to not need this method anymore,
                              // now that devices are enumerated earlier.
                              optional DOMString callID = "");
};

#ifdef MOZ_B2G
// Service Workers/Navigation Controllers
partial interface Navigator {
  [Func="ServiceWorkerContainer::IsEnabled",
   CheckAnyPermissions="serviceworker",
   SameObject]
  readonly attribute ServiceWorkerContainer serviceWorker;
};
#else
// Service Workers/Navigation Controllers
partial interface Navigator {
  [Func="ServiceWorkerContainer::IsEnabled", SameObject]
  readonly attribute ServiceWorkerContainer serviceWorker;
};
#endif

partial interface Navigator {
  [Throws, Pref="beacon.enabled"]
  boolean sendBeacon(DOMString url,
                     optional (ArrayBufferView or Blob or DOMString or FormData)? data = null);
};

#ifdef ENABLE_TV
partial interface Navigator {
  [Pref="dom.tv.enabled", CheckAnyPermissions="tv", AvailableIn=CertifiedApps]
  readonly attribute TVManager? tv;
};
#endif

#ifdef ENABLE_INPUTPORT
partial interface Navigator {
  [Throws, Pref="dom.inputport.enabled", CheckAnyPermissions="inputport", AvailableIn=CertifiedApps]
  readonly attribute InputPortManager inputPortManager;
};
#endif

#ifdef MOZ_PRESENTATION
partial interface Navigator {
  [Throws, Pref="dom.presentation.enabled", Func="Navigator::HasPresentationSupport", SameObject]
  readonly attribute Presentation? presentation;
};
#endif

partial interface Navigator {
  [NewObject, Pref="dom.mozTCPSocket.enabled", CheckAnyPermissions="tcp-socket"]
  readonly attribute LegacyMozTCPSocket mozTCPSocket;
};

#ifdef HAS_KOOST_MODULES
partial interface Navigator {
  [Throws, Pref="dom.softkey.enabled", CheckAnyPermissions="softkey", AvailableIn=CertifiedApps]
  readonly attribute SoftkeyManager softkeyManager;
};
#endif

#ifdef MOZ_EME
partial interface Navigator {
  [Pref="media.eme.apiVisible", NewObject]
  Promise<MediaKeySystemAccess>
  requestMediaKeySystemAccess(DOMString keySystem,
                              sequence<MediaKeySystemConfiguration> supportedConfigurations);
};
#endif

#ifdef NIGHTLY_BUILD
partial interface Navigator {
  [Func="Navigator::IsE10sEnabled"]
  readonly attribute boolean mozE10sEnabled;
};
#endif

#ifdef MOZ_PAY
partial interface Navigator {
  [Throws, NewObject, Pref="dom.mozPay.enabled"]
  // The 'jwts' parameter can be either a single DOMString or an array of
  // DOMStrings. In both cases, it represents the base64url encoded and
  // digitally signed payment information. Each payment provider should
  // define its supported JWT format.
  DOMRequest mozPay(any jwts);
};
#endif

[NoInterfaceObject, Exposed=(Window,Worker)]
interface NavigatorConcurrentHardware {
  readonly attribute unsigned long long hardwareConcurrency;
};

partial interface Navigator {
  // largeTextEnabled is not allowed to be accessed from workers, and it is
  // updated when pref ui.largeText.enabled is changed.
  [AvailableIn=CertifiedApps]
  readonly attribute boolean largeTextEnabled;
};

#ifdef HAS_KOOST_MODULES
partial interface Navigator {
  // This is a safety catch for System App.
  // Return the current flip status. This attribute is only available for apps
  // who runs on the main process, and it should be accessed by System App only.
  [Throws, Pref="dom.flip.enabled", CheckAnyPermissions="flip", AvailableIn=CertifiedApps]
  readonly attribute boolean flipOpened;
};

partial interface Navigator {
  [Throws]
  readonly attribute VolumeManager volumeManager;
};

  // Set true to turn on spatial navigation service, please note that
  // 1. Only available for oop apps.
  // 2. Spatial navigation prevent default and stop propagation on key events of
  //    Enter, Up, Right, Down, Left key. App should be aware of this behavior
  //    if it expects to handle key events, and if any system UI pops up, or
  //    focus on an input element (IME is active), app should turn spatial
  //    navigation off.
  // 3. Spatial navigation do not prevent default or stop propagation on key
  //    event of RSK, but in non-TPS mode, spatial navigation will trigger
  //    contextmenu event on key event of RSK.
  // 4. If status of spatial navigation is successfully changed by this method,
  //    a 'mozbrowserspatialnavigationchanged' event will be dispatched to
  //    System App (or app with browser permission).

partial interface Navigator {
  [Pref="dom.spatialnavigation.enabled", CheckAnyPermissions="spatialnavigation-app-manage", AvailableIn=PrivilegedApps]
  attribute boolean spatialNavigationEnabled;
};

#ifdef ENABLE_SOUNDTRIGGER
partial interface Navigator {
  [Throws, CheckAnyPermissions="sound-trigger", AvailableIn=CertifiedApps,
   Func="SoundTriggerManager::HasSupport"]
  readonly attribute SoundTriggerManager soundTriggerManager;
};
#endif

partial interface Navigator {
  [Throws, Func="Navigator::HasExternalAPISupport"]
  readonly attribute ExternalAPI externalapi;
};
#endif

#ifdef ENABLE_FOTA
partial interface Navigator {
  [CheckAnyPermissions="fota", Throws]
  readonly attribute FotaEngine fota;
};
#endif

#ifdef ENABLE_EMBMS
partial interface Navigator {
  [Throws, Pref="dom.embms.enabled", CheckAnyPermissions="embms", AvailableIn=PrivilegedApps]
  readonly attribute LteBroadcastManager? lteBroadcastManager;
};
#endif

#ifdef ENABLE_TEE_SUI
partial interface Navigator {
  [Throws, Func="Navigator::HasTEEReaderSupport",
   CheckAnyPermissions="teereader-manage", AvailableIn="CertifiedApps"]
  readonly attribute TEEReader? teeReader;
};
#endif

#ifdef ENABLE_RSU
partial interface Navigator {
  [Throws, /*CheckAnyPermissions="rsu",*/ AvailableIn=CertifiedApps]
  readonly attribute RemoteSimUnlock? rsu;
};
#endif
