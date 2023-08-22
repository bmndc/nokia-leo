/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// https://github.com/mozilla-b2g/platform_external_qemu/blob/master/vl-android.c#L765
// static int bt_hci_parse(const char *str) {
//   ...
//   bdaddr.b[0] = 0x52;
//   bdaddr.b[1] = 0x54;
//   bdaddr.b[2] = 0x00;
//   bdaddr.b[3] = 0x12;
//   bdaddr.b[4] = 0x34;
//   bdaddr.b[5] = 0x56 + nb_hcis;
const EMULATOR_ADDRESS = "56:34:12:00:54:52";

// $ adb shell hciconfig /dev/ttyS2 name
// hci0:  Type: BR/EDR  Bus: UART
//        BD Address: 56:34:12:00:54:52  ACL MTU: 512:1  SCO MTU: 0:0
//        Name: 'Full Android on Emulator'
const EMULATOR_NAME = "Full Android on Emulator";

// $ adb shell hciconfig /dev/ttyS2 class
// hci0:  Type: BR/EDR  Bus: UART
//        BD Address: 56:34:12:00:54:52  ACL MTU: 512:1  SCO MTU: 0:0
//        Class: 0x58020c
//        Service Classes: Capturing, Object Transfer, Telephony
//        Device Class: Phone, Smart phone
const EMULATOR_CLASS = 0x58020c;

// Use same definition in QEMU for special bluetooth address,
// which were defined at external/qemu/hw/bt.h:
const BDADDR_ANY   = "00:00:00:00:00:00";
const BDADDR_ALL   = "ff:ff:ff:ff:ff:ff";
const BDADDR_LOCAL = "ff:ff:ff:00:00:00";

// A user friendly name for remote BT device.
const REMOTE_DEVICE_NAME = "Remote_BT_Device";

// Emulate Promise.jsm semantics.
Promise.defer = function() { return new Deferred(); }
function Deferred()  {
  this.promise = new Promise(function(resolve, reject) {
    this.resolve = resolve;
    this.reject = reject;
  }.bind(this));
  Object.freeze(this);
}

var bluetoothManager;

var pendingEmulatorCmdCount = 0;

/**
 * Push required permissions and test if |navigator.mozBluetooth| exists.
 * Resolve if it does, reject otherwise.
 *
 * Fulfill params:
 *   bluetoothManager -- an reference to navigator.mozBluetooth.
 * Reject params: (none)
 *
 * @param aPermissions
 *        Additional permissions to push before any test cases.  Could be either
 *        a string or an array of strings.
 *
 * @return A deferred promise.
 */
function ensureBluetoothManager(aPermissions) {
  let deferred = Promise.defer();

  // TODO: push permissions for running test on content app.
  // let permissions = ["bluetooth"];
  // if (aPermissions) {
  //   if (Array.isArray(aPermissions)) {
  //     permissions = permissions.concat(aPermissions);
  //   } else if (typeof aPermissions == "string") {
  //     permissions.push(aPermissions);
  //   }
  // }
  //
  // let obj = [];
  // for (let perm of permissions) {
  //   obj.push({
  //     "type": perm,
  //     "allow": 1,
  //     "context": document,
  //   });
  // }

  bluetoothManager = window.navigator.mozBluetooth;
  log("navigator.mozBluetooth is " +
      (bluetoothManager ? "available" : "unavailable"));

  if (bluetoothManager instanceof BluetoothManager) {
    deferred.resolve(bluetoothManager);
  } else {
    deferred.reject();
  }

  return deferred.promise;
}

/**
 * Send emulator command with safe guard.
 *
 * We should only call |finish()| after all emulator command transactions
 * end, so here comes with the pending counter.  Resolve when the emulator
 * gives positive response, and reject otherwise.
 *
 * Fulfill params:
 *   result -- an array of emulator response lines.
 *
 * Reject params:
 *   result -- an array of emulator response lines.
 *
 * @return A deferred promise.
 */
function runEmulatorCmdSafe(aCommand) {
  let deferred = Promise.defer();

  ++pendingEmulatorCmdCount;
  runEmulatorCmd(aCommand, function(aResult) {
    --pendingEmulatorCmdCount;

    ok(true, "Emulator response: " + JSON.stringify(aResult));
    if (Array.isArray(aResult) && aResult[aResult.length - 1] === "OK") {
      deferred.resolve(aResult);
    } else {
      ok(false, "Got an abnormal response from emulator.");
      log("Fail to execute emulator command: [" + aCommand + "]");
      deferred.reject(aResult);
    }
  });

  return deferred.promise;
}

/**
 * Add a Bluetooth remote device to scatternet and set its properties.
 *
 * Use QEMU command 'bt remote add' to add a virtual Bluetooth remote
 * and set its properties by setEmulatorDeviceProperty().
 *
 * Fulfill params:
 *   result -- bluetooth address of the remote device.
 * Reject params: (none)
 *
 * @param aProperies
 *        A javascript object with zero or several properties for initializing
 *        the remote device. By now, the properies could be 'name' or
 *        'discoverable'. It valid to put a null object or a javascript object
 *        which don't have any properies.
 *
 * @return A promise object.
 */
function addEmulatorRemoteDevice(aProperties) {
  let address;
  let promise = runEmulatorCmdSafe("bt remote add")
    .then(function(aResults) {
      address = aResults[0].toUpperCase();
    });

  for (let key in aProperties) {
    let value = aProperties[key];
    let propertyName = key;
    promise = promise.then(function() {
      return setEmulatorDeviceProperty(address, propertyName, value);
    });
  }

  return promise.then(function() {
    return address;
  });
}

/**
 * Remove Bluetooth remote devices in scatternet.
 *
 * Use QEMU command 'bt remote remove <addr>' to remove a specific virtual
 * Bluetooth remote device in scatternet or remove them all by  QEMU command
 * 'bt remote remove BDADDR_ALL'.
 *
 * @param aAddress
 *        The string of Bluetooth address with format xx:xx:xx:xx:xx:xx.
 *
 * Fulfill params:
 *   result -- an array of emulator response lines.
 * Reject params: (none)
 *
 * @return A promise object.
 */
function removeEmulatorRemoteDevice(aAddress) {
  let cmd = "bt remote remove " + aAddress;
  return runEmulatorCmdSafe(cmd)
    .then(function(aResults) {
      // 'bt remote remove <bd_addr>' returns a list of removed device one at a line.
      // The last item is "OK".
      return aResults.slice(0, -1);
    });
}

/**
 * Set a property for a Bluetooth device.
 *
 * Use QEMU command 'bt property <bd_addr> <prop_name> <value>' to set property.
 *
 * Fulfill params:
 *   result -- an array of emulator response lines.
 * Reject params:
 *   result -- an array of emulator response lines.
 *
 * @param aAddress
 *        The string of Bluetooth address with format xx:xx:xx:xx:xx:xx.
 * @param aPropertyName
 *        The property name of Bluetooth device.
 * @param aValue
 *        The new value of the specifc property.
 *
 * @return A deferred promise.
 */
function setEmulatorDeviceProperty(aAddress, aPropertyName, aValue) {
  let cmd = "bt property " + aAddress + " " + aPropertyName + " " + aValue;
  return runEmulatorCmdSafe(cmd);
}

/**
 * Get a property from a Bluetooth device.
 *
 * Use QEMU command 'bt property <bd_addr> <prop_name>' to get properties.
 *
 * Fulfill params:
 *   result -- a string with format <prop_name>: <value_of_prop>
 * Reject params:
 *   result -- an array of emulator response lines.
 *
 * @param aAddress
 *        The string of Bluetooth address with format xx:xx:xx:xx:xx:xx.
 * @param aPropertyName
 *        The property name of Bluetooth device.
 *
 * @return A deferred promise.
 */
function getEmulatorDeviceProperty(aAddress, aPropertyName) {
  let cmd = "bt property " + aAddress + " " + aPropertyName;
  return runEmulatorCmdSafe(cmd)
    .then(function(aResults) {
      return aResults[0];
    });
}

/**
 * Get mozSettings value specified by @aKey.
 *
 * Resolve if that mozSettings value is retrieved successfully, reject
 * otherwise.
 *
 * Fulfill params:
 *   The corresponding mozSettings value of the key.
 * Reject params: (none)
 *
 * @param aKey
 *        A string.
 *
 * @return A deferred promise.
 */
function getSettings(aKey) {
  let deferred = Promise.defer();

  let request = navigator.mozSettings.createLock().get(aKey);
  request.addEventListener("success", function(aEvent) {
    ok(true, "getSettings(" + aKey + ")");
    deferred.resolve(aEvent.target.result[aKey]);
  });
  request.addEventListener("error", function() {
    ok(false, "getSettings(" + aKey + ")");
    deferred.reject();
  });

  return deferred.promise;
}

/**
 * Set mozSettings values.
 *
 * Resolve if that mozSettings value is set successfully, reject otherwise.
 *
 * Fulfill params: (none)
 * Reject params: (none)
 *
 * @param aSettings
 *        An object of format |{key1: value1, key2: value2, ...}|.
 *
 * @return A deferred promise.
 */
function setSettings(aSettings) {
  let lock = window.navigator.mozSettings.createLock();
  let request = lock.set(aSettings);
  let deferred = Promise.defer();
  lock.onsettingstransactionsuccess = function () {
    ok(true, "setSettings(" + JSON.stringify(aSettings) + ")");
    deferred.resolve();
  };
  lock.onsettingstransactionfailure = function (aEvent) {
    ok(false, "setSettings(" + JSON.stringify(aSettings) + ")");
    deferred.reject();
  };
  return deferred.promise;
}

/**
 * Get the boolean value which indicates defaultAdapter of bluetooth is enabled.
 *
 * Resolve if that defaultAdapter is enabled
 *
 * Fulfill params:
 *   A boolean value.
 * Reject params: (none)
 *
 * @return A deferred promise.
 */
function getBluetoothEnabled() {
  log("bluetoothManager.defaultAdapter.state: " + bluetoothManager.defaultAdapter.state);

  return (bluetoothManager.defaultAdapter.state == "enabled");
}

/**
 * Set mozSettings value of 'bluetooth.enabled'.
 *
 * Resolve if that mozSettings value is set successfully, reject otherwise.
 *
 * Fulfill params: (none)
 * Reject params: (none)
 *
 * @param aEnabled
 *        A boolean value.
 *
 * @return A deferred promise.
 */
function setBluetoothEnabled(aEnabled) {
  let obj = {};
  obj["bluetooth.enabled"] = aEnabled;
  return setSettings(obj);
}

/**
 * Wait for one named BluetoothManager event.
 *
 * Resolve if that named event occurs. Never reject.
 *
 * Fulfill params: the DOMEvent passed.
 *
 * @param aEventName
 *        The name of the EventHandler.
 *
 * @return A deferred promise.
 */
function waitForManagerEvent(aEventName) {
  let deferred = Promise.defer();

  bluetoothManager.addEventListener(aEventName, function onevent(aEvent) {
    bluetoothManager.removeEventListener(aEventName, onevent);

    ok(true, "BluetoothManager event '" + aEventName + "' got.");
    deferred.resolve(aEvent);
  });

  return deferred.promise;
}

/**
 * Wait for one named BluetoothAdapter event.
 *
 * Resolve if that named event occurs. Never reject.
 *
 * Fulfill params: the DOMEvent passed.
 *
 * @param aAdapter
 *        The BluetoothAdapter you want to use.
 * @param aEventName
 *        The name of the EventHandler.
 *
 * @return A deferred promise.
 */
function waitForAdapterEvent(aAdapter, aEventName) {
  let deferred = Promise.defer();

  aAdapter.addEventListener(aEventName, function onevent(aEvent) {
    aAdapter.removeEventListener(aEventName, onevent);

    ok(true, "BluetoothAdapter event '" + aEventName + "' got.");
    deferred.resolve(aEvent);
  });

  return deferred.promise;
}

/**
 * Wait for 'onattributechanged' events for state changes of BluetoothAdapter
 * with specified order.
 *
 * Resolve if those expected events occur in order. Never reject.
 *
 * Fulfill params: an array which contains every changed attributes during
 *                 the waiting.
 *
 * @param aAdapter
 *        The BluetoothAdapter you want to use.
 * @param aStateChangesInOrder
 *        An array which contains an expected order of BluetoothAdapterState.
 *        Example 1: ["enabling", "enabled"]
 *        Example 2: ["disabling", "disabled"]
 *
 * @return A deferred promise.
 */
function waitForAdapterStateChanged(aAdapter, aStateChangesInOrder) {
  let deferred = Promise.defer();

  let stateIndex = 0;
  let prevStateIndex = 0;
  let statesArray = [];
  let changedAttrs = [];
  aAdapter.onattributechanged = function(aEvent) {
    for (let i in aEvent.attrs) {
      changedAttrs.push(aEvent.attrs[i]);
      switch (aEvent.attrs[i]) {
        case "state":
          log("  'state' changed to " + aAdapter.state);

          // Received state change order may differ from expected one even though
          // state changes in expected order, because the value of state may change
          // again before we receive prior 'onattributechanged' event.
          //
          // For example, expected state change order [A,B,C] may result in
          // received ones:
          // - [A,C,C] if state becomes C before we receive 2nd 'onattributechanged'
          // - [B,B,C] if state becomes B before we receive 1st 'onattributechanged'
          // - [C,C,C] if state becomes C before we receive 1st 'onattributechanged'
          // - [A,B,C] if all 'onattributechanged' are received in perfect timing
          //
          // As a result, we ensure only following conditions instead of exactly
          // matching received and expected state change order.
          // - Received state change order never reverse expected one. For example,
          //   [B,A,C] should never occur with expected state change order [A,B,C].
          // - The changed value of state in received state change order never
          //   appears later than that in expected one. For example, [A,A,C] should
          //   never occur with expected state change order [A,B,C].
          let stateIndex = aStateChangesInOrder.indexOf(aAdapter.state);
          if (stateIndex >= prevStateIndex && stateIndex + 1 > statesArray.length) {
            statesArray.push(aAdapter.state);
            prevStateIndex = stateIndex;

            if (statesArray.length == aStateChangesInOrder.length) {
              aAdapter.onattributechanged = null;
              ok(true, "BluetoothAdapter event 'onattributechanged' got.");
              deferred.resolve(changedAttrs);
            }
          } else {
            ok(false, "The order of 'onattributechanged' events is unexpected.");
          }

          break;
        case "name":
          log("  'name' changed to " + aAdapter.name);
          if (aAdapter.state == "enabling") {
            isnot(aAdapter.name, "", "adapter.name");
          }
          else if (aAdapter.state == "disabling") {
            is(aAdapter.name, "", "adapter.name");
          }
          break;
        case "address":
          log("  'address' changed to " + aAdapter.address);
          if (aAdapter.state == "enabling") {
            isnot(aAdapter.address, "", "adapter.address");
          }
          else if (aAdapter.state == "disabling") {
            is(aAdapter.address, "", "adapter.address");
          }
          break;
        case "discoverable":
          log("  'discoverable' changed to " + aAdapter.discoverable);
          if (aAdapter.state == "enabling") {
            is(aAdapter.discoverable, true, "adapter.discoverable");
          }
          else if (aAdapter.state == "disabling") {
            is(aAdapter.discoverable, false, "adapter.discoverable");
          }
          break;
        case "discovering":
          log("  'discovering' changed to " + aAdapter.discovering);
          if (aAdapter.state == "enabling") {
            is(aAdapter.discovering, true, "adapter.discovering");
          }
          else if (aAdapter.state == "disabling") {
            is(aAdapter.discovering, false, "adapter.discovering");
          }
          break;
        case "unknown":
        default:
          ok(false, "Unknown attribute '" + aEvent.attrs[i] + "' changed." );
          break;
      }
    }
  };

  return deferred.promise;
}

/**
 * Wait for an 'onattributechanged' event for a specified attribute and compare
 * the new value with the expected value.
 *
 * Resolve if the specified event occurs. Never reject.
 *
 * Fulfill params: a BluetoothAttributeEvent with property attrs that contains
 *                 changed BluetoothAdapterAttributes.
 *
 * @param aAdapter
 *        The BluetoothAdapter you want to use.
 * @param aAttrName
 *        The name of the attribute of adapter.
 * @param aExpectedValue
 *        The expected new value of the attribute.
 *
 * @return A deferred promise.
 */
function waitForAdapterAttributeChanged(aAdapter, aAttrName, aExpectedValue) {
  let deferred = Promise.defer();

  aAdapter.onattributechanged = function(aEvent) {
    let i = aEvent.attrs.indexOf(aAttrName);
    if (i >= 0) {
      switch (aEvent.attrs[i]) {
        case "state":
          log("  'state' changed to " + aAdapter.state);
          is(aAdapter.state, aExpectedValue, "adapter.state");
          break;
        case "name":
          log("  'name' changed to " + aAdapter.name);
          is(aAdapter.name, aExpectedValue, "adapter.name");
          break;
        case "address":
          log("  'address' changed to " + aAdapter.address);
          is(aAdapter.address, aExpectedValue, "adapter.address");
          break;
        case "discoverable":
          log("  'discoverable' changed to " + aAdapter.discoverable);
          is(aAdapter.discoverable, aExpectedValue, "adapter.discoverable");
          break;
        case "discovering":
          log("  'discovering' changed to " + aAdapter.discovering);
          is(aAdapter.discovering, aExpectedValue, "adapter.discovering");
          break;
        case "unknown":
        default:
          ok(false, "Unknown attribute '" + aAttrName + "' changed." );
          break;
      }
      aAdapter.onattributechanged = null;
      deferred.resolve(aEvent);
    }
  };

  return deferred.promise;
}

/**
 * Wait for specified number of 'devicefound' events triggered by discovery.
 *
 * Resolve if specified number of devices has been found. Never reject.
 *
 * Fulfill params: an array which contains BluetoothDeviceEvents that we
 *                 received from the BluetoothDiscoveryHandle.
 *
 * @param aDiscoveryHandle
 *        A BluetoothDiscoveryHandle which is used to notify application of
 *        discovered remote bluetooth devices.
 * @param aExpectedNumberOfDevices
 *        The number of remote devices we expect to discover.
 *
 * @return A deferred promise.
 */
function waitForDevicesFound(aDiscoveryHandle, aExpectedNumberOfDevices) {
  let deferred = Promise.defer();

  ok(aDiscoveryHandle instanceof BluetoothDiscoveryHandle,
    "aDiscoveryHandle should be a BluetoothDiscoveryHandle");

  let deviceEvents = [];
  aDiscoveryHandle.ondevicefound = function onDeviceFound(aEvent) {
    ok(aEvent instanceof BluetoothDeviceEvent,
      "aEvent should be a BluetoothDeviceEvent");

    ok(aEvent.device instanceof BluetoothDevice,
      "aEvent.device should be a BluetoothDevice");

    deviceEvents.push(aEvent);
    if (deviceEvents.length >= aExpectedNumberOfDevices) {
      aDiscoveryHandle.ondevicefound = null;
      deferred.resolve(deviceEvents);
    }
  };

  return deferred.promise;
}

/**
 * Wait for specified number of 'devicefound' events triggered by LE scan.
 *
 * Resolve if specified number of devices has been found. Never reject.
 *
 * Fulfill params: an array which contains BluetoothLeDeviceEvents that we
 *                 received from the BluetoothDiscoveryHandle.
 *
 * @param aDiscoveryHandle
 *        A BluetoothDiscoveryHandle which is used to notify application of
 *        discovered remote LE bluetooth devices.
 * @param aExpectedNumberOfDevices
 *        The number of remote devices we expect to scan.
 *
 * @return A deferred promise.
 */
function waitForLeDevicesFound(aDiscoveryHandle, aExpectedNumberOfDevices) {
  let deferred = Promise.defer();

  ok(aDiscoveryHandle instanceof BluetoothDiscoveryHandle,
    "aDiscoveryHandle should be a BluetoothDiscoveryHandle");

  let deviceEvents = [];
  aDiscoveryHandle.ondevicefound = function onDeviceFound(aEvent) {
    ok(aEvent instanceof BluetoothLeDeviceEvent,
      "aEvent should be a BluetoothLeDeviceEvent");

    ok(aEvent.device instanceof BluetoothDevice,
      "aEvent.device should be a BluetoothDevice");

    ok(typeof aEvent.rssi === 'number', "rssi should be a number");
    ok(typeof aEvent.scanRecord === 'object', "scanRecord should be a object");
    isnot(aEvent.device.type, 'classic', "BluetoothDevice.type");

    deviceEvents.push(aEvent);
    if (deviceEvents.length >= aExpectedNumberOfDevices) {
      aDiscoveryHandle.ondevicefound = null;
      deferred.resolve(deviceEvents);
    }
  };

  return deferred.promise;
}

/**
 * Wait for 'devicefound' events of specified devices.
 *
 * Resolve if every specified device has been found. Never reject.
 *
 * Fulfill params: an array which contains the BluetoothDeviceEvents that we
 *                 expected to receive from the BluetoothDiscoveryHandle.
 *
 * @param aDiscoveryHandle
 *        A BluetoothDiscoveryHandle which is used to notify application of
 *        discovered remote bluetooth devices.
 * @param aRemoteAddresses
 *        An array which contains addresses of remote devices.
 *
 * @return A deferred promise.
 */
function waitForSpecifiedDevicesFound(aDiscoveryHandle, aRemoteAddresses) {
  let deferred = Promise.defer();

  ok(aDiscoveryHandle instanceof BluetoothDiscoveryHandle,
    "aDiscoveryHandle should be a BluetoothDiscoveryHandle");

  let devicesArray = [];
  aDiscoveryHandle.ondevicefound = function onDeviceFound(aEvent) {
    if (aRemoteAddresses.indexOf(aEvent.device.address) != -1) {
      devicesArray.push(aEvent);
    }
    if (devicesArray.length == aRemoteAddresses.length) {
      aDiscoveryHandle.ondevicefound = null;
      ok(true, "BluetoothAdapter has found all remote devices.");
      deferred.resolve(devicesArray);
    }
  };

  return deferred.promise;
}

/**
 * Verify the correctness of 'devicepaired' or 'deviceunpaired' events.
 *
 * Use BluetoothAdapter.getPairedDevices() to get current device array.
 * Resolve if the device of from 'devicepaired' event exists in device array or
 * the device of address from 'deviceunpaired' event has been removed from
 * device array.
 * Otherwise, reject the promise.
 *
 * Fulfill params: the device event from 'devicepaired' or 'deviceunpaired'.
 *
 * @param aAdapter
 *        The BluetoothAdapter you want to use.
 * @param aDeviceEvent
 *        The device event from "devicepaired" or "deviceunpaired".
 *
 * @return A deferred promise.
 */
function verifyDevicePairedUnpairedEvent(aAdapter, aDeviceEvent) {
  let deferred = Promise.defer();

  let devices = aAdapter.getPairedDevices();
  let isPromisePending = true;
  if (aDeviceEvent.device) {
    log("  - Verify 'devicepaired' event");
    for (let i in devices) {
      if (devices[i].address == aDeviceEvent.device.address) {
        deferred.resolve(aDeviceEvent);
        isPromisePending = false;
      }
    }
    if (isPromisePending) {
      deferred.reject(aDeviceEvent);
      isPromisePending = false;
    }
  } else if (aDeviceEvent.address) {
    log("  - Verify 'deviceunpaired' event");
    for (let i in devices) {
      if (devices[i].address == aDeviceEvent.address) {
        deferred.reject(aDeviceEvent);
        isPromisePending = false;
      }
    }
    if (isPromisePending) {
      deferred.resolve(aDeviceEvent);
      isPromisePending = false;
    }
  } else {
    log("  - Exception occurs. Unexpected aDeviceEvent properties.");
    deferred.reject(aDeviceEvent);
    isPromisePending = false;
  }

  return deferred.promise;
}

/**
 * Add event handlers for pairing request listener.
 *
 * @param aAdapter
 *        The BluetoothAdapter you want to use.
 * @param aSpecifiedBdName (optional)
 *        Bluetooth device name of the specified remote device which adapter
 *        wants to pair with. If aSpecifiedBdName is an empty string, 'null'
 *        or 'undefined', this function accepts every pairing request.
 */
function addEventHandlerForPairingRequest(aAdapter, aSpecifiedBdName) {
  log("  - Add event handlers for pairing requests.");

  aAdapter.pairingReqs.ondisplaypasskeyreq = function onDisplayPasskeyReq(evt) {
    ok(evt.handle instanceof BluetoothPairingHandle, "type checking for handle.");
    ok(typeof evt.deviceName === 'string', "type checking for deviceName.");
    let passkey = evt.handle.passkey; // passkey to display
    ok(typeof passkey === 'string', "type checking for passkey.");
    log("  - Received 'ondisplaypasskeyreq' event with passkey: " + passkey);

    if (!aSpecifiedBdName || evt.deviceName == aSpecifiedBdName) {
      cleanupPairingListener(aAdapter.pairingReqs);
    }
  };

  aAdapter.pairingReqs.onenterpincodereq = function onEnterPinCodeReq(evt) {
    ok(evt.handle instanceof BluetoothPairingHandle, "type checking for handle.");
    ok(typeof evt.deviceName === 'string', "type checking for deviceName.");
    log("  - Received 'onenterpincodereq' event.");

    if (!aSpecifiedBdName || evt.deviceName == aSpecifiedBdName) {
      // TODO: Allow user to enter pincode.
      let UserEnteredPinCode = "0000";
      let pinCode = UserEnteredPinCode;

      evt.handle.setPinCode(pinCode).then(
        function onResolve() {
          log("  - 'setPinCode' resolve.");
          cleanupPairingListener(aAdapter.pairingReqs);
        },
        function onReject() {
          log("  - 'setPinCode' reject.");
          cleanupPairingListener(aAdapter.pairingReqs);
        });
    }
  };

  aAdapter.pairingReqs.onpairingconfirmationreq
    = function onPairingConfirmationReq(evt) {
    ok(evt.handle instanceof BluetoothPairingHandle, "type checking for handle.");
    ok(typeof evt.deviceName === 'string', "type checking for deviceName.");
    let passkey = evt.handle.passkey; // passkey for user to confirm
    ok(typeof passkey === 'string', "type checking for passkey.");
    log("  - Received 'onpairingconfirmationreq' event with passkey: " + passkey);

    if (!aSpecifiedBdName || evt.deviceName == aSpecifiedBdName) {
      evt.handle.accept().then(
        function onResolve() {
          log("  - 'accept' resolve.");
          cleanupPairingListener(aAdapter.pairingReqs);
        },
        function onReject() {
          log("  - 'accept' reject.");
          cleanupPairingListener(aAdapter.pairingReqs);
        });
    }
  };

  aAdapter.pairingReqs.onpairingconsentreq = function onPairingConsentReq(evt) {
    ok(evt.handle instanceof BluetoothPairingHandle, "type checking for handle.");
    ok(typeof evt.deviceName === 'string', "type checking for deviceName.");
    log("  - Received 'onpairingconsentreq' event.");

    if (!aSpecifiedBdName || evt.deviceName == aSpecifiedBdName) {
      evt.handle.accept().then(
        function onResolve() {
          log("  - 'accept' resolve.");
          cleanupPairingListener(aAdapter.pairingReqs);
        },
        function onReject() {
          log("  - 'accept' reject.");
          cleanupPairingListener(aAdapter.pairingReqs);
        });
    }
  };
}

/**
 * Clean up event handlers for pairing listener.
 *
 * @param aPairingReqs
 *        A BluetoothPairingListener with event handlers.
 */
function cleanupPairingListener(aPairingReqs) {
  aPairingReqs.ondisplaypasskeyreq = null;
  aPairingReqs.onenterpasskeyreq = null;
  aPairingReqs.onpairingconfirmationreq = null;
  aPairingReqs.onpairingconsentreq = null;
}

/**
 * Compare two uuid arrays to see if them are the same.
 *
 * @param aUuidArray1
 *        An array which contains uuid strings.
 * @param aUuidArray2
 *        Another array which contains uuid strings.
 *
 * @return A boolean value.
 */
function isUuidsEqual(aUuidArray1, aUuidArray2) {
  if (!Array.isArray(aUuidArray1) || !Array.isArray(aUuidArray2)) {
    return false;
  }

  if (aUuidArray1.length != aUuidArray2.length) {
    return false;
  }

  for (let i = 0, l = aUuidArray1.length; i < l; i++) {
    if (aUuidArray1[i] != aUuidArray2[i]) {
      return false;
    }
  }
  return true;
}

/**
 * Wait for pending emulator transactions and call |finish()|.
 */
function cleanUp() {
  // Use ok here so that we have at least one test run.
  ok(true, ":: CLEANING UP ::");

  waitFor(() => finish(), function() {
    return pendingEmulatorCmdCount === 0;
  });
}

/**
 * Wait for the initialization of BluetoothManager and defaultAdapter.
 *
 * Resolve when aManager.defaultAdapter is ready. Never reject
 * If the defaultAdapter is already there, resolved the promise immediately.
 *
 * Fulfill params: (none)
 * @param aManager
 *        A BluetoothManager of the target device.
 * @return A deferred promise.
 */
function waitForManagerInit(aManager) {
  let deferred = Promise.defer();

  if (aManager.defaultAdapter != null) {
    deferred.resolve();
  }

  aManager.onattributechanged = function(evt) {
    if (evt.attrs.indexOf("defaultAdapter") != -1) {
      bluetoothManager.onattributechanged = null;
      ok(true, "BluetoothManager event 'onattributechanged' got.");
      deferred.resolve();
    }
  }

  return deferred.promise;
}

/**
 * Wait for 'onconnectionstatechanged' events for state changes of BluetoothGatt
 * with specified order.
 *
 * Resolve if those expected events occur in order. Never reject.
 *
 * Fulfill params: (none)
 *
 * @param aGatt
 *        The BluetoothGatt you want to use.
 * @param aStateChangesInOrder
 *        An array which contains an expected order of BluetoothConnectionState.
 *        Example 1: ["connecting", "connected"]
 *        Example 2: ["disconnecting", "disconnected"]
 *
 * @return A deferred promise.
 */
function waitForGattConnectionStateChanged(aGatt, aStateChangesInOrder) {
  let deferred = Promise.defer();

  let stateIndex = 0;
  let prevStateIndex = 0;
  let statesArray = [];
  aGatt.onconnectionstatechanged = function() {
    log("  'connectionState' changed to " + aGatt.connectionState);

    // Received state change order may differ from expected one even though
    // state changes in expected order, because the value of connectionState may
    // change again before we receive prior 'onconnectionstatechanged' event.
    let stateIndex = aStateChangesInOrder.indexOf(aGatt.connectionState);
    if (stateIndex >= prevStateIndex && stateIndex + 1 > statesArray.length) {
      statesArray.push(aGatt.connectionState);
      prevStateIndex = stateIndex;

      if (statesArray.length == aStateChangesInOrder.length) {
        aGatt.onconnectionstatechanged  = null;
        ok(true, "BluetoothGatt event 'onconnectionstatechanged' got.");
        deferred.resolve();
      }
    } else {
      ok(false, "The order of 'onconnectionstatechanged' events is unexpected.");
    }
  };

  return deferred.promise;
}

function LookupService(uuid)
{
  switch (uuid) {
    case '00001800-0000-1000-8000-00805f9b34fb': return 'Generic Access Service';
    case '00001801-0000-1000-8000-00805f9b34fb': return 'Generic Attribute Service';
    case '00001802-0000-1000-8000-00805f9b34fb': return 'Immediate Alert Service';
    case '00001803-0000-1000-8000-00805f9b34fb': return 'Link Loss Service';
    case '00001804-0000-1000-8000-00805f9b34fb': return 'Tx Power Service';
    case '00001805-0000-1000-8000-00805f9b34fb': return 'Current Time Service';
    case '00001806-0000-1000-8000-00805f9b34fb': return 'Reference Time Update Service';
    case '00001807-0000-1000-8000-00805f9b34fb': return 'Next DST Change Service';
    case '00001808-0000-1000-8000-00805f9b34fb': return 'Glucose Service';
    case '00001809-0000-1000-8000-00805f9b34fb': return 'Health Thermometer Service';
    case '0000180a-0000-1000-8000-00805f9b34fb': return 'Device Information Service';
    case '0000180d-0000-1000-8000-00805f9b34fb': return 'Heart Rate Service';
    case '0000180e-0000-1000-8000-00805f9b34fb': return 'Phone Alert Status Service';
    case '0000180f-0000-1000-8000-00805f9b34fb': return 'Battery Service';
    case '00001810-0000-1000-8000-00805f9b34fb': return 'Blood Pressure Service';
    case '00001811-0000-1000-8000-00805f9b34fb': return 'Alert Notification Service';
    case '00001812-0000-1000-8000-00805f9b34fb': return 'Human Interface Device Service';
    case '00001813-0000-1000-8000-00805f9b34fb': return 'Scan Parameters Service';
    case '00001814-0000-1000-8000-00805f9b34fb': return 'Running Speed and Cadence Service';
    case '00001816-0000-1000-8000-00805f9b34fb': return 'Cycling Speed and Cadence Service';
    case '00001818-0000-1000-8000-00805f9b34fb': return 'Cycling Power Service';
    case '00001819-0000-1000-8000-00805f9b34fb': return 'Location and Navigation Service';
    case '0000181a-0000-1000-8000-00805f9b34fb': return 'Environmental Sensing Service';
    case '0000181b-0000-1000-8000-00805f9b34fb': return 'Body Composition Service';
    case '0000181c-0000-1000-8000-00805f9b34fb': return 'User Data Service';
    case '0000181d-0000-1000-8000-00805f9b34fb': return 'Weight Scale Service';
    case '0000181e-0000-1000-8000-00805f9b34fb': return 'Bond Management Service';
    case '0000181f-0000-1000-8000-00805f9b34fb': return 'Continuous Glucose Monitoring Service';
    case '00001820-0000-1000-8000-00805f9b34fb': return 'Internet Protocol Support Service';
    default:                                     return 'Unknown Customed Service';
  }
  return null;
}

function LookupCharacteristic(uuid)
{
  switch (uuid) {
    case '00002a00-0000-1000-8000-00805f9b34fb': return 'Device Name';
    case '00002a01-0000-1000-8000-00805f9b34fb': return 'Appearance';
    case '00002a02-0000-1000-8000-00805f9b34fb': return 'Peripheral Privacy Flag';
    case '00002a03-0000-1000-8000-00805f9b34fb': return 'Reconnection Address';
    case '00002a04-0000-1000-8000-00805f9b34fb': return 'Peripheral Preferred Connection Parameters';
    case '00002a05-0000-1000-8000-00805f9b34fb': return 'Service Changed';
    case '00002a06-0000-1000-8000-00805f9b34fb': return 'Alert Level';
    case '00002a07-0000-1000-8000-00805f9b34fb': return 'Tx Power Level';
    case '00002a08-0000-1000-8000-00805f9b34fb': return 'Date Time';
    case '00002a09-0000-1000-8000-00805f9b34fb': return 'Day of Week';
    case '00002a0a-0000-1000-8000-00805f9b34fb': return 'Day Date Time';
    case '00002a0c-0000-1000-8000-00805f9b34fb': return 'Exact Time 256';
    case '00002a0d-0000-1000-8000-00805f9b34fb': return 'DST Offset';
    case '00002a0e-0000-1000-8000-00805f9b34fb': return 'Time Zone';
    case '00002a0f-0000-1000-8000-00805f9b34fb': return 'Local Time Information';
    case '00002a11-0000-1000-8000-00805f9b34fb': return 'Time with DST';
    case '00002a12-0000-1000-8000-00805f9b34fb': return 'Time Accuracy';
    case '00002a13-0000-1000-8000-00805f9b34fb': return 'Time Source';
    case '00002a14-0000-1000-8000-00805f9b34fb': return 'Reference Time Information';
    case '00002a16-0000-1000-8000-00805f9b34fb': return 'Time Update Control Point';
    case '00002a17-0000-1000-8000-00805f9b34fb': return 'Time Update State';
    case '00002a18-0000-1000-8000-00805f9b34fb': return 'Glucose Measurement';
    case '00002a19-0000-1000-8000-00805f9b34fb': return 'Battery Level';
    case '00002a1c-0000-1000-8000-00805f9b34fb': return 'Temperature Measurement';
    case '00002a1d-0000-1000-8000-00805f9b34fb': return 'Temperature Type';
    case '00002a1e-0000-1000-8000-00805f9b34fb': return 'Intermediate Temperature';
    case '00002a21-0000-1000-8000-00805f9b34fb': return 'Measurement Interval';
    case '00002a22-0000-1000-8000-00805f9b34fb': return 'Boot Keyboard Input Report';
    case '00002a23-0000-1000-8000-00805f9b34fb': return 'System ID';
    case '00002a24-0000-1000-8000-00805f9b34fb': return 'Model Number String';
    case '00002a25-0000-1000-8000-00805f9b34fb': return 'Serial Number String';
    case '00002a26-0000-1000-8000-00805f9b34fb': return 'Firmware Revision String';
    case '00002a27-0000-1000-8000-00805f9b34fb': return 'Hardware Revision String';
    case '00002a28-0000-1000-8000-00805f9b34fb': return 'Software Revision String';
    case '00002a29-0000-1000-8000-00805f9b34fb': return 'Manufacturer Name String';
    case '00002a2a-0000-1000-8000-00805f9b34fb': return 'IEEE 11073-20601 Regulatory Certification Data List';
    case '00002a2b-0000-1000-8000-00805f9b34fb': return 'Current Time';
    case '00002a2c-0000-1000-8000-00805f9b34fb': return 'Magnetic Declination';
    case '00002a31-0000-1000-8000-00805f9b34fb': return 'Scan Refresh';
    case '00002a32-0000-1000-8000-00805f9b34fb': return 'Boot Keyboard Output Report';
    case '00002a33-0000-1000-8000-00805f9b34fb': return 'Boot Mouse Input Report';
    case '00002a34-0000-1000-8000-00805f9b34fb': return 'Glucose Measurement Context';
    case '00002a35-0000-1000-8000-00805f9b34fb': return 'Blood Pressure Measurement';
    case '00002a36-0000-1000-8000-00805f9b34fb': return 'Intermediate Cuff Pressure';
    case '00002a37-0000-1000-8000-00805f9b34fb': return 'Heart Rate Measurement';
    case '00002a38-0000-1000-8000-00805f9b34fb': return 'Body Sensor Location';
    case '00002a39-0000-1000-8000-00805f9b34fb': return 'Heart Rate Control Point';
    case '00002a3f-0000-1000-8000-00805f9b34fb': return 'Alert Status';
    case '00002a40-0000-1000-8000-00805f9b34fb': return 'Ringer Control Point';
    case '00002a41-0000-1000-8000-00805f9b34fb': return 'Ringer Setting';
    case '00002a42-0000-1000-8000-00805f9b34fb': return 'Alert Category ID Bit Mask';
    case '00002a43-0000-1000-8000-00805f9b34fb': return 'Alert Category ID';
    case '00002a44-0000-1000-8000-00805f9b34fb': return 'Alert Notification Control Point';
    case '00002a45-0000-1000-8000-00805f9b34fb': return 'Unread Alert Status';
    case '00002a46-0000-1000-8000-00805f9b34fb': return 'New Alert';
    case '00002a47-0000-1000-8000-00805f9b34fb': return 'Supported New Alert Category';
    case '00002a48-0000-1000-8000-00805f9b34fb': return 'Supported Unread Alert Category';
    case '00002a49-0000-1000-8000-00805f9b34fb': return 'Blood Pressure Feature';
    case '00002a4a-0000-1000-8000-00805f9b34fb': return 'HID Information';
    case '00002a4b-0000-1000-8000-00805f9b34fb': return 'Report Map';
    case '00002a4c-0000-1000-8000-00805f9b34fb': return 'HID Control Point';
    case '00002a4d-0000-1000-8000-00805f9b34fb': return 'Report';
    case '00002a4e-0000-1000-8000-00805f9b34fb': return 'Protocol Mode';
    case '00002a4f-0000-1000-8000-00805f9b34fb': return 'Scan Interval Window';
    case '00002a50-0000-1000-8000-00805f9b34fb': return 'PnP ID';
    case '00002a51-0000-1000-8000-00805f9b34fb': return 'Glucose Feature';
    case '00002a52-0000-1000-8000-00805f9b34fb': return 'Record Access Control Point';
    case '00002a53-0000-1000-8000-00805f9b34fb': return 'RSC Measurement';
    case '00002a54-0000-1000-8000-00805f9b34fb': return 'RSC Feature';
    case '00002a55-0000-1000-8000-00805f9b34fb': return 'SC Control Point';
    case '00002a56-0000-1000-8000-00805f9b34fb': return 'Digital';
    case '00002a58-0000-1000-8000-00805f9b34fb': return 'Analog';
    case '00002a5a-0000-1000-8000-00805f9b34fb': return 'Aggregate';
    case '00002a5b-0000-1000-8000-00805f9b34fb': return 'CSC Measurement';
    case '00002a5c-0000-1000-8000-00805f9b34fb': return 'CSC Feature';
    case '00002a5d-0000-1000-8000-00805f9b34fb': return 'Sensor Location';
    case '00002a63-0000-1000-8000-00805f9b34fb': return 'Cycling Power Measurement';
    case '00002a64-0000-1000-8000-00805f9b34fb': return 'Cycling Power Vector';
    case '00002a65-0000-1000-8000-00805f9b34fb': return 'Cycling Power Feature';
    case '00002a66-0000-1000-8000-00805f9b34fb': return 'Cycling Power Control Point';
    case '00002a67-0000-1000-8000-00805f9b34fb': return 'Location and Speed';
    case '00002a67-0000-1000-8000-00805f9b34fb': return 'Navigation';
    case '00002a69-0000-1000-8000-00805f9b34fb': return 'Position Quality';
    case '00002a6a-0000-1000-8000-00805f9b34fb': return 'LN Feature';
    case '00002a6b-0000-1000-8000-00805f9b34fb': return 'LN Control Point';
    case '00002a6c-0000-1000-8000-00805f9b34fb': return 'Elevation';
    case '00002a6d-0000-1000-8000-00805f9b34fb': return 'Pressure';
    case '00002a6e-0000-1000-8000-00805f9b34fb': return 'Temperature';
    case '00002a6f-0000-1000-8000-00805f9b34fb': return 'Humidity';
    case '00002a70-0000-1000-8000-00805f9b34fb': return 'True Wind Speed';
    case '00002a71-0000-1000-8000-00805f9b34fb': return 'True Wind Direction';
    case '00002a72-0000-1000-8000-00805f9b34fb': return 'Apparent Wind Speed';
    case '00002a73-0000-1000-8000-00805f9b34fb': return 'Apparent Wind Direction ';
    case '00002a74-0000-1000-8000-00805f9b34fb': return 'Gust Factor';
    case '00002a75-0000-1000-8000-00805f9b34fb': return 'Pollen Concentration';
    case '00002a76-0000-1000-8000-00805f9b34fb': return 'UV Index';
    case '00002a77-0000-1000-8000-00805f9b34fb': return 'Irradiance';
    case '00002a78-0000-1000-8000-00805f9b34fb': return 'Rainfall';
    case '00002a79-0000-1000-8000-00805f9b34fb': return 'Wind Chill';
    case '00002a7a-0000-1000-8000-00805f9b34fb': return 'Heat Index';
    case '00002a7b-0000-1000-8000-00805f9b34fb': return 'Dew Point';
    case '00002a7d-0000-1000-8000-00805f9b34fb': return 'Descriptor Value Changed';
    case '00002a7e-0000-1000-8000-00805f9b34fb': return 'Aerobic Heart Rate Lower Limit';
    case '00002a7f-0000-1000-8000-00805f9b34fb': return 'Aerobic Threshold';
    case '00002a80-0000-1000-8000-00805f9b34fb': return 'Age';
    case '00002a81-0000-1000-8000-00805f9b34fb': return 'Anaerobic Heart Rate Lower Limit';
    case '00002a82-0000-1000-8000-00805f9b34fb': return 'Anaerobic Heart Rate Upper Limit';
    case '00002a83-0000-1000-8000-00805f9b34fb': return 'Anaerobic Threshold';
    case '00002a84-0000-1000-8000-00805f9b34fb': return 'Aerobic Heart Rate Upper Limit';
    case '00002a85-0000-1000-8000-00805f9b34fb': return 'Date of Birth';
    case '00002a86-0000-1000-8000-00805f9b34fb': return 'Date of Threshold Assessment ';
    case '00002a87-0000-1000-8000-00805f9b34fb': return 'Email Address';
    case '00002a88-0000-1000-8000-00805f9b34fb': return 'Fat Burn Heart Rate Lower Limit';
    case '00002a89-0000-1000-8000-00805f9b34fb': return 'Fat Burn Heart Rate Upper Limit';
    case '00002a8a-0000-1000-8000-00805f9b34fb': return 'First Name';
    case '00002a8b-0000-1000-8000-00805f9b34fb': return 'Five Zone Heart Rate Limits';
    case '00002a8c-0000-1000-8000-00805f9b34fb': return 'Gender';
    case '00002a8d-0000-1000-8000-00805f9b34fb': return 'Heart Rate Max';
    case '00002a8e-0000-1000-8000-00805f9b34fb': return 'Height';
    case '00002a8f-0000-1000-8000-00805f9b34fb': return 'Hip Circumference';
    case '00002a90-0000-1000-8000-00805f9b34fb': return 'Last Name';
    case '00002a91-0000-1000-8000-00805f9b34fb': return 'Maximum Recommended Heart Rate';
    case '00002a92-0000-1000-8000-00805f9b34fb': return 'Resting Heart Rate';
    case '00002a93-0000-1000-8000-00805f9b34fb': return 'Sport Type for Aerobic and Anaerobic Thresholds';
    case '00002a94-0000-1000-8000-00805f9b34fb': return 'Three Zone Heart Rate Limits';
    case '00002a95-0000-1000-8000-00805f9b34fb': return 'Two Zone Heart Rate Limit';
    case '00002a96-0000-1000-8000-00805f9b34fb': return 'VO2 Max';
    case '00002a97-0000-1000-8000-00805f9b34fb': return 'Waist Circumference ';
    case '00002a98-0000-1000-8000-00805f9b34fb': return 'Weight';
    case '00002a99-0000-1000-8000-00805f9b34fb': return 'Database Change Increment';
    case '00002a9a-0000-1000-8000-00805f9b34fb': return 'User Index';
    case '00002a9b-0000-1000-8000-00805f9b34fb': return 'Body Composition Feature';
    case '00002a9c-0000-1000-8000-00805f9b34fb': return 'Body Composition Measurement';
    case '00002a9d-0000-1000-8000-00805f9b34fb': return 'Weight Measurement';
    case '00002a9e-0000-1000-8000-00805f9b34fb': return 'Weight Scale Feature';
    case '00002a9f-0000-1000-8000-00805f9b34fb': return 'User Control Point';
    case '00002aa0-0000-1000-8000-00805f9b34fb': return 'Magnetic Flux Density - 2D';
    case '00002aa1-0000-1000-8000-00805f9b34fb': return 'Magnetic Flux Density - 3D';
    case '00002aa2-0000-1000-8000-00805f9b34fb': return 'Language';
    case '00002aa3-0000-1000-8000-00805f9b34fb': return 'Barometric Pressure Trend';
    case '00002aa4-0000-1000-8000-00805f9b34fb': return 'Bond Management Control Point';
    case '00002aa5-0000-1000-8000-00805f9b34fb': return 'Bond Management Feature';
    case '00002aa6-0000-1000-8000-00805f9b34fb': return 'Central Address Resolution';
    case '00002aa7-0000-1000-8000-00805f9b34fb': return 'CGM Measurement';
    case '00002aa8-0000-1000-8000-00805f9b34fb': return 'GM Feature';
    case '00002aa9-0000-1000-8000-00805f9b34fb': return 'CGM Status';
    case '00002aaa-0000-1000-8000-00805f9b34fb': return 'CGM Session Start Time';
    case '00002aab-0000-1000-8000-00805f9b34fb': return 'CGM Session Run Time';
    case '00002aac-0000-1000-8000-00805f9b34fb': return 'CGM Specific Ops Control Point';
    default:                                     return 'Unknown Customed Characteristic';
  }
  return null;
}

function LoopupDescriptor(uuid)
{
  switch (uuid) {
    case '00002900-0000-1000-8000-00805f9b34fb': return 'Characteristic Extended Properties';
    case '00002901-0000-1000-8000-00805f9b34fb': return 'Characteristic User Description';
    case '00002902-0000-1000-8000-00805f9b34fb': return 'Client Characteristic Configuration';
    case '00002903-0000-1000-8000-00805f9b34fb': return 'Server Characteristic Configuration';
    case '00002904-0000-1000-8000-00805f9b34fb': return 'Characteristic Presentation Format';
    case '00002905-0000-1000-8000-00805f9b34fb': return 'Characteristic Aggregate Format';
    case '00002906-0000-1000-8000-00805f9b34fb': return 'Valid Range';
    case '00002907-0000-1000-8000-00805f9b34fb': return 'External Report Reference';
    case '00002908-0000-1000-8000-00805f9b34fb': return 'Report Reference';
    case '00002909-0000-1000-8000-00805f9b34fb': return 'Number of Digitals';
    case '0000290a-0000-1000-8000-00805f9b34fb': return 'Value Trigger Setting';
    case '0000290b-0000-1000-8000-00805f9b34fb': return 'Environmental Sensing Configuration';
    case '0000290c-0000-1000-8000-00805f9b34fb': return 'Environmental Sensing Measurement ';
    case '0000290d-0000-1000-8000-00805f9b34fb': return 'Environmental Sensing Trigger Setting ';
    case '0000290e-0000-1000-8000-00805f9b34fb': return 'Time Trigger Setting';
    default:                                     return 'Unknown Customed Descriptor';
  }
  return null;
}

function startBluetoothTestBase(aPermissions, aTestCaseMain) {
  ensureBluetoothManager(aPermissions)
    .then(function() {
      log("Wait for creating bluetooth adapter...");
      return waitForManagerInit(bluetoothManager);
    })
    .then(aTestCaseMain)
    .then(cleanUp, function() {
      ok(false, "Unhandled rejected promise.");
      cleanUp();
    });
}

function startBluetoothTest(aReenable, aTestCaseMain) {
  startBluetoothTestBase([], function() {
    let origEnabled, needEnable;
    return Promise.resolve()
      .then(function() {
        origEnabled = getBluetoothEnabled();

        needEnable = !origEnabled;
        log("Original state of bluetooth is " + bluetoothManager.defaultAdapter.state);

        if (origEnabled && aReenable) {
          log("Disable Bluetooth ...");
          needEnable = true;

          isnot(bluetoothManager.defaultAdapter, null,
            "bluetoothManager.defaultAdapter")

          return bluetoothManager.defaultAdapter.disable();
        }
      })
      .then(function() {
        if (needEnable) {
          log("Enable Bluetooth ...");

          isnot(bluetoothManager.defaultAdapter, null,
            "bluetoothManager.defaultAdapter")

          return bluetoothManager.defaultAdapter.enable();
        }
      })
      .then(() => bluetoothManager.defaultAdapter)
      .then(aTestCaseMain)
      .then(function() {
        if (!origEnabled) {
          log("Disable Bluetooth ...");

          return bluetoothManager.defaultAdapter.disable();
        }
      });
  });
}
