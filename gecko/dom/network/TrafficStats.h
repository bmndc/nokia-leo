/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#ifndef TRAFFICSTATS_H
#define TRAFFICSTATS_H

#include "nsITrafficStats.h"
#include "nsString.h"

namespace mozilla {
namespace dom {

class TrafficStats final : public nsITrafficStats
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSITRAFFICSTATS

  TrafficStats();
protected:
  virtual ~TrafficStats();

private:
  struct Stats {
    uint64_t rxBytes;
    uint64_t rxPackets;
    uint64_t txBytes;
    uint64_t txPackets;
    uint64_t tcpRxPackets;
    uint64_t tcpTxPackets;
  };

  uint64_t GetStatsType(const struct Stats* aStats, int32_t aType);
  bool ParseIfaceStats(const nsACString &aInterfaceName,
                       struct Stats* aStats);

};

} // dom
} // mozilla
#endif // TRAFFICSTATS_H
