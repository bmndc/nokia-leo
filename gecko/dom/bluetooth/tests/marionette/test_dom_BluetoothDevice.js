/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

///////////////////////////////////////////////////////////////////////////////
// Test Purpose:
//   To verify that all properties of BluetoothDevice is correct when they've
//   been discovered by adapter and delivered by BluetoothDeviceEvent.
//   Testers have to put the B2G devices in an environment which is surrounded
//   by N discoverable remote devices. To pass this test, the number N has to be
//   be greater than (or equal to) EXPECTED_NUMBER_OF_REMOTE_DEVICES.
//
// Test Procedure:
//   [0] Set Bluetooth permission and enable default adapter.
//   [1] Start discovery.
//   [2] Wait for 'devicefound' events.
//   [3] Type checking for BluetoothDeviceEvent and BluetoothDevice.
//   [4] Attach the 'onattributechanged' handler for the first device.
//   [5] Stop discovery.
//   [6] Fetch the UUIDs of the first device.
//   [7] Verify the UUIDs.
//
// Test Coverage:
//   - BluetoothDevice.address
//   - BluetoothDevice.cod
//   - BluetoothDevice.name
//   - BluetoothDevice.paired
//   - BluetoothDevice.uuids
//   - BluetoothDevice.onattributechanged()
//   - BluetoothDevice.fetchUuids()
//   - BluetoothDeviceEvent.address
//   - BluetoothDeviceEvent.device
//
///////////////////////////////////////////////////////////////////////////////

MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

const EXPECTED_NUMBER_OF_REMOTE_DEVICES = 1;

var remoteDevice = null;
var hasReceivedUuidsChanged = false;
var originalUuids;

startBluetoothTest(true, function testCaseMain(aAdapter) {
  log("Checking adapter attributes ...");

  is(aAdapter.state, "enabled", "adapter.state");
  isnot(aAdapter.address, "", "adapter.address");

  // Since adapter has just been re-enabled, these properties should be 'false'.
  is(aAdapter.discovering, false, "adapter.discovering");
  is(aAdapter.discoverable, false, "adapter.discoverable at step [0]");

  log("adapter.address: " + aAdapter.address);
  log("adapter.name: " + aAdapter.name);

  return Promise.resolve()
    .then(function(discoveryHandle) {
      log("[1] Start discovery ... ");
      return aAdapter.startDiscovery();
    })
    .then(function(discoveryHandle) {
      log("[2] Wait for 'devicefound' events ... ");
      return waitForDevicesFound(discoveryHandle, EXPECTED_NUMBER_OF_REMOTE_DEVICES);
    })
    .then(function(deviceEvents) {
      log("[3] Type checking for BluetoothDeviceEvent and BluetoothDevice ... ");

      for (let i in deviceEvents) {
        let deviceEvt = deviceEvents[i];
        is(deviceEvt.address, null, "deviceEvt.address");
        isnot(deviceEvt.device, null, "deviceEvt.device");
        ok(deviceEvt.device instanceof BluetoothDevice,
          "deviceEvt.device should be a BluetoothDevice");

        let device = deviceEvt.device;
        ok(typeof device.address === 'string', "type checking for address.");
        ok(typeof device.name === 'string', "type checking for name.");
        ok(device.cod instanceof BluetoothClassOfDevice, "type checking for cod.");
        ok(typeof device.paired === 'boolean', "type checking for paired.");
        ok(Array.isArray(device.uuids), "type checking for uuids.");

        originalUuids = device.uuids;

        log("  - BluetoothDevice.address: " + device.address);
        log("  - BluetoothDevice.name: " + device.name);
        log("  - BluetoothDevice.cod: " + device.cod);
        log("  - BluetoothDevice.paired: " + device.paired);
        log("  - BluetoothDevice.uuids: " + device.uuids);
      }

      // Pick the first remote device for testing
      remoteDevice = deviceEvents[0].device;
    })
    .then(function() {
      log("[4] Attach the 'onattributechanged' handler for the remote device ... ");
      remoteDevice.onattributechanged = function(aEvent) {
        for (let i in aEvent.attrs) {
          switch (aEvent.attrs[i]) {
            case "cod":
              log("  'cod' changed to " + remoteDevice.cod);
              break;
            case "name":
              log("  'name' changed to " + remoteDevice.name);
              break;
            case "paired":
              log("  'paired' changed to " + remoteDevice.paired);
              break;
            case "uuids":
              log("  'uuids' changed to " + remoteDevice.uuids);
              hasReceivedUuidsChanged = true;
              break;
            case "unknown":
            default:
              ok(false, "Unknown attribute '" + aEvent.attrs[i] + "' changed." );
              break;
          }
        }
      };
    })
    .then(function() {
      log("[5] Stop discovery ... ");
      return aAdapter.stopDiscovery();
    })
    .then(function() {
      log("[6] Fetch the UUIDs of remote device ... ");

      return remoteDevice.fetchUuids();
    })
    .then(function(uuids) {
      log("[7] Verify the UUIDs ... ");

      ok(Array.isArray(uuids), "type checking for 'fetchUuids'.");

      ok(isUuidsEqual(uuids, remoteDevice.uuids),
        "remoteDevice.uuids should equal to the result from 'fetchUuids'");

      let isUuidsChanged = !isUuidsEqual(originalUuids, remoteDevice.uuids);
      is(isUuidsChanged, hasReceivedUuidsChanged, "remoteDevice.uuids has changed.");

      remoteDevice.onattributechanged = null;
    });
});
