/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Promise.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "appsService",
                                   "@mozilla.org/AppsService;1",
                                   "nsIAppsService");

function debug(aMsg) {
  // dump("-- ActivityMessageConfigurator.js " + Date.now() + " : " + aMsg + "\n");
}

/**
  * nsISystemMessagesConfigurator implementation.
  */
function ActivityMessageConfigurator() {
  debug("ActivityMessageConfigurator");
}

ActivityMessageConfigurator.prototype = {
  get mustShowRunningApp() {
    debug("mustShowRunningApp returning true");
    return true;
  },

  shouldDispatch: function shouldDispatch(aManifestURL, aPageURL, aType, aMessage, aExtra) {
    debug("Manifest url: " + aManifestURL);
    debug("Message to dispatch: " + JSON.stringify(aMessage));

    if (!aMessage) {
      return Promise.resolve(false);
    }

    let appId = appsService.getAppLocalIdByManifestURL(aManifestURL);
    if (appId === Ci.nsIScriptSecurityManager.NO_APP_ID) {
      return Promise.resolve(false);
    }

    return new Promise((resolve, reject) => {
      appsService.getManifestFor(aManifestURL)
      .then(manifest => {
        let originList = manifest["origin_activities"];
        // Both sender and receiver applications don't specifiy the origin.
        if (!aMessage.payload.origin && !originList) {
          return resolve(true);
        }

        let origin = (aMessage.payload.origin) ?
                      aMessage.payload.origin.toUpperCase() : "";
        let matching = originList.find((rule) => {
          rule = rule.toUpperCase();

          debug("Origin is " + origin + " rule is " + rule);
          if(rule === "*") {
            return true;
          }

          return rule === origin;
        });

        if (matching) {
          debug("Dispatching");
        } else {
          debug("Don't dispatching");
        }
        resolve(matching);
      })
      .catch(() => {
        debug("Don't dispatching");
        resolve(false);
      });
    });
  },

  classID: Components.ID("{d2296daa-c406-4c5e-b698-e5f2c1715798}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISystemMessagesConfigurator])
}

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([ActivityMessageConfigurator]);
