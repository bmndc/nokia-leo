/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyGetter(this, "RIL", function () {
  let obj = Cu.import("resource://gre/modules/ril_consts.js", null);
  return obj;
});

XPCOMUtils.defineLazyGetter(this, "gRadioInterfaceLayer", function() {
  let ril = { numRadioInterfaces: 0 };
  try {
    ril = Cc["@mozilla.org/ril;1"].getService(Ci.nsIRadioInterfaceLayer);
  } catch(e) {}
  return ril;
});

const GONK_SUBSIDY_LOCK_SERVICE_CID = Components.ID("{edfdc311-cf16-40c5-b2f0-fb6094946179}");
const GONK_SUBSIDY_LOCK_SERVICE_CONTRACTID = "@mozilla.org/subsidylock/gonksubsidylockservice;1";

var DEBUG = RIL.DEBUG_RIL;
function debug(s) {
  dump("SubsidyLockService: " + s);
}

function SubsidyLockService() {
  this._providers = [];

  let numClients = gRadioInterfaceLayer.numRadioInterfaces;
  for (let i = 0; i < numClients; i++) {
    let radioInterface = gRadioInterfaceLayer.getRadioInterface(i);
    let provider = new SubsidyLockProvider(i, radioInterface);
    this._providers.push(provider);
  }
}
SubsidyLockService.prototype = {
  classID: GONK_SUBSIDY_LOCK_SERVICE_CID,
  classInfo: XPCOMUtils.generateCI({classID: GONK_SUBSIDY_LOCK_SERVICE_CID,
                                    contractID: GONK_SUBSIDY_LOCK_SERVICE_CONTRACTID,
                                    classDescription: "SubsidyLockService",
                                    interfaces: [Ci.nsISubsidyLockService],
                                    flags: Ci.nsIClassInfo.SINGLETON}),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISubsidyLockService]),

  // An array of SubsidyLockProvider instances.
  _providers: null,

  /**
   * nsISubsidyLockService interface.
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
};

function SubsidyLockProvider(aClientId, aRadioInterface) {
  this._clientId = aClientId;
  this._radioInterface = aRadioInterface;
}
SubsidyLockProvider.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISubsidyLock]),

  _clientId: null,

  _radioInterface: null,

  /**
   * A utility function to dump debug message.
   */
  _debug: function(aMessage) {
    dump("SubsidyLockProvider[" + this._clientId + "]: " + aMessage + "\n");
  },

  getSubsidyLockStatus(aCallback) {
    this._debug("getSubsidyLockStatus");

    function getPerLockStatus(lockfacility, radioInterface) {
      return new Promise((resolve, reject) => {
        radioInterface.sendWorkerMessage("iccGetSubsidyLockStatus",
                                         {
                                          facility : lockfacility,
                                          isSbusidyLock : true
                                         },
                                         (aResponse) => {
          if (aResponse.errorMsg) {
            reject(aResponse.errorMsg);
            return;
          }
          resolve(aResponse.result);
        });
      });
    }

    let asyncTask = [];
    for ( let i = 0; i < RIL.SUBSIDY_LOCK_FACLITYS.length; i++) {
      let newTask = getPerLockStatus(RIL.SUBSIDY_LOCK_FACLITYS[i], this._radioInterface);
      asyncTask.push(newTask);
    }
    Promise.all(asyncTask).then((result) => {
      let finalResult = [];
      for (let k = 0; k < result.length; k++) {
        if(result[k]) {
          finalResult.push(k+1);
        }
      }
      aCallback.notifyGetSubsidyLockStatusSuccess(finalResult.length, finalResult);
    }).catch((error) => {
      aCallback.notifyError(error);
    });
  },

  unlockSubsidyLock(aLockType, aPassword, aCallback) {
    this._debug("unlockSubsidyLock : " + aLockType + " / " + aPassword);

    // Dummy success result.
    Services.tm.currentThread.dispatch(() => aCallback.notifySuccess(),
                                       Ci.nsIThread.DISPATCH_NORMAL);
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([SubsidyLockService]);
