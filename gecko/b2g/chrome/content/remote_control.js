/* -*- indent-tabs-mode: nil; js-indent-level: 2 -*- /
/* vim: set shiftwidth=2 tabstop=2 autoindent cindent expandtab: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

Cu.import("resource://gre/modules/Services.jsm");

const kPrefPresentationDiscoverable = "dom.presentation.discoverable";

(function setupRemoteControlSettings() {
  // To keep RemoteContorlService in the scope to prevent import again
  var remoteControlScope = {};

  function importRemoteControlService() {
    if (!("RemoteControlService" in remoteControlScope)) {
      Cu.import("resource://gre/modules/RemoteControlService.jsm", remoteControlScope);
    }
  }

  function getTime() {
    return new Date().getTime();
  }

  function debug(s) {
    let time = new Date(getTime()).toString();
    // dump("[" + time + "] remote_control: " + s + "\n");
  }

  // Monitor the network status and the time change
  var statusMonitor = {
    init: function sm_init() {
      debug("init");

      // Monitor the preference change
      Services.prefs.addObserver(kPrefPresentationDiscoverable, this, false);
      // Monitor the network change by nsIIOService
      Services.obs.addObserver(this, "network:offline-status-changed", false);
#ifdef MOZ_WIDGET_GONK
      // Monitor the time change on Gonk platform
      window.addEventListener('moztimechange', this.onTimeChange);
      // Monitor the network change by nsINetworkManager
      if (Ci.nsINetworkManager) {
        Services.obs.addObserver(this, "network-active-changed", false);
      }
#endif
      // Start the service on booting up
      if (!this.startService()) {
        this.stopService();
      }
    },

    observe: function sm_observe(subject, topic, data) {
      debug("observe: " + topic + ", " + data);

      switch (topic) {
#ifdef MOZ_WIDGET_GONK
        case "network-active-changed": {
          if (!subject) {
            // Stop service when there is no active network
            this.stopService();
            break;
          }

          // Start service when active network change with new IP address
          // Other case will be handled by "network:offline-status-changed"
          if (!Services.io.offline) {
            this.startService();
          }

          break;
        }
#endif
        case "network:offline-status-changed": {
          if (data == "offline") {
            // Stop service when network status change to offline
            this.stopService();
          } else { // online
            // Resume service when network status change to online
            this.startService();
          }
          break;
        }
        case "nsPref:changed": {
          if (data != kPrefPresentationDiscoverable) {
            break;
          }

          // Start service if discoverable is true. Otherwise stop service.
          if (!this.startService()) {
            this.stopService();
          }

          break;
        }
        default:
          break;
      }
    },
#ifdef MOZ_WIDGET_GONK
    onTimeChange: function sm_onTimeChange() {
      debug("onTimeChange");
      // Notice remote-control service the time is changed
      this.discoverable &&
      remoteControlScope.RemoteControlService &&
      remoteControlScope.RemoteControlService.onTimeChange(getTime());
    },
#endif
    startService: function sm_startService() {
      debug("startService");
      // Start only when presentation is discoverable
      if (this.discoverable) {
        importRemoteControlService();
        remoteControlScope.RemoteControlService.start();
        return true;
      }
      return false;
    },

    stopService: function sm_stopService() {
      debug("stopService");
      remoteControlScope.RemoteControlService &&
      remoteControlScope.RemoteControlService.stop();
    },

    get discoverable() {
      return Services.prefs.getBoolPref(kPrefPresentationDiscoverable);
    },
  };

  statusMonitor.init();
})();
