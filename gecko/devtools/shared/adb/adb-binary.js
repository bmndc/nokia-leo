/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";
const { dumpn } = require("devtools/shared/DevToolsUtils");

loader.lazyImporter(this, "OS", "resource://gre/modules/osfile.jsm");
loader.lazyRequireGetter(this, "Services");
loader.lazyRequireGetter(this, "FileUtils",
                         "resource://gre/modules/FileUtils.jsm", true);
loader.lazyGetter(this, "BIN_ROOT_PATH", () => {
  return OS.Path.join(OS.Constants.Path.localProfileDir, "../../");
});
loader.lazyGetter(this, "ADB_BINARY_PATH", () => {
  let adbBinaryPath = OS.Path.normalize(BIN_ROOT_PATH);
  // Currently we don't support windows platform, so it's necessary to consider windows case here.
  adbBinaryPath = OS.Path.join(adbBinaryPath, "adb");
  return adbBinaryPath;
});

function getFileForBinary() {
  return new Promise((resolve, reject) => {
    let file = new FileUtils.File(ADB_BINARY_PATH);
    if (!file.exists()) {
      reject("Invalid adb binary path!");
    } else {
      resolve(file);
    }
  });
}

exports.getFileForBinary = getFileForBinary;
