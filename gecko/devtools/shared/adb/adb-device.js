/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

const { shell } = require("devtools/shared/adb/commands/index");
const socketPathQuery = "cat /proc/net/unix|grep 'debugger-socket'";
const { dumpn } = require("devtools/shared/DevToolsUtils");

/**
 * A Device instance is created and registered with the Devices module whenever
 * ADB notices a new device is connected.
 */
class AdbDevice {
  constructor(id) {
    this.id = id;
  }

  initialize() {
    return shell(this.id, "getprop ro.product.model").then((name) => {
      this.model = name.trim();
    });
  }

  get name() {
    return this.model || this.id;
  }

  getRuntimeSocketPaths() {
    // A matching entry looks like:
    // 00000000: 00000002 00000000 00010000 0001 01 6551588
    // Filter "/data/local/debugger-socket" from real device to build debugging socket connection
    return shell(this.id, socketPathQuery).then((rawSocketInfo) => {
        let socketInfos = rawSocketInfo.split(/\r?\n/);
        socketInfos = socketInfos.filter(l => l.includes("debugger-socket"));
        // It's possible to have multiple lines with the same path, so de-dupe them
        const socketPaths = new Set();
        for (let i = 0; i < socketInfos.length; i++) {
          const socketPath = socketInfos[i].split(" ").pop();
          console.log(socketPath);
          socketPaths.add(socketPath);
        }
        return socketPaths;
    }, (errorResponse) => {
      dumpn("errorResponse is " + errorResponse);
      return errorResponse;
    });
  }
}

module.exports = AdbDevice;