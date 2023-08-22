/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#include "EventListenerImpl.h"

void
EventListenerImpl::onMessage(const std::string& session_id,
                             widevine::Cdm::MessageType message_type,
                             const std::string& message)
{
	WV_LOG(LogPriority::LOG_VERBOSE, "session_id:%s. message_type:%d message size:%d", session_id.c_str(), message_type, message.length());

  WV_LOG(LogPriority::LOG_VERBOSE, "============== start of onMessage ===============");
  std::string output = "";
	for (int i = 0; i != message.length(); i++) {
		if (i % 16 == 0) {
			WV_LOG(LogPriority::LOG_VERBOSE, output.c_str());
			output.clear();
		}
		output += ("0x" + toHex(message[i]) + ", ");
	}
	WV_LOG(LogPriority::LOG_VERBOSE, output.c_str());
	WV_LOG(LogPriority::LOG_VERBOSE, "============== end of onMessage ==============");

  if (mCallback) {
    mCallback->onMessage(session_id, message_type, message);
    return;
  }
  WV_LOG(LogPriority::LOG_ERROR, "onMessage no callback");
}

void
EventListenerImpl::onKeyStatusesChange(const std::string& session_id, bool has_new_usable_key)
{
  if (mCallback) {
    mCallback->onKeyStatusesChange(session_id, has_new_usable_key);
    return;
  }
  WV_LOG(LogPriority::LOG_ERROR, "onKeyStatusesChange no callback");
}

void
EventListenerImpl::onRemoveComplete(const std::string& session_id)
{
  WV_LOG(LogPriority::LOG_ERROR, "onRemoveComplete not supported");
}

void
EventListenerImpl::onDeferredComplete(const std::string& session_id, widevine::Cdm::Status result)
{
  WV_LOG(LogPriority::LOG_ERROR, "onDeferredComplete not supported");
}