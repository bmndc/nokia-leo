/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

dictionary lockTypes
{
  sequence<unsigned long> lockTypes;
};

dictionary UnlockOptions
{
  required unsigned long lockType;

  DOMString password;
};

[Pref="dom.subsidylock.enabled"]
interface SubsidyLock
{
  /**
   * Subsidy Lock Type.
   *
   * @See 3GPP TS 22.022, and RIL_PersoSubstate @ ril.h
   */
  // Unknown Key.
  const long SUBSIDY_LOCK_UNKNOWN                  = 0;
  // NCK (Network Control Key).
  const long SUBSIDY_LOCK_SIM_NETWORK              = 1;
  // NSCK (Network Subset Control Key).
  const long SUBSIDY_LOCK_NETWORK_SUBSET           = 2;
  // CCK (Corporate Control Key).
  const long SUBSIDY_LOCK_SIM_CORPORATE            = 3;
  // SPCK (Service Provider Control Key).
  const long SUBSIDY_LOCK_SIM_SERVICE_PROVIDER     = 4;
  // PCK (Personalisation Control Key).
  const long SUBSIDY_LOCK_SIM_SIM                  = 5;
  // PUK for NCK (Network Control Key).
  const long SUBSIDY_LOCK_SIM_NETWORK_PUK          = 6;
  // PUK for NSCK (Network Subset Control Key).
  const long SUBSIDY_LOCK_NETWORK_SUBSET_PUK       = 7;
  // PUK for CCK (Corporate Control Key).
  const long SUBSIDY_LOCK_SIM_CORPORATE_PUK        = 8;
  // PUK for SPCK (Service Provider Control Key).
  const long SUBSIDY_LOCK_SIM_SERVICE_PROVIDER_PUK = 9;
  // PUK for PCK (Personalisation Control Key).
  const long SUBSIDY_LOCK_SIM_SIM_PUK              = 10;


  [Throws, CheckAnyPermissions="mobileconnection"]
  DOMRequest getSubsidyLockStatus();

  [Throws, CheckAnyPermissions="mobileconnection"]
  DOMRequest unlockSubsidyLock(UnlockOptions info);
};