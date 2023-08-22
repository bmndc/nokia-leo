/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

enum VideoCallSessionStatus {
  // reserved.
  "unknown",
  // request success.
  "success",
  // request failed.
  "fail",
  // request is invalid.
  "invalid",
  // request is timed-out.
  "timed-out",
  // request is rejected by remote.
  "rejected-by-remote"
};

[Constructor(DOMString type, optional VideoCallSessionModifyResponseEventInit eventInitDict)]
interface VideoCallSessionModifyResponseEvent : Event
{
  readonly attribute VideoCallSessionStatus status;
  readonly attribute any request;
  readonly attribute any response;
};

dictionary VideoCallSessionModifyResponseEventInit : EventInit
{
  VideoCallSessionStatus status = "unknown";
  any request = null;
  any response = null;
};