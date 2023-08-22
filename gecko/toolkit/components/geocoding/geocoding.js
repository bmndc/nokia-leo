/* -*- indent-tabs-mode: nil; js-indent-level: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (C) 2016 Kai OS Technologies. All rights reserved.
 */

'use strict';

const { classes: Cc, interfaces: Ci, results: Cr, utils: Cu } = Components;

Cu.import('resource://gre/modules/XPCOMUtils.jsm');
Cu.import('resource://gre/modules/Services.jsm');

const DEBUG = false; // set to true to show debug messages

const kGEOCODING_CONTRACTID = '@kaiostech.com/toolkit/geo-coding;1';
const kGEOCODING_CID        = Components.ID('{bd541cf0-9b3f-11e6-bdf4-0800200c9a66}');
const kGEOCODINGINFO_CID    = Components.ID("{411cd940-a0a0-11e6-bdf4-0800200c9a66}");

function GeoCodingInfo() {
  this.latitude = null;
  // TODO: fix typo, should be 'longitude' instead of 'longtitude'
  this.longtitude = null;
  this.city = null;
  this.state = null;
  this.country = null;
  this.countryCode = null;
  this.zip = null;
}
GeoCodingInfo.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIGeoCodingInfo]),
  classID:        kGEOCODINGINFO_CID,
  classInfo:      XPCOMUtils.generateCI({
    classID:          kGEOCODINGINFO_CID,
    classDescription: "GeoCodingInfo",
    interfaces:       [Ci.nsIGeoCodingInfo]
  })
};

function URLFetcher(url, timeout) {
  let self = this;
  let xhr = Cc['@mozilla.org/xmlextras/xmlhttprequest;1']
              .createInstance(Ci.nsIXMLHttpRequest);
  xhr.open('GET', url, true);
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

  xhr.timeout = timeout;
  xhr.ontimeout = function () { self.ontimeout(); };
  xhr.onerror = function () { self.onerror(); };
  xhr.onreadystatechange = function(oEvent) {
    if (xhr.readyState === 4) {
      if (self._isAborted) {
        return;
      }
      if (xhr.status === 200) {
        self.onsuccess(xhr.responseText);
      } else if (xhr.status) {
        self.onredirectorerror(xhr.status);
      }
    }
  };
  xhr.send();
  this._xhr = xhr;
};

URLFetcher.prototype = {
  _isAborted: false,
  ontimeout: function() {},
  onerror: function() {},
  abort: function() {
    if (!this._isAborted) {
      this._isAborted = true;
      this._xhr.abort();
    }
  },
};

function GeoCoding() {
  // Load preference
  this._geoCodingURL = null;
  // key needs to be applied from Google, and store somewhere else.
  this._canonicalSiteKey = null;

  try {
    // _geoCodingURL = 'https://maps.googleapis.com/maps/api/geocode/json?latlng=';
    this._geoCodingURL =
      Services.prefs.getCharPref('google.geocoding.URL');
    this._canonicalSiteKey =
      Services.prefs.getCharPref('google.geocoding.Key');
  } catch(e) {
    debug('google.geocoding.URL or google.geocoding.Key not set.');
  }

  this._maxWaitingTime =
    Services.prefs.getIntPref('google.geocoding.maxWaitingTime');
  this._maxRetryCount =
    Services.prefs.getIntPref('google.geocoding.maxRetryCount');
  debug('Load Prefs {site=' + this._geoCodingURL + ' ,time=' + this._maxWaitingTime
        + " ,max-retry=" + this._maxRetryCount + '}');

  this._runningRequest = null;
  this._requestQueue = []; // Maintain a progress table, store callbacks and the ongoing XHR

  this._geoCodingInfo = new GeoCodingInfo();

  debug('GeoCoding component initiated');
}

GeoCoding.prototype = {
  classID:   kGEOCODING_CID,
  classInfo: XPCOMUtils.generateCI({classID: kGEOCODING_CID,
                                    contractID: kGEOCODING_CONTRACTID,
                                    classDescription: 'GEO Coding',
                                    interfaces: [Ci.nsIGeoCoding]}),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIGeoCoding]),


  _runNextRequest: function _runNextRequest() {
    let nextRequest = this._requestQueue.shift();
    if (nextRequest) {
      this._runningRequest = nextRequest;
      this._startGeoCodingRequest(nextRequest);
    }
  },

  _addRequest: function _addRequest(request) {
    this._requestQueue.push(request);
    if (!this._runningRequest) {
      this._runNextRequest();
    }
  },

  _mayRetry: function _mayRetry() {
    if (this._runningRequest.retryCount++ < this._maxRetryCount) {
      debug('retry : ' + this._runningRequest.retryCount + '/' + this._maxRetryCount);
      this._startGeoCodingRequest(this._runningRequest);
    } else {
        this._runningRequest['cause'] = 'Too many retries';
        this.executeCallback(false);
    }
  },

  _startGeoCodingRequest: function _startGeoCodingRequest(request) {
    let self = this;

    let geoCodingURL = this._geoCodingURL + request['latitude'] + ',' +
      request['longitude'] + '&key=' + this._canonicalSiteKey;
    debug('geoCodingURL = ' + geoCodingURL + ' ,maxWaitingTim = '
      + this._maxWaitingTime);
    let urlFetcher = new URLFetcher(geoCodingURL, this._maxWaitingTime);
    let mayRetry = this._mayRetry.bind(this);

    urlFetcher.ontimeout = mayRetry;
    urlFetcher.onerror = mayRetry;
    urlFetcher.onsuccess = function (content) {
      let resp = JSON.parse(content);
      debug('result : ' + resp.status);
      if (resp.status !== 'OK') {
        self._runningRequest['cause'] = resp.status;
        self.executeCallback(false);
      } else {
        // Parse city, state, country and zip code.
        (resp.results[0].address_components).forEach(function(aComponent) {
          let types = aComponent.types;
          if (types.indexOf('postal_code') != -1) {
            self._geoCodingInfo.zip = aComponent.short_name;
          } else if (types.indexOf('country') != -1)  {
            self._geoCodingInfo.country = aComponent.long_name;
            self._geoCodingInfo.countryCode = aComponent.short_name;
          } else if (types.indexOf('administrative_area_level_1') != -1) {
            self._geoCodingInfo.state = aComponent.short_name;
          } else if (types.indexOf('locality') != -1) {
            self._geoCodingInfo.city = aComponent.short_name;
          }
        });
        debug('Parsed result : ' + JSON.stringify(self._runningRequest));
        self.executeCallback(true);
      }
    };
    urlFetcher.onredirectorerror = function (status) {
        self._runningRequest['cause'] = 'Redirect error';
        self.executeCallback(false);
    };

    this._runningRequest['urlFetcher'] = urlFetcher;
  },

  executeCallback: function executeCallback(success) {
    if (this._runningRequest) {
      debug('callback execution with success = ' + success);
      if (this._runningRequest.hasOwnProperty('callback')) {
        if (success) {
          this._runningRequest.callback.onComplete(this._geoCodingInfo);
        } else {
          this._runningRequest.callback.onError(this._runningRequest['cause']);
        }
      }
      this._runningRequest = null;
      this._runNextRequest();
    }
  },

  // nsIGeoCoding
  requestGeoCoding: function requestGeoCoding(aLatitude, aLongitude, aCallback) {
    if (!aCallback) {
      throw Components.Exception('No callback set up.');
    }

    let request = {};
    let callback = aCallback.QueryInterface(Ci.nsIGeoCodingCallback);

    if (Services.io.offline) {
      debug('Service is offline');
      callback.onError('Service is offline');
      return;
    }

    if (!this._geoCodingURL) {
      callback.onError('No google.geocoding.URL set up.');
      return;
    }

    if (!this._canonicalSiteKey) {
      callback.onError('No google.geocoding.Key set up.');
      return;
    }

    request['callback'] = callback;
    this._geoCodingInfo.latitude = aLatitude;
    request['latitude'] = aLatitude;
    // TODO: fix typo, should be 'longitude' instead of 'longtitude'
    this._geoCodingInfo.longtitude = aLongitude;
    request['longitude'] = aLongitude;
    request['retryCount'] = 0;

    this._addRequest(request);
  },
};

let debug;
if (DEBUG) {
  debug = function (s) {
    dump('-*- GeoCoding component: ' + s + '\n');
  };
} else {
  debug = function (s) {};
}

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([GeoCoding]);