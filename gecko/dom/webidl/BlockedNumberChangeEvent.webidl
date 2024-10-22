/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* (c) 2018 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */


[Constructor(DOMString type, optional BlockedNumberChangeEventInit eventInitDict),
 CheckAnyPermissions="contacts-read contacts-write contacts-create"]
interface BlockedNumberChangeEvent : Event
{
  readonly attribute DOMString? number;
  readonly attribute DOMString? reason;
};

dictionary BlockedNumberChangeEventInit : EventInit
{
  DOMString? number = "";
  DOMString reason = "";
};
