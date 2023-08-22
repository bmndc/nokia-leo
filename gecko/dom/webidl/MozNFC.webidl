/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

 /* Copyright © 2013 Deutsche Telekom, Inc. */

enum NfcErrorMessage {
  "",
  "IOError",
  "Timeout",
  "Busy",
  "ErrorConnect",
  "ErrorDisconnect",
  "ErrorRead",
  "ErrorWrite",
  "InvalidParameter",
  "InsufficientResource",
  "ErrorSocketCreation",
  "FailEnableDiscovery",
  "FailDisableDiscovery",
  "NotInitialize",
  "InitializeFail",
  "DeinitializeFail",
  "NotSupport",
  "FailEnableLowPowerMode",
  "FailDisableLowPowerMode"
};

enum StopPollPowerMode {
  "mode_low_power",
  "mode_full_power",
  "mode_screen_off",
  "mode_screen_lock",
  "mode_ultra_low_power"
};

enum NfcSelfTestType {
  // Turn RF on.
  "carrier_on",

  // Turn RF off.
  "carrier_off",

  // Verify Type-A analog parameter.
  "type-a_transaction",

  // Verify Type-B analog parameter.
  "type-b_transaction",

  // Reserve two types for future use, vendors are able to handle
  // reserved types based on requirement of certification.
  "vendor_defined_type1",
  "vendor_defined_type2"
};

enum SetConfigureResult {
  "success",
  "busy",
  "failed"
};

enum SetConfigureType {
  "setTransit",
  "revertToTransit",
  "revertToDefault"
};

[NoInterfaceObject]
interface MozNFCManager {
  /**
   * API to check if the given application's manifest
   * URL is registered with the Chrome Process or not.
   *
   * Returns success if given manifestUrl is registered for 'onpeerready',
   * otherwise error
   */
  [CheckAnyPermissions="nfc-manager", AvailableIn=CertifiedApps]
  Promise<boolean> checkP2PRegistration(DOMString manifestUrl);

  /**
   * Notify that user has accepted to share nfc message on P2P UI
   */
  [CheckAnyPermissions="nfc-manager", AvailableIn=CertifiedApps]
  void notifyUserAcceptedP2P(DOMString manifestUrl);

  /**
   * Notify the status of sendFile operation
   */
  [CheckAnyPermissions="nfc-manager", AvailableIn=CertifiedApps]
  void notifySendFileStatus(octet status, DOMString requestId);

  /**
   * Power on the NFC hardware and start polling/listening.
   */
  [CheckAnyPermissions="nfc-manager", AvailableIn=CertifiedApps]
  Promise<void> startPoll();

  /**
   * Stop polling for NFC tags or devices. i.e. enter low power mode.
   */
  [CheckAnyPermissions="nfc-manager", AvailableIn=PrivilegedApps]
  Promise<void> stopPoll(StopPollPowerMode mode);

  /**
   * Power off the NFC hardware.
   */
  [CheckAnyPermissions="nfc-manager", AvailableIn=CertifiedApps]
  Promise<void> powerOff();
};

[JSImplementation="@mozilla.org/nfc/manager;1",
 NavigatorProperty="mozNfc",
 Func="Navigator::HasNFCSupport",
 CheckAnyPermissions="nfc nfc-share",
 AvailableIn="PrivilegedApps",
 UnsafeInPrerendering]
interface MozNFC : EventTarget {
  /**
   * Indicate if NFC is enabled.
   */
  readonly attribute boolean enabled;

  /**
   * This event will be fired when another NFCPeer is detected, and user confirms
   * to share data to the NFCPeer object by calling mozNFC.notifyUserAcceptedP2P.
   * The event will be type of NFCPeerEvent.
   */
  [CheckAnyPermissions="nfc-share", AvailableIn=CertifiedApps]
  attribute EventHandler onpeerready;

  /**
   * This event will be fired when a NFCPeer is detected. The application has to
   * be running on the foreground (decided by System app) to receive this event.
   *
   * The default action of this event is to dispatch the event in System app
   * again, and System app will run the default UX behavior (like vibration).
   * So if the application would like to cancel the event, the application
   * should call event.preventDefault() or return false in this event handler.
   */
  attribute EventHandler onpeerfound;

  /**
   * This event will be fired when NFCPeer, earlier detected in onpeerready
   * or onpeerfound, moves out of range, or if the application has been switched
   * to the background (decided by System app).
   */
  attribute EventHandler onpeerlost;

  /**
   * This event will be fired when a NFCTag is detected. The application has to
   * be running on the foreground (decided by System app) to receive this event.
   *
   * The default action of this event is to dispatch the event in System app
   * again, and System app will run the default UX behavior (like vibration) and
   * launch MozActivity to handle the content of the tag. (For example, System
   * app will launch Browser if the tag contains URL). So if the application
   * would like to cancel the event, i.e. in the above example, the application
   * would process the URL by itself without launching Browser, the application
   * should call event.preventDefault() or return false in this event handler.
   */
  attribute EventHandler ontagfound;

  /**
   * This event will be fired if the tag detected in ontagfound has been
   * removed, or if the application has been switched to the background (decided
   * by System app).
   */
  attribute EventHandler ontaglost;

  attribute EventHandler onrffieldevent;

  attribute EventHandler onlistenmodeevent;
};

// Mozilla Only
partial interface MozNFC {
  [ChromeOnly]
  void eventListenerWasAdded(DOMString aType);
  [ChromeOnly]
  void eventListenerWasRemoved(DOMString aType);
};

partial interface MozNFC {
  /**
   * Indicate mPOS reader mode is enabled.
   */
  [CheckAnyPermissions="nfc-manager", AvailableIn=PrivilegedApps]
  readonly attribute boolean mPOSReaderStatus;

  /**
   * Switch mPOS mode
   * Reader mode if enabled is true.
   * Card mode if enabled is false.
   */
  [CheckAnyPermissions="nfc-manager", AvailableIn=PrivilegedApps]
  Promise<void> mPOSSetReaderMode(boolean enabled);

  /**
   * This event will be fired to indicate mPOS reader mode status.
   */
  [CheckAnyPermissions="nfc-manager", AvailableIn=PrivilegedApps]
  attribute EventHandler onmposreaderevent;

  /**
   * Perform Nfc analog test for DTE certification.
   */
  [CheckAnyPermissions="nfc-manager", AvailableIn=CertifiedApps]
  Promise<void> nfcSelfTest(NfcSelfTestType type);

  /**
   * Setup and apply configuration file for transit use case.
   */
  [CheckAnyPermissions="nfc-manager", AvailableIn=PrivilegedApps]
  Promise<SetConfigureResult> setConfig(SetConfigureType type, optional Blob configFile);

  /**
   * Power on the NFC hardware and start polling for NFC tags or devices.
   */
  [CheckAnyPermissions="nfc-manager", AvailableIn=PrivilegedApps]
  Promise<void> setTagReaderMode(boolean enabled);
};

MozNFC implements MozNFCManager;
