/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
* file or any portion thereof may not be reproduced or used in any manner
* whatsoever without the express written permission of KAI OS TECHNOLOGIES
* (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
* LIMITED or its affiliate company and may be registered in some jurisdictions.
* All other trademarks are the property of their respective owners.
*/

[JSImplementation="@kaiostech.com/donglemanager;1",
 NavigatorProperty="dongleManager",
 CheckAnyPermissions="donglemanager",
 UnsafeInPrerendering]
interface DongleManager : EventTarget {
  /**
   * Returns whether or not dongle is currently connected.
   */
  readonly attribute boolean dongleStatus;

  /**
   * Returns device usb server ip address.
   */
  readonly attribute DOMString usbIpAddress;

  /**
   * An event listener that is called with notification about the dongle
   * connection status.
   */
  attribute EventHandler ondonglestatuschange;
};
