/* (c) 2020 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

//enum requestType { "temporary", "permanent" };
//enum unlockedType { "locked", "temp-unlock", "perm-unlock" };
dictionary RsuBlob
{
  DOMString data;        // The data blob from modem
};

dictionary RsuTimer
{
  unsigned long timer;   // The timer of openRF
};

dictionary RsuStatus
{
  unsigned short type;   // The lock status
  unsigned long timer;   // The left time for temp unlock
};

dictionary RsuVersion
{
  unsigned long minorVersion; // The minorVersion
  unsigned long maxVersion;   // The maxVersion
};

[/*CheckAnyPermissions="rsu",*/
 AvailableIn="CertifiedApps"]
interface RemoteSimUnlock {
  /**
   * To get the current lock state.
   *
   * @return Int of lock state.
   */
  [Throws]
  Promise<RsuStatus> getStatus();
  
  /**
   * To get the highest version of binary data supported by modem.
   *
   * @return string.
   */
  [Throws]
  Promise<RsuVersion> getVersion();

  /**
   * To generate binary data for unlock request.
   *
   * @return Int of lock state.
   */
  [Throws]
  Promise<RsuBlob> generateRequest();
  
   /**
   * To unlock device with blob from server.
   *
   * @param type
   *     Optional the binary data for unlock modem.
   *
   * @return Void.
   */
  [Throws]
  Promise<RsuBlob> updataBlob(DOMString data);

   /**
   * To open RF of device temporarily for RSU.
   *
   * @return Void.
   */
  [Throws]
  Promise<RsuTimer> openRF();

   /**
   * To close temporarily opened RF.
   *
   * @return Void.
   */
  [Throws]
  Promise<void> closeRF();

   /**
   * To unlock device temporarily for RSU.
   *
   * @return Void.
   */
  [Throws]
  Promise<void> resetBlob();

  /**
   * To get the allowed unlock type from server.
   *
   * @return  of lock state.
   */
  [Throws]
  Promise<unsigned short> getAllowedType();

  /**
   * To unlock the UICC.
   *
   * @param type
   *     Optional requestType type for different unlock request.
   *
   * @return Void.
   */
  [Throws]
  Promise<void> unlock(optional unsigned short type);

  /**
   * To register with server.
   *
   * @return Void.
   */
  [Throws]
  Promise<void> registerDevice();

  /**
   * To get the SLC version.
   *
   * @return string.
   */
  [Throws]
  Promise<DOMString> getSLCVersion();

  attribute EventHandler onunlockstatuschange;

};
