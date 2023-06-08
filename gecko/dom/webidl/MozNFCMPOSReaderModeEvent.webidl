/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

enum MPOSReaderModeEventType
{
  "MPOS_READER_MODE_FAILED",
  "MPOS_READER_MODE_START_SUCCESS",
  "MPOS_READER_MODE_STOP_SUCCESS",
  "MPOS_READER_MODE_TIMEOUT",
  "MPOS_READER_MODE_REMOVE_CARD",
  "MPOS_READER_MODE_RESTART",
  "MPOS_READER_MODE_TARGET_ACTIVATED",
  "MPOS_READER_MODE_MULTIPLE_TARGETS_DETECTED",
  "MPOS_READER_MODE_INVALID"
};

[Constructor(DOMString type, optional MozNFCMPOSReaderModeEventInit eventInitDict)]
interface MozNFCMPOSReaderModeEvent : Event
{
  readonly attribute MPOSReaderModeEventType eventType;
};

dictionary MozNFCMPOSReaderModeEventInit : EventInit
{
  MPOSReaderModeEventType eventType = "MPOS_READER_MODE_INVALID";
};
