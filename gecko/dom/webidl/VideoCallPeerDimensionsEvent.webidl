/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

[Constructor(DOMString type, optional VideoCallPeerDimensionsEventInit eventInitDict)]
interface VideoCallPeerDimensionsEvent : Event
{
  readonly attribute unsigned short width;
  readonly attribute unsigned short height;
};

dictionary VideoCallPeerDimensionsEventInit : EventInit
{
  unsigned short width = 0;
  unsigned short height = 0;
};