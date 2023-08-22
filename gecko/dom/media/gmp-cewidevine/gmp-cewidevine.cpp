/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#include "CEWidevineAsyncShutdown.h"
#include "CEWidevineSessionManager.h"
#include "CEWidevineUtils.h"

#if defined(WIN32)
#define PUBLIC_FUNC __declspec(dllexport)
#else
#define PUBLIC_FUNC __attribute__((visibility("default")))
#endif

GMPPlatformAPI* g_platform_api = NULL;

GMPPlatformAPI*
GetPlatform()
{
  return g_platform_api;
}

extern "C" {

PUBLIC_FUNC GMPErr
GMPInit(GMPPlatformAPI* aPlatformAPI)
{
  g_platform_api = aPlatformAPI;
  return GMPNoErr;
}

PUBLIC_FUNC GMPErr
GMPGetAPI(const char* aApiName, void* aHostAPI, void** aPluginAPI)
{
  WV_LOG(LogPriority::LOG_DEBUG, "Widevine GMPGetAPI |%s|", aApiName);
  assert(!*aPluginAPI);

  if (!strcmp(aApiName, GMP_API_DECRYPTOR)) {
    *aPluginAPI = new CEWidevineSessionManager();
  } else if (!strcmp(aApiName, GMP_API_ASYNC_SHUTDOWN)) {
    *aPluginAPI = new CEWidevineAsyncShutdown(static_cast<GMPAsyncShutdownHost*>(aHostAPI));
  } else {
    WV_LOG(LogPriority::LOG_ERROR, "GMPGetAPI couldn't resolve API name |%s|", aApiName);
  }

  return *aPluginAPI ? GMPNoErr : GMPNotImplementedErr;
}

PUBLIC_FUNC GMPErr
GMPShutdown(void)
{
  WV_LOG(LogPriority::LOG_DEBUG, "Widevine GMPShutdown");
  g_platform_api = NULL;
  return GMPNoErr;
}

} // extern "C"
