/* (c) 2018 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

[CheckAnyPermissions="geolocation",
 Constructor(DOMString type,
             optional NmeaEventInit eventInitDict)]
interface NmeaEvent : Event
{
  readonly attribute long long    timestamp;
  readonly attribute DOMString    nmea;
};

dictionary NmeaEventInit : EventInit
{
  long long timestamp = 0;
  DOMString nmea = "";
};
