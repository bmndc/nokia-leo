/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// TODO Bug 907060 Per off-line discussion, after the MessagePort is done
// at Bug 643325, we will start to refactorize the common logic of both
// Inter-App Communication and Shared Worker. For now, we hope to design an
// MozInterAppMessagePort to meet the timeline, which still follows exactly
// the same interface and semantic as the MessagePort is. In the future,
// we can then align it back to MessagePort with backward compatibility.

"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/DOMRequestHelper.jsm");

const DEBUG = false;
function debug(aMsg) {
  dump("-- InterAppMessagePort: " + Date.now() + ": " + aMsg + "\n");
}

XPCOMUtils.defineLazyServiceGetter(this, "cpmm",
                                   "@mozilla.org/childprocessmessagemanager;1",
                                   "nsIMessageSender");

XPCOMUtils.defineLazyServiceGetter(this, "appsService",
                                   "@mozilla.org/AppsService;1",
                                   "nsIAppsService");

const kMessages = ["InterAppMessagePort:OnClose",
                   "InterAppMessagePort:OnMessage",
                   "InterAppMessagePort:Shutdown"];

function InterAppMessagePort() {
  if (DEBUG) debug("InterAppMessagePort()");
};

InterAppMessagePort.prototype = {
  __proto__: DOMRequestIpcHelper.prototype,

  classDescription: "MozInterAppMessagePort",

  classID: Components.ID("{c66e0f8c-e3cb-11e2-9e85-43ef6244b884}"),

  contractID: "@mozilla.org/dom/inter-app-message-port;1",

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDOMGlobalPropertyInitializer,
                                         Ci.nsISupportsWeakReference,
                                         Ci.nsIObserver]),

  // Ci.nsIDOMGlobalPropertyInitializer implementation.
  init: function(aWindow) {
    if (DEBUG) debug("Calling init().");

    this.initDOMRequestHelper(aWindow, kMessages);

    let principal = aWindow.document.nodePrincipal;
    this._manifestURL = appsService.getManifestURLByLocalId(principal.appId);
    this._pageURL = principal.URI.specIgnoringRef;

    // Remove query string.
    this._pageURL = this._pageURL.split("?")[0];

    this._started = false;
    this._closed = false;
    this._messageQueue = [];
    this._deferredClose = false;
  },

  // WebIDL implementation for constructor.
  __init: function(aMessagePortID) {
    if (DEBUG) {
      debug("Calling __init(): aMessagePortID: " + aMessagePortID);
    }

    this._messagePortID = aMessagePortID;

    cpmm.sendAsyncMessage("InterAppMessagePort:Register",
                          { messagePortID: this._messagePortID,
                            manifestURL: this._manifestURL,
                            pageURL: this._pageURL });
  },

  // DOMRequestIpcHelper implementation.
  uninit: function() {
    if (DEBUG) debug("Calling uninit().");

    // When the message port is uninitialized, we need to disentangle the
    // coupling ports, as if the close() method had been called.
    if (this._closed) {
      if (DEBUG) debug("close() has been called. Don't need to close again.");
      return;
    }

    this.close();
  },

  postMessage: function(aMessage) {
    if (DEBUG) debug("Calling postMessage().");

    if (this._closed) {
      if (DEBUG) debug("close() has been called. Cannot post message.");
      return;
    }

    cpmm.sendAsyncMessage("InterAppMessagePort:PostMessage",
                          { messagePortID: this._messagePortID,
                            manifestURL: this._manifestURL,
                            message: aMessage });
  },

  start: function() {
    // Begin dispatching messages received on the port.
    if (DEBUG) debug("Calling start().");

    if (this._closed) {
      if (DEBUG) debug("close() has been called. Cannot call start().");
      return;
    }

    if (this._started) {
      if (DEBUG) debug("start() has been called. Don't need to start again.");
      return;
    }

    // When a port's port message queue is enabled, the event loop must use it
    // as one of its task sources.
    this._started = true;
    while (this._messageQueue.length) {
      let message = this._messageQueue.shift();
      this._dispatchMessage(message);
    }

    if (this._deferredClose) {
      this.close();
    }
  },

  close: function() {
    // Disconnecting the port, so that it is no longer active.
    if (DEBUG) debug("Calling close().");

    if (this._closed) {
      if (DEBUG) debug("close() has been called. Don't need to close again.");
      return;
    }

    this._closed = true;
    this._deferredClose = false;
    this._messageQueue.length = 0;

    // When this method called on a local port that is entangled with another
    // port, must cause the user agent to disentangle the coupling ports.
    cpmm.sendAsyncMessage("InterAppMessagePort:Unregister",
                          { messagePortID: this._messagePortID,
                            started: this._started,
                            manifestURL: this._manifestURL });

    this._dispatchClose();

    this.destroyDOMRequestHelper();
  },

  get onmessage() {
    if (DEBUG) debug("Getting onmessage handler.");

    return this.__DOM_IMPL__.getEventHandler("onmessage");
  },

  set onmessage(aHandler) {
    if (DEBUG) debug("Setting onmessage handler.");

    this.__DOM_IMPL__.setEventHandler("onmessage", aHandler);

    // The first time a MessagePort object's onmessage IDL attribute is set,
    // the port's message queue must be enabled, as if the start() method had
    // been called.
    if (this._started) {
      if (DEBUG) debug("start() has been called. Don't need to start again.");
      return;
    }

    this.start();
  },

  get onclose() {
    if (DEBUG) debug("Getting onclose handler.");
    return this.__DOM_IMPL__.getEventHandler("onclose");
  },

  set onclose(aHandler) {
    if (DEBUG) debug("Setting onclose handler.");
    this.__DOM_IMPL__.setEventHandler("onclose", aHandler);
  },

  _dispatchMessage: function _dispatchMessage(aMessage) {
    let wrappedMessage = Cu.cloneInto(aMessage, this._window);
    if (DEBUG) {
      debug("_dispatchMessage: wrappedMessage: " +
            JSON.stringify(wrappedMessage));
    }

    let event = new this._window
                    .MozInterAppMessageEvent("message",
                                             { data: wrappedMessage });
    this.__DOM_IMPL__.dispatchEvent(event);
  },

  _dispatchClose() {
    if (DEBUG) debug("_dispatchClose");
    let event = new this._window.Event("close", {
      bubbles: true,
      cancelable: true
    });
    this.__DOM_IMPL__.dispatchEvent(event);
  },

  receiveMessage: function(aMessage) {
    if (DEBUG) debug("receiveMessage: name: " + aMessage.name);

    let message = aMessage.json;
    if (message.manifestURL != this._manifestURL ||
        message.pageURL != this._pageURL ||
        message.messagePortID != this._messagePortID) {
      if (DEBUG) debug("The message doesn't belong to this page. Returning. " +
                       uneval(message));
      return;
    }

    switch (aMessage.name) {
      case "InterAppMessagePort:OnMessage":
        if (this._closed) {
          if (DEBUG) debug("close() has been called. Drop the message.");
          return;
        }

        if (!this._started) {
          if (DEBUG) debug("Not yet called start(). Queue up the message.");
          this._messageQueue.push(message.message);
          return;
        }

        this._dispatchMessage(message.message);
        break;

      case "InterAppMessagePort:OnClose":
        if (this._closed) {
          if (DEBUG) debug("close() has been called. Drop the message.");
          return;
        }

        // It is possible that one side of the port posts messages and calls
        // close() before calling start() or setting the onmessage handler. In
        // that case we need to queue the messages and defer the onclose event
        // until the messages are delivered to the other side of the port.
        // Another case is if one side "1" calls start(), the other side "2" may never call start(),
        // then side "1" calls close() and side "2" receive OnClose, this._started
        // will be false and set this._deferredClose = true, but we should not do this
        // because side "2" will never call start() to defer close. So we need to use
        // message.started to check whecher side "1" had called start().
        if (!this._started && !message.started) {
          if (DEBUG) debug("Not yet called start(). Defer close notification.");
          this._deferredClose = true;
          return;
        }

        this.close();
        break;

      case "InterAppMessagePort:Shutdown":
        this.close();
        break;

      default:
        dump("WARNING - Invalid InterAppMessagePort message type " +
             aMessage.name + "\n");
        break;
    }
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([InterAppMessagePort]);

