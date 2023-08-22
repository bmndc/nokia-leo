/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

enum VideoCallSessionChangeType {
  // reserved
  "unknown",
  // video receiving paused.
  "rx-pause",
  // video receiving resumed.
  "rx-resume",
  // video transmitting started.
  "tx-start",
  // video transmitting stopped.
  "tx-stop",
  // camera encountered some failure.
  "camera-failure",
  // camera got ready.
  "camera-ready"
};

[Constructor(DOMString type, optional VideoCallSessionChangeEventInit eventInitDict)]
interface VideoCallSessionChangeEvent : Event
{
  readonly attribute VideoCallSessionChangeType type;
};

dictionary VideoCallSessionChangeEventInit : EventInit
{
  VideoCallSessionChangeType type = "unknown";
};