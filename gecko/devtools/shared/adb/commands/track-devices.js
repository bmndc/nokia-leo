/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// Wrapper around the ADB utility.

"use strict";
const { Cu } = require("chrome");

const EventEmitter = require("devtools/shared/event-emitter");
const { dumpn } = require("devtools/shared/DevToolsUtils");
const { TextDecoder }  = Cu.import("resource://gre/modules/osfile.jsm", {});
const client = require("../adb-client");
const ADB_STATUS_OFFLINE = "offline";
const OKAY = 0x59414b4f;

// Start tracking devices connecting and disconnecting from the host.
// We can't reuse runCommand here because we keep the socket alive.
class TrackDevicesCommand extends EventEmitter {
  constructor() {
    super();
    this._onConnected = null;
    this._onDisconnected = null;
  }

  get onConnected() {
    return this._onConnected;
  }

  set onConnected(onConnected) {
    this._onConnected = onConnected;
  }

  get onDisconnected() {
    return this._onDisconnected;
  }

  set onDisconnected(onDisconnected) {
    this._onDisconnected = onDisconnected;
  }

  run() {
    this._waitForFirst = true;
    // Hold device statuses. key: device id, value: status.
    this._devices = new Map();
    this._socket = client.connect();

    this._socket.s.onopen = this._onOpen.bind(this);
    this._socket.s.onerror = this._onError.bind(this);
    this._socket.s.onclose = this._onClose.bind(this);
    this._socket.s.ondata = this._onData.bind(this);
  }

  stop() {
    if (this._socket) {
      this._socket.close();
      this._socket.s.onopen = null;
      this._socket.s.onerror = null;
      this._socket.s.onclose = null;
      this._socket.s.ondata = null;
    }
  }

  _onOpen() {
    dumpn("trackDevices onopen");
    const req = client.createRequest("host:track-devices");
    this._socket.send(req);
  }

  _onError(event) {
    dumpn("trackDevices onerror: " + event);
  }

  _onClose() {
    dumpn("trackDevices onclose");
  }

  _onData(event) {
    dumpn("trackDevices ondata: " + event.data);
    let self = this;
    const data = event.data;
    const dec = new TextDecoder();
    dumpn(dec.decode(new Uint8Array(data)).trim());

    // check the OKAY or FAIL on first packet.
    if (this._waitForFirst) {
      if (!client.checkResponse(data, OKAY)) {
        this._socket.close();
        dumpn("checkResponse is FAIL, data is " + data);
        return;
      } else {
        dumpn("checkResponse is OKAY");
        this._socket.close();
      }
    }

    const packet = client.unpackPacket(data, !this._waitForFirst);
    this._waitForFirst = false;

    if (packet.data == "") {
      // All devices got disconnected.
      this._disconnectAllDevices();
    } else {
      // One line per device, each line being $DEVICE\t(offline|device)
      const lines = packet.data.split("\n");
      const newDevices = new Map();
      lines.forEach(function(line) {
        if (line.length == 0) {
          return;
        }

        const [deviceId, status] = line.split("\t");
        newDevices.set(deviceId, status);
      });

      // Fire events if needed.
      const deviceIds = new Set([...this._devices.keys(), ...newDevices.keys()]);
      console.log(...deviceIds.values());

      deviceIds.forEach(function (element, sameElement, set) {
        console.log(self._devices);
        const currentStatus = self._devices.get(element);
        const newStatus = newDevices.get(element);
        self._fireConnectionEventIfNeeded(sameElement, currentStatus, newStatus);
      });

    }
  }

  _disconnectAllDevices() {
    // Todo if we add event handle in adb.js for "no-devices-detected" in the future, I will
    // add logic to deal with the case "this._devices.size is 0" to make code more gentle.
    for (let [deviceId, status] in this._devices.entries()) {
      if (status !== ADB_STATUS_OFFLINE) {
        this["onDisconnected"].call(null, deviceId);
      }
    }
    this._devices = new Map();
  }

  _fireConnectionEventIfNeeded(deviceId, currentStatus, newStatus) {
    const isCurrentOnline = !!(currentStatus && currentStatus !== ADB_STATUS_OFFLINE);
    const isNewOnline = !!(newStatus && newStatus !== ADB_STATUS_OFFLINE);

    if (isCurrentOnline === isNewOnline) {
      return;
    }

    // Update the new devices' name
    if (isNewOnline) {
      this["onConnected"].call(null, deviceId);
    } else {
      this["onDisconnected"].call(null, deviceId);
    }
  }
}
exports.TrackDevicesCommand = TrackDevicesCommand;