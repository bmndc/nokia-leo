/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#include "CEWidevineSessionCallback.h"

CEWidevineSessionCallback::CEWidevineSessionCallback(CEWidevineSessionManager* aSessionManager)
  : mSessionManager(aSessionManager)
{
}

void
CEWidevineSessionCallback::onMessage(const std::string& session_id,
	                                 widevine::Cdm::MessageType message_type,
	                                 const std::string& message)
{
  mSessionManager->onMessage(session_id, message_type, message);
}

void
CEWidevineSessionCallback::onKeyStatusesChange(const std::string& session_id, bool has_new_usable_key)
{
  mSessionManager->onKeyStatusesChange(session_id, has_new_usable_key);
}