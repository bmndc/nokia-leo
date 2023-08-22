/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "KeyboardEventGenerator.h"
#include "KeyboardEvent.h"

#include "nsIDOMWindowUtils.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsWrapperCache.h"

#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/dom/KeyboardEventGeneratorBinding.h"


namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(KeyboardEventGenerator, mWindow)

NS_IMPL_CYCLE_COLLECTING_ADDREF(KeyboardEventGenerator)
NS_IMPL_CYCLE_COLLECTING_RELEASE(KeyboardEventGenerator)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(KeyboardEventGenerator)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

already_AddRefed<KeyboardEventGenerator>
KeyboardEventGenerator::Constructor(const GlobalObject& aGlobal,
                                    ErrorResult& aRv)
{
  nsCOMPtr<nsPIDOMWindowOuter> window =
    do_QueryInterface(aGlobal.GetAsSupports());
  if (!window) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }
  RefPtr<KeyboardEventGenerator> keg =
    new KeyboardEventGenerator(window);

  return keg.forget();
}

KeyboardEventGenerator::KeyboardEventGenerator(nsPIDOMWindowOuter* aWindow)
  : mWindow(aWindow)
{
}

KeyboardEventGenerator::~KeyboardEventGenerator()
{
}

JSObject*
KeyboardEventGenerator::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return KeyboardEventGeneratorBinding::Wrap(aCx, this, aGivenProto);
}

void
KeyboardEventGenerator::Generate(KeyboardEvent& aEvent, ErrorResult& aRv)
{
  if (!XRE_IsParentProcess()) {
    aRv.Throw(NS_ERROR_FAILURE);
    return;
  }

  nsCOMPtr<nsIDOMWindowUtils> domWindowUtils = do_GetInterface(mWindow);
  if (NS_WARN_IF(!domWindowUtils)) {
    aRv.Throw(NS_ERROR_FAILURE);
    return;
  }

  nsString keyType;
  aEvent.GetType(keyType);

  nsString keyName;
  aEvent.GetKey(keyName);

  uint32_t location;
  aEvent.GetLocation(&location);

  int32_t modifiers = 0;
  if (aEvent.CtrlKey()) {
    modifiers |= MODIFIER_CONTROL;
  }
  if (aEvent.AltKey()) {
    modifiers |= MODIFIER_ALT;
  }
  if (aEvent.ShiftKey()) {
    modifiers |= MODIFIER_SHIFT;
  }
  if (aEvent.MetaKey()) {
    modifiers |= MODIFIER_META;
  }

  bool defaultActionTaken = false;

  nsresult rv =
    domWindowUtils->SendKeyEventByKeyName(keyType, keyName, aEvent.KeyCode(),
                                          aEvent.CharCode(), modifiers,
                                          location, &defaultActionTaken);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    aRv.Throw(rv);
  }
}

} // namespace dom
} // namespace mozilla
