/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// Wrapper around the ADB utility.

"use strict";
const { Ci, Cu, Cc} = require("chrome");
const { dumpn } = require("devtools/shared/DevToolsUtils");
const { TextEncoder, TextDecoder, OS}  = Cu.import("resource://gre/modules/osfile.jsm", {});
const client = require("../adb-client");

const OKAY = 0x59414b4f;

const shell = function(deviceId, command) {
  dumpn("shell:: deviceId is " + deviceId + " and command is " + command);
  if (!deviceId) {
    throw new Error("ADB shell command needs the device id");
  }

  let state;
  let stdout = "";

  dumpn("shell " + command + " on " + deviceId);

  return new Promise((resolve, reject) => {
    const shutdown = function() {
      dumpn("shell shutdown");
      socket.close();
      reject("BAD_RESPONSE");
    };

    const runFSM = function runFSM(data) {
      dumpn("runFSM " + state);
      let req;
      let ignoreResponseCode = false;
      switch (state) {
        case "start":
          state = "send-transport";
          runFSM();
          break;
        case "send-transport":
          req = client.createRequest("host:transport:" + deviceId);
          console.log("send-transport, req is ", req);

          socket.send(req);
          state = "wait-transport";
          break;
        case "wait-transport":
          if (!client.checkResponse(data, OKAY)) {
            dumpn("Client checkResponse not OKAY");
            shutdown();
            return;
          } else {
            console.log("Client checkResponse OKAY", data);
          }
          state = "send-shell";
          runFSM();
          break;
        case "send-shell":
          req = client.createRequest("shell:" + command);
          console.log("run command: ", req);

          socket.send(req);
          state = "rec-shell";
          break;
        case "rec-shell":
          if (!client.checkResponse(data, OKAY)) {
            shutdown();
            return;
          }
          state = "decode-shell";
          if (client.getBuffer(data).byteLength == 4) {
            break;
          }
          ignoreResponseCode = true;
          runFSM(data);
          break;
          // eslint-disable-next-lined no-fallthrough
        case "decode-shell":
          const decoder = new TextDecoder();
          const text = new Uint8Array(client.getBuffer(data),
                                      ignoreResponseCode ? 4 : 0);
          stdout += decoder.decode(text);
          socket.close();
          // Adb server return OKAY+deviceName for some devices while return deviceName directly for others.
          if (stdout.substr(0, 4) === "OKAY") {
            resolve(stdout.substring(4, stdout.end));
          } else {
            resolve(stdout);
          }
          break;
        default:
          dumpn("shell Unexpected State: " + state);
          reject("UNEXPECTED_STATE");
      }
    };

    const socket = client.connect();
    socket.s.onerror = function(event) {
      dumpn("shell onerror");
      reject("SOCKET_ERROR");
    };

    socket.s.onopen = function(event) {
      dumpn("shell onopen");
      state = "start";
      runFSM();
    };

    socket.s.onclose = function(event) {
      resolve(stdout);
      dumpn("shell onclose");
    };

    socket.s.ondata = function(event) {
      dumpn("shell ondata");
      runFSM(event.data);
    };
  });
};

exports.shell = shell;