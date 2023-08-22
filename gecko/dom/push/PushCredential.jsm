/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/DeviceUtils.jsm");
Cu.import("resource://gre/modules/Preferences.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

this.EXPORTED_SYMBOLS = ["PushCredential"];

const prefs = new Preferences("dom.push.");

XPCOMUtils.defineLazyGetter(this, "console", () => {
  let {
    ConsoleAPI
  } = Cu.import("resource://gre/modules/Console.jsm", {});
  return new ConsoleAPI({
    maxLogLevelPref: "dom.push.loglevel",
    prefix: "PushCredential",
  });
});

const TOKEN_EXPIRATION_TIME_SHIFT = -25000; // shift 25s earlier

this.PushCredential = function () {
  this.init();
};

PushCredential.prototype = {
  init: function () {
    console.debug("init");
    this._credential = {
      token: '',
      requestTime: 0,
      expires_in: 0,
      clockOffset: 0
    };
    this._credentialDeferred = null;
    // cache system clock change during fetching access token
    this._clockOffsetCache = 0;

    Services.obs.addObserver(this, "xpcom-shutdown", false);
    // Monitoring system clock change, and then
    // updating the offset for expirationTime of access token.
    Services.obs.addObserver(this, "system-clock-change", false);
  },

  uninit: function () {
    Services.obs.removeObserver(this, "system-clock-change", false);
    Services.obs.removeObserver(this, "xpcom-shutdown", false);
  },

  observe: function (aSubject, aTopic, aData) {
    switch (aTopic) {
      case "xpcom-shutdown":
        this.uninit();
        break;
      case "system-clock-change":
        let offset = parseInt(aData, 10);
        if (this._credentialDeferred) {
          this._clockOffsetCache += offset;
        } else if (this._credential.token) {
          this._credential.clockOffset += offset;
        }
        break;
    }
  },

  get _expirationTime() {
    return this._credential.requestTime + this._credential.expires_in + this._credential.clockOffset;
  },

  get token() {
    return this._credential.token;
  },

  get isExpired() {
    let isExpired = true;
    if (this._credential.requestTime) {
      let currentTime = (new Date()).getTime();
      isExpired = this._expirationTime < currentTime;
    }
    return isExpired;
  },

  resetCredential: function () {
    this._credential.token = '';
    this._credential.requestTime = 0;
    this._credential.expires_in = 0;
    this._credential.clockOffset = 0;
  },

  /**
   * Refresh an access token
   *
   * @return Promise
   *        The promise resolves if an updated access token required.
   *        The promise is rejected if fetch access token with error.
   */
  refreshAccessToken: function () {
    var refreshing = true;
    return this.requireAccessToken(refreshing);
  },

  /**
   * Require an access token
   *
   * @param [isRefreshing=false]
   *        If set to true, it drops a cache and fetch access token
   *        from remote server.
   * @return Promise
   *        The promise resolves if a credential is filled in, either an
   *        unexpired cache one or an updated one.
   *        The promise is rejected if fetching access token with error.
   */
  requireAccessToken: function (isRefreshing = false) {
    if (this._credential.token &&
      !isRefreshing &&
      !this.isExpired) {
      return Promise.resolve();
    }

    let url = prefs.get("token.uri");
    let apiKeyName = prefs.get("authorization.keyName");

    if (this._credentialDeferred) {
      return this._credentialDeferred.promise;
    }

    this._credentialDeferred = Promise.defer();
    let requestTime = (new Date()).getTime();
    this._clockOffsetCache = 0;
    DeviceUtils.fetchAccessToken(url, apiKeyName).then((reponse) => {
      let expires_in = 0;
      if ('expires_in' in reponse) {
        expires_in = reponse.expires_in * 1000;
        expires_in += TOKEN_EXPIRATION_TIME_SHIFT;
      }

      this._credential.token = reponse.access_token;
      this._credential.requestTime = requestTime;
      this._credential.expires_in = expires_in;
      this._credential.clockOffset = 0;
      if (this._clockOffsetCache) {
        this._credential.clockOffset += this._clockOffsetCache;
        this._clockOffsetCache = 0;
      }
      this._credentialDeferred.resolve();
      this._credentialDeferred = null;
    }, (errorStatus) => {
      if (this._credential.token) {
        this.resetCredential();
      }
      console.error("error fetch access token, error status:" + JSON.stringify(errorStatus));
      this._clockOffsetCache = 0;
      this._credentialDeferred.reject(errorStatus);
      this._credentialDeferred = null;
    });

    return this._credentialDeferred.promise;
  },
};
