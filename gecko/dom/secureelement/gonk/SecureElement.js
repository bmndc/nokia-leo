/* Copyright 2012 Mozilla Foundation and Mozilla contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* Copyright © 2014, Deutsche Telekom, Inc. */

"use strict";

/* globals dump, Components, XPCOMUtils, SE, Services, UiccConnector,
   SEUtils, ppmm, gMap, UUIDGenerator */

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/systemlibs.js");

XPCOMUtils.defineLazyServiceGetter(this, "gSettingsService",
                                   "@mozilla.org/settingsService;1",
                                   "nsISettingsService");

XPCOMUtils.defineLazyGetter(this, "SE", () => {
  let obj = {};
  Cu.import("resource://gre/modules/se_consts.js", obj);
  return obj;
});

XPCOMUtils.defineLazyGetter(this, "NFC", function () {
  let obj = {};
  Cu.import("resource://gre/modules/nfc_consts.js", obj);
  return obj;
});

const SE_IPC_SECUREELEMENT_MSG_NAMES = [
  "SE:GetSEReaders",
  "SE:OpenChannel",
  "SE:OpenBasicChannel",
  "SE:CloseChannel",
  "SE:TransmitAPDU",
  "SE:GetAtr",
  "SE:ResetSecureElement",
  "SE:LsExecuteScript",
  "SE:LsGetVersion",
];

const SECUREELEMENTMANAGER_CONTRACTID =
  "@mozilla.org/secureelement/parent-manager;1";
const SECUREELEMENTMANAGER_CID =
  Components.ID("{48f4e650-28d2-11e4-8c21-0800200c9a66}");
const NS_XPCOM_SHUTDOWN_OBSERVER_ID = "xpcom-shutdown";

XPCOMUtils.defineLazyServiceGetter(this, "ppmm",
                                   "@mozilla.org/parentprocessmessagemanager;1",
                                   "nsIMessageBroadcaster");

XPCOMUtils.defineLazyServiceGetter(this, "UUIDGenerator",
                                   "@mozilla.org/uuid-generator;1",
                                   "nsIUUIDGenerator");

XPCOMUtils.defineLazyModuleGetter(this, "SEUtils",
                                  "resource://gre/modules/SEUtils.jsm");

XPCOMUtils.defineLazyGetter(this, "UiccConnector", () => {
  let uiccClass = Cc["@mozilla.org/secureelement/connector/uicc;1"];
  return uiccClass ? uiccClass.getService(Ci.nsISecureElementConnector) : null;
});

XPCOMUtils.defineLazyGetter(this, "EseConnector", () => {
  let eseClass = Cc["@mozilla.org/secureelement/connector/ese;1"];
  return eseClass ? eseClass.getService(Ci.nsISecureElementConnector) : null;
});

const gSecureElementType = libcutils.property_get("ro.moz.nfc.secureelementtype", SE.TYPE_ESE);
const TOPIC_NFCD_UNINITIALIZED = "nfcd_uninitialized";
const TOPIC_CHANGE_RF_STATE = "nfc_change_rf_state";
const SETTING_SE_DEBUG = "nfc.debugging.enabled";
const TOPIC_MOZSETTINGS_CHANGED = "mozsettings-changed";

// set to true in se_consts.js to see debug messages
var DEBUG = SE.DEBUG_SE;
var debug;
function updateDebug() {
  if (DEBUG || SE.DEBUG_SE) {
    debug = function (s) {
      dump("-*- SecureElement: " + s + "\n");
    };
  } else {
    debug = function (s) {};
  }
};
updateDebug();

function getConnector(type) {
  switch (type) {
    case SE.TYPE_UICC:
      return UiccConnector;
    case SE.TYPE_ESE:
      return EseConnector;
    default:
      debug("Unsupported SEConnector : " + type);
      return null;
  }
}

/**
 * 'gMap' is a nested dictionary object that manages all the information
 * pertaining to channels for a given application (appId). It manages the
 * relationship between given application and its opened channels.
 */
XPCOMUtils.defineLazyGetter(this, "gMap", function() {
  return {
    // example structure of AppInfoMap
    // {
    //   "appId1": {
    //     target: target1,
    //     channels: {
    //       "channelToken1": {
    //         seType: "uicc",
    //         aid: "aid1",
    //         channelNumber: 1
    //       },
    //       "channelToken2": { ... }
    //     }
    //   },
    //  "appId2": { ... }
    // }
    appInfoMap: {},

    registerSecureElementTarget: function(appId, target) {
      if (this.isAppIdRegistered(appId)) {
        debug("AppId: " + appId + "already registered");
        return;
      }

      this.appInfoMap[appId] = {
        target: target,
        channels: {}
      };

      debug("Registered a new SE target " + appId);
    },

    unregisterSecureElementTarget: function(target) {
      let appId = Object.keys(this.appInfoMap).find((id) => {
        return this.appInfoMap[id].target === target;
      });

      if (!appId) {
        return;
      }

      debug("Unregistered SE Target for AppId: " + appId);
      delete this.appInfoMap[appId];
    },

    isAppIdRegistered: function(appId) {
      return this.appInfoMap[appId] !== undefined;
    },

    getChannelCountByAppIdType: function(appId, type) {
      return Object.keys(this.appInfoMap[appId].channels)
                   .reduce((cnt, ch) => ch.type === type ? ++cnt : cnt, 0);
    },

    // Add channel to the appId. Upon successfully adding the entry
    // this function will return the 'token'
    addChannel: function(appId, type, aid, channelNumber) {
      let token = UUIDGenerator.generateUUID().toString();
      this.appInfoMap[appId].channels[token] = {
        seType: type,
        aid: aid,
        channelNumber: channelNumber
      };
      return token;
    },

    removeChannel: function(appId, channelToken) {
      if (this.appInfoMap[appId].channels[channelToken]) {
        debug("Deleting channel with token : " + channelToken);
        delete this.appInfoMap[appId].channels[channelToken];
      }

    },

    isChannels: function() {
      let isChannels = false;
      Object.keys(this.appInfoMap).find((id) => {
        if (this.appInfoMap[id].channels) {
          if (Object.keys(this.appInfoMap[id].channels).length > 0) {
            isChannels = true;
          }
        }
      });
      return isChannels;
    },

    getChannel: function(appId, channelToken) {
      if (!this.appInfoMap[appId].channels[channelToken]) {
        return null;
      }

      return this.appInfoMap[appId].channels[channelToken];
    },

    getChannelsByTarget: function(target) {
      let appId = Object.keys(this.appInfoMap).find((id) => {
        return this.appInfoMap[id].target === target;
      });

      if (!appId) {
        return [];
      }

      return Object.keys(this.appInfoMap[appId].channels)
                   .map(token => this.appInfoMap[appId].channels[token]);
    },

    getTargets: function() {
      return Object.keys(this.appInfoMap)
                   .map(appId => this.appInfoMap[appId].target);
    },
  };
});

/**
 * 'SecureElementManager' is the main object that handles IPC messages from
 * child process. It interacts with other objects such as 'gMap' & 'Connector
 * instances (UiccConnector, eSEConnector)' to perform various
 * SE-related (open, close, transmit) operations.
 * @TODO: Bug 1118097 Support slot based SE/reader names
 * @TODO: Bug 1118101 Introduce SE type specific permissions
 */
function SecureElementManager() {
  let lock = gSettingsService.createLock();
  lock.get(SETTING_SE_DEBUG, this);
  Services.obs.addObserver(this, TOPIC_MOZSETTINGS_CHANGED, false);

  this._registerMessageListeners();
  Services.obs.addObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false);
  Services.obs.addObserver(this, TOPIC_NFCD_UNINITIALIZED, false);
  Services.obs.addObserver(this, TOPIC_CHANGE_RF_STATE, false);
  this._acEnforcer =
    Cc["@mozilla.org/secureelement/access-control/ace;1"]
    .getService(Ci.nsIAccessControlEnforcer);
}

SecureElementManager.prototype = {
  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIMessageListener,
    Ci.nsISEListener,
    Ci.nsIObserver]),
  classID: SECUREELEMENTMANAGER_CID,
  classInfo: XPCOMUtils.generateCI({
    classID:          SECUREELEMENTMANAGER_CID,
    classDescription: "SecureElementManager",
    interfaces:       [Ci.nsIMessageListener,
                       Ci.nsISEListener,
                       Ci.nsIObserver]
  }),

  // Stores information about supported SE types and their presence.
  // key: secure element type, value: (Boolean) is present/accessible
  _sePresence: {},

  _acEnforcer: null,

  _shutdown: function() {
    this._acEnforcer = null;
    this.secureelement = null;
    Services.obs.removeObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID);
    Services.obs.removeObserver(this, TOPIC_NFCD_UNINITIALIZED);
    Services.obs.removeObserver(this, TOPIC_CHANGE_RF_STATE);
    this._unregisterMessageListeners();
    this._unregisterSEListeners();
  },

  _registerMessageListeners: function() {
    ppmm.addMessageListener("child-process-shutdown", this);
    for (let msgname of SE_IPC_SECUREELEMENT_MSG_NAMES) {
      ppmm.addMessageListener(msgname, this);
    }
  },

  _unregisterMessageListeners: function() {
    ppmm.removeMessageListener("child-process-shutdown", this);
    for (let msgname of SE_IPC_SECUREELEMENT_MSG_NAMES) {
      ppmm.removeMessageListener(msgname, this);
    }
    ppmm = null;
  },

  _registerSEListeners: function() {
    debug("_registerSEListeners");
    let connector = getConnector(gSecureElementType);
    if (!connector) {
      return;
    }

    this._sePresence[gSecureElementType] = false;
    connector.registerListener(this);
  },

  _unregisterSEListeners: function() {
    debug("_unregisterSEListeners");
    Object.keys(this._sePresence).forEach((type) => {
      let connector = getConnector(type);
      if (connector) {
        connector.unregisterListener(this);
      }
    });

    this._sePresence = {};
  },

  notifySEPresenceChanged: function(type, isPresent) {
    // we need to notify all targets, even those without open channels,
    // app could've stored the reader without actually using it
    debug("notifying DOM about SE state change");
    this._sePresence[type] = isPresent;
    gMap.getTargets().forEach(target => {
      let result = { type: type, isPresent: isPresent };
      try {
        target.sendAsyncMessage("SE:ReaderPresenceChanged", { result: result });
      } catch (err) {
        debug("Target is not available");
      }
    });
  },

  _canOpenChannel: function(appId, type) {
    if (!this._sePresence[type]) {
      debug("can't open channel when SE is not presented");
      return;
    }
    let opened = gMap.getChannelCountByAppIdType(appId, type);
    let limit = SE.MAX_CHANNELS_ALLOWED_PER_SESSION;
    // UICC basic channel is not accessible see comment in se_consts.js
    limit = type === SE.TYPE_UICC ? limit - 1 : limit;
    return opened < limit;
  },

  _handleOpenChannel: function(msg, callback) {
    if (!this._canOpenChannel(msg.appId, msg.type)) {
      debug("Max channels per session exceed");
      callback({ error: SE.ERROR_GENERIC });
      return;
    }

    let connector = getConnector(msg.type);
    if (!connector) {
      debug("No SE connector available");
      callback({ error: SE.ERROR_NOTPRESENT });
      return;
    }

    let notifySuccess = (channelNumber, openResponse) => {
        // Add the new 'channel' to the map upon success
        let channelToken =
          gMap.addChannel(msg.appId, msg.type, msg.aid, channelNumber);
        if (channelToken) {
          callback({
            error: SE.ERROR_NONE,
            channelToken: channelToken,
            isBasicChannel: (channelNumber === SE.BASIC_CHANNEL),
            openResponse: SEUtils.hexStringToByteArray(openResponse)
          });
        } else {
          callback({ error: SE.ERROR_GENERIC });
        }
      };

    let notifyError = (reason) => {
        if (gMap.isChannels()) {
          debug("Failed to open channel to AID : " +
                SEUtils.byteArrayToHexString(msg.aid) +
                ", Rejected with Reason : " + reason);
          callback({ error: SE.ERROR_GENERIC, reason: reason, response: [] });
          return;
        }
        debug("No channel is used, close the secure element connection");
        // Close the connection if there is no channel used.
        connector.closeConnection({
          notifyCloseConnectionSuccess: function () {
            debug("Close secure element connection successfully");
            callback({ error: SE.ERROR_GENERIC, reason: reason, response: [] });
          },
          notifyError: function() {
            debug("Failed to close secure element connection");
            callback({ error: SE.ERROR_GENERIC, reason: reason, response: [] });
          }
        });
      };

    connector.getAtr({
      notifyGetAtrSuccess: (response) => {
        connector.openChannel(SEUtils.byteArrayToHexString(msg.aid), {
          notifyOpenChannelSuccess: notifySuccess,
          notifyError: notifyError
        });
      },

      notifyError: (reason) => {
        connector.openChannel(SEUtils.byteArrayToHexString(msg.aid), {
          notifyOpenChannelSuccess: notifySuccess,
          notifyError: notifyError
        });
      }
    });
  },

  _handleOpenBasicChannel: function(msg, callback) {
    if (!this._canOpenChannel(msg.appId, msg.type)) {
      debug("Max channels per session exceed");
      callback({ error: SE.ERROR_GENERIC });
      return;
    }

    let connector = getConnector(msg.type);
    if (!connector) {
      debug("No SE connector available");
      callback({ error: SE.ERROR_NOTPRESENT });
      return;
    }

    let notifySuccess = (channelNumber, openResponse) => {
        // Add the new 'channel' to the map upon success
        let channelToken =
          gMap.addChannel(msg.appId, msg.type, msg.aid, channelNumber);
        if (channelToken) {
          callback({
            error: SE.ERROR_NONE,
            channelToken: channelToken,
            isBasicChannel: (channelNumber === SE.BASIC_CHANNEL),
            openResponse: SEUtils.hexStringToByteArray(openResponse)
          });
        } else {
          callback({ error: SE.ERROR_GENERIC });
        }
      };

    let notifyError = (reason) => {
        if (gMap.isChannels()) {
          debug("Failed to open basic channel to AID : " +
                SEUtils.byteArrayToHexString(msg.aid) +
                ", Rejected with Reason : " + reason);
          callback({ error: SE.ERROR_GENERIC, reason: reason, response: [] });
          return;
        }
        debug("No channel is used, close the secure element connection");
        // Close the connection if there is no channel used.
        connector.closeConnection({
          notifyCloseConnectionSuccess: function () {
            debug("Close secure element connection successfully");
            callback({ error: SE.ERROR_GENERIC, reason: reason, response: [] });
          },
          notifyError: function() {
            debug("Failed to close secure element connection");
            callback({ error: SE.ERROR_GENERIC, reason: reason, response: [] });
          }
        });
      };

    connector.getAtr({
      notifyGetAtrSuccess: (response) => {
        connector.openBasicChannel(SEUtils.byteArrayToHexString(msg.aid), {
          notifyOpenChannelSuccess: notifySuccess,
          notifyError: notifyError
        });
      },

      notifyError: (reason) => {
        connector.openBasicChannel(SEUtils.byteArrayToHexString(msg.aid), {
          notifyOpenChannelSuccess: notifySuccess,
          notifyError: notifyError
        });
      }
    });
  },

  _handleTransmit: function(msg, callback) {
    let channel = gMap.getChannel(msg.appId, msg.channelToken);
    if (!channel) {
      debug("Invalid token:" + msg.channelToken + ", appId: " + msg.appId);
      callback({ error: SE.ERROR_GENERIC });
      return;
    }

    let connector = getConnector(channel.seType);
    if (!connector) {
      debug("No SE connector available");
      callback({ error: SE.ERROR_NOTPRESENT });
      return;
    }
    debug("_handleTransmit: " + channel.channelNumber + "=" + JSON.stringify(msg.apdu));
    // Bug 1137533 - ACE GPAccessRulesManager APDU filters
    let apduHeader = [];
    apduHeader.push(msg.apdu.cla);
    apduHeader.push(msg.apdu.ins);
    apduHeader.push(msg.apdu.p1);
    apduHeader.push(msg.apdu.p2);
    this._acEnforcer.isAPDUAccessAllowed(msg.appId, channel.seType, SEUtils.byteArrayToHexString(channel.aid), SEUtils.byteArrayToHexString(apduHeader))
    .then((allowed) => {
      if (!allowed) {
        callback({ error: SE.ERROR_SECURITY });
        return;
      }
      connector.exchangeAPDU(channel.channelNumber, msg.apdu.cla, msg.apdu.ins,
                             msg.apdu.p1, msg.apdu.p2,
                             SEUtils.byteArrayToHexString(msg.apdu.data),
                             msg.apdu.le, {
        notifyExchangeAPDUResponse: (sw1, sw2, response) => {
          callback({
            error: SE.ERROR_NONE,
            sw1: sw1,
            sw2: sw2,
            response: SEUtils.hexStringToByteArray(response)
          });
        },

        notifyError: (reason) => {
          debug("Transmit failed, rejected with Reason : " + reason);
          callback({ error: SE.ERROR_INVALIDAPPLICATION, reason: reason });
        }
      });
    })
    .catch((error) => {
      debug("Failed to get info from accessControlEnforcer " + error);
      callback({ error: SE.ERROR_SECURITY });
    });
  },

  _handleCloseChannel: function(msg, callback) {
    let channel = gMap.getChannel(msg.appId, msg.channelToken);
    if (!channel) {
      debug("Invalid token:" + msg.channelToken + ", appId:" + msg.appId);
      callback({ error: SE.ERROR_GENERIC });
      return;
    }

    let connector = getConnector(channel.seType);
    if (!connector) {
      debug("No SE connector available");
      callback({ error: SE.ERROR_NOTPRESENT });
      return;
    }
    connector.closeChannel(channel.channelNumber, {
      notifyCloseChannelSuccess: () => {
        gMap.removeChannel(msg.appId, msg.channelToken);
        if (gMap.isChannels()) {
          callback({ error: SE.ERROR_NONE });
          return;
        }
        debug("No channel is used, close the secure element connection");
        // Close the connection if there is no channel used.
        connector.closeConnection({
          notifyCloseConnectionSuccess: function () {
            debug("Close secure element connection successfully");
            callback({ error: SE.ERROR_NONE });
          },

          notifyError: function() {
            debug("Failed to close secure element connection");
            callback({ error: SE.ERROR_NONE });
          }
        });
      },

      notifyError: (reason) => {
        debug("Failed to close channel with token: " + msg.channelToken +
              ", reason: "+ reason);
        callback({ error: SE.ERROR_BADSTATE, reason: reason });
      }
    });
  },

  _handleResetSecureElement: function(msg, callback) {
    let connector = getConnector(gSecureElementType);
    if (!connector) {
      debug("No SE connector available");
      callback({ error: SE.ERROR_NOTPRESENT });
      return;
    }

    connector.resetSecureElement({
      notifyResetSecureElementSuccess: () => {
        debug("resetSecureElement successfully");
        callback({ error: SE.ERROR_NONE });
      },

      notifyError: (reason) => {
        debug("Failed to reset secure element" + ", reason: "+ reason);
        callback({ error: SE.ERROR_BADSTATE, reason: reason });
      }
    });
  },

  _handleGetAtr: function(msg, callback) {
    let connector = getConnector(gSecureElementType);
    if (!connector) {
      debug("No SE connector available");
      callback({ error: SE.ERROR_NOTPRESENT });
      return;
    }

    connector.getAtr({
      notifyGetAtrSuccess: (response) => {
        debug("_handleGetAtr: " + JSON.stringify(response));
        callback({
          error: SE.ERROR_NONE,
          response: response
        });
      },

      notifyError: (reason) => {
        debug("Failed to reset secure element" + ", reason: "+ reason);
        callback({ error: SE.ERROR_BADSTATE, reason: reason });
      }
    });
  },

  _handleLsExecuteScript: function(msg, callback) {
    let connector = getConnector(gSecureElementType);
    if (!connector) {
      debug("No SE connector available");
      callback({ error: SE.ERROR_NOTPRESENT });
      return;
    }

    connector.lsExecuteScript(msg, {
      notifyLsExecuteScriptSuccess: (response) => {
        callback({
          error: SE.ERROR_NONE,
          response: response
        });
      },

      notifyError: (reason) => {
        debug("Failed to do loader service" + ", reason: "+ reason);
        callback({ error: SE.ERROR_BADSTATE, reason: reason });
      }
    });
  },

  _handleLsGetVersion: function(msg, callback) {
    let connector = getConnector(gSecureElementType);
    if (!connector) {
      debug("No SE connector available");
      callback({ error: SE.ERROR_NOTPRESENT });
      return;
    }

    connector.lsGetVersion(msg, {
      notifyLsGetVersionSuccess: (response) => {
        callback({
          error: SE.ERROR_NONE,
          response: response
        });
      },

      notifyError: (reason) => {
        debug("Failed to get loader service version" + ", reason: "+ reason);
        callback({ error: SE.ERROR_BADSTATE, reason: reason });
      }
    });
  },

  _handleGetSEReadersRequest: function(msg, target, callback) {
    gMap.registerSecureElementTarget(msg.appId, target);
    let readers = Object.keys(this._sePresence).map(type => {
      return { type: type, isPresent: this._sePresence[type] };
    });
    callback({ readers: readers, error: SE.ERROR_NONE });
  },

  _handleChildProcessShutdown: function(target) {

    // Remove the requests when child process is dead.
    for (let i = this._stateRequests.length - 1; i >= 0; i--) {
      if (this._stateRequests[i].msg.target === target) {
        this._stateRequests.splice(i, 1);
      }
    }

    let channels = gMap.getChannelsByTarget(target);

    let channelLength = channels.length;

    let closeConn = (seType) => {
      let connector = getConnector(seType);
      // Don't close the connection if other applications are using the SE.
      if (!connector && gMap.isChannels()) {
        return;
      }
      connector.closeConnection({
        notifyCloseConnectionSuccess: () => {
          debug("Close secure element connection successfully");
        },

        notifyError: () => {
          debug("Failed to close secure element connection");
        }
      });
    };

    if (channelLength <= 0) {
      debug("Channels is empty, close the connection");
      closeConn(gSecureElementType);
      gMap.unregisterSecureElementTarget(target);
      return;
    }

    let processed = 0;
    let createCb = (seType, channelNumber) => {
      return {
        notifyCloseChannelSuccess: () => {
          debug("Closed " + seType + ", channel " + channelNumber);
          // Prevent power drain, force to close the connection.
          if (processed === channelLength) {
            debug("All channels is closed, close the connection");
            closeConn(seType);
          }
        },

        notifyError: (reason) => {
          debug("Failed to close  " + seType + " channel " +
                channelNumber + ", reason: " + reason);
          // Prevent power drain, force to close the connection.
          if (processed === channelLength) {
            debug("All channels is closed, close the connection");
            closeConn(seType);
          }
        }
      };
    };

    channels.forEach((channel) => {
      let connector = getConnector(channel.seType);
      if (!connector) {
        return;
      }

      processed++;

      connector.closeChannel(channel.channelNumber,
                             createCb(channel.seType, channel.channelNumber));
    });

    gMap.unregisterSecureElementTarget(target);
  },

  _sendSEResponse: function(msg, result) {
    let promiseStatus = (result.error === SE.ERROR_NONE) ? "Resolved" : "Rejected";
    result.resolverId = msg.data.resolverId;
    try {
      msg.target.sendAsyncMessage(msg.name + promiseStatus, {result: result});
    } catch (err) {
      debug("Target is not available");
    }
    this.requestDone();
  },

  _isValidMessage: function(msg) {
    let appIdValid = gMap.isAppIdRegistered(msg.data.appId);

    let isValidMsg = false;
    if (SE_IPC_SECUREELEMENT_MSG_NAMES.indexOf(msg.name) !== -1) {
      isValidMsg = true;
    }

    debug("_isValidMessage: msg " + isValidMsg + " appid " + appIdValid);
    return isValidMsg ? true : appIdValid;
  },

  _handleRFStateChange: function(rfState) {
    debug("_handleRFStateChange: " + JSON.stringify(rfState));
    if (rfState === NFC.NFC_RF_STATE_IDLE) {
      // TODO, handle screen off case.
      //this._unregisterSEListeners();
      this._handleSEError();
    } else if (rfState === NFC.NFC_RF_STATE_LISTEN || rfState === NFC.NFC_RF_STATE_DISCOVERY) {
      this._registerSEListeners();
    } else {
      debug("Unknow RF state " + rfState);
    }
  },

  _stateRequests: [],

  _requestProcessing: false,

  _currentRequest: null,

  queueRequest: function(msg, callback) {
    if (!callback) {
        throw "Try to enqueue a request without callback";
    }

    this._stateRequests.push({
      msg: msg,
      callback: callback
    });

    this.nextRequest();
  },

  requestDone: function requestDone() {
    this._currentRequest = null;
    this._requestProcessing = false;
    this.nextRequest();
  },

  nextRequest: function nextRequest() {
    // No request to process
    if (this._stateRequests.length === 0) {
      return;
    }

    // Handling request, wait for it.
    if (this._requestProcessing) {
      return;
    }
    // Hold processing lock
    this._requestProcessing = true;

    // Find next valid request
    this._currentRequest = this._stateRequests.shift();

    this.handleRequest(this._currentRequest);
  },

  handleRequest: function(request) {
    let callback = request.callback;
    let msg = request.msg;
    switch (msg.name) {
      case "SE:GetSEReaders":
        this._handleGetSEReadersRequest(msg.data, msg.target, callback);
        break;
      case "SE:OpenChannel":
        this._handleOpenChannel(msg.data, callback);
        break;
      case "SE:OpenBasicChannel":
        this._handleOpenBasicChannel(msg.data, callback);
        break;
      case "SE:CloseChannel":
        this._handleCloseChannel(msg.data, callback);
        break;
      case "SE:TransmitAPDU":
        this._handleTransmit(msg.data, callback);
        break;
      case "SE:ResetSecureElement":
        this._handleResetSecureElement(msg.data, callback);
        break;
      case "SE:GetAtr":
        this._handleGetAtr(msg.data, callback);
        break;
      case "SE:LsExecuteScript":
        this._handleLsExecuteScript(msg.data, callback);
        break;
      case "SE:LsGetVersion":
        this._handleLsGetVersion(msg.data, callback);
        break;
    }
  },

  /**
   * nsIMessageListener interface methods.
   */

  receiveMessage: function(msg) {
    DEBUG && debug("Received '" + msg.name + "' message from content process" +
                   ": " + JSON.stringify(msg.data));

    if (msg.name === "child-process-shutdown") {
      this._handleChildProcessShutdown(msg.target);
      return null;
    }

    if (SE_IPC_SECUREELEMENT_MSG_NAMES.indexOf(msg.name) !== -1) {
      if (!msg.target.assertPermission("secureelement-manage")) {
        debug("SecureElement message " + msg.name + " from a content process " +
              "with no 'secureelement-manage' privileges.");
        return null;
      }
    } else {
      debug("Ignoring unknown message type: " + msg.name);
      return null;
    }
    let self = this;
    let callback = (result) => self._sendSEResponse(msg, result);

    if (!this._isValidMessage(msg)) {
      debug("Message not valid");
      callback({ error: SE.ERROR_GENERIC });
      return null;
    }

    this.queueRequest(msg, callback);

    return null;
  },


  _sendSEError: function(msg, result) {
    let promiseStatus = (result.error === SE.ERROR_NONE) ? "Resolved" : "Rejected";
    result.resolverId = msg.data.resolverId;
    if (msg.name == "SE:LsExecuteScript" || msg.name == "SE:LsExecuteScript") {
      result.reason = "00000000";
    } else {
      result.reason = "SE is not available";
    }

    // This might be failed when child process is dead.
    try {
      msg.target.sendAsyncMessage(msg.name + promiseStatus, {result: result});
    } catch (err) {
      debug("Child process might be dead");
    }
  },

  _handleSEError: function () {
    debug("_handleSEError");
    // Check and notify error to current request.
    if (this._currentRequest) {
      let msg = this._currentRequest.msg;
      this._sendSEError(msg, {error:SE.ERROR_NOTPRESENT});
    }

    // Check and notify error to others request in queue.
    while (this._stateRequests.length) {
      let request = this._stateRequests.shift();
      let msg = request.msg;
      this._sendSEError(msg, {error:SE.ERROR_NOTPRESENT});
    }

    this._requestProcessing = false;

    let connector = getConnector(gSecureElementType);

    if (!connector) {
      return;
    }

    connector.closeConnection({
      notifyCloseConnectionSuccess: function () {
        debug("Close secure element connection successfully");
      },

      notifyError: function() {
        debug("Failed to close secure element connection");
      }
    });
  },

  /**
   * nsISettingsServiceCallback
   */
  handle: function handle(name, result) {
    switch (name) {
      case SETTING_SE_DEBUG:
        DEBUG = result;
        updateDebug();
        break;
    }
  },

  /**
   * nsIObserver interface methods.
   */

  observe: function(subject, topic, data) {
    let self = this;
    if (topic === NS_XPCOM_SHUTDOWN_OBSERVER_ID) {
      this._shutdown();
    } else if (topic === TOPIC_NFCD_UNINITIALIZED) {
      this._handleSEError();
    } else if (topic === TOPIC_CHANGE_RF_STATE) {
      this._handleRFStateChange(data);
    } else if (topic === TOPIC_MOZSETTINGS_CHANGED) {
      if ("wrappedJSObject" in subject) {
        subject = subject.wrappedJSObject;
      }
      if (subject) {
        this.handle(subject.key, subject.value);
      }
    }
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([SecureElementManager]);
