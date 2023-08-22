/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_powersuppply_PowerSuppplyManager_h
#define mozilla_dom_powersuppply_PowerSuppplyManager_h

#include "Types.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/Observer.h"
#include "nsCycleCollectionParticipant.h"

namespace mozilla {

namespace hal {
class PowerSupplyStatus;
} // namespace hal

namespace dom {
namespace powersupply {

class PowerSupplyManager : public DOMEventTargetHelper
                         , public PowerSupplyObserver
{
public:
  explicit PowerSupplyManager(nsPIDOMWindowInner* aWindow);

  void Init();
  void Shutdown();

  // For IObserver.
  void Notify(const hal::PowerSupplyStatus& aPowerSupplyStatus) override;

  /**
   * WebIDL Interface
   */
  nsPIDOMWindowInner* GetParentObject() const
  {
     return GetOwner();
  }

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  bool PowerSupplyOnline() const;

  void GetPowerSupplyType(nsString& aRetVal) const;

  IMPL_EVENT_HANDLER(powersupplystatuschanged)

private:
  /**
   * Update the power supply status stored in the powersupply manager object using
   * a PowerSupplyStatus object.
   */
  void  UpdateFromPowerSupplyStatus(const hal::PowerSupplyStatus& aPowerSupplyStatus);
  bool  mPowerSupplyOnline;
  nsString  mPowerSupplyType;
};

} // namespace powersupply
} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_powersupply_PowerSupplyManager_h
