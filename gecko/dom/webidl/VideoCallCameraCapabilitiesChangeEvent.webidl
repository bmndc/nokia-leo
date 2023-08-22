/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

[Constructor(DOMString type, optional VideoCallCameraCapabilitiesChangeEventInit eventInitDict)]
interface VideoCallCameraCapabilitiesChangeEvent : Event
{
  readonly attribute unsigned short width;
  readonly attribute unsigned short height;
  readonly attribute unsigned short maxZoom;
  readonly attribute boolean zoomSupported;
};

dictionary VideoCallCameraCapabilitiesChangeEventInit : EventInit
{
  unsigned short width = 0;
  unsigned short height = 0;
  float maxZoom = 0.0;
  boolean zoomSupported = false;
};