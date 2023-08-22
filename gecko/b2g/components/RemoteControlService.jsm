/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

/*
 * RemoteControlService.jsm is the entry point of remote control function.
 * The service initializes a TLS socket server which receives events from user.
 *
 *               RemoteControlService <-- Gecko Preference
 *
 *     user -->  nsITLSSocketServer --> script (gecko)
 *
 * Event handler will have these status with JPAKE authentication:
 *   INITIAL->ROUND_1->ROUND_2->VERIFY_KEY->COMMAND
 * In each status, event handler only accepts correct events from client.
 * If client sends wrong event, event handler will send back error message, then disconnect.
 *
 * INITIAL: wait for handshake event
 * ROUND_1: wait for client JPAKE round 1 result
 * ROUND_2: wait for client JPAKE round 2 result
 * VERIFY_KEY: wait for client's hash of AES key
 * COMMAND: wait for control command
 *
 * Events from user are in JSON format. After they are parsed into control command,
 * these events are passed to script (js), run in sandbox,
 * and dispatch corresponding events to Gecko.
 *
 * Here is related component location:
 * gecko/b2g/components/RemoteControlService.jsm
 * gecko/b2g/chrome/content/remote_command.js
 *
 * For more details, please visit: https://wiki.mozilla.org/Firefox_OS/Remote_Control
 */

"use strict";

this.EXPORTED_SYMBOLS = ["RemoteControlService"];

const { classes: Cc, interfaces: Ci, results: Cr, utils: Cu, Constructor: CC } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "certService", "@mozilla.org/security/local-cert-service;1",
                                   "nsILocalCertService");
XPCOMUtils.defineLazyServiceGetter(this, "UUIDGenerator", "@mozilla.org/uuid-generator;1",
                                   "nsIUUIDGenerator");

XPCOMUtils.defineLazyModuleGetter(this, "SystemAppProxy", "resource://gre/modules/SystemAppProxy.jsm");

const ScriptableInputStream = CC("@mozilla.org/scriptableinputstream;1",
                                 "nsIScriptableInputStream", "init");

// static functions
function debug(aStr) {
  dump("RemoteControlService: " + aStr + "\n");
}

const DEBUG = false;

const MAX_CLIENT_CONNECTIONS = 5; // Allow max 5 clients to use remote control TV
const REMOTECONTROL_PREF_MDNS_SERVICE_TYPE = "_remotecontrol._tcp";
const REMOTECONTROL_PREF_MDNS_SERVICE_NAME = "dom.presentation.device.name";
const REMOTECONTROL_PREF_COMMANDJS_PATH = "chrome://b2g/content/remote_command.js";
const REMOTECONTROL_PREF_DEVICES = "remotecontrol.paired_devices";
const REMOTE_CONTROL_EVENT = "mozChromeRemoteControlEvent";

const PINCODE_ERROR_LIMIT = 5;
const PINCODE_LENGTH = 4;
const SERVICE_CERT = "RemoteControlService";
const TV_SIGNER_ID = "server";
const CLIENT_SIGNER_ID = "client";
// Extra input to SHA256-HMAC in generate entry, includes the full crypto spec.
const HMAC_INPUT ="AES_256_CBC-HMAC256";

const SERVER_STATUS = {
  STOPPED: 0,
  STARTED: 1
};

const EVENT_HANDLER_STATUS = {
  INITIAL: 0,
  ROUND_1: 1,
  ROUND_2: 2,
  VERIFY_KEY: 3,
  COMMAND: 4,
};

let nextConnectionId = 0; // Used for tracking existing connections
let commandJSSandbox = null; // Sandbox runs remote_command.js

this.RemoteControlService = {
  // Remote Control status
  _serverStatus: SERVER_STATUS.STOPPED,

  // TLS socket server
  _port: -1, // The port on which this service listens
  _serverSocket: null, // The server socket associated with this
  _connections: new Map(), // Hash of all open connections, indexed by connection Id
  _mDNSRegistrationHandle: null, // For cancel mDNS registration
  _certFingerprint: null, // For JPAKE secret

  // remote_command.js
  exportFunctions: {}, // Functions export to remote_command.js
  _sharedState: {}, // Shared state storage between connections
  _isCursorMode: false, // Store SystemApp isCursorMode

  // JPAKE pairing
  _pin: null,
  _pairedDevices: {},

  // PUBLIC API
  // Start TLS socket server.
  // Return a promise for start() resolves/reject to
  start: function() {
    if (this._serverStatus == SERVER_STATUS.STARTED) {
      return Promise.reject("AlreadyStarted");
    }

    let promise = new Promise((aResolve, aReject) => {
      this._doStart(aResolve, aReject);
    });
    return promise;
  },

  // Stop TLS socket server, remove registered observer
  // Cancel mDNS registration
  // Return false if server not started, stop failed.
  stop: function() {
    if (this._serverStatus == SERVER_STATUS.STOPPED) {
      return false;
    }

    if (!this._serverSocket) {
      return false;
    }

    DEBUG && debug("Stop listening on port " + this._serverSocket.port);

    SystemAppProxy.removeEventListener("mozContentEvent", this);
    Services.obs.removeObserver(this, "xpcom-shutdown");

    if (this._mDNSRegistrationHandle) {
      this._mDNSRegistrationHandle.cancel(Cr.NS_OK);
      this._mDNSRegistrationHandle = null;
    }

    commandJSSandbox = null;
    this._port = -1;
    this._serverSocket.close();
    this._serverSocket = null;
    this._certFingerprint = null;
    this._serverStatus = SERVER_STATUS.STOPPED;

    return true;
  },

  // Generate PIN code for pairing, format is 4 digits
  // 0000 and random generated number to a new string
  // Then get last 4 characters as new PIN code
  // Return -1 if Web Cryptography API has more than 5 error
  generatePIN: function() {
    let count = 0;
    do {
      try {
        // Generate a random number in [0, 2^16 - 1]
        let random = new Uint16Array(1);
        let crypto = Services.wm.getMostRecentWindow("navigator:browser").crypto;
        crypto.getRandomValues(random);
        // Get last needed digits of the random number
        this._pin = random[0] % Math.pow(10, PINCODE_LENGTH);
      } catch(e) {
        debug("Fail to get random from Web Cryptography API: " + e);
        this._pin = null;
        if(++count >= PINCODE_ERROR_LIMIT) {
          return -1;
        }
      }
    } while(!this._pin); // Make sure this._pin exists and it's not zero

    let padding = new Array(PINCODE_LENGTH + 1).join('0');
    this._pin = (padding + this._pin).slice(-1 * PINCODE_LENGTH);

    return this._pin;
  },

  // Get current PIN
  getPIN: function() {
    return this._pin;
  },

  // Clean current PIN code
  clearPIN: function() {
    this._pin = null;
  },

  // Return first 12 characters of server certifications
  getCertFingerprint: function() {
    return this._certFingerprint;
  },

  generateUUID: function(aAES256Key, aHMAC256Key) {
    let uuidString = UUIDGenerator.generateUUID().toString();

    this._pairedDevices[uuidString] = {
      aesKey: aAES256Key,
      hmacKey: aHMAC256Key,
    };

    this._flushPairedDevices();

    return uuidString;
  },

  getKeysFromUUID: function(aUUID) {
    if(aUUID in this._pairedDevices) {
      return this._pairedDevices[aUUID];
    }

    return null;
  },

  updateUUID: function(aUUID, aAES256Key, aHMAC256Key) {
    if(aUUID in this._pairedDevices) {
      this._pairedDevices[aUUID] = {
        aesKey: aAES256Key,
        hmacKey: aHMAC256Key,
      };
      this._flushPairedDevices();
    }
  },

  // Observers and Listeners
  // nsIObserver
  observe: function(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "xpcom-shutdown": {
        // Stop service when xpcom-shutdown
        this.stop();
        break;
      }
    }
  },

  // SystemAppProxy event listener
  handleEvent: function(evt) {
    if (evt.type !== "mozContentEvent") {
      return;
    }

    let detail = evt.detail;
    if (!detail) {
      return;
    }

    switch (detail.type) {
      case "control-mode-changed":
        // Use mozContentEvent to receive control mode of current app from System App
        // remote_command.js use "getIsCursorMode" to determine what kind event should dispatch to app
        // Add null check when system app initialization, cursor is null
        let isCursorMode = (detail.detail.cursor === null)?false:detail.detail.cursor;
        if (isCursorMode !== this._isCursorMode) {
          this._isCursorMode = isCursorMode;
          // Update mouse cursor when control mode changed
          this.updateMouseCursor();
        }
        break;
      case "remote-control-pin-dismissed":
        this.clearPIN();
        // Notify all event handler PIN dismiss
        this._connections.forEach(function(aConnection){
          aConnection.eventHandler.handlePINDismissed();
        });
        break;
   }
  },

  // nsIServerSocketListener
  onSocketAccepted: function(aSocket, aTrans) {
    DEBUG && debug("onSocketAccepted(aSocket=" + aSocket + ", aTrans=" + aTrans + ")");
    DEBUG && debug("New connection on " + aTrans.host + ":" + aTrans.port);

    if (this._connections.size >= MAX_CLIENT_CONNECTIONS) {
      DEBUG && debug("Reach max " + MAX_CLIENT_CONNECTIONS +
                     "connections already, drop the new one.");
      aTrans.close(Cr.NS_BINDING_ABORTED);
      return;
    }

    const SEGMENT_SIZE = 8192;
    const SEGMENT_COUNT = 1024;

    try {
      var input = aTrans.openInputStream(0, SEGMENT_SIZE, SEGMENT_COUNT)
                        .QueryInterface(Ci.nsIAsyncInputStream);
      var output = aTrans.openOutputStream(0, 0, 0);
    } catch (e) {
      DEBUG && debug("Error opening transport streams: " + e);
      aTrans.close(Cr.NS_BINDING_ABORTED);
      return;
    }

    let connectionId = ++nextConnectionId;

    try {
      // Create a connection for each user connection
      // EventHandler implements nsIInputStreamCallback for incoming message from user
      var conn = new Connection(input, output, this, connectionId);
      let handler = new EventHandler(conn);

      // Update mouse cursor when establish a new connection
      this.updateMouseCursor();

      input.asyncWait(conn, 0, 0, Services.tm.mainThread);
    } catch (e) {
      DEBUG && debug("Error in initial connection: " + e);
      trans.close(Cr.NS_BINDING_ABORTED);
      return;
    }

    this._connections.set(connectionId, conn);
    DEBUG && debug("Start connection " + connectionId);
  },

  // Close all connection when socket closed
  onStopListening: function(aSocket, aStatus) {
    DEBUG && debug("Shut down server on port " + aSocket.port);

    this._connections.forEach(function(aConnection){
      aConnection.close();
    });
  },

  updateMouseCursor: function() {
    let window = Services.wm.getMostRecentWindow("navigator:browser");
    let utils = window.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils);
    let x = isNaN(parseInt(this._getSharedState("x"))) ? 0 : parseInt(this._getSharedState("x"));
    let y = isNaN(parseInt(this._getSharedState("y"))) ? 0 : parseInt(this._getSharedState("y"));

    if (this._isCursorMode && this._connections.size > 0) {
      // Enable mouse cursor if it's in cursor mode and there is user connected.
      utils.sendMouseEvent("MozEnableDrawCursor", x, y, 0, 0, 0);
    } else {
      // Disable mouse cursor
      utils.sendMouseEvent("MozDisableDrawCursor", x, y, 0, 0, 0);
    }
  },

#ifdef MOZ_WIDGET_GONK
  // Refresh the certificate when the time is updated.
  // The getOrCreateCert will validate the certificate itself.
  // When current time > cert's expired day + one day
  // or the current time < cert's started day, the certificate is invalid.
  // Otherwise we can continue to use the old valid one.
  onTimeChange: function(aTimestamp) {
    DEBUG && debug("time is updated to: " + new Date(aTimestamp).toString());

    // Do nothing if the service doesn't be started
    if (this._serverStatus == SERVER_STATUS.STOPPED) {
      return;
    }

    let self = this;
    certService.getOrCreateCert(SERVICE_CERT, {
      handleCert: function(cert, result) {
        if(result) {
          DEBUG && debug("Fail to get service certificate");
        } else if(cert.sha256Fingerprint != self._certFingerprint) {
          // If the certificate needs to be updated, then restart the service
          self.stop();
          self.start();
        }
      }
    });
  },
#endif

  // PRIVATE FUNCTIONS
  _doStart: function(aResolve, aReject) {
    DEBUG && debug("doStart");

    if (this._serverSocket) {
      aReject("SocketAlreadyInit");
      return;
    }

    // Monitor xpcom-shutdown to stop service and clean up
    Services.obs.addObserver(this, "xpcom-shutdown", false);

    // Listen control mode change from gaia, request when service starts
    SystemAppProxy.addEventListener("mozContentEvent", this);
    SystemAppProxy._sendCustomEvent(REMOTE_CONTROL_EVENT, {action: "request-control-mode"});

    // Read already stored devices
    try {
      this._pairedDevices = JSON.parse(Services.prefs.getCharPref(REMOTECONTROL_PREF_DEVICES));
    } catch(e) {
      DEBUG && debug ("Parse stored devices error: " + e);

      // Empty paired devices
      this._pairedDevices = {};
      this._flushPairedDevices();
    }

    // Internal functions export to remote_command.js
    this.exportFunctions = {
      "getSharedState": this._getSharedState,
      "setSharedState": this._setSharedState,
      "getIsCursorMode": this._getIsCursorMode,
    }

    let self = this;
    this._getCertificate().then(function(aCertificate){
      // Store server certificate to synthesize PIN for JPAKE round2
      self._certFingerprint = aCertificate.sha256Fingerprint;

      if (!self._createSocket(aCertificate)) {
        aReject("Start TLSSocketServer fail");
        return;
      }

      let registrationListener = {
        onServiceRegistered: function() {
          DEBUG && debug ("Register service on port: " + self._port + " successfully");
          self._serverStatus = SERVER_STATUS.STARTED;
          aResolve();
        },
        onRegistrationFailed: function() {
          DEBUG && debug ("Fail to register service");
          aReject("Fail to register service");
        },
      };

      self._registerService(registrationListener);
    }).catch(function(aError) {
      aReject(aError);
      return;
    });
  },

  // Get certificate to establish TLS channel
  _getCertificate: function() {
    return new Promise(function(aResolve, aReject) {
      // Start TLSSocketServer with self-signed certification
      // If there is no module use PSM before, handleCert result is SEC_ERROR_NO_MODULE (0x805A1FC0).
      // Get PSM here ensure certService.getOrCreateCert works properly.
      Cc["@mozilla.org/psm;1"].getService(Ci.nsISupports);

#ifdef MOZ_WIDGET_GONK
      // The remote-control service will be started on booting up. However,
      // the Gonk platform can't get the correct time at that moment.
      // If the time is incorrect, then neither is the period of the certificate.
      // At the first time running of this service, we create a new certificate,
      // then we validate it when receiving time-changed event
      // (The time should be correct). If the period is invalid, we will remove
      // the outdated certificate and create a new one, then restart the service.
      // At the following runnings, we directly use the previous valid certificate.
      // The validation will still be checked on time-changed callback.

      // Try toget certificate from x509cert DB first.
      // If the certificate doesn't exist, then create one.
      try {
        let certDB = Cc["@mozilla.org/security/x509certdb;1"]
                     .getService(Ci.nsIX509CertDB);
        let cert = certDB.findCertByNickname(SERVICE_CERT);
        DEBUG && debug("Get the existing certificate");
        aResolve(cert);
        return;
      } catch(e) {
        // Find nothing, fall through to create a new certificate
      }
#endif
      certService.getOrCreateCert(SERVICE_CERT, {
        handleCert: function(cert, result) {
          if(result) {
            aReject("getOrCreateCert " + result);
          } else {
            DEBUG && debug("Create a new certificate");
            aResolve(cert);
          }
        }
      });
    });
  },

  // Try to get random port
  _createSocket: function(aCertificate) {
    try {
      let ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
      let socket;
      for (let i = 100; i; i--) {
        let temp = Cc["@mozilla.org/network/tls-server-socket;1"].createInstance(Ci.nsITLSServerSocket);
        temp.init(this._port, false, MAX_CLIENT_CONNECTIONS);
        temp.serverCert = aCertificate;

        let allowed = ios.allowPort(temp.port, "tls");
        if (!allowed) {
          DEBUG && debug("Warning: obtained TLSServerSocket listens on a blocked port: " + temp.port);
        }

        if (!allowed && this._port == -1) {
          DEBUG && debug("Throw away TLSServerSocket with bad port.");
          temp.close();
          continue;
        }

        socket = temp;
        break;
      }

      if (!socket) {
        throw new Error("No socket server available. Are there no available ports?");
      }

      DEBUG && debug("Listen on port " + socket.port + ", " + MAX_CLIENT_CONNECTIONS + " pending connections");

      socket.serverCert = aCertificate;
      // Set session cache and tickets to false here.
      // Cache disconnects fennect addon when addon sends message
      // Tickets crashes b2g when fennec addon connects
      socket.setSessionCache(false);
      socket.setSessionTickets(false);
      socket.setRequestClientCertificate(Ci.nsITLSServerSocket.REQUEST_NEVER);

      socket.asyncListen(this);
      this._port = socket.port;
      this._serverSocket = socket;
    } catch (e) {
      DEBUG && debug("Could not start server on port " + this._port + ": " + e);
      return false;
    }

    return true;
  },

  // Register mDNS remote control service with this._port
  _registerService: function(aListener) {
    if (("@mozilla.org/toolkit/components/mdnsresponder/dns-sd;1" in Cc)) {
      let serviceInfo = Cc["@mozilla.org/toolkit/components/mdnsresponder/dns-info;1"]
                          .createInstance(Ci.nsIDNSServiceInfo);
      serviceInfo.serviceType = REMOTECONTROL_PREF_MDNS_SERVICE_TYPE;
      serviceInfo.serviceName = Services.prefs.getCharPref(REMOTECONTROL_PREF_MDNS_SERVICE_NAME);
      serviceInfo.port = this._port;

      let mdns = Cc["@mozilla.org/toolkit/components/mdnsresponder/dns-sd;1"]
                   .getService(Ci.nsIDNSServiceDiscovery);
      this._mDNSRegistrationHandle = mdns.registerService(serviceInfo, aListener);
    }
  },

  // Notifies this server that the given connection has been closed.
  _connectionClosed: function(aConnectionId) {
    DEBUG && debug("Close connection " + aConnectionId);
    this._connections.delete(aConnectionId);
  },

  // Get the value corresponding to a given key for remote_command.js state preservation
  _getSharedState: function(aKey) {
    if (aKey in RemoteControlService._sharedState) {
      return RemoteControlService._sharedState[aKey];
    }
    return "";
  },

  // Set the value corresponding to a given key for remote_command.js state preservation
  _setSharedState: function(aKey, aValue) {
    if (typeof aValue !== "string") {
      throw new Error("non-string value passed");
    }
    RemoteControlService._sharedState[aKey] = aValue;
  },

  _getIsCursorMode: function() {
    return RemoteControlService._isCursorMode;
  },

  _flushPairedDevices: function() {
    let data = JSON.stringify(this._pairedDevices);
    Services.prefs.setCharPref(REMOTECONTROL_PREF_DEVICES, data);
    // This preference is consulted during startup.
    Services.prefs.savePrefFile(null);
  },
};

function streamClosed(aException) {
  return aException === Cr.NS_BASE_STREAM_CLOSED ||
         (typeof aException === "object" && aException.result === Cr.NS_BASE_STREAM_CLOSED);
}

// Represents a connection to the server
function Connection(aInput, aOutput, aServer, aConnectionId) {
  DEBUG && debug("Open a new connection " + aConnectionId);

  // Server associated with this connection
  this.server = aServer;

  // Id of this connection
  this.connectionId = aConnectionId;

  this.eventHandler = null;

  // Input and output UTF-8 stream
  this._input = Cc["@mozilla.org/intl/converter-input-stream;1"]
                  .createInstance(Ci.nsIConverterInputStream);
  this._input.init(aInput, "UTF-8", 0,
                   Ci.nsIConverterInputStream.DEFAULT_REPLACEMENT_CHARACTER);

  this._output = Cc["@mozilla.org/intl/converter-output-stream;1"]
                   .createInstance(Ci.nsIConverterOutputStream);
  this._output.init(aOutput, "UTF-8", 0, 0x0000);

  // This allows a connection to disambiguate between a peer initiating a
  // close and the socket being forced closed on shutdown.
  this._closed = false;
}
Connection.prototype = {
  // Closes this connection's input/output streams
  close: function() {
    if (this._closed) {
      return;
    }

    DEBUG && debug("Close connection " + this.connectionId);

    this._input.close();
    this._output.close();
    this._closed = true;

    this.server._connectionClosed(this.connectionId);
    // Update mouse cursor when connection closed
    this.server.updateMouseCursor();
  },

  sendMsg: function(aMessage) {
    this._output.writeString(JSON.stringify(aMessage));
  },

  // nsIInputStreamCallback
  onInputStreamReady: function(aInput) {
    DEBUG && debug("onInputStreamReady(aInput=" + aInput + ") on thread " +
                   Services.tm.currentThread + " (main is " +
                   Services.tm.mainThread + ")");

    try {
      let available = 0, numChars = 0, fullMessage = "";

      // Read and concat messages from input stream buffer
      do {
        let partialMessage = {};

        available = aInput.available();
        numChars = this._input.readString(available, partialMessage);

        fullMessage += partialMessage.value;
      } while(numChars < available);

      if (fullMessage.length > 0) { // While readString contains something
        // Handle incoming JSON string
        let sanitizedMessage = this._sanitizeMessage(fullMessage);

        if (sanitizedMessage.length > 0) {
          try {
            // Parse JSON string to event objects
            let events = JSON.parse(sanitizedMessage);

            events.forEach((event) => {
              if (this.eventHandler !== null) {
                this.eventHandler.handleEvent(event);
              }
            });
          } catch (e) {
            DEBUG && debug ("Parse event error, drop this message, error: " + e);
          }
        }
      }
    } catch (e) {
      if (streamClosed(e)) {
        DEBUG && debug("WARNING: unexpected error when reading from socket; will " +
                       "be treated as if the input stream had been closed");
        DEBUG && debug("WARNING: actual error was: " + e);
      }

      // Input has been closed, but we're still expecting to read more data.
      // available() will throw in this case, destroy the connection.
      DEBUG && debug("onInputStreamReady called on a closed input, destroying connection, error: " + e);
      this.close();
      return;
    }

    // Wait next message
    aInput.asyncWait(this, 0, 0, Services.tm.currentThread);
  },

  _sanitizeMessage: function(aMessage) {
    // If message doesn't start with "{", discard the part before first found "}{"
    if(!aMessage.startsWith("{")) {
      if (aMessage.indexOf("}{") >= 0) {
        aMessage = aMessage.slice(aMessage.indexOf("}{")+1);
      } else {
        // If "}{" not found, discard partial JSON string and return empty string
        return "";
      }
    }

    // If message doesn't end with "}", discard the part after last found "}{"
    if(!aMessage.endsWith("}")) {
      if (aMessage.lastIndexOf("}{") >= 0) {
        aMessage = aMessage.slice(0, aMessage.lastIndexOf("}{")+1);
      } else {
        // If "}{" not found, discard partial JSON string and return empty string
        return "";
      }
    }

    // Add "[]" to handle concatenated message
    return '[' + aMessage.replace(/}{/g, '},{') + ']';
  },
};

// For AES/HMAC key base64 string from JPAKE
// Use ArrayBuffer in Web Crypto (like import key)
function base64FromArrayBuffer(aArrayBuffer) {
  let binary = '';
  let bytes = new Uint8Array(aArrayBuffer);
  let len = bytes.byteLength;
  for (var i = 0; i < len; i++) {
    binary += String.fromCharCode(bytes[i])
  }
  return btoa(binary);
}

function base64ToArrayBuffer(abase64) {
  let binary_string = atob(abase64);
  let len = binary_string.length;
  let bytes = new Uint8Array(len);
  for (var i = 0; i < len; i++) {
    bytes[i] = binary_string.charCodeAt(i);
  }
  return bytes.buffer;
}

// Synthesize the PIN for JPAKE round 2
// The SHA256 is used to make sure the data string is in certain ranges
// Return binary data
function SHA256(aStr) {
  // data is an array of bytes
  let data = bytesFromString(aStr);
  let cryptoHash = Components.classes["@mozilla.org/security/hash;1"]
                   .createInstance(Components.interfaces.nsICryptoHash);
  // Use the SHA256 algorithm
  cryptoHash.init(cryptoHash.SHA256);
  cryptoHash.update(data, data.length);
  // Pass false here to get binary data back
  return cryptoHash.finish(false);
}

function bytesFromString(aStr) {
  let converter =
        Components.classes["@mozilla.org/intl/scriptableunicodeconverter"]
        .createInstance(Components.interfaces.nsIScriptableUnicodeConverter);
  converter.charset = "UTF-8";
  return converter.convertToByteArray(aStr);
}

// Load remote_command.js to commandJSSandbox when receives command event
// Release sandbox in RemoteControlService.stop()
function getCommandJSSandbox(aImportFunctions) {
  if (commandJSSandbox == null) {
    try {
      let channel = Services.io.newChannel2(REMOTECONTROL_PREF_COMMANDJS_PATH, null, null,
                                            null, Services.scriptSecurityManager.getSystemPrincipal(),
                                            null, Ci.nsILoadInfo.SEC_NORMAL, Ci.nsIContentPolicy.TYPE_OTHER);
      var fis = channel.open();
      let sis = new ScriptableInputStream(fis);
      commandJSSandbox = Cu.Sandbox(Cc["@mozilla.org/systemprincipal;1"].createInstance(Ci.nsIPrincipal));

      // Import function registered from external
      for(let functionName in aImportFunctions) {
        commandJSSandbox.importFunction(aImportFunctions[functionName], functionName);
      }

      try {
        // Evaluate remote_command.js in sandbox
        Cu.evalInSandbox(sis.read(fis.available()), commandJSSandbox, "latest");
      } catch (e) {
        DEBUG && debug("Syntax error in remote_command.js at " + channel.URI.path + ": " + e);
      }
    } catch(e) {
      DEBUG && debug("Error initializing sandbox: " + e);
    } finally {
      fis.close();
    }
  }

  return commandJSSandbox;
}

// Parse and dispatch incoming events from client
function EventHandler(aConnection) {
  this._connection = aConnection;
  this._server = aConnection.server;

  aConnection.eventHandler = this;

  this._status = EVENT_HANDLER_STATUS.INITIAL;

  this._clientID = null;
  this._aes256B64 = {};
  this._hmac256B64 = {};
  this._hmac256Key = null;

  this._JPAKE = null;
  // For round 1
  this._gx1 = {};
  this._gv1 = {};
  this._r1 = {};
  this._gx2 = {};
  this._gv2 = {};
  this._r2 = {};
  // For round 2
  this._A = {};
  this._gva = {};
  this._ra = {};

  this._client_round1 = null;
  this._client_round2 = null;
}
EventHandler.prototype = {
  // PUBLIC FUNCTIONS
  handleEvent: function(aEvent) {
    switch (aEvent.type) {
      case "auth":
        // Implement JPAKE authentication
        this._handleAuthEvent(aEvent);
        break;
      case "command":
        // Implement control command dispatch
        this._handleCommandEvent(aEvent);
        break;
      case "ack":
        // Implement ack for connection status detection
        this._handleAckEvent(aEvent);
        break;
      default:
        // Not accepted event type, report error to client
        this._handleError(this, "common", "Not accepted event type");
        break;
    }
  },

  // Notify from system app PIN code is dismissed
  handlePINDismissed: function() {
    // If event handler's status is in ROUND_2 and first connection (no clientID),
    // it means user is typing PIN code for JPAKE round 2.
    // Old PIN code is invalid now, send PIN expire error and trigger client reconnection.
    if(this._status === EVENT_HANDLER_STATUS.ROUND_2 && this._clientID === null) {
      DEBUG && debug("Receive PIN dimiss from system app");
      this._handleError(this, "auth", "PIN expire");
    }
  },

  _handleAuthEvent: function(aEvent) {
    switch (aEvent.action) {
      case "request_handshake":
        this._handleHandshake(aEvent);
        break;
      case "jpake_client_1":
        this._handleJPAKERound1Event(aEvent);
        break;
      case "jpake_client_2":
        this._handleJPAKERound2Event(aEvent);
        break;
      case "client_key_confirmation":
        this._handleKeyConfirmEvent(aEvent);
        break;
      default:
        break;
    }
  },

  _handleHandshake: function(aEvent) {
    if (this._status != EVENT_HANDLER_STATUS.INITIAL) {
      DEBUG && debug("Status not in INITIAL, drop the event");
      this._handleError(this, aEvent.type, "Wrong status");
      return;
    }

    let reply = {
      type: "auth",
      action: "response_handshake",
      detail: 1
    };

    // If client sends an existing ID, then reply is 2 (second connection)
    // Also restore AES and HMAC key from UUID
    if (aEvent.detail && aEvent.detail.id && this._server.getKeysFromUUID(aEvent.detail.id) !== null) {
      reply.detail = 2;

      this._clientID = aEvent.detail.id;
      let keys = this._server.getKeysFromUUID(aEvent.detail.id);
      this._aes256B64 = { value: keys.aesKey };
      this._hmac256B64 = { value: keys.hmacKey };
    }

    this._connection.sendMsg(reply);
    this._status = EVENT_HANDLER_STATUS.ROUND_1;
  },

  _handleJPAKERound1Event: function(aEvent) {
    if (this._status != EVENT_HANDLER_STATUS.ROUND_1) {
      DEBUG && debug("Status not in ROUND_1, drop the event");
      this._handleError(this, aEvent.type, "Wrong status");
      return;
    }

    // First time connection, generate PIN, trigger notification on screen
    if (this._clientID === null) {
      let pin = this._server.getPIN();
      if(pin === null) {
        pin = this._server.generatePIN();
        if (pin != -1) {
          // Show notification on screen
          SystemAppProxy._sendCustomEvent(REMOTE_CONTROL_EVENT, { pincode: pin, action: 'pin-created' });
        } else {
          // Notify the client that we can't generate a secure random PIN
          DEBUG && debug("Unable to generate a secure random PIN");
          this._handleError(this, aEvent.type, "No secure PIN");
          return;
        }
      }
    }

    // Store client round 1
    this._client_round1 = aEvent.detail;

    // Compute server round 1
    this._JPAKE = Cc["@mozilla.org/services-crypto/sync-jpake;1"]
                    .createInstance(Ci.nsISyncJPAKE);
    try {
      this._JPAKE.round1(TV_SIGNER_ID, this._gx1, this._gv1, this._r1,
                         this._gx2, this._gv2, this._r2);
    } catch (e) {
      DEBUG && debug("JPAKE round1 get error: " + e);
      this._handleError(this, aEvent.type, "JPAKE error: " + e.message);
      return;
    }

    // Send round_1 result
    let reply = {
      type: "auth",
      action: "jpake_server_1",
      detail: {
        gx1: this._gx1.value,
        gx2: this._gx2.value,
        zkp_x1: {gr: this._gv1.value, b: this._r1.value, id: TV_SIGNER_ID},
        zkp_x2: {gr: this._gv2.value, b: this._r2.value, id: TV_SIGNER_ID}
      }
    };

    this._connection.sendMsg(reply);
    this._status = EVENT_HANDLER_STATUS.ROUND_2;
  },

  _handleJPAKERound2Event: function(aEvent) {
    if (this._status != EVENT_HANDLER_STATUS.ROUND_2) {
      DEBUG && debug("Status not in ROUND_2, drop the event");
      this._handleError(this, aEvent.type, "Wrong status");
      return;
    }

    // Store client round 2
    this._client_round2 = aEvent.detail;

    // Compute JPAKE round 2
    let pin = null;

    if (this._clientID !== null) {
      // Not first time connection, use previous AES key as PIN code
      pin = this._aes256B64.value;
    } else {
      pin = this._server.getPIN();
      if (pin === null) {
        // Maybe some other client already used the PIN then clean it
        DEBUG && debug("PIN expire");
        this._handleError(this, aEvent.type, "PIN expire");
        return;
      }
    }

    // Synthesize PIN for JPAKE round 2
    let secret = this._synthesizeJPAKEPIN(pin);

    try {
      this._JPAKE.round2(CLIENT_SIGNER_ID, secret,
                         this._client_round1.gx1, this._client_round1.zkp_x1.gr, this._client_round1.zkp_x1.b,
                         this._client_round1.gx2, this._client_round1.zkp_x2.gr, this._client_round1.zkp_x2.b,
                         this._A, this._gva, this._ra);
    } catch (e) {
      DEBUG && debug("JPAKE round2 get error: " + e);
      this._handleError(this, aEvent.type, "JPAKE error: " + e.message);
      return;
    }

    // Dismiss pin
    if (this._clientID === null) {
      this._server.clearPIN();
      SystemAppProxy._sendCustomEvent(REMOTE_CONTROL_EVENT, { action: 'pin-destroyed' });
    }

    // Send round_2 result
    let reply = {
      type: "auth",
      action: "jpake_server_2",
      detail: {
        A: this._A.value,
        zkp_A: { gr: this._gva.value, b: this._ra.value, id: TV_SIGNER_ID }
      }
    };

    this._connection.sendMsg(reply);

    // Compute JPAKE final
    try {
      this._JPAKE.final(this._client_round2.A, this._client_round2.zkp_A.gr, this._client_round2.zkp_A.b,
                        HMAC_INPUT, this._aes256B64, this._hmac256B64);
    } catch (e) {
      DEBUG && debug("JPAKE final get error: " + e);
      this._handleError(this, aEvent.type, "JPAKE error: " + e.message);
      return;
    }

    // Use HMAC key double hash AES key
    let subtle = Services.wm.getMostRecentWindow("navigator:browser").crypto.subtle;
    let self = this;

    // Import HMAC key from base64 string
    subtle.importKey(
      "raw",
      base64ToArrayBuffer(self._hmac256B64.value),
      {
        name: "HMAC",
        hash: { name: "SHA-256" },
      },
      true,
      ["sign", "verify"]
    ).then(function(aHMACKey) {
      self._hmac256Key = aHMACKey;

      // First sign
      subtle.sign(
        {
          name: "HMAC",
        },
        aHMACKey,
        base64ToArrayBuffer(self._aes256B64.value)
      ).then(function(aSignature1) {
        // Second sign
        subtle.sign(
          {
            name: "HMAC",
          },
          aHMACKey,
          aSignature1
        ).then(function(aSignature2) {
          reply = {
            type: "auth",
            action: "server_key_confirm",
            detail: {
              signature: base64FromArrayBuffer(aSignature2)
            }
          };

          self._connection.sendMsg(reply);
          self._status = EVENT_HANDLER_STATUS.VERIFY_KEY;
        }).catch(function(err) {
          DEBUG && debug("Second HMAC sign error: " + err);
          self._handleError(self, aEvent.type, "Second HMAC sign error: " + err);
        });
      }).catch(function(err) {
        DEBUG && debug("First HMAC sign error: " + err);
        self._handleError(self, aEvent.type, "First HMAC sign error: " + err);
      });
    }).catch(function(err) {
      DEBUG && debug("Import HMAC key error: " + err);
      self._handleError(self, aEvent.type, "Import HMAC key error: " + err);
    });
  },

  // Synthesize PIN for J-PAKE round 2 by original PIN
  // and TLS server certificate fingerprint
  _synthesizeJPAKEPIN: function(aPIN) {
    let hash = SHA256(aPIN + this._server.getCertFingerprint());
    // Ignore the highest order bit to make sure this value < 256-bit value
    return String.fromCharCode(hash.charCodeAt(0) & 0x7F) + hash.slice(1);
  },

  _handleKeyConfirmEvent: function(aEvent) {
    if (this._status != EVENT_HANDLER_STATUS.VERIFY_KEY) {
      DEBUG && debug("Status not in VERIFY_KEY, drop the event");
      this._handleError(this, aEvent.type, "Wrong status");
      return;
    }

    // Exam hash of AES key from client
    let subtle = Services.wm.getMostRecentWindow("navigator:browser").crypto.subtle;
    let self = this;

    subtle.verify(
      {
        name: "HMAC",
      },
      this._hmac256Key,
      base64ToArrayBuffer(aEvent.detail.signature),
      base64ToArrayBuffer(this._aes256B64.value)
    ).then(function(isValid) {
      DEBUG && debug("HMAC verify result: " + isValid);

      let reply = {
        type: "auth",
        action: "finish_handshake",
      };

      // Check if we already has client ID, if not, generate UUID and set to reply
      if (self._clientID === null) {
        let uuid = self._server.generateUUID(self._aes256B64.value, self._hmac256B64.value);
        self._clientID = uuid;
        reply.detail = { id: uuid };
      } else {
        // Update client ID with new AES/HMAC key
        self._server.updateUUID(self._clientID, self._aes256B64.value, self._hmac256B64.value);
      }

      // Send back key confirm result, change status to receiving command status
      self._connection.sendMsg(reply);
      self._status = EVENT_HANDLER_STATUS.COMMAND;
    }).catch(function(err) {
      DEBUG && debug("HMAC verify error: " + err);
      self._handleError(self, aEvent.type, "HMAC verify error: " + err);
    });
  },

  _handleCommandEvent: function(aEvent) {
    // If event handler is not ready to receive command, drop this event
    if (this._status != EVENT_HANDLER_STATUS.COMMAND) {
      DEBUG && debug("Status not in COMMAND, drop the event");
      this._handleError(this, aEvent.type, "Wrong status");
      return;
    }

    try {
      let sandbox = getCommandJSSandbox(this._connection.server.exportFunctions);
      sandbox.handleEvent(aEvent);
    } catch (e) {
      DEBUG && debug("Error running remote_command.js :" + e);
    }
  },

  _handleAckEvent: function(aEvent) {
    // If event handler is not ready to receive command, drop this event
    if (this._status != EVENT_HANDLER_STATUS.COMMAND) {
      DEBUG && debug("Status not in COMMAND, drop the event");
      this._handleError(this, aEvent.type, "Wrong status");
      return;
    }

    if (aEvent.detail && aEvent.detail == "ping") {
      let reply = {
        type: aEvent.type,
        detail: "pong"
      };

      this._connection.sendMsg(reply);
    } else {
      this._handleError(this, aEvent.type, "Wrong detail");
    }
  },

  // Once error occurs, send error to client then close connection
  _handleError: function(aHandler, aType, aError) {
    let reply = {
      type: aType,
      error: aError
    };

    aHandler._connection.sendMsg(reply);
    aHandler._connection.close();
  },
};
