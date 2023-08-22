/* -*- Mode: js; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/systemlibs.js");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Promise.jsm");

XPCOMUtils.defineLazyGetter(this, "RIL", function () {
  let obj = Cu.import("resource://gre/modules/ril_consts.js", null);
  return obj;
});

const GONK_MOBILECONNECTIONSERVICE_CONTRACTID =
  "@mozilla.org/mobileconnection/gonkmobileconnectionservice;1";

const GONK_MOBILECONNECTIONSERVICE_CID =
  Components.ID("{0c9c1a96-2c72-4c55-9e27-0ca73eb16f63}");
const MOBILECONNECTIONINFO_CID =
  Components.ID("{8162b3c0-664b-45f6-96cd-f07b4e193b0e}");
const MOBILEDEVICEIDENTITIES_CID =
  Components.ID("{f38786c0-786a-11e5-a837-0800200c9a66}");
const MOBILENETWORKINFO_CID =
  Components.ID("{a6c8416c-09b4-46d1-bf29-6520d677d085}");
const MOBILECELLINFO_CID =
  Components.ID("{0635d9ab-997e-4cdf-84e7-c1883752dff3}");
const MOBILECALLFORWARDINGOPTIONS_CID =
  Components.ID("{e0cf4463-ee63-4b05-ab2e-d94bf764836c}");
const MOBILESIGNALSTRENGTH_CID =
  Components.ID("{0ef33667-389b-4864-930c-0622d2d1833c}");
const NEIGHBORINGCELLINFO_CID =
  Components.ID("{6078cbf1-f34c-44fa-96f8-11a88d4bfdd3}");
const GSMCELLINFO_CID =
  Components.ID("{e3cf3aa0-f992-48fe-967b-ec98a28c8535}");
const WCDMACELLINFO_CID =
  Components.ID("{62e2c83c-b535-4068-9762-8039fac48106}");
const CDMACELLINFO_CID =
  Components.ID("{40f491f0-dd8b-42fd-af32-aef5b002749a}");
const LTECELLINFO_CID =
  Components.ID("{715e2c76-3b08-41e4-8ea5-e60c5ce6393e}");


const NS_XPCOM_SHUTDOWN_OBSERVER_ID      = "xpcom-shutdown";
const NS_PREFBRANCH_PREFCHANGE_TOPIC_ID  = "nsPref:changed";
const NS_NETWORK_ACTIVE_CHANGED_TOPIC_ID = "network-active-changed";
const NS_DATA_CALL_ERROR_TOPIC_ID        = "data-call-error";

const TOPIC_MOZSETTINGS_CHANGED = "mozsettings-changed";
const kPrefRilDataServiceId = "ril.data.defaultServiceId";
const kPrefSupportPrimarySimSwitch = "ril.support.primarysim.switch";
const kPrefAmlEnabled = "ril.aml.enabled";

const kPrefRilDebuggingEnabled = "ril.debugging.enabled";

const UNKNOWN_VALUE = Ci.nsICellInfo.UNKNOWN_VALUE;
const SIGNAL_UNKNOWN_VALUE = Ci.nsIMobileSignalStrength.SIGNAL_UNKNOWN_VALUE;

const SIGNAL_STRENGTH_NONE_OR_UNKNOWN = 0;
const SIGNAL_STRENGTH_POOR = 1;
const SIGNAL_STRENGTH_MODERATE = 2;
const SIGNAL_STRENGTH_GOOD = 3;
const SIGNAL_STRENGTH_GREAT = 4;

// TODO: Customization for rsrp/rssnr range.
const rsrp_thresh = [-140, -128, -118, -108, -98, -44];

XPCOMUtils.defineLazyServiceGetter(this, "gMobileConnectionMessenger",
                                   "@mozilla.org/ril/system-messenger-helper;1",
                                   "nsIMobileConnectionMessenger");

XPCOMUtils.defineLazyServiceGetter(this, "gNetworkManager",
                                   "@mozilla.org/network/manager;1",
                                   "nsINetworkManager");

XPCOMUtils.defineLazyServiceGetter(this, "gIccService",
                                   "@mozilla.org/icc/iccservice;1",
                                   "nsIIccService");

XPCOMUtils.defineLazyServiceGetter(this, "gDataCallManager",
                                   "@mozilla.org/datacall/manager;1",
                                   "nsIDataCallManager");

XPCOMUtils.defineLazyModuleGetter(this, "gTelephonyUtils",
                                  "resource://gre/modules/TelephonyUtils.jsm",
                                  "TelephonyUtils");

XPCOMUtils.defineLazyServiceGetter(this, "gCustomizationInfo",
                                   "@kaiostech.com/customizationinfo;1",
                                   "nsICustomizationInfo");

XPCOMUtils.defineLazyGetter(this, "gRadioInterfaceLayer", function() {
  let ril = { numRadioInterfaces: 0 };
  try {
    ril = Cc["@mozilla.org/ril;1"].getService(Ci.nsIRadioInterfaceLayer);
  } catch(e) {}
  return ril;
});

var DEBUG = RIL.DEBUG_RIL;
var AmlEnabled = false;
function debug(s) {
  dump("MobileConnectionService: " + s + "\n");
}

function MobileNetworkInfo() {
  this.shortName = null;
  this.longName = null;
  this.mcc = null;
  this.mnc = null;
  this.stat = null;
}
MobileNetworkInfo.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIMobileNetworkInfo]),
  classID:        MOBILENETWORKINFO_CID,
  classInfo:      XPCOMUtils.generateCI({
    classID:          MOBILENETWORKINFO_CID,
    classDescription: "MobileNetworkInfo",
    interfaces:       [Ci.nsIMobileNetworkInfo]
  })
};

function MobileSignalStrength(aClientId) {
  this._clientId = aClientId;
}
MobileSignalStrength.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIMobileSignalStrength]),
  classID:        MOBILESIGNALSTRENGTH_CID,
  classInfo:      XPCOMUtils.generateCI({
    classID:          MOBILESIGNALSTRENGTH_CID,
    classDescription: "MobileSignalStrength",
    interfaces:       [Ci.nsIMobileSignalStrength]
  }),

  /**
   * A utility function to dump debug message.
   */
  _debug: function(aMessage) {
    dump("MobileSignalStrength[" + this._clientId + "]: " + aMessage + "\n");
  },

  // nsIMobileSignalStrength
  level: 0,

  gsmSignalStrength: 99,
  gsmBitErrorRate: 99,

  cdmaDbm: -1,
  cdmaEcio: -1,
  cdmaEvdoDbm: -1,
  cdmaEvdoEcio: -1,
  cdmaEvdoSNR: -1,

  lteSignalStrength: 99,
  lteRsrp: SIGNAL_UNKNOWN_VALUE,
  lteRsrq: SIGNAL_UNKNOWN_VALUE,
  lteRssnr: SIGNAL_UNKNOWN_VALUE,
  lteCqi: SIGNAL_UNKNOWN_VALUE,
  lteTimingAdvance: SIGNAL_UNKNOWN_VALUE,

  tdscdmaRscp: SIGNAL_UNKNOWN_VALUE,

  _validateInfo: function(aSignalStrength) {
    let ss = aSignalStrength;

    ss.gsmSignalStrength = ss.gsmSignalStrength > 0 ?
      ss.gsmSignalStrength : 99;

    ss.cdmaDbm = ss.cdmaDbm > 0 ? -ss.cdmaDbm : -120;
    ss.cdmaEcio = ss.cdmaEcio > 0 ? -ss.cdmaEcio : -160;
    ss.cdmaEvdoDbm = ss.cdmaEvdoDbm > 0 ? -ss.cdmaEvdoDbm : -120;
    ss.cdmaEvdoEcio = ss.cdmaEvdoEcio >= 0 ? -ss.cdmaEvdoEcio : -1;
    ss.cdmaEvdoSNR = ((ss.cdmaEvdoSNR > 0) && (ss.cdmaEvdoSNR <=8)) ?
                       ss.cdmaEvdoSNR : -1;

    ss.lteSignalStrength = ss.lteSignalStrength >= 0 ?
      ss.lteSignalStrength : 99;
    ss.lteRsrp = ((ss.lteRsrp >= 44) && (ss.lteRsrp <= 140)) ?
      -ss.lteRsrp : SIGNAL_UNKNOWN_VALUE;
    ss.lteRsrq = ((ss.lteRsrq >= 3) && (ss.lteRsrq <= 20)) ?
      -ss.lteRsrq : SIGNAL_UNKNOWN_VALUE;
    ss.lteRssnr = ((ss.lteRssnr >= -200) && (ss.lteRssnr <= 300)) ?
      -ss.lteRssnr : SIGNAL_UNKNOWN_VALUE;

    ss.tdscdmaRscp = ((ss.tdscdmaRscp >= 25) && (ss.tdscdmaRscp <= 120)) ?
      -ss.tdscdmaRscp : SIGNAL_UNKNOWN_VALUE;

    return ss;
  },

  _updateLevel: function(aRadioTech) {
    return new Promise((aResolve, aReject) => {
      let isGsm = true;
      if (aRadioTech == RIL.NETWORK_CREG_TECH_IS95A ||
        aRadioTech == RIL.NETWORK_CREG_TECH_IS95B ||
        aRadioTech == RIL.NETWORK_CREG_TECH_1XRTT ||
        aRadioTech == RIL.NETWORK_CREG_TECH_EVDO0 ||
        aRadioTech == RIL.NETWORK_CREG_TECH_EVDOA ||
        aRadioTech == RIL.NETWORK_CREG_TECH_EVDOB ||
        aRadioTech == RIL.NETWORK_CREG_TECH_EHRPD) {
        isGsm = false;
      }

      let level;
      if (isGsm) {
        level = this._getLteLevel();
          if (level == SIGNAL_STRENGTH_NONE_OR_UNKNOWN) {
            level = this._getTdscdmaLevel();
              if (level == SIGNAL_STRENGTH_NONE_OR_UNKNOWN) {
                level = this._getGsmLevel();
              }
          }
      } else {
        let cdmaLevel = this._getCdmaLevel();
        let evdoLevel = this._getCdmaEvdoLevel();
        if (evdoLevel == SIGNAL_STRENGTH_NONE_OR_UNKNOWN) {
          level = cdmaLevel;
        } else if (cdmaLevel == SIGNAL_STRENGTH_NONE_OR_UNKNOWN) {
          level = evdoLevel;
        } else {
          level = cdmaLevel < evdoLevel ? cdmaLevel : evdoLevel;
        }
      }

      this.level = level;

      aResolve();
    });
  },

  _getLteLevel: function() {
    let levelRsrp = -1;
    let levelRssnr = -1;
    let levelRssi = SIGNAL_STRENGTH_NONE_OR_UNKNOWN;

    if (this.lteRsrp > rsrp_thresh[5]) {
      levelRsrp = -1;
    } else if (this.lteRsrp > rsrp_thresh[4]) {
      levelRsrp = SIGNAL_STRENGTH_GREAT;
    } else if (this.lteRsrp > rsrp_thresh[3]) {
      levelRsrp = SIGNAL_STRENGTH_GOOD;
    } else if (this.lteRsrp > rsrp_thresh[2]) {
      levelRsrp = SIGNAL_STRENGTH_MODERATE;
    } else if (this.lteRsrp > rsrp_thresh[1]) {
      levelRsrp = SIGNAL_STRENGTH_POOR;
    } else if (this.lteRsrp > rsrp_thresh[0]) {
      levelRsrp = SIGNAL_STRENGTH_NONE_OR_UNKNOWN;
    }

    if (this.lteRssnr > 300) {
      levelRssnr = -1;
    } else if (this.lteRssnr >= 130) {
      levelRssnr = SIGNAL_STRENGTH_GREAT;
    } else if (this.lteRssnr >= 45) {
      levelRssnr = SIGNAL_STRENGTH_GOOD;
    } else if (this.lteRssnr >= 10) {
      levelRssnr = SIGNAL_STRENGTH_MODERATE;
    } else if (this.lteRssnr >= -30) {
      levelRssnr = SIGNAL_STRENGTH_POOR;
    } else if (this.lteRssnr >= -200) {
      levelRssnr = SIGNAL_STRENGTH_NONE_OR_UNKNOWN;
    }

    if (DEBUG) {
      this._debug("getLteLevel - lteRsrp  : " + this.lteRsrp  +
        ". lteRssnr : " + this.lteRssnr + ". levelRsrp : " + levelRsrp +
        ". levelRssnr : " + levelRssnr);
    }

    if (levelRsrp != -1 && levelRssnr != -1) {
      return (levelRsrp < levelRssnr) ? levelRssnr : levelRsrp;
    }

    if (levelRssnr != -1) {
      return levelRssnr;
    }

    if (levelRsrp != -1) {
      return levelRsrp;
    }

    if (this.lteSignalStrength > 63 ) {
      levelRssi = SIGNAL_STRENGTH_NONE_OR_UNKNOWN;
    } else if (this.lteSignalStrength >= 12) {
      levelRssi = SIGNAL_STRENGTH_GREAT;
    } else if (this.lteSignalStrength >= 8) {
      levelRssi = SIGNAL_STRENGTH_GOOD;
    } else if (this.lteSignalStrength >= 5) {
      levelRssi = SIGNAL_STRENGTH_MODERATE;
    } else if (this.lteSignalStrength >= 0) {
      levelRssi = SIGNAL_STRENGTH_POOR;
    }

    if (DEBUG) {
      this._debug("getLteLevel - rssi : " + this.lteSignalStrength +
        ". levelRssi :" + levelRssi);
    }
    return levelRssi;
  },

  _getGsmLevel: function() {
    let level = SIGNAL_STRENGTH_NONE_OR_UNKNOWN;

    if (this.gsmSignalStrength <= 2 || this.gsmSignalStrength == 99) {
      level = SIGNAL_STRENGTH_NONE_OR_UNKNOWN;
    } else if (this.gsmSignalStrength >= 12) {
      level = SIGNAL_STRENGTH_GREAT;
    } else if (this.gsmSignalStrength >= 8) {
      level = SIGNAL_STRENGTH_GOOD;
    } else if (this.gsmSignalStrength >= 5) {
      level = SIGNAL_STRENGTH_MODERATE;
    } else {
      level = SIGNAL_STRENGTH_POOR;
    }

    if (DEBUG) {
      this._debug("getGsmLevel = " + level);
    }
    return level;
  },

  _getCdmaLevel: function() {
    let level = SIGNAL_STRENGTH_NONE_OR_UNKNOWN;
    let levelDbm;
    let levelEcio;

    if (this.cdmaDbm >= -75) {
      levelDbm = SIGNAL_STRENGTH_GREAT;
    } else if (this.cdmaDbm >= -85) {
      levelDbm = SIGNAL_STRENGTH_GOOD;
    } else if (this.cdmaDbm >= -95) {
      levelDbm = SIGNAL_STRENGTH_MODERATE;
    } else if (this.cdmaDbm >= -100) {
      levelDbm = SIGNAL_STRENGTH_POOR;
    } else {
      levelDbm = SIGNAL_STRENGTH_NONE_OR_UNKNOWN;
    }

    if (this.cdmaEcio >= -90) {
      levelEcio = SIGNAL_STRENGTH_GREAT;
    } else if (this.cdmaEcio >= -110) {
      levelEcio = SIGNAL_STRENGTH_GOOD;
    } else if (this.cdmaEcio >= -130) {
      levelEcio = SIGNAL_STRENGTH_MODERATE;
    } else if (this.cdmaEcio >= -150) {
      levelEcio = SIGNAL_STRENGTH_POOR;
    } else {
      levelEcio = SIGNAL_STRENGTH_NONE_OR_UNKNOWN;
    }

    if (levelDbm > levelEcio) {
      level = levelEcio;
    } else {
      level = levelDbm;
    }

    if (DEBUG) {
      this._debug("getCdmaLevel = " + level);
    }
    return level;
  },

  _getCdmaEvdoLevel: function() {
    let level = SIGNAL_STRENGTH_NONE_OR_UNKNOWN;
    let levelEvdoDbm;
    let levelEvdoSnr;

    if (this.cdmaEvdoDbm >= -65) {
      levelEvdoDbm = SIGNAL_STRENGTH_GREAT;
    } else if (this.cdmaEvdoDbm >= -75) {
      levelEvdoDbm = SIGNAL_STRENGTH_GOOD;
    } else if (this.cdmaEvdoDbm >= -90) {
      levelEvdoDbm = SIGNAL_STRENGTH_MODERATE;
    } else if (this.cdmaEvdoDbm >= -105) {
      levelEvdoDbm = SIGNAL_STRENGTH_POOR;
    } else {
      levelEvdoDbm = SIGNAL_STRENGTH_NONE_OR_UNKNOWN;
    }

    if (this.cdmaEvdoSNR >= 7) {
      levelEvdoSnr = SIGNAL_STRENGTH_GREAT;
    } else if (this.cdmaEvdoSNR >= 5) {
      levelEvdoSnr = SIGNAL_STRENGTH_GOOD;
    } else if (this.cdmaEvdoSNR >= 3) {
      levelEvdoSnr = SIGNAL_STRENGTH_MODERATE;
    } else if (this.cdmaEvdoSNR >= 1) {
      levelEvdoSnr = SIGNAL_STRENGTH_POOR;
    } else {
      levelEvdoSnr = SIGNAL_STRENGTH_NONE_OR_UNKNOWN;
    }

    if (levelEvdoDbm > levelEvdoSnr) {
      level = levelEvdoSnr;
    } else {
      level = levelEvdoDbm;
    }

    if (DEBUG) {
      this._debug("getCdmaEvdoLevel = " + level);
    }
    return level;
  },

  _getTdscdmaLevel: function() {
    let level = SIGNAL_STRENGTH_NONE_OR_UNKNOWN;
    if (this.tdscdmaRscp > -25 ||
      this.tdscdmaRscp == SIGNAL_UNKNOWN_VALUE) {
      level = SIGNAL_STRENGTH_NONE_OR_UNKNOWN;
    } else if (this.tdscdmaRscp >= -49) {
      level = SIGNAL_STRENGTH_GREAT;
    } else if (this.tdscdmaRscp >= -73) {
      level = SIGNAL_STRENGTH_GOOD;
    } else if (this.tdscdmaRscp >= -97) {
      level = SIGNAL_STRENGTH_MODERATE;
    } else if (this.tdscdmaRscp >= -110) {
      level = SIGNAL_STRENGTH_POOR;
    }

    if (DEBUG) {
      this._debug("getTdscdmaLevel = " + level);
    }
    return level;
  }
};

function MobileCellInfo() {
  this.gsmLocationAreaCode = -1;
  this.gsmCellId = -1;
  this.cdmaBaseStationId = -1;
  this.cdmaBaseStationLatitude = -2147483648;
  this.cdmaBaseStationLongitude = -2147483648;
  this.cdmaSystemId = -1;
  this.cdmaNetworkId = -1;
  this.cdmaRoamingIndicator = -1;
  this.cdmaSystemIsInPRL = false;
  this.cdmaDefaultRoamingIndicator = -1;
}
MobileCellInfo.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIMobileCellInfo]),
  classID:        MOBILECELLINFO_CID,
  classInfo:      XPCOMUtils.generateCI({
    classID:          MOBILECELLINFO_CID,
    classDescription: "MobileCellInfo",
    interfaces:       [Ci.nsIMobileCellInfo]
  })
};

function MobileConnectionInfo() {}
MobileConnectionInfo.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIMobileConnectionInfo]),
  classID: MOBILECONNECTIONINFO_CID,
  classInfo: XPCOMUtils.generateCI({
    classID: MOBILECONNECTIONINFO_CID,
    classDescription: "MobileConnectionInfo",
    interfaces: [Ci.nsIMobileConnectionInfo]
  }),

  state: null,
  connected: false,
  emergencyCallsOnly: false,
  roaming: false,
  network: null,
  cell: null,
  type: null
};

function MobileDeviceIdentities() {
  this.imei = null;
  this.imeisv = null;
  this.esn = null;
  this.meid = null;
}
MobileDeviceIdentities.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIMobileDeviceIdentities]),
  classID:        MOBILEDEVICEIDENTITIES_CID,
  classInfo:      XPCOMUtils.generateCI({
    classID:          MOBILEDEVICEIDENTITIES_CID,
    classDescription: "MobileDeviceIdentities",
    interfaces:       [Ci.nsIMobileDeviceIdentities]
  })
};

function MobileCallForwardingOptions(aOptions) {
  for (let key in aOptions) {
    this[key] = aOptions[key];
  }
}
MobileCallForwardingOptions.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIMobileCallForwardingOptions]),
  classID: MOBILECALLFORWARDINGOPTIONS_CID,
  classInfo: XPCOMUtils.generateCI({
    classID:          MOBILECALLFORWARDINGOPTIONS_CID,
    classDescription: "MobileCallForwardingOptions",
    interfaces:       [Ci.nsIMobileCallForwardingOptions]
  }),

  // nsIMobileForwardingOptions

  active: false,
  action: Ci.nsIMobileConnection.CALL_FORWARD_ACTION_UNKNOWN,
  reason: Ci.nsIMobileConnection.CALL_FORWARD_REASON_UNKNOWN,
  number: null,
  timeSeconds: -1,
  serviceClass: Ci.nsIMobileConnection.ICC_SERVICE_CLASS_NONE
}

function NeighboringCellInfo(aOptions) {
  this.networkType = aOptions.networkType;
  this.gsmLocationAreaCode = (aOptions.gsmLocationAreaCode !== undefined &&
                              aOptions.gsmLocationAreaCode >= 0 &&
                              aOptions.gsmLocationAreaCode <= 65535) ?
                             aOptions.gsmLocationAreaCode : UNKNOWN_VALUE;
  this.gsmCellId = (aOptions.gsmCellId !== undefined &&
                    aOptions.gsmCellId >= 0 &&
                    aOptions.gsmCellId <= 65535) ?
                   aOptions.gsmCellId : UNKNOWN_VALUE;
  this.wcdmaPsc = (aOptions.wcdmaPsc !== undefined && aOptions.wcdmaPsc >= 0 &&
                   aOptions.wcdmaPsc <= 511) ?
                  aOptions.wcdmaPsc : UNKNOWN_VALUE;
  this.signalStrength = (aOptions.signalStrength !== undefined &&
                         ((aOptions.signalStrength >= 0 &&
                           aOptions.signalStrength <= 31) ||
                          (aOptions.signalStrength >= -120 &&
                           aOptions.signalStrength <= -25))) ?
                        aOptions.signalStrength : UNKNOWN_VALUE;
}
NeighboringCellInfo.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsINeighboringCellInfo]),
  classID:        NEIGHBORINGCELLINFO_CID,
  classInfo:      XPCOMUtils.generateCI({
    classID:          NEIGHBORINGCELLINFO_CID,
    classDescription: "NeighboringCellInfo",
    interfaces:       [Ci.nsINeighboringCellInfo]
  }),

  isValid: function() {
    return !(this.gsmLocationAreaCode == UNKNOWN_VALUE &&
             this.gsmCellId == UNKNOWN_VALUE &&
             this.wcdmaPsc == UNKNOWN_VALUE &&
             this.signalStrength == UNKNOWN_VALUE);
  },

  // nsINeighboringCellInfo

  networkType: null,
  gsmLocationAreaCode: UNKNOWN_VALUE,
  gsmCellId: UNKNOWN_VALUE,
  wcdmaPsc: UNKNOWN_VALUE,
  signalStrength: UNKNOWN_VALUE
};

function CellInfo(aOptions) {
  this.type = aOptions.type;
  this.registered = aOptions.registered;
  this.timestampType = aOptions.timestampType;
  this.timestamp = aOptions.timestamp;
}
CellInfo.prototype = {

  // nsICellInfo

  type: null,
  registered: false,
  timestampType: Ci.nsICellInfo.TIMESTAMP_TYPE_UNKNOWN,
  timestamp: 0
};

function GsmCellInfo(aOptions) {
  CellInfo.call(this, aOptions);

  // Cell Identity
  this.mcc = (aOptions.mcc !== undefined && aOptions.mcc >= 0 &&
              aOptions.mcc <= 999) ? aOptions.mcc : UNKNOWN_VALUE;
  this.mnc = (aOptions.mnc !== undefined && aOptions.mnc >= 0 &&
              aOptions.mnc <= 999) ? aOptions.mnc : UNKNOWN_VALUE;
  this.lac = (aOptions.lac !== undefined && aOptions.lac >= 0 &&
              aOptions.lac <= 65535) ? aOptions.lac : UNKNOWN_VALUE;
  this.cid = (aOptions.cid !== undefined && aOptions.cid >= 0 &&
              aOptions.cid <= 65535) ? aOptions.cid : UNKNOWN_VALUE;

  // Signal Strength
  this.signalStrength = (aOptions.signalStrength !== undefined &&
                         aOptions.signalStrength >= 0 &&
                         aOptions.signalStrength <= 31) ?
                        aOptions.signalStrength : UNKNOWN_VALUE;
  this.bitErrorRate = (aOptions.bitErrorRate !== undefined &&
                       aOptions.bitErrorRate >= 0 && aOptions.bitErrorRate <= 7)
                      ? aOptions.bitErrorRate : UNKNOWN_VALUE;
}
GsmCellInfo.prototype = {
  __proto__: CellInfo.prototype,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsICellInfo,
                                         Ci.nsIGsmCellInfo]),
  classID: GSMCELLINFO_CID,
  classInfo: XPCOMUtils.generateCI({
    classID:          GSMCELLINFO_CID,
    classDescription: "GsmCellInfo",
    interfaces:       [Ci.nsIGsmCellInfo]
  }),

  isValid: function() {
    return !(this.mcc == UNKNOWN_VALUE && this.mnc == UNKNOWN_VALUE &&
             this.lac == UNKNOWN_VALUE && this.cid == UNKNOWN_VALUE &&
             this.signalStrength == UNKNOWN_VALUE &&
             this.bitErrorRate == UNKNOWN_VALUE);
  },

  // nsIGsmCellInfo

  mcc: UNKNOWN_VALUE,
  mnc: UNKNOWN_VALUE,
  lac: UNKNOWN_VALUE,
  cid: UNKNOWN_VALUE,
  signalStrength: UNKNOWN_VALUE,
  bitErrorRate: UNKNOWN_VALUE
};

function WcdmaCellInfo(aOptions) {
  CellInfo.call(this, aOptions);

  // Cell Identity
  this.mcc = (aOptions.mcc !== undefined && aOptions.mcc >= 0 &&
              aOptions.mcc <= 999) ? aOptions.mcc : UNKNOWN_VALUE;
  this.mnc = (aOptions.mnc !== undefined && aOptions.mnc >= 0 &&
              aOptions.mnc <= 999) ? aOptions.mnc : UNKNOWN_VALUE;
  this.lac = (aOptions.lac !== undefined && aOptions.lac >= 0 &&
              aOptions.lac <= 65535) ? aOptions.lac : UNKNOWN_VALUE;
  this.cid = (aOptions.cid !== undefined && aOptions.cid >= 0 &&
              aOptions.cid <= 268435455) ? aOptions.cid : UNKNOWN_VALUE;
  this.psc = (aOptions.psc !== undefined && aOptions.psc >= 0 &&
              aOptions.psc <= 511) ? aOptions.psc : UNKNOWN_VALUE;

  // Signal Strength
  this.signalStrength = (aOptions.signalStrength !== undefined &&
                         aOptions.signalStrength >= 0 &&
                         aOptions.signalStrength <= 31) ?
                        aOptions.signalStrength : UNKNOWN_VALUE;
  this.bitErrorRate = (aOptions.bitErrorRate !== undefined &&
                       aOptions.bitErrorRate >= 0 && aOptions.bitErrorRate <= 7)
                      ? aOptions.bitErrorRate : UNKNOWN_VALUE;
}
WcdmaCellInfo.prototype = {
  __proto__: CellInfo.prototype,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsICellInfo,
                                         Ci.nsIWcdmaCellInfo]),
  classID: WCDMACELLINFO_CID,
  classInfo: XPCOMUtils.generateCI({
    classID:          WCDMACELLINFO_CID,
    classDescription: "WcdmaCellInfo",
    interfaces:       [Ci.nsIWcdmaCellInfo]
  }),

  isValid: function() {
    return !(this.mcc == UNKNOWN_VALUE && this.mnc == UNKNOWN_VALUE &&
             this.lac == UNKNOWN_VALUE && this.cid == UNKNOWN_VALUE &&
             this.psc == UNKNOWN_VALUE && this.signalStrength == UNKNOWN_VALUE &&
             this.bitErrorRate == UNKNOWN_VALUE);
  },

  // nsIWcdmaCellInfo

  mcc: UNKNOWN_VALUE,
  mnc: UNKNOWN_VALUE,
  lac: UNKNOWN_VALUE,
  cid: UNKNOWN_VALUE,
  psc: UNKNOWN_VALUE,
  signalStrength: UNKNOWN_VALUE,
  bitErrorRate: UNKNOWN_VALUE
};

function LteCellInfo(aOptions) {
  CellInfo.call(this, aOptions);

  // Cell Identity
  this.mcc = (aOptions.mcc !== undefined && aOptions.mcc >= 0 &&
              aOptions.mcc <= 999) ? aOptions.mcc : UNKNOWN_VALUE;
  this.mnc = (aOptions.mnc !== undefined && aOptions.mnc >= 0 &&
              aOptions.mnc <= 999) ? aOptions.mnc : UNKNOWN_VALUE;
  this.cid = (aOptions.cid !== undefined && aOptions.cid >= 0 &&
              aOptions.cid <= 268435455) ? aOptions.cid : UNKNOWN_VALUE;
  this.pcid = (aOptions.pcid !== undefined && aOptions.pcid >= 0 &&
               aOptions.pcid <= 503) ? aOptions.pcid : UNKNOWN_VALUE;
  this.tac = (aOptions.tac !== undefined && aOptions.tac >= 0 &&
              aOptions.tac <= 65535) ? aOptions.tac : UNKNOWN_VALUE;

  // Signal Strength
  this.signalStrength = (aOptions.signalStrength !== undefined &&
                         aOptions.signalStrength >= 0 &&
                         aOptions.signalStrength <= 31) ?
                        aOptions.signalStrength : UNKNOWN_VALUE;
  this.rsrp = (aOptions.rsrp !== undefined && aOptions.rsrp >= 44 &&
               aOptions.rsrp <= 140) ? aOptions.rsrp : UNKNOWN_VALUE;
  this.rsrq = (aOptions.rsrq !== undefined && aOptions.rsrq >= 3 &&
               aOptions.rsrq <= 20) ? aOptions.rsrq : UNKNOWN_VALUE;
  this.rssnr = (aOptions.rssnr !== undefined && aOptions.rssnr >= -200 &&
                aOptions.rssnr <= 300) ? aOptions.rssnr : UNKNOWN_VALUE;
  this.cqi = (aOptions.cqi !== undefined && aOptions.cqi >= 0 &&
                aOptions.cqi <= 15) ? aOptions.cqi : UNKNOWN_VALUE;
  this.timingAdvance = (aOptions.timingAdvance !== undefined &&
                        aOptions.timingAdvance >= 0 &&
                        aOptions.timingAdvance <= 2147483646) ?
                       aOptions.timingAdvance : UNKNOWN_VALUE;
}
LteCellInfo.prototype = {
  __proto__: CellInfo.prototype,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsICellInfo,
                                         Ci.nsILteCellInfo]),
  classID: LTECELLINFO_CID,
  classInfo: XPCOMUtils.generateCI({
    classID:          LTECELLINFO_CID,
    classDescription: "LteCellInfo",
    interfaces:       [Ci.nsILteCellInfo]
  }),

  isValid: function() {
    return !(this.mcc == UNKNOWN_VALUE && this.mnc == UNKNOWN_VALUE &&
             this.cid == UNKNOWN_VALUE && this.pcid == UNKNOWN_VALUE &&
             this.tac == UNKNOWN_VALUE && this.signalStrength == UNKNOWN_VALUE &&
             this.rsrp == UNKNOWN_VALUE && this.rsrq == UNKNOWN_VALUE &&
             this.rssnr == UNKNOWN_VALUE && this.cqi == UNKNOWN_VALUE &&
             this.timingAdvance == UNKNOWN_VALUE);
  },

  // nsILteCellInfo

  mcc: UNKNOWN_VALUE,
  mnc: UNKNOWN_VALUE,
  cid: UNKNOWN_VALUE,
  pcid: UNKNOWN_VALUE,
  tac: UNKNOWN_VALUE,
  signalStrength: UNKNOWN_VALUE,
  rsrp: UNKNOWN_VALUE,
  rsrq: UNKNOWN_VALUE,
  rssnr: UNKNOWN_VALUE,
  cqi: UNKNOWN_VALUE,
  timingAdvance: UNKNOWN_VALUE
};

function CdmaCellInfo(aOptions) {
  CellInfo.call(this, aOptions);

  // Cell Identity
  this.networkId = (aOptions.networkId !== undefined &&
                    aOptions.networkId >= 0 && aOptions.networkId <= 65535) ?
                   aOptions.networkId : UNKNOWN_VALUE;
  this.systemId = (aOptions.systemId !== undefined && aOptions.systemId >= 0 &&
                   aOptions.systemId <= 32767) ?
                  aOptions.systemId : UNKNOWN_VALUE;
  this.baseStationId = (aOptions.baseStationId !== undefined &&
                        aOptions.baseStationId >= 0 &&
                        aOptions.baseStationId <= 65535) ?
                       aOptions.baseStationId : UNKNOWN_VALUE;
  this.longitude = (aOptions.longitude !== undefined &&
                    aOptions.longitude >= -2592000 &&
                    aOptions.longitude <= 2592000) ?
                   aOptions.longitude : UNKNOWN_VALUE;
  this.latitude = (aOptions.latitude !== undefined &&
                   aOptions.latitude >= -1296000 &&
                   aOptions.latitude <= 1296000) ?
                  aOptions.latitude : UNKNOWN_VALUE;

  // Signal Strength
  this.cdmaEcio = (aOptions.cdmaEcio !== undefined &&
                   aOptions.cdmaEcio >= 0) ? aOptions.cdmaEcio : UNKNOWN_VALUE;
  this.evdoDbm = (aOptions.evdoDbm !== undefined &&
                  aOptions.evdoDbm >= 0) ? aOptions.evdoDbm : UNKNOWN_VALUE;
  this.evdoEcio = (aOptions.evdoEcio !== undefined &&
                   aOptions.evdoEcio >= 0) ? aOptions.evdoEcio : UNKNOWN_VALUE;
  this.evdoSnr = (aOptions.evdoSnr !== undefined &&
                  aOptions.evdoSnr >= 0 && aOptions.evdoSnr <= 8) ?
                 aOptions.evdoSnr : UNKNOWN_VALUE;
}
CdmaCellInfo.prototype = {
  __proto__: CellInfo.prototype,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsICellInfo,
                                         Ci.nsICdmaCellInfo]),
  classID: CDMACELLINFO_CID,
  classInfo: XPCOMUtils.generateCI({
    classID:          CDMACELLINFO_CID,
    classDescription: "CdmaCellInfo",
    interfaces:       [Ci.nsICdmaCellInfo]
  }),

  isValid: function() {
    return !(this.networkId == UNKNOWN_VALUE && this.systemId == UNKNOWN_VALUE &&
             this.baseStationId == UNKNOWN_VALUE &&
             this.longitude == UNKNOWN_VALUE &&
             this.latitude == UNKNOWN_VALUE && this.cdmaDbm == UNKNOWN_VALUE &&
             this.cdmaEcio == UNKNOWN_VALUE && this.evdoDbm == UNKNOWN_VALUE &&
             this.evdoEcio == UNKNOWN_VALUE && this.evdoSnr == UNKNOWN_VALUE);
  },

  // nsICdmaCellInfo

  networkId: UNKNOWN_VALUE,
  systemId: UNKNOWN_VALUE,
  baseStationId: UNKNOWN_VALUE,
  longitude: UNKNOWN_VALUE,
  latitude: UNKNOWN_VALUE,
  cdmaDbm: UNKNOWN_VALUE,
  cdmaEcio: UNKNOWN_VALUE,
  evdoDbm: UNKNOWN_VALUE,
  evdoEcio: UNKNOWN_VALUE,
  evdoSnr: UNKNOWN_VALUE
};

function MobileDeviceIdentities(aImei, aImeisv, aEsn, aMeid) {
  this.imei = aImei;
  this.imeisv = aImeisv;
  this.esn = aEsn;
  this.meid = aMeid;
}
MobileDeviceIdentities.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIMobileDeviceIdentities]),

  // nsIMobileDeviceIdentities
  imei: null,
  imeisv: null,
  esn: null,
  meid: null
};

function MobileConnectionProvider(aClientId, aRadioInterface) {
  this._clientId = aClientId;
  this._radioInterface = aRadioInterface;
  this._operatorInfo = new MobileNetworkInfo();
  // An array of nsIMobileConnectionListener instances.
  this._listeners = [];

  this.voice = new MobileConnectionInfo();
  this.data = new MobileConnectionInfo();
  this.signalStrength = new MobileSignalStrength(this._clientId);
}
MobileConnectionProvider.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIMobileConnection]),

  _clientId: null,
  _radioInterface: null,
  _operatorInfo: null,
  _listeners: null,

  /**
   * The networks that are currently trying to be selected (or "automatic").
   * This helps ensure that only one network per client is selected at a time.
   */
  _selectingNetwork: null,

  /**
   * The two radio states below stand for the user expectation and the hardware
   * status, respectively. |radioState| will be updated based on their values.
   */
  _expectedRadioState: RIL.GECKO_RADIOSTATE_UNKNOWN,
  _hardwareRadioState: RIL.GECKO_RADIOSTATE_UNKNOWN,

  voice: null,
  data: null,
  networkSelectionMode: Ci.nsIMobileConnection.NETWORK_SELECTION_MODE_UNKNOWN,
  radioState: Ci.nsIMobileConnection.MOBILE_RADIO_STATE_UNKNOWN,
  lastKnownNetwork: null,
  lastKnownHomeNetwork: null,
  deviceIdentities: null,
  isInEmergencyCbMode: false,
  signalStrength: null,

  /**
   * Set flag to indicate there's setPreferredNetworkType needed after modem restart
   */
  _preferredNetworkType: Ci.nsIMobileConnection.PREFERRED_NETWORK_TYPE_LTE_WCDMA_GSM_CDMA_EVDO,
  _needRestorePreferredNetworkType: false,

  /**
   * Device indetities:
   * IMEI if GSM subscription is available.
   * IMEISV if GSM subscription is available.
   * ESN if CDMA subscription is available.
   * MEID if CDMA subscription is available.
   */
  _imei: null,
  _imeisv: null,
  _esn: null,
  _meid: null,

  /**
   * A utility function to dump debug message.
   */
  _debug: function(aMessage) {
    dump("MobileConnectionProvider[" + this._clientId + "]: " + aMessage + "\n");
  },

  /**
   * Function to power off radio and set modem off
   * before device power down or reboot.
   */
  shutdownRequest: function() {
    return new Promise((aResolve, aReject) => {
      this.setRadioEnabled(false, {
        notifySuccess: () => { aResolve(); },
        notifyError: (errorMsg) => {
          if (DEBUG) {
            this._debug("setRadioEnabled notifyError:" + errorMsg);
          }
          aResolve();
        }
      });
    }).then(() => {
      return new Promise((aResolve, aReject) => {
        this._radioInterface.sendWorkerMessage("shutdownRequest", null, (aResponse) => {
          aResolve();
        });
      });
    }).catch((aError) => {
      aResolve();
    });
  },

  /**
   * A utility function to get supportedNetworkTypes from system property.
   */
  _getSupportedNetworkTypes: function() {
    let key = "persist.ril." + this._clientId + ".network_types";
    let supportedNetworkTypes = libcutils.property_get(key, "").split(",");

    // If mozRIL system property is not available, fallback to AOSP system
    // property for support network types.
    if (supportedNetworkTypes.length === 1 && supportedNetworkTypes[0] === "") {
      key = "ro.telephony.default_network";
      let indexString = libcutils.property_get(key, "");
      let index = parseInt(indexString, 10);
      if (DEBUG) this._debug("Fallback to " + key + ": " + index);

      let networkTypes = RIL.RIL_PREFERRED_NETWORK_TYPE_TO_GECKO[index];
      supportedNetworkTypes = networkTypes ?
        networkTypes.replace(/-auto/g, "").split("/") :
        RIL.GECKO_SUPPORTED_NETWORK_TYPES_DEFAULT.split(",");
    }

    let enumNetworkTypes = [];
    for (let type of supportedNetworkTypes) {
      // If the value in system property is not valid, use the default one which
      // is defined in ril_consts.js.
      if (RIL.GECKO_SUPPORTED_NETWORK_TYPES.indexOf(type) < 0) {
        if (DEBUG) {
          this._debug("Unknown network type: " + type);
        }
        RIL.GECKO_SUPPORTED_NETWORK_TYPES_DEFAULT.split(",").forEach(aType => {
          enumNetworkTypes.push(RIL.GECKO_SUPPORTED_NETWORK_TYPES.indexOf(aType));
        });
        break;
      }
      enumNetworkTypes.push(RIL.GECKO_SUPPORTED_NETWORK_TYPES.indexOf(type));
    }
    if (DEBUG) {
      this._debug("Supported Network Types: " + enumNetworkTypes);
    }

    return enumNetworkTypes;
  },

  /**
   * Helper for guarding us against invalid mode for clir.
   */
  _isValidClirMode: function(aMode) {
    switch (aMode) {
      case Ci.nsIMobileConnection.CLIR_DEFAULT:
      case Ci.nsIMobileConnection.CLIR_INVOCATION:
      case Ci.nsIMobileConnection.CLIR_SUPPRESSION:
        return true;
      default:
        return false;
    }
  },

  /**
   * Fix the roaming. RIL can report roaming in some case it is not
   * really the case. See bug 787967
   *
   * See bug 12354, remove check at _updateConnectionInfo to avoid wrong status
   * update, it looks like andorid only left this check for CDMA case.
   * http://androidxref.com/6.0.1_r10/xref/frameworks/opt/telephony/src/java/com
   * /android/internal/telephony/cdma/CdmaServiceStateTracker.java#1573
   */
  _checkRoamingBetweenOperators: function(aNetworkInfo) {
    let icc = gIccService.getIccByServiceId(this._clientId);
    let iccInfo = icc ? icc.iccInfo : null;
    let operator = aNetworkInfo.network;
    let state = aNetworkInfo.state;

    if (!iccInfo || !operator ||
        state !== RIL.GECKO_MOBILE_CONNECTION_STATE_REGISTERED) {
      return false;
    }

    let spn = iccInfo.spn && iccInfo.spn.toLowerCase();
    let longName = operator.longName && operator.longName.toLowerCase();
    let shortName = operator.shortName && operator.shortName.toLowerCase();

    let equalsLongName = longName && (spn == longName);
    let equalsShortName = shortName && (spn == shortName);
    let equalsMcc = iccInfo.mcc == operator.mcc;

    let newRoaming = aNetworkInfo.roaming &&
                     !(equalsMcc && (equalsLongName || equalsShortName));
    if (newRoaming === aNetworkInfo.roaming) {
      return false;
    }

    aNetworkInfo.roaming = newRoaming;
    return true;
  },

  /**
   * The design of this updating function is to update the attribute in
   * |aDestInfo| *only if* new data (e.g. aSrcInfo) contains the same attribute.
   * Thus, for the attribute in |aDestInfo| that isn't showed in |aSrcInfo|, it
   * should just keep the original value unchanged.
   */
  _updateConnectionInfo: function(aDestInfo, aSrcInfo) {
    let isUpdated = false;
    for (let key in aSrcInfo) {
      if (key === "network" || key === "cell") {
        // nsIMobileNetworkInfo and nsIMobileCellInfo are handled explicitly below.
        continue;
      }

      if (aDestInfo[key] !== aSrcInfo[key]) {
        isUpdated = true;
        aDestInfo[key] = aSrcInfo[key];
      }
    }

    // Make sure we also reset the operator and signal strength information
    // if we drop off the network.
    if (aDestInfo.state !== RIL.GECKO_MOBILE_CONNECTION_STATE_REGISTERED &&
       (!AmlEnabled || !aDestInfo.emergencyCallsOnly)) {
      aDestInfo.cell = null;
      aDestInfo.network = null;
    } else {
      aDestInfo.network = this._operatorInfo;

      // If no new cell data is passed, we should just keep the original cell
      // data unchanged.
      if (aSrcInfo.cell) {
        if (!aDestInfo.cell) {
          aDestInfo.cell = new MobileCellInfo();
        }

        isUpdated = this._updateInfo(aDestInfo.cell, aSrcInfo.cell) || isUpdated;
      }
    }

    return isUpdated;
  },

  _updateDeviceIdentities: function(aImei, aImeisv, aEsn, aMeid) {
    this._imei = aImei;
    this._imeisv = aImeisv;
    this._esn = aEsn;
    this._meid = aMeid;
  },

  /**
   * The design of this updating function is to update the attribute in
   * |aDestInfo| *only if* new data (e.g. aSrcInfo) contains the same attribute.
   * Thus, for the attribute in |aDestInfo| that isn't showed in |aSrcInfo|, it
   * should just keep the original value unchanged.
   */
  _updateInfo: function(aDestInfo, aSrcInfo) {
    let isUpdated = false;
    for (let key in aSrcInfo) {
      if (aDestInfo[key] !== aSrcInfo[key]) {
        isUpdated = true;
        aDestInfo[key] = aSrcInfo[key];
      }
    }
    return isUpdated;
  },

  _rulesToCallForwardingOptions: function(aRules) {
    return aRules.map(rule => new MobileCallForwardingOptions(rule));
  },

  _dispatchNotifyError: function(aCallback, aErrorMsg) {
    Services.tm.currentThread.dispatch(() => aCallback.notifyError(aErrorMsg),
                                       Ci.nsIThread.DISPATCH_NORMAL);
  },

  registerListener: function(aListener) {
    if (this._listeners.indexOf(aListener) >= 0) {
      throw Cr.NS_ERROR_UNEXPECTED;
    }

    this._listeners.push(aListener);
  },

  unregisterListener: function(aListener) {
    let index = this._listeners.indexOf(aListener);
    if (index >= 0) {
      this._listeners.splice(index, 1);
    }
  },

  deliverListenerEvent: function(aName, aArgs) {
    let listeners = this._listeners.slice();
    for (let listener of listeners) {
      if (this._listeners.indexOf(listener) === -1) {
        continue;
      }
      let handler = listener[aName];
      if (typeof handler != "function") {
        throw new Error("No handler for " + aName);
      }
      try {
        handler.apply(listener, aArgs);
      } catch (e) {
        if (DEBUG) {
          this._debug("listener for " + aName + " threw an exception: " + e);
        }
      }
    }
  },

  updateVoiceInfo: function(aNewInfo, aBatch = false) {
    let isUpdated = this._updateConnectionInfo(this.voice, aNewInfo);
    if (isUpdated && !aBatch) {
      this.deliverListenerEvent("notifyVoiceChanged");
    }
  },

  updateDataInfo: function(aNewInfo, aBatch = false) {
    // For the data connection, the `connected` flag indicates whether
    // there's an active data call. We get correct `connected` state here.
    let active = gNetworkManager.activeNetworkInfo
                  && gNetworkManager.activeNetworkInfo.QueryInterface(Ci.nsIRilNetworkInfo);

    aNewInfo.connected = false;
    if (active &&
        active.type === Ci.nsINetworkInfo.NETWORK_TYPE_MOBILE &&
        active.serviceId === this._clientId) {
      aNewInfo.connected = true;
    }

    let isUpdated = this._updateConnectionInfo(this.data, aNewInfo);
    if (isUpdated && !aBatch) {
      this.deliverListenerEvent("notifyDataChanged");
    }

    if (isUpdated) {
      this._ensureDataRegistration();
    }
  },

  _dataRegistrationFailed: false,
  _ensureDataRegistration: function() {
    let isDataRegistered =
      this.data &&
      this.data.state == RIL.GECKO_MOBILE_CONNECTION_STATE_REGISTERED;
    let isVoiceRegistered =
      this.voice &&
      this.voice.state == RIL.GECKO_MOBILE_CONNECTION_STATE_REGISTERED;

    if (isVoiceRegistered && isDataRegistered) {
      if (DEBUG) {
        this._debug("Voice and data registered.");
      }
      this._dataRegistrationFailed = false;
      return;
    }

    if (isVoiceRegistered && !isDataRegistered &&
        this._clientId == gDataCallManager.dataDefaultServiceId) {

      // We have been here before, no more recovery.
      if (this._dataRegistrationFailed) {
        if (DEBUG) {
          this._debug("Voice and data not consistent: " + this.voice.state +
                      " != " + this.data.state + ".");
        }
        return;
      }

      if (DEBUG) {
        this._debug("Voice and data not consistent: " + this.voice.state +
                    " != " + this.data.state + ", try to recover.");
      }

      this._dataRegistrationFailed = true;
      // If there is any ongoing call, wait for them to disconnect.
      if (gTelephonyUtils.hasAnyCalls(this._clientId)) {
        gTelephonyUtils.waitForNoCalls(this._clientId)
          .then(() => {
            if (this._dataRegistrationFailed) {
              this._recoverDataRegistration();
            }
          });
        return;
      }

      this._recoverDataRegistration();
    }
  },


  /**
   * To recover data registration, get the current preferred network type first,
   * then set it to a temporary preferred network type, and last set back to the
   * previous preferred network type. This is will cause deregistration and
   * registration on both voice and data networks.
   */
  _recoverDataRegistration: function() {
    if (DEBUG) {
      this._debug("Trying to recover data registration...");
    }

    let currentPreferredNetworkType;

    let resetPreferredNetworkType = () => {
      this.setPreferredNetworkType(currentPreferredNetworkType, {
        QueryInterface: XPCOMUtils.generateQI([Ci.nsIMobileConnectionCallback]),
        notifySuccess: () => {},
        notifyError: aErrorMsg => {}
      });
    };

    let setTemporaryPreferredNetworkType = () => {
      this.setPreferredNetworkType(
        Ci.nsIMobileConnection.PREFERRED_NETWORK_TYPE_WCDMA_GSM_CDMA_EVDO, {
          QueryInterface: XPCOMUtils.generateQI([Ci.nsIMobileConnectionCallback]),
          notifySuccess: () =>  resetPreferredNetworkType(),
          notifyError: aErrorMsg => resetPreferredNetworkType()
      });
    };

    this.getPreferredNetworkType({
      QueryInterface: XPCOMUtils.generateQI([Ci.nsIMobileConnectionCallback]),
      notifyGetPreferredNetworkTypeSuccess: networkType => {
        currentPreferredNetworkType = networkType;
        setTemporaryPreferredNetworkType();
      },
      notifyError: aErrorMsg => {
        currentPreferredNetworkType =
          Ci.nsIMobileConnection.PREFERRED_NETWORK_TYPE_LTE_WCDMA_GSM_CDMA_EVDO;
        setTemporaryPreferredNetworkType();
      }
    });
  },

  updateOperatorInfo: function(aNewInfo, aBatch = false) {
    let isUpdated = this._updateInfo(this._operatorInfo, aNewInfo);

    // Update lastKnownNetwork
    if (this._operatorInfo.mcc && this._operatorInfo.mnc) {
      let network = this._operatorInfo.mcc + "-" + this._operatorInfo.mnc;
      if (this.lastKnownNetwork !== network) {
        if (DEBUG) {
          this._debug("lastKnownNetwork now is " + network);
        }

        this.lastKnownNetwork = network;
        this.deliverListenerEvent("notifyLastKnownNetworkChanged");
      }
    }

    // If the voice is unregistered, no need to send notification.
    if (this.voice.state !== RIL.GECKO_MOBILE_CONNECTION_STATE_REGISTERED &&
        isUpdated && !aBatch) {
      this.deliverListenerEvent("notifyVoiceChanged");
    }

    // If the data is unregistered, no need to send notification.
    if (this.data.state !== RIL.GECKO_MOBILE_CONNECTION_STATE_REGISTERED &&
        isUpdated && !aBatch) {
      this.deliverListenerEvent("notifyDataChanged");
    }
  },

  updateSignalInfo: function(aNewSignalInfo) {
    let newSignalInfo = this.signalStrength._validateInfo(aNewSignalInfo);
    if (this._updateInfo(this.signalStrength, newSignalInfo)) {
      if (DEBUG) {
        this._debug("updateSignalInfo : " + JSON.stringify(aNewSignalInfo));
      }
      let radioTech = RIL.NETWORK_CREG_TECH_UNKNOWN;
      if (this.data.state == RIL.GECKO_MOBILE_CONNECTION_STATE_REGISTERED) {
        radioTech = this.data.radioTech;
      } else if (this.voice.state == RIL.GECKO_MOBILE_CONNECTION_STATE_REGISTERED) {
        radioTech = this.voice.radioTech;
      }

      this.signalStrength._updateLevel(radioTech)
        .then(() => {
          this.deliverListenerEvent("notifySignalStrengthChanged");
        });
    }
  },

  updateRadioState: function(aMessage, aCallback = null) {
    switch (aMessage.msgType) {
      case "ExpectedRadioState":
        this._expectedRadioState = aMessage.msgData;
        break;
      case "HardwareRadioState":
        this._hardwareRadioState = aMessage.msgData;
        break;
      default:
        if (DEBUG) this._debug("updateRadioState: Invalid message type");
        return;
    }

    if (aMessage.msgType === "ExpectedRadioState" && aCallback &&
        this._hardwareRadioState === this._expectedRadioState) {
      // Early resolved
      aCallback.notifySuccess();
      return;
    }

    let newState;
    switch (this._expectedRadioState) {
      case RIL.GECKO_RADIOSTATE_ENABLED:
        newState = this._hardwareRadioState === this._expectedRadioState ?
          Ci.nsIMobileConnection.MOBILE_RADIO_STATE_ENABLED :
          Ci.nsIMobileConnection.MOBILE_RADIO_STATE_ENABLING;
        break;

      case RIL.GECKO_RADIOSTATE_DISABLED:
        newState = this._hardwareRadioState === this._expectedRadioState ?
          Ci.nsIMobileConnection.MOBILE_RADIO_STATE_DISABLED :
          Ci.nsIMobileConnection.MOBILE_RADIO_STATE_DISABLING;
        break;

      default: /* RIL.GECKO_RADIOSTATE_UNKNOWN */
        switch (this._hardwareRadioState) {
          case RIL.GECKO_RADIOSTATE_ENABLED:
            newState = Ci.nsIMobileConnection.MOBILE_RADIO_STATE_ENABLED;
            break;
          case RIL.GECKO_RADIOSTATE_DISABLED:
            newState = Ci.nsIMobileConnection.MOBILE_RADIO_STATE_DISABLED;
            break;
          default: /* RIL.GECKO_RADIOSTATE_UNKNOWN */
            newState = Ci.nsIMobileConnection.MOBILE_RADIO_STATE_UNKNOWN;
        }
    }

    // This update is triggered by underlying layers and the state is UNKNOWN
    if (aMessage.msgType === "HardwareRadioState" &&
        aMessage.msgData === RIL.GECKO_RADIOSTATE_UNKNOWN) {
      // TODO: Find a better way than just setting the radio state to UNKNOWN
      newState = Ci.nsIMobileConnection.MOBILE_RADIO_STATE_UNKNOWN;
    }

    if (newState === Ci.nsIMobileConnection.MOBILE_RADIO_STATE_ENABLING ||
        newState === Ci.nsIMobileConnection.MOBILE_RADIO_STATE_DISABLING) {
      let action = this._expectedRadioState === RIL.GECKO_RADIOSTATE_ENABLED;
      this._radioInterface.sendWorkerMessage("setRadioEnabled",
                                             {enabled: action},
                                             function(aResponse) {
        if (!aCallback) {
          return false;
        }
        if (aResponse.errorMsg) {
          aCallback.notifyError(aResponse.errorMsg);
          return false;
        }
        aCallback.notifySuccess();
        return false;
      });
    }

    if (DEBUG) this._debug("Current Radio State is '" + newState + "'");
    if (this.radioState === newState) {
      return;
    }

    this.radioState = newState;
    if (newState === Ci.nsIMobileConnection.MOBILE_RADIO_STATE_ENABLED &&
        this._needRestorePreferredNetworkType) {
      this.setPreferredNetworkType(this._preferredNetworkType, {
        QueryInterface: XPCOMUtils.generateQI([Ci.nsIMobileConnectionCallback]),
        notifySuccess: () => {},
        notifyError: aErrorMsg => {}
      });
      this._needRestorePreferredNetworkType = false;
    }

    this.deliverListenerEvent("notifyRadioStateChanged");
  },

  notifyCFStateChanged: function(aAction, aReason, aNumber, aTimeSeconds,
                                 aServiceClass) {
    this.deliverListenerEvent("notifyCFStateChanged",
                              [aAction, aReason, aNumber, aTimeSeconds,
                               aServiceClass]);
  },

  notifyDeviceIdentitiesChanged: function(aImei, aImeisv, aEsn, aMeid) {
    if (this.deviceIdentities) {
      if (DEBUG) this._debug("deviceIdentities shall not be changed once being updated.");
      return;
    }

    this.deviceIdentities =
      new MobileDeviceIdentities(aImei, aImeisv, aEsn, aMeid);
    this.deliverListenerEvent("notifyDeviceIdentitiesChanged");
  },

  getSupportedNetworkTypes: function(aTypes) {
    aTypes.value = this._getSupportedNetworkTypes().slice();
    return aTypes.value.length;
  },

  getNetworks: function(aCallback) {
    this._radioInterface.sendWorkerMessage("getAvailableNetworks", null,
                                           (function(aResponse) {
      if (aResponse.errorMsg) {
        aCallback.notifyError(aResponse.errorMsg);
        return false;
      }

      let networks = aResponse.networks;
      for (let i = 0; i < networks.length; i++) {
        let info = new MobileNetworkInfo();
        this._updateInfo(info, networks[i]);
        networks[i] = info;
      }

      aCallback.notifyGetNetworksSuccess(networks.length, networks);
      return false;
    }).bind(this));
  },

  stopNetworkScan: function(aCallback) {
    aCallback.notifyError(RIL.GECKO_ERROR_REQUEST_NOT_SUPPORTED);
    return false;
  },

  selectNetwork: function(aNetwork, aCallback) {
    if (!aNetwork ||
        isNaN(parseInt(aNetwork.mcc, 10)) ||
        isNaN(parseInt(aNetwork.mnc, 10))) {
      this._dispatchNotifyError(aCallback, RIL.GECKO_ERROR_INVALID_PARAMETER);
      return;
    }

    if (this._selectingNetwork) {
      this._dispatchNotifyError(aCallback, "AlreadySelectingANetwork");
      return;
    }

    let options = {mcc: aNetwork.mcc, mnc: aNetwork.mnc};
    this._selectingNetwork = options;
    this._radioInterface.sendWorkerMessage("selectNetwork", options,
                                           (function(aResponse) {
      this._selectingNetwork = null;
      if (aResponse.errorMsg) {
        aCallback.notifyError(aResponse.errorMsg);
        return false;
      }

      aCallback.notifySuccess();
      return false;
    }).bind(this));
  },

  selectNetworkAutomatically: function(aCallback) {
    if (this._selectingNetwork) {
      this._dispatchNotifyError(aCallback, "AlreadySelectingANetwork");
      return;
    }

    this._selectingNetwork = "automatic";
    this._radioInterface.sendWorkerMessage("selectNetworkAuto", null,
                                           (function(aResponse) {
      this._selectingNetwork = null;
      if (aResponse.errorMsg) {
        aCallback.notifyError(aResponse.errorMsg);
        return false;
      }

      aCallback.notifySuccess();
      return false;
    }).bind(this));
  },

  setPreferredNetworkType: function(aType, aCallback) {
    if (this.radioState !== Ci.nsIMobileConnection.MOBILE_RADIO_STATE_ENABLED) {
      this._dispatchNotifyError(aCallback, RIL.GECKO_ERROR_RADIO_NOT_AVAILABLE);
      return;
    }

    this._preferredNetworkType = aType;
    this._radioInterface.sendWorkerMessage("setPreferredNetworkType",
                                           {type: aType},
                                           (function(aResponse) {
      if (aResponse.errorMsg) {
        aCallback.notifyError(aResponse.errorMsg);
        return false;
      }

      aCallback.notifySuccess();
      return false;
    }).bind(this));
  },

  getPreferredNetworkType: function(aCallback) {
    if (this.radioState !== Ci.nsIMobileConnection.MOBILE_RADIO_STATE_ENABLED) {
      this._dispatchNotifyError(aCallback, RIL.GECKO_ERROR_RADIO_NOT_AVAILABLE);
      return;
    }

    this._radioInterface.sendWorkerMessage("getPreferredNetworkType", null,
                                           (function(aResponse) {
      if (aResponse.errorMsg) {
        aCallback.notifyError(aResponse.errorMsg);
        return false;
      }

      aCallback.notifyGetPreferredNetworkTypeSuccess(aResponse.type);
      return false;
    }).bind(this));
  },

  setRoamingPreference: function(aMode, aCallback) {
    this._radioInterface.sendWorkerMessage("setRoamingPreference",
                                           {mode: aMode},
                                           (function(aResponse) {
      if (aResponse.errorMsg) {
        aCallback.notifyError(aResponse.errorMsg);
        return false;
      }

      aCallback.notifySuccess();
      return false;
    }).bind(this));
  },

  getRoamingPreference: function(aCallback) {
    this._radioInterface.sendWorkerMessage("queryRoamingPreference", null,
                                           (function(aResponse) {
      if (aResponse.errorMsg) {
        aCallback.notifyError(aResponse.errorMsg);
        return false;
      }

      aCallback.notifyGetRoamingPreferenceSuccess(aResponse.mode);
      return false;
    }).bind(this));
  },

  setVoicePrivacyMode: function(aEnabled, aCallback) {
    this._radioInterface.sendWorkerMessage("setVoicePrivacyMode",
                                           {enabled: aEnabled},
                                           (function(aResponse) {
      if (aResponse.errorMsg) {
        aCallback.notifyError(aResponse.errorMsg);
        return false;
      }

      aCallback.notifySuccess();
      return false;
    }).bind(this));
  },

  getVoicePrivacyMode: function(aCallback) {
    this._radioInterface.sendWorkerMessage("queryVoicePrivacyMode", null,
                                           (function(aResponse) {
      if (aResponse.errorMsg) {
        aCallback.notifyError(aResponse.errorMsg);
        return false;
      }

      aCallback.notifySuccessWithBoolean(aResponse.enabled);
      return false;
    }).bind(this));
  },

  setCallForwarding: function(aAction, aReason, aNumber, aTimeSeconds,
                              aServiceClass, aCallback) {
    let options = {
      action: aAction,
      reason: aReason,
      number: aNumber,
      timeSeconds: aTimeSeconds,
      serviceClass: aServiceClass
    };

    this._radioInterface.sendWorkerMessage("setCallForward", options,
                                           (function(aResponse) {
      if (aResponse.errorMsg) {
        aCallback.notifyError(aResponse.errorMsg);
        return false;
      }

      this.notifyCFStateChanged(aResponse.action, aResponse.reason,
                                aResponse.number, aResponse.timeSeconds,
                                aResponse.serviceClass);
      aCallback.notifySuccess();
      return false;
    }).bind(this));
  },

  getCallForwarding: function(aReason, aServiceClass, aCallback) {
    this._radioInterface.sendWorkerMessage("queryCallForwardStatus",
                                           {reason: aReason, serviceClass: aServiceClass},
                                           (function(aResponse) {
      if (aResponse.errorMsg) {
        aCallback.notifyError(aResponse.errorMsg);
        return false;
      }

      let infos = this._rulesToCallForwardingOptions(aResponse.rules);
      aCallback.notifyGetCallForwardingSuccess(infos.length, infos);
      return false;
    }).bind(this));
  },

  setCallBarring: function(aProgram, aEnabled, aPassword, aServiceClass,
                           aCallback) {
    let options = {
      program: aProgram,
      enabled: aEnabled,
      password: aPassword,
      serviceClass: aServiceClass
    };

    this._radioInterface.sendWorkerMessage("setCallBarring", options,
                                           (function(aResponse) {
      if (aResponse.errorMsg) {
        aCallback.notifyError(aResponse.errorMsg);
        return false;
      }

      aCallback.notifySuccess();
      return false;
    }).bind(this));
  },

  getCallBarring: function(aProgram, aPassword, aServiceClass, aCallback) {
    let options = {
      program: aProgram,
      password: aPassword,
      serviceClass: aServiceClass
    };

    this._radioInterface.sendWorkerMessage("queryCallBarringStatus", options,
                                           (function(aResponse) {
      if (aResponse.errorMsg) {
        aCallback.notifyError(aResponse.errorMsg);
        return false;
      }

      aCallback.notifyGetCallBarringSuccess(aResponse.program,
                                            aResponse.enabled,
                                            aResponse.serviceClass);
      return false;
    }).bind(this));
  },

  changeCallBarringPassword: function(aPin, aNewPin, aCallback) {
    let options = {
      pin: aPin,
      newPin: aNewPin
    };

    this._radioInterface.sendWorkerMessage("changeCallBarringPassword", options,
                                           (function(aResponse) {
      if (aResponse.errorMsg) {
        aCallback.notifyError(aResponse.errorMsg);
        return false;
      }

      aCallback.notifySuccess();
      return false;
    }).bind(this));
  },

  setCallWaiting: function(aEnabled, aServiceClass, aCallback) {
    let options = {
      enabled: aEnabled,
      serviceClass: aServiceClass
    };

    this._radioInterface.sendWorkerMessage("setCallWaiting", options,
                                           (function(aResponse) {
      if (aResponse.errorMsg) {
        aCallback.notifyError(aResponse.errorMsg);
        return false;
      }

      aCallback.notifySuccess();
      return false;
    }).bind(this));
  },

  getCallWaiting: function(aCallback) {
    this._radioInterface.sendWorkerMessage("queryCallWaiting", null,
                                           (function(aResponse) {
      if (aResponse.errorMsg) {
        aCallback.notifyError(aResponse.errorMsg);
        return false;
      }

      aCallback.notifyGetCallWaitingSuccess(aResponse.serviceClass);
      return false;
    }).bind(this));
  },

  setCallingLineIdRestriction: function(aMode, aCallback) {
    if (!this._isValidClirMode(aMode)) {
      this._dispatchNotifyError(aCallback, RIL.GECKO_ERROR_INVALID_PARAMETER);
      return;
    }

    if (this.radioState !== Ci.nsIMobileConnection.MOBILE_RADIO_STATE_ENABLED) {
      this._dispatchNotifyError(aCallback, RIL.GECKO_ERROR_RADIO_NOT_AVAILABLE);
      return;
    }

    this._radioInterface.sendWorkerMessage("setCLIR", {clirMode: aMode},
                                           (function(aResponse) {
      if (aResponse.errorMsg) {
        aCallback.notifyError(aResponse.errorMsg);
        return false;
      }

      this.deliverListenerEvent("notifyClirModeChanged", [aResponse.mode]);
      aCallback.notifySuccess();
      return false;
    }).bind(this));
  },

  getCallingLineIdRestriction: function(aCallback) {
    if (this.radioState !== Ci.nsIMobileConnection.MOBILE_RADIO_STATE_ENABLED) {
      this._dispatchNotifyError(aCallback, RIL.GECKO_ERROR_RADIO_NOT_AVAILABLE);
      return;
    }

    this._radioInterface.sendWorkerMessage("getCLIR", null,
                                           (function(aResponse) {
      if (aResponse.errorMsg) {
        aCallback.notifyError(aResponse.errorMsg);
        return false;
      }

      aCallback.notifyGetClirStatusSuccess(aResponse.n, aResponse.m);
      return false;
    }).bind(this));
  },

  exitEmergencyCbMode: function(aCallback) {
    this._radioInterface.sendWorkerMessage("exitEmergencyCbMode", null,
                                           (function(aResponse) {
      if (aResponse.errorMsg) {
        aCallback.notifyError(aResponse.errorMsg);
        return false;
      }

      aCallback.notifySuccess();
      return false;
    }).bind(this));
  },

  setRadioEnabled: function(aEnabled, aCallback) {
    if (DEBUG) {
      this._debug("setRadioEnabled: " + aEnabled);
    }

    // Before sending a equest to |ril_worker.js|, we should check radioState.
    switch (this.radioState) {
      case Ci.nsIMobileConnection.MOBILE_RADIO_STATE_UNKNOWN:
      case Ci.nsIMobileConnection.MOBILE_RADIO_STATE_ENABLED:
      case Ci.nsIMobileConnection.MOBILE_RADIO_STATE_DISABLED:
        break;
      default:
        aCallback.notifyError("InvalidStateError");
        return;
    }

    let message = {
      msgType: "ExpectedRadioState",
      msgData: (aEnabled ? RIL.GECKO_RADIOSTATE_ENABLED :
                           RIL.GECKO_RADIOSTATE_DISABLED)
    };
    this.updateRadioState(message, aCallback);
  },

  getCellInfoList: function(aCallback) {
    this._radioInterface.sendWorkerMessage("getCellInfoList",
                                           null,
                                           function(aResponse) {
      if (aResponse.errorMsg) {
        aCallback.notifyGetCellInfoListFailed(aResponse.errorMsg);
        return;
      }

      let cellInfoList = [];
      let count = aResponse.result.length;
      for (let i = 0; i < count; i++) {
        let srcCellInfo = aResponse.result[i];
        let cellInfo;
        switch (srcCellInfo.type) {
          case RIL.CELL_INFO_TYPE_GSM:
            cellInfo = new GsmCellInfo(srcCellInfo);
            break;
          case RIL.CELL_INFO_TYPE_WCDMA:
            cellInfo = new WcdmaCellInfo(srcCellInfo);
            break;
          case RIL.CELL_INFO_TYPE_LTE:
            cellInfo = new LteCellInfo(srcCellInfo);
            break;
          case RIL.CELL_INFO_TYPE_CDMA:
            cellInfo = new CdmaCellInfo(srcCellInfo);
            break;
        }

        if (!cellInfo || !cellInfo.isValid()) {
          continue;
        }

        cellInfoList.push(cellInfo);
      }
      aCallback.notifyGetCellInfoList(cellInfoList.length, cellInfoList);
    }.bind(this));
  },

  getNeighboringCellIds: function(aCallback) {
    this._radioInterface.sendWorkerMessage("getNeighboringCellIds",
                                           null,
                                           function(aResponse) {
      if (aResponse.errorMsg) {
        aCallback.notifyGetNeighboringCellIdsFailed(aResponse.errorMsg);
        return;
      }

      let neighboringCellIds = [];
      let count = aResponse.result.length;
      for (let i = 0; i < count; i++) {
        let srcCellInfo = aResponse.result[i];
        let cellInfo = new NeighboringCellInfo(srcCellInfo);
        if (cellInfo && cellInfo.isValid()) {
          neighboringCellIds.push(cellInfo);
        }
      }
      aCallback.notifyGetNeighboringCellIds(neighboringCellIds.length,
                                            neighboringCellIds);

    }.bind(this));
  },

  getDeviceIdentities: function(aCallback) {
    let deviceId = new MobileDeviceIdentities();
    deviceId.imei = this._imei;
    deviceId.imeisv = this._imeisv;
    deviceId.esn = this._esn;
    deviceId.meid = this._meid;

    aCallback.notifyGetDeviceIdentitiesRequestSuccess(deviceId);
  },
};

function MobileConnectionService() {
  this._providers = [];

  let numClients = gRadioInterfaceLayer.numRadioInterfaces;
  for (let i = 0; i < numClients; i++) {
    let radioInterface = gRadioInterfaceLayer.getRadioInterface(i);
    let provider = new MobileConnectionProvider(i, radioInterface);
    this._providers.push(provider);
  }

  // A preference to determine if device support switching primary SIM feature.
  this._supportPrimarySimSwitch = Services.prefs.getBoolPref(
                                    kPrefSupportPrimarySimSwitch);

  this._updateSupportedNetworkTypes();

  Services.prefs.addObserver(kPrefRilDebuggingEnabled, this, false);
  Services.obs.addObserver(this, NS_NETWORK_ACTIVE_CHANGED_TOPIC_ID, false);
  Services.obs.addObserver(this, NS_DATA_CALL_ERROR_TOPIC_ID, false);
  Services.obs.addObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false);
  if (this.numItems == 2 && this._supportPrimarySimSwitch) {
    Services.obs.addObserver(this, TOPIC_MOZSETTINGS_CHANGED, false);
  }
  try {
    AmlEnabled =
        Services.prefs.getBoolPref(kPrefAmlEnabled) || false;
  } catch (e) {}

  debug("init complete");
}
MobileConnectionService.prototype = {
  classID: GONK_MOBILECONNECTIONSERVICE_CID,
  classInfo: XPCOMUtils.generateCI({classID: GONK_MOBILECONNECTIONSERVICE_CID,
                                    contractID: GONK_MOBILECONNECTIONSERVICE_CONTRACTID,
                                    classDescription: "MobileConnectionService",
                                    interfaces: [Ci.nsIGonkMobileConnectionService,
                                                 Ci.nsIMobileConnectionService],
                                    flags: Ci.nsIClassInfo.SINGLETON}),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIGonkMobileConnectionService,
                                         Ci.nsIMobileConnectionService,
                                         Ci.nsIObserver]),

  // An array of MobileConnectionProvider instances.
  _providers: null,

  // A flag to indicate if device support primary SIM switch.
  _supportPrimarySimSwitch: false,

  _shutdown: function() {
    Services.prefs.removeObserver(kPrefRilDebuggingEnabled, this);
    Services.obs.removeObserver(this, NS_NETWORK_ACTIVE_CHANGED_TOPIC_ID);
    Services.obs.removeObserver(this, NS_DATA_CALL_ERROR_TOPIC_ID);
    Services.obs.removeObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID);
    if (this.numItems == 2 && this._supportPrimarySimSwitch) {
      Services.obs.removeObserver(this, TOPIC_MOZSETTINGS_CHANGED);
    }
  },

  _updateDebugFlag: function() {
    try {
      DEBUG = RIL.DEBUG_RIL ||
              Services.prefs.getBoolPref(kPrefRilDebuggingEnabled);
    } catch (e) {}
  },

  _updateSupportedNetworkTypes: function() {
    // Update "persist.ril.[clientId].network_types" if these keys do not exist.
    for (let i = 0; i < this.numItems; i++) {
      let key = "persist.ril." + i + ".network_types";
      let supportedNetworkTypes = libcutils.property_get(key, "");

      if (!supportedNetworkTypes) {
        key = "ro.moz.ril." + i + ".network_types";
        libcutils.property_set("persist.ril." + i + ".network_types",
          // This should be customizable regarding default value.
          libcutils.property_get(key, i == 0 ? RIL.GECKO_SUPPORTED_NETWORK_TYPES_DEFAULT
                                             : RIL.GECKO_SUPPORTED_NETWORK_TYPES[0]));
      }
    }
  },

  _swapSupportedNetworkTypes: function(aNewPrimarySlot) {
    let roKeyPrimary = "ro.moz.ril.0.network_types";
    let roKeySlave = "ro.moz.ril.1.network_types";

    let persistKeyPrimary = "persist.ril." + aNewPrimarySlot + ".network_types";
    let persistTypesPrimary = libcutils.property_get(persistKeyPrimary, "");

    let persistKeySlave = "persist.ril." + (aNewPrimarySlot ^ 1) + ".network_types";
    let persistTypesSlave = libcutils.property_get(persistKeySlave, "");

    if (!persistTypesPrimary || !persistTypesSlave) {
      if (DEBUG) {
        debug("Don't expect empty values for both slots");
      }
      return;
    }

    // Update persist value for according to aNewPrimarySlot, Default value
    // should be customizable.
    libcutils.property_set(persistKeyPrimary, libcutils.property_get(roKeyPrimary,
      RIL.GECKO_SUPPORTED_NETWORK_TYPES_DEFAULT));
    libcutils.property_set(persistKeySlave, libcutils.property_get(roKeySlave,
      RIL.GECKO_SUPPORTED_NETWORK_TYPES[0]));
  },

  /**
   * nsIMobileConnectionService interface.
   */
  get numItems() {
    return this._providers.length;
  },

  getItemByServiceId: function(aServiceId) {
    let provider = this._providers[aServiceId];
    if (!provider) {
      throw Cr.NS_ERROR_UNEXPECTED;
    }

    return provider;
  },

  shutdownRadio: function(aCallback) {
    let shutdownPromses = [];
    try {
      shutdownPromses.push(this.getItemByServiceId(
        gDataCallManager.dataDefaultServiceId).shutdownRequest());
      for (let i = 0; i < this._providers.length; i++) {
        if (i == gDataCallManager.dataDefaultServiceId) {
          continue;
        }
        shutdownPromses.push(this.getItemByServiceId(i).shutdownRequest());
      }
      Promise.all(shutdownPromses).then(() => {
        if (aCallback) {
          aCallback.notifySuccess();
        }
      });
    } catch (aError) {
      if (aCallback) {
        aCallback.notifySuccess();
      }
    }
  },

  /**
   * nsIGonkMobileConnectionService interface.
   */
  notifyVoiceInfoChanged: function(aClientId, aVoiceInfo) {
    if (DEBUG) {
      debug("notifyVoiceInfoChanged for " + aClientId + ": " +
            JSON.stringify(aVoiceInfo));
    }

    this.getItemByServiceId(aClientId).updateVoiceInfo(aVoiceInfo);
  },

  notifyDeviceIdentities: function(aClientId, aMessage) {
    if (DEBUG) {
      debug("notifyDeviceIdentities for " + aClientId + ": " +
            JSON.stringify(aMessage));
    }

    this.getItemByServiceId(aClientId)._updateDeviceIdentities(aMessage.imei,
                                                               aMessage.imeisv,
                                                               aMessage.esn,
                                                               aMessage.meid);
  },

  notifyDataInfoChanged: function(aClientId, aDataInfo) {
    if (DEBUG) {
      debug("notifyDataInfoChanged for " + aClientId + ": " +
            JSON.stringify(aDataInfo));
    }

    this.getItemByServiceId(aClientId).updateDataInfo(aDataInfo);
  },

  notifyDataError: function(aClientId, aMessage) {
    if (DEBUG) {
      debug("notifyDataError for " + aClientId + ": " + aMessage);
    }

    this.getItemByServiceId(aClientId)
        .deliverListenerEvent("notifyDataError", [aMessage]);
  },

  notifyEmergencyCallbackModeChanged: function(aClientId, aActive, aTimeoutMs) {
    if (DEBUG) {
      debug("notifyEmergencyCbModeChanged for " + aClientId + ": " +
            JSON.stringify({active: aActive, timeoutMs: aTimeoutMs}));
    }

    let provider = this.getItemByServiceId(aClientId);
    provider.isInEmergencyCbMode = aActive;
    provider.deliverListenerEvent("notifyEmergencyCbModeChanged", [aActive, aTimeoutMs]);
  },

  notifyOtaStatusChanged: function(aClientId, aStatus) {
    if (DEBUG) {
      debug("notifyOtaStatusChanged for " + aClientId + ": " + aStatus);
    }

    this.getItemByServiceId(aClientId)
        .deliverListenerEvent("notifyOtaStatusChanged", [aStatus]);
  },

  notifyRadioStateChanged: function(aClientId, aRadioState) {
    if (DEBUG) {
      debug("notifyRadioStateChanged for " + aClientId + ": " + aRadioState);
    }

    let message = {
      msgType: "HardwareRadioState",
      msgData: aRadioState
    };
    this.getItemByServiceId(aClientId).updateRadioState(message);
  },

  notifyNetworkInfoChanged: function(aClientId, aNetworkInfo) {
    if (DEBUG) {
      debug("notifyNetworkInfoChanged for " + aClientId + ": " +
            JSON.stringify(aNetworkInfo));
    }

    let provider = this.getItemByServiceId(aClientId);

    let isVoiceUpdated = false;
    let isDataUpdated = false;
    let operatorMessage = aNetworkInfo[RIL.NETWORK_INFO_OPERATOR];
    let voiceMessage = aNetworkInfo[RIL.NETWORK_INFO_VOICE_REGISTRATION_STATE];
    let dataMessage = aNetworkInfo[RIL.NETWORK_INFO_DATA_REGISTRATION_STATE];
    let signalMessage = aNetworkInfo[RIL.NETWORK_INFO_SIGNAL];
    let selectionMessage = aNetworkInfo[RIL.NETWORK_INFO_NETWORK_SELECTION_MODE];

    // Batch the *InfoChanged messages together
    if (operatorMessage) {
      provider.updateOperatorInfo(operatorMessage, true);
    }

    if (voiceMessage) {
      provider.updateVoiceInfo(voiceMessage, true);
    }

    if (dataMessage) {
      provider.updateDataInfo(dataMessage, true);
    }

    if (provider.voice.roaming && !this._isOperatorConsideredRoaming(aClientId, provider) &&
        (this._isSameNamedOperators(aClientId, provider) || this._isOperatorConsideredNonRoaming(aClientId, provider))) {
      provider.voice.roaming = false;
    }

    if (provider.data.roaming && !this._isOperatorConsideredRoaming(aClientId, provider) &&
        (this._isSameNamedOperators(aClientId, provider) || this._isOperatorConsideredNonRoaming(aClientId, provider))) {
      provider.data.roaming = false;
    }

    if (selectionMessage) {
      this.notifyNetworkSelectModeChanged(aClientId, selectionMessage.mode);
    }

    if (voiceMessage || operatorMessage) {
      provider.deliverListenerEvent("notifyVoiceChanged");
    }

    if (dataMessage || operatorMessage) {
      provider.deliverListenerEvent("notifyDataChanged");
    }
  },

  _isOperatorConsideredRoaming: function(aClientId, provider) {
    let operatorNumeric = provider._operatorInfo.mcc + provider._operatorInfo.mnc;
    let numericArray = [];

    numericArray = gCustomizationInfo.getCustomizedValue(aClientId, "roamingOperatorStringArray", []);

    if (numericArray.length == 0 || operatorNumeric == 0) {
      return false;
    }

    // This will check operatorNumeric from the beginning to see if it contains the numeric.
    for (let numeric of numericArray) {
      if (operatorNumeric.indexOf(numeric) !== -1) {
        return true;
      }
    }
    return false;
  },

  _isSameNamedOperators: function(aClientId, provider) {
    let icc = gIccService.getIccByServiceId(aClientId);
    let iccInfo = icc ? icc.iccInfo : null;

    let equalsMcc = iccInfo.mcc == provider._operatorInfo.mcc;
    if (!equalsMcc) {
      return false;
    }
    let spn = iccInfo && iccInfo.spn;
    let onsl = provider._operatorInfo.longName;
    let onss = provider._operatorInfo.shortName;

    let equalsOnsl = (onsl != null) && (spn == onsl);
    let equalsOnss = (onss != null) && (spn == onss);

    return equalsMcc && (equalsOnsl || equalsOnss);
  },

  _isOperatorConsideredNonRoaming: function(aClientId, provider) {
    let operatorNumeric = provider._operatorInfo.mcc + provider._operatorInfo.mnc;
    let numericArray = [];

    numericArray = gCustomizationInfo.getCustomizedValue(aClientId, "nonRoamingOperatorStringArray", []);

    if (numericArray.length == 0 || operatorNumeric == 0) {
      return false;
    }

    // This will check operatorNumeric from the beginning to see if it contains the numeric.
    for (let numeric of numericArray) {
      if (operatorNumeric.indexOf(numeric) !== -1) {
        return true;
      }
    }
    return false;
  },

  notifySignalStrengthChanged: function(aClientId, aSignal) {
    if (DEBUG) {
      debug("notifySignalStrengthChanged for " + aClientId + ": " +
            JSON.stringify(aSignal));
    }

    this.getItemByServiceId(aClientId).updateSignalInfo(aSignal);
  },

  notifyOperatorChanged: function(aClientId, aOperator) {
    if (DEBUG) {
      debug("notifyOperatorChanged for " + aClientId + ": " +
            JSON.stringify(aOperator));
    }

    this.getItemByServiceId(aClientId).updateOperatorInfo(aOperator);
  },

  notifyNetworkSelectModeChanged: function(aClientId, aMode) {
    if (DEBUG) {
      debug("notifyNetworkSelectModeChanged for " + aClientId + ": " + aMode);
    }

    let provider = this.getItemByServiceId(aClientId);
    if (provider.networkSelectionMode === aMode) {
      return;
    }

    provider.networkSelectionMode = aMode;
    provider.deliverListenerEvent("notifyNetworkSelectionModeChanged");
  },

  notifySpnAvailable: function(aClientId) {
    if (DEBUG) {
      debug("notifySpnAvailable for " + aClientId);
    }

    let provider = this.getItemByServiceId(aClientId);

    // Update voice roaming state
    provider.updateVoiceInfo({});

    // Update data roaming state
    provider.updateDataInfo({});
  },

  notifyLastHomeNetworkChanged: function(aClientId, aNetwork) {
    if (DEBUG) {
      debug("notifyLastHomeNetworkChanged for " + aClientId + ": " + aNetwork);
    }

    let provider = this.getItemByServiceId(aClientId);
    if (provider.lastKnownHomeNetwork === aNetwork) {
      return;
    }

    provider.lastKnownHomeNetwork = aNetwork;
    provider.deliverListenerEvent("notifyLastKnownHomeNetworkChanged");
  },

  notifyCFStateChanged: function(aClientId, aAction, aReason, aNumber,
                                 aTimeSeconds, aServiceClass) {
    if (DEBUG) {
      debug("notifyCFStateChanged for " + aClientId);
    }

    let provider = this.getItemByServiceId(aClientId);
    provider.notifyCFStateChanged(aAction, aReason, aNumber, aTimeSeconds,
                                  aServiceClass);
  },

  notifyCdmaInfoRecDisplay: function(aClientId, aDisplay) {
    gMobileConnectionMessenger.notifyCdmaInfoRecDisplay(aClientId, aDisplay);
  },

  notifyCdmaInfoRecCalledPartyNumber: function(aClientId, aType, aPlan, aNumber,
                                               aPi, aSi) {
    gMobileConnectionMessenger
      .notifyCdmaInfoRecCalledPartyNumber(aClientId, aType, aPlan, aNumber,
                                          aPi, aSi);
  },

  notifyCdmaInfoRecCallingPartyNumber: function(aClientId, aType, aPlan, aNumber,
                                                aPi, aSi) {
    gMobileConnectionMessenger
      .notifyCdmaInfoRecCallingPartyNumber(aClientId, aType, aPlan, aNumber,
                                           aPi, aSi);
  },

  notifyCdmaInfoRecConnectedPartyNumber: function(aClientId, aType, aPlan, aNumber,
                                                  aPi, aSi) {
    gMobileConnectionMessenger
      .notifyCdmaInfoRecConnectedPartyNumber(aClientId, aType, aPlan, aNumber,
                                             aPi, aSi);
  },

  notifyCdmaInfoRecSignal: function(aClientId, aType, aAlertPitch, aSignal){
    gMobileConnectionMessenger
      .notifyCdmaInfoRecSignal(aClientId, aType, aAlertPitch, aSignal);
  },

  notifyCdmaInfoRecRedirectingNumber: function(aClientId, aType, aPlan, aNumber,
                                               aPi, aSi, aReason) {
    gMobileConnectionMessenger
      .notifyCdmaInfoRecRedirectingNumber(aClientId, aType, aPlan, aNumber,
                                          aPi, aSi, aReason);
  },

  notifyCdmaInfoRecLineControl: function(aClientId, aPolarityIncluded, aToggle,
                                         aReverse, aPowerDenial) {
    gMobileConnectionMessenger
      .notifyCdmaInfoRecLineControl(aClientId, aPolarityIncluded, aToggle,
                                    aReverse, aPowerDenial);
  },

  notifyCdmaInfoRecClir: function(aClientId, aCause) {
    gMobileConnectionMessenger.notifyCdmaInfoRecClir(aClientId, aCause);
  },

  notifyCdmaInfoRecAudioControl: function(aClientId, aUpLink, aDownLink) {
    gMobileConnectionMessenger
      .notifyCdmaInfoRecAudioControl(aClientId, aUpLink, aDownLink);
  },

  notifyDeviceIdentitiesChanged: function(aClientId, aImei, aImeisv,
                                          aEsn, aMeid) {
    this.getItemByServiceId(aClientId)
      .notifyDeviceIdentitiesChanged(aImei, aImeisv, aEsn, aMeid);
  },

  notifyModemRestart: function(aClientId, aReason) {
    if (DEBUG) {
      this._debug("notifyModemRestart: clientId=" + aClientId + ", reason=" + aReason);
    }

    let provider = this.getItemByServiceId(aClientId);
    if (provider._expectedRadioState === RIL.GECKO_RADIOSTATE_ENABLED) {
      provider._needRestorePreferredNetworkType = true;
    }

    provider.deliverListenerEvent("notifyModemRestart", [aReason]);
  },

  /**
   * nsIObserver interface.
   */
  observe: function(aSubject, aTopic, aData) {
    switch (aTopic) {
      case NS_NETWORK_ACTIVE_CHANGED_TOPIC_ID:
        for (let i = 0; i < this.numItems; i++) {
          let provider = this._providers[i];
          // Update connected flag only.
          provider.updateDataInfo({});
        }
        break;
      case NS_DATA_CALL_ERROR_TOPIC_ID:
        try {
          if (aSubject instanceof Ci.nsIRilNetworkInfo) {
            let rilInfo = aSubject.QueryInterface(Ci.nsIRilNetworkInfo);
            this.notifyDataError(rilInfo.serviceId, aData);
          }
        } catch (e) {}
        break;
      case NS_PREFBRANCH_PREFCHANGE_TOPIC_ID:
        if (aData === kPrefRilDebuggingEnabled) {
          this._updateDebugFlag();
        }
        break;
      case NS_XPCOM_SHUTDOWN_OBSERVER_ID:
        this._shutdown();
        break;
      case TOPIC_MOZSETTINGS_CHANGED:
        if ("wrappedJSObject" in aSubject) {
          aSubject = aSubject.wrappedJSObject;
        }
        this.handle(aSubject.key, aSubject.value);
        break;
    }
  },

  handle: function(aName, aResult) {
    switch (aName) {
      case kPrefRilDataServiceId:
        aResult = aResult || 0;
        if (DEBUG) {
          debug("'ril.data.defaultServiceId' is now " + aResult);
        }
        this._swapSupportedNetworkTypes(aResult);
        break;
    }
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([MobileConnectionService]);
