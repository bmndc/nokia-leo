/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

///////////////////////////////////////////////////////////////////////////////
// Test Purpose:
//   To verify entire pairing process of BluetoothAdapter, except for
//   in-line pairing.
//   Testers have to put a pairable remote device near the testing device
//   and click the 'confirm' button on remote device when it receives a pairing
//   request from testing device.
//   To pass this test, Bluetooth address of the remote device should equal to
//   ADDRESS_OF_TARGETED_REMOTE_DEVICE.
//
// Test Procedure:
//   [0] Set Bluetooth permission and enable default adapter.
//   [1] Unpair device if it's already paired.
//   [2] Pair and wait for confirmation.
//   [3] Get paired devices and verify 'devicepaired' event.
//   [4] Pair again.
//   [5] Unpair.
//   [6] Get paired devices and verify 'deviceunpaired' event.
//   [7] Unpair again.
//
// Test Coverage:
//   - BluetoothAdapter.pair()
//   - BluetoothAdapter.unpair()
//   - BluetoothAdapter.getPairedDevices()
//   - BluetoothAdapter.ondevicepaired()
//   - BluetoothAdapter.ondeviceunpaired()
//   - BluetoothAdapter.pairingReqs
//
//   - BluetoothPairingListener.ondisplaypassykeyreq()      // not covered yet
//   - BluetoothPairingListener.onenterpincodereq()         // not covered yet
//   - BluetoothPairingListener.onpairingconfirmationreq()
//   - BluetoothPairingListener.onpairingconsentreq()
//
//   - BluetoothPairingEvent.address        // not covered yet
//   - BluetoothPairingEvent.deviceName
//   - BluetoothPairingEvent.handle
//
//   - BluetoothPairingHandle.accept()
//   - BluetoothPairingHandle.reject()      // not covered yet
//   - BluetoothPairingHandle.setPinCode()  // not covered yet
//
///////////////////////////////////////////////////////////////////////////////

MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

// The default remote device is Sony SBH50
const ADDRESS_OF_TARGETED_REMOTE_DEVICE = "40:b8:37:fe:ff:2c";

// The specified remote device which tester wants to pair with.
// 'null' representing accepts every pairing request.
const NAME_OF_TARGETED_REMOTE_DEVICE = null;

startBluetoothTest(true, function testCaseMain(aAdapter) {
  log("Checking adapter attributes ...");

  is(aAdapter.state, "enabled", "adapter.state");
  isnot(aAdapter.address, "", "adapter.address");

  // Since adapter has just been re-enabled, these properties should be 'false'.
  is(aAdapter.discovering, false, "adapter.discovering");
  is(aAdapter.discoverable, false, "adapter.discoverable");

  log("adapter.address: " + aAdapter.address);
  log("adapter.name: " + aAdapter.name);

  return Promise.resolve()
    .then(function() {
      log("[1] Unpair device if it's already paired ... ");
      let devices = aAdapter.getPairedDevices();
      for (let i in devices) {
        if (devices[i].address == ADDRESS_OF_TARGETED_REMOTE_DEVICE) {
          log("  - The device has already been paired. Unpair it ...");
          return aAdapter.unpair(devices[i].address);
        }
      }
      log("  - The device hasn't been paired. Skip to Step [2] ...");
      return;
    })
    .then(function() {
      log("[2] Pair and wait for confirmation ... ");

      addEventHandlerForPairingRequest(aAdapter, NAME_OF_TARGETED_REMOTE_DEVICE);

      let promises = [];
      promises.push(waitForAdapterEvent(aAdapter, "devicepaired"));
      promises.push(aAdapter.pair(ADDRESS_OF_TARGETED_REMOTE_DEVICE));

      return Promise.all(promises);
    })
    .then(function(results) {
      log("[3] Get paired devices and verify 'devicepaired' event ... ");
      return verifyDevicePairedUnpairedEvent(aAdapter, results[0]);
    })
    .then(function(deviceEvent) {
      log("[4] Pair again... ");
      return aAdapter.pair(deviceEvent.device.address).then(() => deviceEvent.device.address);
    })
    .then(function(deviceAddress) {
      log("[5] Unpair ... ");
      let promises = [];
      promises.push(waitForAdapterEvent(aAdapter, "deviceunpaired"));
      promises.push(aAdapter.unpair(deviceAddress));
      return Promise.all(promises);
    })
    .then(function(results) {
      log("[6] Get paired devices and verify 'deviceunpaired' event ... ");
      return verifyDevicePairedUnpairedEvent(aAdapter, results[0])
    })
    .then(function(deviceEvent) {
      log("[7] Unpair again... ");
      return aAdapter.unpair(deviceEvent.address);
    });
});
