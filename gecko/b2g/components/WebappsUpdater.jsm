/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

this.EXPORTED_SYMBOLS = ["WebappsUpdater"];

const Cc = Components.classes;
const Cu = Components.utils;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "settings",
                                   "@mozilla.org/settingsService;1",
                                   "nsISettingsService");

XPCOMUtils.defineLazyModuleGetter(this, "SystemAppProxy",
                                  "resource://gre/modules/SystemAppProxy.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "powerManagerService",
                                   "@mozilla.org/power/powermanagerservice;1",
                                   "nsIPowerManagerService");

var debug;
function debugPrefObserver() {
  debug = Services.prefs.getBoolPref("dom.mozApps.debug")
            ? (aMsg) => dump("--*-- WebappsUpdater: " + aMsg)
            : (aMsg) => {};
}
debugPrefObserver();
Services.prefs.addObserver("dom.mozApps.debug", debugPrefObserver, false);

this.WebappsUpdater = {
  _checkingApps: false,
  _wakeLock: null,

  handleContentStart: function() {
  },

  sendChromeEvent: function(aType, aDetail) {
    let detail = aDetail || {};
    detail.type = aType;

    let sent = SystemAppProxy.dispatchEvent(detail);
    if (!sent) {
      debug("Warning: Couldn't send update event " + aType +
          ": no content browser. Will send again when content becomes available.");
      return false;
    }

    return true;
  },

  _appsUpdated: function(aApps) {
    debug("appsUpdated: " + aApps.length + " apps to update");
    let lock = settings.createLock();
    lock.set("apps.updateStatus", "check-complete", null);
    this.sendChromeEvent("apps-update-check", { apps: aApps });
    this._checkingApps = false;
    if ( this._wakeLock ) {
      this._wakeLock.unlock();
      this._wakeLock = null;
    }
  },

  // Trigger apps update check and wait for all to be done before
  // notifying gaia.
  updateApps: function() {
    debug("updateApps (" + this._checkingApps + ")");
    // Don't start twice.
    if (this._checkingApps) {
      return;
    }

    let allowUpdate = true;
    try {
      allowUpdate = Services.prefs.getBoolPref("webapps.update.enabled");
    } catch (ex) { }

    if (!allowUpdate) {
      return;
    }

    this._checkingApps = true;
    this._wakeLock = powerManagerService.newWakeLock("cpu");

    let self = this;

    let window = Services.wm.getMostRecentWindow("navigator:browser");
    let all = window.navigator.mozApps.mgmt.getAll();

    all.onsuccess = function() {
      let appsCount = this.result.length;
      let appsChecked = 0;
      let appsToUpdate = [];
      this.result.forEach(function updateApp(aApp) {
        // Do not check for update if app is disabled.
        if (aApp.enabled === false) {
          appsChecked += 1;
          if (appsChecked == appsCount) {
            self._appsUpdated(appsToUpdate);
          }
          return;
        }
        let update = aApp.checkForUpdate();
        update.onsuccess = function() {
          if (update.timer) {
            update.timer.cancel();
            update.timer = null;
          }
          if (aApp.downloadAvailable) {
            appsToUpdate.push(aApp.manifestURL);
          }

          appsChecked += 1;
          if (appsChecked == appsCount) {
            self._appsUpdated(appsToUpdate);
          }
        }
        update.onerror = function() {
          if (update.timer) {
            update.timer.cancel();
            update.timer = null;
          }
          appsChecked += 1;
          if (appsChecked == appsCount) {
            self._appsUpdated(appsToUpdate);
          }
        }
        update.timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
        update.timer.initWithCallback(function(aTimer) {
          if (aTimer == update.timer) {
            update.timer = null;
            appsChecked += 1;
            if (appsChecked == appsCount) {
              self._appsUpdated(appsToUpdate);
            }
          }
        }, 3*60*1000, Ci.nsITimer.TYPE_ONE_SHOT);
      });
    }

    all.onerror = function() {
      // Could not get the app list, just notify to update nothing.
      self._appsUpdated([]);
    }
  }
};
