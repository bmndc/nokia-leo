/* -*- indent-tabs-mode: nil; js-indent-level: 2 -*- */
/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyGetter(this, "RIL", function () {
  let obj = Cu.import("resource://gre/modules/ril_consts.js", null);
  return obj;
});

XPCOMUtils.defineLazyGetter(this, "SMSCB", function () {
  let obj = {};
  Cu.import("resource://gre/modules/sms_cb_consts.js", obj);
  return obj;
});

const GONK_CELLBROADCASTCONFIGSERVICE_CONTRACTID =
  "@kaios.com/cellbroadcast/gonkconfigservice;1";
const GONK_CELLBROADCASTCONFIGSERVICE_CID =
  Components.ID("{50e98d38-536a-4f13-99c2-7fb2de6bf2e0}");

const GONK_CELLBROADCASTCONFIGHANDLER_CID =
  Components.ID("{e4f7b5dc-53ba-4653-8b87-62065db2a274}");

const NS_XPCOM_SHUTDOWN_OBSERVER_ID = "xpcom-shutdown";
const NS_PREFBRANCH_PREFCHANGE_TOPIC_ID = "nsPref:changed";

XPCOMUtils.defineLazyGetter(this, "gRadioInterfaceLayer", function() {
  let ril = { numRadioInterfaces: 0 };
  try {
    ril = Cc["@mozilla.org/ril;1"].getService(Ci.nsIRadioInterfaceLayer);
  } catch(e) {}
  return ril;
});

XPCOMUtils.defineLazyServiceGetter(this, "gCellBroadcastService",
                                   "@mozilla.org/cellbroadcast/cellbroadcastservice;1",
                                   "nsICellBroadcastService");

XPCOMUtils.defineLazyServiceGetter(this, "gCustomizationInfo",
                                   "@kaiostech.com/customizationinfo;1",
                                   "nsICustomizationInfo");

XPCOMUtils.defineLazyServiceGetter(this, "gIccService",
                                   "@mozilla.org/icc/iccservice;1",
                                   "nsIIccService");

XPCOMUtils.defineLazyServiceGetter(this, "gSettingsService",
                                   "@mozilla.org/settingsService;1",
                                   "nsISettingsService");

XPCOMUtils.defineLazyServiceGetter(this, "gMobileConnectionService",
                                   "@mozilla.org/mobileconnection/mobileconnectionservice;1",
                                   "nsIMobileConnectionService");

var DEBUG = false;
function debug(s) {
  dump("GonkCellBroadcastConfigService: " + s);
}

function GonkCellBroadcastConfigService() {
  let numOfRilClients = gRadioInterfaceLayer.numRadioInterfaces;
  for (let clientId = 0; clientId < numOfRilClients; clientId++) {
    let handler = new GonkCellBroadcastConfigHandler(clientId);
    this._handlers.push(handler);
  }

  Services.obs.addObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false);
}
GonkCellBroadcastConfigService.prototype = {
  classID:   GONK_CELLBROADCASTCONFIGSERVICE_CID,

  classInfo: XPCOMUtils.generateCI({classID: GONK_CELLBROADCASTCONFIGSERVICE_CID,
                                    contactID: GONK_CELLBROADCASTCONFIGSERVICE_CONTRACTID,
                                    classDescription: "Cell Broadcast Cconfiguration Service",
                                    interfaces: [Ci.nsIGonkCellBroadcastConfigService],
                                    flags: Ci.nsIClassInfo.SINGLETON}),
  contactID: GONK_CELLBROADCASTCONFIGSERVICE_CONTRACTID,

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIGonkCellBroadcastConfigService,
                                         Ci.nsIObserver]),

  _handlers: [],

  _updateDebugFlag: function() {
    try {
      DEBUG = DEBUG || RIL.DEBUG_RIL ||
              Services.prefs.getBoolPref(kPrefRilDebuggingEnabled);
    } catch (e) {}
  },

  /**
   * nsIObserver interface.
   */
  observe: function(aSubject, aTopic, aData) {
    switch (aTopic) {
      case NS_XPCOM_SHUTDOWN_OBSERVER_ID:
        Services.obs.removeObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID);
        this._handlers.forEach((handler) => {
          handler.shutdown();
        });
        break;
    }
  },

  /**
   * nsIObserver interface.
   */
  getCBSearchList: function(aClientId, aGsmCount, aGsms, aCdmaCount, aCdmas) {
    let handler = this._handlers[aCLientId];
    if (!handler) {
      if (DEBUG) debug("getCBSearchList unexpected client: " + aClientId);
      throw Cr.NS_ERROR_NOT_AVAILABLE;
    }

    let config = handler.getConfig();
    aGsmCount.value = config.gsm.length;
    aGsms.value = config.gsm;
    aCdmaCount.value = config.cdma.length;
    aCdmas.value = config.cdma;
  },

  getCBDisabled: function(aClientId) {
    // Currently, we always enable CB for every slot.
    return false;
  }
};

function GonkCellBroadcastConfigHandler(aClientId) {
  if (DEBUG) debug("start");
  this._clientId = aClientId;
  this._icc = gIccService.getIccByServiceId(aClientId);
  this._icc.registerListener(this);

  this._conn = gMobileConnectionService.getItemByServiceId(aClientId);
  this._conn.registerListener(this);

  this._config = {cdma: [], gsm: []};

  // Manual mccmnc query, in case mccmnc get ready before constructor.
  this.notifyIccInfoChanged();
}
GonkCellBroadcastConfigHandler.prototype = {
  classID:   GONK_CELLBROADCASTCONFIGHANDLER_CID,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIIccListener,
                                         Ci.nsIMobileConnectionListener]),

  _clientId: 0,
  _icc: null,
  _mccmnc: null,
  _config: null,
  _conn: null,

  /**
   * nsIIccListener interface methods.
   */
  notifyStkCommand: function(aStkProactiveCmd) {},

  notifyStkSessionEnd: function(){},

  notifyCardStateChanged: function() {},

  notifyIccInfoChanged: function () {
    if (DEBUG) debug("notifyIccInfoChanged");
    if (!this._icc.iccInfo || !this._icc.iccInfo.mcc || !this._icc.iccInfo.mnc) {
      if (DEBUG) debug("iccinfo is not ready");
      return;
    }

    let mccmnc = this._icc.iccInfo.mcc + this._icc.iccInfo.mnc;
    if (this._mccmnc == mccmnc) {
      if (DEBUG) debug("same mcc, mnc");
      return;
    }

    this._mccmnc = mccmnc;

    if (!this._conn || this._conn.radioState != Ci.nsIMobileConnection.MOBILE_RADIO_STATE_ENABLED) {
      if (DEBUG) debug("radio is not enabled");
      return;
    }

    this._updateCellBroadcastConfig();
  },

  notifyIsimInfoChanged: function() {},

  /**
   * nsIMobileConnectionListener interface methods.
   */
  notifyVoiceChanged: function() {},
  notifyDataChanged: function() {},
  notifyDataError: function(message) {},
  notifyCFStateChanged: function(action, reason, number, timeSeconds, serviceClass) {},
  notifyEmergencyCbModeChanged: function(active, timeoutMs) {},
  notifyOtaStatusChanged: function(status) {},
  notifyRadioStateChanged: function() {
    if (DEBUG) debug("current radio state: " + this._conn.radioState);
    if (this._conn.radioState != Ci.nsIMobileConnection.MOBILE_RADIO_STATE_ENABLED) {
      if (DEBUG) debug("radio is not ready");
      return;
    }

    if (!this._mccmnc) {
      if (DEBUG) debug("mcc, mnc is not ready");
      return;
    }

    this._updateCellBroadcastConfig();
  },
  notifyClirModeChanged: function(mode) {},
  notifyLastKnownNetworkChanged: function() {},
  notifyLastKnownHomeNetworkChanged: function() {},
  notifyNetworkSelectionModeChanged: function() {},
  notifyDeviceIdentitiesChanged: function() {},
  notifySignalStrengthChanged: function() {},
  notifyModemRestart: function(reason) {},

  /**
   * GonkCellBroadcastConfigHandler methods
   */
  shutdown: function() {
    if(this._icc) {
      this._icc.unregisterListener(this);
    }

    if(this._conn) {
      this._conn.unregisterListener(this);
    }
  },

  getConfig: function() {
    return this._config;
  },

  _updateCellBroadcastConfig: function() {
    if (this._mccmnc === null) {
      if (DEBUG) debug("no mccmnc: " + this._clientId);
      return;
    }

    let radioInterface = gRadioInterfaceLayer.getRadioInterface(this._clientId);
    if (!radioInterface) {
      if (DEBUG) debug("no radio interface: " + this._clientId);
      return;
    }

    let config = this._getCellBroadcastConfig();
    if (this._isDifferentConfig(this._config, config)) {
      gCellBroadcastService.setCBSearchList(this._clientId, config.gsm.length,
                                    config.gsm, config.cdma.length, config.cdma);
      this._config = config;
    }
  },

  _isDifferentConfig: function(aOld, aNew) {
    if (aOld.gsm.length != aNew.gsm.length) {
      return true;
    }

    if (aOld.cdma.length != aNew.cdma.length) {
      return true;
    }

    if (aOld.gsm.toString() !== aNew.gsm.toString()) {
      return true;
    }

    if (aOld.cdma.toString() !== aNew.cdma.toString()) {
      return true;
    }
  },

  _getCellBroadcastConfig: function() {
    let enableEmergencyAlerts =
        gCustomizationInfo.getCustomizedValue(this._clientId, SMSCB.KEY_ENABLE_EMERGENCY_ALERTS, true);

    let enableEtwsAlerts = enableEmergencyAlerts;

    let enableCmasExtremeAlerts = enableEmergencyAlerts &&
        gCustomizationInfo.getCustomizedValue(this._clientId, SMSCB.KEY_ENABLE_CMAS_EXTREME_THREAT_ALERTS, true);

    let enableCmasSevereAlerts = enableEmergencyAlerts &&
        gCustomizationInfo.getCustomizedValue(this._clientId, SMSCB.KEY_ENABLE_CMAS_SEVERE_THREAT_ALERTS, true);

    let enableCmasAmberAlerts = enableEmergencyAlerts &&
        gCustomizationInfo.getCustomizedValue(this._clientId, SMSCB.KEY_ENABLE_CMAS_AMBER_ALERTS, true);

    let forceDisableEtwsCmasTest =
        gCustomizationInfo.getCustomizedValue(this._clientId, SMSCB.KEY_CARRIER_FORCE_DISABLE_ETWS_CMAS_TEST_BOOL, false);

    let enableEtwsTestAlerts = !forceDisableEtwsCmasTest && enableEmergencyAlerts &&
        gCustomizationInfo.getCustomizedValue(this._clientId, SMSCB.KEY_ENABLE_ETWS_TEST_ALERTS, false);

    /* BDC: kanxj 20200605 modify the default CMAS test config to true */
    let enableCmasTestAlerts = !forceDisableEtwsCmasTest && enableEmergencyAlerts &&
        gCustomizationInfo.getCustomizedValue(this._clientId, SMSCB.KEY_ENABLE_CMAS_TEST_ALERTS, true);

    let config = {cdma: [], gsm: []};

    // The range of ril_worker accepts is half-open format [a-b).

    /** Enable CDMA CMAS series messages. */

    // Always enable CDMA Presidential messages.
    config.cdma.push(Ci.nsIGonkCellBroadcastConfigService.SERVICE_CATEGORY_CMAS_PRESIDENTIAL_LEVEL_ALERT);
    config.cdma.push(Ci.nsIGonkCellBroadcastConfigService.SERVICE_CATEGORY_CMAS_PRESIDENTIAL_LEVEL_ALERT+1);

    // Enable/Disable CDMA CMDAS extreme messages.
    if (enableCmasExtremeAlerts) {
      config.cdma.push(Ci.nsIGonkCellBroadcastConfigService.SERVICE_CATEGORY_CMAS_EXTREME_THREAT);
      config.cdma.push(Ci.nsIGonkCellBroadcastConfigService.SERVICE_CATEGORY_CMAS_EXTREME_THREAT+1);
    }

    // Enable/Disable CDMA CMAS severe messages.
    if (enableCmasSevereAlerts) {
      config.cdma.push(Ci.nsIGonkCellBroadcastConfigService.SERVICE_CATEGORY_CMAS_SEVERE_THREAT);
      config.cdma.push(Ci.nsIGonkCellBroadcastConfigService.SERVICE_CATEGORY_CMAS_SEVERE_THREAT+1);
    }

    // Enable/Disable CDMA CMAS amber alert messages.
    if (enableCmasAmberAlerts) {
      config.cdma.push(Ci.nsIGonkCellBroadcastConfigService.SERVICE_CATEGORY_CMAS_CHILD_ABDUCTION_EMERGENCY);
      config.cdma.push(Ci.nsIGonkCellBroadcastConfigService.SERVICE_CATEGORY_CMAS_CHILD_ABDUCTION_EMERGENCY+1);
    }

    // Enable/Disable CDMA CMAS test messages.
    if (enableCmasTestAlerts) {
      config.cdma.push(Ci.nsIGonkCellBroadcastConfigService.SERVICE_CATEGORY_CMAS_TEST_MESSAGE);
      config.cdma.push(Ci.nsIGonkCellBroadcastConfigService.SERVICE_CATEGORY_CMAS_TEST_MESSAGE+1);
    }

    /** Enable GSM ETWS series messages. */

    // Enable/Disable GSM ETWS messages (4352~4354).
    if (enableEtwsAlerts) {
      config.gsm.push(Ci.nsIGonkCellBroadcastConfigService.MESSAGE_ID_ETWS_EARTHQUAKE_WARNING);
      config.gsm.push(Ci.nsIGonkCellBroadcastConfigService.MESSAGE_ID_ETWS_EARTHQUAKE_AND_TSUNAMI_WARNING+1);
    }

    // Enable/Disable GSM ETWS messages (4356)
    if (enableEtwsAlerts) {
      config.gsm.push(Ci.nsIGonkCellBroadcastConfigService.MESSAGE_ID_ETWS_OTHER_EMERGENCY_TYPE);
      config.gsm.push(Ci.nsIGonkCellBroadcastConfigService.MESSAGE_ID_ETWS_OTHER_EMERGENCY_TYPE+1);
    }

    // Enable/Disable GSM ETWS test messages.(4335).
    if (enableEtwsTestAlerts) {
      config.gsm.push(Ci.nsIGonkCellBroadcastConfigService.MESSAGE_ID_ETWS_TEST_MESSAGE);
      config.gsm.push(Ci.nsIGonkCellBroadcastConfigService.MESSAGE_ID_ETWS_TEST_MESSAGE+1);
    }

    /** Enable GSM CMAS series messages. */

    // Enable/Disable GSM CMAS presidential message (4370)
    config.gsm.push(Ci.nsIGonkCellBroadcastConfigService.MESSAGE_ID_CMAS_ALERT_PRESIDENTIAL_LEVEL);
    config.gsm.push(Ci.nsIGonkCellBroadcastConfigService.MESSAGE_ID_CMAS_ALERT_PRESIDENTIAL_LEVEL+1);

    // Enable/Disable GSM CMAS extreme messages (4371~4372).
    if (enableCmasExtremeAlerts) {
      config.gsm.push(Ci.nsIGonkCellBroadcastConfigService.MESSAGE_ID_CMAS_ALERT_EXTREME_IMMEDIATE_OBSERVED);
      config.gsm.push(Ci.nsIGonkCellBroadcastConfigService.MESSAGE_ID_CMAS_ALERT_EXTREME_IMMEDIATE_LIKELY+1);
    }

    // Enable/Disable GSM CMAS severe messages (4373~4378).
    if (enableCmasSevereAlerts) {
      config.gsm.push(Ci.nsIGonkCellBroadcastConfigService.MESSAGE_ID_CMAS_ALERT_EXTREME_EXPECTED_OBSERVED);
      config.gsm.push(Ci.nsIGonkCellBroadcastConfigService.MESSAGE_ID_CMAS_ALERT_SEVERE_EXPECTED_LIKELY+1);
    }

    // Enable/Disable GSM CMAS amber alert messages (4379).
    if (enableCmasAmberAlerts) {
      config.gsm.push(Ci.nsIGonkCellBroadcastConfigService.MESSAGE_ID_CMAS_ALERT_CHILD_ABDUCTION_EMERGENCY);
      config.gsm.push(Ci.nsIGonkCellBroadcastConfigService.MESSAGE_ID_CMAS_ALERT_CHILD_ABDUCTION_EMERGENCY+1);
    }

    // Enable/Disable GSM CMAS test messages (4380~4382).
    if (enableCmasTestAlerts) {
      config.gsm.push(Ci.nsIGonkCellBroadcastConfigService.MESSAGE_ID_CMAS_ALERT_REQUIRED_MONTHLY_TEST);
      config.gsm.push(Ci.nsIGonkCellBroadcastConfigService.MESSAGE_ID_CMAS_ALERT_OPERATOR_DEFINED_USE+1);
    }

    /** Enable GSM CMAS series messages for additional languages. */

    // Enable/Disable GSM CMAS presidential messages for additional languages (4383).
      config.gsm.push(Ci.nsIGonkCellBroadcastConfigService.MESSAGE_ID_CMAS_ALERT_PRESIDENTIAL_LEVEL_LANGUAGE);
      config.gsm.push(Ci.nsIGonkCellBroadcastConfigService.MESSAGE_ID_CMAS_ALERT_PRESIDENTIAL_LEVEL_LANGUAGE+1);

    // Enable/Disable GSM CMAS extreme messages for additional languages (4384~4385).
    if (enableCmasExtremeAlerts) {
      config.gsm.push(Ci.nsIGonkCellBroadcastConfigService.MESSAGE_ID_CMAS_ALERT_EXTREME_IMMEDIATE_OBSERVED_LANGUAGE);
      config.gsm.push(Ci.nsIGonkCellBroadcastConfigService.MESSAGE_ID_CMAS_ALERT_EXTREME_IMMEDIATE_LIKELY_LANGUAGE+1);
    }

    // Enable/Disable GSM CMAS severe messages for additional languages (4386~4391).
    if (enableCmasSevereAlerts) {
      config.gsm.push(Ci.nsIGonkCellBroadcastConfigService.MESSAGE_ID_CMAS_ALERT_EXTREME_EXPECTED_OBSERVED_LANGUAGE);
      config.gsm.push(Ci.nsIGonkCellBroadcastConfigService.MESSAGE_ID_CMAS_ALERT_SEVERE_EXPECTED_LIKELY_LANGUAGE+1);
    }

    // Enable/Disable GSM CMAS amber alert messages for additional languages (4392).
    if (enableCmasAmberAlerts) {
      config.gsm.push(Ci.nsIGonkCellBroadcastConfigService.MESSAGE_ID_CMAS_ALERT_CHILD_ABDUCTION_EMERGENCY_LANGUAGE);
      config.gsm.push(Ci.nsIGonkCellBroadcastConfigService.MESSAGE_ID_CMAS_ALERT_CHILD_ABDUCTION_EMERGENCY_LANGUAGE+1);
    }

    // Enable/Disable GSM CMAS test messages for additional languages (4393~4395).
    if (enableCmasTestAlerts) {
      config.gsm.push(Ci.nsIGonkCellBroadcastConfigService.MESSAGE_ID_CMAS_ALERT_REQUIRED_MONTHLY_TEST_LANGUAGE);
      config.gsm.push(Ci.nsIGonkCellBroadcastConfigService.MESSAGE_ID_CMAS_ALERT_OPERATOR_DEFINED_USE_LANGUAGE+1);
    }

    /* BDC kanxj 20190325 porting to config cellbroadcast channels for operators start */
    if (this._mccmnc  === '20404') {
      //Enable message (919)
      config.gsm.push(0x397);
      config.gsm.push(0x397+1);
    } else if (this._icc.iccInfo.mcc === '466'){
      //Enable message (919)
      config.gsm.push(0x397);
      config.gsm.push(0x397+1);

      //Enable message (911)
      config.gsm.push(0x38F);
      config.gsm.push(0x38F+1);

      /* << BTS-2852 :BDC kanxj 20191012 change for PWS Failing on 2nd PA testing >> */
      //Enable message (4374)
      config.gsm.push(Ci.nsIGonkCellBroadcastConfigService.MESSAGE_ID_CMAS_ALERT_EXTREME_EXPECTED_LIKELY);
      config.gsm.push(Ci.nsIGonkCellBroadcastConfigService.MESSAGE_ID_CMAS_ALERT_EXTREME_EXPECTED_LIKELY+1);
    } else if (this._mccmnc  === '22201') {
      //Enable message (49191)
      config.gsm.push(0xC027);
      config.gsm.push(0xC027+1);
    } else if (this._mccmnc  === '22288') {
      //Enable message (4200)
      config.gsm.push(0x1068);
      config.gsm.push(0x1068+1);
    } else if (this._mccmnc  === '22299') {
      //Enable message (4300)
      config.gsm.push(0x10CC);
      config.gsm.push(0x10CC+1);
    /* BDC kanxj 20190423 add to config cellbroadcast channels for operators start */
    } else if (this._icc.iccInfo.mcc === '425'){
      //Enable message (919)
      config.gsm.push(0x397);
      config.gsm.push(0x397+1);

      //Enable message (920)
      config.gsm.push(0x398);
      config.gsm.push(0x398+1);

      //Enable message (921)
      config.gsm.push(0x399);
      config.gsm.push(0x399+1);

      //Enable message (922)
      config.gsm.push(0x39A);
      config.gsm.push(0x39A+1);
    } else if (this._mccmnc  === '65501'
         || this._mccmnc  === '65502'
         || this._mccmnc  === '65507'){
      //Enable message (50)
      config.gsm.push(0x32);
      config.gsm.push(0x32+1);
    /* << LIO-43 :BDC kanxj 20200605 Turkey - Cell Broadcast Emergency Channel Definition */
    } else if (this._icc.iccInfo.mcc === '286'){
      //Enable message (112)
      config.gsm.push(0x70);
      config.gsm.push(0x70+1);
    }  else if (this._icc.iccInfo.mcc === '732'
         || this._icc.iccInfo.mcc === '334'){
      //Enable message (50)
      config.gsm.push(0x0032);
      config.gsm.push(0x0032+1);

      //Enable message (919)
      config.gsm.push(0x397);
      config.gsm.push(0x397+1);
    /* << LIO-38 :BDC kanxj 20200605 [Peru] Cell Broadcast Channel Definition */
    } else if (this._icc.iccInfo.mcc === '716') {
      //Enable message (50)
      config.gsm.push(0x0032);
      config.gsm.push(0x0032+1);

      //Enable message (519)
      config.gsm.push(0x207);
      config.gsm.push(0x207+1);

      //Enable message (919)
      config.gsm.push(0x397);
      config.gsm.push(0x397+1);

      config.gsm.push(Ci.nsIGonkCellBroadcastConfigService.MESSAGE_ID_CMAS_ALERT_OPERATOR_DEFINED_USE_LANGUAGE+1);
      config.gsm.push(Ci.nsIGonkCellBroadcastConfigService.MESSAGE_ID_CMAS_LAST_IDENTIFIER +1);
    /* >> LIO-38 */
    /* << LIO-45 :BDC kanxj 20200724 UAE Public Warning System feature */
    } else if (this._icc.iccInfo.mcc === '419'
         || this._icc.iccInfo.mcc === '424'
         || this._icc.iccInfo.mcc === '426'
         || this._icc.iccInfo.mcc === '427') {
      //Enable message (4396~4399)
      config.gsm.push(Ci.nsIGonkCellBroadcastConfigService.MESSAGE_ID_CMAS_ALERT_OPERATOR_DEFINED_USE_LANGUAGE+1);
      config.gsm.push(Ci.nsIGonkCellBroadcastConfigService.MESSAGE_ID_CMAS_LAST_IDENTIFIER +1);
    /* >> LIO-45 */
    /* << [LIO-2317]: BDC kanxj 20210716 Cell Broadcast for Ecuador >> */
    } else if (this._icc.iccInfo.mcc === '740') {
      //Enable message (519)
      config.gsm.push(0x207);
      config.gsm.push(0x207+1);

      //Enable message (919)
      config.gsm.push(0x397);
      config.gsm.push(0x397+1);

      //Enable message (6400)
      config.gsm.push(0x1900);
      config.gsm.push(0x1900+1);
    }

    let customRange = gCustomizationInfo.getCustomizedValue(this._clientId, SMSCB.KEY_OTHER_CHANNEL_RANGES, []);
    if (customRange) {
      customRange.forEach((item) => {
        config.gsm.push(item);
      });
    }

    if (DEBUG) debug("_getCellBroadcastConfig: " + this._clientId + ", " + JSON.stringify(config));
    return config;
  },
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([GonkCellBroadcastConfigService]);
