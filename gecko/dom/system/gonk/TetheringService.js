/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");
Cu.import("resource://gre/modules/systemlibs.js");

const TETHERINGSERVICE_CONTRACTID = "@mozilla.org/tethering/service;1";
const TETHERINGSERVICE_CID =
  Components.ID("{527a4121-ee5a-4651-be9c-f46f59cf7c01}");

XPCOMUtils.defineLazyServiceGetter(this, "gNetworkManager",
                                   "@mozilla.org/network/manager;1",
                                   "nsINetworkManager");

XPCOMUtils.defineLazyServiceGetter(this, "gNetworkService",
                                   "@mozilla.org/network/service;1",
                                   "nsINetworkService");

XPCOMUtils.defineLazyServiceGetter(this, "gSettingsService",
                                   "@mozilla.org/settingsService;1",
                                   "nsISettingsService");

XPCOMUtils.defineLazyServiceGetter(this, "gMobileConnectionService",
                                   "@mozilla.org/mobileconnection/mobileconnectionservice;1",
                                   "nsIMobileConnectionService");

XPCOMUtils.defineLazyGetter(this, "gRil", function() {
  try {
    return Cc["@mozilla.org/ril;1"].getService(Ci.nsIRadioInterfaceLayer);
  } catch (e) {}

  return null;
});

const TOPIC_MOZSETTINGS_CHANGED      = "mozsettings-changed";
const TOPIC_PREF_CHANGED             = "nsPref:changed";
const TOPIC_XPCOM_SHUTDOWN           = "xpcom-shutdown";
const PREF_MANAGE_OFFLINE_STATUS     = "network.gonk.manage-offline-status";
const PREF_NETWORK_DEBUG_ENABLED     = "network.debugging.enabled";

const POSSIBLE_USB_INTERFACE_NAME = "rndis0,usb0";
const DEFAULT_USB_INTERFACE_NAME  = "rndis0";
const DEFAULT_3G_INTERFACE_NAME   = "rmnet0";
const DEFAULT_WIFI_INTERFACE_NAME = "wlan0";

// The kernel's proc entry for network lists.
const KERNEL_NETWORK_ENTRY = "/sys/class/net";

const TETHERING_TYPE_WIFI = "WiFi";
const TETHERING_TYPE_USB  = "USB";

const WIFI_FIRMWARE_AP            = "AP";
const WIFI_FIRMWARE_STATION       = "STA";
const WIFI_SECURITY_TYPE_NONE     = "open";
const WIFI_SECURITY_TYPE_WPA_PSK  = "wpa-psk";
const WIFI_SECURITY_TYPE_WPA2_PSK = "wpa2-psk";
const WIFI_CTRL_INTERFACE         = "wl0.1";

const NETWORK_INTERFACE_UP   = "up";
const NETWORK_INTERFACE_DOWN = "down";

const TETHERING_STATE_ONGOING = "ongoing";
const TETHERING_STATE_IDLE    = "idle";
const TETHERING_STATE_ACTIVE  = "active";

// Settings DB path for USB tethering.
const SETTINGS_USB_ENABLED             = "tethering.usb.enabled";
const SETTINGS_USB_IP                  = "tethering.usb.ip";
const SETTINGS_USB_PREFIX              = "tethering.usb.prefix";
const SETTINGS_USB_DHCPSERVER_STARTIP  = "tethering.usb.dhcpserver.startip";
const SETTINGS_USB_DHCPSERVER_ENDIP    = "tethering.usb.dhcpserver.endip";
const SETTINGS_USB_DNS1                = "tethering.usb.dns1";
const SETTINGS_USB_DNS2                = "tethering.usb.dns2";

// Settings DB path for WIFI tethering.
const SETTINGS_WIFI_TETHERING_ENABLED  = "tethering.wifi.enabled";
const SETTINGS_WIFI_DHCPSERVER_STARTIP = "tethering.wifi.dhcpserver.startip";
const SETTINGS_WIFI_DHCPSERVER_ENDIP   = "tethering.wifi.dhcpserver.endip";

// Settings DB patch for dun required setting.
const SETTINGS_DUN_REQUIRED = "tethering.dun.required";

// Default value for USB tethering.
const DEFAULT_USB_IP                   = "192.168.0.1";
const DEFAULT_USB_PREFIX               = "24";
const DEFAULT_USB_DHCPSERVER_STARTIP   = "192.168.0.10";
const DEFAULT_USB_DHCPSERVER_ENDIP     = "192.168.0.30";

// Backup value for USB tethering.
const BACKUP_USB_IP                    = "172.16.0.1";
const BACKUP_USB_PREFIX                = "24";
const BACKUP_USB_DHCPSERVER_STARTIP    = "172.16.0.10";
const BACKUP_USB_DHCPSERVER_ENDIP      = "172.16.0.30";

const DEFAULT_DNS1                     = "8.8.8.8";
const DEFAULT_DNS2                     = "8.8.4.4";

const DEFAULT_WIFI_DHCPSERVER_STARTIP  = "192.168.1.10";
const DEFAULT_WIFI_DHCPSERVER_ENDIP    = "192.168.1.30";

const SETTINGS_DATA_DEFAULT_SERVICE_ID = "ril.data.defaultServiceId";
const MOBILE_DUN_CONNECT_TIMEOUT       = 15000;
const MOBILE_DUN_RETRY_INTERVAL        = 5000;
const MOBILE_DUN_MAX_RETRIES           = 2;

var debug;
function updateDebug() {
  let debugPref = false; // set default value here.
  try {
    debugPref = debugPref || Services.prefs.getBoolPref(PREF_NETWORK_DEBUG_ENABLED);
  } catch (e) {}

  if (debugPref) {
    debug = function(s) {
      dump("-*- TetheringService: " + s + "\n");
    };
  } else {
    debug = function(s) {};
  }
}
updateDebug();

function TetheringService() {
  Services.obs.addObserver(this, TOPIC_XPCOM_SHUTDOWN, false);
  Services.obs.addObserver(this, TOPIC_MOZSETTINGS_CHANGED, false);
  Services.prefs.addObserver(PREF_NETWORK_DEBUG_ENABLED, this, false);
  Services.prefs.addObserver(PREF_MANAGE_OFFLINE_STATUS, this, false);

  try {
    this._manageOfflineStatus =
      Services.prefs.getBoolPref(PREF_MANAGE_OFFLINE_STATUS);
  } catch(ex) {
    // Ignore.
  }

  this._dataDefaultServiceId = 0;

  // Possible usb tethering interfaces for different gonk platform.
  this.possibleInterface = POSSIBLE_USB_INTERFACE_NAME.split(",");

  // Default values for internal and external interfaces.
  this._externalInterface = DEFAULT_3G_INTERFACE_NAME;
  this._internalInterface = {};
  this._internalInterface[TETHERING_TYPE_USB] = DEFAULT_USB_INTERFACE_NAME;
  this._internalInterface[TETHERING_TYPE_WIFI] = DEFAULT_WIFI_INTERFACE_NAME;

  gNetworkService.createNetwork(DEFAULT_3G_INTERFACE_NAME, Ci.nsINetworkInfo.NETWORK_TYPE_UNKNOWN,
    function () {
      debug("Create a default Network interface: " + DEFAULT_3G_INTERFACE_NAME);
  });

  this.tetheringSettings = {};
  this.initTetheringSettings();

  let settingsLock = gSettingsService.createLock();
  // Read the default service id for data call.
  settingsLock.get(SETTINGS_DATA_DEFAULT_SERVICE_ID, this);

  // Read usb tethering data from settings DB.
  settingsLock.get(SETTINGS_USB_IP, this);
  settingsLock.get(SETTINGS_USB_PREFIX, this);
  settingsLock.get(SETTINGS_USB_DHCPSERVER_STARTIP, this);
  settingsLock.get(SETTINGS_USB_DHCPSERVER_ENDIP, this);
  settingsLock.get(SETTINGS_USB_DNS1, this);
  settingsLock.get(SETTINGS_USB_DNS2, this);
  settingsLock.get(SETTINGS_USB_ENABLED, this);

  // Read wifi tethering data from settings DB.
  settingsLock.get(SETTINGS_WIFI_TETHERING_ENABLED, this);
  settingsLock.get(SETTINGS_WIFI_DHCPSERVER_STARTIP, this);
  settingsLock.get(SETTINGS_WIFI_DHCPSERVER_ENDIP, this);

  this._usbTetheringSettingsToRead = [SETTINGS_USB_IP,
                                      SETTINGS_USB_PREFIX,
                                      SETTINGS_USB_DHCPSERVER_STARTIP,
                                      SETTINGS_USB_DHCPSERVER_ENDIP,
                                      SETTINGS_USB_DNS1,
                                      SETTINGS_USB_DNS2,
                                      SETTINGS_USB_ENABLED,
                                      SETTINGS_WIFI_DHCPSERVER_STARTIP,
                                      SETTINGS_WIFI_DHCPSERVER_ENDIP];

  this.wantConnectionEvent = null;

  this.dunConnectTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);

  this.dunRetryTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);

  this._pendingTetheringRequests = [];
}
TetheringService.prototype = {
  classID:   TETHERINGSERVICE_CID,
  classInfo: XPCOMUtils.generateCI({classID: TETHERINGSERVICE_CID,
                                    contractID: TETHERINGSERVICE_CONTRACTID,
                                    classDescription: "Tethering Service",
                                    interfaces: [Ci.nsITetheringService]}),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsITetheringService,
                                         Ci.nsISupportsWeakReference,
                                         Ci.nsIObserver,
                                         Ci.nsISettingsServiceCallback]),

  // Flag to record the default client id for data call.
  _dataDefaultServiceId: null,

  // Number of usb tehering requests to be processed.
  _usbTetheringRequestCount: 0,

  // Usb tethering state.
  _usbTetheringAction: TETHERING_STATE_IDLE,

  // Tethering settings.
  tetheringSettings: null,

  // Tethering settings need to be read from settings DB.
  _usbTetheringSettingsToRead: null,

  // Previous usb tethering enabled state.
  _oldUsbTetheringEnabledState: null,

  // External and internal interface name.
  _externalInterface: null,

  _internalInterface: null,

  // Dun connection timer.
  dunConnectTimer: null,

  // Dun connection retry times.
  dunRetryTimes: 0,

  // Dun retry timer.
  dunRetryTimer: null,

  // Pending tethering request to handle after dun is connected.
  _pendingTetheringRequests: null,

  // Flag to indicate wether wifi tethering is being processed.
  _wifiTetheringRequestOngoing: false,

  // Flag to notify restart USB tethering
  _usbTetheringRequestRestart: false,

  // Arguments for pending wifi tethering request.
  _pendingWifiTetheringRequestArgs: null,

  // The state of tethering.
  state: Ci.nsITetheringService.TETHERING_STATE_INACTIVE,

  // Flag to check if we can modify the Services.io.offline.
  _manageOfflineStatus: true,

  // nsIObserver

  observe: function(aSubject, aTopic, aData) {
    let network;

    switch(aTopic) {
      case TOPIC_PREF_CHANGED:
        if (aData === PREF_NETWORK_DEBUG_ENABLED) {
          updateDebug();
        }
        break;
      case TOPIC_MOZSETTINGS_CHANGED:
        if ("wrappedJSObject" in aSubject) {
          aSubject = aSubject.wrappedJSObject;
        }
        this.handle(aSubject.key, aSubject.value);
        break;
      case TOPIC_XPCOM_SHUTDOWN:
        Services.obs.removeObserver(this, TOPIC_XPCOM_SHUTDOWN);
        Services.obs.removeObserver(this, TOPIC_MOZSETTINGS_CHANGED);
        Services.prefs.removeObserver(PREF_NETWORK_DEBUG_ENABLED, this);
        Services.prefs.removeObserver(PREF_MANAGE_OFFLINE_STATUS, this);

        this.dunConnectTimer.cancel();
        this.dunRetryTimer.cancel();
        break;
      case PREF_MANAGE_OFFLINE_STATUS:
        try {
          this._manageOfflineStatus =
            Services.prefs.getBoolPref(PREF_MANAGE_OFFLINE_STATUS);
        } catch(ex) {
          // Ignore.
        }
        break;
    }
  },

  // nsISettingsServiceCallback

  handle: function(aName, aResult) {
    switch(aName) {
      case SETTINGS_DATA_DEFAULT_SERVICE_ID:
        this._dataDefaultServiceId = aResult || 0;
        debug("'_dataDefaultServiceId' is now " + this._dataDefaultServiceId);
        break;
      case SETTINGS_WIFI_TETHERING_ENABLED:
        if (aResult !== null) {
          this.tetheringSettings[aName] = aResult;
        }
        debug("'" + aName + "'" + " is now " + this.tetheringSettings[aName]);
        return;
      case SETTINGS_USB_ENABLED:
        this._oldUsbTetheringEnabledState = this.tetheringSettings[SETTINGS_USB_ENABLED];
      case SETTINGS_USB_IP:
      case SETTINGS_USB_PREFIX:
      case SETTINGS_USB_DHCPSERVER_STARTIP:
      case SETTINGS_USB_DHCPSERVER_ENDIP:
      case SETTINGS_USB_DNS1:
      case SETTINGS_USB_DNS2:
      case SETTINGS_WIFI_DHCPSERVER_STARTIP:
      case SETTINGS_WIFI_DHCPSERVER_ENDIP:
        if (aResult !== null) {
          this.tetheringSettings[aName] = aResult;
        }
        debug("'" + aName + "'" + " is now " + this.tetheringSettings[aName]);
        let index = this._usbTetheringSettingsToRead.indexOf(aName);

        if (index != -1) {
          this._usbTetheringSettingsToRead.splice(index, 1);
        }

        if (this._usbTetheringSettingsToRead.length) {
          debug("We haven't read completely the usb Tethering data from settings db.");
          break;
        }

        if (this._oldUsbTetheringEnabledState === this.tetheringSettings[SETTINGS_USB_ENABLED]) {
          debug("No changes for SETTINGS_USB_ENABLED flag. Nothing to do.");
          this.handlePendingWifiTetheringRequest();
          break;
        }

        this._usbTetheringRequestCount++;
        if (this._usbTetheringRequestCount === 1) {
          if (this._wifiTetheringRequestOngoing) {
            debug('USB tethering request is blocked by ongoing wifi tethering request.');
          } else {
            this.handleLastUsbTetheringRequest();
          }
        }
        break;
    };
  },

  handleError: function(aErrorMessage) {
    debug("There was an error while reading Tethering settings.");
    this.tetheringSettings = {};
    this.tetheringSettings[SETTINGS_USB_ENABLED] = false;
  },

  initTetheringSettings: function() {
    this.tetheringSettings[SETTINGS_USB_ENABLED] = false;
    this.tetheringSettings[SETTINGS_USB_IP] = DEFAULT_USB_IP;
    this.tetheringSettings[SETTINGS_USB_PREFIX] = DEFAULT_USB_PREFIX;
    this.tetheringSettings[SETTINGS_USB_DHCPSERVER_STARTIP] = DEFAULT_USB_DHCPSERVER_STARTIP;
    this.tetheringSettings[SETTINGS_USB_DHCPSERVER_ENDIP] = DEFAULT_USB_DHCPSERVER_ENDIP;
    this.tetheringSettings[SETTINGS_USB_DNS1] = DEFAULT_DNS1;
    this.tetheringSettings[SETTINGS_USB_DNS2] = DEFAULT_DNS2;

    this.tetheringSettings[SETTINGS_WIFI_TETHERING_ENABLED] = false;
    this.tetheringSettings[SETTINGS_WIFI_DHCPSERVER_STARTIP] = DEFAULT_WIFI_DHCPSERVER_STARTIP;
    this.tetheringSettings[SETTINGS_WIFI_DHCPSERVER_ENDIP]   = DEFAULT_WIFI_DHCPSERVER_ENDIP;

    this.tetheringSettings[SETTINGS_DUN_REQUIRED] =
      libcutils.property_get("ro.tethering.dun_required") === "1";
  },

  getNetworkInfo: function(aType, aServiceId) {
    for (let networkId in gNetworkManager.allNetworkInfo) {
      let networkInfo = gNetworkManager.allNetworkInfo[networkId];
      if (networkInfo.type == aType) {
        try {
          if (networkInfo instanceof Ci.nsIRilNetworkInfo) {
            let rilNetwork = networkInfo.QueryInterface(Ci.nsIRilNetworkInfo);
            if (rilNetwork.serviceId != aServiceId) {
              continue;
            }
          }
        } catch (e) {}
        return networkInfo;
      }
    }
    return null;
  },

  handleLastUsbTetheringRequest: function() {
    debug('handleLastUsbTetheringRequest... ' + this._usbTetheringRequestCount);

    if (this._usbTetheringRequestCount === 0) {
      if (this.wantConnectionEvent) {
        if (this.tetheringSettings[SETTINGS_USB_ENABLED] ||
            this.tetheringSettings[SETTINGS_WIFI_TETHERING_ENABLED]) {
          this.wantConnectionEvent.call(this);
        }
        this.wantConnectionEvent = null;
      }
      this.handlePendingWifiTetheringRequest();
      return;
    }

    // Cancel the accumlated count to 1 since we only care about the
    // last state.
    this._usbTetheringRequestCount = 1;
    this.handleUSBTetheringToggle(this.tetheringSettings[SETTINGS_USB_ENABLED]);
    this.wantConnectionEvent = null;
  },

  handlePendingWifiTetheringRequest: function() {
    if (this._pendingWifiTetheringRequestArgs) {
      this.setWifiTethering.apply(this, this._pendingWifiTetheringRequestArgs);
      this._pendingWifiTetheringRequestArgs = null;
    }
  },

  /**
   * Callback when dun connection fails to connect within timeout.
   */
  onDunConnectTimerTimeout: function() {
    while (this._pendingTetheringRequests.length > 0) {
      debug("onDunConnectTimerTimeout: callback without network info.");
      let callback = this._pendingTetheringRequests.shift();
      if (typeof callback === 'function') {
        callback();
      }
    }
  },

  setupDunConnection: function() {
    this.dunRetryTimer.cancel();
    let connection =
      gMobileConnectionService.getItemByServiceId(this._dataDefaultServiceId);
    let data = connection && connection.data;
    if (data && data.state === "registered") {
      let ril = gRil.getRadioInterface(this._dataDefaultServiceId);

      this.dunRetryTimes = 0;
      ril.setupDataCallByType(Ci.nsINetworkInfo.NETWORK_TYPE_MOBILE_DUN);
      this.dunConnectTimer.cancel();
      this.dunConnectTimer.
        initWithCallback(this.onDunConnectTimerTimeout.bind(this),
                         MOBILE_DUN_CONNECT_TIMEOUT, Ci.nsITimer.TYPE_ONE_SHOT);
      return;
    }

    if (this.dunRetryTimes++ >= MOBILE_DUN_MAX_RETRIES) {
      debug("setupDunConnection: max retries reached.");
      this.dunRetryTimes = 0;
      // same as dun connect timeout.
      this.onDunConnectTimerTimeout();
      return;
    }

    debug("Data not ready, retry dun after " + MOBILE_DUN_RETRY_INTERVAL + " ms.");
    this.dunRetryTimer.
      initWithCallback(this.setupDunConnection.bind(this),
                       MOBILE_DUN_RETRY_INTERVAL, Ci.nsITimer.TYPE_ONE_SHOT);
  },

  _dunActiveUsers: 0,
  handleDunConnection: function(aEnable, aCallback) {
    debug("handleDunConnection: " + aEnable);
    let dun = this.getNetworkInfo(
      Ci.nsINetworkInfo.NETWORK_TYPE_MOBILE_DUN, this._dataDefaultServiceId);

    if (!aEnable) {
      this._dunActiveUsers--;
      if (this._dunActiveUsers > 0) {
        debug("Dun still needed by others, do not disconnect.")
        return;
      }

      this.dunRetryTimes = 0;
      this.dunRetryTimer.cancel();
      this.dunConnectTimer.cancel();
      this._pendingTetheringRequests = [];

      if (dun && (dun.state == Ci.nsINetworkInfo.NETWORK_STATE_CONNECTED)) {
        gRil.getRadioInterface(this._dataDefaultServiceId)
          .deactivateDataCallByType(Ci.nsINetworkInfo.NETWORK_TYPE_MOBILE_DUN);
      }
      return;
    }

    this._dunActiveUsers++;
    // If Wifi active, enable tethering directly.
    if (gNetworkManager.activeNetworkInfo &&
        gNetworkManager.activeNetworkInfo.type === Ci.nsINetworkInfo.NETWORK_TYPE_WIFI) {
      debug("Wifi active, enable tethering directly " + gNetworkManager.activeNetworkInfo.name);
      aCallback(gNetworkManager.activeNetworkInfo);
      return;
    }
    // else, trigger dun connection first.
    else if (!dun ||
              (dun.state != Ci.nsINetworkInfo.NETWORK_STATE_CONNECTED)) {
      debug("DUN data call inactive, setup dun data call!")
      this._pendingTetheringRequests.push(aCallback);
      this.dunRetryTimes = 0;
      this.setupDunConnection();

      return;
    }

    aCallback(dun);
  },

  handleUSBTetheringToggle: function(aEnable) {
    debug("handleUSBTetheringToggle: " + aEnable);
    if (aEnable &&
        (this._usbTetheringAction === TETHERING_STATE_ONGOING ||
         this._usbTetheringAction === TETHERING_STATE_ACTIVE)) {
      debug("Usb tethering already connecting/connected.");
      this._usbTetheringRequestCount = 0;
      this.handlePendingWifiTetheringRequest();
      return;
    }

    if (!aEnable &&
        this._usbTetheringAction === TETHERING_STATE_IDLE) {
      debug("Usb tethering already disconnected.");
      this._usbTetheringRequestCount = 0;
      this.handlePendingWifiTetheringRequest();
      return;
    }

    if (!aEnable) {
      this.tetheringSettings[SETTINGS_USB_ENABLED] = false;
      gNetworkService.enableUsbRndis(false, this.enableUsbRndisResult.bind(this));
      return;
    }

    // Refine USB tethering IP if subnet conflict with external interface
    if (aEnable && this.isTetherSubnetConflict())
      this.refineTetherSubnet(false);

    this.tetheringSettings[SETTINGS_USB_ENABLED] = true;
    this._usbTetheringAction = TETHERING_STATE_ONGOING;

    if (this.tetheringSettings[SETTINGS_DUN_REQUIRED]) {
      this.handleDunConnection(true, (aNetworkInfo) => {
        if (!aNetworkInfo){
          this.usbTetheringResultReport(aEnable, "Dun connection failed");
          return;
        }
        gNetworkService.enableUsbRndis(true, this.enableUsbRndisResult.bind(this));
      });
      return;
    }

    gNetworkService.enableUsbRndis(true, this.enableUsbRndisResult.bind(this));
  },

  getUSBTetheringParameters: function(aEnable, aTetheringInterface) {
    let interfaceIp = this.tetheringSettings[SETTINGS_USB_IP];
    let prefix = this.tetheringSettings[SETTINGS_USB_PREFIX];
    let wifiDhcpStartIp = this.tetheringSettings[SETTINGS_WIFI_DHCPSERVER_STARTIP];
    let wifiDhcpEndIp = this.tetheringSettings[SETTINGS_WIFI_DHCPSERVER_ENDIP];
    let usbDhcpStartIp = this.tetheringSettings[SETTINGS_USB_DHCPSERVER_STARTIP];
    let usbDhcpEndIp = this.tetheringSettings[SETTINGS_USB_DHCPSERVER_ENDIP];
    let dns1 = this.tetheringSettings[SETTINGS_USB_DNS1];
    let dns2 = this.tetheringSettings[SETTINGS_USB_DNS2];
    let internalInterface = aTetheringInterface;
    let dnses = (gNetworkManager.activeNetworkInfo) ?
      gNetworkManager.activeNetworkInfo.getDnses() : new Array(0);

    // Assigned external interface if try to bring up Usb Tethering
    if (aEnable) {
      this.setExternalInterface();
    }

    // Using the default values here until application support these settings.
    if (interfaceIp == "" || prefix == "" ||
        wifiDhcpStartIp == "" || wifiDhcpEndIp == "" ||
        usbDhcpStartIp == "" || usbDhcpEndIp == "") {
      debug("Invalid subnet information.");
      return null;
    }

    return {
      ifname: internalInterface,
      ip: interfaceIp,
      prefix: prefix,
      wifiStartIp: wifiDhcpStartIp,
      wifiEndIp: wifiDhcpEndIp,
      usbStartIp: usbDhcpStartIp,
      usbEndIp: usbDhcpEndIp,
      dns1: dns1,
      dns2: dns2,
      internalIfname: internalInterface,
      externalIfname: this._externalInterface,
      enable: aEnable,
      link: aEnable ? NETWORK_INTERFACE_UP : NETWORK_INTERFACE_DOWN,
      dnses: dnses,
    };
  },

  notifyError: function(aResetSettings, aCallback, aMsg) {
    if (aResetSettings) {
      let settingsLock = gSettingsService.createLock();
      // Disable wifi tethering with a useful error message for the user.
      settingsLock.set("tethering.wifi.enabled", false, null, aMsg);
    }

    debug("setWifiTethering: " + (aMsg ? aMsg : "success"));

    if (aCallback) {
      // Callback asynchronously to avoid netsted toggling.
      Services.tm.currentThread.dispatch(() => {
        aCallback.wifiTetheringEnabledChange(aMsg);
      }, Ci.nsIThread.DISPATCH_NORMAL);
    }
  },

  enableWifiTethering: function(aEnable, aConfig, aCallback) {
    // Fill in config's required fields.
    aConfig.ifname         = this._internalInterface[TETHERING_TYPE_WIFI];
    aConfig.internalIfname = this._internalInterface[TETHERING_TYPE_WIFI];
    aConfig.dnses = (gNetworkManager.activeNetworkInfo) ?
      gNetworkManager.activeNetworkInfo.getDnses() : new Array(0);

    // Assigned external interface if try to bring up Wifi Tethering
    if (aEnable) {
      this.setExternalInterface();
    }
    aConfig.externalIfname = this._externalInterface;

    this._wifiTetheringRequestOngoing = true;
    gNetworkService.setWifiTethering(aEnable, aConfig, (aError) => {
      // Change the tethering state to WIFI if there is no error.
      if (aEnable && !aError) {
        this.state = Ci.nsITetheringService.TETHERING_STATE_WIFI;
      } else {
        // If wifi thethering is disable, or any error happens,
        // then consider the following statements.

        // Check whether the state is USB now or not. If no then just change
        // it to INACTIVE, if yes then just keep it.
        // It means that don't let the disable or error of WIFI affect
        // the original active state.
        if (this.state != Ci.nsITetheringService.TETHERING_STATE_USB) {
          this.state = Ci.nsITetheringService.TETHERING_STATE_INACTIVE;
        }

        // Disconnect dun on error or when wifi tethering is disabled.
        if (this.tetheringSettings[SETTINGS_DUN_REQUIRED]) {
          this.handleDunConnection(false);
        }
      }

      if (this._manageOfflineStatus) {
        Services.io.offline = !this.isAnyConnected() &&
                              (this.state ===
                               Ci.nsITetheringService.TETHERING_STATE_INACTIVE);
      }

      let resetSettings = aError;
      debug('gNetworkService.setWifiTethering finished');
      this.notifyError(resetSettings, aCallback, aError);
      this._wifiTetheringRequestOngoing = false;
      this.handleLastUsbTetheringRequest();
    });
  },

  // Enable/disable WiFi tethering by sending commands to netd.
  setWifiTethering: function(aEnable, aInterfaceName, aConfig, aCallback) {
    debug("setWifiTethering: " + aEnable);
    if (!aInterfaceName) {
      this.notifyError(true, aCallback, "invalid network interface name");
      return;
    }

    if (!aConfig) {
      this.notifyError(true, aCallback, "invalid configuration");
      return;
    }

    if (this._usbTetheringRequestCount > 0) {
      // If there's still pending usb tethering request, save
      // the request params and redo |setWifiTethering| on
      // usb tethering task complete.
      debug('USB tethering request is being processed. Queue this wifi tethering request.');
      this._pendingWifiTetheringRequestArgs = Array.prototype.slice.call(arguments);
      debug('Pending args: ' + JSON.stringify(this._pendingWifiTetheringRequestArgs));
      return;
    }

    // Re-check again, test cases set this property later.
    this.tetheringSettings[SETTINGS_DUN_REQUIRED] =
      libcutils.property_get("ro.tethering.dun_required") === "1";

    if (!aEnable) {
      this.enableWifiTethering(false, aConfig, aCallback);
      return;
    }

    this._internalInterface[TETHERING_TYPE_WIFI] = aInterfaceName;

    if (this.tetheringSettings[SETTINGS_DUN_REQUIRED]) {
      this.handleDunConnection(true, (aNetworkInfo) => {
        if (!aNetworkInfo) {
          debug("Dun connection failed, remain enable hotspot with default interface.");
        }
        this.enableWifiTethering(true, aConfig, aCallback);
      });
      return;
    }

    this.enableWifiTethering(true, aConfig, aCallback);
  },

  // Enable/disable USB tethering by sending commands to netd.
  setUSBTethering: function(aEnable, aTetheringInterface, aCallback) {
    let self = this;
    let params = this.getUSBTetheringParameters(aEnable, aTetheringInterface);

    if (params === null) {
      gNetworkService.enableUsbRndis(false, function() {
        self.usbTetheringResultReport(aEnable, "Invalid parameters");
      });
      return;
    }

    gNetworkService.setUSBTethering(aEnable, params, aCallback);
  },

  getUsbInterface: function() {
    // Find the rndis interface.
    for (let i = 0; i < this.possibleInterface.length; i++) {
      try {
        let file = new FileUtils.File(KERNEL_NETWORK_ENTRY + "/" +
                                      this.possibleInterface[i]);
        if (file.exists()) {
          return this.possibleInterface[i];
        }
      } catch (e) {
        debug("Not " + this.possibleInterface[i] + " interface.");
      }
    }
    debug("Can't find rndis interface in possible lists.");
    return DEFAULT_USB_INTERFACE_NAME;
  },

  enableUsbRndisResult: function(aSuccess, aEnable) {
    if (aSuccess) {
      // If enable is false, don't find usb interface cause it is already down,
      // just use the internal interface in settings.
      if (aEnable) {
        this._internalInterface[TETHERING_TYPE_USB] = this.getUsbInterface();
      }
      this.setUSBTethering(aEnable,
                           this._internalInterface[TETHERING_TYPE_USB],
                           this.usbTetheringResultReport.bind(this, aEnable));
    } else {
      this.usbTetheringResultReport(aEnable, "enableUsbRndisResult failure");
      throw new Error("failed to set USB Function to adb");
    }
  },

  usbTetheringResultReport: function(aEnable, aError) {
    let self = this;
    this._usbTetheringRequestCount--;

    let settingsLock = gSettingsService.createLock();

    debug('usbTetheringResultReport callback. enable: ' + aEnable +
          ', error: ' + aError);

    // Disable tethering settings when fail to enable it.
    if (aError) {
      if (this.tetheringSettings[SETTINGS_DUN_REQUIRED]) {
        this.handleDunConnection(false);
      }
      if (aError == "enableUsbRndisResult failure") {
        gNetworkService.enableUsbRndis(false, function() {
          self.setUSBTethering(false,
                               self._internalInterface[TETHERING_TYPE_USB], null);
        });
      } else if (aError == "Dun connection failed") {
        // If dun and wifi not active, just enable rndis with default interface.
        this._usbTetheringRequestCount++;
        gNetworkService.enableUsbRndis(true, this.enableUsbRndisResult.bind(this));
        return;
      }
      this.tetheringSettings[SETTINGS_USB_ENABLED] = false;
      settingsLock.set("tethering.usb.enabled", false, null);
      // Skip others request when we found an error.
      this._usbTetheringRequestCount = 0;
      this._usbTetheringRequestRestart = false;
      this._usbTetheringAction = TETHERING_STATE_IDLE;
      // If the thethering state is WIFI now, then just keep it,
      // if not, just change the state to INACTIVE.
      // It means that don't let the error of USB affect the original active state.
      if (this.state != Ci.nsITetheringService.TETHERING_STATE_WIFI) {
        this.state = Ci.nsITetheringService.TETHERING_STATE_INACTIVE;
      }
    } else {
      if (aEnable) {
        this._usbTetheringAction = TETHERING_STATE_ACTIVE;
        this.state = Ci.nsITetheringService.TETHERING_STATE_USB;
      } else {
        this._usbTetheringAction = TETHERING_STATE_IDLE;
        // If the state is now WIFI, don't let the disable of USB affect it.
        if (this.state != Ci.nsITetheringService.TETHERING_STATE_WIFI) {
          this.state = Ci.nsITetheringService.TETHERING_STATE_INACTIVE;
        }
        if (this.tetheringSettings[SETTINGS_DUN_REQUIRED]) {
          this.handleDunConnection(false);
        }
        // Restart USB tethering if needed
        if (this._usbTetheringRequestRestart) {
          debug("Restart USB tethering by request");
          this._usbTetheringRequestRestart = false;
          settingsLock.set("tethering.usb.enabled", true, null);
        }
      }

      if (this._manageOfflineStatus) {
        Services.io.offline = !this.isAnyConnected() &&
                              (this.state ===
                               Ci.nsITetheringService.TETHERING_STATE_INACTIVE);
      }

      this.handleLastUsbTetheringRequest();
    }
  },

  onExternalConnectionChangedReport: function(type, aSuccess, aExternalIfname) {
    debug("onExternalConnectionChangedReport result: success= " + aSuccess + " ,type= " + type);

    if (aSuccess) {
      // Update the external interface.
      this._externalInterface = aExternalIfname;
      debug("Change the interface name to " + aExternalIfname);
    }
  },

  // Re-design the external connection state change flow for tethering function.
  onExternalConnectionChanged: function(aNetworkInfo) {
    let self = this;
    // Check if the aNetworkInfo change is the external connection.
    // SETTINGS_DUN_REQUIRED
    // true: dun as external interface
    // false: default as external interface
    if (aNetworkInfo.type === Ci.nsINetworkInfo.NETWORK_TYPE_WIFI ||
        (this.tetheringSettings[SETTINGS_DUN_REQUIRED] &&
         aNetworkInfo.type === Ci.nsINetworkInfo.NETWORK_TYPE_MOBILE_DUN) ||
        (!this.tetheringSettings[SETTINGS_DUN_REQUIRED] &&
          aNetworkInfo.type === Ci.nsINetworkInfo.NETWORK_TYPE_MOBILE)) {
      debug("onExternalConnectionChanged. aNetworkInfo.type = " + aNetworkInfo.type +
            " , aNetworkInfo.state = " + aNetworkInfo.state + " , dun_required = "
            + this.tetheringSettings[SETTINGS_DUN_REQUIRED]);

      let tetheringType = null;
      // Handle USB tethering case.
      if (this.tetheringSettings[SETTINGS_USB_ENABLED]) {
        tetheringType = TETHERING_TYPE_USB;
      }
      // Handle WIFI tethering case.
      else if (this.tetheringSettings[SETTINGS_WIFI_TETHERING_ENABLED]) {
        tetheringType = TETHERING_TYPE_WIFI;
      } else {
        debug("onExternalConnectionChanged. Tethering is not enabled, return.");
        return;
      }
      debug("onExternalConnectionChanged. tetheringType = " + tetheringType);

      // Re-config the NETD.
      let previous = {
        internalIfname: this._internalInterface[tetheringType],
        externalIfname: this._externalInterface
      };

      let current = {
        internalIfname: this._internalInterface[tetheringType],
        externalIfname: aNetworkInfo.name,
        dns1: this.tetheringSettings[SETTINGS_USB_DNS1],
        dns2: this.tetheringSettings[SETTINGS_USB_DNS2],
        dnses: aNetworkInfo.getDnses()
      };

      if (aNetworkInfo.state == Ci.nsINetworkInfo.NETWORK_STATE_CONNECTED) {
        // Handle the wifi as external interface case, when embedded is dun.
        if (this.tetheringSettings[SETTINGS_DUN_REQUIRED] &&
            aNetworkInfo.type === Ci.nsINetworkInfo.NETWORK_TYPE_WIFI) {
          let dun = this.getNetworkInfo(
            Ci.nsINetworkInfo.NETWORK_TYPE_MOBILE_DUN, this._dataDefaultServiceId);
          if (dun && (dun.state == Ci.nsINetworkInfo.NETWORK_STATE_CONNECTED)) {
            debug("Wifi connected, switch dun to Wifi as external interface.");
            gRil.getRadioInterface(this._dataDefaultServiceId)
              .deactivateDataCallByType(Ci.nsINetworkInfo.NETWORK_TYPE_MOBILE_DUN);
          }
        }
        // Handle the dun as external interface case, when embedded is dun.
        else if (this.tetheringSettings[SETTINGS_DUN_REQUIRED] &&
            aNetworkInfo.type === Ci.nsINetworkInfo.NETWORK_TYPE_MOBILE_DUN) {
          this.dunConnectTimer.cancel();
          // Handle the request change case.
          if (this._pendingTetheringRequests.length > 0) {
            debug("DUN data call connected, process callbacks.");
            while (this._pendingTetheringRequests.length > 0) {
              let callback = this._pendingTetheringRequests.shift();
              if (typeof callback === 'function') {
                callback(aNetworkInfo);
              }
            }
            return;
          } else {
            // Handle the aNetworkInfo side change case. Run the updateUpStream flow.
            debug("DUN data call connected, process updateUpStream.");
          }
        }

        let callback = (function() {

          // Restart and refine USB tethering if subnet conflict with external interface
          if (this.isTetherSubnetConflict()) {
            this.refineTetherSubnet(true);
            return;
          }

          // Update external aNetworkInfo interface.
          debug("Update upstream interface to " + aNetworkInfo.name);
          gNetworkService.updateUpStream(tetheringType, previous, current, this.onExternalConnectionChangedReport.bind(this));
        }).bind(this);

        if (this._usbTetheringAction === TETHERING_STATE_ONGOING ||
            (this.tetheringSettings[SETTINGS_WIFI_TETHERING_ENABLED] &&
             this.state != Ci.nsITetheringService.TETHERING_STATE_WIFI)) {
          debug("Postpone the event and handle it when state is idle.");
          this.wantConnectionEvent = callback;
          return;
        }
        this.wantConnectionEvent = null;

        callback.call(this);
      } else if (aNetworkInfo.state == Ci.nsINetworkInfo.NETWORK_STATE_DISCONNECTED) {
        // Remove current forwarding forwarding rules
        debug("removing forwarding first before interface removed.");
        gNetworkService.removeUpStream(current, (aSuccess) => {
          if (aSuccess) {
            debug("remove forwarding rules successuly!");
            self.setExternalInterface();
          }
        });
        // If dun required, retrigger dun connection.
        if (aNetworkInfo.type === Ci.nsINetworkInfo.NETWORK_TYPE_WIFI &&
            this.tetheringSettings[SETTINGS_DUN_REQUIRED]) {
          let dun = this.getNetworkInfo(
            Ci.nsINetworkInfo.NETWORK_TYPE_MOBILE_DUN, this._dataDefaultServiceId);
          if (!dun || (dun.state != Ci.nsINetworkInfo.NETWORK_STATE_CONNECTED)) {
            debug("Wifi disconnect, retrigger dun connection.");
            this.dunRetryTimes = 0;
            this.setupDunConnection();
          }
        }
      }
    } else {
      if(this.tetheringSettings[SETTINGS_DUN_REQUIRED]) {
        debug("onExternalConnectionChanged. Not dun type state change, return.");
      } else {
        debug("onExternalConnectionChanged. Not default type state change, return.");
      }
    }
  },

  isAnyConnected: function() {
    let allNetworkInfo = gNetworkManager.allNetworkInfo;
    for (let networkId in allNetworkInfo) {
      if (allNetworkInfo.hasOwnProperty(networkId) &&
          allNetworkInfo[networkId].state === Ci.nsINetworkInfo.NETWORK_STATE_CONNECTED) {
          return true;
      }
    }
    return false;
  },

  // Provide suitable external interface
  setExternalInterface: function() {
    // Dun case, find Wifi or Dun as external interface.
    if (this.tetheringSettings[SETTINGS_DUN_REQUIRED]) {
      let allNetworkInfo = gNetworkManager.allNetworkInfo;
      this._externalInterface = null;
      for (let networkId in allNetworkInfo) {
        if (allNetworkInfo[networkId].state === Ci.nsINetworkInfo.NETWORK_STATE_CONNECTED &&
            (allNetworkInfo[networkId].type == Ci.nsINetworkInfo.NETWORK_TYPE_MOBILE_DUN ||
            allNetworkInfo[networkId].type ==Ci.nsINetworkInfo.NETWORK_TYPE_WIFI)) {
          this._externalInterface = allNetworkInfo[networkId].name;
        }
      }
      if (!this._externalInterface) {
        this._externalInterface = DEFAULT_3G_INTERFACE_NAME;
      }
    // Non Dun case, find available external interface.
    } else {
      if (gNetworkManager.activeNetworkInfo) {
        this._externalInterface = gNetworkManager.activeNetworkInfo.name;
      } else {
        this._externalInterface = DEFAULT_3G_INTERFACE_NAME;
      }
    }
  },

  // Check external/internal interface subnet conlict or not
  isTetherSubnetConflict: function() {

    // Check USB tethering only since no Wi-Fi concurrent mode
    if (!this.tetheringSettings[SETTINGS_USB_ENABLED])
      return false;

    if (gNetworkManager.activeNetworkInfo &&
        gNetworkManager.activeNetworkInfo.type == Ci.nsINetworkInfo.NETWORK_TYPE_WIFI) {

      // Get external interface ipaddr & prefix
      let ips = {};
      let prefixLengths = {};
      gNetworkManager.activeNetworkInfo.getAddresses(ips, prefixLengths);

      if (!this.tetheringSettings[SETTINGS_USB_IP] ||
          !ips.value || !prefixLengths.value) {
        debug("fail to compare subnet due to empty argument");
        return false;
      }

      for ( let i in ips.value) {
        // We only care about IPv4 conflict
        if (ips.value[i].indexOf(".") == -1) {
          continue;
        }
        // Filter wan/lan interface network segment
        let subnet = this.cidrToSubnet(prefixLengths.value[i]);
        let lanMask = [], wanMask = [];
        let lanIpaddrStr = this.tetheringSettings[SETTINGS_USB_IP].toString().split(".");
        let wanIpaddrStr = ips.value[i].toString().split(".");
        let subnetStr = subnet.toString().split(".");

        for (let j = 0;j < lanIpaddrStr.length; j++) {
          lanMask.push(parseInt(lanIpaddrStr[j]) & parseInt(subnetStr[j]));
          wanMask.push(parseInt(wanIpaddrStr[j]) & parseInt(subnetStr[j]));
        }

        if(lanMask.join(".") == wanMask.join(".")) {
          debug("Internal/External interface subnet Conflict : " + lanMask);
          return true;
        }
      }
    }
    return false;
  },

  // Transfer CIDR prefix to subnet
  cidrToSubnet: function(cidr) {
    let mask=[];
    for (let i = 0; i < 4; i++) {
      let n = Math.min(cidr, 8);
      mask.push(256 - Math.pow(2, 8-n));
      cidr -= n;
    }
    return mask.join('.');
  },

  // Replace non-conflict subnet for tether ipaddr
  refineTetherSubnet: function(restartTether) {
    let settingsLock = gSettingsService.createLock();

    if (this.tetheringSettings[SETTINGS_USB_IP] == DEFAULT_USB_IP) {
      debug("setup backup tethering settings");
      settingsLock.set(SETTINGS_USB_IP, BACKUP_USB_IP, null);
      settingsLock.set(SETTINGS_USB_DHCPSERVER_STARTIP, BACKUP_USB_DHCPSERVER_STARTIP, null);
      settingsLock.set(SETTINGS_USB_DHCPSERVER_ENDIP, BACKUP_USB_DHCPSERVER_ENDIP, null);
    } else {
      debug("setup defaul tethering settings");
      settingsLock.set(SETTINGS_USB_IP, DEFAULT_USB_IP, null);
      settingsLock.set(SETTINGS_USB_DHCPSERVER_STARTIP, DEFAULT_USB_DHCPSERVER_STARTIP, null);
      settingsLock.set(SETTINGS_USB_DHCPSERVER_ENDIP, DEFAULT_USB_DHCPSERVER_ENDIP, null);
    }

    if (restartTether) {
      debug("restart USB tethering due to subnet conflict with external interface");
      this._usbTetheringRequestRestart = restartTether;
      settingsLock.set(SETTINGS_USB_ENABLED, false, null);
    }
  },
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([TetheringService]);
