/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_MobileDeviceIdentities_h
#define mozilla_dom_MobileDeviceIdentities_h

#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/dom/MobileConnectionDeviceIdsBinding.h"
#include "nsIMobileDeviceIdentities.h"
#include "nsWrapperCache.h"

struct JSContext;

namespace mozilla {
namespace dom {

class MobileDeviceIdentities final : public nsIMobileDeviceIdentities
                                       , public nsWrapperCache
{
public:
  NS_DECL_NSIMOBILEDEVICEIDENTITIES
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(MobileDeviceIdentities)

  explicit MobileDeviceIdentities(nsPIDOMWindowInner* aWindow);

  MobileDeviceIdentities(const nsAString& aImei, const nsAString& aImeisv,
                         const nsAString& aEsn, const nsAString& aMeid);

  nsPIDOMWindowInner*
  GetParentObject() const
  {
    return mWindow;
  }

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

private:
  ~MobileDeviceIdentities();
  nsCOMPtr<nsPIDOMWindowInner> mWindow;
  nsString mImei;
  nsString mImeisv;
  nsString mEsn;
  nsString mMeid;
};

} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_MobileDeviceIdentities_h
