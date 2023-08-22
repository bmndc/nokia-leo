/* -*- indent-tabs-mode: nil; js-indent-level: 2 -*- */
/* vim: set shiftwidth=2 tabstop=2 autoindent cindent expandtab: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/systemlibs.js");
Cu.import("resource://gre/modules/IpClient.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "gNetworkService",
                                   "@mozilla.org/network/service;1",
                                   "nsINetworkService");

this.EXPORTED_SYMBOLS = ["WifiNetUtil"];

const DHCP_PROP = "init.svc.dhcpcd";
const DHCP      = "dhcpcd";
const DEBUG     = false;

this.WifiNetUtil = function(controlMessage) {
  function debug(msg) {
    if (DEBUG) {
      dump('-------------- NetUtil: ' + msg);
    }
  }

  var util = {};

  var ipClient = new IpClient("WiFi");

  util.runDhcp = function (ifname, gen, callback) {
    util.stopDhcp(ifname, function(success) {
      ipClient.requestDhcp(ifname, function(success, dhcpInfo) {
        if (!success) {
          callback({ info: null }, gen);
          return;
        }
        util.runIpConfig(ifname, dhcpInfo, function(data) {
          callback(data, gen);
        });
      });
    });
  };

  util.stopDhcp = function (ifname, callback) {
    ipClient.stopDhcp(ifname, function(success) {
      callback(success);
    });
  };

  util.startDhcpServer = function (config, callback) {
    gNetworkService.setDhcpServer(true, config, function (error) {
      callback(!error);
    });
  };

  util.stopDhcpServer = function (callback) {
    gNetworkService.setDhcpServer(false, null, function (error) {
      callback(!error);
    });
  };

  util.runIpConfig = function (name, data, callback) {
    if (!data) {
      debug("IP config failed to run");
      callback({ info: data });
      return;
    }

    setProperty("net." + name + ".dns1", ipToString(data.dns1),
                function(ok) {
      if (!ok) {
        debug("Unable to set net.<ifname>.dns1");
        return;
      }
      setProperty("net." + name + ".dns2", ipToString(data.dns2),
                  function(ok) {
        if (!ok) {
          debug("Unable to set net.<ifname>.dns2");
          return;
        }
        setProperty("net." + name + ".gw", ipToString(data.gateway),
                    function(ok) {
          if (!ok) {
            debug("Unable to set net.<ifname>.gw");
            return;
          }
          callback({ info: data });
        });
      });
    });
  };

  //--------------------------------------------------
  // Helper functions.
  //--------------------------------------------------

  // Wrapper around libcutils.property_set that returns true if setting the
  // value was successful.
  // Note that the callback is not called asynchronously.
  function setProperty(key, value, callback) {
    let ok = true;
    try {
      libcutils.property_set(key, value);
    } catch(e) {
      ok = false;
    }
    callback(ok);
  }

  function ipToString(n) {
    return String((n >>  0) & 0xFF) + "." +
                 ((n >>  8) & 0xFF) + "." +
                 ((n >> 16) & 0xFF) + "." +
                 ((n >> 24) & 0xFF);
  }

  return util;
};
