/* (c) 2020 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */
#include "RSUParent.h"

#include "mozilla/unused.h"
#include "nsArrayUtils.h"
#include "nsServiceManagerUtils.h"

#include <android/log.h>
#undef LOG
#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "RSUParent" , ## args)

namespace mozilla {
namespace dom {

/*
 * Implementation of RSUParent
 */

//NS_IMPL_ISUPPORTS(RSUParent, nsIRSUManagerListener)
NS_IMPL_ISUPPORTS0(RSUParent)

RSUParent::RSUParent() {}

RSUParent::~RSUParent() {}

/* virtual */ void
RSUParent::ActorDestroy(ActorDestroyReason aWhy)
{
  //mService->UnregisterListener(this);
  mService = nullptr;
}

/* virtual */ bool
RSUParent::RecvPRSURequestConstructor(PRSURequestParent* aActor,
                                     const RSUIPCRequest& aRequest)
{
  RSURequestParent* actor = static_cast<RSURequestParent*>(aActor);

  switch (aRequest.type()) {
    case RSUIPCRequest::TRSUGetStatusRequest:
      LOG("RSUIPCRequest::TRSUGetStatusRequest");
      actor->DoRequest(aRequest.get_RSUGetStatusRequest());
      break;
    case RSUIPCRequest::TRSUUnlockRequest:
      actor->DoRequest(aRequest.get_RSUUnlockRequest());
      break;
    case RSUIPCRequest::TRSUGenerateBlobRequest:
      actor->DoRequest(aRequest.get_RSUGenerateBlobRequest());
      break;
    case RSUIPCRequest::TRSUUpdataBlobRequest:
      actor->DoRequest(aRequest.get_RSUUpdataBlobRequest());
      break;
    case RSUIPCRequest::TRSUResetBlobRequest:
      actor->DoRequest(aRequest.get_RSUResetBlobRequest());
      break;
    case RSUIPCRequest::TRSUGetVersionRequest:
      actor->DoRequest(aRequest.get_RSUGetVersionRequest());
      break;
    case RSUIPCRequest::TRSUOpenRFRequest:
      actor->DoRequest(aRequest.get_RSUOpenRFRequest());
      break;
    case RSUIPCRequest::TRSUCloseRFRequest:
      actor->DoRequest(aRequest.get_RSUCloseRFRequest());
      break;
    default:
      MOZ_CRASH("Unknown RSUIPCRequest type");
  }

  return true;
}

/* virtual */ PRSURequestParent*
RSUParent::AllocPRSURequestParent(const RSUIPCRequest& aRequest)
{
  MOZ_ASSERT(mService);
  RefPtr<RSURequestParent> actor = new RSURequestParent(/*mService*/);
  return actor.forget().take();
}

/* virtual */ bool
RSUParent::DeallocPRSURequestParent(PRSURequestParent* aActor)
{
  RefPtr<RSURequestParent> actor =
    dont_AddRef(static_cast<RSURequestParent*>(aActor));
  return true;
}

///* virtual */ bool
/*RSUParent::RecvRegisterListener()
{
  MOZ_ASSERT(mService);
  NS_WARN_IF(
    NS_FAILED(mService->RegisterListener(this)));
  return true;
}*/

///* virtual */ bool
/*RSUParent::RecvUnregisterListener()
{
  MOZ_ASSERT(mService);
  NS_WARN_IF(
    NS_FAILED(mService->UnregisterListener(this)));
  return true;
}

NS_IMETHODIMP
TEEParent::NotifyStatusChange(const uint16_t aStatus)
{
  if (NS_WARN_IF(!SendNotifyStatusChange(aStatus))) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}
*/

/*
 * Implementation of RSURequestCallback
 */

class RSURequestCallback : public nsIRSUManagerCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRSUMANAGERCALLBACK

  explicit RSURequestCallback(RSURequestParent* aRequestParent)
    : mRequestParent(aRequestParent)
  {
    MOZ_ASSERT(aRequestParent);
  }

protected:
  virtual ~RSURequestCallback() {}

  RefPtr<RSURequestParent> mRequestParent;
};

//we can fulfill other callback rather than nsIRSUManagerCallback.
NS_IMPL_ISUPPORTS(RSURequestCallback, nsIRSUManagerCallback)

NS_IMETHODIMP
RSURequestCallback::NotifyGetStatusResponse(uint16_t aStatus, uint32_t aTimer)
{
  MOZ_ASSERT(mRequestParent);
  nsresult rv = mRequestParent->SendResponse(RSUGetStatusResponse(aStatus, aTimer));
  mRequestParent = nullptr;
  return rv;
}

NS_IMETHODIMP
RSURequestCallback::NotifySuccess(void)
{
  MOZ_ASSERT(mRequestParent);
  nsresult rv = mRequestParent->SendResponse(RSUSuccessResponse());
  mRequestParent = nullptr;
  return rv;
}

NS_IMETHODIMP
RSURequestCallback::NotifyError(const nsAString& aError)
{
  MOZ_ASSERT(mRequestParent);
  nsresult rv = mRequestParent->SendResponse(RSUErrorResponse(nsAutoString(aError)));
  mRequestParent = nullptr;
  return rv;
}

NS_IMETHODIMP
RSURequestCallback::NotifyGetBlobResponse(const nsAString& aData)
{
  MOZ_ASSERT(mRequestParent);
  nsresult rv = mRequestParent->SendResponse(RSUGetBlobResponse(nsAutoString(aData)));
  mRequestParent = nullptr;
  return rv;
}

NS_IMETHODIMP
RSURequestCallback::NotifyGetVersionResponse(uint32_t aMinorVersion, uint32_t aMaxVersion)
{
  MOZ_ASSERT(mRequestParent);
  LOG("RSURequestCallback::NotifyGetVersionResponse");
  nsresult rv = mRequestParent->SendResponse(RSUGetVersionResponse(aMinorVersion, aMaxVersion));
  mRequestParent = nullptr;
  return rv;
}

NS_IMETHODIMP
RSURequestCallback::NotifyOpenRFResponse(uint32_t aTimer)
{
  MOZ_ASSERT(mRequestParent);
  LOG("RSURequestCallback::NotifyOpenRFResponse");
  nsresult rv = mRequestParent->SendResponse(RSUOpenRFResponse(aTimer));
  mRequestParent = nullptr;
  return rv;
}

/*
 * Implementation of RSURequestParent
 */

NS_IMPL_ISUPPORTS0(RSURequestParent)

RSURequestParent::RSURequestParent()
{
  MOZ_COUNT_CTOR(RSURequestParent);
}

RSURequestParent::~RSURequestParent()
{
  MOZ_COUNT_DTOR(RSURequestParent);
  //mService = nullptr;
}

/* virtual */ void
RSURequestParent::ActorDestroy(ActorDestroyReason aWhy)
{
  //mService = nullptr;
}

void
RSURequestParent::DoRequest(const RSUGetStatusRequest& aRequest)
{
  //MOZ_ASSERT(mService);
  LOG("RSUIPCRequest::DoRequest RSUGetStatusRequest");
  nsCOMPtr<nsIRSUManagerCallback> callback = new RSURequestCallback(this);
  nsCOMPtr<nsIRSUManager> service = do_GetService(RSU_MANAGER_CONTRACTID);
  nsresult rv = service->GetStatus(callback);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    Unused << SendResponse(RSUErrorResponse(NS_LITERAL_STRING("service error")));
  }
}

void
RSURequestParent::DoRequest(const RSUUnlockRequest& aRequest)
{
  //MOZ_ASSERT(mService);
  nsCOMPtr<nsIRSUManagerCallback> callback = new RSURequestCallback(this);
  nsCOMPtr<nsIRSUManager> service = do_GetService(RSU_MANAGER_CONTRACTID);
  nsresult rv = service->Unlock(callback);

  if (NS_WARN_IF(NS_FAILED(rv))) {
    Unused << SendResponse(RSUErrorResponse(NS_LITERAL_STRING("service error")));
  }
}

void
RSURequestParent::DoRequest(const RSUGenerateBlobRequest& aRequest)
{
  //MOZ_ASSERT(mService);
  nsCOMPtr<nsIRSUManagerCallback> callback = new RSURequestCallback(this);
  nsCOMPtr<nsIRSUManager> service = do_GetService(RSU_MANAGER_CONTRACTID);
  nsresult rv = service->GenerateBlob(callback);

  if (NS_WARN_IF(NS_FAILED(rv))) {
    Unused << SendResponse(RSUErrorResponse(NS_LITERAL_STRING("service error")));
  }
}

void
RSURequestParent::DoRequest(const RSUUpdataBlobRequest& aRequest)
{
  //MOZ_ASSERT(mService);
  nsCOMPtr<nsIRSUManagerCallback> callback = new RSURequestCallback(this);
  nsCOMPtr<nsIRSUManager> service = do_GetService(RSU_MANAGER_CONTRACTID);
  nsresult rv = service->UpdataBlob(aRequest.data(), callback);

  if (NS_WARN_IF(NS_FAILED(rv))) {
    Unused << SendResponse(RSUErrorResponse(NS_LITERAL_STRING("service error")));
  }
}

void
RSURequestParent::DoRequest(const RSUResetBlobRequest& aRequest)
{
  //MOZ_ASSERT(mService);
  nsCOMPtr<nsIRSUManagerCallback> callback = new RSURequestCallback(this);
  nsCOMPtr<nsIRSUManager> service = do_GetService(RSU_MANAGER_CONTRACTID);
  nsresult rv = service->ResetBlob(callback);

  if (NS_WARN_IF(NS_FAILED(rv))) {
    Unused << SendResponse(RSUErrorResponse(NS_LITERAL_STRING("service error")));
  }
}

void
RSURequestParent::DoRequest(const RSUGetVersionRequest& aRequest)
{
  //MOZ_ASSERT(mService);
  nsCOMPtr<nsIRSUManagerCallback> callback = new RSURequestCallback(this);
  nsCOMPtr<nsIRSUManager> service = do_GetService(RSU_MANAGER_CONTRACTID);
  nsresult rv = service->GetVersion(callback);

  if (NS_WARN_IF(NS_FAILED(rv))) {
    Unused << SendResponse(RSUErrorResponse(NS_LITERAL_STRING("service error")));
  }
}

void
RSURequestParent::DoRequest(const RSUOpenRFRequest& aRequest)
{
  //MOZ_ASSERT(mService);
  nsCOMPtr<nsIRSUManagerCallback> callback = new RSURequestCallback(this);
  nsCOMPtr<nsIRSUManager> service = do_GetService(RSU_MANAGER_CONTRACTID);
  nsresult rv = service->OpenRF(callback);

  if (NS_WARN_IF(NS_FAILED(rv))) {
    Unused << SendResponse(RSUErrorResponse(NS_LITERAL_STRING("service error")));
  }
}

void
RSURequestParent::DoRequest(const RSUCloseRFRequest& aRequest)
{
  //MOZ_ASSERT(mService);
  nsCOMPtr<nsIRSUManagerCallback> callback = new RSURequestCallback(this);
  nsCOMPtr<nsIRSUManager> service = do_GetService(RSU_MANAGER_CONTRACTID);
  nsresult rv = service->CloseRF(callback);

  if (NS_WARN_IF(NS_FAILED(rv))) {
    Unused << SendResponse(RSUErrorResponse(NS_LITERAL_STRING("service error")));
  }
}

nsresult
RSURequestParent::SendResponse(const RSUIPCResponse& aResponse)
{
  if (NS_WARN_IF(!Send__delete__(this, aResponse))) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

} // namespace dom
} // namespace mozilla
