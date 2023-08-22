/* Copyright (c) 2017 KaiOS Technologies. All rights reserved
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

///////////////////////////////////////////////////////////////////////////////
// Test Purpose:
//   To verify the LE scan process of BluetoothAdapter.
//   Testers have to put the B2G devices in an environment which is surrounded
//   by N discoverable remote devices. To pass this test, the number N has to be
//   greater or equals than EXPECTED_NUMBER_OF_REMOTE_DEVICES.
//
// Test Procedure:
//   [0] Set Bluetooth permission and enable default adapter.
//   [1] Start LE scan and verify the correctness.
//   [2] Attach event handler for 'ondevicefound'.
//   [3] Stop LE scan and verify the correctness.
//   [4] Mark the BluetoothDiscoveryHandle from [1] as expired.
//   [5] Start LE scan and verify the correctness.
//   [6] Wait for 'devicefound' events.
//   [7] Stop LE scan and verify the correctness.
//   [8] Call 'startLeScan' twice continuously.
//   [9] Call 'stopLeScan' twice continuously.
//   [10] Clean up the event handler of [2].
//
// Test Coverage:
//   - BluetoothAdapter.startLeScan()
//   - BluetoothAdapter.stopLeScan()
//   - BluetoothDiscoveryHandle.ondevicefound()
//   - BluetoothLeDeviceEvent.device.type
//   - BluetoothLeDeviceEvent.rssi
//   - BluetoothLeDeviceEvent.scanRecord
//
///////////////////////////////////////////////////////////////////////////////

MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

const EXPECTED_NUMBER_OF_REMOTE_DEVICES = 2;

startBluetoothTest(true, function testCaseMain(aAdapter) {
  log("Checking adapter attributes ...");

  is(aAdapter.state, "enabled", "adapter.state");
  isnot(aAdapter.address, "", "adapter.address");

  log("adapter.address: " + aAdapter.address);
  log("adapter.name: " + aAdapter.name);

  let discoveryHandle_A = null;
  let discoveryHandle_B = null;

  return Promise.resolve()
    .then(function() {
      log("[1] Start LE scan and verify the correctness ... ");
      return aAdapter.startLeScan([]);
    })
    .then(function(result) {
      log("[2] Attach event handler for 'ondevicefound' ... ");
      discoveryHandle_A = result;
      isHandleExpired = false;
      discoveryHandle_A.ondevicefound = function onDeviceFound(aEvent) {
        if (isHandleExpired) {
          ok(false, "Expired BluetoothDiscoveryHandle received an event.");
        }
      };
    })
    .then(function() {
      log("[3] Stop LE scan and and verify the correctness ... ");
      return aAdapter.stopLeScan(discoveryHandle_A);
    })
    .then(function() {
      log("[4] Mark the BluetoothDiscoveryHandle from [1] as expired ... ");
      isHandleExpired = true;
    })
    .then(function() {
      log("[5] Start LE scan and verify the correctness ... ");
      return aAdapter.startLeScan([]);
    })
    .then(function(result) {
      log("[6] Wait for 'devicefound' events ... ");
      discoveryHandle_B = result;
      return waitForLeDevicesFound(discoveryHandle_B, EXPECTED_NUMBER_OF_REMOTE_DEVICES);
    })
    .then(function(devices) {
      log("[7] Stop LE scan and and verify the correctness ... ");
      return aAdapter.stopLeScan(discoveryHandle_B);
    })
    .then(function() {
      log("[8] Call 'startLeScan' twice continuously ... ");
      let promises = [];
      promises.push(aAdapter.startLeScan([]));
      promises.push(aAdapter.startLeScan([]));
      return Promise.all(promises);
    })
    .then(function(discoveryHandles) {
      log("[9] Call 'stopLeScan' twice continuously ... ");
      let promises = [];
      promises.push(aAdapter.stopLeScan(discoveryHandles[0]));
      promises.push(aAdapter.stopLeScan(discoveryHandles[1]));
      return Promise.all(promises);
    })
    .then(function() {
      log("[10] Clean up the event handler of [2] ... ");
      if (discoveryHandle_A) {
        discoveryHandle_A.ondevicefound = null;
      }
    });
});
