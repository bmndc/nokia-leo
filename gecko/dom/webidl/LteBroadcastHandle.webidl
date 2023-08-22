/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

interface LteBroadcastHandleName {
  readonly attribute DOMString name;

  readonly attribute DOMString language;
};

interface LteBroadcastHandle : EventTarget {
  /**
   * The identifier of service handle.
   */
  readonly attribute DOMString handleId;

  /**
   * The locationlization names for display.
   * This information is used for display information.
   */
  [Cached, Pure]
  readonly attribute sequence<LteBroadcastHandleName> names;

  /**
   * Service class
   */
  readonly attribute DOMString class;

  /**
   * Streaming content's language and it is not supported by every device.
   * If this information is unavailable, APP may retrieve it via MPD.
   */
  readonly attribute DOMString? language;

  /**
   * sessionStartTime indicating when this service becomes available.
   */
  readonly attribute unsigned long sessionStartTime;

  /**
   * sessionEndTime indicating when this service stops being avilable.
   */
  readonly attribute unsigned long sessionEndTime;

  /**
   * The 'message' event is notified whenever some handle related events happen.
   * ex: started, stopped.
   * If it is file download related event, file info will be provided.
   * Please refer LteBroadcastMessageEvent for event structure.
   */
  attribute EventHandler onmessage;

  /**
   * All eMBMS handle related error events will go through this callback.
   * What is an error: for those event that middleware cannot recover automatically.
   * If it is file download related event, file info will be provided.
   * Possible erros are:
   *  Streaming related
   *
   *  File download related
   *    "failure",
   *    "expired",
   *    "ioerror",
   *    "outofstorage"
   *
   *  Others
   */
  attribute EventHandler onerror;
};
