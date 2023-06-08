/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

///////////////////////////////////////////////////////////////////////////////
// Test Purpose:
//   To verify the basic functionality of BluetoothManager.
//
// Test Coverage:
//   - BluetoothManager.defaultAdapter
//   - BluetoothManager.getAdapters()
// TODO:
//   - BluetoothManager.onattributechanged()
//   - BluetoothManager.onadapteradded()
//   - BluetoothManager.onadapterremoved()
//
///////////////////////////////////////////////////////////////////////////////

MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

startBluetoothTestBase(["settings-read", "settings-write", "settings-api-read", "settings-api-write"],
                       function testCaseMain() {
  let adapters = bluetoothManager.getAdapters();
  ok(Array.isArray(adapters), "Can not got the array of adapters");
  ok(adapters.length, "The number of adapters should not be zero");
  ok(bluetoothManager.defaultAdapter, "defaultAdapter should not be null.");
});
