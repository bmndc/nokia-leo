/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#ifndef __CEWidevineUtils_h__
#define __CEWidevineUtils_h__

typedef enum {
  // This log level should only be used for |g_cutoff|, in order to silence all
  // logging. It should never be passed to |Log()| as a log level.
  LOG_SILENT = -1,
  LOG_ERROR = 0,
  LOG_WARN = 1,
  LOG_INFO = 2,
  LOG_DEBUG = 3,
  LOG_VERBOSE = 4,
} LogPriority;

#define g_wvAdapterCutoff LOG_ERROR

#if defined(MOZ_WIDGET_GONK)
#include "android/log.h"
#define WV_LOG(level, args...) \
  do { \
    if (level <= g_wvAdapterCutoff) { \
      __android_log_print(ANDROID_LOG_INFO, "wvadapter", ## args); \
    } \
  } while(0)
#else
#define WV_LOG(args...) /* do nothing */
#endif

struct GMPPlatformAPI;
extern GMPPlatformAPI* GetPlatform();

GMPMutex* GMPCreateMutex();

#endif // __CEWidevineUtils_h__