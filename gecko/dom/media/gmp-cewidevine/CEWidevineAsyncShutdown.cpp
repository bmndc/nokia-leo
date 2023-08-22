/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#include "CEWidevineAsyncShutdown.h"
#include "CEWidevineUtils.h"
#include "gmp-task-utils.h"

CEWidevineAsyncShutdown::CEWidevineAsyncShutdown(GMPAsyncShutdownHost *aHostAPI)
  : mHost(aHostAPI)
{
  WV_LOG(LogPriority::LOG_DEBUG, "CEWidevineAsyncShutdown::CEWidevineAsyncShutdown");
  AddRef();
}

CEWidevineAsyncShutdown::~CEWidevineAsyncShutdown()
{
  WV_LOG(LogPriority::LOG_DEBUG, "CEWidevineAsyncShutdown::~CEWidevineAsyncShutdown");
}

void
ShutdownTask(CEWidevineAsyncShutdown* aSelf, GMPAsyncShutdownHost* aHost)
{
  WV_LOG(LogPriority::LOG_DEBUG, "CEWidevineAsyncShutdown::BeginShutdown calling ShutdownComplete");
  aHost->ShutdownComplete();
  aSelf->Release();
}

void
CEWidevineAsyncShutdown::BeginShutdown()
{
  WV_LOG(LogPriority::LOG_DEBUG, "CEWidevineAsyncShutdown::BeginShutdown dispatching asynchronous shutdown task");
  GetPlatform()->runonmainthread(WrapTaskNM(ShutdownTask, this, mHost));
}