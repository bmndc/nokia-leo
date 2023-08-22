/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

const { Cc, Ci } = require("chrome");
const { dumpn } = require("devtools/shared/DevToolsUtils");
const { Services } = require("resource://gre/modules/Services.jsm");
const { getFileForBinary } = require("./adb-binary");

loader.lazyRequireGetter(this, "runCommand", "devtools/shared/adb/commands/index", true);
loader.lazyRequireGetter(this, "check", "devtools/shared/adb/adb-running-checker", true);

// Class representing the ADB process.
class AdbProcess {
  constructor() {
    this._ready = false;
    this._didRunInitially = false;
  }

  get ready() {
    return this._ready;
  }

  _runProcess(process, params) {
    return new Promise((resolve, reject) => {
      process.runAsync(params, params.length, {
        observe(subject, topic, data) {
          switch (topic) {
            case "process-finished":
              resolve();
              break;
            case "process-failed":
              reject();
              break;
          }
        },
      }, false);
    });
  }

  // We startup by launching adb in server mode, and setting
  // the tcp socket preference to |true|
  start() {
    let self = this;
    return new Promise((resolve, reject) => {
      const onSuccessfulStart = () => {
        // Todo set "adb-ready" event
        // Services.obs.notifyObservers(null, "adb-ready");
        this._ready = true;
        resolve(true);
      };

      // Check whether adb process is running and its version is higher than 31.
      check().then((isAdbRunning) => {
        if (isAdbRunning){
          dumpn("Found ADB process running!");
          onSuccessfulStart();
        } else {
          dumpn("Found ADB Not running!")
          self._didRunInitially = true;
          self.startAdbServer();
          resolve(false);
        }
      });

    });
  }

  startAdbServer() {
    let self = this;
    const process = Cc["@mozilla.org/process/util;1"].createInstance(Ci.nsIProcess);
    return getFileForBinary().then((adbFile) => {
      process.init(adbFile);
      
      const params = ["start-server"];
      self._runProcess(process, params).then(() => {
        dumpn("Run adb process successfully!");
        self._ready = true;
      }, () => {
        dumpn("Run adb process failure!");
        self._ready = false;
      }).catch((e) => {
        dumpn("Run Process Error!");
      });
    });
  }

  /**
   * Stop the ADB server, but only if we started it.  If it was started before
   * us, we return immediately.
   */
  stop() {
    if (!this._didRunInitially) {
      return; // We didn't start the server, nothing to do
    }

    return this.kill();
  }

  /**
   * Kill the ADB server.
   */
  kill() {
    return runCommand("host:kill").then(() => {
      dumpn("adb server was terminated by host:kill");
      this._ready = false;
      this._didRunInitially = false;
    }, () => {
      dumpn("Fail to terminate adb server with host:kill");
    }).catch((e) => {
      dumpn("Failed to send host:kill command");
    });
  }

}

exports.adbProcess = new AdbProcess();