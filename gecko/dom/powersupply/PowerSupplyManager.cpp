/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <limits>
#include "PowerSupplyManager.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/Hal.h"
#include "mozilla/dom/PowerSupplyEvent.h"
#include "mozilla/dom/PowerSupplyManagerBinding.h"
#include "nsIDOMClassInfo.h"

/**
 * We have to use macros here because our leak analysis tool things we are
 * leaking strings when we have |static const nsString|. Sad :(
 */
#define POWERSUPPLY_STATUS_CHANGE_NAME NS_LITERAL_STRING("powersupplystatuschanged")

namespace mozilla {
namespace dom {
namespace powersupply {

PowerSupplyManager::PowerSupplyManager(nsPIDOMWindowInner* aWindow)
  : DOMEventTargetHelper(aWindow)
  , mPowerSupplyOnline(false)
  , mPowerSupplyType(NS_LITERAL_STRING("Unknown"))
{
}

void
PowerSupplyManager::Init()
{
  hal::RegisterPowerSupplyObserver(this);

  hal::PowerSupplyStatus powerSupplyStatus;
  hal::GetCurrentPowerSupplyStatus(&powerSupplyStatus);

  UpdateFromPowerSupplyStatus(powerSupplyStatus);
}

void
PowerSupplyManager::Shutdown()
{
  hal::UnregisterPowerSupplyObserver(this);
}

JSObject*
PowerSupplyManager::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return PowerSupplyManagerBinding::Wrap(aCx, this, aGivenProto);
}

bool
PowerSupplyManager::PowerSupplyOnline() const
{
  return mPowerSupplyOnline;
}

void
PowerSupplyManager::GetPowerSupplyType(nsString& aRetVal) const
{
  aRetVal = mPowerSupplyType;
}

void
PowerSupplyManager::UpdateFromPowerSupplyStatus(const hal::PowerSupplyStatus& aPowerSupplyStatus)
{
  mPowerSupplyOnline = aPowerSupplyStatus.powerSupplyOnline();
  mPowerSupplyType = NS_ConvertUTF8toUTF16(aPowerSupplyStatus.powerSupplyType());
}

void
PowerSupplyManager::Notify(const hal::PowerSupplyStatus& aPowerSupplyStatus)
{
  bool previousPowerSupplyOnline = mPowerSupplyOnline;

  UpdateFromPowerSupplyStatus(aPowerSupplyStatus);

  if (previousPowerSupplyOnline != mPowerSupplyOnline) {
    PowerSupplyEventInit init;
    init.mPowerSupplyOnline = mPowerSupplyOnline;
    init.mPowerSupplyType = mPowerSupplyType;
    RefPtr<PowerSupplyEvent> event = PowerSupplyEvent::Constructor(this,
                                                       POWERSUPPLY_STATUS_CHANGE_NAME,
                                                       init);
    DispatchTrustedEvent(event);
  }
}

} // namespace powersupply
} // namespace dom
} // namespace mozilla
