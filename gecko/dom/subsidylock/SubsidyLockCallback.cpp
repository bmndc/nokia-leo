/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#include "SubsidyLockCallback.h"

#include "mozilla/dom/SubsidyLockBinding.h"
#include "mozilla/dom/ToJSValue.h"
#include "nsJSUtils.h"
#include "nsServiceManagerUtils.h"

namespace mozilla {
namespace dom {
namespace subsidylock {

NS_IMPL_ISUPPORTS(SubsidyLockCallback, nsISubsidyLockCallback)

SubsidyLockCallback::SubsidyLockCallback(nsPIDOMWindowInner* aWindow, DOMRequest* aRequest)
  : mWindow(aWindow)
  , mRequest(aRequest)
{
}

nsresult
SubsidyLockCallback::NotifySuccess(JS::Handle<JS::Value> aResult)
{
  nsCOMPtr<nsIDOMRequestService> rs =
    do_GetService(DOMREQUEST_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(rs, NS_ERROR_FAILURE);

  return rs->FireSuccessAsync(mRequest, aResult);
}

// nsISubsidyLockCallback

NS_IMETHODIMP
SubsidyLockCallback::NotifyGetSubsidyLockStatusSuccess(uint32_t aCount,
                                                       uint32_t* aTypes)
{
  nsTArray<uint32_t> result;
  for (uint32_t i = 0; i < aCount; i++) {
    uint32_t type = aTypes[i];
    result.AppendElement(type);
  }

  AutoJSAPI jsapi;
  if (NS_WARN_IF(!jsapi.Init(mWindow))) {
    return NS_ERROR_FAILURE;
  }

  JSContext* cx = jsapi.cx();
  JS::Rooted<JS::Value> jsResult(cx);

  if (!ToJSValue(cx, result, &jsResult)) {
    jsapi.ClearException();
    return NS_ERROR_TYPE_ERR;
  }

  return NotifySuccess(jsResult);
}

NS_IMETHODIMP
SubsidyLockCallback::NotifyError(const nsAString& aError)
{
  nsCOMPtr<nsIDOMRequestService> rs =
    do_GetService(DOMREQUEST_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(rs, NS_ERROR_FAILURE);

  return rs->FireErrorAsync(mRequest, aError);
}

NS_IMETHODIMP
SubsidyLockCallback::NotifySuccess()
{
  return NotifySuccess(JS::UndefinedHandleValue);
}


NS_IMETHODIMP
SubsidyLockCallback::NotifyUnlockSubsidyLockError(const nsAString& aError,
                                                  int32_t aRetryCount)
{
  // Refer to IccCardLockError for detail retry count.
  nsCOMPtr<nsIDOMRequestService> rs =
    do_GetService(DOMREQUEST_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(rs, NS_ERROR_FAILURE);

  return rs->FireErrorAsync(mRequest, aError);
}

} // namespace subsidylock
} // namespace dom
} // namespace mozilla