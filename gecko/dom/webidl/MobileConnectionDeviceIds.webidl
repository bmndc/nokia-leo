/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

[Pref="dom.mobileconnection.enabled"]
interface MobileConnectionDeviceIds
{
/**
   * IMEI (International Mobile Equipment Identity).
   *
   * Valid if GSM subscription is available.
   */
  readonly attribute DOMString? imei;

  /**
   * IMEIsv (International Mobile Equipment Identity and Software Version).
   *
   * Valid if GSM subscription is available.
   */
  readonly attribute DOMString? imeisv;

  /**
   * ESN (Electronic Serial Number).
   *
   * Valid if CDMA subscription is available.
   */
  readonly attribute DOMString? esn;

  /**
   * MEID (Mobile Equipment Identifier).
   *
   * Valid if CDMA subscription is available.
   */
  readonly attribute DOMString? meid;
};
