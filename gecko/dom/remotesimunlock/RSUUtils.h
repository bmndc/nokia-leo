/* (c) 2020 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#ifndef __RSUUTILS__
#define __RSUUTILS__

#include "nsString.h"
#include "mozilla/UniquePtr.h"

#define GET_SHARED_KEY           1
#define UPDATE_SIMLOCK_SETTINGS  2
#define GET_SIMLOCK_VERSION      3
#define RESET_SIMLOCK_SETTINGS   4
#define GET_MODEM_STATUS         5
#define START_DELAY_TIMER        6
#define GENERATE_BLOB            7
#define STOP_DELAY_TIMER         8

#define REBOOT_REQUEST                 1
#define OK                             0
#define CONNECTION_FAILED             -1
#define UNSUPPORTED_COMMAND           -2
#define VERIFICATION_FAILED           -3
#define BUFFER_TOO_SHORT              -4
#define COMMAND_FAILED                -5
#define GET_TIME_FAILED               -6
#define INVALID_CMD_ARGS              -7
#define SIMLOCK_RESP_TIMER_EXPIRED    -8

/*enum ModemLockState
{
  LOCKED                     = 0,
  TEMPORARY_UNLOCK           = 1,
  PERMANENT_UNLOCK           = 2,
  // For testing a different-length modem state blob that is nonetheless valid
  PERMANENT_UNLOCK_DUALSIM   = 255
};
*/

struct ResultOptions
{
  ResultOptions()
    : mType(0)
    , mStatus(0)
    , mLockStatus(0)
    , mTimer(0)
    , mMinorVersion(0)
    , mMaxVersion(0)
  {

  }

  ResultOptions(uint32_t aType, int32_t aStatus)
    : mType(aType)
    , mStatus(aStatus)
    , mLockStatus(0) { }

  uint32_t mType;
  int32_t mStatus;
  uint32_t mLockStatus;
  uint32_t mTimer;
  uint32_t mMinorVersion;
  uint32_t mMaxVersion;
  nsString mResponse;
};

struct ReqOptions
{
  ReqOptions(uint32_t aType)
    : mType(aType)
  {

  }

  ReqOptions(uint32_t aType, nsString aData)
    : mType(aType)
    , mData(aData)
  {

  }

  uint32_t mType;
  nsString mData;
};

#endif //__RSUUTILS__
