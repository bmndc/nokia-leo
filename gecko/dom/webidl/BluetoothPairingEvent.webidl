/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

[CheckAnyPermissions="bluetooth",
 Constructor(DOMString type,
             optional BluetoothPairingEventInit eventInitDict)]
interface BluetoothPairingEvent : Event
{
  readonly attribute DOMString               address;
  readonly attribute DOMString               deviceName;

  // BluetoothPairingHandle is only available on bluetooth app which is
  // responsible for replying pairing requests.
  readonly attribute BluetoothPairingHandle? handle;
};

dictionary BluetoothPairingEventInit : EventInit
{
  DOMString address = "";
  DOMString deviceName = "";
  BluetoothPairingHandle? handle = null;
};
