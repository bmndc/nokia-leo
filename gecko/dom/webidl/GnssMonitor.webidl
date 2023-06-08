/* (c) 2018 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

[Pref="geo.gnssMonitor.enabled", CheckAnyPermissions="geolocation"]
interface GnssMonitor : EventTarget {
  // Fired when the platform receives GNSS NMEA sentences
  attribute EventHandler    onnmeaupdate;
};
