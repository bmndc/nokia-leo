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

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/DOMRequestHelper.jsm");

XPCOMUtils.defineLazyGetter(this, "NFC", function () {
  let obj = {};
  Cu.import("resource://gre/modules/nfc_consts.js", obj);
  return obj;
});

Cu.import("resource://gre/modules/systemlibs.js");
const NFC_ENABLED = libcutils.property_get("ro.moz.nfc.enabled", "false") === "true";

// set to true to in nfc_consts.js to see debug messages
var DEBUG = NFC.DEBUG_CONTENT_HELPER;

var debug;
function updateDebug() {
  if (DEBUG || NFC.DEBUG_CONTENT_HELPER) {
    debug = function (s) {
      dump("-*- NfcContentHelper: " + s + "\n");
    };
  } else {
    debug = function (s) {};
  }
};
updateDebug();

const NFCCONTENTHELPER_CID =
  Components.ID("{4d72c120-da5f-11e1-9b23-0800200c9a66}");

const NFC_IPC_MSG_NAMES = [
  "NFC:ReadNDEFResponse",
  "NFC:WriteNDEFResponse",
  "NFC:MakeReadOnlyResponse",
  "NFC:FormatResponse",
  "NFC:TransceiveResponse",
  "NFC:CheckP2PRegistrationResponse",
  "NFC:DOMEvent",
  "NFC:NotifySendFileStatusResponse",
  "NFC:ChangeRFStateResponse",
  "NFC:MPOSReaderModeResponse",
  "NFC:NfcSelfTestResponse",
  "NFC:SetConfigResponse",
  "NFC:SetTagReaderModeResponse"
];

XPCOMUtils.defineLazyServiceGetter(this, "cpmm",
                                   "@mozilla.org/childprocessmessagemanager;1",
                                   "nsISyncMessageSender");

function NfcContentHelper() {
  Services.obs.addObserver(this, "xpcom-shutdown", false);

  this._requestMap = [];
  this.initDOMRequestHelper(/* window */ null, NFC_IPC_MSG_NAMES);
  this.eventListeners = {};
}

NfcContentHelper.prototype = {
  __proto__: DOMRequestIpcHelper.prototype,

  QueryInterface: XPCOMUtils.generateQI([Ci.nsINfcContentHelper,
                                         Ci.nsINfcBrowserAPI,
                                         Ci.nsISupportsWeakReference,
                                         Ci.nsIObserver]),
  classID:   NFCCONTENTHELPER_CID,
  classInfo: XPCOMUtils.generateCI({
    classID:          NFCCONTENTHELPER_CID,
    classDescription: "NfcContentHelper",
    interfaces:       [Ci.nsINfcContentHelper,
                       Ci.nsINfcBrowserAPI]
  }),

  _requestMap: null,
  _mPOSReaderModeOn: false,
  eventListeners: null,

  queryRFState: function queryRFState() {
    return cpmm.sendSyncMessage("NFC:QueryInfo")[0].rfState;
  },

  setFocusTab: function setFocusTab(tabId, focusAppManifestUrl, isFocus) {
    cpmm.sendAsyncMessage("NFC:SetFocusTab", {
      tabId: tabId,
      focusAppManifestUrl: focusAppManifestUrl,
      isFocus: isFocus
    });
  },

  // NFCTag interface
  readNDEF: function readNDEF(sessionToken, callback) {
    let requestId = callback.getCallbackId();
    this._requestMap[requestId] = callback;

    cpmm.sendAsyncMessage("NFC:ReadNDEF", {
      requestId: requestId,
      sessionToken: sessionToken
    });
  },

  writeNDEF: function writeNDEF(records, isP2P, sessionToken, callback) {
    let requestId = callback.getCallbackId();
    this._requestMap[requestId] = callback;

    cpmm.sendAsyncMessage("NFC:WriteNDEF", {
      requestId: requestId,
      sessionToken: sessionToken,
      records: records,
      isP2P: isP2P
    });
  },

  makeReadOnly: function makeReadOnly(sessionToken, callback) {
    let requestId = callback.getCallbackId();
    this._requestMap[requestId] = callback;

    cpmm.sendAsyncMessage("NFC:MakeReadOnly", {
      requestId: requestId,
      sessionToken: sessionToken
    });
  },

  format: function format(sessionToken, callback) {
    let requestId = callback.getCallbackId();
    this._requestMap[requestId] = callback;

    cpmm.sendAsyncMessage("NFC:Format", {
      requestId: requestId,
      sessionToken: sessionToken
    });
  },

  transceive: function transceive(sessionToken, technology, command, callback) {
    let requestId = callback.getCallbackId();
    this._requestMap[requestId] = callback;

    cpmm.sendAsyncMessage("NFC:Transceive", {
      requestId: requestId,
      sessionToken: sessionToken,
      technology: technology,
      command: command
    });
  },

  sendFile: function sendFile(data, sessionToken, callback) {
    let requestId = callback.getCallbackId();
    this._requestMap[requestId] = callback;

    cpmm.sendAsyncMessage("NFC:SendFile", {
      requestId: requestId,
      sessionToken: sessionToken,
      blob: data.blob
    });
  },

  notifySendFileStatus: function notifySendFileStatus(status, requestId) {
    cpmm.sendAsyncMessage("NFC:NotifySendFileStatus", {
      status: status,
      requestId: requestId
    });
  },

  addEventListener: function addEventListener(listener, tabId) {
    let _window = listener.window;

    this.eventListeners[tabId] = listener;
    cpmm.sendAsyncMessage("NFC:AddEventListener", { tabId: tabId });
  },

  removeEventListener: function removeEventListener(tabId) {
    delete this.eventListeners[tabId];

    cpmm.sendAsyncMessage("NFC:RemoveEventListener", { tabId: tabId });
  },

  registerTargetForPeerReady: function registerTargetForPeerReady(appId) {
    cpmm.sendAsyncMessage("NFC:RegisterPeerReadyTarget", { appId: appId });
  },

  unregisterTargetForPeerReady: function unregisterTargetForPeerReady(appId) {
    cpmm.sendAsyncMessage("NFC:UnregisterPeerReadyTarget", { appId: appId });
  },

  checkP2PRegistration: function checkP2PRegistration(appId, callback) {
    let requestId = callback.getCallbackId();
    this._requestMap[requestId] = callback;

    cpmm.sendAsyncMessage("NFC:CheckP2PRegistration", {
      appId: appId,
      requestId: requestId
    });
  },

  notifyUserAcceptedP2P: function notifyUserAcceptedP2P(appId) {
    cpmm.sendAsyncMessage("NFC:NotifyUserAcceptedP2P", {
      appId: appId
    });
  },

  changeRFState: function changeRFState(rfState, callback) {
    let requestId = callback.getCallbackId();
    this._requestMap[requestId] = callback;

    cpmm.sendAsyncMessage("NFC:ChangeRFState",
                          {requestId: requestId,
                           rfState: rfState});
  },

  changeRFStateWithMode: function changeRFStateWithMode(rfState, powerMode, callback) {
    let requestId = callback.getCallbackId();
    this._requestMap[requestId] = callback;

    cpmm.sendAsyncMessage("NFC:ChangeRFState",
                          {requestId: requestId,
                           rfState: rfState,
                           powerMode: powerMode});
  },

  mPOSReaderMode: function mPOSReaderMode(enabled, callback) {
    let requestId = callback.getCallbackId();
    this._requestMap[requestId] = callback;
    this._mPOSReaderModeOn = enabled;
    cpmm.sendAsyncMessage("NFC:MPOSReaderMode",
                          {requestId: requestId,
                           mPOSReaderMode: enabled});
  },

  nfcSelfTest: function nfcSelfTest(type, callback) {
    let requestId = callback.getCallbackId();
    this._requestMap[requestId] = callback;
    cpmm.sendAsyncMessage("NFC:NfcSelfTest",
                          {requestId: requestId,
                           selfTestType: type});
  },

  setConfig: function setConfig(type, confFile, callback) {
    let requestId = callback.getCallbackId();
    this._requestMap[requestId] = callback;
    cpmm.sendAsyncMessage("NFC:SetConfig",
                          {requestId: requestId,
                           rfConfType: type,
                           confBlob: confFile});
  },

  get isMPOSReaderMode() {
    return this._mPOSReaderModeOn;
  },

  setTagReaderMode: function setTagReaderMode(enabled, callback) {
    let requestId = callback.getCallbackId();
    this._requestMap[requestId] = callback;
    cpmm.sendAsyncMessage("NFC:SetTagReaderMode",
                          {requestId: requestId,
                           tagReaderMode: enabled});
  },

  callDefaultFoundHandler: function callDefaultFoundHandler(sessionToken,
                                                            isP2P,
                                                            records) {
    cpmm.sendAsyncMessage("NFC:CallDefaultFoundHandler",
                          {sessionToken: sessionToken,
                           isP2P: isP2P,
                           records: records});
  },

  callDefaultLostHandler: function callDefaultLostHandler(sessionToken, isP2P) {
    cpmm.sendAsyncMessage("NFC:CallDefaultLostHandler",
                          {sessionToken: sessionToken,
                           isP2P: isP2P});
  },

  // nsIObserver
  observe: function observe(subject, topic, data) {
    if (topic == "xpcom-shutdown") {
      this.destroyDOMRequestHelper();
      Services.obs.removeObserver(this, "xpcom-shutdown");
      cpmm = null;
    }
  },

  // nsIMessageListener
  receiveMessage: function receiveMessage(message) {
    DEBUG && debug("Message received: " + JSON.stringify(message));
    let result = message.data;

    switch (message.name) {
      case "NFC:ReadNDEFResponse":
        this.handleReadNDEFResponse(result);
        break;
      case "NFC:CheckP2PRegistrationResponse":
        this.handleCheckP2PRegistrationResponse(result);
        break;
      case "NFC:TransceiveResponse":
        this.handleTransceiveResponse(result);
        break;
      case "NFC:WriteNDEFResponse": // Fall through.
      case "NFC:MakeReadOnlyResponse":
      case "NFC:FormatResponse":
      case "NFC:NotifySendFileStatusResponse":
      case "NFC:ChangeRFStateResponse":
        this.handleGeneralResponse(result);
        break
      case "NFC:MPOSReaderModeResponse":
        this.handleMPOSReaderModeResponse(result);
        break;
      case "NFC:NfcSelfTestResponse":
        this.handleNfcSelfTestResponse(result);
        break;
      case "NFC:SetConfigResponse":
        this.handleSetConfigResponse(result);
        break;
      case "NFC:SetTagReaderModeResponse":
        this.handleTagReaderModeResponse(result);
        break;
      case "NFC:DOMEvent":
        this.handleDOMEvent(result);
        break;
    }
  },

  handleTagReaderModeResponse: function handleTagReaderModeResponse(result) {
    this.handleGeneralResponse(result);
  },

  handleMPOSReaderModeResponse: function handleMPOSReaderModeResponse(result) {
    if (result.errorMsg) {
      this._mPOSReaderModeOn = false;
    }
    this.handleGeneralResponse(result);
  },

  handleNfcSelfTestResponse: function handleNfcSelfTestResponse(result) {
    this.handleGeneralResponse(result);
  },

  handleSetConfigResponse: function handleSetConfigResponse(result) {
    let requestId = result.requestId;
    let callback = this._requestMap[requestId];
    if (!callback) {
      debug("not firing message " + result.type + " for id: " + requestId);
      return;
    }
    delete this._requestMap[requestId];

    // Return true if the result is invalid.
    function check(setConfigResult) {
      let inValid = false;

      if ((result.setConfigResult < NFC.NFC_SETCONFIG_SUCCESS) ||
          (result.setConfigResult > NFC.NFC_SETCONFIG_FAILED)) {
        inValid = true;
      }

      return inValid;
    }

    if (result.errorMsg || check(result.setConfigResult)) {
      callback.notifySuccessWithInt(NFC.NFC_SETCONFIG_FAILED);
      return;
    }

    callback.notifySuccessWithInt(result.setConfigResult);
  },

  handleGeneralResponse: function handleGeneralResponse(result) {
    let requestId = result.requestId;
    let callback = this._requestMap[requestId];
    if (!callback) {
      debug("not firing message " + result.type + " for id: " + requestId);
      return;
    }
    delete this._requestMap[requestId];

    if (result.errorMsg) {
      callback.notifyError(result.errorMsg);
    } else {
      callback.notifySuccess();
    }
  },

  handleReadNDEFResponse: function handleReadNDEFResponse(result) {
    let requestId = result.requestId;
    let callback = this._requestMap[requestId];
    if (!callback) {
      debug("not firing message handleReadNDEFResponse for id: " + requestId);
      return;
    }
    delete this._requestMap[requestId];

    if (result.errorMsg) {
      callback.notifyError(result.errorMsg);
      return;
    }

    callback.notifySuccessWithNDEFRecords(result.records);
  },

  handleCheckP2PRegistrationResponse: function handleCheckP2PRegistrationResponse(result) {
    let requestId = result.requestId;
    let callback = this._requestMap[requestId];
    if (!callback) {
      debug("not firing message handleCheckP2PRegistrationResponse for id: " + requestId);
      return;
    }
    delete this._requestMap[requestId];

    // Privilaged status API. Always fire success to avoid using exposed props.
    // The receiver must check the boolean mapped status code to handle.
    callback.notifySuccessWithBoolean(!result.errorMsg);
  },

  handleTransceiveResponse: function handleTransceiveResponse(result) {
    let requestId = result.requestId;
    let callback = this._requestMap[requestId];
    if (!callback) {
      debug("not firing message handleTransceiveResponse for id: " + requestId);
      return;
    }
    delete this._requestMap[requestId];

    if (result.errorMsg) {
      callback.notifyError(result.errorMsg);
      return;
    }

    callback.notifySuccessWithByteArray(result.response);
  },

  handleDOMEvent: function handleDOMEvent(result) {
    let listener = this.eventListeners[result.tabId];
    if (!listener) {
      debug("no listener for tabId " + result.tabId);
      return;
    }

    switch (result.event) {
      case NFC.PEER_EVENT_READY:
        listener.notifyPeerFound(result.sessionToken, /* isPeerReady */ true);
        break;
      case NFC.PEER_EVENT_FOUND:
        listener.notifyPeerFound(result.sessionToken);
        break;
      case NFC.PEER_EVENT_LOST:
        listener.notifyPeerLost(result.sessionToken);
        break;
      case NFC.TAG_EVENT_FOUND:
        let ndefInfo = null;
        if (result.tagType !== undefined &&
            result.maxNDEFSize !== undefined &&
            result.isReadOnly !== undefined &&
            result.isFormatable !== undefined) {
          ndefInfo = new TagNDEFInfo(result.tagType,
                                     result.maxNDEFSize,
                                     result.isReadOnly,
                                     result.isFormatable);
        }

        let tagInfo = new TagInfo(result.techList, result.tagId);
        listener.notifyTagFound(result.sessionToken,
                                          tagInfo,
                                          ndefInfo,
                                          result.records);
        break;
      case NFC.TAG_EVENT_LOST:
        listener.notifyTagLost(result.sessionToken);
        break;
      case NFC.RF_EVENT_STATE_CHANGED:
        listener.notifyRFStateChanged(result.rfState);
        break;
      case NFC.FOCUS_CHANGED:
        listener.notifyFocusChanged(result.focus);
        break;
      case NFC.MPOS_READER_MODE_EVENT:
        listener.notifyMPOSReaderModeEvent(result.type);
        break;
      case NFC.RF_FIELD_ACTIVATE_EVENT:
        listener.notifyRfFieldEvent(true);
        break;
      case NFC.RF_FIELD_DEACTIVATE_EVENT:
        listener.notifyRfFieldEvent(false);
        break;
      case NFC.LISTEN_MODE_ACTIVATE_EVENT:
        listener.notifyListenModeEvent(true);
        break;
      case NFC.LISTEN_MODE_DEACTIVATE_EVENT:
        listener.notifyListenModeEvent(false);
        break;
    }
  }
};

function TagNDEFInfo(tagType, maxNDEFSize, isReadOnly, isFormatable) {
  this.tagType = tagType;
  this.maxNDEFSize = maxNDEFSize;
  this.isReadOnly = isReadOnly;
  this.isFormatable = isFormatable;
}
TagNDEFInfo.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsITagNDEFInfo]),

  tagType: null,
  maxNDEFSize: 0,
  isReadOnly: false,
  isFormatable: false
};

function TagInfo(techList, tagId) {
  this.techList = techList;
  this.tagId = tagId;
}
TagInfo.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsITagInfo]),

  techList: null,
  tagId: null,
};

if (NFC_ENABLED) {
  this.NSGetFactory = XPCOMUtils.generateNSGetFactory([NfcContentHelper]);
}
