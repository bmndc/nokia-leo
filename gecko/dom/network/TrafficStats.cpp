/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#include "TrafficStats.h"
#include <inttypes.h>

#define QTAGUID_IFACE_STATS   "/proc/net/xt_qtaguid/iface_stat_fmt"

static const uint64_t UNKNOWN = -1;

namespace mozilla {
namespace dom {

TrafficStats::TrafficStats()
{
}

TrafficStats::~TrafficStats()
{
}

uint64_t
TrafficStats::GetStatsType(const struct Stats* aStats, int32_t aType)
{
  switch (aType) {
    case RX_BYTES:
      return aStats->rxBytes;
    case RX_PACKETS:
      return aStats->rxPackets;
    case TX_BYTES:
      return aStats->txBytes;
    case TX_PACKETS:
      return aStats->txPackets;
    case TCP_RX_PACKETS:
      return aStats->tcpRxPackets;
    case TCP_TX_PACKETS:
      return aStats->tcpTxPackets;
    default:
      return UNKNOWN;
  }
}

bool
TrafficStats::ParseIfaceStats(const nsACString& aInterfaceName,
                              struct Stats* aStats)
{
  FILE *fp = fopen(QTAGUID_IFACE_STATS, "r");
  if (!fp) {
    return false;
  }

  char buffer[384];
  char curIface[32];
  bool foundTcp = false;
  uint64_t rxBytes, rxPackets, txBytes, txPackets, tcpRxPackets, tcpTxPackets;

  while (fgets(buffer, sizeof(buffer), fp) != NULL) {

    // Format : wlan0 97 190 28 194 0 0 49 89 48 1 0 0 644 88 84 6
    int matched = sscanf(buffer, "%31s %" SCNu64 " %" SCNu64 " %" SCNu64
            " %" SCNu64 " " "%*u %" SCNu64 " %*u %*u %*u %*u "
            "%*u %" SCNu64 " %*u %*u %*u %*u", curIface, &rxBytes,
            &rxPackets, &txBytes, &txPackets, &tcpRxPackets, &tcpTxPackets);

    if (matched >= 5 &&
        !strcmp(PromiseFlatCString(aInterfaceName).get(), curIface)) {
      aStats->rxBytes += rxBytes;
      aStats->rxPackets += rxPackets;
      aStats->txBytes += txBytes;
      aStats->txPackets += txPackets;
      if (matched == 7) {
        foundTcp = true;
        aStats->tcpRxPackets += tcpRxPackets;
        aStats->tcpTxPackets += tcpTxPackets;
      }
    }
  }

  if (!foundTcp) {
    aStats->tcpRxPackets = UNKNOWN;
    aStats->tcpTxPackets = UNKNOWN;
  }

  if (fclose(fp) != 0) {
    return false;
  }
  return true;
}

//-----------------------------------------------------------------------------
// TrafficStats::nsISupports
//-----------------------------------------------------------------------------

NS_IMPL_ISUPPORTS(TrafficStats, nsITrafficStats)

//-----------------------------------------------------------------------------
// TrafficStats::TrafficStats
//-----------------------------------------------------------------------------

NS_IMETHODIMP
TrafficStats::GetCurrentPacket(const nsACString& aInterfaceName,
                               int32_t aType,
                               uint64_t* aPacketCount)
{
  struct Stats stats;
  memset(&stats, 0, sizeof(Stats));
  if (ParseIfaceStats(aInterfaceName, &stats)) {
    *aPacketCount = GetStatsType(&stats, aType);
  } else {
    *aPacketCount = UNKNOWN;
  }

  return NS_OK;
}

} // dom
} // mozilla
