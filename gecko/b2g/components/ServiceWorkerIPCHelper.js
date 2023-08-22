/* (c) 2018 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

"use strict";

let DEBUG = false;

function debug(s) {
  if (DEBUG) {
    dump("-*- ServiceWorkerIPCHelper.js: " + s + "\n");
  }
}

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/AppsUtils.jsm");

const NS_SERVICEWORKER_IPCHELPER_CID = "{ec24be4e-6245-11e8-88f7-ef9ea5d89e80}";
const NS_SERVICEWORKER_IPCHELPER_CONTRACTID =
  "@mozilla.org/serviceworkers/ipchelper;1";

XPCOMUtils.defineLazyModuleGetter(this, "Services",
                                  "resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "SystemAppProxy",
                                  "resource://gre/modules/SystemAppProxy.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "appsService",
                                   "@mozilla.org/AppsService;1",
                                   "nsIAppsService");

XPCOMUtils.defineLazyServiceGetter(this, "SystemMessenger",
                                   "@mozilla.org/system-message-internal;1",
                                   "nsISystemMessagesInternal");

debug("loaded");

function ServiceWorkerIPCHelper() {
  Services.obs.addObserver(this, "xpcom-shutdown", false);
}

ServiceWorkerIPCHelper.prototype = {

  observe: function (aSubject, aTopic, aData) {
    if (aTopic === "xpcom-shutdown") {
      Services.obs.removeObserver(this, "xpcom-shutdown");
    }
  },

  _focusApp: function (aAppID) {
    debug("focusing app AppID: " + aAppID);

    let app = appsService.getAppByLocalId(aAppID);
    if (!app) {
      debug("no app found with appID: " + aAppID);
      return;
    }

    debug("app manifest URL: " + app.manifestURL);
    debug("app origin: " + app.origin);

    appsService.getManifestFor(app.manifestURL).then((manifest) => {
      let helper = new ManifestHelper(manifest, app.origin, app.manifestURL);

      let eventType = "open-app";
      let launchPath = helper.fullLaunchPath();

      debug("resolved launchpath: " + launchPath);
      SystemAppProxy._sendCustomEvent(eventType, {
        url: launchPath,
        manifestURL: app.manifestURL,
        showApp: true,
        onlyShowApp: false,
      }, false);

    }).catch((e) => {
      debug(e);
    });
  },

  sendClientFocusEvent: function (aAppID) {
    this._focusApp(aAppID);
  },

  sendClientsOpenAppEvent: function (aAppID, aMsg) {
    let app = appsService.getAppByLocalId(aAppID);
    if (!app) {
      debug("no app found with appID: " + aAppID);
      return;
    }

    debug("app manifest URL: " + app.manifestURL);

    let manifestURI = Services.io.newURI(app.manifestURL, null, null);
    let data = {
      msg: aMsg
    };
    let extra = {
      showApp: true
    };

    let type = 'serviceworker-notification';
    SystemMessenger.sendMessage(type, data, null, manifestURI, extra);
  },

  classID: Components.ID(NS_SERVICEWORKER_IPCHELPER_CID),
  contractID: NS_SERVICEWORKER_IPCHELPER_CONTRACTID,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIServiceWorkerIPCHelper]),
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([ServiceWorkerIPCHelper]);