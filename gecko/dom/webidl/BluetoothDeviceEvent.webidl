/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

[CheckAnyPermissions="bluetooth",
 Constructor(DOMString type, optional BluetoothDeviceEventInit eventInitDict)]
interface BluetoothDeviceEvent : Event
{
  // should not be null except for BluetoothAdapter.ondeviceunpaired
  //                           and BluetoothAdapter.onpairingaborted
  readonly attribute BluetoothDevice? device;

  // would be null except for BluetoothAdapter.ondeviceunpaired
  //                      and BluetoothAdapter.onpairingaborted
  readonly attribute DOMString?       address;
};

dictionary BluetoothDeviceEventInit : EventInit
{
  BluetoothDevice? device = null;
  DOMString?       address = null;
};
