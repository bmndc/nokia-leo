/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * Some specific (certified) apps need to get access to certain KaiOS Accounts
 * functionality that allows them to manage accounts (this is mostly sign up,
 * sign in, logout and delete) and get information about the currently existing
 * ones.
 *
 * This service listens for requests coming from these apps, triggers the
 * appropriate Kai Accounts flows and send reponses back to the UI.
 *
 * The communication mechanism is based in mozFxAccountsContentEvent (for
 * messages coming from the UI) and mozFxAccountsChromeEvent (for messages
 * sent from the chrome side) custom events.
 */

"use strict";

this.EXPORTED_SYMBOLS = ["KaiAccountsMgmtService"];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/KaiAccountsCommon.js");

XPCOMUtils.defineLazyModuleGetter(this, "KaiAccountsManager",
  "resource://gre/modules/KaiAccountsManager.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "SystemAppProxy",
                                  "resource://gre/modules/SystemAppProxy.jsm");

this.KaiAccountsMgmtService = {
  _onFulfill: function(aMsgId, aData) {
    SystemAppProxy._sendCustomEvent("mozFxAccountsChromeEvent", {
      id: aMsgId,
      data: aData ? aData : null
    });
  },

  _onReject: function(aMsgId, aReason) {
    SystemAppProxy._sendCustomEvent("mozFxAccountsChromeEvent", {
      id: aMsgId,
      error: aReason ? aReason : null
    });
  },

  init: function() {
    Services.obs.addObserver(this, ONLOGIN_NOTIFICATION, false);
    Services.obs.addObserver(this, ONVERIFIED_NOTIFICATION, false);
    Services.obs.addObserver(this, ONLOGOUT_NOTIFICATION, false);
    SystemAppProxy.addEventListener("mozFxAccountsContentEvent",
                                    KaiAccountsMgmtService);
  },

  observe: function(aSubject, aTopic, aData) {
    log.debug("Observed " + aTopic);
    switch (aTopic) {
      case ONLOGIN_NOTIFICATION:
      case ONVERIFIED_NOTIFICATION:
      case ONLOGOUT_NOTIFICATION:
        // KaiAccounts notifications have the form of kaiaccounts:*
        SystemAppProxy._sendCustomEvent("mozFxAccountsUnsolChromeEvent", {
          eventName: aTopic.substring(aTopic.indexOf(":") + 1)
        });
        break;
    }
  },

  handleEvent: function(aEvent) {
    let msg = aEvent.detail;
    log.debug("MgmtService got content event: " + JSON.stringify(msg));
    let self = KaiAccountsMgmtService;

    if (!msg.id) {
      return;
    }

    if (msg.error) {
      self._onReject(msg.id, msg.error);
      return;
    }

    let data = msg.data;
    if (!data) {
      return;
    }

    switch (data.method) {
      case "getAccounts":
        KaiAccountsManager.getAccount().then(
          account => {
            self._onFulfill(msg.id, account);
          },
          reason => {
            self._onReject(msg.id, reason);
          }
        ).then(null, Components.utils.reportError);
        break;
      case "logout":
        KaiAccountsManager.signOut().then(
          () => {
            self._onFulfill(msg.id);
          },
          reason => {
            self._onReject(msg.id, reason);
          }
        ).then(null, Components.utils.reportError);
        break;
      case "signIn":
        KaiAccountsManager[data.method](data.accountId, data.password).then(
          user => {
            self._onFulfill(msg.id, user);
          },
          reason => {
            self._onReject(msg.id, reason);
          }
        ).then(null, Components.utils.reportError);
        break;
      case "signUp":
        KaiAccountsManager.signUp(data.phone, data.email, data.password, data.info).then(
          result => {
            self._onFulfill(msg.id, result);
          },
          reason => {
            self._onReject(msg.id, reason);
          }
        ).then(null, Components.utils.reportError);
        break;
      case "verifyPassword":
        KaiAccountsManager[data.method](data.password).then(
          user => {
            self._onFulfill(msg.id, user);
          },
          reason => {
            self._onReject(msg.id, reason);
          }
        ).then(null, Components.utils.reportError);
        break;
      case "changePassword":
        KaiAccountsManager.changePassword(data.oldPassword, data.newPassword).then(
          result => {
            self._onFulfill(msg.id, result);
          },
          reason => {
            self._onReject(msg.id, reason);
          }
        ).then(null, Components.utils.reportError);
        break;
      case "updateAccount":
        KaiAccountsManager[data.method](data.phone, data.email, data.info, data.password).then(
          result => {
            self._onFulfill(msg.id, result);
          },
          reason => {
            self._onReject(msg.id, reason);
          }
        ).then(null, Components.utils.reportError);
        break;
      case "deleteAccount":
        KaiAccountsManager[data.method]().then(
          result => {
            self._onFulfill(msg.id, result);
          },
          reason => {
            self._onReject(msg.id, reason);
          }
        ).then(null, Components.utils.reportError);
        break;
      // TODO: remove the below methods when the methods get implemented in gaia
      case "checkPhoneExistence":
        KaiAccountsManager[data.method](data.phone).then(
          result => {
            self._onFulfill(msg.id, result);
          },
          reason => {
            self._onReject(msg.id, reason);
          }
        ).then(null, Components.utils.reportError);
        break;
      case "checkEmailExistence":
        KaiAccountsManager[data.method](data.email).then(
          result => {
            self._onFulfill(msg.id, result);
          },
          reason => {
            self._onReject(msg.id, reason);
          }
        ).then(null, Components.utils.reportError);
        break;
      case "requestPhoneVerification":
        KaiAccountsManager[data.method](data.phone, data.uid).then(
          result => {
            self._onFulfill(msg.id, result);
          },
          reason => {
            self._onReject(msg.id, reason);
          }
        ).then(null, Components.utils.reportError);
        break;
      case "resolvePhoneVerification":
        KaiAccountsManager[data.method](data.phone, data.verificationId, data.code, data.uid).then(
          result => {
            self._onFulfill(msg.id, result);
          },
          reason => {
            self._onReject(msg.id, reason);
          }
        ).then(null, Components.utils.reportError);
        break;
      case "requestEmailVerification":
        KaiAccountsManager[data.method](data.email, data.uid).then(
          result => {
            self._onFulfill(msg.id, result);
          },
          reason => {
            self._onReject(msg.id, reason);
          }
        ).then(null, Components.utils.reportError);
        break;
    }
  }
};

KaiAccountsMgmtService.init();
