/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * This component triggers an app update check even when system updates are
 * disabled to make sure we always check for app updates.
 */

"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/WebappsUpdater.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "AlarmService",
                                  "resource://gre/modules/AlarmService.jsm");

var debug;
function debugPrefObserver() {
  debug = Services.prefs.getBoolPref("dom.mozApps.debug")
            ? (aMsg) => dump("--*-- WebappsUpdateTimer: " + aMsg)
            : (aMsg) => {};
}
debugPrefObserver();
Services.prefs.addObserver("dom.mozApps.debug", debugPrefObserver, false);

function WebappsUpdateTimer() {
  dump('WebappsUpdateTimer Start');
  let timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
  timer.initWithCallback(this, 1000 * 60,
                         Ci.nsITimer.TYPE_ONE_SHOT);
  this._timer = timer;
}

WebappsUpdateTimer.prototype = {

  QueryInterface: XPCOMUtils.generateQI([Ci.nsITimerCallback]),

  classID: Components.ID("{637b0f77-2429-49a0-915f-abf5d0db8b9a}"),

  _timer: null,

  _interval: 86400,

  _alarmId: null,

  _saveDelay: 0,

  _addAlarm: function(callback, delay) {
    debug("_addAlarm - delay: " + delay);
    AlarmService.add({
      date: new Date(delay),
      ignoreTimezone: true
    },
    function onAlarmFired(aAlarm) {
      dump("WebappsUpdateTimer onAlarmFired.");
      callback();
    },
    function onSuccess(aAlarmId) {
      debug("_addAlarm onSuccess - aAlarmId: " + aAlarmId);
      this._alarmId = aAlarmId;
    }.bind(this),
    function onError(error) {
      debug("_addAlarm onError - Error: " + error);
    });
  },

  _newAlarm: function() {
    debug("_newAlarm");

    if (this._alarmId) {
      AlarmService.remove(this._alarmId);
      this._alarmId = null;
    }

    let alarmDelay = (this._saveDelay > 0 && this._saveDelay > Date.now()) ?
                      this._saveDelay : Date.now() + this._interval * 1000;

    this._addAlarm( (function() {
      // If network is offline, wait to be online to start the update check.
      if (Services.io.offline) {
        debug("Network is offline. Setting up an offline status observer.");
        Services.obs.addObserver(this, "network:offline-status-changed", false);
        return;
      }
      WebappsUpdater.updateApps();
      this._newAlarm();
    }).bind(this), alarmDelay);

    Services.prefs.setCharPref("webapps.update.delay", alarmDelay);
  },

  _updateAlarm: function() {
    debug("_updateAlarm");

    if (this._alarmId !== null) {
      debug("remove alarmId:" + this._alarmId);
      AlarmService.remove(this._alarmId);
      this._alarmId = null;
    }

    try {
      this._interval = Services.prefs.getIntPref("webapps.update.interval");
    } catch(e) {}

    this._saveDelay = 0;
    this._newAlarm();
  },

  _initAlarm: function() {
    debug("_initAlarm");

    try {
      this._interval = Services.prefs.getIntPref("webapps.update.interval");
    } catch(e) {}

    try {
      this._saveDelay = parseFloat(Services.prefs.getCharPref("webapps.update.delay"));
    } catch(e) {}

    if (this._saveDelay < Date.now()) {
      // If network is offline, wait to be online to start the update check.
      if (Services.io.offline) {
        debug("Network is offline. Setting up an offline status observer.");
        Services.obs.addObserver(this, "network:offline-status-changed", false);
        return;
      }
      // This will trigger app updates in b2g/components/WebappsUpdater.jsm
      // that also takes care of notifying gaia.
      WebappsUpdater.updateApps();
    }

    this._newAlarm();
    Services.prefs.addObserver("webapps.update.interval", this, false);
  },

  notify: function(aTimer) {
    debug(" timer notify ");

    this._timer = null;
    try {
      // We don't want to continue when webapps update is not enabled.
      if (!Services.prefs.getBoolPref("webapps.update.enabled")) {
        return;
      }
    } catch(e) {
      // That should never happen.
    }

    // Init alarm service to check for update.
    this._initAlarm();
  },

  observe: function(aSubject, aTopic, aData) {

    if (aTopic === "nsPref:changed" &&
        aData === "webapps.update.interval" ) {
      this._updateAlarm();
      return;
    }

    if (aTopic === "network:offline-status-changed" &&
        aData === "online") {
      debug("Network is online. Checking updates.");
      Services.obs.removeObserver(this, "network:offline-status-changed");

      WebappsUpdater.updateApps();
      this._newAlarm();
      return;
    }
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([WebappsUpdateTimer]);
