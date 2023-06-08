/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * Temporary abstraction layer for common Kai Accounts operations.
 * For now, we will be using this module only from B2G but in the end we might
 * want this to be merged with KaiAccounts.jsm and let other products also use
 * it.
 */

"use strict";

this.EXPORTED_SYMBOLS = ["KaiAccountsManager"];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/KaiAccountsCommon.js");

XPCOMUtils.defineLazyModuleGetter(this, "kaiAccounts",
  "resource://gre/modules/KaiAccounts.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "permissionManager",
                                   "@mozilla.org/permissionmanager;1",
                                   "nsIPermissionManager");

this.KaiAccountsManager = {

  init: function() {
    Services.obs.addObserver(this, ONLOGOUT_NOTIFICATION, false);
    Services.obs.addObserver(this, ON_FXA_UPDATE_NOTIFICATION, false);
  },

  observe: function(aSubject, aTopic, aData) {
    // Both topics indicate our cache is invalid
    this._activeSession = null;

    if (aData == ONVERIFIED_NOTIFICATION) {
      log.debug("KaiAccountsManager: cache cleared, broadcasting: " + aData);
      Services.obs.notifyObservers(null, aData, null);
    }
  },

  // We don't really need to save kaiAccounts instance but this way we allow
  // to mock KaiAccounts from tests.
  get _kaiAccounts() {
    return kaiAccounts;
  },

  // We keep the session details here so consumers don't need to deal with
  // session tokens and are only required to handle the accountId.
  _activeSession: null,

  // Are we refreshing our authentication?
  _refreshing: false,

  get _user() {
    if (!this._activeSession) {
      return null;
    }

    return {
      uid: this._activeSession.uid,
      phone: this._activeSession.phone,
      email: this._activeSession.email,
      altPhone: this._activeSession.altPhone,
      altEmail: this._activeSession.altEmail,
      pending: this._activeSession.pending ? {
        phone: this._activeSession.pending.phone,
        email: this._activeSession.pending.email,
        altPhone: this._activeSession.pending.altPhone,
        altEmail: this._activeSession.pending.altEmail
      } : {},
      yob: this._activeSession.yob,
      birthday: this._activeSession.birthday,
      gender: this._activeSession.gender
    };
  },

  _isVerified(user) {
    if ((user.pending && (user.pending.phone || user.pending.email ||
      user.pending.altPhone || user.pending.altEmail)) ||
      !user.uid) {
      return false;
    }
    return true;
  },

  _error: function(aErrorString) {
    log.error(aErrorString);
    let reason = {
      error: aErrorString
    };
    return Promise.reject(reason);
  },

  _getError: function(aServerResponse) {
    if (!aServerResponse || !aServerResponse.error || !aServerResponse.error.errno) {
      return;
    }
    let error = SERVER_ERRNO_TO_ERROR[aServerResponse.error.errno];
    return error;
  },

  _serverError: function(aServerResponse) {
    let error = this._getError({ error: aServerResponse });
    return this._error(error ? error : ERROR_SERVER_ERROR);
  },

  // As with _kaiAccounts, we don't really need this method, but this way we
  // allow tests to mock KaiAccountsClient.  By default, we want to return the
  // client used by the kaiAccounts object because deep down they should have
  // access to the same hawk request object which will enable them to share
  // local clock skeq data.
  _getKaiAccountsClient: function() {
    return this._kaiAccounts.getAccountsClient();
  },

  _signIn: function(aAccountId, aPassword) {
    if (Services.io.offline) {
      return this._error(ERROR_OFFLINE);
    }

    if (!aAccountId) {
      return this._error(ERROR_INVALID_ACCOUNTID);
    }

    if (!aPassword) {
      return this._error(ERROR_INVALID_PASSWORD);
    }

    let client = this._getKaiAccountsClient();
    return this._kaiAccounts.getSignedInUser().then(
      user => {
        return client.signIn(aAccountId, aPassword);
      }
    ).then(
      user => {
        let error = this._getError(user);
        if (error) {
          return this._error(error);
        }
        this._activeSession = null;
        return this._kaiAccounts.setSignedInUser(user).then(
          () => {
            return this._kaiAccounts.getAccountInfo().then(
              accountInfo => {
              },
              reason => {
                log.error("Obtaining account info failed reason " + JSON.stringify(reason));
              }
            ).then(
              () => {
                return this.getAccount().then(
                  result => {
                    log.debug("User signed in: " + JSON.stringify(result));
                    return Promise.resolve({
                      accountCreated: false,
                      user: result
                    });
                  }
                );
              }
            );
          }
        );
      },
      reason => { return this._serverError(reason); }
    );
  },

  _signUp: function(aPhone, aEmail, aPassword, aInfo) {
    if (Services.io.offline) {
      return this._error(ERROR_OFFLINE);
    }

    if (!aPhone) {
      return this._error(ERROR_INVALID_ACCOUNTID);
    }

    if (!aPassword) {
      return this._error(ERROR_INVALID_PASSWORD);
    }

    let client = this._getKaiAccountsClient();
    return this._kaiAccounts.getSignedInUser().then(
      user => {
        return client.signUp(aPhone, aEmail, aPassword, aInfo);
      }
    ).then(
      user => {
        let error = this._getError(user);
        if (error) {
          return this._error(error);
        }

        log.debug("User signed Up: " + JSON.stringify(user));
            return Promise.resolve({
              accountCreated: true,
              uid: user.id
        });
      },
      reason => { return this._serverError(reason); }
    );
  },

  /**
   * Determine whether the incoming error means that the current account
   * has new server-side state via deletion or password change, and if so,
   * spawn the appropriate UI (sign in or refresh); otherwise re-reject.
   *
   * See the latter method for possible (error code, errno) pairs.
   */
  _handleGetAssertionError: function(reason, aAudience, aPrincipal) {
    log.debug("KaiAccountsManager._handleGetAssertionError()");
    if (reason && reason.errno == ERRNO_INVALID_AUTH_TOKEN) {
      let error = this._getError({ error: reason });
      return this._localSignOut(error).then(
            () => {
              return this._uiRequest(UI_REQUEST_SIGN_IN_FLOW, aAudience,
                                     aPrincipal);
            },
            (reason) => {
              // reject primary problem, not signout failure
              log.error("Signing out in response to server error threw: " +
                        reason);
              return this._error(reason);
            }
          );
    }
    return this._serverError(reason);
  },

  _getAssertion: function(aAudience, aPrincipal) {
    return this._kaiAccounts.getAssertion(aAudience).then(
      (result) => {
        if (aPrincipal) {
          this._addPermission(aPrincipal);
        }
        return result;
      },
      (reason) => {
        return this._handleGetAssertionError(reason, aAudience, aPrincipal);
      }
    );
  },

  /**
   * "Refresh authentication" means:
   *   Interactively demonstrate knowledge of the FxA password
   *   for the currently logged-in account.
   * There are two very different scenarios:
   *   1) The password has changed on the server. Failure should log
   *      the current account OUT.
   *   2) The person typing can't prove knowledge of the password used
   *      to log in. Failure should do nothing.
   */
  _refreshAuthentication: function(aAudience, aAccountId, aPrincipal,
                                   logoutOnFailure=false) {
    this._refreshing = true;
    return this._uiRequest(UI_REQUEST_REFRESH_AUTH,
                           aAudience, aPrincipal, aAccountId).then(
      (assertion) => {
        this._refreshing = false;
        return assertion;
      },
      (reason) => {
        this._refreshing = false;
        if (logoutOnFailure) {
          return this._signOut(this._activeSession).then(
            () => {
              return this._error(reason);
            }
          );
        }
        return this._error(reason);
      }
    );
  },

  _localSignOut: function(errorReason) {
    return this._kaiAccounts.signOut(true, errorReason);
  },

  _signOut: function(user) {
    if (Services.io.offline) {
      return this._error(ERROR_OFFLINE);
    }
    let client = this._getKaiAccountsClient();
    return this._kaiAccounts.getCertificate(user).then(
      cert => {
        return client.signOut(cert).then(
          result => {
          },
          reason => {
            log.error("Sign out account failed reason " + JSON.stringify(reason));
          }
        ).then(
          () => {
            return this._localSignOut().then(
              () => {
                log.debug("Signed out");
                return Promise.resolve();
              }
            );
          }
        );
      }
    );
  },

  _uiRequest: function(aRequest, aAudience, aPrincipal, aParams) {
    if (Services.io.offline) {
      return this._error(ERROR_OFFLINE);
    }
    let ui = Cc["@mozilla.org/fxaccounts/fxaccounts-ui-glue;1"]
               .createInstance(Ci.nsIFxAccountsUIGlue);
    if (!ui[aRequest]) {
      return this._error(ERROR_UI_REQUEST);
    }

    if (!aParams || !Array.isArray(aParams)) {
      aParams = [aParams];
    }

    return ui[aRequest].apply(this, aParams).then(
      result => {
        return this.getAccount().then(
          user => {
            if (user) {
              return this._getAssertion(aAudience, aPrincipal);
            }
            return this._error(ERROR_UNVERIFIED_ACCOUNT);
          }
        );
      },
      error => {
        return this._error(ERROR_UI_ERROR);
      }
    );
  },

  _addPermission: function(aPrincipal) {
    // This will fail from tests cause we are running them in the child
    // process until we have chrome tests in b2g. Bug 797164.
    try {
      permissionManager.addFromPrincipal(aPrincipal, KAIACCOUNTS_PERMISSION,
                                         Ci.nsIPermissionManager.ALLOW_ACTION);
    } catch (e) {
      log.warn("Could not add permission " + e);
    }
  },

  // -- API --

  signIn: function(aAccountId, aPassword) {
    return this._signIn(aAccountId, aPassword);
  },

  signUp: function(aPhone, aEmail, aPassword, aInfo) {
    return this._signUp(aPhone, aEmail, aPassword, aInfo);
  },

  signOut: function() {
    if (!this._activeSession) {
      // If there is no cached active session, we try to get it from the
      // account storage.
      return this._kaiAccounts.getSignedInUser().then(
        user => {
          if (!user) {
            return Promise.resolve();
          }
          return this._signOut(user);
        }
      );
    }
    return this._signOut(this._activeSession);
  },

  getAccount: function() {
    // We check first if we have session details cached.
    if (this._activeSession && this._isVerified(this._activeSession)) {
      log.debug("Account " + JSON.stringify(this._user));
      return Promise.resolve(this._user);
    }

    // If no cached information, we try to get it from the persistent storage.
    return this._kaiAccounts.getSignedInUser().then(
      storedUser => {
        if (!storedUser) {
          log.debug("No signed in account");
          return Promise.resolve(null);
        }

        return Promise.resolve().then(
          () => {
            // If we get a stored information of a not yet verified account,
            // we kick off refetching latest account info.
            if (!Services.io.offline && !this._isVerified(storedUser)) {
              return this._kaiAccounts.getAccountInfo().then(
                accountInfo => {
                  return this._kaiAccounts.getSignedInUser().then(
                    latestUser => {
                      return latestUser;
                    }
                  );
                },
                reason => {
                  log.error("Obtaining account info failed reason " + JSON.stringify(reason));
                  return storedUser;
                }
              );
            }
            return storedUser;
          }
        ).then(
          user => {
            this._activeSession = user;
            log.debug("Account " + JSON.stringify(this._user));
            return Promise.resolve(this._user);
          }
        );
      }
    );
  },

  verifyPassword: function(aPassword) {
    if (Services.io.offline) {
      return this._error(ERROR_OFFLINE);
    }

    if (!aPassword) {
      return this._error(ERROR_INVALID_PASSWORD);
    }

    return this._kaiAccounts.getSignedInUser().then(
      user => {
        if (!user) {
          return Promise.reject("no login user");
        }

        // Sync latest account info to ensure the correctness of identity
        return this._kaiAccounts.getAccountInfo().then(
          accountInfo => {
            let client = this._getKaiAccountsClient();
            let accountId = accountInfo.phone ? accountInfo.phone : accountInfo.email;
            return client.verifyPassword(accountId, aPassword).then(
              result => {
                let error = this._getError(result);
                if (error) {
                  return this._error(error);
                }
                log.debug("Verified password");
                return Promise.resolve();
              },
              reason => {
                log.error("Verify password failed reason " + JSON.stringify(reason));
                return this._serverError(reason);
              }
            );
          },
          reason => {
            log.error("Obtaining account info failed reason " + JSON.stringify(reason));
            return this._serverError(reason);
          }
        );
      }
    );
  },

  changePassword: function(aOldPassword, aNewPassword) {
    if (Services.io.offline) {
      return this._error(ERROR_OFFLINE);
    }

    if (!aOldPassword || !aNewPassword) {
      return this._error(ERROR_INVALID_PASSWORD);
    }

    if (aOldPassword == aNewPassword) {
      return this._error(ERROR_INVALID_PASSWORD);
    }

    return this._kaiAccounts.getSignedInUser().then(
      user => {
        if (!user) {
          return Promise.reject("no login user");
        }

        // Sync latest account info to ensure the correctness of identity
        return this._kaiAccounts.getAccountInfo().then(
          accountInfo => {
            return this._kaiAccounts.getCertificate(user).then(
              cert => {
                let client = this._getKaiAccountsClient();
                return client.changePassword(accountInfo.phone, accountInfo.email, aOldPassword, aNewPassword, cert);
              }
            ).then(
              result => {
                let error = this._getError(result);
                if (error) {
                  return this._error(error);
                }
                log.debug("Changed password");
                return Promise.resolve();
              },
              reason => {
                log.error("Change password failed reason " + JSON.stringify(reason));
                return this._serverError(reason);
              }
            );
          },
          reason => {
            log.error("Obtaining account info failed reason " + JSON.stringify(reason));
            return this._serverError(reason);
          }
        );
      }
    );
  },

  updateAccount: function(aPhone, aEmail, aInfo, aPassword) {
    if (Services.io.offline) {
      return this._error(ERROR_OFFLINE);
    }

    if ((aPhone || aEmail || (aInfo && aInfo.altPhone) || (aInfo && aInfo.altEmail)) && !aPassword) {
      // Password is necessary when updating anyone of (phone/email/altPhone/altEmail)
      return this._error(ERROR_INVALID_OPERATION);
    }

    return this._kaiAccounts.getSignedInUser().then(
      user => {
        if (!user) {
          return Promise.reject("no login user");
        }

        // Sync latest account info to ensure the correctness of identity
        return this._kaiAccounts.getAccountInfo().then(
          accountInfo => {
            return this._kaiAccounts.getCertificate(user).then(
              cert => {
                let client = this._getKaiAccountsClient();
                if (aPassword) {
                  // Identity proving requires present phone/email and password for computing hashed onePW
                  return client.updateAccount(aPhone, aEmail, aInfo, aPassword,
                    accountInfo.phone, accountInfo.email, cert);
                } else {
                  return client.updateAccount(null, null, aInfo, null, null, null, cert);
                }
              }
            ).then(
              result => {
                let error = this._getError(result);
                if (error) {
                  return this._error(error);
                }
                log.debug("Updated account");
                return Promise.resolve();
              }
            ).then(
              () => {
                this._activeSession = null;
                return this._kaiAccounts.getAccountInfo().then(
                  accountInfo => {
                  },
                  reason => {
                    log.error("Obtaining account info failed reason " + JSON.stringify(reason));
                  }
                );
              },
              reason => {
                log.error("Update account failed reason " + JSON.stringify(reason));
                return this._serverError(reason);
              }
            );
          },
          reason => {
            log.error("Obtaining account info failed reason " + JSON.stringify(reason));
            return this._serverError(reason);
          }
        );
      }
    );
  },

  deleteAccount: function() {
    if (Services.io.offline) {
      return this._error(ERROR_OFFLINE);
    }

    return this._kaiAccounts.getSignedInUser().then(
      user => {
        if (!user) {
          return Promise.reject("no login user");
        }
        return this._kaiAccounts.getCertificate(user).then(
          cert => {
            let client = this._getKaiAccountsClient();
            return client.deleteAccount(cert).then(
              result => {
                let error = this._getError(result);
                if (error) {
                  return this._error(error);
                }
                return this._localSignOut().then(
                  () => {
                    log.debug("Deleted account then signed out");
                    return Promise.resolve();
                  }
                );
              },
              reason => {
                log.error("Delete account failed reason " + JSON.stringify(reason));
                return this._serverError(reason);
              }
            );
          }
        );
      }
    );
  },

  checkPhoneExistence: function(aPhone) {
    if (Services.io.offline) {
      return this._error(ERROR_OFFLINE);
    }

    if (!aPhone) {
      return this._error(ERROR_INVALID_ACCOUNTID);
    }

    let client = this._getKaiAccountsClient();
    return client.checkPhoneExistence(aPhone).then(
      result => {
        return Promise.resolve({
          registered: result
        });
      },
      reason => { return this._serverError(reason); }
    );
  },

  checkEmailExistence: function(aEmail) {
    if (Services.io.offline) {
      return this._error(ERROR_OFFLINE);
    }

    if (!aEmail) {
      return this._error(ERROR_INVALID_ACCOUNTID);
    }

    let client = this._getKaiAccountsClient();
    return client.checkEmailExistence(aEmail).then(
      result => {
        return Promise.resolve({
          registered: result
        });
      },
      reason => { return this._serverError(reason); }
    );
  },

  requestPhoneVerification: function(aPhone, aUid) {
    if (Services.io.offline) {
      return this._error(ERROR_OFFLINE);
    }

    if (!aPhone) {
      return this._error(ERROR_INVALID_ACCOUNTID);
    }

    let client = this._getKaiAccountsClient();
    return client.requestPhoneVerification(aPhone, aUid).then(
      result => {
        return Promise.resolve({
          verificationId: result.id
        });
      },
      reason => { return this._serverError(reason); }
    );
  },

  resolvePhoneVerification: function(aPhone, aVerificationId, aCode, aUid) {
    if (Services.io.offline) {
      return this._error(ERROR_OFFLINE);
    }

    if (!aPhone) {
      return this._error(ERROR_INVALID_ACCOUNTID);
    }

    if (!aVerificationId || !aCode) {
      return this._error(ERROR_INVALID_VERIFICATION_CODE);
    }

    let client = this._getKaiAccountsClient();
    return client.resolvePhoneVerification(aPhone, aVerificationId, aCode, aUid).then(
      result => {
        return Promise.resolve();
      },
      reason => { return this._serverError(reason); }
    );
  },

  requestEmailVerification: function(aEmail, aUid) {
    if (Services.io.offline) {
      return this._error(ERROR_OFFLINE);
    }

    if (!aEmail) {
      return this._error(ERROR_INVALID_ACCOUNTID);
    }

    let client = this._getKaiAccountsClient();
    return client.requestEmailVerification(aEmail, aUid).then(
      result => {
        return Promise.resolve({
          verificationId: result.id
        });
      },
      reason => { return this._serverError(reason); }
    );
  },

  /*
   * Try to get an assertion for the given audience. Here we implement
   * the heart of the response to navigator.mozId.request() on device.
   * (We can also be called via the IAC API, but it's request() that
   * makes this method complex.) The state machine looks like this,
   * ignoring simple errors:
   *   If no one is signed in, and we aren't suppressing the UI:
   *     trigger the sign in flow.
   *   else if we were asked to refresh and the grace period is up:
   *     trigger the refresh flow.
   *   else:
   *      request user permission to share an assertion if we don't have it
   *      already and ask the core code for an assertion, which might itself
   *      trigger either the sign in or refresh flows (if our account
   *      changed on the server).
   *
   * aOptions can include:
   *   refreshAuthentication  - (bool) Force re-auth.
   *   silent                 - (bool) Prevent any UI interaction.
   *                            I.e., try to get an automatic assertion.
   */
  getAssertion: function(aAudience, aPrincipal, aOptions) {
    if (!aAudience) {
      return this._error(ERROR_INVALID_AUDIENCE);
    }

    let secMan = Cc["@mozilla.org/scriptsecuritymanager;1"]
                   .getService(Ci.nsIScriptSecurityManager);
    let uri = Services.io.newURI(aPrincipal.origin, null, null);
    log.debug("KaiAccountsManager.getAssertion() aPrincipal: ",
              aPrincipal.origin, aPrincipal.appId,
              aPrincipal.isInBrowserElement);
    let principal = secMan.getAppCodebasePrincipal(uri,
      aPrincipal.appId, aPrincipal.isInBrowserElement);

    return this.getAccount().then(
      user => {
        if (user) {
          // Second case: do we need to refresh?
          if (aOptions &&
              (typeof(aOptions.refreshAuthentication) != "undefined")) {
            let gracePeriod = aOptions.refreshAuthentication;
            if (typeof(gracePeriod) !== "number" || isNaN(gracePeriod)) {
              return this._error(ERROR_INVALID_REFRESH_AUTH_VALUE);
            }
            // Forcing refreshAuth to silent is a contradiction in terms,
            // though it might succeed silently if we didn't reject here.
            if (aOptions.silent) {
              return this._error(ERROR_NO_SILENT_REFRESH_AUTH);
            }

            return this._refreshAuthentication(aAudience, user.accountId,
                                                 principal,
                                                 false /* logoutOnFailure */);
          }
          // Third case: we are all set *locally*. Probably we just return
          // the assertion, but the attempt might lead to the server saying
          // we are deleted or have a new password, which will trigger a flow.
          // Also we need to check if we have permission to get the assertion,
          // otherwise we need to show the forceAuth UI to let the user know
          // that the RP with no kaia permissions is trying to obtain an
          // assertion. Once the user authenticates herself in the forceAuth UI
          // the permission will be remembered by default.
          let permission = permissionManager.testPermissionFromPrincipal(
            principal,
            KAIACCOUNTS_PERMISSION
          );
          if (permission == Ci.nsIPermissionManager.PROMPT_ACTION &&
              !this._refreshing) {
            return this._refreshAuthentication(aAudience, user.accountId,
                                               principal,
                                               false /* logoutOnFailure */);
          } else if (permission == Ci.nsIPermissionManager.DENY_ACTION &&
                     !this._refreshing) {
            return this._error(ERROR_PERMISSION_DENIED);
          }
          return this._getAssertion(aAudience, principal);
        }
        log.debug("No signed in user");
        if (aOptions && aOptions.silent) {
          return Promise.resolve(null);
        }
        return this._uiRequest(UI_REQUEST_SIGN_IN_FLOW, aAudience, principal);
      }
    );
  }

};

KaiAccountsManager.init();
