/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#include "Hal.h"


using namespace mozilla::hal;

namespace mozilla {
namespace hal_impl {

bool
GetFlashlightEnabled()
{
  return true;
}

void
SetFlashlightEnabled(bool aEnabled)
{
}

void
RequestCurrentFlashlightState()
{
}

void
EnableFlashlightNotifications()
{
}

void
DisableFlashlightNotifications()
{
}

} // hal_impl
} // namespace mozilla
