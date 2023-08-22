/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/Task.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "FileUtils",
  "resource://gre/modules/FileUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "TrustedRootCertificate",
  "resource://gre/modules/StoreTrustAnchor.jsm");

XPCOMUtils.defineLazyGetter(this, "appsUpdater", function() {
  return Cc["@kaiostech.com/apps-updater;1"]
         .getService(Ci.nsIAppsUpdater);
});

this.EXPORTED_SYMBOLS = ["AppsUpdater"];

var debug;
function debugPrefObserver() {
  debug = Services.prefs.getBoolPref("dom.mozApps.debug")
            ? (aMsg) => dump("-*- AppsUpdater.jsm : " + aMsg + "\n")
            : (aMsg) => {};
}
debugPrefObserver();
Services.prefs.addObserver("dom.mozApps.debug", debugPrefObserver, false);

this.AppsUpdater = {

  _request: 1,
  _requestList: [],
  _registered: false,

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIAppsUpdaterListener]),

  /**
    * Select trusted root CA and verity the signed zip file.
    */
  _openSignedFile: function(aZipFile, aCertDb, cert) {
    let deferred = Promise.defer();

    let root = TrustedRootCertificate.index;

    if (cert == "stage") {
      root = Ci.nsIX509CertDB.AppServiceCenterDevPublicRoot;
    } else if (cert == "test") {
      root = Ci.nsIX509CertDB.AppServiceCenterTestRoot;
    } else if (cert != "production") {
      throw "INVALID_CERT_TYPE";
    }

    aCertDb.openSignedAppFileAsync(root, aZipFile,
       function(aRv) {
         deferred.resolve(aRv);
       }
    );

    return deferred.promise;
  },

  /**
    * Verify a signed zip file in a new Task.
    */
  _verifySignedFile: function(aZipFile, cert) {
    return Task.spawn((function*() {
      let certDb;
      try {
        certDb = Cc["@mozilla.org/security/x509certdb;1"]
                   .getService(Ci.nsIX509CertDB);
      } catch (e) {
        debug("nsIX509CertDB error: " + e);
        throw "CERTDB_ERROR";
      }

      let result = yield this._openSignedFile(aZipFile, certDb, cert);

      let isSigned = false;
      if (Components.isSuccessCode(result)) {
        isSigned = true;
      } else {
        throw "INVALID_SIGNATURE";
      }

      return isSigned;
    }).bind(this));
  },

  /**
    * Verify a signed zip package and return the result.
    */
  _doVerifyZip: function(msg) {

    return Task.spawn((function*() {
      let response = {
        "name": msg.name,
        "id": 0,
        "data": {}
      };

      if (!msg.id || !msg.data.path || !msg.data.cert) {
        response.data.success = false;
        response.data.error = "INVALID_COMMAND";
        return appsUpdater.sendCommand(response);
      }

      response.id = msg.id;
      response.data.path = msg.data.path;

      let zipFile = new FileUtils.File(msg.data.path);

      try {
        let result = yield this._verifySignedFile(zipFile, msg.data.cert);
        response.data.success = result;
      } catch (e) {
        response.data.success = false;
        response.data.error = e;
      }
      appsUpdater.sendCommand(response);
    }).bind(this));
  },

  /**
    * A nofitier for updater connector to report the response.
    */
  notifyResponse: function(response) {

    debug("notifyResponse: " + JSON.stringify(response));

    // check and  process message from updater.
    let command = response.name;

    switch (command) {
      case "package_match":
      case "can_update":
      case "update_package":
      case "check":
        if (response.id && this._requestList[response.id]) {
          debug("notifyResponse resolve id: " + response.id);
          this._requestList[response.id].resolve(response);
          delete this._requestList[response.id];
        }
        break;
      case "verify_zip":
        this._doVerifyZip(response);
        break;
      default:
        debug("Receive unkown response: " + command);
    }
  },

  /**
    * Send command to updater throgh unisocket connector.
    * Param: command - command include package name and version.
    * Return a Promise.
    */
  updaterCommand: function(command) {

    if (!command.id || !command.name || !command.data) {
      throw "INVALID_UPDATER_COMMAND";
    }

    let deferred = Promise.defer();
    this._requestList[command.id] = deferred;
    appsUpdater.sendCommand(command);

    return deferred.promise;
  },

  /**
    * Process and queue all requests which will be resolve/reject in notifyResponse.
    * Param: packages - packages list, command - command to be verified.
    * Return: A Promise.
    */
  sendCommands: function(packages, command) {

    let deferred = Promise.defer();
    let requests = [];
    let responses = [];

    debug(command + ": " + JSON.stringify(packages));
    packages.forEach((pack)=>{
      requests.push(this.updaterCommand({
        "name": command,
        "id": this._request++,
        "data": pack
      })
      .then(msg => {
        responses.push(msg);
      }).catch((aError) => {
        debug("Oops, something is wrong.");
        throw "APPS_UPDATER_ERROR";
      }));
    });

    // If there are more than one packages,
    // resolve only when all promises are finished.
    Promise.all(requests).then(() => {
      deferred.resolve(responses);
    }).catch((aError) => {
      deferred.reject(aError);
    });

    return deferred.promise;
  },

  /**
    * Check and update the dependencies of an App.
    * Param: packages with verison list to be verify.
    * Return: A Promise.
    */
  checkDependencies: function(dependencies) {

    let deferred = Promise.defer();
    let packages = [];

    // Split dependencies per package which will be processed later.
    for (let key in dependencies) {
      packages.push({
        "name": key,
        "version": dependencies[key]
      });
    }

    // Get version of package from list
    let getVersion = function(packages, name) {
      let version = '';
      for (let i in packages) {
        if ( packages[i].name == name ) {
          version = packages[i].version;
          break;
        }
      }
      return version;
    };

    // Process the response of package_update
    let packageUpdateHandler = (function(results) {

      if (results.length == 0) {
        throw "PACKAGE_UPDATE_ERROR";
      } else {

        let failed = 0;
        results.forEach( pack => {
          if (pack.data && (!pack.data.success || pack.data.success != true)) {
            failed++;
          }
        });

        if (failed == 0) {
          deferred.resolve();
        } else {
          deferred.reject();
        }
      }
    }).bind(this);

    // Process the response of can_update
    let canUpdateHandler = (function(needupdates) {

      if (needupdates.length == 0) {
        throw "PACKAGE_CHECk_ERROR";
      } else {

        let canupdates = [];
        needupdates.forEach( pack => {
          if (pack.data && (pack.data.success || pack.data.success == true)) {
            canupdates.push({
              "name": pack.data.name
            });
          }
        });

        if ( canupdates.length < needupdates.length ) {
          if (!this.checked) {
            // will try can_update again after check.
            this.checked = true;
            this.sendCommands([{}], "check")
            .then(msg=>{
              packageMatchHandler(needupdates);
            })
            .catch(error=>{
              deferred.reject();
            });
          } else {
            // will report error if can_update fails again.
            deferred.reject();
          }
        } else {
          this.sendCommands(canupdates, "update_package")
          .then(msg=>{
            packageUpdateHandler(msg);
          })
          .catch(error=>{
            deferred.reject();
          });
        }
      }
    }).bind(this);

    // Process the response of package_match
    let packageMatchHandler = (function(results) {

      if (results.length == 0) {
        throw "PACKAGE_MATCH_ERROR";
      } else {

        let needupdates = [];
        results.forEach( pack => {
          if (pack.data && (!pack.data.success || pack.data.success != true)) {
            needupdates.push({
              "name": pack.data.name,
              "version": getVersion(packages, pack.data.name)
            });
          }
        });
        if (needupdates.length > 0) {
          this.sendCommands(needupdates, "can_update")
          .then( msg => {
            canUpdateHandler(msg);
          })
          .catch(error => {
            deferred.reject();
          });
        } else {
          deferred.resolve();
        }
      }
    }).bind(this);

    // always start with checked as false.
    this.checked = false;
    this.sendCommands(packages, "package_match")
    .then(msg=>{
      packageMatchHandler(msg);
    })
    .catch(error => {
      deferred.reject();
    });

    return deferred.promise;
  },

  /**
    * Register to updater connector module which will connect to the socket.
    */
  register: function() {
    if(!this._registered) {
      appsUpdater.register(this);
      this._registered = true;
    }
  }
}
