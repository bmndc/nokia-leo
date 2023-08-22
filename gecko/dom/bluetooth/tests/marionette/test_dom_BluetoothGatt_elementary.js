/* Copyright (c) 2017 KaiOS Technologies. All rights reserved
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

///////////////////////////////////////////////////////////////////////////////
// Test Purpose:
//   To verify the essential APIs of GATT client.
//   Testers have to put a remote LE device near the testing device.
//   Please pair with the remote device first if bonded state is required for
//   GATT connection. To pass this test, Bluetooth address of the remote device
//   should equal to ADDRESS_OF_TARGETED_REMOTE_DEVICE.
//
// Test Procedure:
//   [0] Set Bluetooth permission and enable default adapter.
//   [1] Start LE scan ...
//   [2] Wait for 'devicefound' events ...
//   [3] Stop LE scan ...
//   [4] Make sure GATT is disconnected ...
//   [5] Connect GATT and check the correctness of 'onconnectionstatechanged' ...
//   [6] Read Remote Rssi ...
//   [7] Discover Services ...
//   [8] Traverse services, characteristics and descriptors ...
//   [9] Disconnect GATT and check the correctness of 'onconnectionstatechanged' ...
//
// Test Coverage:
//   - BluetoothGatt.connectionState
//   - BluetoothGatt.onconnectionstatechanged
//   - BluetoothGatt.connect()
//   - BluetoothGatt.disconnect()
//   - BluetoothGatt.discoverServices()
//   - BluetoothGatt.readRemoteRssi()
//
//   - BluetoothGattService.isPrimary
//   - BluetoothGattService.uuid
//   - BluetoothGattService.instanceId
//
//   - BluetoothGattCharacteristic.uuid
//   - BluetoothGattCharacteristic.instanceId
//   - BluetoothGattCharacteristic.value
//   - BluetoothGattCharacteristic.permissions
//   - BluetoothGattCharacteristic.properties
//
//   - BluetoothGattDescriptor.uuid
//   - BluetoothGattDescriptor.value
//   - BluetoothGattDescriptor.permissions
//
// These APIs are NOT covered:
//   - BluetoothGatt.oncharacteristicchanged
//   - BluetoothGatt.beginReliableWrite()
//   - BluetoothGatt.executeReliableWrite()
//   - BluetoothGatt.abortReliableWrite()
//
//   - BluetoothGattService.addCharacteristic()
//   - BluetoothGattService.addIncludedService()
//
//   - BluetoothGattCharacteristic.readValue()
//   - BluetoothGattCharacteristic.writeValue()
//   - BluetoothGattCharacteristic.startNotifications()
//   - BluetoothGattCharacteristic.stopNotifications()
//   - BluetoothGattCharacteristic.addDescriptor()
//
//   - BluetoothGattDescriptor.readValue()
//   - BluetoothGattDescriptor.writeValue()
//
///////////////////////////////////////////////////////////////////////////////

MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

// The default remote device is MI1S which doesn't require pairing.
const ADDRESS_OF_TARGETED_REMOTE_DEVICE = "c8:0f:10:2f:b3:f7";

startBluetoothTest(true, function testCaseMain(aAdapter) {
  log("Checking adapter attributes ...");

  is(aAdapter.state, "enabled", "adapter.state");
  isnot(aAdapter.address, "", "adapter.address");

  log("adapter.address: " + aAdapter.address);
  log("adapter.name: " + aAdapter.name);

  let gatt = null;
  let discoveryHandle = null;
  return Promise.resolve()
    .then(function() {
      log("[1] Start LE scan ... ");
      return aAdapter.startLeScan([]);
    })
    .then(function(result) {
      log("[2] Wait for 'devicefound' events ... ");
      discoveryHandle = result;
      return waitForSpecifiedDevicesFound(discoveryHandle, [ADDRESS_OF_TARGETED_REMOTE_DEVICE]);
    })
    .then(function(deviceEvents) {
      log("[3] Stop LE scan ... ");
      if (!deviceEvents[0] || !deviceEvents[0].device || !deviceEvents[0].device.gatt) {
        return Promise.reject();
      }

      gatt = deviceEvents[0].device.gatt;
      ok(gatt instanceof BluetoothGatt, "gatt should be a BluetoothGatt");

      return aAdapter.stopLeScan(discoveryHandle);
    })
    .then(function() {
      log("[4] Make sure GATT is disconnected ... ");
      ok(Array.isArray(gatt.services), "gatt.services should be an Array");

      switch (gatt.connectionState) {
        case 'disconnected':
          return Promise.resolve();
          break;
        case 'connected':
          log("  - disconnect the original connection.");
          return gatt.disconnect();
          break;
        case 'disconnecting':
        case 'connecting':
        default:
          ok(false, "gatt.connectionState should not be " + gatt.connectionState);
          return Promise.reject();
      }
    })
    .then(function() {
      log("[5] Connect GATT and check the correctness of 'onconnectionstatechanged' ... ");
      let promises = [];
      promises.push(waitForGattConnectionStateChanged(gatt, ["connecting", "connected"]));
      promises.push(gatt.connect());
      return Promise.all(promises);
    })
    .then(function() {
      log("[6] Read Remote Rssi ... ");
      is(gatt.connectionState, "connected", "BluetoothGatt.connectionState")
      return gatt.readRemoteRssi();
    })
    .then(function() {
      log("[7] Discover Services ... ");
      return gatt.discoverServices();
    })
    .then(function() {
      log("[8] Traverse services, characteristics and descriptors ... ");
      //  check BluetoothGattService
      gatt.services.forEach(function(service) {
        ok(service instanceof BluetoothGattService, "service should be a BluetoothGattService");
        ok(Array.isArray(service.characteristics), "characteristics should be an Array");
        ok(Array.isArray(service.includedServices), "includedServices should be an Array");
        ok(typeof service.isPrimary === 'boolean');
        ok(typeof service.uuid === 'string');
        ok(typeof service.instanceId === 'number');

        let name = LookupService(service.uuid);
        log("  - GATT Service: " + service.uuid + ", { " + name + " } ");

        //  check BluetoothGattService.includedServices
        service.includedServices.forEach(function(subService) {
          ok(subService instanceof BluetoothGattService, "subService should be a BluetoothGattService");

          let name = LookupService(subService.uuid);
          log("    included service: " + subService.uuid + ",  { " + name + " }   ");
        });

        // check BluetoothGattCharacteristic
        service.characteristics.forEach(function(chara) {
          ok(chara instanceof BluetoothGattCharacteristic, "chara should be a BluetoothGattCharacteristic");
          is(chara.service, service, "BluetoothGattService");
          ok(Array.isArray(chara.descriptors), "descriptors should be an Array");
          ok(typeof chara.uuid === 'string');
          ok(typeof chara.instanceId === 'number');
          ok(typeof chara.permissions === 'object');
          ok(typeof chara.properties === 'object');
          if (chara.value !== null) {
            ok(chara.value instanceof ArrayBuffer, "chara.value should be a ArrayBuffer");
          }

          // check BluetoothGattDescriptor
          let descName = "";
          chara.descriptors.forEach(function(descripter) {
            ok(descripter instanceof BluetoothGattDescriptor, "descripter should be a BluetoothGattDescriptor");
            is(descripter.characteristic, chara, "BluetoothGattCharacteristic");
            ok(typeof descripter.uuid === 'string');
            if (descripter.value !== null) {
              ok(descripter.value instanceof ArrayBuffer, "chara.value should be a ArrayBuffer");
            }
            ok(typeof descripter.permissions === 'object');

            descName += "(" + LoopupDescriptor(descripter.uuid) + ") ";
          }); // forEach of GATT descriptors

          let charaName = LookupCharacteristic(chara.uuid);
          log("    [" + charaName + "] " + descName);
        }); // forEach of GATT characteristic
      }); // forEach of GATT services
    })
    .then(function() {
      log("[9] Disconnect GATT and check the correctness of 'onconnectionstatechanged' ... ");
      let promises = [];
      promises.push(waitForGattConnectionStateChanged(gatt, ["disconnecting", "disconnected"]));
      promises.push(gatt.disconnect());
      return Promise.all(promises);
    });
});
