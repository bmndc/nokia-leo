/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

/**
 * MemoryCleanerManager provides some APIs to release memory and send event when
 * memory is higher than threshold.
 */

enum AnimationStatus {
  "started",
  "stage1",
  "stage1tostage2",
  "stage2",
  "completed",
  "stopped",
  "unknown"
};

[CheckAnyPermissions="embed-apps"]
interface MemoryCleanerManager : EventTarget {
  attribute AnimationStatus status;
  [Throws]
  void reset();
  attribute EventHandler onb2gmemoryoverused;
};
