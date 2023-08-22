/* Copyright 2012 Mozilla Foundation and Mozilla contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* Copyright © 2013, Deutsche Telekom, Inc. */

"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/FileUtils.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "gSettingsService",
                                   "@mozilla.org/settingsService;1",
                                   "nsISettingsService");

XPCOMUtils.defineLazyModuleGetter(this, "SEUtils",
                                  "resource://gre/modules/SEUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "DOMApplicationRegistry",
                                  "resource://gre/modules/Webapps.jsm");

XPCOMUtils.defineLazyGetter(this, "NFC", function () {
  let obj = {};
  Cu.import("resource://gre/modules/nfc_consts.js", obj);
  return obj;
});

Cu.import("resource://gre/modules/systemlibs.js");
const NFC_ENABLED = libcutils.property_get("ro.moz.nfc.enabled", "false") === "true";
const LOADER_SERVICE_TMP = "/data/nfc/";
const LOADER_SERVICE_SD = "/storage/emulated/";
const TOPIC_NFCD_UNINITIALIZED = "nfcd_uninitialized";
const TOPIC_CHANGE_RF_STATE = "nfc_change_rf_state";
// set to true in nfc_consts.js to see debug messages
var DEBUG = NFC.DEBUG_NFC;

var debug;
function updateDebug() {
  if (DEBUG || NFC.DEBUG_NFC) {
    debug = function (s) {
      dump("-*- Nfc: " + s + "\n");
    };
    // Update the debug property used in nfcd.
    libcutils.property_set("debug.nfcd.enabled", "true");
  } else {
    debug = function (s) {};
    // Update the debug property used in nfcd.
    libcutils.property_set("debug.nfcd.enabled", "false");
  }
};
updateDebug();

const NFC_CONTRACTID = "@mozilla.org/nfc;1";
const NFC_CID =
  Components.ID("{2ff24790-5e74-11e1-b86c-0800200c9a66}");

const NFC_IPC_MSG_ENTRIES = [
  { permission: null,
    messages: ["NFC:AddEventListener",
               "NFC:RemoveEventListener",
               "NFC:QueryInfo",
               "NFC:CallDefaultFoundHandler",
               "NFC:CallDefaultLostHandler"] },

  { permission: "nfc",
    messages: ["NFC:ReadNDEF",
               "NFC:WriteNDEF",
               "NFC:MakeReadOnly",
               "NFC:Format",
               "NFC:Transceive"] },

  { permission: "nfc-share",
    messages: ["NFC:SendFile",
               "NFC:RegisterPeerReadyTarget",
               "NFC:UnregisterPeerReadyTarget"] },

  { permission: "nfc-manager",
    messages: ["NFC:CheckP2PRegistration",
               "NFC:NotifyUserAcceptedP2P",
               "NFC:NotifySendFileStatus",
               "NFC:ChangeRFState",
               "NFC:SetFocusTab",
               "NFC:MPOSReaderMode",
               "NFC:NfcSelfTest",
               "NFC:SetConfig",
               "NFC:SetTagReaderMode"] }
];

// Should be consistent with NfcRequestType defined in NfcOptions.webidl.
const NfcRequestType = {
  CHANGE_RF_STATE: "changeRFState",
  READ_NDEF: "readNDEF",
  WRITE_NDEF: "writeNDEF",
  MAKE_READ_ONLY: "makeReadOnly",
  FORMAT: "format",
  TRANSCEIVE: "transceive",
  OPENCONNECTION: "openConnection",
  TRANSMIT: "transmit",
  CLOSECONNECTION: "closeConnection",
  RESETSECUREELEMENT: "resetSecureElement",
  GETATR: "getAtr",
  LSEXECUTESCRIPT: "lsExecuteScript",
  LSGETVERSION: "lsGetVersion",
  MPOSREADERMODE: "mPOSReaderMode",
  NFCSELFTEST: "nfcSelfTest",
  SETCONFIG: "setConfig",
  TAGREADERMODE: "tagReaderMode"
};

const CommandMsgTable = {};
CommandMsgTable["NFC:ChangeRFState"] = NfcRequestType.CHANGE_RF_STATE;
CommandMsgTable["NFC:ReadNDEF"] = NfcRequestType.READ_NDEF;
CommandMsgTable["NFC:WriteNDEF"] = NfcRequestType.WRITE_NDEF;
CommandMsgTable["NFC:MakeReadOnly"] = NfcRequestType.MAKE_READ_ONLY;
CommandMsgTable["NFC:Format"] = NfcRequestType.FORMAT;
CommandMsgTable["NFC:Transceive"] = NfcRequestType.TRANSCEIVE;
CommandMsgTable["NFC:OpenConnection"] = NfcRequestType.OPENCONNECTION;
CommandMsgTable["NFC:Transmit"] = NfcRequestType.TRANSMIT;
CommandMsgTable["NFC:CloseConnection"] = NfcRequestType.CLOSECONNECTION;
CommandMsgTable["NFC:ResetSecureElement"] = NfcRequestType.RESETSECUREELEMENT;
CommandMsgTable["NFC:GetAtr"] = NfcRequestType.GETATR;
CommandMsgTable["NFC:LsExecuteScript"] = NfcRequestType.LSEXECUTESCRIPT;
CommandMsgTable["NFC:LsGetVersion"] = NfcRequestType.LSGETVERSION;
CommandMsgTable["NFC:MPOSReaderMode"] = NfcRequestType.MPOSREADERMODE;
CommandMsgTable["NFC:NfcSelfTest"] = NfcRequestType.NFCSELFTEST;
CommandMsgTable["NFC:SetConfig"] = NfcRequestType.SETCONFIG;
CommandMsgTable["NFC:SetTagReaderMode"] = NfcRequestType.TAGREADERMODE;

// Should be consistent with NfcResponseType defined in NfcOptions.webidl.
const NfcResponseType = {
  CHANGE_RF_STATE_RSP: "changeRFStateRsp",
  READ_NDEF_RSP: "readNDEFRsp",
  WRITE_NDEF_RSP: "writeNDEFRsp",
  MAKE_READ_ONLY_RSP: "makeReadOnlyRsp",
  FORMAT_RSP: "formatRsp",
  TRANSCEIVE_RSP: "transceiveRsp",
  OPENCONNECTION_RSP: "openConnectionRsp",
  TRANSMIT_RSP: "transmitRsp",
  CLOSECONNECTION_RSP: "closeConnectionRsp",
  RESETSECUREELEMENT_RSP: "resetSecureElementRsp",
  GETATR_RSP: "getAtrRsp",
  LSEXECUTESCRIPT_RSP: "lsExecuteScriptRsp",
  LSGETVERSION_RSP: "lsGetVersionRsp",
  MPOSREADERMODE_RSP: "mPOSReaderModeRsp",
  CONNECTERROR_RSP: "connectErrorRsp",
  NFCSELFTEST_RSP: "nfcSelfTestRsp",
  SETCONFIG_RSP: "setConfigRsp",
  TAGREADERMODE_RSP: "tagReaderModeRsp"
};

const EventMsgTable = {};
EventMsgTable[NfcResponseType.CHANGE_RF_STATE_RSP] = "NFC:ChangeRFStateResponse";
EventMsgTable[NfcResponseType.READ_NDEF_RSP] = "NFC:ReadNDEFResponse";
EventMsgTable[NfcResponseType.WRITE_NDEF_RSP] = "NFC:WriteNDEFResponse";
EventMsgTable[NfcResponseType.MAKE_READ_ONLY_RSP] = "NFC:MakeReadOnlyResponse";
EventMsgTable[NfcResponseType.FORMAT_RSP] = "NFC:FormatResponse";
EventMsgTable[NfcResponseType.TRANSCEIVE_RSP] = "NFC:TransceiveResponse";
EventMsgTable[NfcResponseType.OPENCONNECTION_RSP] = "NFC:OpenConnectionResponse";
EventMsgTable[NfcResponseType.TRANSMIT_RSP] = "NFC:TransmitResponse";
EventMsgTable[NfcResponseType.CLOSECONNECTION_RSP] = "NFC:CloseConnectionResponse";
EventMsgTable[NfcResponseType.RESETSECUREELEMENT_RSP] = "NFC:ResetSecureElementResponse";
EventMsgTable[NfcResponseType.GETATR_RSP] = "NFC:GetAtrResponse";
EventMsgTable[NfcResponseType.LSEXECUTESCRIPT_RSP] = "NFC:LsExecuteScriptResponse";
EventMsgTable[NfcResponseType.LSGETVERSION_RSP] = "NFC:LsGetVersionResponse";
EventMsgTable[NfcResponseType.MPOSREADERMODE_RSP] = "NFC:MPOSReaderModeResponse";
EventMsgTable[NfcResponseType.NFCSELFTEST_RSP] = "NFC:NfcSelfTestResponse";
EventMsgTable[NfcResponseType.SETCONFIG_RSP] = "NFC:SetConfigResponse";
EventMsgTable[NfcResponseType.TAGREADERMODE_RSP] = "NFC:SetTagReaderModeResponse";

// Should be consistent with NfcNotificationType defined in NfcOptions.webidl.
const NfcNotificationType = {
  INITIALIZED: "initialized",
  TECH_DISCOVERED: "techDiscovered",
  TECH_LOST: "techLost",
  HCI_EVENT_TRANSACTION: "hciEventTransaction",
  NDEF_RECEIVED: "ndefReceived",
  MPOS_READER_MODE_EVENT: "mPOSReaderModeEvent",
  RF_FIELD_ACTIVATED_EVENT: "rfFieldActivateEvent",
  RF_FIELD_DEACTIVATED_EVENT: "rfFieldDeActivateEvent",
  LISTEN_MODE_ACTIVATED_EVENT: "listenModeActivateEvent",
  LISTEN_MODE_DEACTIVATED_EVENT: "listenModeDeActivateEvent",
  UNINITIALIZED: "unInitialized",
  ENABLETIMEOUT: "enableTimeout",
  DISABLETIMEOUT: "disableTimeout"
};

XPCOMUtils.defineLazyServiceGetter(this, "ppmm",
                                   "@mozilla.org/parentprocessmessagemanager;1",
                                   "nsIMessageBroadcaster");
XPCOMUtils.defineLazyServiceGetter(this, "gSystemMessenger",
                                   "@mozilla.org/system-message-internal;1",
                                   "nsISystemMessagesInternal");
XPCOMUtils.defineLazyServiceGetter(this, "UUIDGenerator",
                                    "@mozilla.org/uuid-generator;1",
                                    "nsIUUIDGenerator");
XPCOMUtils.defineLazyGetter(this, "gMessageManager", function () {
  return {
    QueryInterface: XPCOMUtils.generateQI([Ci.nsIMessageListener,
                                           Ci.nsIObserver]),

    nfc: null,

    // Manage registered Peer Targets
    peerTargets: {},

    eventListeners: {},

    focusId: NFC.SYSTEM_APP_ID,

    focusAppManifestUrl: "",

    init: function init(nfc) {
      this.nfc = nfc;
      Services.obs.addObserver(this, NFC.TOPIC_XPCOM_SHUTDOWN, false);
      this._registerMessageListeners();
    },

    listenDebugEvent: function listenDebugEvent() {
      let lock = gSettingsService.createLock();
      lock.get(NFC.SETTING_NFC_DEBUG, this.nfc);
      Services.obs.addObserver(this, NFC.TOPIC_MOZSETTINGS_CHANGED, false);
    },

    _shutdown: function _shutdown() {
      this.nfc.shutdown();
      this.nfc = null;

      Services.obs.removeObserver(this, NFC.TOPIC_MOZSETTINGS_CHANGED);
      Services.obs.removeObserver(this, NFC.TOPIC_XPCOM_SHUTDOWN);
      this._unregisterMessageListeners();
    },

    _registerMessageListeners: function _registerMessageListeners() {
      ppmm.addMessageListener("child-process-shutdown", this);

      for (let entry of NFC_IPC_MSG_ENTRIES) {
        for (let message of entry.messages) {
          ppmm.addMessageListener(message, this);
        }
      }
    },

    _unregisterMessageListeners: function _unregisterMessageListeners() {
      ppmm.removeMessageListener("child-process-shutdown", this);

      for (let entry of NFC_IPC_MSG_ENTRIES) {
        for (let message of entry.messages) {
          ppmm.removeMessageListener(message, this);
        }
      }

      ppmm = null;
    },

    registerPeerReadyTarget: function registerPeerReadyTarget(target, appId) {
      if (!this.peerTargets[appId]) {
        this.peerTargets[appId] = target;
      }
    },

    unregisterPeerReadyTarget: function unregisterPeerReadyTarget(appId) {
      if (this.peerTargets[appId]) {
        delete this.peerTargets[appId];
      }
    },

    removePeerTarget: function removePeerTarget(target) {
      Object.keys(this.peerTargets).forEach((appId) => {
        if (this.peerTargets[appId] === target) {
          delete this.peerTargets[appId];
        }
      });
    },

    notifyFocusTab: function notifyFocusTab(options) {
      let tabId = this.getFocusTabId();
      options.tabId = tabId;

      this.notifyDOMEvent(this.eventListeners[tabId], options);
    },

    notifyDOMEvent: function notifyDOMEvent(target, options) {
      if (!target) {
        dump("invalid target");
        return;
      }

      target.sendAsyncMessage("NFC:DOMEvent", options);
    },

    getFocusTabId: function getFocusTabId() {
      return this.eventListeners[this.focusId] ? this.focusId
                                               : NFC.SYSTEM_APP_ID;
    },

    setFocusTab: function setFocusTab(id, focusAppManifestUrl, isFocus) {
      // if calling setNFCFocus(true) on the browser-element which is already
      // focused, or calling setNFCFocus(false) on the browser-element which has
      // lost focus already, ignore.
      if (isFocus) {
        this.focusAppManifestUrl = focusAppManifestUrl;
      }
      if (isFocus == (id == this.focusId)) {
        return;
      }

      if (this.focusId != NFC.SYSTEM_APP_ID) {
        this.onFocusChanged(this.focusId, false);
      }

      if (isFocus) {
        // Now we only support one focus app.
        this.focusId = id;
        this.onFocusChanged(this.focusId, true);
      } else if (this.focusId == id){
        // Set focusId to SystemApp means currently there is no foreground app.
        this.focusId = NFC.SYSTEM_APP_ID;
      }
    },

    addEventListener: function addEventListener(target, id) {
      if (this.eventListeners[id] !== undefined) {
        return;
      }

      this.eventListeners[id] = target;
    },

    removeEventListener: function removeEventListener(target) {
      for (let id in this.eventListeners) {
        if (target == this.eventListeners[id]) {
          this.removeEventListenerById(id);
          break;
        }
      }
    },

    removeEventListenerById: function removeEventListenerById(id) {
      delete this.eventListeners[id];
    },

    checkP2PRegistration: function checkP2PRegistration(message) {
      let target = this.peerTargets[message.data.appId];
      let sessionToken = SessionHelper.getCurrentP2PToken();
      let isValid = (sessionToken != null) && (target != null);
      let respMsg = { requestId: message.data.requestId };
      if (!isValid) {
        respMsg.errorMsg = this.nfc.getErrorMessage(NFC.NFC_GECKO_ERROR_P2P_REG_INVALID);
      }
      // Notify the content process immediately of the status
      message.target.sendAsyncMessage(message.name + "Response", respMsg);
    },

    notifyUserAcceptedP2P: function notifyUserAcceptedP2P(appId) {
      let target = this.peerTargets[appId];
      let sessionToken = SessionHelper.getCurrentP2PToken();
      if (!sessionToken || !target) {
        debug("Peer already lost or " + appId + " is not a registered PeerReadytarget");
        return;
      }

      this.notifyDOMEvent(target, {tabId: this.getFocusTabId(),
                                   event: NFC.PEER_EVENT_READY,
                                   sessionToken: sessionToken});
    },

    notifySendFileStatus: function notifySendFileStatus(message) {
      if (message.data.status) {
        message.data.errorMsg =
            this.nfc.getErrorMessage(NFC.NFC_GECKO_ERROR_SEND_FILE_FAILED);
      }
      this.nfc.sendFileStatusResponse(message.data);
    },

    callDefaultFoundHandler: function callDefaultFoundHandler(message) {
      let sysMsg = new NfcTechDiscoveredSysMsg(message.sessionToken,
                                               message.isP2P,
                                               message.records || null);
      gSystemMessenger.broadcastMessage("nfc-manager-tech-discovered", sysMsg);
    },

    callDefaultLostHandler: function callDefaultLostHandler(message) {
      // message.isP2P is not used.
      gSystemMessenger.broadcastMessage("nfc-manager-tech-lost", message.sessionToken);
    },

    onTagFound: function onTagFound(message) {
      message.event = NFC.TAG_EVENT_FOUND;
      this.notifyFocusTab(message);
      delete message.event;
    },

    onTagLost: function onTagLost(sessionToken) {
      this.notifyFocusTab({ event: NFC.TAG_EVENT_LOST,
                            sessionToken: sessionToken });
    },

    onPeerEvent: function onPeerEvent(eventType, sessionToken) {
      this.notifyFocusTab({ event: eventType,
                            sessionToken: sessionToken });
    },

    onRFStateChanged: function onRFStateChanged(rfState) {
      for (let id in this.eventListeners) {
        this.notifyDOMEvent(this.eventListeners[id],
                            { tabId: id,
                              event: NFC.RF_EVENT_STATE_CHANGED,
                              rfState: rfState });
      }
    },

    onFocusChanged: function onFocusChanged(focusId, focus) {
      let target = this.eventListeners[focusId];
      if (!target) {
        return;
      }

      this.notifyDOMEvent(target, { tabId: this.focusId,
                                    event: NFC.FOCUS_CHANGED,
                                    focus: focus });
    },

    onMPOSReaderModeEvent: function onMPOSReaderModeEvent(message) {
      let tabId = this.getFocusTabId();
      for (let id in this.eventListeners) {
        if (id == tabId) {
          this.notifyDOMEvent(this.eventListeners[id],
                              { tabId: id,
                                event: NFC.MPOS_READER_MODE_EVENT,
                                type: message.mPOSReaderModeEvent });
        }
      }
    },

    isAct: false,
    pending: false,
    previous: false,
    debounceTimer: null,

    debounce: function debounce (callback, wait, immediate) {
      if (!this.pending) {
        this.pending = true;

        if (immediate) {
          callback();
          return;
        }

        if (!this.debounceTimer) {
          this.debounceTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
        }
        this.debounceTimer.initWithCallback(function(timer) {
          callback();
          }, wait, Ci.nsITimer.TYPE_ONE_SHOT);
      }
    },

    onRfFieldEvent: function onRfFieldEvent(isAct) {
      this.isAct = isAct;
      let self = this;
      this.debounce(function() {

        if (self.previous != self.isAct) {
          let tabId = self.getFocusTabId();
          for (let id in self.eventListeners) {
            if (id == tabId) {
              self.notifyDOMEvent( self.eventListeners[id],
                { tabId: id,
                  event: self.isAct ?
                         NFC.RF_FIELD_ACTIVATE_EVENT : NFC.RF_FIELD_DEACTIVATE_EVENT
                });
              debug("Fire RfFieldEvent: " + self.isAct + " to tab " + id);
            }
          }
          self.previous = self.isAct;
        }
        self.pending = false;
      }, 250, true);
    },

    onListenModeEvent: function onListenModeEvent(isAct) {
      let tabId = this.getFocusTabId();
      for (let id in this.eventListeners) {
        if (id == tabId) {
          this.notifyDOMEvent(this.eventListeners[id],
                { tabId: id,
                  event: isAct ?
                         NFC.LISTEN_MODE_ACTIVATE_EVENT : NFC.LISTEN_MODE_DEACTIVATE_EVENT
                });
          debug("Fire Listen Mode Event: " + isAct + " to tab " + id);
        }
      }
    },

    /**
     * nsIMessageListener interface methods.
     */

    receiveMessage: function receiveMessage(message) {
      DEBUG && debug("Received message from content process: " + JSON.stringify(message));

      if (message.name == "child-process-shutdown") {
        this.removePeerTarget(message.target);
        this.nfc.removeTarget(message.target);
        this.nfc.onChildProcessShutdown(message.target);
        this.removeEventListener(message.target);
        return null;
      }

      for (let entry of NFC_IPC_MSG_ENTRIES) {
        if (entry.messages.indexOf(message.name) != -1) {
          if (entry.permission &&
              !message.target.assertPermission(entry.permission)) {
            debug("Nfc message " + message.name + "doesn't have " +
                  entry.permission + " permission.");
            return null;
          }
          break;
        }
      }

      switch (message.name) {
        case "NFC:SetFocusTab":
          this.setFocusTab(message.data.tabId, message.data.focusAppManifestUrl, message.data.isFocus);
          return null;
        case "NFC:AddEventListener":
          this.addEventListener(message.target, message.data.tabId);
          return null;
        case "NFC:RemoveEventListener":
          this.removeEventListenerById(message.data.tabId);
          return null;
        case "NFC:RegisterPeerReadyTarget":
          this.registerPeerReadyTarget(message.target, message.data.appId);
          return null;
        case "NFC:UnregisterPeerReadyTarget":
          this.unregisterPeerReadyTarget(message.data.appId);
          return null;
        case "NFC:CheckP2PRegistration":
          this.checkP2PRegistration(message);
          return null;
        case "NFC:NotifyUserAcceptedP2P":
          this.notifyUserAcceptedP2P(message.data.appId);
          return null;
        case "NFC:NotifySendFileStatus":
          // Upon receiving the status of sendFile operation, send the response
          // to appropriate content process.
          this.notifySendFileStatus(message);
          return null;
        case "NFC:CallDefaultFoundHandler":
          this.callDefaultFoundHandler(message.data);
          return null;
        case "NFC:CallDefaultLostHandler":
          this.callDefaultLostHandler(message.data);
          return null;
        case "NFC:SendFile":
          // Chrome process is the arbitrator / mediator between
          // system app (content process) that issued nfc 'sendFile' operation
          // and system app that handles the system message :
          // 'nfc-manager-send-file'. System app subsequently handover's
          // the data to alternate carrier's (BT / WiFi) 'sendFile' interface.

          // Notify system app to initiate BT send file operation
          let sysMsg = new NfcSendFileSysMsg(message.data.requestId,
                                             message.data.sessionToken,
                                             message.data.blob);
          gSystemMessenger.broadcastMessage("nfc-manager-send-file",
                                            sysMsg);
          return null;
        default:
          return this.nfc.receiveMessage(message);
      }
    },

    /**
     * nsIObserver interface methods.
     */

    observe: function observe(subject, topic, data) {
      switch (topic) {
        case NFC.TOPIC_MOZSETTINGS_CHANGED:
          if ("wrappedJSObject" in subject) {
            subject = subject.wrappedJSObject;
          }
          if (subject) {
            this.nfc.handle(subject.key, subject.value);
          }
          break;
        case NFC.TOPIC_XPCOM_SHUTDOWN:
          this._shutdown();
          break;
      }
    },
  };
});

var SessionHelper = {
  tokenMap: {},

  registerSession: function registerSession(id, isP2P) {
    if (this.tokenMap[id]) {
      return this.tokenMap[id].token;
    }

    this.tokenMap[id] = {
      token: UUIDGenerator.generateUUID().toString(),
      isP2P: isP2P
    };

    return this.tokenMap[id].token;
  },

  unregisterSession: function unregisterSession(id) {
    if (this.tokenMap[id]) {
      delete this.tokenMap[id];
    }
  },

  getToken: function getToken(id) {
    return this.tokenMap[id] ? this.tokenMap[id].token : null;
  },

  getCurrentP2PToken: function getCurrentP2PToken() {
    for (let id in this.tokenMap) {
      if (this.tokenMap[id] && this.tokenMap[id].isP2P) {
        return this.tokenMap[id].token;
      }
    }
    return null;
  },

  getId: function getId(token) {
    for (let id in this.tokenMap) {
      if (this.tokenMap[id].token == token) {
        return id;
      }
    }

    return 0;
  },

  isP2PSession: function isP2PSession(id) {
    return (this.tokenMap[id] != null) && this.tokenMap[id].isP2P;
  }
};

function Nfc(isXPCShell) {
  // TODO: Bug 1239954: xpcshell test timed out with
  // SettingsSevice.createlock().get()
  // gSettingsService.createLock will cause timeout while running xpshell-test,
  // so we try to prevent to run gSettingsService under xpcshell-test here.
  gMessageManager.init(this);
  if (!isXPCShell && !NFC.DEBUG_NFC) {
    gMessageManager.listenDebugEvent();
  }

  this.targetsByRequestId = {};
  this.outputPathByRequestId = {};
}

Nfc.prototype = {

  classID:   NFC_CID,
  classInfo: XPCOMUtils.generateCI({classID: NFC_CID,
                                    classDescription: "Nfc",
                                    interfaces: [Ci.nsINfcService,
                                    Ci.nsISecureElementService]}),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver, Ci.nsINfcGonkEventListener,
                                         Ci.nsISecureElementService]),

  rfState: NFC.NFC_RF_STATE_IDLE,

  nfcService: null,

  targetsByRequestId: null,

  outputPathByRequestId: null,

  // temporary variables while NFC initialization is pending
  pendingNfcService: null,
  pendingMessageQueue: [],

  // Temporary variable to store RF change request while doing loader service.
  isLoaderService: 0,
  pendingRequestMessage: null,

  _domRequest: [],

  _mPOSReaderModeOn: false,

  /**
   * Start NFC service
   */
  startNfcService: function startNfcService() {
    debug("Starting Nfc Service");

    let nfcService =
      Cc["@mozilla.org/nfc/service;1"].getService(Ci.nsINfcService);
    if (!nfcService) {
      debug("No nfc service component available!");
      return false;
    }

    nfcService.start(this);
    this.pendingNfcService = nfcService;

    return true;
  },

  /**
   * Shutdown NFC service
   */
  shutdownNfcService : function shutdownNfcService() {
    debug("Shutting down Nfc Service");

    if (this.isLoaderService) {
      this.isLoaderService = 0;
      if (this.pendingRequestMessage) {
        this.sendNfcErrorResponse(this.pendingRequestMessage, "NotInitialize");
        this.pendingRequestMessage = null;
      }
    }

    this.nfcService.shutdown();
    this.nfcService = null;
    this.pendingNfcService = null;
  },

  /**
   *  Handle child process shutdown.
   */
  onChildProcessShutdown: function onChildProcessShutdown(target) {
    if (this._domRequest.length <= 0) return;

    let handler = [];
    handler["NFC:SetConfig"] = (name) => {
      if(!Services.prefs.getBoolPref("dom.nfc.revertTransit.enabled")) {
        debug("Revert to transit is disabled.");
        return;
      }

      let command = CommandMsgTable[name];
      if (!command) {
        debug("Unknown message");
        return;
      }

      let confFile = new Blob([''], {type: "text/plain"});
      this.sendToNfcService(command, {
        requestId: this._getRandomId(),
        rfConfType: "revertToTransit",
        confBlob: confFile
      });
    };

    handler["NFC:MPOSReaderMode"] = (name) => {
      if (!this._mPOSReaderModeOn) {
        debug("Reader mode is not enabled");
        return
      }

      let command = CommandMsgTable[name];
      if (!command) {
        debug("Unknown message");
        return;
      }

      this.sendToNfcService(command, {
        requestId: this._getRandomId(),
        mPOSReaderMode: false
      });
    };

    for (let i = this._domRequest.length - 1; i >= 0; i--) {
      if (this._domRequest[i].msg.manager !== target) continue;

      let msgName = this._domRequest[i].name;
      let func = handler[msgName];
      if (func) {
        func(msgName);
      }

      this._domRequest.splice(i, 1);
    }
  },
  /**
   * Send arbitrary message to Nfc service.
   *
   * @param nfcMessageType
   *        A text message type.
   * @param message [optional]
   *        An optional message object to send.
   */
  sendToNfcService: function sendToNfcService(nfcMessageType, message) {
    message = message || {};
    message.type = nfcMessageType;
    this.nfcService.sendCommand(message);
  },

  sendFileStatusResponse: function sendFileStatusResponse(message) {
    let target = this.getTargetByRequestId(message.requestId);
    if (!target) {
      return;
    }

    target.sendAsyncMessage("NFC:NotifySendFileStatusResponse", message);
  },

  sendNfcResponse: function sendNfcResponse(message) {
    let target = this.getTargetByRequestId(message.requestId);
    if (!target) {
      return;
    }

    target.sendAsyncMessage(EventMsgTable[message.type], message);
  },

  getTargetByRequestId: function getTargetByRequestId(requestId) {
    let target = this.targetsByRequestId[requestId];
    if (!target) {
      debug("No target for requestId: " + requestId);
      return null;
    }
    delete this.targetsByRequestId[requestId];

    return target;
  },

  getPathByRequestId: function getPathByRequestId(requestId) {
    let path = this.outputPathByRequestId[requestId];
    if (!path) {
      debug("No path for requestId: " + requestId);
      return null;
    }
    delete this.outputPathByRequestId[requestId];

    return path;
  },

  /**
   * Send Error response to content process.
   *
   * @param message
   *        An nsIMessageListener's message parameter.
   * @param errorMsg
   *        A string with an error message.
   */
  sendNfcErrorResponse: function sendNfcErrorResponse(message, errorMsg) {
    if (!message.target) {
      return;
    }

    let nfcMsgType = message.name + "Response";
    message.data.errorMsg = errorMsg;
    message.target.sendAsyncMessage(nfcMsgType, message.data);
  },

  getErrorMessage: function getErrorMessage(errorCode) {
    return NFC.NFC_ERROR_MSG[errorCode];
  },

  reStartTimer: null,

  createRestartTimer: function createRestartTimer(rfState, timeout) {
    // Clean up previous timer.
    if (this.reStartTimer) {
      this.reStartTimer.cancel();
      this.reStartTimer = null;
    }

    this.reStartTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    this.reStartTimer.initWithCallback((function(timer) {
      let message = {
        name: "NFC:ChangeRFState",
        data: {
          requestId: this._getRandomId(),
          rfState: rfState
        }
      };
      this.receiveMessage(message);
      }).bind(this), timeout, Ci.nsITimer.TYPE_ONE_SHOT);
  },

  /**
   * Process the incoming message from the NFC Service.
   */
  onEvent: function onEvent(event) {
    let message = Cu.cloneInto(event, this);
    DEBUG && debug("Received message from NFC Service: " + JSON.stringify(message));

    message.type = message.rspType || message.ntfType;
    switch (message.type) {
      case NfcNotificationType.INITIALIZED:
        this.nfcService = this.pendingNfcService;
        // Send messages that have been queued up during initialization
        // TODO: Bug 1141007: send error responses if the message
        // indicates an error during initialization.
        while (this.pendingMessageQueue.length) {
          this.receiveMessage(this.pendingMessageQueue.shift());
        }
        this.pendingNfcService = null;
        break;
      case NfcNotificationType.UNINITIALIZED:
        Services.obs.notifyObservers(event, TOPIC_NFCD_UNINITIALIZED, null);
        this.shutdownNfcService();

        if (this.rfState == NFC.NFC_RF_STATE_IDLE) {
          return;
        }

        this.createRestartTimer(NFC.NFC_RF_STATE_DISCOVERY, 1000);
        break;
      case NfcNotificationType.ENABLETIMEOUT:
        // Notify application the error.
        message.type = NfcResponseType.CHANGE_RF_STATE_RSP;
        message.rfState = NFC.NFC_RF_STATE_IDLE;
        message.errorMsg = "ErrorConnect";
        this.sendNfcResponse(message);

        gMessageManager.onRFStateChanged(NFC.NFC_RF_STATE_IDLE);

        Services.obs.notifyObservers(event, TOPIC_NFCD_UNINITIALIZED, null);

        this.shutdownNfcService();
        break;
      case NfcNotificationType.DISABLETIMEOUT:
        // Notify application the error.
        message.type = NfcResponseType.CHANGE_RF_STATE_RSP;
        message.rfState = NFC.NFC_RF_STATE_IDLE;
        message.errorMsg = "ErrorConnect";
        this.sendNfcResponse(message);

        gMessageManager.onRFStateChanged(NFC.NFC_RF_STATE_IDLE);

        Services.obs.notifyObservers(null, TOPIC_CHANGE_RF_STATE, NFC.NFC_RF_STATE_IDLE);

        this.shutdownNfcService();
        break;
      case NfcNotificationType.TECH_DISCOVERED:
        // Update the upper layers with a session token (alias)
        message.sessionToken =
          SessionHelper.registerSession(message.sessionId, message.isP2P);
        // Do not expose the actual session to the content
        let sessionId = message.sessionId;
        delete message.sessionId;

        if (SessionHelper.isP2PSession(sessionId)) {
          gMessageManager.onPeerEvent(NFC.PEER_EVENT_FOUND, message.sessionToken);
        } else {
          gMessageManager.onTagFound(message);
        }
        break;
      case NfcNotificationType.TECH_LOST:
        // Update the upper layers with a session token (alias)
        message.sessionToken = SessionHelper.getToken(message.sessionId);
        if (SessionHelper.isP2PSession(message.sessionId)) {
          gMessageManager.onPeerEvent(NFC.PEER_EVENT_LOST, message.sessionToken);
        } else {
          gMessageManager.onTagLost(message.sessionToken);
        }

        SessionHelper.unregisterSession(message.sessionId);
        break;
      case NfcNotificationType.HCI_EVENT_TRANSACTION:
        this.notifyHCIEventTransaction(message);
        break;
      case NfcNotificationType.MPOS_READER_MODE_EVENT:
        gMessageManager.onMPOSReaderModeEvent(message);
        break;
      case NfcNotificationType.RF_FIELD_ACTIVATED_EVENT:
        gMessageManager.onRfFieldEvent(true);
        break;
      case NfcNotificationType.RF_FIELD_DEACTIVATED_EVENT:
        gMessageManager.onRfFieldEvent(false);
        break;
      case NfcNotificationType.LISTEN_MODE_ACTIVATED_EVENT:
        gMessageManager.onListenModeEvent(true);
        break;
      case NfcNotificationType.LISTEN_MODE_DEACTIVATED_EVENT:
        gMessageManager.onListenModeEvent(false);
        break;
      case NfcNotificationType.NDEF_RECEIVED:
        message.sessionToken = SessionHelper.getToken(message.sessionId);
        delete message.sessionId;
        message.isP2P = true;
        // TODO: Bug 1082493.
        // This event should be sent to the focus app, but before Bug 1082493
        // is landed we forward this to System app.
        gMessageManager.callDefaultFoundHandler(message);
        break;
      case NfcResponseType.CHANGE_RF_STATE_RSP:
        this.sendNfcResponse(message);
        // Report the event even if there is a error.　　
        this.rfState = message.rfState;
        gMessageManager.onRFStateChanged(this.rfState);
        Services.obs.notifyObservers(null, TOPIC_CHANGE_RF_STATE, this.rfState);
        if (this.rfState == NFC.NFC_RF_STATE_IDLE) {
          this.shutdownNfcService();
        }
        break;
      case NfcResponseType.READ_NDEF_RSP: // Fall through.
      case NfcResponseType.WRITE_NDEF_RSP:
      case NfcResponseType.MAKE_READ_ONLY_RSP:
      case NfcResponseType.FORMAT_RSP:
      case NfcResponseType.TRANSCEIVE_RSP:
        this.sendNfcResponse(message);
        break;
      case NfcResponseType.OPENCONNECTION_RSP:
        this.openConnectionResponse(message);
        break;
      case NfcResponseType.TRANSMIT_RSP:
        this.transmitResponse(message);
        break;
      case NfcResponseType.CLOSECONNECTION_RSP:
        this.closeConnectionResponse(message);
        break;
      case NfcResponseType.RESETSECUREELEMENT_RSP:
        this.resetSecureElementResponse(message);
        break;
      case NfcResponseType.GETATR_RSP:
        this.getAtrResponse(message);
        break;
      case NfcResponseType.LSEXECUTESCRIPT_RSP:
        this.lsExecuteScriptResponse(message);
        break;
      case NfcResponseType.LSGETVERSION_RSP:
        this.lsGetVersionResponse(message);
        break;
      case NfcResponseType.MPOSREADERMODE_RSP:
        this.mPOSReaderModeResponse(message);
        break;
      case NfcResponseType.NFCSELFTEST_RSP:
        this.nfcSelfTestResponse(message);
        break;
      case NfcResponseType.SETCONFIG_RSP:
        if ((message.setConfigResult > NFC.NFC_SETCONFIG_FAILED) ||
            (message.setConfigResult < NFC.NFC_SETCONFIG_SUCCESS)) {
          message.setConfigResult = NFC.NFC_SETCONFIG_FAILED;
        }

        this.setConfigResponse(message);

        // Restart nfcd if it is blocked while restart nfc HAL layer.
        if ((message.setConfigResult == NFC.NFC_SETCONFIG_BUSY) &&
              (message.errorMsg == 'Timeout')) {
          Services.obs.notifyObservers(event, TOPIC_NFCD_UNINITIALIZED, null);
          // Restart NFC service.
          this.shutdownNfcService();
          this.createRestartTimer(NFC.NFC_RF_STATE_DISCOVERY, 1000);
        }
        break;
      case NfcResponseType.TAGREADERMODE_RSP:
        this.tagReaderModeResponse(message);
        break;
      default:
        throw new Error("Don't know about this message type: " + message.type);
    }
  },

  // HCI Event Transaction
  notifyHCIEventTransaction: function notifyHCIEventTransaction(message) {
    delete message.type;
    /**
     * FIXME:
     * GSMA 6.0 7.4 UI Application triggering requirements
     * This specifies the need for the following parameters to be derived and
     * sent. One unclear spec is what the URI format "secure:0" refers to, given
     * SEName can be something like "SIM1" or "SIM2".
     *
     * 1) Mime-type - Secure Element application dependent
     * 2) URI,  of the format:  nfc://secure:0/<SEName>/<AID>
     *     - SEName reflects the originating SE. It must be compliant with
     *       SIMAlliance Open Mobile APIs
     *     - AID reflects the originating UICC applet identifier
     * 3) Data - Data payload of the transaction notification, if any.
     */
    gSystemMessenger.broadcastMessage("nfc-hci-event-transaction", message);
  },

  /**
   * Process a message from the gMessageManager.
   */
  receiveMessage: function receiveMessage(message) {
    // Return early if we don't need the NFC Service. We won't start
    // the NFC daemon here.
    DEBUG && debug("Received message from content process: " + JSON.stringify(message));

    switch (message.name) {
      case "NFC:QueryInfo":
        return {rfState: this.rfState};
      default:
        break;
    }

    // Start NFC Service if necessary. Messages are held in a
    // queue while initialization is being performed.
    if (!this.nfcService) {
      if ((message.name == "NFC:ChangeRFState") &&
          (message.data.rfState == NFC.NFC_RF_STATE_DISCOVERY) &&
          !this.pendingNfcService) {
        this.startNfcService(); // error handled in next branch
      }
      if (this.pendingNfcService) {
        this.pendingMessageQueue.push(message);
      } else {
        this.sendNfcErrorResponse(message, "NotInitialize");
      }
      return;
    }

    let msg = message.data || {};
    msg.manager = message.target;

    // We are interested in SetConfig and MPOSReaderMode DOMRequests.
    if (message.name == "NFC:SetConfig" || message.name == "NFC:MPOSReaderMode") {
      this._domRequest.push({name: message.name, msg:msg});

      if (message.name == "NFC:MPOSReaderMode") {
        this._mPOSReaderModeOn = msg.mPOSReaderMode;
      }
    }

    // NFC Service is running and we have a message for it. This
    // is the case during normal operation.
    if (message.name != "NFC:ChangeRFState") {
      // Update the current sessionId before sending to the NFC service.
      message.data.sessionId = SessionHelper.getId(message.data.sessionToken);
    } else {
      if (this.isLoaderService) {
        this.pendingRequestMessage = message;
        let fakeMessage = {
          type: NfcResponseType.CHANGE_RF_STATE_RSP,
          rfState: message.rfState,
          requestId: message.data.requestId,
          rspType: "changeRFStateRsp"
        };
        this.targetsByRequestId[message.data.requestId] = message.target;

        this.sendNfcResponse(fakeMessage);
        gMessageManager.onRFStateChanged(fakeMessage.rfState);
        return;
      }
    }

    let command = CommandMsgTable[message.name];
    if (!command) {
      debug("Unknown message: " + message.name);
      return null;
    }
    this.targetsByRequestId[message.data.requestId] = message.target;
    this.sendToNfcService(command, message.data);
    return null;
  },

  removeTarget: function removeTarget(target) {
    Object.keys(this.targetsByRequestId).forEach((requestId) => {
      if (this.targetsByRequestId[requestId] === target) {
        delete this.targetsByRequestId[requestId];
      }
    });
  },

  /**
   * nsISettingsServiceCallback
   */
  handle: function handle(name, result) {
    switch (name) {
      case NFC.SETTING_NFC_DEBUG:
        DEBUG = result;
        updateDebug();
        break;
    }
  },

  /**
   * nsIObserver interface methods.
   */
  observe: function(subject, topic, data) {
    if (topic != "profile-after-change") {
      debug("Should receive 'profile-after-change' only, received " + topic);
    }
  },

  shutdown: function shutdown() {
    if (this.isLoaderService) {
      this.isLoaderService = 0;
      if (this.pendingRequestMessage) {
        this.sendNfcErrorResponse(this.pendingRequestMessage, "NotInitialize");
        this.pendingRequestMessage = null;
      }
    }
    // We shutdown before initialization has been completed. The
    // pending messages will receive an error response.
    while (this.pendingMessageQueue.length) {
      this.sendNfcErrorResponse(this.pendingMessageQueue.shift(), "NotInitialize");
    }
    if (this.nfcService) {
      this.shutdownNfcService();
    }
  },

  /**
   * nsISecureElementService interface methods.
   */

  _getRandomId: function() {
    return Cc["@mozilla.org/uuid-generator;1"]
             .getService(Ci.nsIUUIDGenerator).generateUUID().toString();
  },

  _hexStringToUint8Array: function(hexStr) {
    if (typeof hexStr !== "string" || hexStr.length % 2 !== 0) {
      return [];
    }

    let array = new Uint8Array(hexStr.length/2);
    let index = 0;
    let len = hexStr.length;
    for (let i = 0; i < len; i += 2) {
      array[index++] = parseInt(hexStr.substr(i, 2), 16);
    }

    return array;
  },

  _uint8ArrayToHexString: function(array) {
    let hexStr = "";

    let len = array ? array.length : 0;
    for (let i = 0; i < len; i++) {
      let hex = (array[i] & 0xff).toString(16);
      hex = (hex.length === 1) ? "0" + hex : hex;
      hexStr += hex;
    }

    return hexStr.toUpperCase();
  },



  openConnection: function(callback) {
    if (!this.nfcService) {
      callback.notifyError("NotInitialize");
      return;
    }

    let command = CommandMsgTable["NFC:OpenConnection"];
    if (!command) {
      debug("Unknown message");
      return;
    }

    let message = {
      requestId: this._getRandomId(),
    };

    this.targetsByRequestId[message.requestId] = callback;

    debug("OpenConnection: send command to nfcd " + JSON.stringify(message));
    this.sendToNfcService(command, message);
  },

  openConnectionResponse: function(message) {
    let callback = this.getTargetByRequestId(message.requestId);
    if (!callback) {
      debug("openConnectionResponse: can't find callback");
      return;
    }

    if (message.errorMsg) {
      debug("openConnectionResponse: receive error from nfcd:" + message.errorMsg);
      callback.notifyError(message.errorMsg);
      return;
    }

    debug("OpenConnectionResponse: receive from nfcd " + JSON.stringify(message));
    callback.notifyOpenConnectionSuccess(message.handle);
  },

  transmit: function(handle, data, callback) {
    if (!this.nfcService) {
      callback.notifyError("NotInitialize");
      return;
    }

    let command = CommandMsgTable["NFC:Transmit"];
    if (!command) {
      debug("Unknown message");
      return;
    }

    let buffer = this._hexStringToUint8Array(data);
    let message = {
      requestId: this._getRandomId(),
      handle: handle,
      apduCommand: buffer
    };

    this.targetsByRequestId[message.requestId] = callback;
    debug("Transmit: send command to nfcd " + JSON.stringify(message));
    this.sendToNfcService(command, message);
  },

  transmitResponse: function(message) {
    let callback = this.getTargetByRequestId(message.requestId);
    if (!callback) {
      debug("TransmitResponse: can't find callback");
      return;
    }

    if (message.errorMsg) {
      debug("TransmitResponse: receive error from nfcd " + message.errorMsg);
      callback.notifyError(message.errorMsg);
      return;
    }

    if (!message.apduResponse) {
      debug("TransmitResponse: no valid response from eSE");
      callback.notifyError("No valid response from eSE");
      return;
    }
    debug("TransmitResponse:" + JSON.stringify(message.apduResponse));
    callback.notifyTransmitResponse(message.apduResponse);
  },

  closeConnection: function(handle, callback) {
    if (!this.nfcService) {
      callback.notifyError("NotInitialize");
      return;
    }

    let command = CommandMsgTable["NFC:CloseConnection"];
    if (!command) {
      debug("Unknown message");
      return null;
    }

    let message = {
      requestId: this._getRandomId(),
      handle: handle
    };

    this.targetsByRequestId[message.requestId] = callback;

    debug("closeConnection: send message to nfcd " + JSON.stringify(message));
    this.sendToNfcService(command, message);
  },

  closeConnectionResponse: function(message) {
    let callback = this.getTargetByRequestId(message.requestId);
    if (!callback) {
      debug("closeConnectionResponse: can't find callback");
      return;
    }

    if (message.errorMsg) {
      debug("closeConnectionResponse: receive error from nfcd " + message.errorMsg);
      callback.notifyError(message.errorMsg);
      return;
    }

    debug("closeConnectionResponse: receive message from nfcd " + JSON.stringify(message));
    callback.notifyCloseConnectionSuccess();
  },

  resetSecureElement: function(handle, callback) {
    if (!this.nfcService) {
      callback.notifyError("NotInitialize");
      return;
    }

    let command = CommandMsgTable["NFC:ResetSecureElement"];
    if (!command) {
      debug("Unknown message");
      return null;
    }

    let message = {
      requestId: this._getRandomId(),
      handle: handle
    };

    this.targetsByRequestId[message.requestId] = callback;

    debug("resetSecureElement: send message to nfcd " + JSON.stringify(message));
    this.sendToNfcService(command, message);
  },

  resetSecureElementResponse: function(message)
  {
    let callback = this.getTargetByRequestId(message.requestId);
    if (!callback) {
      debug("resetSecureElementResponse: can't find callback");
      return;
    }

    if (message.errorMsg) {
      debug("resetSecureElementResponse: receive error from nfcd " + message.errorMsg);
      callback.notifyError(message.errorMsg);
      return;
    }

    debug("resetSecureElementResponse: receive message from nfcd " + JSON.stringify(message));
    callback.notifyResetSecureElementSuccess();
  },

  getAtr: function(handle, callback) {
    if (!this.nfcService) {
      callback.notifyError("NotInitialize");
      return;
    }

    let command = CommandMsgTable["NFC:GetAtr"];
    if (!command) {
      debug("Unknown message");
      return null;
    }

    let message = {
      requestId: this._getRandomId(),
      handle: handle
    };

    this.targetsByRequestId[message.requestId] = callback;

    debug("getAtr: send message to nfcd " + JSON.stringify(message));
    this.sendToNfcService(command, message);

  },

  getAtrResponse: function(message)
  {
    let callback = this.getTargetByRequestId(message.requestId);
    if (!callback) {
      debug("getAtrResponse: can't find callback");
      return;
    }

    if (message.errorMsg) {
      debug("resetSecureElementResponse: receive error from nfcd " + message.errorMsg);
      callback.notifyError(message.errorMsg);
      return;
    }

    if (!message.response) {
      debug("resetSecureElementResponse: no valid response from eSE");
      callback.notifyError("No valid response from eSE");
      return;
    }

    debug("getAtrResponse: receive message from nfcd " + JSON.stringify(message));
    callback.notifyGetAtrSuccess(message.response);
  },

  _copyFile: function(sourcePath, destPath, fileNewName, callback) {
    debug('copyFile: ' + sourcePath + ' destPath: ' + destPath + ' fileName:' + fileNewName);

    let sourcefile = Cc['@mozilla.org/file/local;1'].createInstance(Ci.nsILocalFile);
    sourcefile.initWithPath(sourcePath);

    let destfile = Cc['@mozilla.org/file/local;1'].createInstance(Ci.nsILocalFile);
    destfile.initWithPath(destPath);

    let dest = Cc['@mozilla.org/file/local;1'].createInstance(Ci.nsILocalFile);
    dest.initWithPath(destPath + fileNewName);

    if (dest.exists()) {
      try {
        dest.remove(false);
      } catch (e) {
        debug("Unable to remove the file");
      }
    }

    let cbData = {};
    cbData['processState'] = 'process-finished';
    try {
      sourcefile.copyTo(destfile, fileNewName);
    }
    catch (e) {
      debug("_copyFile: error: " + e.result);
      cbData['processState'] = 'process-failed';
      if(e.result == Cr.NS_ERROR_FILE_TARGET_DOES_NOT_EXIST) {
        cbData['errorType'] = 'SourceFileNotExist';
      } else {
        cbData['errorType'] = 'DefaultError';
      }
    }
    finally {
      callback(cbData);
    }
  },

  lsExecuteScript: function(msg, callback) {
    if (!this.nfcService) {
      callback.notifyError("NotInitialize");
      return;
    }

    let command = CommandMsgTable["NFC:LsExecuteScript"];
    if (!command) {
      debug("Unknown message");
      return null;
    }
    let self = this;

    let fullScriptFileName = msg.appletScriptFile;
    let regx = /^\/$|(^(?=\/)|^\.|^\.\.)(\/(?=[^/\0])[^/\0]+)*\/?$/;
    let valid = regx.test(fullScriptFileName);
    if (!valid) {
      callback.notifyError("Invalid script file name");
      return null;
    }

    let fullResponseFileName = msg.responseFile;
    valid = regx.test(fullResponseFileName);
    if (!valid) {
      callback.notifyError("Invalid script file name");
      return null;
    }

    let lastSlashIndex = fullScriptFileName.lastIndexOf('/');
    let scriptFileName = fullScriptFileName.substr(lastSlashIndex + 1);

    lastSlashIndex = fullResponseFileName.lastIndexOf('/');
    let responseFileName = fullResponseFileName.substr(lastSlashIndex + 1);

    let dstDir = LOADER_SERVICE_TMP;

    // Create the response file in LOADER_SERVICE_TMP
    let destFile = Cc['@mozilla.org/file/local;1'].createInstance(Ci.nsILocalFile);
    destFile.initWithPath(dstDir + responseFileName);
    if (!destFile.exists()) {
      try {
        destFile.create(Ci.nsIFile.NORMAL_FILE_TYPE, FileUtils.PERMS_DIRECTORY);
      } catch (e) {
        debug("Failed to create the file");
      }
    }

    // Copy the applet file
    this._copyFile(fullScriptFileName, dstDir, scriptFileName, function(data) {
      let manifestURL = DOMApplicationRegistry.getManifestURLByLocalId(msg.uniqueAppID);
      // Remove app:// from url.
      var removeProtocol = manifestURL.substr(6);
      // Get the first index of '/'.
      var end = removeProtocol.indexOf('/');
      var manifest = removeProtocol.substr(0, end);

      let message = {
        requestId: self._getRandomId(),
        lsScriptFile: dstDir + scriptFileName,
        lsResponseFile: dstDir + responseFileName,
        uniqueApplicationID: manifest
      };
      debug("lsExecuteScript: send message to nfcd " + JSON.stringify(message));

      self.targetsByRequestId[message.requestId] = callback;

      self.outputPathByRequestId[message.requestId] = {
        scriptFileName: scriptFileName,
        fullScriptFileName: fullScriptFileName,
        responseFileName: responseFileName,
        fullResponseFileName: fullResponseFileName
      };

      self.isLoaderService++;
      self.sendToNfcService(command, message);
    });
  },

  lsExecuteScriptResponse: function(message)
  {
    if (this.isLoaderService) {
      this.isLoaderService--;
      if (this.pendingRequestMessage) {
        this.receiveMessage(this.pendingRequestMessage);
        this.pendingRequestMessage = null;
      }
    }

    let callback = this.getTargetByRequestId(message.requestId);
    if (!callback) {
      debug("lsExecuteScriptResponse: can't find callback");
      return;
    }

    let path = this.getPathByRequestId(message.requestId);

    var index = path.fullScriptFileName.lastIndexOf('/');
    var dstDir = path.fullScriptFileName.substr(0, index + 1);

    let srcFile = LOADER_SERVICE_TMP + path.responseFileName;

    debug("lsExecuteScriptResponse: receive message from nfcd srcFile " + srcFile + " " + " dstFile " + dstDir + path.responseFileName);

    let self = this;
    this._copyFile(srcFile, dstDir, path.responseFileName, function(data) {
      // Remove the temporary files.
      let script = Cc['@mozilla.org/file/local;1'].createInstance(Ci.nsILocalFile);
      script.initWithPath((LOADER_SERVICE_TMP + path.scriptFileName));
      if (script.exists()) {
        try {
          script.remove(false);
        } catch (e) {
          debug("Unable to remove the file");
        }
      }
      let rsp = Cc['@mozilla.org/file/local;1'].createInstance(Ci.nsILocalFile);
      rsp.initWithPath((LOADER_SERVICE_TMP + path.responseFileName));
      if (rsp.exists()) {
        try {
          rsp.remove(false);
        } catch (e) {
          debug("Unable to remove the file");
        }
      }

      if (!message.response) {
        debug("lsExecuteScriptResponse: no valid response from eSE " + JSON.stringify(message));
        callback.notifyError("No valid response from eSE");
        return;
      }

      if (message.errorMsg) {
        debug("lsExecuteScriptResponse: receive error from nfcd " + message.errorMsg);
        callback.notifyError(self._uint8ArrayToHexString(message.response));
        return;
      }

      debug("lsExecuteScriptResponse: receive message from nfcd " + JSON.stringify(message));
      callback.notifyLsExecuteScriptSuccess(message.response);
    });
  },

  lsGetVersion: function(msg, callback) {
    if (!this.nfcService) {
      callback.notifyError("NotInitialize");
      return;
    }

    let command = CommandMsgTable["NFC:LsGetVersion"];
    if (!command) {
      debug("Unknown message");
      return null;
    }

    let message = {
      requestId: this._getRandomId(),
    };

    this.targetsByRequestId[message.requestId] = callback;

    debug("lsGetVersion: send message to nfcd " + JSON.stringify(message));
    this.sendToNfcService(command, message);

  },

  lsGetVersionResponse: function(message)
  {
    let callback = this.getTargetByRequestId(message.requestId);
    if (!callback) {
      debug("lsGetVersionResponse: can't find callback");
      return;
    }

    if (!message.response) {
      debug("lsGetVersionResponse: no valid response from eSE " + JSON.stringify(message));
      callback.notifyError("No valid response from eSE");
      return;
    }

    if (message.errorMsg) {
      debug("lsGetVersionResponse: receive error from nfcd " + message.errorMsg);
      callback.notifyError(this._uint8ArrayToHexString(message.response));
      return;
    }

    debug("lsGetVersionResponse: receive message from nfcd " + JSON.stringify(message));
    callback.notifyLsGetVersionSuccess(message.response);
  },

  mPOSReaderModeResponse: function(message)
  {
    debug("mPOSReaderModeResponse: send mpos reader mode response:" + JSON.stringify(message));
    if (message.errorMsg) {
      this._mPOSReaderModeOn = false;
    }

    this.sendNfcResponse(message);
  },

  nfcSelfTestResponse: function(message)
  {
    debug("nfcSelfTestResponse: send nfc self test response:" + JSON.stringify(message));
    this.sendNfcResponse(message);
  },

  setConfigResponse: function(message)
  {
    debug("setConfigResponse: send setconfig response:" + JSON.stringify(message));
    this.sendNfcResponse(message);
  },

  tagReaderModeResponse: function(message)
  {
    debug("tagReaderModeResponse: send tag reader mode response:" + JSON.stringify(message));
    this.sendNfcResponse(message);
  },

  registerListener: function(listener) {
    // TODO, wait for integration
    debug("register listener");
  },

  unregisterListener: function(listener) {
    // TODO, wait for integration.
    debug("unregister listener");
  },

  get focusId() {
    return gMessageManager.focusId;
  },

  get focusAppManifestUrl() {
    return gMessageManager.focusAppManifestUrl;
  }
};

function NfcTechDiscoveredSysMsg(sessionToken, isP2P, records) {
  this.sessionToken = sessionToken;
  this.isP2P = isP2P;
  this.records = records;
}
NfcTechDiscoveredSysMsg.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsINfcTechDiscoveredSysMsg]),

  sessionToken: null,
  isP2P: null,
  records: null
};

function NfcSendFileSysMsg(requestId, sessionToken, blob) {
  this.requestId = requestId;
  this.sessionToken = sessionToken;
  this.blob = blob;
}
NfcSendFileSysMsg.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsINfcSendFileSysMsg]),

  requestId: null,
  sessionToken: null,
  blob: null
};

if (NFC_ENABLED) {
  this.NSGetFactory = XPCOMUtils.generateNSGetFactory([Nfc]);
}
