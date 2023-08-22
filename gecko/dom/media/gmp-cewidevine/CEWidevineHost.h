/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */
#ifndef __CEWidevineHost_h__
#define __CEWidevineHost_h__

#include <queue>
#include "cdm.h"

class CEWidevineHost : public widevine::Cdm::IStorage,
                       public widevine::Cdm::IClock,
                       public widevine::Cdm::ITimer {
public:
  CEWidevineHost();
  bool read(const std::string& name, std::string* data) override;
  bool write(const std::string& name, const std::string& data) override;
  bool exists(const std::string& name) override;
  bool remove(const std::string& name) override;
  int32_t size(const std::string& name) override;
  bool list(std::vector<std::string>* names) override;
  int64_t now() override;
  void setTimeout(int64_t delay_ms, IClient* client, void* context) override;
  void cancel(IClient* client) override;

private:
  struct Timer {
    Timer(int64_t expiry_time, IClient* client, void* context)
        : expiry_time_(expiry_time), client_(client), context_(context) {}

    bool operator<(const Timer& other) const {
      // We want to reverse the order so that the smallest expiry times go to
      // the top of the priority queue.
      return expiry_time_ > other.expiry_time_;
    }

    int64_t expiry_time() { return expiry_time_; }
    IClient* client() { return client_; }
    void* context() { return context_; }

  private:
    int64_t expiry_time_;
    IClient* client_;
    void* context_;
  };

  void Reset();
  int64_t now_;
  std::priority_queue<Timer> timers_;
  typedef std::map<std::string, std::string> StorageMap;
  StorageMap mFiles;
};

#endif  // __CEWidevineHost_h__
