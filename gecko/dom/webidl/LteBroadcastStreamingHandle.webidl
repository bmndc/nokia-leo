/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

enum LteBroadcastHandleAvailability {
  "broadcast",
  "unicast"
};

interface LteBroadcastStreamingHandle : LteBroadcastHandle {
  readonly attribute DOMString mpdUri;

  /**
   * To indicate this handles' availability.
   */
  readonly attribute LteBroadcastHandleAvailability availability;

  /**
   * To start this streaming service.
   * App would receive Promise.resolve() if streaming service get started.
   */
  Promise<void> start();

  /**
   * To stop this streaming service.
   * App would receive Promise.resolve() if streaming service being stoped.
   */
  Promise<void> stop();
};
