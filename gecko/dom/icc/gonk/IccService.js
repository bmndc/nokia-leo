/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyGetter(this, "RIL", function () {
  let obj = Cu.import("resource://gre/modules/ril_consts.js", null);
  return obj;
});

const GONK_ICCSERVICE_CONTRACTID = "@mozilla.org/icc/gonkiccservice;1";
const GONK_ICCSERVICE_CID = Components.ID("{df854256-9554-11e4-a16c-c39e8d106c26}");

const NS_XPCOM_SHUTDOWN_OBSERVER_ID      = "xpcom-shutdown";
const NS_PREFBRANCH_PREFCHANGE_TOPIC_ID  = "nsPref:changed";

const kPrefRilDebuggingEnabled = "ril.debugging.enabled";
const kPrefRilNumRadioInterfaces = "ril.numRadioInterfaces";

const ICC_MAX_LINEAR_FIXED_RECORDS = 0xfe;

const CARDCONTACT_TYPE_ADN = 0;

const CONTACT_OPERATION_ADD = 0;
const CONTACT_OPERATION_REMOVE = 1;
const CONTACT_OPERATION_UPDATE = 2;

XPCOMUtils.defineLazyServiceGetter(this, "gRadioInterfaceLayer",
                                   "@mozilla.org/ril;1",
                                   "nsIRadioInterfaceLayer");

XPCOMUtils.defineLazyServiceGetter(this, "gMobileConnectionService",
                                   "@mozilla.org/mobileconnection/mobileconnectionservice;1",
                                   "nsIGonkMobileConnectionService");

XPCOMUtils.defineLazyServiceGetter(this, "gIccMessenger",
                                   "@mozilla.org/ril/system-messenger-helper;1",
                                   "nsIIccMessenger");

XPCOMUtils.defineLazyServiceGetter(this, "gStkCmdFactory",
                                   "@mozilla.org/icc/stkcmdfactory;1",
                                   "nsIStkCmdFactory");

var DEBUG = RIL.DEBUG_RIL;
function debug(s) {
  dump("IccService: " + s);
}

function IccInfo() {}
IccInfo.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIIccInfo]),

  // nsIIccInfo

  iccType: null,
  iccid: null,
  mcc: null,
  mnc: null,
  spn: null,
  imsi: null,
  gid1: null,
  gid2: null,
  isDisplayNetworkNameRequired: false,
  isDisplaySpnRequired: false
};

function GsmIccInfo() {}
GsmIccInfo.prototype = {
  __proto__: IccInfo.prototype,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIGsmIccInfo,
                                         Ci.nsIIccInfo]),

  // nsIGsmIccInfo

  msisdn: null
};

function CdmaIccInfo() {}
CdmaIccInfo.prototype = {
  __proto__: IccInfo.prototype,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsICdmaIccInfo,
                                         Ci.nsIIccInfo]),

  // nsICdmaIccInfo

  mdn: null,
  prlVersion: 0
};

function IsimIccInfo() {}
IsimIccInfo.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIIsimIccInfo]),

  // nsIIsimIccInfo

  impi: null,
  impus: [],
};

function IccContact(aContact) {
  this.id = aContact.contactId || null;
  this._names = [];
  this._numbers = [];
  this._emails = [];

  this._names.push(aContact.alphaId);
  this._numbers.push(aContact.number);

  let anrLen = aContact.anr ? aContact.anr.length : 0;
  for (let i = 0; i < anrLen; i++) {
    this._numbers.push(aContact.anr[i]);
  }

  if (aContact.email) {
    this._emails.push(aContact.email);
  }
}
IccContact.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIIccContact]),

  _names: null,
  _numbers: null,
  _emails: null,

  // nsIIccContact

  id: null,

  getNames: function(aCount) {
    if (!this._names) {
      if (aCount) {
        aCount.value = 0;
      }
      return null;
    }

    if (aCount) {
      aCount.value = this._names.length;
    }

    return this._names.slice();
  },

  getNumbers: function(aCount) {
    if (!this._numbers) {
      if (aCount) {
        aCount.value = 0;
      }
      return null;
    }

    if (aCount) {
      aCount.value = this._numbers.length;
    }

    return this._numbers.slice();
  },

  getEmails: function(aCount) {
    if (!this._emails) {
      if (aCount) {
        aCount.value = 0;
      }
      return null;
    }

    if (aCount) {
      aCount.value = this._emails.length;
    }

    return this._emails.slice();
  },
};

function IccService() {
  this._iccs = [];

  let numClients = gRadioInterfaceLayer.numRadioInterfaces;
  for (let i = 0; i < numClients; i++) {
    this._iccs.push(new Icc(i));
  }

  this._updateDebugFlag();

  Services.prefs.addObserver(kPrefRilDebuggingEnabled, this, false);
  Services.obs.addObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false);
}
IccService.prototype = {
  classID: GONK_ICCSERVICE_CID,

  classInfo: XPCOMUtils.generateCI({classID: GONK_ICCSERVICE_CID,
                                    contractID: GONK_ICCSERVICE_CONTRACTID,
                                    classDescription: "IccService",
                                    interfaces: [Ci.nsIIccService,
                                                 Ci.nsIGonkIccService],
                                    flags: Ci.nsIClassInfo.SINGLETON}),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIIccService,
                                         Ci.nsIGonkIccService,
                                         Ci.nsIObserver]),

  // An array of Icc instances.
  _iccs: null,

  _updateDebugFlag: function() {
    try {
      DEBUG = DEBUG ||
              Services.prefs.getBoolPref(kPrefRilDebuggingEnabled);
    } catch (e) {}
  },

  /**
   * nsIIccService interface.
   */
  getIccByServiceId: function(aServiceId) {
    let icc = this._iccs[aServiceId];
    if (!icc) {
      throw Cr.NS_ERROR_UNEXPECTED;
    }

    return icc;
  },

  /**
   * nsIGonkIccService interface.
   */
  notifyStkCommand: function(aServiceId, aStkcommand) {
    if (DEBUG) {
      debug("notifyStkCommand for service Id: " + aServiceId);
    }

    let icc = this.getIccByServiceId(aServiceId);

    if (!icc.iccInfo || !icc.iccInfo.iccid) {
      debug("Warning: got STK command when iccid is invalid.");
      return;
    }

    gIccMessenger.notifyStkProactiveCommand(icc.iccInfo.iccid, aStkcommand);

    icc._deliverListenerEvent("notifyStkCommand", [aStkcommand]);
  },

  notifyStkSessionEnd: function(aServiceId) {
    if (DEBUG) {
      debug("notifyStkSessionEnd for service Id: " + aServiceId);
    }

    this.getIccByServiceId(aServiceId)
      ._deliverListenerEvent("notifyStkSessionEnd");
  },

  notifyCardStateChanged: function(aServiceId, aCardState) {
    if (DEBUG) {
      debug("notifyCardStateChanged for service Id: " + aServiceId +
            ", CardState: " + aCardState);
    }

    this.getIccByServiceId(aServiceId)._updateCardState(aCardState);
  },

  notifyIccInfoChanged: function(aServiceId, aIccInfo) {
    if (DEBUG) {
      debug("notifyIccInfoChanged for service Id: " + aServiceId +
            ", IccInfo: " + JSON.stringify(aIccInfo));
    }

    this.getIccByServiceId(aServiceId)._updateIccInfo(aIccInfo);
  },

  notifyImsiChanged: function(aServiceId, aImsi) {
    if (DEBUG) {
      debug("notifyImsiChanged for service Id: " + aServiceId +
            ", Imsi: " + aImsi);
    }

    let icc = this.getIccByServiceId(aServiceId);
    icc.imsi = aImsi || null;
    if (icc.iccInfo) {
      icc.iccInfo.imsi = icc.imsi;
    }
    icc._deliverListenerEvent("notifyIccInfoChanged");
  },

  notifyIsimInfoChanged: function(aServiceId, aIsimInfo) {
    if (DEBUG) {
      debug("notifyIsimInfoChanged for service Id: " + aServiceId +
            ", IsimInfo: " + JSON.stringify(aIsimInfo));
    }

    this.getIccByServiceId(aServiceId)._updateIsimIccInfo(aIsimInfo);
  },

  /**
   * nsIObserver interface.
   */
  observe: function(aSubject, aTopic, aData) {
    switch (aTopic) {
      case NS_PREFBRANCH_PREFCHANGE_TOPIC_ID:
        if (aData === kPrefRilDebuggingEnabled) {
          this._updateDebugFlag();
        }
        break;
      case NS_XPCOM_SHUTDOWN_OBSERVER_ID:
        Services.prefs.removeObserver(kPrefRilDebuggingEnabled, this);
        Services.obs.removeObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID);
        break;
    }
  }
};

function Icc(aClientId) {
  this._clientId = aClientId;
  this._radioInterface = gRadioInterfaceLayer.getRadioInterface(aClientId);
  this._listeners = [];
  this._totalContacts = -1;
  this._totalRecords = -1;
}
Icc.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIIcc]),

  _clientId: 0,
  _radioInterface: null,
  _listeners: null,
  _totalRecords: null,
  _totalContacts: null,

  _updateCardState: function(aCardState) {
    if (this.cardState != aCardState) {
      this.cardState = aCardState;
      //Reread the total contact records
      if(this.cardState === RIL.GECKO_CARDSTATE_READY) {
        this.getMaxContactCount(CARDCONTACT_TYPE_ADN);
      }
    }

    this._deliverListenerEvent("notifyCardStateChanged");
  },

  // An utility function to copy objects.
  _updateInfo: function(aSrcInfo, aDestInfo) {
    for (let key in aSrcInfo) {
      aDestInfo[key] = aSrcInfo[key];
    }
  },

  /**
   * A utility function to compare objects. The srcInfo may contain
   * "rilMessageType", should ignore it.
   */
  _isInfoChanged: function(srcInfo, destInfo) {
    if (!destInfo) {
      return true;
    }

    for (let key in srcInfo) {
      if (key === "rilMessageType") {
        continue;
      }
      if (srcInfo[key] !== destInfo[key]) {
        return true;
      }
    }

    return false;
  },

  /**
   * We need to consider below cases when update iccInfo:
   * 1. Should clear iccInfo to null if there is no card detected.
   * 2. Need to create corresponding object based on iccType.
   */
  _updateIccInfo: function(aIccInfo) {
    let oldSpn = this.iccInfo ? this.iccInfo.spn : null;

    // Card is not detected, clear iccInfo to null.
    if (!aIccInfo || !aIccInfo.iccid) {
      if (this.iccInfo) {
        if (DEBUG) {
          debug("Card is not detected, clear iccInfo to null.");
        }
        this.imsi = null;
        this.iccInfo = null;
        this._deliverListenerEvent("notifyIccInfoChanged");
      }
      return;
    }

    if (!this._isInfoChanged(aIccInfo, this.iccInfo)) {
      return;
    }

    // If iccInfo is null, new corresponding object based on iccType.
    if (!this.iccInfo ||
        this.iccInfo.iccType != aIccInfo.iccType) {
      if (aIccInfo.iccType === "ruim" || aIccInfo.iccType === "csim") {
        this.iccInfo = new CdmaIccInfo();
      } else if (aIccInfo.iccType === "sim" || aIccInfo.iccType === "usim") {
        this.iccInfo = new GsmIccInfo();
      } else {
        this.iccInfo = new IccInfo();
      }
    }

    this._updateInfo(aIccInfo, this.iccInfo);
    this.iccInfo.imsi = this.imsi || null;

    this._deliverListenerEvent("notifyIccInfoChanged");

    // Update lastKnownSimMcc.
    if (aIccInfo.mcc) {
      try {
        Services.prefs.setCharPref("ril.lastKnownSimMcc",
                                   aIccInfo.mcc.toString());
      } catch (e) {}
    }

    // Update lastKnownHomeNetwork.
    if (aIccInfo.mcc && aIccInfo.mnc) {
      let lastKnownHomeNetwork = aIccInfo.mcc + "-" + aIccInfo.mnc;
      // Append spn information if available.
      if (aIccInfo.spn) {
        lastKnownHomeNetwork += "-" + aIccInfo.spn;
      }

      gMobileConnectionService.notifyLastHomeNetworkChanged(this._clientId,
                                                            lastKnownHomeNetwork);
    }

    // If spn becomes available, we should check roaming again.
    if (!oldSpn && aIccInfo.spn) {
      gMobileConnectionService.notifySpnAvailable(this._clientId);
    }

    //BDC zhangwp 20190709 add for get GID. begin
    debug("ready to update gid = " + aResponse.gid1);
    this._radioInterface.sendWorkerMessage("getGID1",
                                           null,
                                           (aResponse) => {
      if(aResponse.gid1) {
        debug("update gid = " + aResponse.gid1);
        this gid = aResponse.gid1;
      }
    });
    //BDC zhangwp 20190709 add for get GID. end
  },

  _deliverListenerEvent: function(aName, aArgs) {
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
          debug("listener for " + aName + " threw an exception: " + e);
        }
      }
    }
  },

  _modifyCardLock: function(aOperation, aOptions, aCallback) {
    this._radioInterface.sendWorkerMessage(aOperation,
                                           aOptions,
                                           (aResponse) => {
      if (aResponse.errorMsg) {
        let retryCount =
          (aResponse.retryCount !== undefined) ? aResponse.retryCount : -1;
        aCallback.notifyCardLockError(aResponse.errorMsg, retryCount);
        return;
      }

      aCallback.notifySuccess();
    });
  },

  /**
   * Helper to match the MVNO pattern with IMSI.
   *
   * Note: Characters 'x' and 'X' in MVNO are skipped and not compared.
   *       E.g., if the aMvnoData passed is '310260x10xxxxxx', then the
   *       function returns true only if imsi has the same first 6 digits, 8th
   *       and 9th digit.
   *
   * @param aMvnoData
   *        MVNO pattern.
   * @param aImsi
   *        IMSI of this ICC.
   *
   * @return true if matched.
   */
  _isImsiMatches: function(aMvnoData, aImsi) {
    // This should not be an error, but a mismatch.
    if (aMvnoData.length > aImsi.length) {
      return false;
    }

    for (let i = 0; i < aMvnoData.length; i++) {
      let c = aMvnoData[i];
      if ((c !== 'x') && (c !== 'X') && (c !== aImsi[i])) {
        return false;
      }
    }
    return true;
  },

  /**
   * nsIIsimIccInfo interface.
   */
  isimInfo: null,
  _updateIsimIccInfo: function(aIsimInfo) {
    if (!this.isimInfo) {
      this.isimInfo = new IsimIccInfo();
    }

    this.isimInfo.impi = aIsimInfo.impi;
    this.isimInfo.impus = aIsimInfo.impus.slice();

    this._deliverListenerEvent("notifyIsimInfoChanged");
  },

  /**
   * nsIIcc interface.
   */
  iccInfo: null,
  cardState: Ci.nsIIcc.CARD_STATE_UNKNOWN,
  imsi: null,
  //BDC zhangwp 20190709 add for get GID
  gid: null,

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

  getCardLockEnabled: function(aLockType, aCallback) {
    this._radioInterface.sendWorkerMessage("iccGetCardLockEnabled",
                                           { lockType: aLockType },
                                           (aResponse) => {
      if (aResponse.errorMsg) {
        aCallback.notifyError(aResponse.errorMsg);
        return;
      }

      aCallback.notifySuccessWithBoolean(aResponse.enabled);
    });
  },

  unlockCardLock: function(aLockType, aPassword, aNewPin, aCallback) {
    this._modifyCardLock("iccUnlockCardLock",
                         { lockType: aLockType,
                           password: aPassword,
                           newPin: aNewPin },
                         aCallback);
  },

  setCardLockEnabled: function(aLockType, aPassword, aEnabled, aCallback) {
    this._modifyCardLock("iccSetCardLockEnabled",
                         { lockType: aLockType,
                           password: aPassword,
                           enabled: aEnabled },
                         aCallback);
  },

  changeCardLockPassword: function(aLockType, aPassword, aNewPassword, aCallback) {
    this._modifyCardLock("iccChangeCardLockPassword",
                         { lockType: aLockType,
                           password: aPassword,
                           newPassword: aNewPassword, },
                         aCallback);
  },

  getCardLockRetryCount: function(aLockType, aCallback) {
    this._radioInterface.sendWorkerMessage("iccGetCardLockRetryCount",
                                           { lockType: aLockType },
                                           (aResponse) => {
      if (aResponse.errorMsg) {
        aCallback.notifyError(aResponse.errorMsg);
        return;
      }

      aCallback.notifyGetCardLockRetryCount(aResponse.retryCount);
    });
  },

  matchMvno: function(aMvnoType, aMvnoData, aCallback) {
    if (!aMvnoData) {
      aCallback.notifyError(RIL.GECKO_ERROR_INVALID_PARAMETER);
      return;
    }

    switch (aMvnoType) {
      case Ci.nsIIcc.CARD_MVNO_TYPE_IMSI:
        let imsi = this.imsi;
        if (!imsi) {
          aCallback.notifyError(RIL.GECKO_ERROR_GENERIC_FAILURE);
          break;
        }
        aCallback.notifySuccessWithBoolean(
          this._isImsiMatches(aMvnoData, imsi));
        break;
      case Ci.nsIIcc.CARD_MVNO_TYPE_SPN:
        let spn = this.iccInfo && this.iccInfo.spn;
        if (!spn) {
          aCallback.notifyError(RIL.GECKO_ERROR_GENERIC_FAILURE);
          break;
        }
        aCallback.notifySuccessWithBoolean(spn == aMvnoData);
        break;
      case Ci.nsIIcc.CARD_MVNO_TYPE_GID:
        this._radioInterface.sendWorkerMessage("getGID1",
                                               null,
                                               (aResponse) => {
          let gid = aResponse.gid1;
          let mvnoDataLength = aMvnoData.length;

          if (!gid) {
            aCallback.notifyError(RIL.GECKO_ERROR_GENERIC_FAILURE);
          } else if (mvnoDataLength > gid.length) {
            aCallback.notifySuccessWithBoolean(false);
          } else {
            let result =
              gid.substring(0, mvnoDataLength).toLowerCase() ==
              aMvnoData.toLowerCase();
            aCallback.notifySuccessWithBoolean(result);
          }
        });
        break;
      default:
        aCallback.notifyError(RIL.GECKO_ERROR_MODE_NOT_SUPPORTED);
        break;
    }
  },

  getServiceStateEnabled: function(aService, aCallback) {
    this._radioInterface.sendWorkerMessage("getIccServiceState",
                                           { service: aService },
                                           (aResponse) => {
      if (aResponse.errorMsg) {
        aCallback.notifyError(aResponse.errorMsg);
        return;
      }

      aCallback.notifySuccessWithBoolean(aResponse.result);
    });
  },

  iccOpenChannel: function(aAid, aCallback) {
    this._radioInterface.sendWorkerMessage("iccOpenChannel",
                                           { aid: aAid },
                                           (aResponse) => {
      if (aResponse.errorMsg) {
        aCallback.notifyError(aResponse.errorMsg);
        return;
      }

      aCallback.notifyOpenChannelSuccess(aResponse.channel);
    });
  },

  iccExchangeAPDU: function(aChannel, aCla, aIns, aP1, aP2, aP3, aData, aCallback) {
    if (!aData) {
      if (DEBUG) debug('data is not set , aP3 : ' + aP3);
    }

    let apdu = {
      cla: aCla,
      command: aIns,
      p1: aP1,
      p2: aP2,
      p3: aP3,
      data: aData
    };

    this._radioInterface.sendWorkerMessage("iccExchangeAPDU",
                                           { channel: aChannel, apdu: apdu },
                                           (aResponse) => {
      if (aResponse.errorMsg) {
        aCallback.notifyError(aResponse.errorMsg);
        return;
      }

      aCallback.notifyExchangeAPDUResponse(aResponse.sw1,
                                           aResponse.sw2,
                                           aResponse.simResponse);
    });
  },

  iccCloseChannel: function(aChannel, aCallback) {
    this._radioInterface.sendWorkerMessage("iccCloseChannel",
                                           { channel: aChannel },
                                           (aResponse) => {
      if (aResponse.errorMsg) {
        aCallback.notifyError(aResponse.errorMsg);
        return;
      }

      aCallback.notifyCloseChannelSuccess();
    });
  },

  sendStkResponse: function(aCommand, aResponse) {
    let response = gStkCmdFactory.createResponseMessage(aResponse);
    response.command = gStkCmdFactory.createCommandMessage(aCommand);
    this._radioInterface.sendWorkerMessage("sendStkTerminalResponse", response);
  },

  sendStkMenuSelection: function(aItemIdentifier, aHelpRequested) {
    this._radioInterface
      .sendWorkerMessage("sendStkMenuSelection", {
        itemIdentifier: aItemIdentifier,
        helpRequested: aHelpRequested
      });
  },

  sendStkTimerExpiration: function(aTimerId, aTimerValue) {
    this._radioInterface
      .sendWorkerMessage("sendStkTimerExpiration",{
        timer: {
          timerId: aTimerId,
          timerValue: aTimerValue
        }
      });
  },

  sendStkEventDownload: function(aEvent) {
    this._radioInterface
      .sendWorkerMessage("sendStkEventDownload",
                         { event: gStkCmdFactory.createEventMessage(aEvent) });
  },

  readContacts: function(aContactType, aCallback) {
    this._radioInterface
      .sendWorkerMessage("readICCContacts",
                         { contactType: aContactType },
                         (aResponse) => {
      if (aResponse.errorMsg) {
        aCallback.notifyError(aResponse.errorMsg);
        return;
      }

      let iccContacts = [];

      aResponse.contacts.forEach(c => iccContacts.push(new IccContact(c)));
      if(aContactType == CARDCONTACT_TYPE_ADN) {
        this._totalContacts = iccContacts.length;
        if (DEBUG) {
          debug("_totalContacts = " + this._totalContacts);
        }
      }
      aCallback.notifyRetrievedIccContacts(iccContacts, iccContacts.length);
    });
  },

  getUserOperation: function(aContact) {
    let iccid = this.iccInfo.iccid;
    function isExistContact(aContact) {
      let isExist = false;
      if (typeof aContact.contactId === "string" &&
        aContact.contactId.startsWith(iccid)) {
        let recordIndex = aContact.contactId.substring(iccid.length);
        aContact.pbrIndex = Math.floor(recordIndex / ICC_MAX_LINEAR_FIXED_RECORDS);
        aContact.recordId = recordIndex % ICC_MAX_LINEAR_FIXED_RECORDS;
        isExist = aContact.recordId > 0 && aContact.recordId < 0xff;
      }
      return isExist;
    }

    if(isExistContact(aContact)) {
      if (aContact.alphaId ){
        return CONTACT_OPERATION_UPDATE;
      }
      if (aContact.number) {
        return CONTACT_OPERATION_UPDATE;
      }
      return CONTACT_OPERATION_REMOVE;
    }
    return CONTACT_OPERATION_ADD;
  },

  getMaxContactCount: function(aContactType) {
    this._radioInterface.sendWorkerMessage("getMaxContactCount",
      {contactType : aContactType},
      (aResponse) => {
        if (aResponse.errorMsg) {
          if (DEBUG) {
            debug("getMaxContactCount failed " + errorMsg);
          }
          return;
        }
        this._totalRecords = aResponse.totalRecords;
        if (DEBUG) {
          debug("_totalRecords = " + this._totalRecords);
        }
    })
  },

  updateContact: function(aContactType, aContact, aPin2, aCallback) {
    let iccContact = { contactId: aContact.id };
    let count = { value: 0 };
    let names = aContact.getNames(count);
    if (count.value > 0) {
      iccContact.alphaId = names[0];
    }

    let numbers = aContact.getNumbers(count);
    if (count.value > 0) {
      iccContact.number = numbers[0];

      let anrArray = numbers.slice(1);
      let length = anrArray.length;
      if (length > 0) {
        iccContact.anr = [];
        for (let i = 0; i < length; i++) {
          iccContact.anr.push(anrArray[i]);
        }
      }
    }

    let emails = aContact.getEmails(count);
    if (count.value > 0) {
      iccContact.email = emails[0];
    }

    let operation;
    if(aContactType == CARDCONTACT_TYPE_ADN) {
      operation = this.getUserOperation(iccContact);
      if(DEBUG) {
        debug("User operation is" + operation);
      }
      if(operation === CONTACT_OPERATION_ADD &&
        this._totalRecords != -1 &&
        this._totalContacts != -1 &&
        this._totalContacts >= this._totalRecords) {
        aCallback.notifyError(RIL.CONTACT_ERR_NO_FREE_RECORD_FOUND);
        return;
      }
    }

    this._radioInterface
      .sendWorkerMessage("updateICCContact",
                         { contactType: aContactType,
                           contact: iccContact,
                           pin2: aPin2 },
                         (aResponse) => {
      if (aResponse.errorMsg) {
        aCallback.notifyError(aResponse.errorMsg);
        return;
      }
      switch (operation) {
        case CONTACT_OPERATION_ADD:
          if(this._totalContacts != -1) {
            this._totalContacts += 1;
          }
          break;
        case CONTACT_OPERATION_REMOVE:
          if(this._totalContacts != -1) {
            this._totalContacts -= 1;
          }
          break;
        default:
          break;
      }
      if(DEBUG) {
        debug("current _totalContacts =" + this._totalContacts)
      }
      aCallback.notifyUpdatedIccContact(new IccContact(aResponse.contact));
    });
  },

  getIccAuthentication: function(aAppType, aAuthType, aData, aCallback) {
    this._radioInterface.sendWorkerMessage("getIccAuthentication",
                                           { appType: aAppType,
                                             authType: aAuthType,
                                             data: aData },
                                           (aResponse) => {
      if (aResponse.errorMsg) {
        aCallback.notifyError(aResponse.errorMsg);
        return;
      }

      aCallback.notifyAuthResponse(aResponse.responseData);
    });
  },
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([IccService]);
