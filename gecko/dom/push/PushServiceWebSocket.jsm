/* jshint moz: true, esnext: true */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/AppConstants.jsm");
Cu.import("resource://gre/modules/Preferences.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Timer.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

const {PushDB} = Cu.import("resource://gre/modules/PushDB.jsm");
const {PushRecord} = Cu.import("resource://gre/modules/PushRecord.jsm");
const {
  PushCrypto,
  getCryptoParams,
} = Cu.import("resource://gre/modules/PushCrypto.jsm");

const {PushCredential} = Cu.import("resource://gre/modules/PushCredential.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "AlarmService",
                                  "resource://gre/modules/AlarmService.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "gDNSService",
                                   "@mozilla.org/network/dns-service;1",
                                   "nsIDNSService");

if (AppConstants.MOZ_B2G) {
  XPCOMUtils.defineLazyServiceGetter(this, "gPowerManagerService",
                                     "@mozilla.org/power/powermanagerservice;1",
                                     "nsIPowerManagerService");
}

var threadManager = Cc["@mozilla.org/thread-manager;1"]
                      .getService(Ci.nsIThreadManager);

const kPUSHWSDB_DB_NAME = "pushapi";
const kPUSHWSDB_DB_VERSION = 6; // Change this if the IndexedDB format changes
const kPUSHWSDB_STORE_NAME = "pushapi";

const kUDP_WAKEUP_WS_STATUS_CODE = 4774;  // WebSocket Close status code sent
                                          // by server to signal that it can
                                          // wake client up using UDP.

const kMinRequestTimeOut = 1000; // 1s

// Maps ack statuses, unsubscribe reasons, and delivery error reasons to codes
// included in request payloads.
const kACK_STATUS_TO_CODE = {
  [Ci.nsIPushErrorReporter.ACK_DELIVERED]: 100,
  [Ci.nsIPushErrorReporter.ACK_DECRYPTION_ERROR]: 101,
  [Ci.nsIPushErrorReporter.ACK_NOT_DELIVERED]: 102,
};

const kUNREGISTER_REASON_TO_CODE = {
  [Ci.nsIPushErrorReporter.UNSUBSCRIBE_MANUAL]: 200,
  [Ci.nsIPushErrorReporter.UNSUBSCRIBE_QUOTA_EXCEEDED]: 201,
  [Ci.nsIPushErrorReporter.UNSUBSCRIBE_PERMISSION_REVOKED]: 202,
  [Ci.nsIPushErrorReporter.UNSUBSCRIBE_PENDING_RECORD]: 203,
};

const kDELIVERY_REASON_TO_CODE = {
  [Ci.nsIPushErrorReporter.DELIVERY_UNCAUGHT_EXCEPTION]: 301,
  [Ci.nsIPushErrorReporter.DELIVERY_UNHANDLED_REJECTION]: 302,
  [Ci.nsIPushErrorReporter.DELIVERY_INTERNAL_ERROR]: 303,
};

const prefs = new Preferences("dom.push.");

this.EXPORTED_SYMBOLS = ["PushServiceWebSocket"];

XPCOMUtils.defineLazyGetter(this, "console", () => {
  let {ConsoleAPI} = Cu.import("resource://gre/modules/Console.jsm", {});
  return new ConsoleAPI({
    maxLogLevelPref: "dom.push.loglevel",
    prefix: "PushServiceWebSocket",
  });
});

/**
 * A proxy between the PushService and the WebSocket. The listener is used so
 * that the PushService can silence messages from the WebSocket by setting
 * PushWebSocketListener._pushService to null. This is required because
 * a WebSocket can continue to send messages or errors after it has been
 * closed but the PushService may not be interested in these. It's easier to
 * stop listening than to have checks at specific points.
 */
var PushWebSocketListener = function(pushService) {
  this._pushService = pushService;
};

PushWebSocketListener.prototype = {
  onStart: function(context) {
    if (!this._pushService) {
        return;
    }
    this._pushService._wsOnStart(context);
  },

  onStop: function(context, statusCode) {
    if (!this._pushService) {
        return;
    }
    this._pushService._wsOnStop(context, statusCode);
  },

  onAcknowledge: function(context, size) {
    // EMPTY
  },

  onBinaryMessageAvailable: function(context, message) {
    // EMPTY
  },

  onMessageAvailable: function(context, message) {
    if (!this._pushService) {
        return;
    }
    this._pushService._wsOnMessageAvailable(context, message);
  },

  onServerClose: function(context, aStatusCode, aReason) {
    if (!this._pushService) {
        return;
    }
    this._pushService._wsOnServerClose(context, aStatusCode, aReason);
  }
};

// websocket states
// websocket is off
const STATE_SHUT_DOWN = 0;
// Websocket has been opened on client side, waiting for successful open.
// (_wsOnStart)
const STATE_WAITING_FOR_WS_START = 1;
// Websocket opened, hello sent, waiting for server reply (_handleHelloReply).
const STATE_WAITING_FOR_HELLO = 2;
// Websocket operational, handshake completed, begin protocol messaging.
const STATE_READY = 3;

this.PushServiceWebSocket = {
  _mainPushService: null,
  _serverURI: null,

  newPushDB: function() {
    return new PushDB(kPUSHWSDB_DB_NAME,
                      kPUSHWSDB_DB_VERSION,
                      kPUSHWSDB_STORE_NAME,
                      "channelID",
                      PushRecordWebSocket);
  },

  serviceType: function() {
    return "WebSocket";
  },

  disconnect: function() {
    this._shutdownWS();
  },

  observe: function(aSubject, aTopic, aData) {
    if (aTopic == "nsPref:changed" && aData == "dom.push.userAgentID") {
      this._onUAIDChanged();
    } else if (aTopic == "timer-callback") {
      this._onTimerFired(aSubject);
    }
  },

  /**
   * Handles a UAID change. Unlike reconnects, we cancel all pending requests
   * after disconnecting. Existing subscriptions stored in IndexedDB will be
   * dropped on reconnect.
   */
  _onUAIDChanged() {
    console.debug("onUAIDChanged()");

    this._shutdownWS();
    this._startBackoffAlarm();
  },

  /** Handles a ping, backoff, or request timeout timer event. */
  _onTimerFired(timer) {
    console.debug("onTimerFired()");

    if (timer == this._requestTimeoutTimer) {
      this._timeOutRequests();
      return;
    }
  },

  /**
   * There is only one alarm active at any time. This alarm has 3 intervals
   * corresponding to 3 tasks.
   *
   * 1) Reconnect on ping timeout.
   *    If we haven't received any messages from the server by the time this
   *    alarm fires, the connection is closed and PushService tries to
   *    reconnect, repurposing the alarm for (3).
   *
   * 2) Send a ping.
   *    The protocol sends a ping ({}) on the wire every pingInterval ms. Once
   *    it sends the ping, the alarm goes to task (1) which is waiting for
   *    a pong. If data is received after the ping is sent,
   *    _wsOnMessageAvailable() will reset the ping alarm (which cancels
   *    waiting for the pong). So as long as the connection is fine, pong alarm
   *    never fires.
   *
   * 3) Reconnect after backoff.
   *    The alarm is set by _reconnectAfterBackoff() and increases in duration
   *    every time we try and fail to connect.  When it triggers, websocket
   *    setup begins again. On successful socket setup, the socket starts
   *    receiving messages. The alarm now goes to (2) where it monitors the
   *    WebSocket by sending a ping.  Since incoming data is a sign of the
   *    connection being up, the ping alarm is reset every time data is
   *    received.
   */
  _onAlarmFired: function() {
    // Conditions are arranged in decreasing specificity.
    // i.e. when _waitingForPong is true, other conditions are also true.
    if (this._waitingForPong) {
      console.debug("Did not receive pong in time. Reconnecting WebSocket.");
      this._shutdownWS();
      this._startBackoffAlarm();
    } else if (this._currentState == STATE_READY) {
      // Send a ping.
      // Bypass the queue; we don't want this to be kept pending.
      // Watch out for exception in case the socket has disconnected.
      // When this happens, we pretend the ping was sent and don't specially
      // handle the exception, as the lack of a pong will lead to the socket
      // being reset.
      this._sendPing();

      this._waitingForPong = true;
      this._setAlarm(prefs.get("requestTimeout"));
    } else if (this._alarmID !== null) {
      console.debug("reconnect alarm fired.");
      // Reconnect after back-off.
      // The check for a non-null _alarmID prevents a situation where the alarm
      // fires, but _shutdownWS() is called from another code-path (e.g.
      // network state change) and we don't want to reconnect.
      //
      // It also handles the case where _beginWSSetup() is called from another
      // code-path.
      //
      // alarmID will be non-null only when no shutdown/connect is
      // called between _reconnectAfterBackoff() setting the alarm and the
      // alarm firing.

      // Websocket is shut down. Backoff interval expired, try to connect.
      this._beginWSSetup();
    }
  },

  /**
   * Sends a ping to the server. Bypasses the request queue, but starts the
   * request timeout timer. If the socket is already closed, or the server
   * does not respond within the timeout, the client will reconnect.
   */
  _sendPing() {
    console.debug("sendPing()");

    try {
      this._wsSendMessage({});
    } catch (e) {
      console.debug("sendPing: Error sending ping", e);
      this._reconnect();
    }
  },

  /** Times out any pending requests. */
  _timeOutRequests() {
    console.debug("timeOutRequests()");

    if (!this._hasPendingRequests()) {
      // Cancel the repeating timer and exit early if we aren't waiting for
      // requests.
      this._requestTimeoutTimer.cancel();
      this._requestTimeout = this._requestTimeoutBase;
      return;
    }

    let now = Date.now();

    // Set to true if at least one request timed out, or we're still waiting
    // for the request timeout.
    let requestTimedOut = false;

    for (let [channelID, request] of this._registerRequests) {
      let duration = now - request.ctime;
      // If any of the registration requests time out, all the ones after it
      // also made to fail, since we are going to be disconnecting the
      // socket.
      requestTimedOut |= duration > this._requestTimeout;
      if (requestTimedOut) {
        request.reject(new Error(
          "Register request timed out for channel ID " + channelID));
          this._registerRequests.delete(channelID);
      }
    }

    // The most likely reason for registration request timing out is
    // that the socket has disconnected. Best to reconnect.
    if (requestTimedOut) {
      this._reconnect();
    }
  },

  validServerURI: function(serverURI) {
    return serverURI.scheme == "ws" || serverURI.scheme == "wss";
  },

  get _UAID() {
    return prefs.get("userAgentID");
  },

  set _UAID(newID) {
    if (typeof(newID) !== "string") {
      console.warn("Got invalid, non-string UAID", newID,
        "Not updating userAgentID");
      return;
    }
    console.debug("New _UAID", newID);
    prefs.set("userAgentID", newID);
    // Ensure the UAID changed is immediately persisted
    Services.prefs.savePrefFile(null);
  },

  _ws: null,
  _registerRequests: new Map(),
  _currentState: STATE_SHUT_DOWN,
  _requestTimeout: 0,
  _requestTimeoutTimer: null,
  _requestTimeoutBase: 0,
  _retryFailCount: 0,

  /**
   * According to the WS spec, servers should immediately close the underlying
   * TCP connection after they close a WebSocket. This causes wsOnStop to be
   * called with error NS_BASE_STREAM_CLOSED. Since the client has to keep the
   * WebSocket up, it should try to reconnect. But if the server closes the
   * WebSocket because it will wake up the client via UDP, then the client
   * shouldn't re-establish the connection. If the server says that it will
   * wake up the client over UDP, this is set to true in wsOnServerClose. It is
   * checked in wsOnStop.
   */
  _willBeWokenUpByUDP: false,

  /**
   * Holds if the adaptive ping is enabled. This is read on init().
   * If adaptive ping is enabled, a new ping is calculed each time we receive
   * a pong message, trying to maximize network resources while minimizing
   * cellular signalling storms.
   */
  _adaptiveEnabled: false,

  /**
   * Holds if the udp wake up feature is enabled. This is read on init().
   * If udp wake up feature is enabled, the client is capable of being
   * woken up by UDP (which currently just means having an mcc and mnc along
   * with an IP, and optionally a netid).
   */
  _udpWakeUpEnabled: false,

  /**
   * This saves a flag about if we need to recalculate a new ping, based on:
   *   1) the gap between the maximum working ping and the first ping that
   *      gives an error (timeout) OR
   *   2) we have reached the pref of the maximum value we allow for a ping
   *      (dom.push.adaptive.upperLimit)
   */
  _recalculatePing: true,

  /**
   * This map holds a (pingInterval, triedTimes) of each pingInterval tried.
   * It is used to check if the pingInterval has been tested enough to know that
   * is incorrect and is above the limit the network allow us to keep the
   * connection open.
   */
  _pingIntervalRetryTimes: {},

  /**
   * Holds the lastGoodPingInterval for our current connection.
   */
  _lastGoodPingInterval: 0,

  /**
   * Maximum ping interval that we can reach.
   */
  _upperLimit: 0,

  /** Indicates whether the server supports Web Push-style message delivery. */
  _dataEnabled: false,

  _socketWakeLock: {},

  /**
   * Sends a message to the Push Server through an open websocket.
   * typeof(msg) shall be an object
   */
  _wsSendMessage: function(msg) {
    if (!this._ws) {
      console.warn("wsSendMessage: No WebSocket initialized.",
        "Cannot send a message");
      return;
    }
    msg = JSON.stringify(msg);
    console.debug("wsSendMessage: Sending message", msg);
    this._ws.sendMsg(msg);
  },

  init: function(options, mainPushService, serverURI) {
    console.debug("init()");

    this._mainPushService = mainPushService;
    this._serverURI = serverURI;
    this._alarmID = null;

    // Override the default WebSocket factory function. The returned object
    // must be null or satisfy the nsIWebSocketChannel interface. Used by
    // the tests to provide a mock WebSocket implementation.
    if (options.makeWebSocket) {
      this._makeWebSocket = options.makeWebSocket;
    }

    // Override the default UDP socket factory function. The returned object
    // must be null or satisfy the nsIUDPSocket interface. Used by the
    // UDP tests.
    if (options.makeUDPSocket) {
      this._makeUDPSocket = options.makeUDPSocket;
    }

    this._networkInfo = options.networkInfo;
    if (!this._networkInfo) {
      this._networkInfo = PushNetworkInfo;
    }

    this._requestTimeoutBase = prefs.get("requestTimeout");
    if (this._requestTimeoutBase < kMinRequestTimeOut) {
      this._requestTimeoutBase = kMinRequestTimeOut;
      console.warn("requestTimeout smaller than min, set it as default");
    }
    this._requestTimeout = this._requestTimeoutBase;
    this._adaptiveEnabled = prefs.get('adaptive.enabled');
    this._udpWakeUpEnabled = prefs.get('udp.wakeupEnabled');
    this._upperLimit = prefs.get('adaptive.upperLimit');

    if (prefs.get("authorization.enabled")) {
      this.credential = new PushCredential();
    }

    // reset adaptive-set value if adaptive mode is disabled
    if (!this._adaptiveEnabled) {
      prefs.reset('pingInterval');
      // mobile network
      prefs.reset('adaptive.mobile');
      prefs.reset('pingInterval.mobile');
      prefs.reset('adaptive.lastGoodPingInterval.mobile');
      // wifi network
      prefs.reset('pingInterval.wifi');
      prefs.reset('adaptive.lastGoodPingInterval.wifi');
    }

    return Promise.resolve();
  },

  _reconnect: function() {
    console.debug("reconnect()");
    this._shutdownWS(false);
    this._startBackoffAlarm();
  },

  _shutdownWS: function(shouldCancelPending = true) {
    console.debug("shutdownWS()");
    this._willBeWokenUpByUDP = false;

    prefs.ignore("userAgentID", this);

    if (this._currentState >= STATE_WAITING_FOR_WS_START) {
      if (this._wsListener) {
        this._wsListener._pushService = null;
      }

      if (this._ws) {
        try {
          this._ws.close(0, null);
        } catch (e) {}
        this._ws = null;
      }
    } else {
      if (this._wsListener) {
        this._wsListener._pushService = null;
      }
      this._ws = null;
    }
    this._currentState = STATE_SHUT_DOWN;

    if (shouldCancelPending) {
      this._cancelRegisterRequests();
    }

    if (this._notifyRequestQueue) {
      this._notifyRequestQueue();
      this._notifyRequestQueue = null;
    }

    this._waitingForPong = false;
    this._stopAlarm();
  },

  uninit: function() {
    if (this._udpServer) {
      this._udpServer.close();
      this._udpServer = null;
    }

    // All pending requests (ideally none) are dropped at this point. We
    // shouldn't have any applications performing registration/unregistration
    // or receiving notifications.
    this._shutdownWS();
    // Release all wakeLocks.
    for (var index in this._socketWakeLock) {
      this._releaseWakeLock(index);
    }

    if (this._requestTimeoutTimer) {
      this._requestTimeoutTimer.cancel();
      this._requestTimeout = this._requestTimeoutBase;
    }

    this._mainPushService = null;

    this._dataEnabled = false;
    if (this.credential) {
      this.credential = null;
    }
  },

  /**
   * How retries work:  The goal is to ensure websocket is always up on
   * networks not supporting UDP. So the websocket should only be shutdown if
   * onServerClose indicates UDP wakeup.  If WS is closed due to socket error,
   * _startBackoffAlarm() is called. The retry timer is started and when
   * it times out, beginWSSetup() is called again.
   *
   * If we are in the middle of a timeout (i.e. waiting), but
   * a register/unregister is called, we don't want to wait around anymore.
   * _sendRequest will automatically call beginWSSetup(), which will cancel the
   * timer. In addition since the state will have changed, even if a pending
   * timer event comes in (because the timer fired the event before it was
   * cancelled), so the connection won't be reset.
   */
  _startBackoffAlarm() {
    console.debug("startBackoffAlarm()");
    //Calculate new ping interval
    this._calculateAdaptivePing(true /* wsWentDown */);

    // Calculate new timeout, but cap it to pingInterval.
    let retryTimeout = prefs.get("retryBaseInterval") *
                       Math.pow(2, this._retryFailCount);
    retryTimeout = Math.min(retryTimeout, prefs.get("pingInterval"));

    this._retryFailCount++;

    console.debug("startBackoffAlarm: Retry in", retryTimeout,
      "Try number", this._retryFailCount);

    this._setAlarm(retryTimeout);
  },

  /** Indicates whether we're waiting requests. */
  _hasPendingRequests() {
    return this._registerRequests.size > 0;
  },

  /**
   * Starts the request timeout timer unless we're already waiting for a
   * register request.
   */
  _startRequestTimeoutTimer() {
    if (this._hasPendingRequests()) {
      return;
    }
    if (!this._requestTimeoutTimer) {
      this._requestTimeoutTimer = Cc["@mozilla.org/timer;1"]
                                    .createInstance(Ci.nsITimer);
    }

    let extendTimeout = 0;
    if (!this._ws) {
      if (this.credential &&
          this.credential.isExpired) {
        extendTimeout += prefs.get("extendTimeout.token");
      }
    }
    if (extendTimeout > 0) {
      this._requestTimeout = this._requestTimeoutBase +
                             extendTimeout;
    }
    this._requestTimeoutTimer.init(this,
                                   this._requestTimeout,
                                   Ci.nsITimer.TYPE_REPEATING_SLACK);
  },

  /** |delay| should be in milliseconds. */
  _setAlarm: function(delay) {
    // Bug 909270: Since calls to AlarmService.add() are async, calls must be
    // 'queued' to ensure only one alarm is ever active.
    if (this._settingAlarm) {
      // onSuccess will handle the set. Overwriting the variable enforces the
      // last-writer-wins semantics.
      this._queuedAlarmDelay = delay;
      this._waitingForAlarmSet = true;
      return;
    }

    // Stop any existing alarm.
    this._stopAlarm();

    this._settingAlarm = true;
    AlarmService.add(
      {
        date: new Date(Date.now() + delay),
        ignoreTimezone: true
      },
      this._onAlarmFired.bind(this),
      function onSuccess(alarmID) {
        this._alarmID = alarmID;
        console.debug("Set alarm " + delay + " in the future " + this._alarmID);
        this._settingAlarm = false;

        if (this._waitingForAlarmSet) {
          this._waitingForAlarmSet = false;
          this._setAlarm(this._queuedAlarmDelay);
        }
      }.bind(this)
    )
  },

  _stopAlarm: function() {
    if (this._alarmID !== null) {
      console.debug("Stopped existing alarm " + this._alarmID);
      AlarmService.remove(this._alarmID);
      this._alarmID = null;
    }
  },

  /**
   * We need to calculate a new ping based on:
   *  1) Latest good ping
   *  2) A safe gap between 1) and the calculated new ping (which is
   *  by default, 1 minute)
   *
   * This is for 3G networks, whose connections keepalives differ broadly,
   * for example:
   *  1) Movistar Spain: 29 minutes
   *  2) VIVO Brazil: 5 minutes
   *  3) Movistar Colombia: XXX minutes
   *
   * So a fixed ping is not good for us for two reasons:
   *  1) We might lose the connection, so we need to reconnect again (wasting
   *  resources)
   *  2) We use a lot of network signaling just for pinging.
   *
   * This algorithm tries to search the best value between a disconnection and a
   * valid ping, to ensure better battery life and network resources usage.
   *
   * The value is saved in dom.push.pingInterval
   * @param wsWentDown [Boolean] if the WebSocket was closed or it is still
   * alive
   *
   */
  _calculateAdaptivePing: function(wsWentDown) {
    console.debug("_calculateAdaptivePing()");
    if (!this._adaptiveEnabled) {
      console.debug("calculateAdaptivePing: Adaptive ping is disabled");
      return;
    }

    if (this._retryFailCount > 0) {
      console.warn("calculateAdaptivePing: Push has failed to connect to the",
        "Push Server", this._retryFailCount, "times. Do not calculate a new",
        "pingInterval now");
      return;
    }

    if (!this._recalculatePing && !wsWentDown) {
      console.debug("calculateAdaptivePing: We do not need to recalculate the",
        "ping now, based on previous data");
      return;
    }

    // Save actual state of the network
    let ns = this._networkInfo.getNetworkInformation();

    if (ns.ip) {
      // mobile
      console.debug("calculateAdaptivePing: mobile");
      let oldNetwork = prefs.get('adaptive.mobile');
      let newNetwork = 'mobile-' + ns.mcc + '-' + ns.mnc;

      // Mobile networks differ, reset all intervals and pings
      if (oldNetwork !== newNetwork) {
        // Network differ, reset all values
        console.debug("calculateAdaptivePing: Mobile networks differ. Old",
          "network is", oldNetwork, "and new is", newNetwork);
        prefs.set('adaptive.mobile', newNetwork);
        //We reset the upper bound member
        this._recalculatePing = true;
        this._pingIntervalRetryTimes = {};

        // Put default values
        let defaultPing = prefs.get('pingInterval.default');
        prefs.set('pingInterval', defaultPing);
        this._lastGoodPingInterval = defaultPing;

      } else {
        // Mobile network is the same, let's just update things
        prefs.set('pingInterval', prefs.get('pingInterval.mobile'));
        this._lastGoodPingInterval = prefs.get('adaptive.lastGoodPingInterval.mobile');
      }

    } else {
      // wifi
      console.debug("calculateAdaptivePing: wifi");
      prefs.set('pingInterval', prefs.get('pingInterval.wifi'));
      this._lastGoodPingInterval = prefs.get('adaptive.lastGoodPingInterval.wifi');
    }

    let nextPingInterval;
    let lastTriedPingInterval = prefs.get('pingInterval');

    if (wsWentDown) {
      console.debug("calculateAdaptivePing: The WebSocket was disconnected.",
        "Calculating next ping");

      // If we have not tried this pingInterval yet, initialize
      this._pingIntervalRetryTimes[lastTriedPingInterval] =
           (this._pingIntervalRetryTimes[lastTriedPingInterval] || 0) + 1;

       // Try the pingInterval at least 3 times, just to be sure that the
       // calculated interval is not valid.
       if (this._pingIntervalRetryTimes[lastTriedPingInterval] < 2) {
         console.debug("calculateAdaptivePing: pingInterval=",
          lastTriedPingInterval, "tried only",
          this._pingIntervalRetryTimes[lastTriedPingInterval], "times");
         return;
       }

       // Latest ping was invalid, we need to lower the limit to limit / 2
       nextPingInterval = Math.floor(lastTriedPingInterval / 2);

      // If the new ping interval is close to the last good one, we are near
      // optimum, so stop calculating.
      if (nextPingInterval - this._lastGoodPingInterval <
          prefs.get('adaptive.gap')) {
        console.debug("calculateAdaptivePing: We have reached the gap, we",
          "have finished the calculation. nextPingInterval=", nextPingInterval,
          "lastGoodPing=", this._lastGoodPingInterval);
        nextPingInterval = this._lastGoodPingInterval;
        this._recalculatePing = false;
      } else {
        console.debug("calculateAdaptivePing: We need to calculate next time");
        this._recalculatePing = true;
      }

    } else {
      console.debug("calculateAdaptivePing: The WebSocket is still up");
      this._lastGoodPingInterval = lastTriedPingInterval;
      nextPingInterval = Math.floor(lastTriedPingInterval * 1.5);
    }

    // Check if we have reached the upper limit
    if (this._upperLimit < nextPingInterval) {
      console.debug("calculateAdaptivePing: Next ping will be bigger than the",
        "configured upper limit, capping interval");
      this._recalculatePing = false;
      this._lastGoodPingInterval = lastTriedPingInterval;
      nextPingInterval = lastTriedPingInterval;
    }

    console.debug("calculateAdaptivePing: Setting the pingInterval to",
      nextPingInterval);
    prefs.set('pingInterval', nextPingInterval);

    //Save values for our current network
    if (ns.ip) {
      prefs.set('pingInterval.mobile', nextPingInterval);
      prefs.set('adaptive.lastGoodPingInterval.mobile',
                this._lastGoodPingInterval);
    } else {
      prefs.set('pingInterval.wifi', nextPingInterval);
      prefs.set('adaptive.lastGoodPingInterval.wifi',
                this._lastGoodPingInterval);
    }
  },

  _makeWebSocket: function(uri) {
    if (!prefs.get("connection.enabled")) {
      console.warn("makeWebSocket: connection.enabled is not set to true.",
        "Aborting.");
      return null;
    }
    if (Services.io.offline) {
      console.warn("makeWebSocket: Network is offline.");
      return null;
    }
    let socket = Cc["@mozilla.org/network/protocol;1?name=wss"]
                   .createInstance(Ci.nsIWebSocketChannel);

    socket.initLoadInfo(null, // aLoadingNode
                        Services.scriptSecurityManager.getSystemPrincipal(),
                        null, // aTriggeringPrincipal
                        Ci.nsILoadInfo.SEC_NORMAL,
                        Ci.nsIContentPolicy.TYPE_WEBSOCKET);

    return socket;
  },

  _beginWSSetup: function() {
    console.debug("beginWSSetup()");
    if (this._currentState != STATE_SHUT_DOWN) {
      console.error("_beginWSSetup: Not in shutdown state! Current state",
        this._currentState);
      return Promise.resolve();
    }

    let uri = this._serverURI;
    if (!uri) {
      return Promise.resolve();
    }
    let socket = this._makeWebSocket(uri);
    if (!socket) {
      return Promise.resolve();
    }
    this._ws = socket.QueryInterface(Ci.nsIWebSocketChannel);

    this._wsListener = new PushWebSocketListener(this);
    this._ws.protocol = "push-notification";

    function tryOpenWS() {
      console.debug("beginWSSetup: Connecting to", uri.spec);
      try {
        // Grab a wakelock before we open the socket to ensure we don't go to
        // sleep before connection the is opened.
        this._ws.asyncOpen(uri, uri.spec, 0, this._wsListener, null);
        this._acquireWakeLock('WebSocketSetup', this._requestTimeout);
        this._currentState = STATE_WAITING_FOR_WS_START;
      } catch(e) {
        console.error("beginWSSetup: Error opening websocket.",
          "asyncOpen failed", e);
        this._reconnect();
      }
      return Promise.resolve();
    }

    if (this.credential) {
      return this.credential.requireAccessToken()
        .then(_ => {
          return tryOpenWS.bind(this)();
        }, errorStatus => {
          if (!this._hasPendingRequests()) {
            if (errorStatus > 0) {
              return this._mainPushService.getAllUnexpired()
                .then(records => {
                  if (records.length > 0) {
                    this._reconnect();
                  } else {
                    this._shutdownWS();
                  }
                  return Promise.resolve();
                });
            } else {
              this._shutdownWS();
              return Promise.resolve();
            }
          } else {
            return Promise.resolve();
          }
        });
    } else {
      return tryOpenWS.bind(this)();
    }
  },

  connect: function(records) {
    console.debug("connect()");
    // Check to see if we need to do anything.
    if (records.length > 0) {
      this._beginWSSetup();
    }
  },

  isConnected: function() {
    return !!this._ws;
  },

  _acquireWakeLock: function (reason, duration) {
    if (!AppConstants.MOZ_B2G) {
      return;
    }

    if (!this._socketWakeLock[reason]) {
      this._socketWakeLock[reason] = {
        wakeLock: null,
        timer: null
      };
      console.debug("acquireWakeLock: Acquiring " + reason + " Wakelock and Creating Timer");
      this._socketWakeLock[reason].wakeLock = gPowerManagerService.newWakeLock("cpu");
      this._socketWakeLock[reason].timer = Cc["@mozilla.org/timer;1"]
                                             .createInstance(Ci.nsITimer);
    } else {
      if (!this._socketWakeLock[reason].wakeLock) {
        console.debug("acquireWakeLock: Acquiring " + reason + " Wakelock");
        this._socketWakeLock[reason].wakeLock = gPowerManagerService.newWakeLock("cpu");
      }
      if (!this._socketWakeLock[reason].timer) {
        console.debug("acquireWakeLock: Creating " + reason + " WakeLock Timer");
        this._socketWakeLock[reason].timer = Cc["@mozilla.org/timer;1"]
                                               .createInstance(Ci.nsITimer);
      }
    }

    let timeout = this._requestTimeoutBase;
    if (typeof duration === 'number' &&
        duration > 0) {
      timeout = duration;
    }
    console.debug("acquireWakeLock: Setting  " + reason + "  WakeLock Timer");
    this._socketWakeLock[reason].timer
      .initWithCallback(this._releaseWakeLock.bind(this, reason),
                        // Allow the same time for socket setup as we do for
                        // requests after the setup. Fudge it a bit since
                        // timers can be a little off and we don't want to go
                        // to sleep just as the socket connected.
                        timeout + 1000,
                        Ci.nsITimer.TYPE_ONE_SHOT);
  },

  _releaseWakeLock: function (reason) {
    if (!AppConstants.MOZ_B2G) {
      return;
    }
    if (!this._socketWakeLock[reason]) {
      console.warn("Drop releasing " + reason + " wakeLock due to nonexistent!");
      return;
    }

    console.debug("releaseWakeLock: Releasing " + reason + " WakeLock");
    if (this._socketWakeLock[reason].timer) {
      this._socketWakeLock[reason].timer.cancel();
    }
    if (this._socketWakeLock[reason].wakeLock) {
      this._socketWakeLock[reason].wakeLock.unlock();
      this._socketWakeLock[reason].wakeLock = null;
    }
  },

  /**
   * Protocol handler invoked by server message.
   */
  _handleHelloReply: function(reply) {
    console.debug("handleHelloReply()");
    if (this._currentState != STATE_WAITING_FOR_HELLO) {
      console.error("handleHelloReply: Unexpected state", this._currentState,
        "(expected STATE_WAITING_FOR_HELLO)");
      this._shutdownWS();
      return;
    }

    if (reply.status == 401) {
      console.error("handleHelloReply: Invalid Token");
      this._shutdownWS();
      if (this.credential) {
        this._mainPushService.getAllUnexpired().then(records => {
          if (records.length > 0) {
            this.credential.refreshAccessToken();
            this._reconnect();
          }
        });
      }
      return;
    }

    if (typeof reply.uaid !== "string") {
      console.error("handleHelloReply: Received invalid UAID", reply.uaid);
      this._shutdownWS();
      return;
    }

    if (reply.uaid === "") {
      console.error("handleHelloReply: Received empty UAID");
      this._shutdownWS();
      return;
    }

    // To avoid sticking extra large values sent by an evil server into prefs.
    if (reply.uaid.length > 128) {
      console.error("handleHelloReply: UAID received from server was too long",
        reply.uaid);
      this._shutdownWS();
      return;
    }

    this._retryFailCount = 0;

    let sendRequests = () => {
      if (this._notifyRequestQueue) {
        this._notifyRequestQueue();
        this._notifyRequestQueue = null;
      }
      this._sendRegisterRequests();
    };

    function finishHandshake() {
      this._UAID = reply.uaid;
      this._currentState = STATE_READY;
      prefs.observe("userAgentID", this);

      this._dataEnabled = !!reply.use_webpush;
      if (this._dataEnabled) {
        this._mainPushService.executeAllPendingUnregistering();
        this._mainPushService.getAllUnexpired().then(records =>
          Promise.all(records.map(record =>
            this._mainPushService.ensureCrypto(record).catch(error => {
              console.error("finishHandshake: Error updating record",
                record.keyID, error);
            })
          ))
        ).then(sendRequests);
      } else {
        sendRequests();
      }
    }

    // By this point we've got a UAID from the server that we are ready to
    // accept.
    //
    // We unconditionally drop all existing registrations and notify service
    // workers if we receive a new UAID. This ensures we expunge all stale
    // registrations if the `userAgentID` pref is reset.
    if (this._UAID != reply.uaid) {
      console.debug("handleHelloReply: Received new UAID");

      this._mainPushService.dropUnexpiredRegistrations()
          .then(finishHandshake.bind(this));

      return;
    }

    // otherwise we are good to go
    finishHandshake.bind(this)();
  },

  /**
   * Protocol handler invoked by server message.
   */
  _handleRegisterReply: function(reply) {
    console.debug("handleRegisterReply()");
    if (typeof reply.channelID !== "string" ||
        !this._registerRequests.has(reply.channelID)) {
      return;
    }

    let tmp = this._registerRequests.get(reply.channelID);
    this._registerRequests.delete(reply.channelID);
    if (!this._hasPendingRequests()) {
      this._requestTimeoutTimer.cancel();
      this._requestTimeout = this._requestTimeoutBase;
    }

    if (reply.status == 200) {
      try {
        Services.io.newURI(reply.pushEndpoint, null, null);
      }
      catch (e) {
        tmp.reject(new Error("Invalid push endpoint: " + reply.pushEndpoint));
        return;
      }

      let record = new PushRecordWebSocket({
        channelID: reply.channelID,
        pushEndpoint: reply.pushEndpoint,
        scope: tmp.record.scope,
        originAttributes: tmp.record.originAttributes,
        version: null,
        systemRecord: tmp.record.systemRecord,
        appServerKey: tmp.record.appServerKey,
        ctime: Date.now(),
      });
      Services.telemetry.getHistogramById("PUSH_API_SUBSCRIBE_WS_TIME").add(Date.now() - tmp.ctime);
      tmp.resolve(record);
    } else {
      console.error("handleRegisterReply: Unexpected server response", reply);
      tmp.reject(new Error("Wrong status code for register reply: " +
        reply.status));
    }
  },

  _queueUpdateStart: Promise.resolve(),
  _queueUpdate: null,
  _enqueueUpdateCount: 0,
  _enqueueUpdate: function(op) {
    console.debug("enqueueUpdate()");
    if (!this._queueUpdate) {
      this._queueUpdate = this._queueUpdateStart;
      this._enqueueUpdateCount = 0;
    }
    this._enqueueUpdateCount++;
    this._queueUpdate = this._queueUpdate
                          .then(op)
                          .catch(_ => {})
                          .then(_ => {
                            this._enqueueUpdateCount--;
                            if (this._enqueueUpdateCount == 0) {
                              this._releaseWakeLock('DataUpdate');
                            }
                          });
  },

  _handleDataUpdate: function(update) {
    this._acquireWakeLock('DataUpdate');

    let promise;
    if (typeof update.channelID != "string") {
      console.warn("handleDataUpdate: Discarding update without channel ID",
        update);
      return;
    }
    if (typeof update.data != "string") {
      promise = this._mainPushService.receivedPushMessage(
        update.channelID,
        update.version,
        null,
        null,
        record => record
      );
    } else {
      let params = getCryptoParams(update.headers);
      if (params) {
        let message = ChromeUtils.base64URLDecode(update.data, {
          // The Push server may append padding.
          padding: "ignore",
        });
        promise = this._mainPushService.receivedPushMessage(
          update.channelID,
          update.version,
          message,
          params,
          record => record
        );
      } else {
        promise = Promise.reject(new Error("Invalid crypto headers"));
      }
    }
    return promise.then(status => {
      this._sendAck(update.channelID, update.version, status);
    }, err => {
      console.error("handleDataUpdate: Error delivering message", update, err);
      this._sendAck(update.channelID, update.version,
        Ci.nsIPushErrorReporter.ACK_DECRYPTION_ERROR);
    }).catch(err => {
      console.error("handleDataUpdate: Error acknowledging message", update,
        err);
    });
  },

  /**
   * Protocol handler invoked by server message.
   */
  _handleNotificationReply: function(reply) {
    console.debug("handleNotificationReply()");
    if (this._dataEnabled) {
      this._enqueueUpdate(_ => this._handleDataUpdate(reply));
      return;
    }

    if (typeof reply.updates !== 'object') {
      console.warn("handleNotificationReply: Missing updates", reply.updates);
      return;
    }

    console.debug("handleNotificationReply: Got updates", reply.updates);
    for (let i = 0; i < reply.updates.length; i++) {
      let update = reply.updates[i];
      console.debug("handleNotificationReply: Handling update", update);
      if (typeof update.channelID !== "string") {
        console.debug("handleNotificationReply: Invalid update at index",
          i, update);
        continue;
      }

      if (update.version === undefined) {
        console.debug("handleNotificationReply: Missing version", update);
        continue;
      }

      let version = update.version;

      if (typeof version === "string") {
        version = parseInt(version, 10);
      }

      if (typeof version === "number" && version >= 0) {
        // FIXME(nsm): this relies on app update notification being infallible!
        // eventually fix this
        this._receivedUpdate(update.channelID, version);
      }
    }
  },

  _handleUnregisterReply: function(reply) {
    console.debug("handleUnregisterReply");
    if (reply.status == 200) {
      if (typeof reply.channelID !== "string") {
        console.warn("handleUnregisterReply: Discarding delete db without channel ID");
        return;
      }
      this._mainPushService.removePendingUnsubscribe(reply.channelID);
    } else {
      console.error("handleUnregisterReply: unregister fail, status: ", reply.status);
    }
  },

  reportDeliveryError(messageID, reason) {
    console.debug("reportDeliveryError()");
    let code = kDELIVERY_REASON_TO_CODE[reason];
    if (!code) {
      throw new Error('Invalid delivery error reason');
    }
    let data = {messageType: 'nack',
                version: messageID,
                code: code};
    this._queueRequest(data);
  },

  _sendAck(channelID, version, status) {
    console.debug("sendAck()");
    let code = kACK_STATUS_TO_CODE[status];
    if (!code) {
      throw new Error('Invalid ack status');
    }
    let data = {messageType: 'ack',
                updates: [{channelID: channelID,
                           version: version,
                           code: code}]};
    this._queueRequest(data);
  },

  _generateID: function() {
    let uuidGenerator = Cc["@mozilla.org/uuid-generator;1"]
                          .getService(Ci.nsIUUIDGenerator);
    // generateUUID() gives a UUID surrounded by {...}, slice them off.
    return uuidGenerator.generateUUID().toString().slice(1, -1);
  },

  register(record) {
    console.debug("register() ", record);

    // start the timer since we now have at least one request
    this._startRequestTimeoutTimer();

    let data = {
      channelID: this._generateID(),
      messageType: "register"
    };

    if (record.appServerKey) {
      data.key = ChromeUtils.base64URLEncode(record.appServerKey, {
        // The Push server requires padding.
        pad: true,
      });
    }

    return new Promise((resolve, reject) => {
      this._registerRequests.set(data.channelID, {
        record: record,
        resolve: resolve,
        reject: reject,
        ctime: Date.now(),
        key: data.key || null
      });
      this._queueRequest(data);
    }).then(record => {
      if (!this._dataEnabled) {
        return record;
      }
      return PushCrypto.generateKeys()
        .then(([publicKey, privateKey]) => {
          record.p256dhPublicKey = publicKey;
          record.p256dhPrivateKey = privateKey;
          record.authenticationSecret = PushCrypto.generateAuthenticationSecret();
          return record;
        });
    });
  },

  unregister(record, reason) {
    console.debug("unregister() ", record, reason);

    let code = kUNREGISTER_REASON_TO_CODE[reason];
    if (!code) {
      return Promise.reject(new Error('Invalid unregister reason'));
    }
    let data = {channelID: record.channelID,
                messageType: "unregister",
                code: code};
    this._queueRequest(data);
    return Promise.resolve();
  },

  _queueStart: Promise.resolve(),
  _notifyRequestQueue: null,
  _queue: null,
  _enqueue: function(op) {
    console.debug("enqueue()");
    if (!this._queue) {
      this._queue = this._queueStart;
    }
    this._queue = this._queue
                    .then(op)
                    .catch(_ => {});
  },

  _send(data) {
    if (this._currentState == STATE_READY) {
      if (data.messageType != "register" ||
          this._registerRequests.has(data.channelID)) {

        // check if request has not been cancelled
        this._wsSendMessage(data);
      }
    }
  },

  _sendRegisterRequests() {
    this._enqueue(_ => {
      for (let [channelID, request] of this._registerRequests) {
        let data = {
          messageType: "register",
          channelID: channelID
        };

        if (request.key) {
          data.key = request.key;
        }
        this._send(data);
      }
    });
  },

  _queueRequest(data) {
    if (data.messageType != "register") {
      if (this._currentState != STATE_READY && !this._notifyRequestQueue) {
        let promise = new Promise((resolve, reject) => {
          this._notifyRequestQueue = resolve;
        });
        this._enqueue(_ => promise);
      }

      this._enqueue(_ => this._send(data));
    } else if (this._currentState == STATE_READY) {
      this._send(data);
    }

    if (!this._ws) {
      // This will end up calling notifyRequestQueue().
      return this._beginWSSetup()
        .then(_ => {
          // If beginWSSetup does not succeed to make ws, notifyRequestQueue will
          // not be call.
          if (!this._ws && this._notifyRequestQueue) {
            this._notifyRequestQueue();
            this._notifyRequestQueue = null;
          }
      });
    }
  },

  _receivedUpdate: function(aChannelID, aLatestVersion) {
    console.debug("receivedUpdate: Updating", aChannelID, "->", aLatestVersion);

    this._mainPushService.receivedPushMessage(aChannelID, "", null, null, record => {
      if (record.version === null ||
          record.version < aLatestVersion) {
        console.debug("receivedUpdate: Version changed for", aChannelID,
          aLatestVersion);
        record.version = aLatestVersion;
        return record;
      }
      console.debug("receivedUpdate: No significant version change for",
        aChannelID, aLatestVersion);
      return null;
    }).then(status => {
      this._sendAck(aChannelID, aLatestVersion, status);
    }).catch(err => {
      console.error("receivedUpdate: Error acknowledging message", aChannelID,
        aLatestVersion, err);
    });
  },

  // begin Push protocol handshake
  _wsOnStart: function(context) {
    console.debug("wsOnStart()");
    this._releaseWakeLock('WebSocketSetup');

    if (this._currentState != STATE_WAITING_FOR_WS_START) {
      console.error("wsOnStart: NOT in STATE_WAITING_FOR_WS_START. Current",
        "state", this._currentState, "Skipping");
      return;
    }

    let data = {
      messageType: "hello",
      use_webpush: true,
    };

    if (this._UAID) {
      data.uaid = this._UAID;
    }

    if (this.credential && this.credential.token) {
      data.token = this.credential.token;
    }

    if(this._udpWakeUpEnabled) {
      this._networkInfo.getNetworkState((networkState) => {
        if (networkState.ip) {
          // Opening an available UDP port.
          this._listenForUDPWakeup();

          // Host-port is apparently a thing.
          data.wakeup_hostport = {
            ip: networkState.ip,
            port: this._udpServer && this._udpServer.port
          };

          data.mobilenetwork = {
            mcc: networkState.mcc,
            mnc: networkState.mnc,
            netid: networkState.netid
          };
        }

        this._wsSendMessage(data);
        this._currentState = STATE_WAITING_FOR_HELLO;
      });
    } else {
      this._wsSendMessage(data);
      this._currentState = STATE_WAITING_FOR_HELLO;
    }
  },

  /**
   * This statusCode is not the websocket protocol status code, but the TCP
   * connection close status code.
   *
   * If we do not explicitly call ws.close() then statusCode is always
   * NS_BASE_STREAM_CLOSED, even on a successful close.
   */
  _wsOnStop: function(context, statusCode) {
    console.debug("wsOnStop()");
    this._releaseWakeLock('WebSocketSetup');

    if (statusCode != Cr.NS_OK &&
        !(statusCode == Cr.NS_BASE_STREAM_CLOSED && this._willBeWokenUpByUDP)) {
      console.debug("wsOnStop: Socket error", statusCode);
      this._reconnect();
      return;
    }

    this._shutdownWS();
  },

  _wsOnMessageAvailable: function(context, message) {
    console.debug("wsOnMessageAvailable()", message);

    // Clearing the last ping time indicates we're no longer waiting for a pong.
    this._waitingForPong = false;

    let reply;
    try {
      reply = JSON.parse(message);
    } catch(e) {
      console.warn("wsOnMessageAvailable: Invalid JSON", message, e);
      return;
    }

    // If we receive a message, we know the connection succeeded. Reset the
    // connection attempt and ping interval counters.
    if (this._currentState == STATE_READY) {
      this._retryFailCount = 0;
      this._pingIntervalRetryTimes = {};
    }

    let doNotHandle = false;
    if ((message === '{}') ||
        (reply.messageType === undefined) ||
        (reply.messageType === "ping") ||
        (typeof reply.messageType != "string")) {
      console.debug("wsOnMessageAvailable: Pong received");
      this._calculateAdaptivePing(false);
      doNotHandle = true;
    }

    // Reset the ping timer.  Note: This path is executed at every step of the
    // handshake, so this timer does not need to be set explicitly at startup.
    this._setAlarm(prefs.get("pingInterval"));

    // If it is a ping, do not handle the message.
    if (doNotHandle) {
      return;
    }

    // A whitelist of protocol handlers. Add to these if new messages are added
    // in the protocol.
    let handlers = ["Hello", "Register", "Notification", "Unregister"];

    // Build up the handler name to call from messageType.
    // e.g. messageType == "register" -> _handleRegisterReply.
    let handlerName = reply.messageType[0].toUpperCase() +
                      reply.messageType.slice(1).toLowerCase();

    if (handlers.indexOf(handlerName) == -1) {
      console.warn("wsOnMessageAvailable: No whitelisted handler", handlerName,
        "for message", reply.messageType);
      return;
    }

    let handler = "_handle" + handlerName + "Reply";

    if (typeof this[handler] !== "function") {
      console.warn("wsOnMessageAvailable: Handler", handler,
        "whitelisted but not implemented");
      return;
    }

    this[handler](reply);
  },

  /**
   * The websocket should never be closed. Since we don't call ws.close(),
   * _wsOnStop() receives error code NS_BASE_STREAM_CLOSED (see comment in that
   * function), which calls reconnect and re-establishes the WebSocket
   * connection.
   *
   * If the server said it'll use UDP for wakeup, we set _willBeWokenUpByUDP
   * and stop reconnecting in _wsOnStop().
   */
  _wsOnServerClose: function(context, aStatusCode, aReason) {
    console.debug("wsOnServerClose()", aStatusCode, aReason);

    // Switch over to UDP.
    if (aStatusCode == kUDP_WAKEUP_WS_STATUS_CODE) {
      console.debug("wsOnServerClose: Server closed with promise to wake up");
      this._willBeWokenUpByUDP = true;
      // TODO: there should be no pending requests
    }
  },

  /**
   * Rejects all pending register requests with errors.
   */
  _cancelRegisterRequests: function() {
    for (let request of this._registerRequests.values()) {
      request.reject(new Error("Register request aborted"));
    }
    this._registerRequests.clear();
  },

  _makeUDPSocket: function() {
    return Cc["@mozilla.org/network/udp-socket;1"]
             .createInstance(Ci.nsIUDPSocket);
  },

  /**
   * This method should be called only if the device is on a mobile network!
   */
  _listenForUDPWakeup: function() {
    console.debug("listenForUDPWakeup()");

    if (this._udpServer) {
      console.warn("listenForUDPWakeup: UDP Server already running");
      return;
    }

    if (!this._udpWakeUpEnabled) {
      console.debug("listenForUDPWakeup: UDP support disabled");
      return;
    }

    let socket = this._makeUDPSocket();
    if (!socket) {
      return;
    }
    this._udpServer = socket.QueryInterface(Ci.nsIUDPSocket);
    this._udpServer.init(-1, false, Services.scriptSecurityManager.getSystemPrincipal());
    this._udpServer.asyncListen(this);
    console.debug("listenForUDPWakeup: Listening on", this._udpServer.port);

    return this._udpServer.port;
  },

  /**
   * Called by UDP Server Socket. As soon as a ping is received via UDP,
   * reconnect the WebSocket and get the actual data.
   */
  onPacketReceived: function(aServ, aMessage) {
    console.debug("onPacketReceived: Recv UDP datagram on port",
      this._udpServer.port);
    this._beginWSSetup();
  },

  /**
   * Called by UDP Server Socket if the socket was closed for some reason.
   *
   * If this happens, we reconnect the WebSocket to not miss out on
   * notifications.
   */
  onStopListening: function(aServ, aStatus) {
    console.debug("onStopListening: UDP Server socket was shutdown. Status",
      aStatus);
    this._udpServer = undefined;
    this._beginWSSetup();
  },
};

var PushNetworkInfo = {
  /**
   * Returns information about MCC-MNC and the IP of the current connection.
   */
  getNetworkInformation: function() {
    console.debug("PushNetworkInfo: getNetworkInformation()");

    try {
      let nm = Cc["@mozilla.org/network/manager;1"]
                 .getService(Ci.nsINetworkManager);
      if (nm.activeNetworkInfo &&
          nm.activeNetworkInfo.type == Ci.nsINetworkInfo.NETWORK_TYPE_MOBILE) {
        let iccService = Cc["@mozilla.org/icc/iccservice;1"]
                           .getService(Ci.nsIIccService);
        // TODO: Bug 927721 - PushService for multi-sim
        // In Multi-sim, there is more than one client in iccService. Each
        // client represents a icc handle. To maintain backward compatibility
        // with single sim, we always use client 0 for now. Adding support
        // for multiple sim will be addressed in bug 927721, if needed.
        let clientId = 0;
        let icc = iccService.getIccByServiceId(clientId);
        let iccInfo = icc && icc.iccInfo;
        if (iccInfo) {
          console.debug("getNetworkInformation: Running on mobile data");

          let ips = {};
          let prefixLengths = {};
          nm.activeNetworkInfo.getAddresses(ips, prefixLengths);

          return {
            mcc: iccInfo.mcc,
            mnc: iccInfo.mnc,
            ip:  ips.value[0]
          };
        }
      }
    } catch (e) {
      console.error("getNetworkInformation: Error recovering mobile network",
        "information", e);
    }

    console.debug("getNetworkInformation: Running on wifi");
    return {
      mcc: 0,
      mnc: 0,
      ip: undefined
    };
  },

  /**
   * Get mobile network information to decide if the client is capable of being
   * woken up by UDP (which currently just means having an mcc and mnc along
   * with an IP, and optionally a netid).
   */
  getNetworkState: function(callback) {
    console.debug("PushNetworkInfo: getNetworkState()");

    if (typeof callback !== 'function') {
      throw new Error("No callback method. Aborting push agent !");
    }

    var networkInfo = this.getNetworkInformation();

    if (networkInfo.ip) {
      this._getMobileNetworkId(networkInfo, function(netid) {
        console.debug("getNetworkState: Recovered netID", netid);
        callback({
          mcc: networkInfo.mcc,
          mnc: networkInfo.mnc,
          ip:  networkInfo.ip,
          netid: netid
        });
      });
    } else {
      callback(networkInfo);
    }
  },

  /*
   * Get the mobile network ID (netid)
   *
   * @param networkInfo
   *        Network information object { mcc, mnc, ip, port }
   * @param callback
   *        Callback function to invoke with the netid or null if not found
   */
  _getMobileNetworkId: function(networkInfo, callback) {
    console.debug("PushNetworkInfo: getMobileNetworkId()");
    if (typeof callback !== 'function') {
      return;
    }

    function queryDNSForDomain(domain) {
      console.debug("queryDNSForDomain: Querying DNS for", domain);
      let netIDDNSListener = {
        onLookupComplete: function(aRequest, aRecord, aStatus) {
          if (aRecord) {
            let netid = aRecord.getNextAddrAsString();
            console.debug("queryDNSForDomain: NetID found", netid);
            callback(netid);
          } else {
            console.debug("queryDNSForDomain: NetID not found");
            callback(null);
          }
        }
      };
      gDNSService.asyncResolve(domain, 0, netIDDNSListener,
        threadManager.currentThread);
      return [];
    }

    console.debug("getMobileNetworkId: Getting mobile network ID");

    let netidAddress = "wakeup.mnc" + ("00" + networkInfo.mnc).slice(-3) +
      ".mcc" + ("00" + networkInfo.mcc).slice(-3) + ".3gppnetwork.org";
    queryDNSForDomain(netidAddress, callback);
  }
};

function PushRecordWebSocket(record) {
  PushRecord.call(this, record);
  this.channelID = record.channelID;
  this.version = record.version;
}

PushRecordWebSocket.prototype = Object.create(PushRecord.prototype, {
  keyID: {
    get() {
      return this.channelID;
    },
  },
});

PushRecordWebSocket.prototype.toSubscription = function() {
  let subscription = PushRecord.prototype.toSubscription.call(this);
  subscription.version = this.version;
  return subscription;
};
