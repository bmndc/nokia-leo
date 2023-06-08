/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/systemlibs.js");

XPCOMUtils.defineLazyServiceGetter(this, "gSettingsService",
                                   "@mozilla.org/settingsService;1",
                                   "nsISettingsService");

XPCOMUtils.defineLazyServiceGetter(this, "gNetworkManager",
                                   "@mozilla.org/network/manager;1",
                                   "nsINetworkManager");

XPCOMUtils.defineLazyServiceGetter(this, "gMobileConnectionService",
                                   "@mozilla.org/mobileconnection/mobileconnectionservice;1",
                                   "nsIMobileConnectionService");

XPCOMUtils.defineLazyServiceGetter(this, "gIccService",
                                   "@mozilla.org/icc/iccservice;1",
                                   "nsIIccService");

XPCOMUtils.defineLazyServiceGetter(this, "gDataCallInterfaceService",
                                   "@mozilla.org/datacall/interfaceservice;1",
                                   "nsIDataCallInterfaceService");

XPCOMUtils.defineLazyServiceGetter(this, "gCustomizationInfo",
                                   "@kaiostech.com/customizationinfo;1",
                                   "nsICustomizationInfo");

XPCOMUtils.defineLazyServiceGetter(this, "gSystemMessenger",
                                   "@mozilla.org/system-message-internal;1",
                                   "nsISystemMessagesInternal");

XPCOMUtils.defineLazyGetter(this, "RIL", function () {
  let obj = Cu.import("resource://gre/modules/ril_consts.js", null);
  return obj;
});

// Ril quirk to attach data registration on demand.
var RILQUIRKS_DATA_REGISTRATION_ON_DEMAND =
  libcutils.property_get("ro.moz.ril.data_reg_on_demand", "false") == "true";

// Ril quirk to control the uicc/data subscription.
var RILQUIRKS_SUBSCRIPTION_CONTROL =
  libcutils.property_get("ro.moz.ril.subscription_control", "false") == "true";

// Ril quirk to enable IPv6 protocol/roaming protocol in APN settings.
var RILQUIRKS_HAVE_IPV6 =
  libcutils.property_get("ro.moz.ril.ipv6", "false") == "true";

const DATACALLMANAGER_CID =
  Components.ID("{35b9efa2-e42c-45ce-8210-0a13e6f4aadc}");
const DATACALLHANDLER_CID =
  Components.ID("{132b650f-c4d8-4731-96c5-83785cb31dee}");
const RILNETWORKINTERFACE_CID =
  Components.ID("{9574ee84-5d0d-4814-b9e6-8b279e03dcf4}");
const RILNETWORKINFO_CID =
  Components.ID("{dd6cf2f0-f0e3-449f-a69e-7c34fdcb8d4b}");

const TOPIC_XPCOM_SHUTDOWN      = "xpcom-shutdown";
const TOPIC_MOZSETTINGS_CHANGED = "mozsettings-changed";
const TOPIC_PREF_CHANGED        = "nsPref:changed";
const TOPIC_DATA_CALL_ERROR     = "data-call-error";
const PREF_RIL_DEBUG_ENABLED    = "ril.debugging.enabled";

const NETWORK_TYPE_UNKNOWN      = Ci.nsINetworkInfo.NETWORK_TYPE_UNKNOWN;
const NETWORK_TYPE_WIFI         = Ci.nsINetworkInfo.NETWORK_TYPE_WIFI;
const NETWORK_TYPE_MOBILE       = Ci.nsINetworkInfo.NETWORK_TYPE_MOBILE;
const NETWORK_TYPE_MOBILE_MMS   = Ci.nsINetworkInfo.NETWORK_TYPE_MOBILE_MMS;
const NETWORK_TYPE_MOBILE_SUPL  = Ci.nsINetworkInfo.NETWORK_TYPE_MOBILE_SUPL;
const NETWORK_TYPE_MOBILE_IMS   = Ci.nsINetworkInfo.NETWORK_TYPE_MOBILE_IMS;
const NETWORK_TYPE_MOBILE_DUN   = Ci.nsINetworkInfo.NETWORK_TYPE_MOBILE_DUN;
const NETWORK_TYPE_MOBILE_FOTA  = Ci.nsINetworkInfo.NETWORK_TYPE_MOBILE_FOTA;
const NETWORK_TYPE_MOBILE_HIPRI = Ci.nsINetworkInfo.NETWORK_TYPE_MOBILE_HIPRI;
const NETWORK_TYPE_MOBILE_CBS   = Ci.nsINetworkInfo.NETWORK_TYPE_MOBILE_CBS;
const NETWORK_TYPE_MOBILE_IA    = Ci.nsINetworkInfo.NETWORK_TYPE_MOBILE_IA;
const NETWORK_TYPE_MOBILE_ECC   = Ci.nsINetworkInfo.NETWORK_TYPE_MOBILE_ECC;
const NETWORK_TYPE_MOBILE_XCAP  = Ci.nsINetworkInfo.NETWORK_TYPE_MOBILE_XCAP;

const NETWORK_STATE_UNKNOWN       = Ci.nsINetworkInfo.NETWORK_STATE_UNKNOWN;
const NETWORK_STATE_CONNECTING    = Ci.nsINetworkInfo.NETWORK_STATE_CONNECTING;
const NETWORK_STATE_CONNECTED     = Ci.nsINetworkInfo.NETWORK_STATE_CONNECTED;
const NETWORK_STATE_DISCONNECTING = Ci.nsINetworkInfo.NETWORK_STATE_DISCONNECTING;
const NETWORK_STATE_DISCONNECTED  = Ci.nsINetworkInfo.NETWORK_STATE_DISCONNECTED;

const INT32_MAX = 2147483647;
const kPcoValueChangeObserverTopic = "pco-value-change";

//ref RIL.GECKO_RADIO_TECH
const TCP_BUFFER_SIZES = [
  null,
  "4092,8760,48000,4096,8760,48000",              // gprs
  "4093,26280,70800,4096,16384,70800",            // edge
  "58254,349525,1048576,58254,349525,1048576",    // umts
  "16384,32768,131072,4096,16384,102400",         // is95a = 1xrtt
  "16384,32768,131072,4096,16384,102400",         // is95b = 1xrtt
  "16384,32768,131072,4096,16384,102400",         // 1xrtt
  "4094,87380,262144,4096,16384,262144",          // evdo0
  "4094,87380,262144,4096,16384,262144",          // evdoa
  "61167,367002,1101005,8738,52429,262114",       // hsdpa
  "40778,244668,734003,16777,100663,301990",      // hsupa = hspa
  "40778,244668,734003,16777,100663,301990",      // hspa
  "4094,87380,262144,4096,16384,262144",          // evdob
  "131072,262144,1048576,4096,16384,524288",      // ehrpd
  "524288,1048576,2097152,262144,524288,1048576", // lte
  "122334,734003,2202010,32040,192239,576717",    // hspa+
  "4096,87380,110208,4096,16384,110208",          // gsm (using default value)
  "4096,87380,110208,4096,16384,110208",          // tdscdma (using default value)
  "122334,734003,2202010,32040,192239,576717",    // iwlan
  "122334,734003,2202010,32040,192239,576717"     // ca
];

// set to true in ril_consts.js to see debug messages
var DEBUG = RIL.DEBUG_RIL;

function updateDebugFlag() {
  // Read debug setting from pref
  let debugPref;
  try {
    debugPref = Services.prefs.getBoolPref(PREF_RIL_DEBUG_ENABLED);
  } catch (e) {
    debugPref = false;
  }
  DEBUG = debugPref || RIL.DEBUG_RIL;
}
updateDebugFlag();

function DataCallManager() {
  this._connectionHandlers = [];
  this.hasUpdateApn = false;
  let numRadioInterfaces = gMobileConnectionService.numItems;
  for (let clientId = 0; clientId < numRadioInterfaces; clientId++) {
    this._connectionHandlers.push(new DataCallHandler(clientId));

    let icc = gIccService.getIccByServiceId(clientId);
    icc.registerListener(this);
  }

  let lock = gSettingsService.createLock();
  // Read the data enabled setting from DB.
  lock.get("ril.data.enabled", this);
  lock.get("ril.data.roaming_enabled", this);
  // Read the default client id for data call.
  lock.get("ril.data.defaultServiceId", this);
  lock.get("operatorvariant.iccId", this);

  Services.obs.addObserver(this, TOPIC_XPCOM_SHUTDOWN, false);
  Services.obs.addObserver(this, TOPIC_MOZSETTINGS_CHANGED, false);
  Services.prefs.addObserver(PREF_RIL_DEBUG_ENABLED, this, false);
}
DataCallManager.prototype = {
  classID:   DATACALLMANAGER_CID,
  classInfo: XPCOMUtils.generateCI({classID: DATACALLMANAGER_CID,
                                    classDescription: "Data Call Manager",
                                    interfaces: [Ci.nsIDataCallManager]}),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDataCallManager,
                                         Ci.nsIObserver,
                                         Ci.nsIIccListener,
                                         Ci.nsISettingsServiceCallback]),

  _connectionHandlers: null,

  // Flag to determine the data state to start with when we boot up. It
  // corresponds to the 'ril.data.enabled' setting from the UI.
  _dataEnabled: false,

  // Flag to record the default client id for data call. It corresponds to
  // the 'ril.data.defaultServiceId' setting from the UI.
  _dataDefaultClientId: -1,

  // Flag to record the current default client id for data call.
  // It differs from _dataDefaultClientId in that it is set only when
  // the switch of client id process is done.
  _currentDataClientId: -1,

  // Pending function to execute when we are notified that another data call has
  // been disconnected.
  _pendingDataCallRequest: null,

  debug: function(aMsg) {
    dump("-*- DataCallManager: " + aMsg + "\n");
  },

  get dataDefaultServiceId() {
    return this._dataDefaultClientId;
  },

  getDataCallHandler: function(aClientId) {
    let handler = this._connectionHandlers[aClientId]
    if (!handler) {
      throw Cr.NS_ERROR_UNEXPECTED;
    }

    return handler;
  },

  _setDataRegistration: function(aDataCallInterface, aAttach) {
    return new Promise(function(aResolve, aReject) {
      let callback = {
        QueryInterface: XPCOMUtils.generateQI([Ci.nsIDataCallCallback]),
        notifySuccess: function() {
          aResolve();
        },
        notifyError: function(aErrorMsg) {
          aReject(aErrorMsg);
        }
      };

      aDataCallInterface.setDataRegistration(aAttach, callback);
    });
  },

  _handleDataClientIdChange: function(aNewClientId) {
    if (this._dataDefaultClientId === aNewClientId) {
       return;
    }
    this._dataDefaultClientId = aNewClientId;
    //Reset the hasUpdateApn due to _dataDefaultClientId change.
    this.hasUpdateApn = false;

    // This is to handle boot up stage.
    if (this._currentDataClientId == -1) {
      this._currentDataClientId = this._dataDefaultClientId;
      let connHandler = this._connectionHandlers[this._currentDataClientId];
      let dcInterface = connHandler.dataCallInterface;
      if (RILQUIRKS_DATA_REGISTRATION_ON_DEMAND ||
          RILQUIRKS_SUBSCRIPTION_CONTROL) {
        this._setDataRegistration(dcInterface, true);
      }
      if (this._dataEnabled) {
        let settings = connHandler.dataCallSettings;
        settings.oldEnabled = settings.enabled;
        settings.enabled = true;
        connHandler.updateAllRILNetworkInterface();
      }
      return;
    }

    let oldConnHandler = this._connectionHandlers[this._currentDataClientId];
    let oldIface = oldConnHandler.dataCallInterface;
    let oldSettings = oldConnHandler.dataCallSettings;
    let newConnHandler = this._connectionHandlers[this._dataDefaultClientId];
    let newIface = newConnHandler.dataCallInterface;
    let newSettings = newConnHandler.dataCallSettings;

    let applyPendingDataSettings = () => {
      if (DEBUG) {
        this.debug("Apply pending data registration and settings.");
      }

      if (RILQUIRKS_DATA_REGISTRATION_ON_DEMAND ||
          RILQUIRKS_SUBSCRIPTION_CONTROL) {

        // Config the mobile network setting.
        if (this._dataEnabled) {
          newSettings.oldEnabled = newSettings.enabled;
          newSettings.enabled = true;
        }
        this._currentDataClientId = this._dataDefaultClientId;

        // Config the DDS value.
        oldSettings.defaultDataSlot = false;
        newSettings.defaultDataSlot = true;
        this._setDataRegistration(oldIface, false);
        this._setDataRegistration(newIface, true).then(() => {
          newConnHandler.updateAllRILNetworkInterface();
        });
        return;
      }

      if (this._dataEnabled) {
        newSettings.oldEnabled = newSettings.enabled;
        newSettings.enabled = true;
      }

      this._currentDataClientId = this._dataDefaultClientId;
      newConnHandler.updateAllRILNetworkInterface();
    };

    if (this._dataEnabled) {
      oldSettings.oldEnabled = oldSettings.enabled;
      oldSettings.enabled = false;
    }

    oldConnHandler.deactivateAllDataCallsAndWait(RIL.DATACALL_DEACTIVATE_SERVICEID_CHANGED)
      .then(() => {
        applyPendingDataSettings();
      });
  },

  _shutdown: function() {
    for (let handler of this._connectionHandlers) {
      handler.shutdown();
    }
    this._connectionHandlers = null;
    Services.prefs.removeObserver(PREF_RIL_DEBUG_ENABLED, this);
    Services.obs.removeObserver(this, TOPIC_XPCOM_SHUTDOWN);
    Services.obs.removeObserver(this, TOPIC_MOZSETTINGS_CHANGED);
  },

  /**
   * nsIIccListener interface methods.
   */
  notifyStkCommand: function() {},

  notifyStkSessionEnd: function() {},

  notifyIsimInfoChanged: function() {},

  notifyCardStateChanged: function() {},

  notifyIccInfoChanged: function () {

    // Updated the latest icc id report by ril for each handler.
    for (let clientId in this._connectionHandlers) {
      let handler = this._connectionHandlers[clientId];
      let icc = gIccService.getIccByServiceId(clientId);
      let iccInfo = icc && icc.iccInfo;
      handler.newIccid = iccInfo && iccInfo.iccid;
    }

    if (this._dataDefaultClientId == -1) {
      this.debug("this._dataDefaultClientId not initial yet");
      return;
    }

    if (this.hasUpdateApn === true) {
      return;
    }

    let ddsHandler = this._connectionHandlers[this._dataDefaultClientId];

    if (DEBUG) {
      this.debug("newIccid=" + ddsHandler.newIccid + ", oldIccid=" + ddsHandler.oldIccid);
    }
    if (ddsHandler.newIccid && ddsHandler.oldIccid && (ddsHandler.newIccid === ddsHandler.oldIccid)) {
      let lock = gSettingsService.createLock();
      lock.get("ril.data.apnSettings", this);
      this.hasUpdateApn = true;
    }
  },
  /**
   * nsISettingsServiceCallback
   */
  handle: function(aName, aResult) {
    switch (aName) {
      case "ril.data.apnSettings":
        if (DEBUG) {
          this.debug("'ril.data.apnSettings' is now " +
                     JSON.stringify(aResult));
        }
        if (!aResult) {
          break;
        }
        for (let clientId in this._connectionHandlers) {
          let handler = this._connectionHandlers[clientId];

          // Loading the apn value.
          let apnSetting = aResult[clientId];
          if (handler && apnSetting) {
            if (gCustomizationInfo.getCustomizedValue(clientId, "xcap", null) != null) {
              apnSetting.push(gCustomizationInfo.getCustomizedValue(clientId, "xcap"));
            }
            handler.updateApnSettings(apnSetting);
          }

          // Once got the apn, loading the white list config if any.
          if (apnSetting && apnSetting.length > 0) {
            let whiteList = gCustomizationInfo.getCustomizedValue(clientId, "mobileSettingWhiteList", []);
            if (whiteList.length > 0) {
              handler.mobileWhiteList = whiteList;
              this.debug("mobileWhiteList[" + clientId + "]:" + JSON.stringify(handler.mobileWhiteList));
            }

            // Config the setting whitelist value.
            let lock = gSettingsService.createLock();
            lock.set('ril.data.mobileWhiteList', handler.mobileWhiteList, null);
          }
        }
        break;
      case "ril.data.enabled":
        if (DEBUG) {
          this.debug("'ril.data.enabled' is now " + aResult);
        }
        if (this._dataEnabled === aResult) {
          break;
        }
        this._dataEnabled = aResult;

        if (DEBUG) {
          this.debug("Default id for data call: " + this._dataDefaultClientId);
        }
        if (this._dataDefaultClientId === -1) {
          // We haven't got the default id for data from db.
          break;
        }

        let connHandler = this._connectionHandlers[this._dataDefaultClientId];
        let settings = connHandler.dataCallSettings;
        settings.oldEnabled = settings.enabled;
        settings.enabled = aResult;
        connHandler.updateAllRILNetworkInterface();
        break;
      case "ril.data.roaming_enabled":
        if (DEBUG) {
          this.debug("'ril.data.roaming_enabled' is now " + aResult);
          this.debug("Default id for data call: " + this._dataDefaultClientId);
        }
        for (let clientId = 0; clientId < this._connectionHandlers.length; clientId++) {
          let connHandler = this._connectionHandlers[clientId];
          let settings = connHandler.dataCallSettings;
          settings.roamingEnabled = Array.isArray(aResult) ? aResult[clientId]
                                                           : aResult;
        }
        if (this._dataDefaultClientId === -1) {
          // We haven't got the default id for data from db.
          break;
        }
        this._connectionHandlers[this._dataDefaultClientId].updateAllRILNetworkInterface();
        break;
      case "ril.data.defaultServiceId":
        aResult = aResult || 0;
        if (DEBUG) {
          this.debug("'ril.data.defaultServiceId' is now " + aResult);
        }
        this._handleDataClientIdChange(aResult);
        break;
      // We only get operatorvariant.iccId once when the device starts up now.
      // After modem gives us the hot swap event, we will get operatorvariant.iccId
      // as old iccid and decide whether to get apn or not.
      case "operatorvariant.iccId":

        aResult = aResult || 0;

        // Updated the previous icc id store in the setting for each handler.
        for (let clientId in this._connectionHandlers) {
          let handler = this._connectionHandlers[clientId];
          handler.oldIccid = aResult[clientId];
        }

        if (this._dataDefaultClientId == -1) {
          this.debug("this._dataDefaultClientId not initial yet");
          break;
        }

        if (this.hasUpdateApn === true) {
          break;
        }

        let ddsHandler = this._connectionHandlers[this._dataDefaultClientId];

        let newIccid = ddsHandler.newIccid;
        let oldIccid = ddsHandler.oldIccid;

        if (DEBUG) {
          this.debug("oldIccid=" + oldIccid + ", newIccid=" + newIccid);
        }
        if (oldIccid && newIccid && (oldIccid === newIccid)) {
          let lock = gSettingsService.createLock();
          lock.get("ril.data.apnSettings", this);
          this.hasUpdateApn = true;
        }
        break;
    }
  },

  handleError: function(aErrorMessage) {
    if (DEBUG) {
      this.debug("There was an error while reading RIL settings.");
    }
  },

  /**
   * nsIObserver interface methods.
   */
  observe: function(aSubject, aTopic, aData) {
    switch (aTopic) {
      case TOPIC_MOZSETTINGS_CHANGED:
        if ("wrappedJSObject" in aSubject) {
          aSubject = aSubject.wrappedJSObject;
        }
        this.handle(aSubject.key, aSubject.value);
        break;
      case TOPIC_PREF_CHANGED:
        if (aData === PREF_RIL_DEBUG_ENABLED) {
          updateDebugFlag();
        }
        break;
      case TOPIC_XPCOM_SHUTDOWN:
        this._shutdown();
        break;
    }
  },
};

// Check rat value include in the bit map or not.
function bitmaskHasTech(aBearerBitmask, aRadioTech) {
  if (aBearerBitmask == 0) {
    return true;
  } else if (aRadioTech > 0) {
    return ((aBearerBitmask & (1 << (aRadioTech - 1))) != 0);
  }
  return false;
};

// Show the detail rat type.
function bitmaskToString(aBearerBitmask){
  if(aBearerBitmask == 0 || aBearerBitmask === undefined){
    return 0;
  }

  let val = "";
  for (let i = 1 ; i < RIL.GECKO_RADIO_TECH.length ; i++ ){
    if ((aBearerBitmask & (1 << (i - 1))) != 0){
      val = val.concat(i + "|");
    }
  }
  return val;
};

function convertToDataCallType(aNetworkType) {
  switch (aNetworkType) {
    case NETWORK_TYPE_MOBILE:
      return "default";
    case NETWORK_TYPE_MOBILE_MMS:
      return "mms";
    case NETWORK_TYPE_MOBILE_SUPL:
      return "supl";
    case NETWORK_TYPE_MOBILE_IMS:
      return "ims";
    case NETWORK_TYPE_MOBILE_DUN:
      return "dun";
    case NETWORK_TYPE_MOBILE_FOTA:
      return "fota";
    case NETWORK_TYPE_MOBILE_IA:
      return "ia";
    case NETWORK_TYPE_MOBILE_XCAP:
      return "xcap";
    case NETWORK_TYPE_MOBILE_CBS:
      return "cbs";
    case NETWORK_TYPE_MOBILE_HIPRI:
      return "hipri";
    case NETWORK_TYPE_MOBILE_ECC:
      return "ecc";
    default:
      return "unknown";
  }
};

function convertToDataCallState(aState) {
  switch (aState) {
    case NETWORK_STATE_CONNECTING:
      return "connecting";
    case NETWORK_STATE_CONNECTED:
      return "connected";
    case NETWORK_STATE_DISCONNECTING:
      return "disconnecting";
    case NETWORK_STATE_DISCONNECTED:
      return "disconnected";
    default:
      return "unknown";
  }
};

function DataCallHandler(aClientId) {
  // Initial owning attributes.
  this.clientId = aClientId;
  this.dataCallSettings = {
    oldEnabled: false,
    enabled: false,
    roamingEnabled: false,
    defaultDataSlot: false
  };
  this._dataCalls = [];
  this._listeners = [];

  // This map is used to collect all the apn types and its corresponding
  // RILNetworkInterface.
  this.dataNetworkInterfaces = new Map();

  this.dataCallInterface = gDataCallInterfaceService.getDataCallInterface(aClientId);
  this.dataCallInterface.registerListener(this);

  let mobileConnection = gMobileConnectionService.getItemByServiceId(aClientId);
  mobileConnection.registerListener(this);

  this._dataInfo = {
    state: mobileConnection.data.state,
    type: mobileConnection.data.type,
    roaming: mobileConnection.data.roaming
  }

  this.newIccid = null;
  this.oldIccid = null;
  this.needRecoverAfterReset = false;
  this.mobileWhiteList = [];
}
DataCallHandler.prototype = {
  classID:   DATACALLHANDLER_CID,
  classInfo: XPCOMUtils.generateCI({classID: DATACALLHANDLER_CID,
                                    classDescription: "Data Call Handler",
                                    interfaces: [Ci.nsIDataCallHandler]}),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDataCallHandler,
                                         Ci.nsIDataCallInterfaceListener,
                                         Ci.nsIMobileConnectionListener]),

  clientId: 0,
  dataCallInterface: null,
  dataCallSettings: null,
  dataNetworkInterfaces: null,
  _dataCalls: null,
  _dataInfo: null,

  // Apn settings to be setup after data call are cleared.
  _pendingApnSettings: null,

  // Modem reset recover.
  needRecoverAfterReset: false,

  /** White list control
   * Config the value in the /gecko/proprietary/customization/resources by mccmnc json.
   * Key: mobileSettingWhiteList, Value: String Array.
   * Example:
   * "mobileSettingWhiteList" : {
   *  "description" : "This flag is for the mobile setting white list, any apn type in this list would not be control by mobile network setting.",
   *  "value" : ["ims","mms"]
   * }
   */
  mobileWhiteList: null,

  debug: function(aMsg) {
    dump("-*- DataCallHandler[" + this.clientId + "]: " + aMsg + "\n");
  },

  shutdown: function() {
    // Shutdown all RIL network interfaces
    this.dataNetworkInterfaces.forEach(function(networkInterface) {
      gNetworkManager.unregisterNetworkInterface(networkInterface);
      networkInterface.shutdown();
      networkInterface = null;
    });
    this.dataNetworkInterfaces.clear();
    this._dataCalls = [];
    this.clientId = null;

    this.dataCallInterface.unregisterListener(this);
    this.dataCallInterface = null;

    let mobileConnection =
      gMobileConnectionService.getItemByServiceId(this.clientId);
    mobileConnection.unregisterListener(this);
  },

  /**
   * Check if we get all necessary APN data.
   */
  _validateApnSetting: function(aApnSetting) {
    return (aApnSetting &&
            aApnSetting.apn &&
            aApnSetting.types &&
            aApnSetting.types.length);
  },

  _convertApnType: function(aApnType) {
    switch (aApnType) {
      case "default":
        return NETWORK_TYPE_MOBILE;
      case "mms":
        return NETWORK_TYPE_MOBILE_MMS;
      case "supl":
        return NETWORK_TYPE_MOBILE_SUPL;
      case "ims":
        return NETWORK_TYPE_MOBILE_IMS;
      case "dun":
        return NETWORK_TYPE_MOBILE_DUN;
      case "fota":
        return NETWORK_TYPE_MOBILE_FOTA;
      case "ia":
        return NETWORK_TYPE_MOBILE_IA;
      case "xcap":
        return NETWORK_TYPE_MOBILE_XCAP;
      case "cbs":
        return NETWORK_TYPE_MOBILE_CBS;
      case "hipri":
        return NETWORK_TYPE_MOBILE_HIPRI;
      case "ecc":
        return NETWORK_TYPE_MOBILE_ECC;
      default:
        return NETWORK_TYPE_UNKNOWN;
     }
  },

  _compareDataCallOptions: function(aDataCall, aNewDataCall) {
    return aDataCall.apnProfile.apn == aNewDataCall.apnProfile.apn &&
           aDataCall.apnProfile.user == aNewDataCall.apnProfile.user &&
           aDataCall.apnProfile.password == aNewDataCall.apnProfile.passwd &&
           aDataCall.apnProfile.authType == aNewDataCall.apnProfile.authType &&
           aDataCall.apnProfile.protocol == aNewDataCall.apnProfile.protocol &&
           aDataCall.apnProfile.roaming_protocol == aNewDataCall.apnProfile.roaming_protocol &&
           aDataCall.apnProfile.bearer == aNewDataCall.apnProfile.bearer;
  },

  /**
    * Handle muti apn with same apn type mechanism.
    */
  _setupApnSettings: function(aNewApnSettings){
    if (!aNewApnSettings) {
      return;
    }
    if (DEBUG) this.debug("_setupApnSettings: " + JSON.stringify(aNewApnSettings));

    // Shutdown all network interfaces and clear data calls.
    this.dataNetworkInterfaces.forEach(function(networkInterface) {
      gNetworkManager.unregisterNetworkInterface(networkInterface);
      networkInterface.shutdown();
      networkInterface = null;
    });
    this.dataNetworkInterfaces.clear();
    this._dataCalls = [];
    let apnContextsList = new Map();

    //1. Create mapping table for apn setting and DataCall.
    for (let inputApnSetting of aNewApnSettings) {
      if (!this._validateApnSetting(inputApnSetting)) {
        continue;
      }
      //Create data call list.
      let dataCall;
      for (let i = 0; i < this._dataCalls.length; i++) {
        if (this._dataCalls[i].canHandleApn(inputApnSetting)) {
          if (DEBUG) this.debug("Found shareable DataCall, reusing it.");
          dataCall = this._dataCalls[i];
          break;
        }
      }
      if (!dataCall) {
        if (DEBUG) this.debug("No shareable DataCall found, creating one. inputApnSetting=" + JSON.stringify(inputApnSetting));
        dataCall = new DataCall(this.clientId, inputApnSetting, this);
        this._dataCalls.push(dataCall);
      }
      //Create apn type map to DC list.
      for (let i = 0; i < inputApnSetting.types.length; i++) {
        let networkType = this._convertApnType(inputApnSetting.types[i]);
        if (networkType === NETWORK_TYPE_UNKNOWN) {
          if (DEBUG) this.debug("Invalid apn type: " + networkType);
          continue;
        }
        let dataCallsList = [];
        if(apnContextsList.get(networkType) !== undefined){
          dataCallsList = apnContextsList.get(networkType);
        }

        if (DEBUG) this.debug("type: " + convertToDataCallType(networkType) + ", dataCall:"+  dataCall.apnProfile.apn);
        dataCallsList.push(dataCall);
        apnContextsList.set(networkType , dataCallsList);
      }
    }

    //2. Create RadioNetworkInterface
    for (let [networkType, dataCallsList] of apnContextsList) {
      try {
        if (DEBUG) this.debug("Preparing RILNetworkInterface for type: " + convertToDataCallType(networkType));
        let networkInterface = new RILNetworkInterface(this, networkType,
                                                       null,
                                                       dataCallsList);
        gNetworkManager.registerNetworkInterface(networkInterface);
        this.dataNetworkInterfaces.set(networkType, networkInterface);
        //Set the default networkInterface to enable.
        if (networkInterface.info.type == NETWORK_TYPE_MOBILE) {
          this.debug("Enable the default RILNetworkInterface.");
          networkInterface.enable();
        }
      } catch (e) {
        if (DEBUG) {
          this.debug("Error setting up RILNetworkInterface for type " +
                      apnType + ": " + e);
        }
      }
    }
  },

  /**
   * Check if all data is disconnected.
   */
  allDataDisconnected: function() {
    for (let i = 0; i < this._dataCalls.length; i++) {
      let dataCall = this._dataCalls[i];
      if (dataCall.state != NETWORK_STATE_UNKNOWN &&
          dataCall.state != NETWORK_STATE_DISCONNECTED) {
        return false;
      }
    }
    return true;
  },

  deactivateAllDataCallsAndWait: function(aReason) {
    return new Promise((aResolve, aReject) => {
      this.deactivateAllDataCalls(aReason,
        {
          notifyDataCallsDisconnected: function() {
            aResolve();
        }
      });
    });
  },

  updateApnSettings: function(aNewApnSettings) {
    if (!aNewApnSettings) {
      return;
    }
    if (this._pendingApnSettings) {
      // Change of apn settings in process, just update to the newest.
      this._pengingApnSettings = aNewApnSettings;
      return;
    }

    this._pendingApnSettings = aNewApnSettings;
    this.setInitialAttachApn(this._pendingApnSettings);
    this.deactivateAllDataCallsAndWait(RIL.DATACALL_DEACTIVATE_APN_CHANGED).then(() => {
      this._setupApnSettings(this._pendingApnSettings);
      this._pendingApnSettings = null;
      this.updateAllRILNetworkInterface();
    });
  },

  setInitialAttachApn: function(aNewApnSettings) {
      if (!aNewApnSettings) {
        return;
      }

      let iaApnSetting;
      let defaultApnSetting;
      let firstApnSetting;

      for (let inputApnSetting of aNewApnSettings) {
        if(!this._validateApnSetting(inputApnSetting)) {
          continue;
        }

        if (!firstApnSetting) {
          firstApnSetting = inputApnSetting;
        }

        for (let i = 0; i < inputApnSetting.types.length; i++) {
          let apnType = inputApnSetting.types[i];
          let networkType = this._convertApnType(apnType);
          if (networkType == NETWORK_TYPE_MOBILE_IA) {
            iaApnSetting = inputApnSetting;
          } else if (networkType == NETWORK_TYPE_MOBILE) {
            defaultApnSetting = inputApnSetting;
          }
        }
      }

      let initalAttachApn;
      if (iaApnSetting) {
        initalAttachApn = iaApnSetting;
      } else if (defaultApnSetting) {
        initalAttachApn = defaultApnSetting;
      } else if (firstApnSetting) {
        initalAttachApn = firstApnSetting;
      } else {
        if (DEBUG) {
          this.debug("Can not find any initial attach APN!");
        }
        return;
      }

      let connection = gMobileConnectionService.getItemByServiceId(this.clientId);
      let dataInfo = connection && connection.data;
      if (dataInfo == null) {
        return;
      }

      let pdpType = !dataInfo.roaming
                  ? RIL.RIL_DATACALL_PDP_TYPES.indexOf(initalAttachApn.protocol)
                  : RIL.RIL_DATACALL_PDP_TYPES.indexOf(initalAttachApn.roaming_protocol);
      if (pdpType == -1) {
        pdpType = RIL.GECKO_DATACALL_PDP_TYPE_IP;
      } else {
        // Found, convert back to IP/IPV4V6/IPV6 string.
        pdpType = RIL.RIL_DATACALL_PDP_TYPES[pdpType];
      }

      let authtype = RIL.RIL_DATACALL_AUTH_TO_GECKO.indexOf(initalAttachApn.authtype);
      if (authtype == -1) {
        authtype = RIL.RIL_DATACALL_AUTH_TO_GECKO.indexOf(RIL.GECKO_DATACALL_AUTH_DEFAULT);
      }

      this.dataCallInterface.setInitialAttachApn(
        initalAttachApn.apn || "",
        pdpType,
        authtype,
        initalAttachApn.user || "",
        initalAttachApn.password || ""
      );
  },

  updatePcoData: function(aCid, aBearerProtom, aPcoId, aContents, aCount) {
    if (DEBUG) {
      this.debug("updatePcoData aCid=" + aCid +
                  " ,aBearerProtom=" + aBearerProtom +
                  " ,aPcoId=" + aPcoId +
                  " ,aContents=" + JSON.stringify(aContents) +
                  " ,aCount=" + aCount);
    }

    let pcoDataCalls = [];
    let dataCalls = this._dataCalls.slice();

    // Looking target pco data connection.(Connected)
    for (let i = 0; i < dataCalls.length; i++) {
      let dataCall = dataCalls[i];
      if (dataCall &&
          dataCall.state == NETWORK_STATE_CONNECTED &&
          dataCall.linkInfo.cid == aCid) {
        if (DEBUG) {
          this.debug("Found pco data cid: " + dataCall.linkInfo.cid);
        }
        pcoDataCalls.push(dataCall);
      }
    }

    // Looking protential pco data connection.(Connecting)
    if (pcoDataCalls.length === 0) {
      for (let i = 0; i < dataCalls.length; i++) {
        let dataCall = dataCalls[i];
        if (dataCall &&
            dataCall.state == NETWORK_STATE_CONNECTING &&
            dataCall.requestedNetworkIfaces.lengh > 0) {
          if (DEBUG) {
            this.debug("Found pco protential data. apn=" + dataCall.apnSetting.apn);
          }
          pcoDataCalls.push(dataCall);
        }
      }
    }

    if (pcoDataCalls.length === 0) {
      this.debug("PCO_DATA - couldn't infer cid.");
      return;
    }
    // Sending out the PCO information for each apn type.
    for (let i = 0; i < pcoDataCalls.length; i++) {
      let pcoDataCall = pcoDataCalls[i];
      for (let i = 0; i < pcoDataCall.requestedNetworkIfaces.length; i++) {
        try {
          gSystemMessenger.broadcastMessage(
            kPcoValueChangeObserverTopic, {
              apnType:            pcoDataCall.requestedNetworkIfaces[i].info.type,
              bearerProto:        aBearerProtom,
              pcoId:              aPcoId,
              contents:           aContents
          });
        } catch (e) {
          if (DEBUG) {
            this.debug("Failed to broadcastPcoChangeMessage: " + e);
          }
        }
      }
    }
  },

  updateRILNetworkInterface: function() {
    if (DEBUG) {
      this.debug("updateRILNetworkInterface");
    }

    let networkInterface = this.dataNetworkInterfaces.get(NETWORK_TYPE_MOBILE);

    if (!networkInterface) {
       if (DEBUG) {
         this.debug("updateRILNetworkInterface No network interface for default data.");
       }
       return;
    }

    this.onUpdateRILNetworkInterface(networkInterface);
  },

  onUpdateRILNetworkInterface: function(aNetworkInterface) {
    if (!aNetworkInterface) {
      if (DEBUG) {
        this.debug("onUpdateRILNetworkInterface No network interface.");
      }
       return;
    }

    if (DEBUG) {
      this.debug("onUpdateRILNetworkInterface type:" + convertToDataCallType(aNetworkInterface.info.type));
    }

    let connection =
      gMobileConnectionService.getItemByServiceId(this.clientId);

    let dataInfo = connection && connection.data;
    let radioTechType = dataInfo && dataInfo.type;
    let radioTechnology = RIL.GECKO_RADIO_TECH.indexOf(radioTechType);

    // This check avoids data call connection if the radio is not ready
    // yet after toggling off airplane mode.
    let radioState = connection && connection.radioState;
    if (radioState != Ci.nsIMobileConnection.MOBILE_RADIO_STATE_ENABLED) {
      if (radioTechnology != RIL.NETWORK_CREG_TECH_IWLAN) {
        if (DEBUG) {
          this.debug("RIL is not ready for data connection: radio's not ready");
        }
        return;
      } else {
        if (DEBUG) {
          this.debug("IWLAN network consider as radio power on.");
        }
      }
    }

    let isRegistered =
      dataInfo &&
      dataInfo.state == RIL.GECKO_MOBILE_CONNECTION_STATE_REGISTERED;
    let haveDataConnection =
      dataInfo &&
      dataInfo.type != RIL.GECKO_MOBILE_CONNECTION_STATE_UNKNOWN;
    if (!isRegistered || !haveDataConnection) {
      if (DEBUG) {
        this.debug("RIL is not ready for data connection: Phone's not " +
                   "registered or doesn't have data connection.");
      }
      return;
    }

    let wifi_active = false;
    if (gNetworkManager.activeNetworkInfo &&
        gNetworkManager.activeNetworkInfo.type == NETWORK_TYPE_WIFI) {
      wifi_active = true;
    }

    let isDefault = (aNetworkInterface.info.type == NETWORK_TYPE_MOBILE) ? true : false;
    let dataCallConnected = aNetworkInterface.connected;

    if ((!this.isDataAllow(aNetworkInterface) ||
         (dataInfo.roaming && !this.dataCallSettings.roamingEnabled))) {
      if (DEBUG) {
        this.debug("Data call settings: disconnect data call."
          + " ,MobileEnable: " + this.dataCallSettings.enabled
          + " ,Roaming: " + dataInfo.roaming
          + " ,RoamingEnabled: " + this.dataCallSettings.roamingEnabled);
      }
      aNetworkInterface.disconnect(Ci.nsINetworkInfo.REASON_SETTING_DISABLED);
      return;
    }

    if (isDefault && dataCallConnected && wifi_active) {
      if (DEBUG) {
        this.debug("Disconnect default data call when Wifi is connected.");
      }
      aNetworkInterface.disconnect(Ci.nsINetworkInfo.REASON_WIFI_CONNECTED);
      return;
    }

    if (isDefault && wifi_active) {
      if (DEBUG) {
        this.debug("Don't connect default data call when Wifi is connected.");
      }
      return;
    }

    if (dataCallConnected) {
      if (DEBUG) {
        this.debug("Already connected. dataCallConnected: " + dataCallConnected);
      }
      return;
    }

    if (this._pendingApnSettings) {
      if (DEBUG) this.debug("We're changing apn settings, ignore any changes.");
      return;
    }

    if (this._deactivatingAllDataCalls) {
      if (DEBUG) this.debug("We're deactivating all data calls, ignore any changes.");
      return;
    }

    if (DEBUG) {
      this.debug("Data call settings: connect data call.");
    }

    aNetworkInterface.connect();
  },

  isDataAllow: function(aNetworkInterface) {
    let allow = this.dataCallSettings.enabled;
    let isDefault = (aNetworkInterface.info.type == NETWORK_TYPE_MOBILE) ? true : false;

    // Default type can not be whitelist member.
    if (!allow && !isDefault) {
      allow = this.mobileWhiteList.indexOf(convertToDataCallType(aNetworkInterface.info.type)) == -1 ? false : true;
      if (DEBUG && allow) {
        this.debug("Allow data call for mobile whitelist type:" + convertToDataCallType(aNetworkInterface.info.type));
      }
    }
    return allow;
  },

  _isMobileNetworkType: function(aNetworkType) {
    if (aNetworkType === NETWORK_TYPE_MOBILE ||
        aNetworkType === NETWORK_TYPE_MOBILE_MMS ||
        aNetworkType === NETWORK_TYPE_MOBILE_SUPL ||
        aNetworkType === NETWORK_TYPE_MOBILE_IMS ||
        aNetworkType === NETWORK_TYPE_MOBILE_DUN ||
        aNetworkType === NETWORK_TYPE_MOBILE_FOTA ||
        aNetworkType === NETWORK_TYPE_MOBILE_HIPRI ||
        aNetworkType === NETWORK_TYPE_MOBILE_CBS ||
        aNetworkType === NETWORK_TYPE_MOBILE_IA ||
        aNetworkType === NETWORK_TYPE_MOBILE_ECC ||
        aNetworkType === NETWORK_TYPE_MOBILE_XCAP) {
      return true;
    }

    return false;
  },

  getDataCallStateByType: function(aNetworkType) {
    if (!this._isMobileNetworkType(aNetworkType)) {
      if (DEBUG) this.debug(aNetworkType + " is not a mobile network type!");
      throw Cr.NS_ERROR_INVALID_ARG;
    }

    let networkInterface = this.dataNetworkInterfaces.get(aNetworkType);
    if (!networkInterface) {
      return NETWORK_STATE_UNKNOWN;
    }
    return networkInterface.info.state;
  },

  setupDataCallByType: function(aNetworkType) {
    if (DEBUG) {
      this.debug("setupDataCallByType: " + convertToDataCallType(aNetworkType));
    }

    if (!this._isMobileNetworkType(aNetworkType)) {
      if (DEBUG) this.debug(aNetworkType + " is not a mobile network type!");
      throw Cr.NS_ERROR_INVALID_ARG;
    }

    let networkInterface = this.dataNetworkInterfaces.get(aNetworkType);
    if (!networkInterface) {
      if (DEBUG) {
        this.debug("No network interface for type: " + convertToDataCallType(aNetworkType));
      }
      return;
    }

    networkInterface.enable();
    this.onUpdateRILNetworkInterface(networkInterface);
  },

  deactivateDataCallByType: function(aNetworkType) {
    if (DEBUG) {
      this.debug("deactivateDataCallByType: " + convertToDataCallType(aNetworkType));
    }

    if (!this._isMobileNetworkType(aNetworkType)) {
      if (DEBUG) this.debug(aNetworkType + " is not a mobile network type!");
      throw Cr.NS_ERROR_INVALID_ARG;
    }

    let networkInterface = this.dataNetworkInterfaces.get(aNetworkType);
    if (!networkInterface) {
      if (DEBUG) {
        this.debug("No network interface for type: " + convertToDataCallType(aNetworkType));
      }
      return;
    }

    // Not allow AP control the default RILNetworkInterface
    if (networkInterface.info.type == NETWORK_TYPE_MOBILE) {
      if (DEBUG) {
        this.debug("Not allow upper layer control the default RILNetworkInterface ");
      }
      return;
    }
    networkInterface.disable();
  },

  _deactivatingAllDataCalls: false,

  deactivateAllDataCalls: function(aReason, aCallback) {
    if (DEBUG) {
      this.debug("deactivateAllDataCalls: aReason=" + aReason);
    }
    let dataDisconnecting = false;
    this.dataNetworkInterfaces.forEach(function(networkInterface) {
      if (networkInterface.enabled) {
        if (networkInterface.info.state != NETWORK_STATE_UNKNOWN &&
            networkInterface.info.state != NETWORK_STATE_DISCONNECTED) {
          dataDisconnecting = true;
        }
        networkInterface.disconnect(aReason);
      }
    });

    this._deactivatingAllDataCalls = dataDisconnecting;
    if (!dataDisconnecting) {
      aCallback.notifyDataCallsDisconnected();
      return;
    }

    let callback = {
      notifyAllDataDisconnected: () => {
        this._unregisterListener(callback);
        aCallback.notifyDataCallsDisconnected();
      }
    };
    this._registerListener(callback);
  },

  _listeners: null,

  _notifyListeners: function(aMethodName, aArgs) {
    let listeners = this._listeners.slice();
    for (let listener of listeners) {
      if (this._listeners.indexOf(listener) == -1) {
        // Listener has been unregistered in previous run.
        continue;
      }

      let handler = listener[aMethodName];
      try {
        handler.apply(listener, aArgs);
      } catch (e) {
        this.debug("listener for " + aMethodName + " threw an exception: " + e);
      }
    }
  },

  _registerListener: function(aListener) {
    if (this._listeners.indexOf(aListener) >= 0) {
      return;
    }

    this._listeners.push(aListener);
  },

  _unregisterListener: function(aListener) {
    let index = this._listeners.indexOf(aListener);
    if (index >= 0) {
      this._listeners.splice(index, 1);
    }
  },

  _findDataCallByCid: function(aCid) {
    if (aCid === undefined || aCid < 0) {
      return -1;
    }

    for (let i = 0; i < this._dataCalls.length; i++) {
      let datacall = this._dataCalls[i];
      if (datacall.linkInfo.cid != null &&
          datacall.linkInfo.cid == aCid) {
        return i;
      }
    }

    return -1;
  },

  /**
   * Notify about data call setup error, called from DataCall.
   */
  notifyDataCallError: function(aDataCall, aErrorMsg) {
    // Notify data call error only for default APN.
    let networkInterface = this.dataNetworkInterfaces.get(NETWORK_TYPE_MOBILE);

    if (networkInterface && networkInterface.enable) {
      for (let i = 0; i < aDataCall.requestedNetworkIfaces.length; i++) {
       if (aDataCall.requestedNetworkIfaces[i].info.type == NETWORK_TYPE_MOBILE) {
          Services.obs.notifyObservers(networkInterface.info,
                                         TOPIC_DATA_CALL_ERROR, aErrorMsg);
          break;
        }
      }
    }
  },

  /**
   * Notify about data call changed, called from DataCall.
   */
  notifyDataCallChanged: function(aUpdatedDataCall) {
    // Process pending radio power off request after all data calls
    // are disconnected.
    if (aUpdatedDataCall.state == NETWORK_STATE_DISCONNECTED ||
        aUpdatedDataCall.state == NETWORK_STATE_UNKNOWN &&
        this.allDataDisconnected() && this._deactivatingAllDataCalls) {
      this._deactivatingAllDataCalls = false;
      this._notifyListeners("notifyAllDataDisconnected", {
        clientId: this.clientId
      });
    }
  },

  // nsIDataCallInterfaceListener

  notifyDataCallListChanged: function(aCount, aDataCallList) {
    let currentDataCalls = this._dataCalls.slice();
    for (let i = 0; i < aDataCallList.length; i++) {
      let dataCall = aDataCallList[i];
      let index = this._findDataCallByCid(dataCall.cid);
      if (index == -1) {
        if (DEBUG) {
          this.debug("Unexpected new data call: " + JSON.stringify(dataCall));
        }
        continue;
      }
      currentDataCalls[index].onDataCallChanged(dataCall);
      currentDataCalls[index] = null;
    }

    // If there is any CONNECTED DataCall left in currentDataCalls, means that
    // it is missing in dataCallList, we should send a DISCONNECTED event to
    // notify about this.
    for (let i = 0; i < currentDataCalls.length; i++) {
      let currentDataCall = currentDataCalls[i];
      if (currentDataCall && currentDataCall.linkInfo.cid != null &&
          currentDataCall.state == NETWORK_STATE_CONNECTED) {
        if (DEBUG) {
          this.debug("Expected data call missing: " + JSON.stringify(
            currentDataCall.apnProfile) + ", must have been DISCONNECTED.");
        }
        currentDataCall.onDataCallChanged({
          state: NETWORK_STATE_DISCONNECTED
        });
      }
    }
  },

  // Rat change should trigger DC to check the current rat can be handle or not
  // and process the retry for all dataNetworkInterfaces(apntype).
  handleDataRegistrationChange: function() {
    if ( !this._dataInfo ||
        this._dataInfo.state != RIL.GECKO_MOBILE_CONNECTION_STATE_REGISTERED ||
        this._dataInfo.type == RIL.GECKO_MOBILE_CONNECTION_STATE_UNKNOWN) {
      this.debug("handleDataRegistrationChange: Network state not ready. Abort.");
      return;
    }

    let radioTechnology = RIL.GECKO_RADIO_TECH.indexOf(this._dataInfo.type);
    if (DEBUG) {
      this.debug("handleDataRegistrationChange radioTechnology: "+ radioTechnology);
    }
    // Let datacall decided if currecnt DC can support this rat type.
    for (let i = 0; i < this._dataCalls.length; i++) {
      let datacall = this._dataCalls[i];
      datacall.dataRegistrationChanged(radioTechnology);
    }

    // Retry datacall if any.
    if (DEBUG) {
      this.debug("Retry data call");
    }
    Services.tm.currentThread.dispatch(() => this.updateAllRILNetworkInterface(),
                                             Ci.nsIThread.DISPATCH_NORMAL);
  },

  updateAllRILNetworkInterface: function() {
    if (DEBUG) {
      this.debug("updateAllRILNetworkInterface");
    }
    for (let [networkType, networkInterface] of this.dataNetworkInterfaces) {
      if (networkInterface.enabled) {
        this.onUpdateRILNetworkInterface(networkInterface);
      }
    }
  },

  // nsIMobileConnectionListener

  notifyVoiceChanged: function() {},

  notifyDataChanged: function () {
    if (DEBUG) {
      this.debug("notifyDataChanged");
    }
    let connection = gMobileConnectionService.getItemByServiceId(this.clientId);
    let newDataInfo = connection.data;

    if (this._dataInfo.state == newDataInfo.state &&
        this._dataInfo.type == newDataInfo.type &&
        this._dataInfo.roaming == newDataInfo.roaming) {
      return;
    }

    this._dataInfo.state = newDataInfo.state;
    this._dataInfo.type = newDataInfo.type;
    this._dataInfo.roaming = newDataInfo.roaming;
    this.handleDataRegistrationChange();
  },

  notifyDataError: function (aMessage) {},

  notifyCFStateChanged: function(aAction, aReason, aNumber, aTimeSeconds, aServiceClass) {},

  notifyEmergencyCbModeChanged: function(aActive, aTimeoutMs) {},

  notifyOtaStatusChanged: function(aStatus) {},

  notifyRadioStateChanged: function() {
    let connection = gMobileConnectionService.getItemByServiceId(this.clientId);
    let radioOn = (connection.radioState === Ci.nsIMobileConnection.MOBILE_RADIO_STATE_ENABLED);

    if (radioOn) {
      if (this.needRecoverAfterReset) {
        return new Promise(function(aResolve, aReject) {
          let callback = {
            QueryInterface: XPCOMUtils.generateQI([Ci.nsIDataCallCallback]),
            notifySuccess: function() {
              aResolve();
            },
            notifyError: function(aErrorMsg) {
              aReject(aErrorMsg);
            }
          };
          this.debug("modem reset, recover the PS service.");
          if (this.dataCallSettings.defaultDataSlot) {
            this.dataCallInterface.setDataRegistration(true, callback);
          } else {
            this.dataCallInterface.setDataRegistration(false, callback);
          }
        }.bind(this));
      }
      // Reset this value.
      this.needRecoverAfterReset = false;
    }
  },

  notifyClirModeChanged: function(aMode) {},

  notifyLastKnownNetworkChanged: function() {},

  notifyLastKnownHomeNetworkChanged: function() {},

  notifyNetworkSelectionModeChanged: function() {},

  notifyDeviceIdentitiesChanged: function() {},

  notifySignalStrengthChanged: function() {},

  notifyModemRestart: function(reason) {
    if (DEBUG) {
      this.debug("modem reset, prepare to recover the PS service.");
    }
    this.needRecoverAfterReset = true;
  }
};

function DataCall(aClientId, aApnSetting, aDataCallHandler) {
  this.clientId = aClientId;
  this.dataCallHandler = aDataCallHandler;
  this.apnProfile = {
    apn: aApnSetting.apn,
    user: aApnSetting.user,
    password: aApnSetting.password,
    authType: aApnSetting.authtype,
    protocol: aApnSetting.protocol,
    roaming_protocol: aApnSetting.roaming_protocol,
    bearer: aApnSetting.bearer
  };
  this.linkInfo = {
    cid: null,
    ifname: null,
    addresses: [],
    dnses: [],
    gateways: [],
    pcscf: [],
    mtu: null,
    tcpbuffersizes: null
  };
  this.apnSetting = aApnSetting;
  this.state = NETWORK_STATE_UNKNOWN;
  this.requestedNetworkIfaces = [];
}
DataCall.prototype = {
  /**
   * Standard values for the APN connection retry process
   * Retry funcion: time(secs) = A * numer_of_retries^2 + B
   */
  NETWORK_APNRETRY_FACTOR: 8,
  NETWORK_APNRETRY_ORIGIN: 3,
  NETWORK_APNRETRY_MAXRETRIES: 10,

  dataCallHandler: null,

  // Event timer for connection retries
  timer: null,

  // APN failed connections. Retry counter
  apnRetryCounter: 0,

  // Array to hold RILNetworkInterfaces that requested this DataCall.
  requestedNetworkIfaces: null,

  /**
   * @return "deactivate" if <ifname> changes or one of the aCurrentDataCall
   *         addresses is missing in updatedDataCall, or "identical" if no
   *         changes found, or "changed" otherwise.
   */
  _compareDataCallLink: function(aUpdatedDataCall, aCurrentDataCall) {
    // If network interface is changed, report as "deactivate".
    if (aUpdatedDataCall.ifname != aCurrentDataCall.ifname) {
      return "deactivate";
    }

    // If any existing address is missing, report as "deactivate".
    for (let i = 0; i < aCurrentDataCall.addresses.length; i++) {
      let address = aCurrentDataCall.addresses[i];
      if (aUpdatedDataCall.addresses.indexOf(address) < 0) {
        return "deactivate";
      }
    }

    if (aCurrentDataCall.addresses.length != aUpdatedDataCall.addresses.length) {
      // Since now all |aCurrentDataCall.addresses| are found in
      // |aUpdatedDataCall.addresses|, this means one or more new addresses are
      // reported.
      return "changed";
    }

    let fields = ["gateways", "dnses"];
    for (let i = 0; i < fields.length; i++) {
      // Compare <datacall>.<field>.
      let field = fields[i];
      let lhs = aUpdatedDataCall[field], rhs = aCurrentDataCall[field];
      if (lhs.length != rhs.length) {
        return "changed";
      }
      for (let i = 0; i < lhs.length; i++) {
        if (lhs[i] != rhs[i]) {
          return "changed";
        }
      }
    }

    if (aCurrentDataCall.mtu != aUpdatedDataCall.mtu) {
      return "changed";
    }

    return "identical";
  },

  _getGeckoDataCallState:function (aDataCall) {
    if (aDataCall.active == Ci.nsIDataCallInterface.DATACALL_STATE_ACTIVE_UP ||
        aDataCall.active == Ci.nsIDataCallInterface.DATACALL_STATE_ACTIVE_DOWN) {
      return NETWORK_STATE_CONNECTED;
    }

    return NETWORK_STATE_DISCONNECTED;
  },

  updateTcpBufferSizes: function(aRadioTech) {
    // Handle setup data call result case. Get the rat in case the rat is change before result come back.
    if (!aRadioTech) {
      let connection =
        gMobileConnectionService.getItemByServiceId(this.clientId);
      let dataInfo = connection && connection.data;
      if (dataInfo == null ||
          dataInfo.state != RIL.GECKO_MOBILE_CONNECTION_STATE_REGISTERED ||
          dataInfo.type == RIL.GECKO_MOBILE_CONNECTION_STATE_UNKNOWN) {
        return null;
      }

      let radioTechType = dataInfo.type;
      aRadioTech = RIL.GECKO_RADIO_TECH.indexOf(radioTechType);
    }

    let ratName = RIL.GECKO_RADIO_TECH[aRadioTech];
    this.debug("updateTcpBufferSizes ratName=" + ratName  + " ,aRadioTech=" + aRadioTech);

    // Consider the evdo0, evdoa and evdob to evdo.
    if (ratName == "evdo0" || ratName == "evdoa" || ratName == "evdob") {
      ratName = "evdo";
    }
    // Consider the is95a and is95b to 1xrtt.
    if (ratName == "is95a" || ratName == "is95b") {
      ratName = "1xrtt";
    }

    // Try to get the custommization value for each rat.
    let prefName = "net.tcp.buffersize.mobile." + ratName;
    let sizes = null;
    try {
      sizes = Services.prefs.getStringPref(prefName, null);
    } catch (e) {
      sizes = null;
    }
    // Try to get the default value for each rat.
    if (sizes == null) {
      try {
        sizes = TCP_BUFFER_SIZES[aRadioTech];
      } catch (e) {
        sizes = null;
      }
    }
    this.debug("updateTcpBufferSizes ratName=" + ratName + " , sizes=" + sizes);
    return sizes;
  },

  onSetupDataCallResult: function(aDataCall) {
    this.debug("onSetupDataCallResult: " + JSON.stringify(aDataCall));
    let errorMsg = aDataCall.errorMsg;
    if (aDataCall.failCause &&
        aDataCall.failCause != Ci.nsIDataCallInterface.DATACALL_FAIL_NONE) {
      errorMsg =
        RIL.RIL_DATACALL_FAILCAUSE_TO_GECKO_DATACALL_ERROR[aDataCall.failCause];
    }

    if (errorMsg) {
      if (DEBUG) {
        this.debug("SetupDataCall error for apn " + this.apnProfile.apn + ": " +
                   errorMsg + " (" + aDataCall.failCause + "), retry time: " +
                   aDataCall.suggestedRetryTime);
      }

      this.state = NETWORK_STATE_DISCONNECTED;

      if (this.requestedNetworkIfaces.length === 0) {
        if (DEBUG) this.debug("This DataCall is not requested anymore.");
        Services.tm.currentThread.dispatch(() => this.deactivate(),
                                           Ci.nsIThread.DISPATCH_NORMAL);
        return;
      }

      // Let DataCallHandler notify MobileConnectionService
      this.dataCallHandler.notifyDataCallError(this, errorMsg);

      // For suggestedRetryTime, the value of INT32_MAX(0x7fffffff) means no retry.
      if (aDataCall.suggestedRetryTime === INT32_MAX ||
          this.isPermanentFail(aDataCall.failCause, errorMsg)) {
        if (DEBUG) this.debug("Data call error: no retry needed.");
        this.notifyInterfacesWithReason(RIL.DATACALL_PERMANENT_FAILURE);
        return;
      }

      this.retry(aDataCall.suggestedRetryTime);
      return;
    }

    this.apnRetryCounter = 0;
    this.linkInfo.cid = aDataCall.cid;

    if (this.requestedNetworkIfaces.length === 0) {
      if (DEBUG) {
        this.debug("State is connected, but no network interface requested" +
                   " this DataCall");
      }
      this.deactivate();
      return;
    }

    this.linkInfo.ifname = aDataCall.ifname;
    this.linkInfo.addresses = aDataCall.addresses ? aDataCall.addresses.split(" ") : [];
    this.linkInfo.gateways = aDataCall.gateways ? aDataCall.gateways.split(" ") : [];
    this.linkInfo.dnses = aDataCall.dnses ? aDataCall.dnses.split(" ") : [];
    this.linkInfo.pcscf = aDataCall.pcscf ? aDataCall.pcscf.split(" ") : [];
    this.linkInfo.mtu = aDataCall.mtu > 0 ? aDataCall.mtu : 0;
    this.state = this._getGeckoDataCallState(aDataCall);
    this.linkInfo.tcpbuffersizes = this.updateTcpBufferSizes();

    // Notify DataCallHandler about data call connected.
    this.dataCallHandler.notifyDataCallChanged(this);

    for (let i = 0; i < this.requestedNetworkIfaces.length; i++) {
      this.requestedNetworkIfaces[i].notifyRILNetworkInterface();
    }
  },

  onDeactivateDataCallResult: function() {
    if (DEBUG) this.debug("onDeactivateDataCallResult");

    this.reset();

    if (this.requestedNetworkIfaces.length > 0) {
      if (DEBUG) {
        this.debug("State is disconnected/unknown, but this DataCall is" +
                   " requested.");
      }
      this.setup();
      return;
    }

    // Notify DataCallHandler about data call disconnected.
    this.dataCallHandler.notifyDataCallChanged(this);
  },

  onDataCallChanged: function(aUpdatedDataCall) {
    if (DEBUG) {
      this.debug("onDataCallChanged: " + JSON.stringify(aUpdatedDataCall));
    }

    if (this.state == NETWORK_STATE_CONNECTING ||
        this.state == NETWORK_STATE_DISCONNECTING) {
      if (DEBUG) {
        this.debug("We are in " + convertToDataCallState(this.state) + ", ignore any " +
                   "unsolicited event for now.");
      }
      return;
    }

    let dataCallState = this._getGeckoDataCallState(aUpdatedDataCall);
    if (this.state == dataCallState &&
        dataCallState != NETWORK_STATE_CONNECTED) {
      return;
    }

    let newLinkInfo = {
      ifname: aUpdatedDataCall.ifname,
      addresses: aUpdatedDataCall.addresses ? aUpdatedDataCall.addresses.split(" ") : [],
      dnses: aUpdatedDataCall.dnses ? aUpdatedDataCall.dnses.split(" ") : [],
      gateways: aUpdatedDataCall.gateways ? aUpdatedDataCall.gateways.split(" ") : [],
      pcscf: aUpdatedDataCall.pcscf ? aUpdatedDataCall.pcscf.split(" ") : [],
      mtu: aUpdatedDataCall.mtu > 0 ? aUpdatedDataCall.mtu : 0
    };

    switch (dataCallState) {
      case NETWORK_STATE_CONNECTED:
        if (this.state == NETWORK_STATE_CONNECTED) {
          let result =
            this._compareDataCallLink(newLinkInfo, this.linkInfo);

          if (result == "identical") {
            if (DEBUG) this.debug("No changes in data call.");
            return;
          }
          if (result == "deactivate") {
            if (DEBUG) this.debug("Data link changed, cleanup.");
            this.deactivate();
            return;
          }
          // Minor change, just update and notify.
          if (DEBUG) {
            this.debug("Data link minor change, just update and notify.");
          }

          this.linkInfo.addresses = newLinkInfo.addresses.slice();
          this.linkInfo.gateways = newLinkInfo.gateways.slice();
          this.linkInfo.dnses = newLinkInfo.dnses.slice();
          this.linkInfo.pcscf = newLinkInfo.pcscf.slice();
          this.linkInfo.mtu = newLinkInfo.mtu;
        }
        break;
      case NETWORK_STATE_DISCONNECTED:
      case NETWORK_STATE_UNKNOWN:
        if (this.state == NETWORK_STATE_CONNECTED) {
          // Notify first on unexpected data call disconnection.
          this.state = dataCallState;
          for (let i = 0; i < this.requestedNetworkIfaces.length; i++) {
            this.requestedNetworkIfaces[i].notifyRILNetworkInterface();
          }
        }
        this.reset();

        // Handle network drop call case.
        if (this.requestedNetworkIfaces.length > 0) {
          if (DEBUG) {
            this.debug("State is disconnected/unknown, but this DataCall is" +
                       " requested.");
          }

          // Process retry to establish the data call.
          let dataInfo = this.dataCallHandler._dataInfo;
          if (!(dataInfo) ||
              dataInfo.state != RIL.GECKO_MOBILE_CONNECTION_STATE_REGISTERED ||
              dataInfo.type == RIL.GECKO_MOBILE_CONNECTION_STATE_UNKNOWN) {
            if (DEBUG){
              this.debug("dataCallStateChanged: Network state not ready. Abort.");
            }
            return;
          }
          let radioTechnology = RIL.GECKO_RADIO_TECH.indexOf(dataInfo.type);

          let targetBearer;
          if(this.apnSetting.bearer === undefined){
            targetBearer = 0;
          }else{
            targetBearer = this.apnSetting.bearer;
          }

          if (DEBUG){
            this.debug("dataCallStateChanged: radioTechnology:"+ radioTechnology +" ,targetBearer: " + bitmaskToString(targetBearer));
          }

          if(bitmaskHasTech(targetBearer , radioTechnology)){
            // Current DC can support this rat, process the retry datacall.
            // Do it in the next event tick, so that DISCONNECTED event can have
            // time to propagate before state becomes CONNECTING.
            Services.tm.currentThread.dispatch(() => this.retry(),
                                             Ci.nsIThread.DISPATCH_NORMAL);
          }else{
            if (DEBUG){
              this.debug("dataCallStateChanged: current APN do not support this rat reset DC. APN:" + JSON.stringify(apnSetting));
            }
            // Clean the requestedNetworkIfaces due to current DC can not support this rat.
            let targetRequestedNetworkIfaces = requestedNetworkIfaces.slice();
            for (let networkInterface of targetRequestedNetworkIfaces){
              this.disconnect(networkInterface);
            }
            // Retry datacall if any.
            if (DEBUG){
              this.debug("Retry data call");
            }
            Services.tm.currentThread.dispatch(() => this.dataCallHandler.updateAllRILNetworkInterface(),
                                                        Ci.nsIThread.DISPATCH_NORMAL);
          }
          return;
        }
        break;
    }

    this.state = dataCallState;

    // Notify DataCallHandler about data call changed.
    this.dataCallHandler.notifyDataCallChanged(this);

    for (let i = 0; i < this.requestedNetworkIfaces.length; i++) {
      this.requestedNetworkIfaces[i].notifyRILNetworkInterface();
    }
  },

  dataRegistrationChanged: function(aRadioTech) {
    // 1. Update the tcp buffer size for connected datacall.
    if (this.requestedNetworkIfaces.length > 0  &&
        this.state == RIL.GECKO_NETWORK_STATE_CONNECTED) {
      this.linkInfo.tcpbuffersizes = this.updateTcpBufferSizes(aRadioTech);
      for (let i = 0; i < this.requestedNetworkIfaces.length; i++) {
        this.requestedNetworkIfaces[i].notifyRILNetworkInterface();
      }
    }
    // 2. Only handle retrying state(disconnected + requestedNetworkIfaces) for rat change need to clean the DC if the rat not support.
    //    For the Connected/Connecting connection will let modem decide if this connection can work in current rat.
    //    Let the retry handle by handler.
    if (this.requestedNetworkIfaces.length === 0 ||
        ((this.state != RIL.GECKO_NETWORK_STATE_DISCONNECTED)
          && (this.state != RIL.GECKO_NETWORK_STATE_UNKNOWN))) {
      return;
    }

    let targetBearer;
    if(this.apnSetting.bearer === undefined){
      targetBearer = 0;
    }else{
      targetBearer = this.apnSetting.bearer;
    }
    if (DEBUG){
      this.debug("dataRegistrationChanged: targetBearer: " + bitmaskToString(targetBearer));
    }

    if(bitmaskHasTech(targetBearer , aRadioTech)){
      // Ignore same rat type. Let handler process the retry.
    }else{
      if (DEBUG){
        this.debug("dataRegistrationChanged: current APN do not support this rat reset DC. APN:" + JSON.stringify(apnSetting));
      }
      // Clean the requestedNetworkIfaces due to current DC can not support this rat under DC retrying state.
      // Let handler process the retry.
      let targetRequestedNetworkIfaces = requestedNetworkIfaces.slice();
      for (let networkInterface of targetRequestedNetworkIfaces){
        this.disconnect(networkInterface);
      }
    }
  },

  // Helpers

  debug: function(aMsg) {
    dump("-*- DataCall[" + this.clientId + ":" + this.apnProfile.apn + "]: " +
      aMsg + "\n");
  },

  get connected() {
    return this.state == NETWORK_STATE_CONNECTED;
  },

  isPermanentFail: function(aDataFailCause, aErrorMsg) {
    // Check ril.h for 'no retry' data call fail causes.
    if (aErrorMsg === RIL.GECKO_ERROR_RADIO_NOT_AVAILABLE ||
        aErrorMsg === RIL.GECKO_ERROR_INVALID_PARAMETER ||
        aDataFailCause === Ci.nsIDataCallInterface.DATACALL_FAIL_OPERATOR_BARRED ||
        aDataFailCause === Ci.nsIDataCallInterface.DATACALL_FAIL_MISSING_UKNOWN_APN ||
        aDataFailCause === Ci.nsIDataCallInterface.DATACALL_FAIL_UNKNOWN_PDP_ADDRESS_TYPE ||
        aDataFailCause === Ci.nsIDataCallInterface.DATACALL_FAIL_USER_AUTHENTICATION ||
        aDataFailCause === Ci.nsIDataCallInterface.DATACALL_FAIL_ACTIVATION_REJECT_GGSN ||
        aDataFailCause === Ci.nsIDataCallInterface.DATACALL_FAIL_SERVICE_OPTION_NOT_SUPPORTED ||
        aDataFailCause === Ci.nsIDataCallInterface.DATACALL_FAIL_SERVICE_OPTION_NOT_SUBSCRIBED ||
        aDataFailCause === Ci.nsIDataCallInterface.DATACALL_FAIL_NSAPI_IN_USE ||
        aDataFailCause === Ci.nsIDataCallInterface.DATACALL_FAIL_ONLY_IPV4_ALLOWED ||
        aDataFailCause === Ci.nsIDataCallInterface.DATACALL_FAIL_ONLY_IPV6_ALLOWED ||
        aDataFailCause === Ci.nsIDataCallInterface.DATACALL_FAIL_PROTOCOL_ERRORS ||
        aDataFailCause === Ci.nsIDataCallInterface.DATACALL_FAIL_RADIO_POWER_OFF ||
        aDataFailCause === Ci.nsIDataCallInterface.DATACALL_FAIL_TETHERED_CALL_ACTIVE) {
      return true;
    }

    return false;
  },

  inRequestedTypes: function(aType) {
    for (let i = 0; i < this.requestedNetworkIfaces.length; i++) {
      if (this.requestedNetworkIfaces[i].info.type == aType) {
        return true;
      }
    }
    return false;
  },

  canHandleApn: function(aApnSetting) {
    let isIdentical = this.apnProfile.apn == aApnSetting.apn &&
                      (this.apnProfile.user || '') == (aApnSetting.user || '') &&
                      (this.apnProfile.password || '') == (aApnSetting.password || '') &&
                      (this.apnProfile.authType || '') == (aApnSetting.authtype || '');

    isIdentical = isIdentical &&
                  (this.apnProfile.protocol || '') == (aApnSetting.protocol || '') &&
                  (this.apnProfile.roaming_protocol || '') == (aApnSetting.roaming_protocol || '');

    return isIdentical;
  },

  resetLinkInfo: function() {
    this.linkInfo.cid = null;
    this.linkInfo.ifname = null;
    this.linkInfo.addresses = [];
    this.linkInfo.dnses = [];
    this.linkInfo.gateways = [];
    this.linkInfo.pcscf = [];
    this.linkInfo.mtu = null;
    this.linkInfo.tcpbuffersizes = null;
  },

  reset: function() {
    this.debug("reset");
    this.resetLinkInfo();

    // Reset the retry counter.
    this.apnRetryCounter = 0;
    this.state = NETWORK_STATE_DISCONNECTED;
  },

  connect: function(aNetworkInterface) {
    if (DEBUG) this.debug("connect: " + convertToDataCallType(aNetworkInterface.info.type));

    if (this.requestedNetworkIfaces.indexOf(aNetworkInterface) == -1) {
      this.requestedNetworkIfaces.push(aNetworkInterface);
    }

    if (this.state == NETWORK_STATE_CONNECTING ||
        this.state == NETWORK_STATE_DISCONNECTING) {
      return;
    }
    if (this.state == NETWORK_STATE_CONNECTED) {
      // This needs to run asynchronously, to behave the same way as the case of
      // non-shared apn, see bug 1059110.
      Services.tm.currentThread.dispatch(() => {
        // Do not notify if state changed while this event was being dispatched,
        // the state probably was notified already or need not to be notified.
        if (aNetworkInterface.info.state == RIL.GECKO_NETWORK_STATE_CONNECTED) {
          aNetworkInterface.notifyRILNetworkInterface();
        }
      }, Ci.nsIEventTarget.DISPATCH_NORMAL);
      return;
    }

    // If retry mechanism is running on background, stop it since we are going
    // to setup data call now.
    if (this.timer) {
      this.timer.cancel();
    }

    this.setup();
  },

  setup: function() {
    if (DEBUG) {
      this.debug("Going to set up data connection with APN " +
                 this.apnProfile.apn);
    }

    let connection =
      gMobileConnectionService.getItemByServiceId(this.clientId);
    let dataInfo = connection && connection.data;
    if (dataInfo == null ||
        dataInfo.state != RIL.GECKO_MOBILE_CONNECTION_STATE_REGISTERED ||
        dataInfo.type == RIL.GECKO_MOBILE_CONNECTION_STATE_UNKNOWN) {
      return;
    }

    let radioTechType = dataInfo.type;
    let radioTechnology = RIL.GECKO_RADIO_TECH.indexOf(radioTechType);
    let authType = RIL.RIL_DATACALL_AUTH_TO_GECKO.indexOf(this.apnProfile.authType);
    // Use the default authType if the value in database is invalid.
    // For the case that user might not select the authentication type.
    if (authType == -1) {
      if (DEBUG) {
        this.debug("Invalid authType '" + this.apnProfile.authtype +
                   "', using '" + RIL.GECKO_DATACALL_AUTH_DEFAULT + "'");
      }
      authType = RIL.RIL_DATACALL_AUTH_TO_GECKO.indexOf(RIL.GECKO_DATACALL_AUTH_DEFAULT);
    }

    let pdpType = Ci.nsIDataCallInterface.DATACALL_PDP_TYPE_IPV4;
    pdpType = !dataInfo.roaming
            ? RIL.RIL_DATACALL_PDP_TYPES.indexOf(this.apnProfile.protocol)
            : RIL.RIL_DATACALL_PDP_TYPES.indexOf(this.apnProfile.roaming_protocol);
    if (pdpType == -1) {
      if (DEBUG) {
        this.debug("Invalid pdpType '" + (!dataInfo.roaming
                   ? this.apnProfile.protocol
                   : this.apnProfile.roaming_protocol) +
                   "', using '" + RIL.GECKO_DATACALL_PDP_TYPE_DEFAULT + "'");
      }
      pdpType = RIL.RIL_DATACALL_PDP_TYPES.indexOf(RIL.GECKO_DATACALL_PDP_TYPE_DEFAULT);
    }

    let dcInterface = this.dataCallHandler.dataCallInterface;
    dcInterface.setupDataCall(
      this.apnProfile.apn, this.apnProfile.user, this.apnProfile.password,
      authType, pdpType, {
        QueryInterface: XPCOMUtils.generateQI([Ci.nsIDataCallCallback]),
        notifySetupDataCallSuccess: (aDataCall) => {
          this.onSetupDataCallResult(aDataCall);
        },
        notifyError: (aErrorMsg) => {
          this.onSetupDataCallResult({errorMsg: aErrorMsg});
        }
      });
    this.state = NETWORK_STATE_CONNECTING;
  },

  retry: function(aSuggestedRetryTime) {
    let apnRetryTimer;

    // We will retry the connection in increasing times
    // based on the function: time = A * numer_of_retries^2 + B
    if (this.apnRetryCounter >= this.NETWORK_APNRETRY_MAXRETRIES) {
      this.apnRetryCounter = 0;
      this.timer = null;
      if (DEBUG) this.debug("Too many APN Connection retries - STOP retrying");
      this.notifyInterfacesWithReason(RIL.DATACALL_RETRY_FAILED);
      return;
    }

    // If there is a valid aSuggestedRetryTime, override the retry timer.
    if (aSuggestedRetryTime !== undefined && aSuggestedRetryTime >= 0) {
      apnRetryTimer = aSuggestedRetryTime / 1000;
    } else {
      apnRetryTimer = this.NETWORK_APNRETRY_FACTOR *
                      (this.apnRetryCounter * this.apnRetryCounter) +
                      this.NETWORK_APNRETRY_ORIGIN;
    }
    this.apnRetryCounter++;
    if (DEBUG) {
      this.debug("Data call - APN Connection Retry Timer (secs-counter): " +
                 apnRetryTimer + "-" + this.apnRetryCounter);
    }

    if (this.timer == null) {
      // Event timer for connection retries
      this.timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    }
    this.timer.initWithCallback(this, apnRetryTimer * 1000,
                                Ci.nsITimer.TYPE_ONE_SHOT);
  },

  disconnect: function(aNetworkInterface) {
    if (DEBUG) this.debug("disconnect: " + convertToDataCallType(aNetworkInterface.info.type));

    let index = this.requestedNetworkIfaces.indexOf(aNetworkInterface);
    if (index != -1) {
      this.requestedNetworkIfaces.splice(index, 1);

      if (this.state == NETWORK_STATE_DISCONNECTED ||
          this.state == NETWORK_STATE_UNKNOWN) {
        if (this.timer) {
          this.timer.cancel();
        }
        this.reset();
        return;
      }

      // Notify the DISCONNECTED event immediately after network interface is
      // removed from requestedNetworkIfaces, to make the DataCall, shared or
      // not, to have the same behavior.
      Services.tm.currentThread.dispatch(() => {
        // Do not notify if state changed while this event was being dispatched,
        // the state probably was notified already or need not to be notified.
        if (aNetworkInterface.info.state == RIL.GECKO_NETWORK_STATE_DISCONNECTED) {
          aNetworkInterface.notifyRILNetworkInterface();

          // Clear link info after notifying NetworkManager.
          if (this.requestedNetworkIfaces.length === 0) {
            this.resetLinkInfo();
          }
        }
      }, Ci.nsIEventTarget.DISPATCH_NORMAL);
    }

    // Only deactivate data call if no more network interface needs this
    // DataCall and if state is CONNECTED, for other states, we simply remove
    // the network interface from requestedNetworkIfaces.
    if (this.requestedNetworkIfaces.length > 0 ||
        this.state != NETWORK_STATE_CONNECTED) {
      return;
    }

    this.deactivate();
  },

  deactivate: function() {
    let reason = Ci.nsIDataCallInterface.DATACALL_DEACTIVATE_NO_REASON;
    if (DEBUG) {
      this.debug("Going to disconnect data connection cid " + this.linkInfo.cid);
    }

    let dcInterface = this.dataCallHandler.dataCallInterface;
    dcInterface.deactivateDataCall(this.linkInfo.cid, reason, {
      QueryInterface: XPCOMUtils.generateQI([Ci.nsIDataCallCallback]),
      notifySuccess: () => {
        this.onDeactivateDataCallResult();
      },
      notifyError: (aErrorMsg) => {
        this.onDeactivateDataCallResult();
      }
    });

    this.state = NETWORK_STATE_DISCONNECTING;
  },

  // Entry method for timer events. Used to reconnect to a failed APN
  notify: function(aTimer) {
    this.debug("Received the retry notify.");
    this.setup();
  },

  shutdown: function() {
    if (this.timer) {
      this.timer.cancel();
      this.timer = null;
    }
  },

  notifyInterfacesWithReason: function(aReason) {
    for (let i = 0; i < this.requestedNetworkIfaces.length; i++) {
      let networkInterface = this.requestedNetworkIfaces[i];
      networkInterface.info.setReason(aReason);
      networkInterface.notifyRILNetworkInterface();
    }
  },
};

function RILNetworkInfo(aClientId, aType, aNetworkInterface)
{
  this.serviceId = aClientId;
  this.type = aType;
  this.reason = Ci.nsINetworkInfo.REASON_NONE;

  this.networkInterface = aNetworkInterface;
}
RILNetworkInfo.prototype = {
  classID:   RILNETWORKINFO_CID,
  classInfo: XPCOMUtils.generateCI({classID: RILNETWORKINFO_CID,
                                    classDescription: "RILNetworkInfo",
                                    interfaces: [Ci.nsINetworkInfo,
                                                 Ci.nsIRilNetworkInfo]}),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsINetworkInfo,
                                         Ci.nsIRilNetworkInfo]),

  networkInterface: null,

  getDataCall: function() {
    let dataCallsList = this.networkInterface.dataCallsList;
    for(let i = 0; i < dataCallsList.length; i++){
      if(dataCallsList[i].inRequestedTypes(this.type)){
        return dataCallsList[i];
      }
    }
    return null;
  },

  getApnSetting: function() {
    let dataCall = this.getDataCall();
    if(dataCall){
      return dataCall.apnSetting;
    }
    return null;
  },

  debug: function(aMsg) {
    dump("-*- RILNetworkInfo[" + this.serviceId + ":" + this.type + "]: " +
         aMsg + "\n");
  },

  /**
   * nsINetworkInfo Implementation
   */
  get state() {
    let dataCall = this.getDataCall();
    if(dataCall){
      return dataCall.state;
    }
    return NETWORK_STATE_DISCONNECTED;
  },

  type: null,

  get name() {
    let dataCall = this.getDataCall();
    if((dataCall) && (dataCall.state == NETWORK_STATE_CONNECTED)){
      return dataCall.linkInfo.ifname;
    }
    return "";
  },

  get tcpbuffersizes() {
    let dataCall = this.getDataCall();
    if((dataCall) && (dataCall.state == NETWORK_STATE_CONNECTED)){
      return dataCall.linkInfo.tcpbuffersizes;
    }
    return "";
  },

  getAddresses: function(aIps, aPrefixLengths) {
    let dataCall = this.getDataCall();
    let addresses = "";
    if((dataCall) && (dataCall.state == NETWORK_STATE_CONNECTED)){
      addresses = dataCall.linkInfo.addresses;
    }

    let ips = [];
    let prefixLengths = [];
    for (let i = 0; i < addresses.length; i++) {
      let [ip, prefixLength] = addresses[i].split("/");
      ips.push(ip);
      prefixLengths.push(prefixLength);
    }

    aIps.value = ips.slice();
    aPrefixLengths.value = prefixLengths.slice();

    return ips.length;
  },

  getGateways: function(aCount) {
    let dataCall = this.getDataCall();
    let linkInfo = []; //default value
    if((dataCall) && (dataCall.state == NETWORK_STATE_CONNECTED)){
      linkInfo = dataCall.linkInfo;
    }

    if (aCount && linkInfo && linkInfo.gateways) {
      aCount.value = linkInfo.gateways.length;
    }

    if (linkInfo && linkInfo.gateways) {
      return linkInfo.gateways.slice();
    } else {
      return linkInfo;
    }
  },

  getDnses: function(aCount) {
    let dataCall = this.getDataCall();
    let linkInfo = []; //default value
    if((dataCall) && (dataCall.state == NETWORK_STATE_CONNECTED)){
      linkInfo = dataCall.linkInfo;
    }

    if (aCount && linkInfo && linkInfo.dnses) {
      aCount.value = linkInfo.dnses.length;
    }
    if (linkInfo && linkInfo.dnses) {
      return linkInfo.dnses.slice();
    } else {
      return linkInfo;
    }
  },

  /**
   * nsIRilNetworkInfo Implementation
   */

  serviceId: 0,

  get iccId() {
    let icc = gIccService.getIccByServiceId(this.serviceId);
    let iccInfo = icc && icc.iccInfo;

    return iccInfo && iccInfo.iccid;
  },

  get mmsc() {
    if (this.type != NETWORK_TYPE_MOBILE_MMS) {
      if (DEBUG) this.debug("Error! Only MMS network can get MMSC.");
      throw Cr.NS_ERROR_UNEXPECTED;
    }
    let apnSetting = this.getApnSetting();
    if(apnSetting){
      return apnSetting.mmsc || "";
    }
    return "";
  },

  get mmsProxy() {
    if (this.type != NETWORK_TYPE_MOBILE_MMS) {
      if (DEBUG) this.debug("Error! Only MMS network can get MMS proxy.");
      throw Cr.NS_ERROR_UNEXPECTED;
    }
    let apnSetting = this.getApnSetting();
    if(apnSetting){
      return apnSetting.mmsproxy || "";
    }
    return "";
  },

  get mmsPort() {
    if (this.type != NETWORK_TYPE_MOBILE_MMS) {
      if (DEBUG) this.debug("Error! Only MMS network can get MMS port.");
      throw Cr.NS_ERROR_UNEXPECTED;
    }

    // Note: Port 0 is reserved, so we treat it as invalid as well.
    // See http://www.iana.org/assignments/port-numbers
    let apnSetting = this.getApnSetting();
    if(apnSetting){
      return apnSetting.mmsport || "-1";
    }
    return "-1";
  },

  reason: null,

  getPcscf: function(aCount) {
    if (this.type != NETWORK_TYPE_MOBILE_IMS) {
      if (DEBUG) this.debug("Error! Only IMS network can get pcscf.");
      throw Cr.NS_ERROR_UNEXPECTED;
    }
    let dataCall = this.getDataCall();
    let linkInfo = []; //default value
    if( dataCall && (dataCall.state == NETWORK_STATE_CONNECTED)){
      linkInfo = dataCall.linkInfo;
    }

    if (aCount && linkInfo && linkInfo.pcscf) {
      aCount.value = linkInfo.pcscf.length;
    }
    if (linkInfo && linkInfo.pcscf) {
      return linkInfo.pcscf.slice();
    } else {
      return linkInfo;
    }
  },

  setReason: function(aReason) {
    this.reason = aReason;
  },
};

function RILNetworkInterface(aDataCallHandler, aType, aApnSetting, aDataCall) {
  if (!aDataCall) {
    throw new Error("No dataCall for RILNetworkInterface: " + type);
  }

  this.dataCallHandler = aDataCallHandler;
  this.enabled = false;
  this.apnSetting = aApnSetting;
  // Store datacall which can be use by this apn type.
  if(aDataCall instanceof Array){
    this.dataCallsList = aDataCall.slice();
  }else if(this.dataCallsList.indexOf(aDataCall) == -1){
    this.dataCallsList.push(aDataCall);
  }

  this.info = new RILNetworkInfo(aDataCallHandler.clientId, aType, this);
}

RILNetworkInterface.prototype = {
  classID:   RILNETWORKINTERFACE_CID,
  classInfo: XPCOMUtils.generateCI({classID: RILNETWORKINTERFACE_CID,
                                    classDescription: "RILNetworkInterface",
                                    interfaces: [Ci.nsINetworkInterface]}),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsINetworkInterface]),

  // If this RILNetworkInterface type is enabled or not.
  enabled: null,

  // It can have muti DC. But only has one DC in none disconnected state at same time.
  dataCallsList:[],

  /**
   * nsINetworkInterface Implementation
   */

  info: null,

  // For record how many APP using this apn type.
  activeUsers: 0,

  get httpProxyHost() {
    if(this.dataCallsList){
      for(let i = 0; i < this.dataCallsList.length; i++){
        if(this.dataCallsList[i].inRequestedTypes(this.type)){
          return this.dataCallsList[i].apnSetting.proxy || "";
        }
      }
    }
    return "";
  },

  get httpProxyPort() {
    if(this.dataCallsList){
      for(let i = 0; i < this.dataCallsList.length; i++){
        if(this.dataCallsList[i].inRequestedTypes(this.type)){
          return this.dataCallsList[i].apnSetting.port || "";
        }
      }
    }

    return "";
  },

  get mtu() {
    // Value provided by network has higher priority than apn settings.
    let apnSettingMtu = -1;
    let linkInfoMtu = -1;

    if(this.dataCallsList){
      for(let i = 0; i < this.dataCallsList.length; i++){
        if(this.dataCallsList[i].inRequestedTypes(this.type)){
          linkInfoMtu = this.dataCallsList[i].linkInfo.mtu;
          apnSettingMtu = this.dataCallsList[i].apnSetting.mtu;
        }
      }
    }

    return linkInfoMtu || apnSettingMtu || -1;
  },

  // Helpers

  debug: function(aMsg) {
    dump("-*- RILNetworkInterface[" + this.dataCallHandler.clientId + ":" +
         convertToDataCallType(this.info.type)  + "]: " + aMsg + "\n");
  },

  get connected() {
    return this.info.state == NETWORK_STATE_CONNECTED;
  },

  notifyRILNetworkInterface: function() {
    if (DEBUG) {
      this.debug("notifyRILNetworkInterface type: " + convertToDataCallType(this.info.type) +
                 ", state: " + convertToDataCallState(this.info.state) + ", reason: " + this.info.reason);
    }

    gNetworkManager.updateNetworkInterface(this);
  },

  enable: function() {
    if (this.type != NETWORK_TYPE_MOBILE) {
      this.activeUsers++;
    }
    this.enabled = true;
    this.info.reason = Ci.nsINetworkInfo.REASON_NONE;
  },

  connect: function() {
    let dataInfo = this.dataCallHandler._dataInfo;
    if (dataInfo == null ||
        dataInfo.state != RIL.GECKO_MOBILE_CONNECTION_STATE_REGISTERED ||
        dataInfo.type == RIL.GECKO_MOBILE_CONNECTION_STATE_UNKNOWN) {
      if (DEBUG){
        this.debug("connect: Network state not ready. Abort.");
      }
      return;
    }
    let radioTechnology = RIL.GECKO_RADIO_TECH.indexOf(dataInfo.type);
    if (DEBUG){
      this.debug("connect: radioTechnology: "+ radioTechnology);
    }

    let targetDataCall = null;
    let targetBearer = 0;
    if(this.dataCallsList){
      for(let i = 0; i < this.dataCallsList.length; i++){
        // Error handle for those apn setting do not config bearer value.
        if(this.dataCallsList[i].apnSetting.bearer === undefined){
          targetBearer = 0;
        }else{
          targetBearer = this.dataCallsList[i].apnSetting.bearer;
        }

        if (DEBUG){
          this.debug("connect: apn:" + this.dataCallsList[i].apnSetting.apn + " ,targetBearer: "+ bitmaskToString(targetBearer));
        }
        if(bitmaskHasTech(targetBearer , radioTechnology)){
          targetDataCall = this.dataCallsList[i];
          break;
        }
      }
    }

    if(targetDataCall != null){
      targetDataCall.connect(this);
    }else{
      this.debug("connect: There is no DC support this rat. Abort.");
    }
  },

  disable: function(aReason = Ci.nsINetworkInfo.REASON_NONE) {
    if (DEBUG) {
      this.debug("disable aReason: " + aReason);
    }
    if (!this.enabled) {
      return;
    }
    if (this.info.type == NETWORK_TYPE_MOBILE) {
      // Devcie should not enter here.
      this.enabled = false;
    } else {
      this.activeUsers--;
      this.enabled = (this.activeUsers > 0 ? true : false);
    }

    if (!this.enabled) {
      this.activeUsers = 0;
      this.enabled = false;
      this.disconnect(Ci.nsINetworkInfo.REASON_APN_DISABLED);
    }
  },

  disconnect: function(aReason = Ci.nsINetworkInfo.REASON_NONE) {
    if (DEBUG) {
      this.debug("disconnect aReason: " + aReason);
    }

    this.info.setReason(aReason);

    if(this.dataCallsList){
      for(let i = 0; i < this.dataCallsList.length; i++){
        if(this.dataCallsList[i].inRequestedTypes(this.info.type)){
          this.dataCallsList[i].disconnect(this);
        }
      }
    }
  },

  shutdown: function() {
    if(this.dataCallsList){
      for(let i = 0; i < this.dataCallsList.length; i++){
        this.dataCallsList[i].shutdown;
      }
    }
    this.dataCallsList = null;
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([DataCallManager]);
