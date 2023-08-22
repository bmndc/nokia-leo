/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

[Constructor(DOMString type, optional PowerSupplyEventInit eventInitDict), Pref="dom.powersupply.enabled"]
interface PowerSupplyEvent : Event
{
  readonly attribute boolean powerSupplyOnline;
  // AC/USB/Wireless will be returned if powerSupplyOnline is true,
  // otherwise, Unknown is expected.
  readonly attribute DOMString powerSupplyType;
};

dictionary PowerSupplyEventInit : EventInit
{
  boolean powerSupplyOnline = false;
  DOMString powerSupplyType = "Unknown";
};