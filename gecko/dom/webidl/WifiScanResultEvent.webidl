/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
* file or any portion thereof may not be reproduced or used in any manner
* whatsoever without the express written permission of KAI OS TECHNOLOGIES
* (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
* LIMITED or its affiliate company and may be registered in some jurisdictions.
* All other trademarks are the property of their respective owners.
*/

[Constructor(DOMString type, optional WifiScanResultEventInit eventInitDict)]
interface WifiScanResultEvent : Event
{
  /**
   * Wifi Scan Result.
   */
  readonly attribute any scanResult;
};

dictionary WifiScanResultEventInit : EventInit
{
  any scanResult = null;
};
