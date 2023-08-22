/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright © 2015, Deutsche Telekom, Inc. */

"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://services-crypto/utils.js");

XPCOMUtils.defineLazyModuleGetter(this, "DOMApplicationRegistry",
                                  "resource://gre/modules/Webapps.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "SEUtils",
                                  "resource://gre/modules/SEUtils.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "gSettingsService",
                                   "@mozilla.org/settingsService;1",
                                   "nsISettingsService");

XPCOMUtils.defineLazyGetter(this, "SE", function() {
  let obj = {};
  Cu.import("resource://gre/modules/se_consts.js", obj);
  return obj;
});

XPCOMUtils.defineLazyGetter(this, "NFC", function () {
  let obj = {};
  Cu.import("resource://gre/modules/nfc_consts.js", obj);
  return obj;
});

const APP_HASH_LEN = 20;

var DEBUG = SE.DEBUG_ACE;
var debug;
function updateDebug() {
  if (DEBUG) {
    debug = function (s) {
      dump("ACE:ACEservice: " + s + "\n");
    };
  } else {
    debug = function (s) {};
  }
};
updateDebug();

/**
 * Implements decision making algorithm as described in GPD specification,
 * mostly in 3.1, 3.2 and 4.2.3.
 *
 * TODO: Bug 1137533: Implement GPAccessRulesManager APDU filters
 */
function GPAccessDecision(rules, certHash, aid) {
  this.rules = rules;
  this.certHash = certHash;
  this.aid = aid;
}

GPAccessDecision.prototype = {
  isAccessAllowed: function isAccessAllowed() {
    // GPD SE Access Control v1.1, 3.4.1, Table 3-2: (Conflict resolution)
    // If a specific rule allows, all other non-specific access is denied.
    // Conflicting specific rules will resolve to the first Allowed == "true"
    // match. Given no specific rule, the global "All" rules will determine
    // access. "Some", skips further processing if access Allowed == "true".
    //
    // Access must be decided before the SE connector openChannel, and the
    // exchangeAPDU call.
    //
    // NOTE: This implementation may change with the introduction of APDU
    //       filters.
    let apduHeader = [];
    return this.isApduAccessAllowed(apduHeader);
  },

  isApduAccessAllowed: function isApduAccessAllowed(apduHeader) {
    let rule = this._findSpecificRule(this.aid, this.certHash);
    if (rule) {
      debug("Matched specific rule:" + JSON.stringify(rule));
      return this._apduAllowed(rule.apduRules, apduHeader);
    }

    rule = this._findSpecificAidRule(this.aid);
    if (rule) {
      debug("Matched specific aid rule:" + JSON.stringify(rule));
      return false;
    }

    rule = this._findGenericAidRule(this.aid)
    if (rule) {
      debug("Matched generic aid rule:" + JSON.stringify(rule));
      return this._apduAllowed(rule.apduRules, apduHeader);
    }

    rule = this._findGenericHashRule(this.certHash);
    if (rule) {
      debug("Matched generic hash rule:" + JSON.stringify(rule));
      return this._apduAllowed(rule.apduRules, apduHeader);
    }

    rule = this._findGenericRule();
    if (rule) {
      debug("Matched generic rule:" + JSON.stringify(rule));
      return this._apduAllowed(rule.apduRules, apduHeader);
    }
    debug("Can't find matched rule, access denied");
    return false;
  },

  _apduAllowed: function _apduAllowed(apduRules, apduHeader) {
    //
    // rule.apduRules contains a generic APDU access rule:
    // NEVER (0): APDU access is not allowed
    // ALWAYS(1): APDU access is allowed
    // or contains a specific APDU access rule based on one or more APDU filter(s):
    //
    // APDU filter: 8-byte APDU filter consists of:
    // 4-byte APDU filter header (defines the header of allowed APDUs, i.e. CLA, INS, P1, and P2 as defined in [7816-4])
    // 4-byte APDU filter mask (bit set defines the bits which shall be considered for the APDU header comparison)
    // An APDU filter shall be applied to the header of the APDU being checked, as follows:
    // if((APDU_header & APDU_filter_mask) == APDU_filter_header)
    // then allow APDU
    //
    debug("rule apdu fileter:" + apduRules + " apduHeader:" + apduHeader);
    if (apduRules && apduRules.length == 1) {
      return apduRules[0] ? true : false;
    }

    let apduFilters = this._parseApduFilter(apduRules);
    let length = apduFilters.length;

    for (let i = 0; i < length; i++) {
      let filter = apduFilters[i];
      debug("apduHeader:" + apduHeader + " mask:" + filter.mask + " header:" + filter.header);
      if (((apduHeader[0] & filter.mask[0]) == filter.header[0]) &&
          ((apduHeader[1] & filter.mask[1]) == filter.header[1]) &&
          ((apduHeader[2] & filter.mask[2]) == filter.header[2]) &&
          ((apduHeader[3] & filter.mask[3]) == filter.header[3])) {
        return true;
      }
    }
    return false;
  },

  _parseApduFilter: function _parseApduFilter(apduArdo) {
    function chunk(arr, chunkSize) {
      var R = [];
      for (var i=0,len=arr.length; i<len; i+=chunkSize)
        R.push(arr.slice(i,i+chunkSize));
      return R;
    }

    let apduFilters = chunk(apduArdo, 8);
    let apduFilterRules = [];

    apduFilters.forEach(filter => {
      let slice = chunk(filter, 4);
      let rule = {
        header: slice[0],
        mask: slice[1]
      };
      apduFilterRules.push(rule);
    });

    return apduFilterRules;
  },

  _findSpecificRule: function _findSpecificRule(aid, hash) {
    return this.rules.find(rule => {
      if (rule.hash.length != hash.length) return false;
      if (rule.aid.length != aid.length) return false;
      if (SEUtils.arraysEqual(rule.hash, hash) && SEUtils.arraysEqual(rule.aid, aid)) {
        debug("Found the rule that matches aid:" + aid + " App hash:" + hash);
        return true;
      }
      return false;
    });
  },

  _findSpecificAidRule: function _findSpecificAidRule(aid) {
    return this.rules.find(rule => {
      if (rule.aid.length != aid.length) return false;
      if (rule.hash.length != APP_HASH_LEN) return false;
      if (SEUtils.arraysEqual(rule.aid, aid)) {
        debug("Found the rule that matches aid:" + aid);
        return true;
      }
      return false
    });
  },

  _findGenericAidRule: function _findGenericAidRule(aid) {
    return this.rules.find(rule => {
      if (rule.aid.length != aid.length) return false;
      if (rule.hash.length) return false;
      if (SEUtils.arraysEqual(rule.aid, aid)) {
        debug("Found the generic rule that matches aid:" + aid);
        return true;
      }
      return false;
    });
  },

  _findGenericHashRule: function _findGenericHashRule(hash) {
    return this.rules.find(rule => {
      if (rule.hash.length != hash.length) return false;
      if (rule.aid.length) return false;
      if (SEUtils.arraysEqual(rule.hash, hash)) {
        debug("Found the generic rule that matches App hash:" + hash);
        return true;
      }
      return false;
    });
  },

  _findGenericRule: function _findGenericRule() {
    return this.rules.find(rule => {
      if (rule.hash.length || rule.aid.length) return false;
      debug("Found the generic rule, aid:" + rule.aid + " hash:" + rule.hash);
      return true;
    });
  },

  _decideAppAccess: function _decideAppAccess(rule) {
    let appMatched, appletMatched;

    // GPD SE AC 4.2.3: Algorithm for Applying Rules
    // Specific rule overrides global rule.
    //
    // DeviceAppID is the application hash, and the AID is SE Applet ID:
    //
    // GPD SE AC 4.2.3 A:
    //   SearchRuleFor(DeviceAppID, AID)
    // GPD SE AC 4.2.3 B: If no rule fits A:
    //   SearchRuleFor(<AllDeviceApplications>, AID)
    // GPD SE AC 4.2.3 C: If no rule fits A or B:
    //   SearchRuleFor(DeviceAppID, <AllSEApplications>)
    // GPD SE AC 4.2.3 D: If no rule fits A, B, or C:
    //   SearchRuleFor(<AllDeviceApplications>, <AllSEApplications>)

    // Device App
    appMatched = Array.isArray(rule.application) ?
      // GPD SE AC 4.2.3 A and 4.2.3 C (DeviceAppID rule)
      this._appCertHashMatches(rule.application) :
      // GPD SE AC 4.2.3 B and 4.2.3 D (All Device Applications)
      rule.application === Ci.nsIAccessRulesManager.ALLOW_ALL;

    if (!appMatched) {
      return false; // bail out early.
    }

    // SE Applet
    appletMatched = Array.isArray(rule.applet) ?
      // GPD SE AC 4.2.3 A and 4.2.3 B (AID rule)
      SEUtils.arraysEqual(rule.applet, this.aid) :
      // GPD SE AC 4.2.3 C and 4.2.3 D (All AID)
      rule.applet === Ci.nsIAccessRulesManager.ALL_APPLET;

    return appletMatched;
  },

  _appCertHashMatches: function _appCertHashMatches(hashArray) {
    if (!Array.isArray(hashArray)) {
      return false;
    }

    return !!(hashArray.find((hash) => {
      return SEUtils.arraysEqual(hash, this.certHash);
    }));
  }
};

function ACEService() {
  this._rulesManagers = Cc["@mozilla.org/secureelement/access-control/rules-manager;1"]
                          .createInstance(Ci.nsIAccessRulesManager);
  if (!this._rulesManagers) {
    debug("Can't find rules manager");
  }

  let lock = gSettingsService.createLock();
  lock.get(NFC.SETTING_NFC_DEBUG, this);
  Services.obs.addObserver(this, NFC.TOPIC_MOZSETTINGS_CHANGED, false);
}

ACEService.prototype = {
  _rulesManagers: null,

  isAPDUAccessAllowed: function isAPDUAccessAllowed(localId, seType, aid, apduHeader) {
    if(!Services.prefs.getBoolPref("dom.secureelement.ace.enabled")) {
      debug("Access control enforcer is disabled, allowing access");
      return Promise.resolve(true);
    }

    let manifestURL = DOMApplicationRegistry.getManifestURLByLocalId(localId);
    if (!manifestURL) {
      return Promise.reject(Error("Missing manifest for app: " + localId));
    }

    let appsService = Cc["@mozilla.org/AppsService;1"].getService(Ci.nsIAppsService);
    let currentApp = appsService.getAppByManifestURL(manifestURL);

    if (currentApp && currentApp.principal &&
          currentApp.principal.appStatus === Ci.nsIPrincipal.APP_STATUS_CERTIFIED){
      debug("Allowing access for Certified app:" + currentApp.principal.appStatus);
      return Promise.resolve(true);
    }

    return new Promise((resolve, reject) => {
      debug("isAPDUAccessAllowed for " + manifestURL + " to " + aid + " apduHeader:" + apduHeader);

      let certHash = this._getDevCertHashForApp(manifestURL);
      debug(manifestURL + " is requesting for access " + certHash);
      if (!certHash) {
        debug("App " + manifestURL + " tried to access SE, but no developer" +
              " certificate present");
        reject(Error("No developer certificate found"));
        return;
      }

      this._rulesManagers.getAccessRules(seType).then((rules) => {
        let decision = new GPAccessDecision(rules,
          SEUtils.hexStringToByteArray(certHash), SEUtils.hexStringToByteArray(aid));

        if (seType == SE.TYPE_ESE && rules.length <= 0) {
          // Allow access when there is no access rule in the ARAM.
          debug("No rule in the ARAM. Access allowed !!!");
          resolve(true);
          return;
        }

        resolve(decision.isApduAccessAllowed(SEUtils.hexStringToByteArray(apduHeader)));
      }).catch(()=>{
        reject(Error("Failed to get access rules"));
      });
    });
  },

  isAccessAllowed: function isAccessAllowed(localId, seType, aid) {
    if(!Services.prefs.getBoolPref("dom.secureelement.ace.enabled")) {
      debug("Access control enforcer is disabled, allowing access");
      return Promise.resolve(true);
    }

    let manifestURL = DOMApplicationRegistry.getManifestURLByLocalId(localId);
    if (!manifestURL) {
      return Promise.reject(Error("Missing manifest for app: " + localId));
    }

    return new Promise((resolve, reject) => {
      debug("isAccessAllowed for " + manifestURL + " to " + aid);

      let certHash = this._getDevCertHashForApp(manifestURL);
      debug(manifestURL + " is requesting for access " + certHash);
      if (!certHash) {
        debug("App " + manifestURL + " tried to access SE, but no developer" +
              " certificate present");
        reject(Error("No developer certificate found"));
        return;
      }

      this._rulesManagers.getAccessRules(seType).then((rules) => {
        let decision = new GPAccessDecision(rules,
          SEUtils.hexStringToByteArray(certHash), aid);

        if (seType == SE.TYPE_ESE && rules.length <= 0) {
          // Allow access when there is no access rule in the ARAM.
          debug("No rule in the ARAM. Access allowed !!!");
          resolve(true);
          return;
        }
        resolve(decision.isAccessAllowed());
      }).catch(()=>{
        reject(Error("Failed to get access rules"));
      });
    });
  },

  isHCIEventAccessAllowed: function isHCIEventAccessAllowed(localId, seType, aid) {
    if(!Services.prefs.getBoolPref("dom.secureelement.ace.enabled")) {
      debug("Access control enforcer is disabled, allowing access");
      return Promise.resolve(true);
    }

    let manifestURL = DOMApplicationRegistry.getManifestURLByLocalId(localId);
    if (!manifestURL) {
      return Promise.reject(Error("Missing manifest for app: " + localId));
    }

    let appsService = Cc["@mozilla.org/AppsService;1"].getService(Ci.nsIAppsService);
    let currentApp = appsService.getAppByManifestURL(manifestURL);

    if (currentApp && currentApp.principal &&
          currentApp.principal.appStatus === Ci.nsIPrincipal.APP_STATUS_CERTIFIED){
      debug("Allowing access for Certified app:" + currentApp.principal.appStatus);
      return Promise.resolve(true);
    }

    // seType could be eSE/eSE1/eSE2... for embedded secure element,
    // or SIM/SIM1/SIM2 for uicc, convert it to internal type.
    if (seType.startsWith("eSE")) {
      seType = SE.TYPE_ESE;
    } else if (seType.startWith("SIM")){
      seType = SE.TYPE_UICC;
    } else {
      return Promise.reject(Error("UnSupported seType: " + seType));
    }

    return new Promise((resolve, reject) => {
      debug("isHCIEventAccessAllowed for " + manifestURL + " to " + aid);

      let certHash = this._getDevCertHashForApp(manifestURL);
      debug(manifestURL + " is requesting for access " + certHash);
      if (!certHash) {
        debug("App " + manifestURL + " tried to access SE, but no developer" +
              " certificate present");
        reject(Error("No developer certificate found"));
        return;
      }

      this._rulesManagers.getAccessRules(seType).then((rules) => {
        let decision = new GPAccessDecision(rules,
          SEUtils.hexStringToByteArray(certHash), SEUtils.hexStringToByteArray(aid));

        if (seType == SE.TYPE_ESE && rules.length <= 0) {
          // Allow access when there is no access rule in the ARAM.
          debug("No rule in the ARAM. Access allowed !!!");
          resolve(true);
          return;
        }
        resolve(decision.isAccessAllowed());
      }).catch(()=>{
        reject(Error("Failed to get access rules"));
      });
    });
  },

  _getDevCertHashForApp: function getDevCertHashForApp(manifestURL) {
    return CryptoUtils.sha1(manifestURL);
  },

  classID: Components.ID("{882a7463-2ca7-4d61-a89a-10eb6fd70478}"),
  contractID: "@mozilla.org/secureelement/access-control/ace;1",
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver, Ci.nsIAccessControlEnforcer]),

  /**
   * nsISettingsServiceCallback
   */
  handle: function handle(name, result) {
    switch (name) {
      case NFC.SETTING_NFC_DEBUG:
        DEBUG = result;
        updateDebug();
        break;
    }
  },

  /**
   * nsIObserver interface methods.
   */

  observe: function observe(subject, topic, data) {
    switch (topic) {
      case NFC.TOPIC_MOZSETTINGS_CHANGED:
        if ("wrappedJSObject" in subject) {
          subject = subject.wrappedJSObject;
        }
        if (subject) {
          this.handle(subject.key, subject.value);
        }
        break;
    }
  },
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([ACEService]);

