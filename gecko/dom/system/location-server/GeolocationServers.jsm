/* (c) 2020 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

"use strict";

this.EXPORTED_SYMBOLS = ["GeolocationServers"];

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");

const Ci = Components.interfaces;

// Supported 3rd party location servers
const COMBAIN = "combain";
const SKYHOOK = "skyhook";
const COMBAIN_SERVER_URI = "https://kaioslocate.combain.com";
const SKYHOOK_SERVER_URI = "https://global.skyhookwireless.com/wps2/location";

XPCOMUtils.defineLazyModuleGetter(this, "SkyhookGeoHelper",
                                  "resource://gre/modules/SkyhookGeoHelper.jsm");

function WifiGeoCoordsObject(lat, lon, acc, alt, altacc) {
  this.latitude = lat;
  this.longitude = lon;
  this.accuracy = acc;
  this.altitude = alt;
  this.altitudeAccuracy = altacc;
}

WifiGeoCoordsObject.prototype = {
  QueryInterface:  XPCOMUtils.generateQI([Ci.nsIDOMGeoPositionCoords])
};

function WifiGeoPositionObject(lat, lng, acc, source) {
  this.coords = new WifiGeoCoordsObject(lat, lng, acc, 0, 0);
  this.address = null;
  this.timestamp = Date.now();
  this.positioningMethod = source;
  // can't get GNSS time from network geolocation provider
  this.gnssUtcTime = 0;
}

WifiGeoPositionObject.prototype = {
  QueryInterface:   XPCOMUtils.generateQI([Ci.nsIDOMGeoPositionExtended])
};

this.GeolocationServers = {
  activeServcie: {
    name: "",
    uri: "",
    key: ""
  },

  e911: false,

  _dumpLog: function(aMsg) {
    aMsg = "GeolocationServers_GEO: " + aMsg;
    dump(aMsg);
  },

  _getSystemUri: function() {
    return Services.urlFormatter.formatURLPref("geo.wifi.uri");
  },

  _getSystemKey: function() {
    return Services.urlFormatter.formatURLPref("geo.authorization.key");
  },

  init: function() {
    let systemUri = this._getSystemUri();
    let systemKey = this._getSystemKey();

    if (systemUri && systemKey) {
      switch(systemUri) {
        case COMBAIN_SERVER_URI:
          this.activeServcie.name = COMBAIN;
          break;
        case SKYHOOK_SERVER_URI:
          this.activeServcie.name = SKYHOOK;
          break;
        default:
          this.activeServcie.name = "";
      }

      if (this.activeServcie.name) {
        this.activeServcie.uri = systemUri;
        this.activeServcie.key = systemKey;
      }
    }
  },

  // whether the server use XML instead of JSON for HTTP request/response
  useXml: function() {
    // Currently, only Skyhook use XML
    if (this.activeServcie.name === SKYHOOK) {
      return true;
    }
    return false;
  },

  // stringify the request data from JSON to string.
  // the return value would be null if the server doesn't need request body.
  stringifyRequestData: function(data, enableLogging) {
    if (!data) return null;
    switch (this.activeServcie.name) {
      case SKYHOOK:
        return SkyhookGeoHelper.generateXmlRequest(
          data.wifiAccessPoints,
          data.cellTowers,
          enableLogging,
          this.activeServcie.key);
      case COMBAIN:
      default: // default KaiOS server
        return JSON.stringify(data);
    }
  },

  // gennerate the URL of HTTP request with parameters
  generateUrl: function () {
    let url = this.activeServcie.uri;

    switch (this.activeServcie.name) {
      case COMBAIN:
        url += "?key=" + this.activeServcie.key;
        break;
      case SKYHOOK:
         // Skyhook server doesn't add any parameter via request url
        break;
      default:
        // Kai server doesn't add any parameter via request url
        break;
    }
    return url;
  },

  // whether the http response of XHR is valid
  isValidResponse: function (xhr) {
    if (!xhr) return false;
    if (!xhr.response && !xhr.responseXML) return false;

    switch (this.activeServcie.name) {
      case SKYHOOK:
        return (xhr.responseXML instanceof Ci.nsIDOMXMLDocument);
      case COMBAIN:
        // fallthrough
      default:  // default KaiOS server
        return !!xhr.response.location;
    }
  },

  // create WifiGeoPositionObject by the HTTP response
  createLocationObject: function (response, source, enableLogging) {
    if (!response) return false;

    let latitude, longitude, accuracy;
    switch (this.activeServcie.name) {
      case SKYHOOK: {
        let location = SkyhookGeoHelper.getLocationFromXmlResponse(response, enableLogging);
        latitude = location.latitude;
        longitude = location.longitude;
        accuracy = location.accuracy;
        break;
      }
      case COMBAIN:
        // fallthrough
      default:
        latitude = response.location.lat;
        longitude = response.location.lng;
        accuracy = response.accuracy;
        break;
    }
    return new WifiGeoPositionObject(latitude, longitude, accuracy, source);
  },

  // Enable/disable E911 API key/uri as the active network location server
  setE911: function(enable) {
    this.e911 = enable;
    this._updateActiveService();
  },

  // Return true if E911 API key/uri is used. Otherwise, return false.
  getE911: function() {
    return this.e911;
  },

  // Whether do we have API key for the active service.
  hasKey: function() {
    return this.activeServcie.key != "";
  },

  _getE911Uri: function() {
    return Services.urlFormatter.formatURLPref("geo.wifi.e911_uri");
  },

  _getE911Key: function() {
    return Services.urlFormatter.formatURLPref("geo.authorization.e911_key");
  },

  _getServerName: function(uri, key) {
    if (uri && key) {
      switch(uri) {
        case COMBAIN_SERVER_URI:
          return COMBAIN;
        case SKYHOOK_SERVER_URI:
          return SKYHOOK;
        default:
          return "";
      }
    }
    return "";
  },

  _updateActiveService: function() {
    let name, uri, key;
    if (this.e911) {
      uri = this._getE911Uri();
      key = this._getE911Key();
      name = this._getServerName(uri, key);
      if (name) {
        this._dumpLog("use E911 API key of [" + name + "] for network positioning");
        this.activeServcie = {name, uri, key};
        return;
      }
    }

    uri = this._getSystemUri();
    key = this._getSystemKey();
    name = this._getServerName(uri, key);
    if (name) {
      this.activeServcie = {name, uri, key};
    } else {
      this.activeServcie = {name: "", uri: "", key: ""};
    }
  },

};

GeolocationServers.init();
