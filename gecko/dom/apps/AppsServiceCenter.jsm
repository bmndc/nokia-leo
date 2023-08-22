/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

"use strict"

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

this.EXPORTED_SYMBOLS = ["AppsServiceCenter"];

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");
Cu.import("resource://gre/modules/Webapps.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Promise.jsm")
Cu.import("resource://gre/modules/DeviceUtils.jsm");

let debug = Services.prefs.getBoolPref("dom.mozApps.debug")
              ? (aMsg) => dump("-*- AppsServiceCenter.jsm : " + aMsg + "\n")
              : (aMsg) => {};

function getRandomInt(max) {
  return Math.floor(Math.random() * max);
}

this.AppsServiceCenter = {

  _registeredOnlineObserver: false,

  _URI: {},

  _checkTimer: null,

  _checkInterval: 86400000,

  _doCheck: function() {

    if (Services.io.offline) {
      debug("_doCheck - network is offline");
      this._registerOnlineObserver();
      return;
    }

    var request = new Promise((aResolve, aReject) => {
      let url = this._URI.spec;
      let xhr =  Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
                   .createInstance(Ci.nsIXMLHttpRequest);

      xhr.mozBackgroundRequest = true;
      xhr.open("INFO", url);
      xhr.setRequestHeader("Content-Type",
                           "application/x-www-form-urlencoded");

      // Prevent the request from reading from the cache.
      xhr.channel.loadFlags |= Ci.nsIRequest.LOAD_BYPASS_CACHE;
      // Prevent the request from writing to the cache.
      xhr.channel.loadFlags |= Ci.nsIRequest.INHIBIT_CACHING;
      // Prevent privacy leaks
      xhr.channel.loadFlags |= Ci.nsIRequest.LOAD_ANONYMOUS;
      // The Cache-Control header is only interpreted by proxies and the
      // final destination. It does not help if a resource is already
      // cached locally.
      xhr.setRequestHeader("Cache-Control", "no-cache");
      // HTTP/1.0 servers might not implement Cache-Control and
      // might only implement Pragma: no-cache
      xhr.setRequestHeader("Pragma", "no-cache");

      xhr.addEventListener("load", function(aRequest) {
        debug("_doCheck - on request load : " + xhr.status);

        if (xhr.status == 200) {
            aResolve();
        } else {
          aReject();
        }
      }.bind(this));

      xhr.addEventListener("error", function(event) {
        var status = event.target.channel.QueryInterface(Ci.nsIRequest).status;
        debug("_doCheck - on request error : " + status);

        if (status === Cr.NS_ERROR_OFFLINE) {
          // Check again when network is online.
          this._registerOnlineObserver();
        } else {
          aReject();
        }
      }.bind(this));

      xhr.send(null);
    });

    request.then(function(data) {
      try {
        // Enable service center.
        let manifest = Services.prefs.getCharPref("apps.serviceCenter.manifest");
        let app = DOMApplicationRegistry.getAppByManifestURL(manifest);
        if (app) {
          debug("_doCheck - Enable service service center");
          DOMApplicationRegistry.setEnabled({manifestURL: app.manifestURL, enabled: true});
        }
      } catch(e) {
        debug("_doCheck - Failed to enable service center, error:" + e);
      }
    }.bind(this)).catch(function(data) {
        // Server is not ready, schedule a timer to check agian later.
        debug("_doCheck - setup retry timer");
        this._checkTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
        this._checkTimer.initWithCallback(function() {
          this._doCheck();
        }.bind(this), this._checkInterval, Ci.nsITimer.TYPE_ONE_SHOT);
    }.bind(this));
  },

  /**
   * Register an observer when the network comes online,
   * so we can try to check status again when network available.
   */
  _registerOnlineObserver: function() {
    if (this._registeredOnlineObserver) {
      debug("_registerOnlineObserver - observer already registered");
      return;
    }

    debug("_registerOnlineObserver - waiting for the network to be online");
    Services.obs.addObserver(this, "network:offline-status-changed", false);
    this._registeredOnlineObserver = true;
  },

  /**
   * Called from the network:offline-status-changed observer.
   */
  _offlineStatusChanged: function(status) {
    debug("_offlineStatusChanged - status: " + status);
    if (status !== "online") {
      return;
    }

    Services.obs.removeObserver(this, "network:offline-status-changed");
    this._registeredOnlineObserver = false;
    debug("_offlineStatusChanged - network is online check again");
    this._doCheck();
  },

  /**
   * Handle Observer Service notifications
   * @param   subject
   *          The subject of the notification
   * @param   topic
   *          The notification name
   * @param   data
   *          Additional data
   */
  observe: function(aSubject, aTopic, aData) {
    debug("observe - topic: " + aTopic);
    switch (aTopic) {
    case "network:offline-status-changed":
      this._offlineStatusChanged(aData);
      break;
    }
  },

  init: function() {

    let appInfo = Cc["@mozilla.org/xre/app-info;1"];
    let isParentProcess = !appInfo || appInfo.getService(Ci.nsIXULRuntime)
                          .processType == Ci.nsIXULRuntime.PROCESS_TYPE_DEFAULT;

    // Need to run this service in parent only.
    if (!isParentProcess) {
      return;
    }

    // Check if service center is already enabled.
    try {
      var manifest = Services.prefs.getCharPref("apps.serviceCenter.manifest");
    } catch(e) {
      debug("init - Failed to get service center manifest, error: " + e);
      return;
    }
    let app = DOMApplicationRegistry.getAppByManifestURL(manifest);
    if (app && app.enabled === true) {
      debug("init - Service center is already enabled");
      return;
    }

    try {
      var url = Services.prefs.getCharPref("apps.serviceCenter.serverURL");
    } catch(e) {
      debug("init - Failed to get service center url, error: " + e);
      return;
    }
    url = url.replace(/%DEVICE_REF%/g, DeviceUtils.cuRef);
    this._URI = Services.io.newURI(url, null, null);

    var checkInterval = 86400000; // set default interval to 1 day.
    try {
      checkInterval = Services.prefs.getIntPref("apps.serviceCenter.checkInterval");
    } catch(e) { }
    // Add a random to prevent too many requests occur in the same time.
    this._checkInterval = (checkInterval + getRandomInt(3600)) * 1000;

    this._doCheck();
  }
}

AppsServiceCenter.init();
