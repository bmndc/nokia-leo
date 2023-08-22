/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

this.EXPORTED_SYMBOLS = ["kaiAccounts", "KaiAccounts"];

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/osfile.jsm");
Cu.import("resource://services-common/utils.js");
Cu.import("resource://services-crypto/utils.js");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/KaiAccountsCommon.js");

XPCOMUtils.defineLazyModuleGetter(this, "KaiAccountsClient",
  "resource://gre/modules/KaiAccountsClient.jsm");

// All properties exposed by the public KaiAccounts API.
let publicProperties = [
  "getAccountsClient",
  "getAssertion",
  "getCertificate",
  "getSignedInUser",
  "localtimeOffsetMsec",
  "now",
  "getAccountInfo",
  "setSignedInUser",
  "signOut",
  "version"
];

// An AccountState object holds all state related to one specific account.
// Only one AccountState is ever "current" in the KaiAccountsInternal object -
// whenever a user logs out or logs in, the current AccountState is discarded,
// making it impossible for the wrong state or state data to be accidentally
// used.
// In addition, it has some promise-related helpers to ensure that if an
// attempt is made to resolve a promise on a "stale" state (eg, if an
// operation starts, but a different user logs in before the operation
// completes), the promise will be rejected.
// It is intended to be used thusly:
// somePromiseBasedFunction: function() {
//   let currentState = this.currentAccountState;
//   return someOtherPromiseFunction().then(
//     data => currentState.resolve(data)
//   );
// }
// If the state has changed between the function being called and the promise
// being resolved, the .resolve() call will actually be rejected.
let AccountState = function(kaiaInternal) {
  this.kaiaInternal = kaiaInternal;
};

AccountState.prototype = {
  cert: null,
  keyPair: null,
  signedInUser: null,
  whenVerifiedDeferred: null,
  whenKeysReadyDeferred: null,

  get isCurrent() {
    return this.kaiaInternal && this.kaiaInternal.currentAccountState === this;
  },

  abort: function() {
    if (this.whenVerifiedDeferred) {
      this.whenVerifiedDeferred.reject(
        new Error("Verification aborted; Another user signing in"));
      this.whenVerifiedDeferred = null;
    }

    if (this.whenKeysReadyDeferred) {
      this.whenKeysReadyDeferred.reject(
        new Error("Verification aborted; Another user signing in"));
      this.whenKeysReadyDeferred = null;
    }
    this.cert = null;
    this.keyPair = null;
    this.signedInUser = null;
    this.kaiaInternal = null;
  },

  getUserAccountData: function() {
    // Skip disk if user is cached.
    if (this.signedInUser) {
      return this.resolve(this.signedInUser.accountData);
    }

    return this.kaiaInternal.signedInUserStorage.get().then(
      user => {
        if (logPII) {
          // don't stringify unless it will be written. We should replace this
          // check with param substitutions added in bug 966674
          log.debug("getUserAccountData -> " + JSON.stringify(user));
        }
        if (user) {
          if (user.version == this.kaiaInternal.version) {
            log.debug("setting signed in user");
            this.signedInUser = user;
          } else {
            // to handle backward compatibility for user signed-in with an out-of-date version
            log.debug("Unmatched version: " + user.version + " -> " + this.kaiaInternal.version);
            // workaround which reforms the incompatible fields between v2 and v3
            if (user.version == 2 && this.kaiaInternal.version == 3 && user.accountData) {
              if (!user.accountData.email && !user.accountData.phone && user.accountData.accountId) {
                user.accountData.email = user.accountData.accountId;
              }
            }
            this.signedInUser = user;
          }
        }
        return this.resolve(user ? user.accountData : null);
      },
      err => {
        if (err instanceof OS.File.Error && err.becauseNoSuchFile) {
          // File hasn't been created yet.  That will be done
          // on the first call to getSignedInUser
          return this.resolve(null);
        }
        log.error("Error during getUserAccountData: " + err);
        return this.reject({});
      }
    );
  },

  setUserAccountData: function(accountData) {
    return this.kaiaInternal.signedInUserStorage.get().then(record => {
      if (!this.isCurrent) {
        return this.reject(new Error("Another user has signed in"));
      }
      record.accountData = accountData;
      this.signedInUser = record;
      return this.kaiaInternal.signedInUserStorage.set(record)
        .then(() => this.resolve(accountData));
    });
  },

  resolve: function(result) {
    if (!this.isCurrent) {
      log.info("An accountState promise was resolved, but was actually rejected" +
               " due to a different user being signed in. Originally resolved" +
               " with: " + result);
      return Promise.reject(new Error("A different user signed in"));
    }
    return Promise.resolve(result);
  },

  reject: function(error) {
    // It could be argued that we should just let it reject with the original
    // error - but this runs the risk of the error being (eg) a 401, which
    // might cause the consumer to attempt some remediation and cause other
    // problems.
    if (!this.isCurrent) {
      log.info("An accountState promise was rejected, but we are ignoring that" +
               "reason and rejecting it due to a different user being signed in." +
               "Originally rejected with: " + error);
      return Promise.reject(new Error("A different user signed in"));
    }
    return Promise.reject(error);
  },
}

/**
 * Copies properties from a given object to another object.
 *
 * @param from (object)
 *        The object we read property descriptors from.
 * @param to (object)
 *        The object that we set property descriptors on.
 * @param options (object) (optional)
 *        {keys: [...]}
 *          Lets the caller pass the names of all properties they want to be
 *          copied. Will copy all properties of the given source object by
 *          default.
 *        {bind: object}
 *          Lets the caller specify the object that will be used to .bind()
 *          all function properties we find to. Will bind to the given target
 *          object by default.
 */
function copyObjectProperties(from, to, opts = {}) {
  let keys = (opts && opts.keys) || Object.keys(from);
  let thisArg = (opts && opts.bind) || to;

  for (let prop of keys) {
    let desc = Object.getOwnPropertyDescriptor(from, prop);

    if (typeof(desc.value) == "function") {
      desc.value = desc.value.bind(thisArg);
    }

    if (desc.get) {
      desc.get = desc.get.bind(thisArg);
    }

    if (desc.set) {
      desc.set = desc.set.bind(thisArg);
    }

    Object.defineProperty(to, prop, desc);
  }
}

/**
 * The public API's constructor.
 */
this.KaiAccounts = function (mockInternal) {
  let internal = new KaiAccountsInternal();
  let external = {};

  // Copy all public properties to the 'external' object.
  let prototype = KaiAccountsInternal.prototype;
  let options = {keys: publicProperties, bind: internal};
  copyObjectProperties(prototype, external, options);

  // Copy all of the mock's properties to the internal object.
  if (mockInternal && !mockInternal.onlySetInternal) {
    copyObjectProperties(mockInternal, internal);
  }

  if (mockInternal) {
    // Exposes the internal object for testing only.
    external.internal = internal;
  }

  return Object.freeze(external);
}

/**
 * The internal API's constructor.
 */
function KaiAccountsInternal() {
  this.version = DATA_FORMAT_VERSION;
  this.currentTimer = null;
  this.currentAccountState = new AccountState(this);

  this.signedInUserStorage = new JSONStorage({
    filename: DEFAULT_STORAGE_FILENAME,
    baseDir: OS.Constants.Path.profileDir,
  });
}

/**
 * The internal API's prototype.
 */
KaiAccountsInternal.prototype = {

  /**
   * The current data format's version number.
   */
  version: DATA_FORMAT_VERSION,

  // The timeout (in ms) we use to poll for a verified mail for the first 2 mins.
  VERIFICATION_POLL_TIMEOUT_INITIAL: 5000, // 5 seconds
  // And how often we poll after the first 2 mins.
  VERIFICATION_POLL_TIMEOUT_SUBSEQUENT: 15000, // 15 seconds.

  _kaiAccountsClient: null,

  _refreshTokenDeferred: null,

  get kaiAccountsClient() {
    if (!this._kaiAccountsClient) {
      this._kaiAccountsClient = new KaiAccountsClient();
    }
    return this._kaiAccountsClient;
  },

  /**
   * Return the current time in milliseconds as an integer.  Allows tests to
   * manipulate the date to simulate certificate expiration.
   */
  now: function() {
    return this.kaiAccountsClient.now();
  },

  getAccountsClient: function() {
    return this.kaiAccountsClient;
  },

  /**
   * Return clock offset in milliseconds, as reported by the kaiAccountsClient.
   * This can be overridden for testing.
   *
   * The offset is the number of milliseconds that must be added to the client
   * clock to make it equal to the server clock.  For example, if the client is
   * five minutes ahead of the server, the localtimeOffsetMsec will be -300000.
   */
  get localtimeOffsetMsec() {
    return this.kaiAccountsClient.localtimeOffsetMsec;
  },

  /**
   * Get the user currently signed in to KaiOS Accounts.
   *
   * @return Promise
   *        The promise resolves to the credentials object of the signed-in user:
   *        {
   *          token_type: Set to the fixed value "hawk".
   *          scope: Requested Access Scope.
   *          expires_in: Time to Live (in seconds) of the access authorization represented by
   *                      the mac key.
   *          kid: the key id to be used for hawk
   *          mac_key: the key to be used for hawk and matching the kid
   *          mac_algorithm: Only 'hmac-sha-256' is supported.
   *          refresh_token: Refresh Token returned by auth server to renew token.
   *          verified: email verification status
   *        }
   *        or null if no user is signed in.
   */
  getSignedInUser: function getSignedInUser() {
    let currentState = this.currentAccountState;
    return currentState.getUserAccountData().then(data => {
      if (!data) {
        return null;
      }
      return data;
    }).then(result => currentState.resolve(result));
  },

  /**
   * Set the current user signed in to Firefox Accounts.
   *
   * @param credentials
   *        The credentials object obtained by logging in or creating
   *        an account on the KaiA server:
   *        {
   *          token_type: Set to the fixed value "hawk".
   *          scope: Requested Access Scope.
   *          expires_in: Time to Live (in seconds) of the access authorization represented by
   *                      the mac key.
   *          kid: the key id to be used for hawk
   *          mac_key: the key to be used for hawk and matching the kid
   *          mac_algorithm: Only 'hmac-sha-256' is supported.
   *          refresh_token: Refresh Token returned by auth server to renew token.
   *          verified: email verification status
   *        }
   * @return Promise
   *         The promise resolves to null when the data is saved
   *         successfully and is rejected on error.
   */
  setSignedInUser: function setSignedInUser(credentials) {
    log.debug("setSignedInUser - aborting any existing flows");
    this.abortExistingFlow();

    let record = {version: this.version, accountData: credentials};
    let currentState = this.currentAccountState;
    // Cache a clone of the credentials object.
    currentState.signedInUser = JSON.parse(JSON.stringify(record));

    // This promise waits for storage, but not for verification.
    // We're telling the caller that this is durable now.
    return this.signedInUserStorage.set(record).then(() => {
      this.notifyObservers(ONLOGIN_NOTIFICATION);
    }).then(result => currentState.resolve(result));
  },

  getCertificate: function getCertificate(data) {
    // If a cached certificate is valid or device offline,
    // return the cached one directly.
    if (data.validUntil > this.now() || Services.io.offline) {
      let cert_tmp = {
        kid: data.kid,
        mac_key: data.mac_key,
        localtimeOffsetMsec: data.localtimeOffsetMsec
      };
      return Promise.resolve(cert_tmp);
    }

    if (this._refreshTokenDeferred) {
      return this._refreshTokenDeferred.promise;
    }

    this._refreshTokenDeferred = Promise.defer();
    let requestTime = this.now();

    this.kaiAccountsClient.refreshToken(data).then( cert_refreshed => {
      data.kid = cert_refreshed.kid;
      data.mac_key = cert_refreshed.mac_key;
      data.refresh_token = cert_refreshed.refresh_token;
      data.localtimeOffsetMsec = this.localtimeOffsetMsec;
      data.validUntil = requestTime + cert_refreshed.expires_in * 1000;
      let currentState = this.currentAccountState;
      currentState.setUserAccountData(data);
      let cert = {
        kid: data.kid,
        mac_key: data.mac_key,
        localtimeOffsetMsec: data.localtimeOffsetMsec
      };
      this._refreshTokenDeferred.resolve(cert);
      this._refreshTokenDeferred = null;
    }, error => {
      this._refreshTokenDeferred.reject(error);
      this._refreshTokenDeferred = null;
    });
    return this._refreshTokenDeferred.promise;
  },

  /**
   * returns a promise that fires with the assertion.  If there is no verified
   * signed-in user, fires with null.
   */
  getAssertion: function getAssertion(audience) {
    log.debug("enter getAssertion()");
    let currentState = this.currentAccountState;
    return currentState.getUserAccountData().then(data => {
      if (!data) {
        // No signed-in user
        return null;
      }
      return this.getCertificate(data).then(cert => {
          return JSON.stringify(cert);
        });
    }).then(result => currentState.resolve(result));
  },

  /**
   * Obtain account information for the currently signed-in user.
   */
  getAccountInfo: function getAccountInfo() {
    return this.getSignedInUser().then(
      user => {
        if (user) {
          return this.getCertificate(user).then(
            cert => {
              return this.kaiAccountsClient.getAccountInfo(cert).then(
                result => {
                  let accountData = user;
                  accountData.uid = result.uid;
                  accountData.phone = result.phone;
                  accountData.email = result.email;
                  accountData.altPhone = result.altPhone;
                  accountData.altEmail = result.altEmail;
                  accountData.pending = result.pending ? {
                    phone: result.pending.phone,
                    email: result.pending.email,
                    altPhone: result.pending.altPhone,
                    altEmail: result.pending.altEmail
                  } : {};
                  accountData.yob = result.yob;
                  accountData.birthday = result.birthday;
                  accountData.gender = result.gender;
                  let currentState = this.currentAccountState;
                  currentState.setUserAccountData(accountData);
                  return Promise.resolve(result);
                },
                reason => {
                  return Promise.reject(reason);
                }
              );
            }
          );
        }
        return Promise.reject("no login user");
      }
    );
  },

  /*
   * Reset state such that any previous flow is canceled.
   */
  abortExistingFlow: function abortExistingFlow() {
    if (this.currentTimer) {
      log.debug("Polling aborted; Another user signing in");
      clearTimeout(this.currentTimer);
      this.currentTimer = 0;
    }
    this.currentAccountState.abort();
    this.currentAccountState = new AccountState(this);
  },

  signOut: function signOut(localOnly, errorReason) {
    let currentState = this.currentAccountState;
    let user;
    return currentState.getUserAccountData().then(data => {
      user = data;
      return this._signOutLocal();
    }, error => {
      log.debug("Error getUserAccountData: " + error);
    }).then(() => {
      // KaiAccountsManager calls here, then does its own call
      // to KaiAccountsClient.signOut().
      if (!localOnly) {
        // Wrap this in a promise so *any* errors in signOut won't
        // block the local sign out. This is *not* returned.
        Promise.resolve().then(() => {
          // This can happen in the background and shouldn't block
          // the user from signing out. The server must tolerate
          // clients just disappearing, so this call should be best effort.
          return this.getCertificate(user).then(cert => {
            return this._signOutServer(cert);
          });
        }).then(null, err => {
          log.error("Error during remote sign out of KaiOS Accounts: " + err);
        });
      }
    }).then(() => {
      this.notifyObservers(ONLOGOUT_NOTIFICATION, errorReason);
    });
  },

  /**
   * This function should be called in conjunction with a server-side
   * signOut via KaiAccountsClient.
   */
  _signOutLocal: function signOutLocal() {
    this.abortExistingFlow();
    this.currentAccountState.signedInUser = null; // clear in-memory cache
    return this.signedInUserStorage.set(null);
  },

  _signOutServer: function signOutServer(cert) {
    return this.kaiAccountsClient.signOut(cert);
  },

  getUserAccountData: function() {
    return this.currentAccountState.getUserAccountData();
  },

  notifyObservers: function(topic, data) {
    log.debug("Notifying observers of " + topic);
    Services.obs.notifyObservers(null, topic, data);
  },

};

/**
 * JSONStorage constructor that creates instances that may set/get
 * to a specified file, in a directory that will be created if it
 * doesn't exist.
 *
 * @param options {
 *                  filename: of the file to write to
 *                  baseDir: directory where the file resides
 *                }
 * @return instance
 */
function JSONStorage(options) {
  this.baseDir = options.baseDir;
  this.path = OS.Path.join(options.baseDir, options.filename);
};

JSONStorage.prototype = {
  set: function(contents) {
    return OS.File.makeDir(this.baseDir, {ignoreExisting: true})
      .then(CommonUtils.writeJSON.bind(null, contents, this.path));
  },

  get: function() {
    return CommonUtils.readJSON(this.path);
  }
};

// A getter for the instance to export
XPCOMUtils.defineLazyGetter(this, "kaiAccounts", function() {
  let a = new KaiAccounts();
  return a;
});
