/* (c) 2018 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

"use strict";

this.EXPORTED_SYMBOLS = [ "CustomHeaderInjector" ];

var Ci = Components.interfaces;
var Cc = Components.classes;
var Cu = Components.utils;
var Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/DeviceUtils.jsm");

const DEBUG = false;
function debug(msg) {
  dump("-* CustomHeaderInjector *- " + msg + "\n");
}

const kPrefCustomHeaderHosts = "network.http.customheader.hosts";
const kCustomHeaderVersion = "v1";

XPCOMUtils.defineLazyServiceGetter(this, "gMobileConnectionService",
                                   "@mozilla.org/mobileconnection/mobileconnectionservice;1",
                                   "nsIMobileConnectionService");
XPCOMUtils.defineLazyServiceGetter(this, "gIccService",
                                   "@mozilla.org/icc/iccservice;1",
                                   "nsIIccService");

var gCustomHeaderValue;
var gDeviceUid;
var gComRef;
var gMnc;
var gMcc;
var gNetMnc;
var gNetMcc;
var gNetType;
var gKaiServiceHosts;

this.CustomHeaderInjector = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIMobileConnectionListener,
                                         Ci.nsIIccListener]),

  init: function() {
    this.buildKaiServiceHosts();
    if (DEBUG) debug("Hosts require injection: " + JSON.stringify(gKaiServiceHosts));
    this.initCustomHeaderValue().then(() => {
      gCustomHeaderValue = this.buildCustomHeader();
    }, () => {
      if (DEBUG) debug("Rejected by initCustomHeaderValue.");
      // Even getting device Id is somehow rejected, we still want to build the
      // ads-service header because other values are good.
      gCustomHeaderValue = this.buildCustomHeader();
    });
    Services.obs.addObserver(this, "http-on-custom-header-inject-request", false);
    Services.obs.addObserver(this, "network-active-changed", false);
    gMobileConnectionService.getItemByServiceId(0).registerListener(this);
    gIccService.getIccByServiceId(0).registerListener(this);
  },

  uninit: function() {
    Services.obs.removeObserver(this, "http-on-custom-header-inject-request");
    Services.obs.removeObserver(this, "network-active-changed");
    gMobileConnectionService.getItemByServiceId(0).unregisterListener(this);
    gIccService.getIccByServiceId(0).unregisterListener(this);
  },

  buildKaiServiceHosts: function() {
    gKaiServiceHosts = [];
    try {
      let hosts = Services.prefs.getCharPref(kPrefCustomHeaderHosts);
      gKaiServiceHosts = hosts.split(',');
    } catch (e) {
    }
  },

  buildCustomHeader: function() {
    let value = kCustomHeaderVersion + ";";
    value += gDeviceUid + ";";
    value += gComRef + ";";
    value += gMnc + ";";
    value += gMcc + ";";
    value += gNetMnc + ";";
    value += gNetMcc + ";";
    value += gNetType;

    return value;
  },

  initCustomHeaderValue: function() {
    // Get SIM mnc and mcc.
    let iccInfo = DeviceUtils.iccInfo;
    if (iccInfo) {
      gMnc = iccInfo.mnc;
      gMcc = iccInfo.mcc;
    }

    // Consist return value to empty string if mnc/mcc is not avaliable.
    gMnc = (!gMnc) ? "" : gMnc;
    gMcc = (!gMcc) ? "" : gMcc;

    // Get Network mnc and mcc.
    gNetMnc = DeviceUtils.networkMnc;
    gNetMcc = DeviceUtils.networkMcc;

    // Get Network connection type.
    gNetType = DeviceUtils.networkType;

    // Get commercial reference.
    gComRef = DeviceUtils.cuRef;
    gComRef = (!gComRef) ? "" : gComRef;
    gComRef = gComRef.replace(/;/g, "\\u003B");

    // Get Device Id.
    return DeviceUtils.getDeviceId().then((deviceid) => {
      gDeviceUid = deviceid;
      gDeviceUid = (!gDeviceUid) ? "" : gDeviceUid;
      gDeviceUid = gDeviceUid.replace(/;/g, "\\u003B");
      return Promise.resolve();
    }, (reason) => {
      return Promise.reject('Fail when retrieving deviceId.');
    });
  },

  updateCustomHeaderNetValue: function() {
    // Network mnc, network mcc, network connection type may varies.
    // Otherwise, no need to rebuild the custom header.
    let net_mnc = DeviceUtils.networkMnc;
    let net_mcc = DeviceUtils.networkMcc;
    let net_type = DeviceUtils.networkType;
    if (gNetMnc != net_mnc || gNetMcc != net_mcc || gNetType != net_type) {
      gNetMnc = net_mnc;
      gNetMcc = net_mcc;
      gNetType = net_type;
      gCustomHeaderValue = this.buildCustomHeader();
    }
  },

  updateCustomHeaderIccValue: function() {
    // Icc mnc, Icc mcc may varies.
    // Otherwise, no need to rebuild the custom header.
    let iccInfo = DeviceUtils.iccInfo;
    let mnc = iccInfo.mnc;
    let mcc = iccInfo.mcc;

    if (gMnc != mnc || gMcc != mcc) {
      gMnc = mnc;
      gMcc = mcc;
      gCustomHeaderValue = this.buildCustomHeader();
    }
  },

  isKaiServiceConnection: function(aURI) {
    for (let host of gKaiServiceHosts) {
      if (aURI.host.endsWith(host)) {
        return true;
      }
    }
    return false;
  },

  observe: function(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "http-on-custom-header-inject-request": {
        let channel = aSubject.QueryInterface(Ci.nsIHttpChannel);
        let uri = channel.URI;
        if (uri.schemeIs("https") && this.isKaiServiceConnection(uri)) {
          if (DEBUG) debug("Inject custom header for KaiService connection.");
          if (DEBUG) debug(" host: " + uri.host);
          if (DEBUG) debug(" header value: " + gCustomHeaderValue);
          channel.setRequestHeader("X-Kai-Ads", gCustomHeaderValue, false);
        }
        break;
      }
      case "network-active-changed": {
        this.updateCustomHeaderNetValue();
        break;
      }
    }
  },

  // nsIMobileConnectionListener
  notifyVoiceChanged: function() {
    this.updateCustomHeaderNetValue();
  },
  notifyDataChanged: function() {},
  notifyDataError: function(message) {},
  notifyCFStateChanged: function(action, reason, number, timeSeconds, serviceClass) {},
  notifyEmergencyCbModeChanged: function(active, timeoutMs) {},
  notifyOtaStatusChanged: function(status) {},
  notifyRadioStateChanged: function() {},
  notifyClirModeChanged: function(mode) {},
  notifyLastKnownNetworkChanged: function() {},
  notifyLastKnownHomeNetworkChanged: function() {},
  notifyNetworkSelectionModeChanged: function() {},
  notifyDeviceIdentitiesChanged: function() {},
  notifySignalStrengthChanged: function() {},

  // nsIIccListener
  notifyStkCommand(aStkProactiveCmd){},
  notifyStkSessionEnd() {},
  notifyCardStateChanged() {
    this.updateCustomHeaderIccValue();
  },
  notifyIccInfoChanged() {},
  notifyIsimInfoChanged() {}
}
