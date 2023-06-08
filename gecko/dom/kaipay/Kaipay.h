/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_Kaipay_h
#define mozilla_dom_Kaipay_h

#include "DOMRequest.h"
#include "mozilla/dom/BindingDeclarations.h"
#include "mozilla/dom/KaipayBinding.h"
#include "mozilla/dom/MozActivityBinding.h"
#include "nsIActivityProxy.h"
#include "mozilla/Preferences.h"
#include "nsPIDOMWindow.h"

namespace mozilla {
namespace dom {

class Kaipay : public DOMRequest
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(Kaipay, DOMRequest)

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  static already_AddRefed<Kaipay>
  Constructor(const GlobalObject& aOwner,
              const KaipayOptions& aOptions,
              ErrorResult& aRv)
  {
    nsCOMPtr<nsPIDOMWindowInner> window = do_QueryInterface(aOwner.GetAsSupports());
    if (!window) {
      aRv.Throw(NS_ERROR_UNEXPECTED);
      return nullptr;
    }

    RefPtr<Kaipay> kaipay = new Kaipay(window);
    aRv = kaipay->Initialize(window, aOwner.Context(), aOptions);
    return kaipay.forget();
  }

  explicit Kaipay(nsPIDOMWindowInner* aWindow);

protected:
  nsresult Initialize(nsPIDOMWindowInner* aWindow,
                      JSContext* aCx,
                      const KaipayOptions& aOptions);

  nsCOMPtr<nsIActivityProxy> mProxy;

  ~Kaipay();
};

} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_Kaipay_h
