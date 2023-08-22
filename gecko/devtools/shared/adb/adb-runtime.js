/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

const { RuntimeTypes } = require("devtools/client/webide/modules/runtimes");
const { prepareTCPConnection } = require("devtools/shared/adb/commands/index");

class AdbRuntime {
  constructor(adbDevice, socketPath) {
    this.type = RuntimeTypes.USB;

    this._adbDevice = adbDevice;
    this._socketPath = socketPath;
  }

  get id() {
    return this._adbDevice.id + "|" + this._socketPath;
  }

  get deviceId() {
    return this._adbDevice.id;
  }

  get name() {
    return this._adbDevice.name;
  }

  connect(connection) {
    return prepareTCPConnection(this.deviceId, this._socketPath).then(port => {
      connection.host = "localhost";
      connection.port = port;
      connection.connect();
    });
  }

  isUnknown() {
    return false;
  }
}
exports.AdbRuntime = AdbRuntime;