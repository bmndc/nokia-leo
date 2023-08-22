/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

[Constructor(DOMString type, optional CaptivePortalLoginEventInit eventInitDict)]
interface CaptivePortalLoginEvent : Event
{
  /**
   * Returns whether or not captive portal login success or not.
   */
  readonly attribute boolean loginSuccess;

  /**
   * Network object with a SSID field describing the network affected by
   * this change.
   */
  readonly attribute any network;
};

dictionary CaptivePortalLoginEventInit : EventInit
{
  boolean loginSuccess = false;
  any network = null;
};
