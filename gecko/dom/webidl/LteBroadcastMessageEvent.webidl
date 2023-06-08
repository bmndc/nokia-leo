/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

enum LteBroadcastMessage {
  /**
   * Manager events.
   */
  "coveragechanged",
  "saichanged",

  /**
   * Service events.
   */
  "handlesupdated",

  /**
   * Streaming Handle events.
   */
  // Streaming availability changed.
  "availabilitychanged",
  // Streaming started.
  "started",
  // Streaming stalled due to network or other reason and would be recovered by MW.
  // APP expects to receive streaming_started event once it get recovered.
  "paused",
  // Streaming stopped due to session end or other unrecoverable reasons.
  "stopped",

  /**
   * Download Handle events.
   */
  /* Download status*/
  // Not Downloaded.
  "notdownloaded",
  // Downloading.
  "downloading",
  // Download is complete.
  "succeeded",
  // Download is suspednded.
  "suspended",
  /* Download Progress*/
  // Download progress updated.
  "progressupdated"
};

[Constructor(DOMString type, optional LteBroadcastMessageEventInit eventInitDict)]
interface LteBroadcastMessageEvent : Event {
  readonly attribute LteBroadcastMessage? message;
  /**
   * The file uri if this event is file download related.
   */
  readonly attribute LteBroadcastFile? file;
};

dictionary LteBroadcastMessageEventInit : EventInit
{
  LteBroadcastMessage? message = null;
  LteBroadcastFile? file = null;
};
