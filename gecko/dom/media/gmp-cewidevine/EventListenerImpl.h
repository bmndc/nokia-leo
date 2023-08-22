/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */
#ifndef __EventListenerImpl_h__
#define __EventListenerImpl_h__

#include "cdm.h"
#include "CEWidevineSessionCallback.h"

class CEWidevineSessionCallback;

class EventListenerImpl: public widevine::Cdm::IEventListener
{
public:
  // A message (license request, renewal, etc.) to be dispatched to the
  // application's license server.
  // The response, if successful, should be provided back to the CDM via a
  // call to Cdm::update().
  virtual void onMessage(const std::string& session_id, widevine::Cdm::MessageType message_type, const std::string& message) override;

  // There has been a change in the keys in the session or their status.
  virtual void onKeyStatusesChange(const std::string& session_id, bool has_new_usable_key) override;

  // A remove() operation has been completed.
  virtual void onRemoveComplete(const std::string& session_id) override;

  // Called when a deferred action has completed.
  virtual void onDeferredComplete(const std::string& session_id, widevine::Cdm::Status result) override;

  EventListenerImpl(CEWidevineSessionCallback* aCallback): mCallback(aCallback) {};

private:
  CEWidevineSessionCallback* mCallback;
};

#endif  //__EventListenerImpl_h__