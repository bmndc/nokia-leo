#include "Hal.h"


using namespace mozilla::hal;

namespace mozilla {
namespace hal_impl {

bool
IsFlipOpened()
{
  return true;
}

void
NotifyFlipStateFromInputDevice(bool)
{
}

void
RequestCurrentFlipState()
{
}

void
EnableFlipNotifications()
{
}

void
DisableFlipNotifications()
{
}

} // hal_impl
} // namespace mozilla
