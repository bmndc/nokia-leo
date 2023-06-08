/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/Promise.jsm");
Components.utils.import("resource://gre/modules/DeviceUtils.jsm");

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cu = Components.utils;

const POSITION_UNAVAILABLE = Ci.nsIDOMGeoPositionError.POSITION_UNAVAILABLE;
const SETTINGS_DEBUG_ENABLED = "geolocation.debugging.enabled";
const SETTINGS_CHANGED_TOPIC = "mozsettings-changed";
const NETWORK_CHANGED_TOPIC = "network-active-changed";

const SETTINGS_WIFI_ENABLED = "wifi.enabled";
const GEO_KAI_HAWK_KID = "geolocation.kaios.hawk_key_id";
const GEO_KAI_HAWK_MAC = "geolocation.kaios.hawk_mac_key";

const HTTP_CODE_OK = 200;
const HTTP_CODE_CREATED = 201;
const HTTP_CODE_BAD_REQUEST = 400;
const HTTP_CODE_UNAUTHORIZED = 401;

// Define a cooldown to prevent overly retrying in case the location server
// have any internal errors.
const TOKEN_REFRESH_COOLDOWN_IN_MS = 1800000; // half hour

XPCOMUtils.defineLazyServiceGetter(this, "gMobileConnectionService",
                                   "@mozilla.org/mobileconnection/mobileconnectionservice;1",
                                   "nsIMobileConnectionService");

#ifdef HAS_KOOST_MODULES
// it's ok to use lazy getter here since other module would use hawkHelper first
XPCOMUtils.defineLazyServiceGetter(this, "hawkHelper",
                                   "@kaiostech.com/kaiauth/hawk-helper;1",
                                   "nsIHawkHelper");
#endif

XPCOMUtils.defineLazyServiceGetter(this, "gNetworkManager",
                                   "@mozilla.org/network/manager;1",
                                   "nsINetworkManager");

XPCOMUtils.defineLazyModuleGetter(this, "Preferences",
                                  "resource://gre/modules/Preferences.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "GeolocationServers",
                                  "resource://gre/modules/GeolocationServers.jsm");

var gLoggingEnabled = false;

/*
   The gLocationRequestTimeout controls how long we wait on receiving an update
   from the Wifi subsystem.  If this timer fires, we believe the Wifi scan has
   had a problem and we no longer can use Wifi to position the user this time
   around (we will continue to be hopeful that Wifi will recover).

   This timeout value is also used when Wifi scanning is disabled (see
   gWifiScanningEnabled).  In this case, we use this timer to collect cell/ip
   data and xhr it to the location server.
*/

var gLocationRequestTimeout = 5000;

var gWifiScanningEnabled = true;
var gCellScanningEnabled = false;

// the cache of restricted token
var gRestrictedToken = null;

// timestamp of the cached restricted token
var gTokenRefreshedTimestamp = 0;

function LOG(aMsg) {
  if (gLoggingEnabled) {
    aMsg = "*** WIFI GEO: " + aMsg + "\n";
    Cc["@mozilla.org/consoleservice;1"].getService(Ci.nsIConsoleService).logStringMessage(aMsg);
    dump(aMsg);
  }
}

function ERR(aMsg) {
    aMsg = "*** WIFI GEO ERR: " + aMsg + "\n";
    Cc["@mozilla.org/consoleservice;1"].getService(Ci.nsIConsoleService).logStringMessage(aMsg);
    dump(aMsg);
}

function CachedRequest(loc, cellInfo, wifiList) {
  this.location = loc;

  let wifis = new Set();
  if (wifiList) {
    for (let i = 0; i < wifiList.length; i++) {
      wifis.add(wifiList[i].macAddress);
    }
  }

  // Use only these values for equality
  // (the JSON will contain additional values in future)
  function makeCellKey(cell) {
    return "" + cell.radio + ":" + cell.mobileCountryCode + ":" +
    cell.mobileNetworkCode + ":" + cell.locationAreaCode + ":" +
    cell.cellId;
  }

  let cells = new Set();
  if (cellInfo) {
    for (let i = 0; i < cellInfo.length; i++) {
      cells.add(makeCellKey(cellInfo[i]));
    }
  }

  this.hasCells = () => cells.size > 0;

  this.hasWifis = () => wifis.size > 0;

  // if fields match
  this.isCellEqual = function(cellInfo) {
    if (!this.hasCells()) {
      return false;
    }

    let len1 = cells.size;
    let len2 = cellInfo.length;

    if (len1 != len2) {
      LOG("cells not equal len");
      return false;
    }

    for (let i = 0; i < len2; i++) {
      if (!cells.has(makeCellKey(cellInfo[i]))) {
        return false;
      }
    }
    return true;
  };

  // if 50% of the SSIDS match
  this.isWifiApproxEqual = function(wifiList) {
    if (!this.hasWifis()) {
      return false;
    }

    // if either list is a 50% subset of the other, they are equal
    let common = 0;
    for (let i = 0; i < wifiList.length; i++) {
      if (wifis.has(wifiList[i].macAddress)) {
        common++;
      }
    }
    let kPercentMatch = 0.5;
    return common >= (Math.max(wifis.size, wifiList.length) * kPercentMatch);
  };

  this.isGeoip = function() {
    return !this.hasCells() && !this.hasWifis();
  };

  this.isCellAndWifi = function() {
    return this.hasCells() && this.hasWifis();
  };

  this.isCellOnly = function() {
    return this.hasCells() && !this.hasWifis();
  };

  this.isWifiOnly = function() {
    return this.hasWifis() && !this.hasCells();
  };
 }

var gCachedRequest = null;
var gDebugCacheReasoning = ""; // for logging the caching logic

// This function serves two purposes:
// 1) do we have a cached request
// 2) is the cached request better than what newCell and newWifiList will obtain
// If the cached request exists, and we know it to have greater accuracy
// by the nature of its origin (wifi/cell/geoip), use its cached location.
//
// If there is more source info than the cached request had, return false
// In other cases, MLS is known to produce better/worse accuracy based on the
// inputs, so base the decision on that.
function isCachedRequestMoreAccurateThanServerRequest(newCell, newWifiList)
{
  gDebugCacheReasoning = "";
  let isNetworkRequestCacheEnabled = true;
  try {
    // Mochitest needs this pref to simulate request failure
    isNetworkRequestCacheEnabled = Services.prefs.getBoolPref("geo.wifi.debug.requestCache.enabled");
    if (!isNetworkRequestCacheEnabled) {
      gCachedRequest = null;
    }
  } catch (e) {}

  if (!gCachedRequest || !isNetworkRequestCacheEnabled) {
    gDebugCacheReasoning = "No cached data";
    return false;
  }

  if (!newCell && !newWifiList) {
    gDebugCacheReasoning = "New req. is GeoIP.";
    return true;
  }

  if (newCell && newWifiList && (gCachedRequest.isCellOnly() || gCachedRequest.isWifiOnly())) {
    gDebugCacheReasoning = "New req. is cell+wifi, cache only cell or wifi.";
    return false;
  }

  if (newCell && gCachedRequest.isWifiOnly()) {
    // In order to know if a cell-only request should trump a wifi-only request
    // need to know if wifi is low accuracy. >5km would be VERY low accuracy,
    // it is worth trying the cell
    var isHighAccuracyWifi = gCachedRequest.location.coords.accuracy < 5000;
    gDebugCacheReasoning = "Req. is cell, cache is wifi, isHigh:" + isHighAccuracyWifi;
    return isHighAccuracyWifi;
  }

  let hasEqualCells = false;
  if (newCell) {
    hasEqualCells = gCachedRequest.isCellEqual(newCell);
  }

  let hasEqualWifis = false;
  if (newWifiList) {
    hasEqualWifis = gCachedRequest.isWifiApproxEqual(newWifiList);
  }

  gDebugCacheReasoning = "EqualCells:" + hasEqualCells + " EqualWifis:" + hasEqualWifis;

  if (gCachedRequest.isCellOnly()) {
    gDebugCacheReasoning += ", Cell only.";
    if (hasEqualCells) {
      return true;
    }
  } else if (gCachedRequest.isWifiOnly() && hasEqualWifis) {
    gDebugCacheReasoning +=", Wifi only."
    return true;
  } else if (gCachedRequest.isCellAndWifi()) {
     gDebugCacheReasoning += ", Cache has Cell+Wifi.";
    if ((hasEqualCells && hasEqualWifis) ||
        (!newWifiList && hasEqualCells) ||
        (!newCell && hasEqualWifis))
    {
     return true;
    }
  }

  return false;
}

function WifiGeoPositionProvider() {
  try {
    gLoggingEnabled = Services.prefs.getBoolPref("geo.wifi.logging.enabled");
  } catch (e) {}

  try {
    gLocationRequestTimeout = Services.prefs.getIntPref("geo.wifi.timeToWaitBeforeSending");
  } catch (e) {}

  try {
    gWifiScanningEnabled = Services.prefs.getBoolPref("geo.wifi.scan");
  } catch (e) {}

  try {
    gCellScanningEnabled = Services.prefs.getBoolPref("geo.cell.scan");
  } catch (e) {}

  this.wifiService = null;
  this.timer = null;
  this.started = false;
  this.serverUri = Services.urlFormatter.formatURLPref("geo.wifi.uri");
}

WifiGeoPositionProvider.prototype = {
  classID:          Components.ID("{77DA64D3-7458-4920-9491-86CC9914F904}"),
  QueryInterface:   XPCOMUtils.generateQI([Ci.nsIGeolocationProvider,
                                           Ci.nsIGeolocationNetworkProvider,
                                           Ci.nsIWifiListener,
                                           Ci.nsITimerCallback,
                                           Ci.nsIObserver]),
  listener: null,

  observe: function(aSubject, aTopic, aData) {
    switch (aTopic) {
    case SETTINGS_CHANGED_TOPIC:
      try {
        if ("wrappedJSObject" in aSubject) {
          aSubject = aSubject.wrappedJSObject;
        }
        if (aSubject.key == SETTINGS_DEBUG_ENABLED) {
          gLoggingEnabled = aSubject.value;
        } else if (aSubject.key == SETTINGS_WIFI_ENABLED) {
          gWifiScanningEnabled = aSubject.value;
          if (this.wifiService) {
            this.wifiService.stopWatching(this);
            this.wifiService = null;
          }
          if (gWifiScanningEnabled && this.isActive()) {
            this.wifiService = Cc["@mozilla.org/wifi/monitor;1"].getService(Ci.nsIWifiMonitor);
            this.wifiService.startWatching(this);
          }
        }
      } catch (e) {}
      break;
    case NETWORK_CHANGED_TOPIC:
      // aSubject will be a nsINetworkInfo if network is connected,
      // otherwise, aSubject should be null.
      if (aSubject) {
        this.hasNetwork = true;
        if (gWifiScanningEnabled && !this.wifiService && this.isActive()) {
          this.wifiService = Cc["@mozilla.org/wifi/monitor;1"].getService(Ci.nsIWifiMonitor);
          this.wifiService.startWatching(this);
        }
        this.resetTimer();
      } else {
        this.hasNetwork = false;
        if (this.wifiService) {
          this.wifiService.stopWatching(this);
          this.wifiService = null;
        }
      }
      break;
    }
  },

  resetTimer: function() {
    if (this.timer) {
      this.timer.cancel();
      this.timer = null;
    }

    // Stop the timer if network/API key aren't available
    if (!this.isActive()) {
      return;
    }

    // wifi thread triggers WifiGeoPositionProvider to proceed, with no wifi, do manual timeout
    this.timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    this.timer.initWithCallback(this,
                                gLocationRequestTimeout,
                                this.timer.TYPE_REPEATING_SLACK);
  },

  startup:  function() {
    if (this.started)
      return;

    // Check whether there are any active network
    if (gNetworkManager && gNetworkManager.activeNetworkInfo) {
      this.hasNetwork = true;
    } else {
      this.hasNetwork = false;
      LOG("startup: has no active network.");
    }

    this.started = true;
    let self = this;
    let settingsCallback = {
      handle: function(name, result) {
        // Stop the B2G UI setting from overriding the js prefs setting, and turning off logging
        // If gLoggingEnabled is already on during startup, that means it was set in js prefs.
        if (name == SETTINGS_DEBUG_ENABLED && !gLoggingEnabled) {
          gLoggingEnabled = result;
        } else if (name == SETTINGS_WIFI_ENABLED) {
          gWifiScanningEnabled = result;
          if (self.wifiService) {
            self.wifiService.stopWatching(self);
            self.wifiService = null;
          }
          if (gWifiScanningEnabled && self.isActive()) {
            self.wifiService = Cc["@mozilla.org/wifi/monitor;1"].getService(Ci.nsIWifiMonitor);
            self.wifiService.startWatching(self);
          }
        }
      },

      handleError: function(message) {
        gLoggingEnabled = false;
        LOG("settings callback threw an exception, dropping");
      }
    };

    Services.obs.addObserver(this, SETTINGS_CHANGED_TOPIC, false);
    Services.obs.addObserver(this, NETWORK_CHANGED_TOPIC, false);

    let settingsService = Cc["@mozilla.org/settingsService;1"];
    if (settingsService) {
      let settings = settingsService.getService(Ci.nsISettingsService);
      settings.createLock().get(SETTINGS_WIFI_ENABLED, settingsCallback);
      settings.createLock().get(SETTINGS_DEBUG_ENABLED, settingsCallback);
    }

    this.resetTimer();
    LOG("startup called.");
  },

  watch: function(c) {
    this.listener = c;
  },

  shutdown: function() {
    LOG("shutdown called");
    if (this.started == false) {
      return;
    }

    // Without clearing this, we could end up using the cache almost indefinitely
    // TODO: add logic for cache lifespan, for now just be safe and clear it
    gCachedRequest = null;

    if (this.timer) {
      this.timer.cancel();
      this.timer = null;
    }

    if(this.wifiService) {
      this.wifiService.stopWatching(this);
      this.wifiService = null;
    }

    Services.obs.removeObserver(this, SETTINGS_CHANGED_TOPIC);
    Services.obs.removeObserver(this, NETWORK_CHANGED_TOPIC);

    this.listener = null;
    this.started = false;
  },

  setHighAccuracy: function(enable) {
  },

  setE911: function(enable) {
    LOG("setE911: " + enable);

    if (GeolocationServers.getE911() == enable) {
      return;
    }

    GeolocationServers.setE911(enable);

    if (this.isActive()) {
      if (gWifiScanningEnabled && !this.wifiService) {
        this.wifiService = Cc["@mozilla.org/wifi/monitor;1"].getService(Ci.nsIWifiMonitor);
        this.wifiService.startWatching(this);
      }
    } else {
      if (this.wifiService) {
        this.wifiService.stopWatching(this);
        this.wifiService = null;
      }
    }
    this.resetTimer();
  },

  // Whether the location provider is active
  isActive: function() {
    return this.hasNetwork && GeolocationServers.hasKey();
  },

  // nsIWifiListener.onChange
  onChange: function(accessPoints) {
    // we got some wifi data, rearm the timer.
    this.resetTimer();

    function isPublic(ap) {
      let mask = "_nomap";
      let result = ap.ssid.indexOf(mask, ap.ssid.length - mask.length);
      if (result != -1) {
        LOG("Filtering out " + ap.ssid + " " + result);
        return false;
      }
      return true;
    };

    function sort(a, b) {
      return b.signal - a.signal;
    };

    function encode(ap) {
      return { 'macAddress': ap.mac, 'signalStrength': ap.signal, 'ssid': ap.ssid };
    };

    let wifiData = null;
    if (accessPoints) {
      wifiData = accessPoints.filter(isPublic).sort(sort).map(encode);
    }
    this.sendLocationRequest(wifiData);
  },

  // nsIWifiListener.onError
  onError: function (code) {
    LOG("wifi error: " + code);
    this.sendLocationRequest(null);
  },

  getMobileInfo: function() {
    LOG("getMobileInfo called");
    try {
      let radioService = Cc["@mozilla.org/ril;1"]
                    .getService(Ci.nsIRadioInterfaceLayer);
      let service = gMobileConnectionService;

      let result = [];
      for (let i = 0; i < service.numItems; i++) {
        LOG("Looking for SIM in slot:" + i + " of " + service.numItems);
        let connection = service.getItemByServiceId(i);
        let voice = connection && connection.voice;
        let cell = voice && voice.cell;
        let type = voice && voice.type;
        let network = voice && voice.network;
        let signalStrength = connection && connection.signalStrength;

        if (network && cell && type && signalStrength) {
          let radioTechFamily;
          let mobileSignal;
          switch (type) {
            case "gsm":
            case "gprs":
            case "edge":
              radioTechFamily = "gsm";
              mobileSignal = 2 * signalStrength.gsmSignalStrength - 113; // ASU to dBm
              break;
            case "umts":
            case "hsdpa":
            case "hsupa":
            case "hspa":
            case "hspa+":
              radioTechFamily = "wcdma";
              mobileSignal = 2 * signalStrength.gsmSignalStrength - 113; // ASU to dBm
              break;
            case "lte":
              radioTechFamily = "lte";
              // lteSignalStrength, ASU format, value range (0-31, 99)
              mobileSignal = signalStrength.lteSignalStrength;
              if (mobileSignal !== 99 && mobileSignal >= 0 && mobileSignal <= 31) {
                mobileSignal = 2 * signalStrength.lteSignalStrength - 113; // ASU to dBm
              } else {
                // lteSignalStrength is invalid value, try lteRsrp.
                // Reference Signal Receive Power in dBm, range: -140 to -44 dBm.
                var rsrp = signalStrength.lteRsrp;

                if (rsrp != Ci.nsIMobileSignalStrength.SIGNAL_UNKNOWN_VALUE &&
                    rsrp >= -140 && rsrp <= -44) {
                  mobileSignal = rsrp;
                } else {
                  ERR("Can't find valid LTE signal strength");
                  mobileSignal = undefined;
                }
              }
              break;
            // CDMA cases to be handled in bug 1010282
          }
          result.push({ radioType: radioTechFamily,
                      mobileCountryCode: parseInt(voice.network.mcc, 10),
                      mobileNetworkCode: parseInt(voice.network.mnc, 10),
                      locationAreaCode: cell.gsmLocationAreaCode,
                      cellId: cell.gsmCellId,
                      signalStrength: mobileSignal });
        }
      }
      return result;
    } catch (e) {
      return null;
    }
  },

  // nsITimerCallback.notify
  notify: function (timer) {
    if (!this.isActive()) {
      // Cancel the timer if network/API key aren't available
      if (this.timer) {
        this.timer.cancel();
        this.timer = null;
      }
      return;
    }

    this.sendLocationRequest(null);
  },

  // fetch restricted token and cache it as 'gRestrictedToken'
  fetchRestrictedToken: function() {
    if (Date.now() - gTokenRefreshedTimestamp <= TOKEN_REFRESH_COOLDOWN_IN_MS) {
      LOG("can't fetch restricted token due to the cooldown protection.");
      return Promise.reject(HTTP_CODE_UNAUTHORIZED);
    }

    let uri, apiKeyName;
    try {
      uri = Services.prefs.getCharPref("geo.token.uri");
      apiKeyName = Services.prefs.getCharPref("geo.authorization.key");
    } catch(e) {
      ERR("can't fetch restricted token due to the failure of PREF reading.");
      return Promise.reject(HTTP_CODE_BAD_REQUEST);
    }

    LOG("fetching restricted token from " + uri);
    return DeviceUtils.fetchAccessToken(uri, apiKeyName)
      .then((credential) => {
        LOG("restricted token has been refreshed, Hawk kid:" + credential.kid);
        gRestrictedToken = credential;
        gTokenRefreshedTimestamp = Date.now();

        // Store GEO_KAI_HAWK_MAC and GEO_KAI_HAWK_KID for GeoSubmit
        Preferences.set(GEO_KAI_HAWK_KID, credential.kid);
        Preferences.set(GEO_KAI_HAWK_MAC, credential.mac_key);
      }, (errorStatus) => {
        ERR("failed to fetch restricted token, errorStatus: " + errorStatus);
        gRestrictedToken = null;
        gTokenRefreshedTimestamp = Date.now();
        return Promise.reject(errorStatus);
      }); // end of fetchAccessToken
  },

  sendLocationRequest: function (wifiData) {
    let data = { cellTowers: undefined, wifiAccessPoints: undefined };
    if (wifiData && wifiData.length >= 2) {
      data.wifiAccessPoints = wifiData;
    }

    if (gCellScanningEnabled) {
      let cellData = this.getMobileInfo();
      if (cellData && cellData.length > 0) {
        data.cellTowers = cellData;
      }
    }

    let useCached = isCachedRequestMoreAccurateThanServerRequest(data.cellTowers,
                                                                 data.wifiAccessPoints);

    LOG("Use request cache:" + useCached + " reason:" + gDebugCacheReasoning);

    if (useCached) {
      gCachedRequest.location.timestamp = Date.now();
      this.notifyListener("update", [gCachedRequest.location]);
      return;
    }

    // early return if none of data is available
    if (!data.cellTowers && !data.wifiAccessPoints) {
      return;
    }

    // From here on, do a network geolocation request //
    let url = GeolocationServers.generateUrl();

    let xhr = Components.classes["@mozilla.org/xmlextras/xmlhttprequest;1"]
                        .createInstance(Ci.nsIXMLHttpRequest);

    // request body of HTTP POST
    let requestData = GeolocationServers.stringifyRequestData(data, gLoggingEnabled);
    try {
      xhr.open("POST", url, true);
    } catch (e) {
      LOG("Failed to open XHR.");
      this.notifyListener("notifyError", [POSITION_UNAVAILABLE]);
      return;
    }

    if (GeolocationServers.useXml()) {
      xhr.setRequestHeader("Content-Type", "text/xml; charset=UTF-8");
      xhr.responseType = "xml";
    } else {
      xhr.setRequestHeader("Content-Type", "application/json; charset=UTF-8");
      xhr.responseType = "json";
    }

    // Append authorization header if it's required
    let needAuthorization =
      Services.prefs.getBoolPref("geo.provider.need_authorization");
    if (needAuthorization) {
      if (gRestrictedToken != null) {
        // compute the Hawk header for GeoSubmit
#ifdef HAS_KOOST_MODULES
        let hawkHeader = hawkHelper.computeHawkHeader(
          "POST",
          gRestrictedToken.kid,
          gRestrictedToken.mac_key,
          this.serverUri,
          requestData);
        xhr.setRequestHeader("Authorization", hawkHeader);
#endif
      } else {
        this.sendLocationRequestWithRefreshedToken(wifiData);
        return;
      }
    }

    xhr.mozBackgroundRequest = true;
    xhr.channel.loadFlags = Ci.nsIChannel.LOAD_ANONYMOUS;
    xhr.timeout = Services.prefs.getIntPref("geo.wifi.xhr.timeout");
    xhr.ontimeout = (function() {
      LOG("Location request XHR timed out.")
      this.notifyListener("notifyError", [POSITION_UNAVAILABLE]);
    }).bind(this);
    xhr.onerror = (function() {
      this.notifyListener("notifyError", [POSITION_UNAVAILABLE]);
    }).bind(this);
    xhr.onload = (function() {
      if (GeolocationServers.useXml()) {
        LOG("server returned status: " + xhr.status + " --> " +  xhr.responseText);
      } else {
        LOG("server returned status: " + xhr.status + " --> " +  JSON.stringify(xhr.response));
      }

      if ((xhr.channel instanceof Ci.nsIHttpChannel && xhr.status != HTTP_CODE_OK) ||
          !GeolocationServers.isValidResponse(xhr)) {
        this.notifyListener("notifyError", [POSITION_UNAVAILABLE]);
        if (needAuthorization && xhr.status == HTTP_CODE_UNAUTHORIZED) {
          LOG("got 401 unauthorized. Refreshing restricted token...");
          this.sendLocationRequestWithRefreshedToken(wifiData);
        }
        return;
      }

      let positionSource = 'unknown';
      if (data.cellTowers && data.wifiAccessPoints) {
        positionSource = 'wifi-and-cell';
      } else if (data.cellTowers) {
        positionSource = 'cell';
      } else if (data.wifiAccessPoints) {
        positionSource = 'wifi';
      }

      let newLocation = GeolocationServers.useXml()
        ? GeolocationServers.createLocationObject(xhr.responseXML, positionSource, gLoggingEnabled)
        : GeolocationServers.createLocationObject(xhr.response, positionSource, gLoggingEnabled);

      this.notifyListener("update", [newLocation]);
      gCachedRequest = new CachedRequest(newLocation, data.cellTowers, data.wifiAccessPoints);
    }).bind(this);

    if (GeolocationServers.useXml()) {
      // Do not print 'requestData' since the secret key is stored in XML
      LOG("sending HTTP POST: " + url);
    } else {
      LOG("sending HTTP POST: " + url + " with " + requestData);
    }
    xhr.send(requestData);
  },

  // fetch restricted token and send location request to server
  sendLocationRequestWithRefreshedToken: function(wifiData) {
    this.fetchRestrictedToken().then(
      () => this.sendLocationRequest(wifiData),
      (status_code) => this.notifyListener("notifyError", [POSITION_UNAVAILABLE]));
  },

  notifyListener: function(listenerFunc, args) {
    args = args || [];
    try {
      this.listener[listenerFunc].apply(this.listener, args);
    } catch(e) {
      Cu.reportError(e);
    }
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([WifiGeoPositionProvider]);
