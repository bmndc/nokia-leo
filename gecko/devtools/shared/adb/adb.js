/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

const { clearInterval, setInterval } = require("resource://gre/modules/Timer.jsm");
const EventEmitter = require("devtools/shared/event-emitter");
const { adbProcess } = require("devtools/shared/adb/adb-process");
const AdbDevice = require("devtools/shared/adb/adb-device");
const { AdbRuntime } = require("devtools/shared/adb/adb-runtime");
const { TrackDevicesCommand } = require("devtools/shared/adb/commands/track-devices");
const { dumpn } = require("devtools/shared/DevToolsUtils");

// Duration in milliseconds of the runtime polling. .
const UPDATE_RUNTIMES_INTERVAL = 2000;

class Adb extends EventEmitter {
  constructor() {
    super();

    this._trackDevicesCommand = new TrackDevicesCommand();

    this._isTrackingDevices = false;

    this._listeners = new Set();
    this._devices = new Map();
    this._runtimes = [];

    this._updateAdbProcess = this._updateAdbProcess.bind(this);

    let self = this;
    this._trackDevicesCommand.onConnected = function _onDeviceConnected(deviceId) {
      dumpn("_onDeviceConnected, deviceId is " + deviceId);
      return new Promise((resolve, reject) => {
        const adbDevice = new AdbDevice(deviceId);
        adbDevice.initialize().then(() => {
          try {
            self._devices.set(deviceId, adbDevice);
            self.updateRuntimes();
            resolve();
          } catch(e) {
            console.error(e);
            reject();
          }
        });
      });
    };

    this._trackDevicesCommand.onDisconnected = function _onDeviceDisconnected(deviceId) {
      dumpn("_onDeviceDisConnected, deviceId is " + deviceId);
      this._devices.delete(deviceId);
      this.updateRuntimes();
    };
  }

  registerListener(listener) {
    this._listeners.add(listener);
    this.on("runtime-list-updated", listener);
    this._updateAdbProcess();
  }

  unregisterListener(listener) {
    this._listeners.delete(listener);
    this.off("runtime-list-updated", listener);
    this._updateAdbProcess();
  }

  updateRuntimes() {
    let self = this;
    const devices = [...this._devices.values()];
    console.log(devices);

    try {
      let promises = devices.map((d) => {
        return this._getDeviceRuntimes(d).then((deviceRuntimes)=> {
          return deviceRuntimes;
        }, (unknownRuntimes) => {
          return unknownRuntimes;
        });
      });

      Promise.all([...promises]).then((values) => {
        console.log(values);
        self._runtimes = [];

        for(let i=0; i<values.length; i++) {
          console.log(values[i]);
          self._runtimes.push(...values[i].values());
        }
        self.emit("runtime-list-updated");
      });

    } catch(e) {
      console.error(e);
    }
  }

  getRuntimes() {
    return this._runtimes;
  }

  _startAdb() {
    let self = this;
    this._isTrackingDevices = true;
    let isStart = adbProcess.start();

    isStart.then((isrunning) => {
      if (isrunning == true) {
        dumpn("Adb process is running");
        self._trackDevicesCommand.run();
      } else {
        dumpn("Adb process is not running!");
      }
      }).catch((e) => {
        console.error(e);
    });
  }

  _stopAdb () {
    clearInterval(this._timer);
    this._trackDevicesCommand.stop();
    return new Promise((resolve, reject) => {
        adbProcess.stop();
        resolve();
    }).then(() => {
      this._devices = new Map();
      this._runtimes = [];
      this.updateRuntimes();
    });
  }

  _shouldTrack() {
    return this._listeners.size > 0;
  }

  _updateAdbProcess() {
    if (!this._isTrackingDevices && this._shouldTrack()) {
      this._startAdb();
      this._timer = setInterval(this.refreshTrackDevice.bind(this), UPDATE_RUNTIMES_INTERVAL);
    } else if (this._isTrackingDevices && !this._shouldTrack()) {
      this._stopAdb();
    }
  }

  refreshTrackDevice() {
    dumpn("refreshTrackDevice");
    if (!adbProcess.ready) {
      dumpn("Adb process is not ready and skip refreshTrackDevice this time!");
      return;
    }
    this._trackDevicesCommand.run();
    this.updateRuntimes();
  }

  _getDeviceRuntimes(device) {
    console.log("getDeviceRuntimes, device is:", device);
    return device.getRuntimeSocketPaths().then((socketPaths) => {
      if(socketPaths === "BAD_RESPONSE" || socketPaths.length === 0) {
         dumpn("socketPaths error or empty!");
         return new Set();
      }

      let runtimes = new Set();
      socketPaths.forEach(function (element, sameElement, set) {
        runtimes.add(new AdbRuntime(device, element));
      });

      return runtimes;
    }, (badResponse) => {
      dumpn("_getDeviceRuntimes, badResponse is " + badResponse);
      return new Set();
    });
  }
}

exports.adb = new Adb();