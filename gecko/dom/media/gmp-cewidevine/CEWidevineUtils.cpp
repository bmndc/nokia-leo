/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#include "CEWidevineUtils.h"

GMPMutex*
GMPCreateMutex()
{
  GMPMutex* mutex;
  auto err = GetPlatform()->createmutex(&mutex);
  assert(mutex);
  return GMP_FAILED(err) ? nullptr : mutex;
}

static std::string
toHex(const unsigned char num)
{
  std::string result = "";

  int a = num / 16;
  if (a < 0 || a > 16) {
  	return "";
  }

  if (a < 10) {
    result += a + '0';
  } else {
    a -= 10;
    result += a + 'A';
  }

  int b = num % 16;
  if (b < 0 || b > 16) {
  	return "";
  }

  if (b < 10) {
    result += b + '0';
  } else {
    b -= 10;
    result += b + 'A';
  }
  return result;
}