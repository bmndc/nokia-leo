/* -*- Mode: IDL; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

dictionary KaipayOptions {
  /**
   * Kaipay will use activities proxy and alway pass "kaipay".
   */
  DOMString name = "kaipay";
  /**
   * A data object passed from caller app to payment app for transaction.
   */
  any data = null;
  /**
   * Enable filter for ActivitiesService, Kaipay do not use it.
   * Disable it by default.
   */
  boolean getFilterResults = false;
};

[Pref="dom.sysmsg.enabled",
 Pref="dom.kaipay.enabled",
 Constructor(optional KaipayOptions options)]
interface Kaipay : DOMRequest {
};
