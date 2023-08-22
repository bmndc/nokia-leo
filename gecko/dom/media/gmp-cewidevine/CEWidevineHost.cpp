/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */
#include "CEWidevineHost.h"
#include <chrono>

using namespace widevine;

const std::string kWildcard = "*";

CEWidevineHost::CEWidevineHost()
{
  Reset();
}

void
CEWidevineHost::Reset()
{
  auto now = std::chrono::steady_clock().now();
  now_ = now.time_since_epoch() / std::chrono::milliseconds(1);

  // Surprisingly, std::priority_queue has no clear().
  while (!timers_.empty()) {
    timers_.pop();
  }

  mFiles.clear();
}

bool
CEWidevineHost::read(const std::string& name, std::string* data)
{
  StorageMap::iterator it = mFiles.find(name);
  bool ok = it != mFiles.end();
  if (!ok) {
    return false;
  }
  *data = it->second;
  return true;
}

bool
CEWidevineHost::write(const std::string& name, const std::string& data)
{
  mFiles[name] = data;
  return true;
}

bool
CEWidevineHost::exists(const std::string& name)
{
  StorageMap::iterator it = mFiles.find(name);
  bool ok = it != mFiles.end();
  return ok;
}

bool
CEWidevineHost::remove(const std::string& name)
{
  WV_LOG(LogPriority::LOG_DEBUG, "remove: %s\n", name.c_str());

  size_t wildcard_pos = name.find(kWildcard);
  if (wildcard_pos == std::string::npos) { // normal file
    if (!exists(name)) {
      WV_LOG(LogPriority::LOG_ERROR, "don't exist: %s\n", name.c_str());
      return false;
    }
    mFiles.erase(name);
  } else {
    std::string partA = name.substr(0, wildcard_pos);
    std::string partB = name.substr(wildcard_pos + 1);

    std::vector<std::string> fileToRemove;

    for (std::map<std::string, std::string>::iterator it = mFiles.begin(); it != mFiles.end(); ++it) {
      std::string curFileName = it->first;
      if (curFileName.size() < (partA.size() + partB.size())) {
        continue;
      }
      if (curFileName.find(partA) == 0 && curFileName.rfind(partB) == (curFileName.length() - partB.length())) {
        fileToRemove.push_back(curFileName);
      }
    }

    for (std::vector<std::string>::iterator it = fileToRemove.begin(); it != fileToRemove.end(); it++){
      WV_LOG(LogPriority::LOG_DEBUG, "ready to remove: %s\n", (*it).c_str());
      if (!remove((*it).c_str())) {
        WV_LOG(LogPriority::LOG_ERROR, "fail to delete: %s\n", (*it).c_str());
        return false;
      }
    }
  }
  return true;
}

int32_t
CEWidevineHost::size(const std::string& name)
{
  StorageMap::iterator it = mFiles.find(name);
  if (it == mFiles.end()) {
    return -1;
  }
  return it->second.size();
}

bool
CEWidevineHost::list(std::vector<std::string>* names)
{
  names->clear();
  for (StorageMap::iterator it = mFiles.begin(); it != mFiles.end(); it++) {
    names->push_back(it->first);
  }
  return true;
}

int64_t
CEWidevineHost::now()
{
  return now_;
}

void
CEWidevineHost::setTimeout(int64_t delay_ms, IClient* client, void* context)
{
  int64_t expiry_time = now_ + delay_ms;
  timers_.push(Timer(expiry_time, client, context));
}

void
CEWidevineHost::cancel(IClient* client)
{
  // Filter out the timers for this client and put the rest into |others|.
  std::priority_queue<Timer> others;

  while (timers_.size()) {
    Timer t = timers_.top();
    timers_.pop();

    if (t.client() != client) {
      others.push(t);
    }
  }

  // Now swap the queues.
  std::swap(timers_, others);
}
