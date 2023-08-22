/* (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

"use strict";

this.EXPORTED_SYMBOLS = ["SkyhookGeoHelper"];

Components.utils.import("resource://gre/modules/Services.jsm");

// Relative age of cell info., in milliseconds.
const kAgeElementCell = '<age>20</age>';

// Relative age of WiFI AP, in milliseconds.
const kAgeElementWiFi = '<age>350</age>';

// SkyhookGeoHelper is implemented based on Skyhook Location API v2.28
// https://resources.skyhookwireless.com/wiki/type/documentation/precision-location/xml--location-api-v228/199269041
this.SkyhookGeoHelper = {
  _hashedDeviceId: null,

  _dumpLog: function(aMsg) {
    aMsg = "Skyhook_GEO: " + aMsg;
    dump(aMsg);
  },

  generateXmlRequest: function(wifiAccessPoints, cellTowers, enableLogging, apiKey) {
    if (!this._hashedDeviceId) {
      this._hashedDeviceId = Services.prefs.getCharPref('app.update.imei_hash');
    }

    let wifiXmlString = this._wifiApListToXmlString(wifiAccessPoints);
    let cellXmlString = this._cellTowerListToXmlString(cellTowers);

    if (enableLogging) {
      this._dumpLog("WiFi xml: " + wifiXmlString);
      this._dumpLog("Cell xml: " + cellXmlString);
    }

    // The default HPE(Horizontal Positioning Error) of v2.28 is 68%.
    // Set the HPE to 95% to follow the confidence level of W3C Geolocatoin API
    // Use hashed IMEI as 'username' which can be used to improve accuracy and
    // count active Skyhook devices.
    return '<LocationRQ xmlns="http://skyhookwireless.com/wps/2005" version="2.28" hpe-confidence="95">' +
           '<authentication version="2.2">' +
           `<key key="${apiKey}" username="${this._hashedDeviceId}"/>` +
           '</authentication>' +
           wifiXmlString +
           cellXmlString +
           '</LocationRQ>';
  },

  getLocationFromXmlResponse: function(xmlDoc, enableLogging) {
    let latList = xmlDoc.getElementsByTagName("latitude");
    let lonList = xmlDoc.getElementsByTagName("longitude");
    let hpeList = xmlDoc.getElementsByTagName("hpe"); // Horizontal Positioning Error, estimated in meters

    let ret = {};
    ret.latitude  = latList.length ? parseFloat(latList[0].textContent) : 0.0;
    ret.longitude = lonList.length ? parseFloat(lonList[0].textContent) : 0.0;
    ret.accuracy = hpeList.length ? parseFloat(hpeList[0].textContent) : 99999.0;

    if (enableLogging) {
      this._dumpLog("lat: " + ret.latitude + ", lng: " + ret.longitude + ", acc: " + ret.accuracy);
    }

    return ret;
  },

  _wifiApListToXmlString: function(wifiAccessPoints) {
    if (!wifiAccessPoints) return '';
    let ret = '';
    for (let i = 0; i < wifiAccessPoints.length; i++) {
      ret += this._wifiApToXmlString(wifiAccessPoints[i]);
    }
    return ret;
  },

  _wifiApToXmlString: function(wifiAccessPoint) {
    // convert mac address from xx-xx-xx-xx-xx-xx to XXXXXXXXXXXX
    wifiAccessPoint.macAddress
      = wifiAccessPoint.macAddress.replace(/-/g, '').toUpperCase();
    //  mac address should be a string with 12 letters
    if (wifiAccessPoint.macAddress.length != 12) {
      return '';
    }

    // & and < must not appear in their literal form in XML
    wifiAccessPoint.ssid = wifiAccessPoint.ssid.replace("&", "&amp;");
    wifiAccessPoint.ssid = wifiAccessPoint.ssid.replace("<", "&lt;");

    let ret = '<access-point>';
    ret += '<mac>' + wifiAccessPoint.macAddress + '</mac>';
    ret += '<ssid>' + wifiAccessPoint.ssid + '</ssid>';
    ret += '<signal-strength>' + wifiAccessPoint.signalStrength + '</signal-strength>';
    ret += kAgeElementWiFi;
    ret += '</access-point>';
    return ret;
  },

  _cellTowerListToXmlString: function(cellTowers) {
    if (!cellTowers) return '';
    let ret = '';
    for (let i = 0; i < cellTowers.length; i++) {
      ret += this._cellTowerToXmlString(cellTowers[i]);
    }
    return ret;
  },

  _cellTowerToXmlString: function(cellTower) {
    let ret = '';
    switch (cellTower.radioType) {
      case 'gsm':
        ret = '<gsm-tower>';
        ret += '<mcc>' + cellTower.mobileCountryCode + '</mcc>';
        ret += '<mnc>' + cellTower.mobileNetworkCode + '</mnc>';
        ret += '<lac>' + cellTower.locationAreaCode + '</lac>';
        ret += '<ci>' + cellTower.cellId + '</ci>';
        ret += '<rssi>' + cellTower.signalStrength + '</rssi>';
        ret += kAgeElementCell;
        ret += '</gsm-tower>';
        break;
      case 'wcdma':
        ret = '<umts-tower>';
        ret += '<mcc>' + cellTower.mobileCountryCode + '</mcc>';
        ret += '<mnc>' + cellTower.mobileNetworkCode + '</mnc>';
        ret += '<lac>' + cellTower.locationAreaCode + '</lac>';
        ret += '<ci>' + cellTower.cellId + '</ci>';
        ret += '<rscp>' + cellTower.signalStrength + '</rscp>';
        ret += kAgeElementCell;
        ret += '</umts-tower>';
        break;
      case 'lte':
        ret = '<lte-tower>';
        ret += '<mcc>' + cellTower.mobileCountryCode + '</mcc>';
        ret += '<mnc>' + cellTower.mobileNetworkCode + '</mnc>';
        ret += '<eucid>' + cellTower.cellId + '</eucid>';
        ret += '<rsrp>' + cellTower.signalStrength + '</rsrp>';
        ret += kAgeElementCell;
        ret += '</lte-tower>';
        break;
      case 'cdma':
        // CDMA isn't supported, fallthrough
        // <cdma-tower>
        //   <sid>###</sid>
        //   <nid>###</nid>
        //   <bsid>###</bsid>
        //   <rssi>###</rssi>
        // </cdma-tower>
      default:
        return '';
    }
    return ret;
  },
};
