/* (c) 2020 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#include "RSUIPCService.h"

#include "mozilla/ClearOnShutdown.h"
#include "mozilla/dom/ContentChild.h"
#include "RSUChild.h"

namespace mozilla {
namespace dom {

static StaticAutoPtr<RSUChild> sRSUChild;

NS_IMPL_ISUPPORTS(RSUIPCService, nsIRSUManager)

RSUIPCService::RSUIPCService()
{
  ContentChild* contentChild = ContentChild::GetSingleton();
  if (NS_WARN_IF(!contentChild)) {
    return;
  }

  sRSUChild = new RSUChild(this);
  NS_WARN_IF(!contentChild->SendPRSUConstructor(sRSUChild));
}

RSUIPCService::~RSUIPCService()
{
  sRSUChild = nullptr;
}

NS_IMETHODIMP
RSUIPCService::GetStatus(nsIRSUManagerCallback* aCallback)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!aCallback) {
    return NS_ERROR_INVALID_ARG;
  }
  return SendRequest(aCallback, RSUGetStatusRequest());
}

NS_IMETHODIMP
RSUIPCService::Unlock(nsIRSUManagerCallback* aCallback)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!aCallback) {
    return NS_ERROR_INVALID_ARG;
  }
  return SendRequest(aCallback, RSUUnlockRequest());
}

NS_IMETHODIMP
RSUIPCService::GenerateBlob(nsIRSUManagerCallback* aCallback)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!aCallback) {
    return NS_ERROR_INVALID_ARG;
  }
  return SendRequest(aCallback, RSUGenerateBlobRequest());
}

NS_IMETHODIMP
RSUIPCService::UpdataBlob(const nsAString& data, nsIRSUManagerCallback* aCallback)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!aCallback) {
    return NS_ERROR_INVALID_ARG;
  }
  return SendRequest(aCallback, RSUUpdataBlobRequest(nsString(data)));
}

NS_IMETHODIMP
RSUIPCService::ResetBlob(nsIRSUManagerCallback* aCallback)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!aCallback) {
    return NS_ERROR_INVALID_ARG;
  }
  return SendRequest(aCallback, RSUResetBlobRequest());
}

NS_IMETHODIMP
RSUIPCService::GetVersion(nsIRSUManagerCallback* aCallback)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!aCallback) {
    return NS_ERROR_INVALID_ARG;
  }
  return SendRequest(aCallback, RSUGetVersionRequest());
}

NS_IMETHODIMP
RSUIPCService::OpenRF(nsIRSUManagerCallback* aCallback)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!aCallback) {
    return NS_ERROR_INVALID_ARG;
  }
  return SendRequest(aCallback, RSUOpenRFRequest());
}

NS_IMETHODIMP
RSUIPCService::CloseRF(nsIRSUManagerCallback* aCallback)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!aCallback) {
    return NS_ERROR_INVALID_ARG;
  }
  return SendRequest(aCallback, RSUCloseRFRequest());
}

/*NS_IMETHODIMP
RSUIPCService::RegisterListener(nsIRSUManagerListener* aListener)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!mListener) {
    mListener = aListener;
  }

  if (sRSUChild) {
    NS_WARN_IF(!sRSUChild->SendRegisterListener());
  }

  return NS_OK;
}

NS_IMETHODIMP
RSUIPCService::UnregisterListener(nsIRSUManagerListener* aListener)
{
  MOZ_ASSERT(NS_IsMainThread());

  mListener = nullptr;

  if (sRSUChild) {
    NS_WARN_IF(!sRSUChild->SendUnregisterListener());
  }

  return NS_OK;
}*/

nsresult
RSUIPCService::SendRequest(nsIRSUManagerCallback* aCallback,
                           const RSUIPCRequest& aRequest)
{
  if (sRSUChild) {
    RSURequestChild* actor = new RSURequestChild(aCallback);
    NS_WARN_IF(!sRSUChild->SendPRSURequestConstructor(actor, aRequest));
  }
  return NS_OK;
}

/*void RSUIPCService::Shutdown()
{
  mListener = nullptr;
}*/

/*nsresult
RSUIPCService::NotifyStatusChange(uint16_t aStatus)
{
  if (mListener) {
    mListener->NotifyStatusChange(aStatus);
  }

  return NS_OK;
}*/

} // dom
} // mozilla
