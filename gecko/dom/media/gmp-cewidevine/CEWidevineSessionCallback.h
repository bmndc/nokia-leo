/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#ifndef __CEWidevineSessionCallback_h__
#define __CEWidevineSessionCallback_h__

#include "CEWidevineSessionManager.h"
#include "cdm.h"

class CEWidevineSessionManager;

class CEWidevineSessionCallback
{
public:
  void onMessage(const std::string& session_id, widevine::Cdm::MessageType message_type, const std::string& message);
  void onKeyStatusesChange(const std::string&, bool);
  CEWidevineSessionCallback(CEWidevineSessionManager*);
private:
  // Warning: Weak ref.
  CEWidevineSessionManager* mSessionManager;
};

#endif  //__CEWidevineSessionCallback_h__