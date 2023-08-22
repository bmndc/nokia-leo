/* (c) 2020 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */
#include "nsRSUManager.h"
#include "ipc/RSUIPCService.h"

#include "mozilla/ClearOnShutdown.h"
#include "mozilla/RefPtr.h"
#include "nsCOMPtr.h"
#include "mozilla/dom/ToJSValue.h"
#include "nsXULAppAPI.h"
#include "prinit.h"

#include <android/log.h>
#undef LOG
#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "nsRSUManager" , ## args)

//using namespace mozilla;
//using namespace mozilla::dom;

namespace mozilla {
  namespace dom {

    StaticRefPtr<nsRSUManager> gRSUManager;
    StaticRefPtr<nsIRSUManager> sRSUIPCService;

    class RSUServiceCallback : public nsIRSUCallback
    {
    public:
      NS_DECL_ISUPPORTS
      NS_DECL_NSIRSUCALLBACK

      explicit RSUServiceCallback(nsIRSUManagerCallback* aManagerCallback)
        : mManagerCallback(aManagerCallback)
      {
        MOZ_ASSERT(aManagerCallback);
      }

    protected:
      virtual ~RSUServiceCallback() {}

      RefPtr<nsIRSUManagerCallback> mManagerCallback;
    };
    //Fulfill nsIRSUCallback.
    NS_IMPL_ISUPPORTS(RSUServiceCallback, nsIRSUCallback)

    NS_IMETHODIMP
    RSUServiceCallback::NotifyLockStatus(int32_t status, uint32_t timer)
    {
      LOG("NotifyLockStatus %d", status);
      MOZ_ASSERT(mManagerCallback);
      nsresult rv = mManagerCallback->NotifyGetStatusResponse(status, timer);
      mManagerCallback = nullptr;
      return rv;
    }

    NS_IMETHODIMP
    RSUServiceCallback::NotifySuccess(uint32_t aRequestType)
    {
      LOG("NotifySuccess");
      MOZ_ASSERT(mManagerCallback);
      nsresult rv = mManagerCallback->NotifySuccess();
      mManagerCallback = nullptr;
      return rv;
    }

    NS_IMETHODIMP
    RSUServiceCallback::NotifyError(const nsAString& aError, uint32_t aRequestType)
    {
      LOG("NotifyError %s", ToNewUTF8String(aError));
      MOZ_ASSERT(mManagerCallback);
      nsresult rv = mManagerCallback->NotifyError(aError);
      mManagerCallback = nullptr;
      return rv;
    }

    NS_IMETHODIMP
    RSUServiceCallback::NotifyBlobVersion(uint32_t minorVersion, uint32_t maxVersion)
    {
      LOG("NotifyBlobVersion minorVersion = %d, maxVersion = %d", minorVersion, maxVersion);
      MOZ_ASSERT(mManagerCallback);
      nsresult rv = mManagerCallback->NotifyGetVersionResponse(minorVersion, maxVersion);
      mManagerCallback = nullptr;
      return rv;
    }

    NS_IMETHODIMP
    RSUServiceCallback::NotifyGetBlob(const nsAString& aBlob)
    {
      LOG("NotifyGetBlob %s", ToNewUTF8String(aBlob));
      MOZ_ASSERT(mManagerCallback);
      nsresult rv = mManagerCallback->NotifyGetBlobResponse(aBlob);
      mManagerCallback = nullptr;
      return rv;
    }

    NS_IMETHODIMP
    RSUServiceCallback::NotifyOpenRF(uint32_t aTimer)
    {
      LOG("NotifyOpenRF %d", aTimer);
      MOZ_ASSERT(mManagerCallback);
      nsresult rv = mManagerCallback->NotifyOpenRFResponse(aTimer);
      mManagerCallback = nullptr;
      return rv;
    }

    NS_IMETHODIMP
    RSUServiceCallback::NotifySharedKey(const nsAString& aData)
    {
      MOZ_ASSERT(mManagerCallback);
      return NS_OK;
    }

    NS_IMPL_ISUPPORTS(nsRSUManager, nsIRSUManager)

    nsRSUManager::nsRSUManager()
    {
      MOZ_COUNT_CTOR(nsRSUManager);
      LOG("constructor\n");
    }

    nsRSUManager::~nsRSUManager()
    {
      MOZ_COUNT_DTOR(nsRSUManager);
      LOG("destructor\n");
    }

    already_AddRefed<nsIRSUManager> nsRSUManager::FactoryCreate()
    {
      MOZ_ASSERT(NS_IsMainThread());
      RefPtr<nsIRSUManager> service;
      if (XRE_GetProcessType() != GeckoProcessType_Default) {
        if (sRSUIPCService) {
          // This is a little tricky. But we allow one client
          // to create RSU service only.
          return nullptr;
        }

        sRSUIPCService = new RSUIPCService();
        ClearOnShutdown(&sRSUIPCService);

        service = sRSUIPCService.get();
        return service.forget();
      }

      if (!gRSUManager) {
	gRSUManager = new nsRSUManager();
	ClearOnShutdown(&gRSUManager);
      }

      service = gRSUManager.get();
      return service.forget();
    }

    NS_IMETHODIMP nsRSUManager::GetStatus(nsIRSUManagerCallback* aCallback)
    {
      MOZ_ASSERT(NS_IsMainThread());
      LOG("GetStatus\n");
      nsCOMPtr<nsIRSUCallback> callback = new RSUServiceCallback(aCallback);
      nsCOMPtr<nsIRSURequestService> service = do_GetService(RSU_SERVICE_CONTRACTID);

      nsresult rv = service->GetSimlockStatus(callback);
      return rv;
    }

    //Reserve API
    NS_IMETHODIMP nsRSUManager::Unlock(nsIRSUManagerCallback* aCallback)
    {
      MOZ_ASSERT(NS_IsMainThread());
      return NS_OK;      
    }

    NS_IMETHODIMP nsRSUManager::GenerateBlob(nsIRSUManagerCallback* aCallback)
    {
      MOZ_ASSERT(NS_IsMainThread());
      LOG("UpdataBlob\n");
      nsCOMPtr<nsIRSUCallback> callback = new RSUServiceCallback(aCallback);
      nsCOMPtr<nsIRSURequestService> service = do_GetService(RSU_SERVICE_CONTRACTID);

      nsresult rv = service->GenerateBlob(callback);
      return rv;
    }

    NS_IMETHODIMP nsRSUManager::UpdataBlob(const nsAString& data, nsIRSUManagerCallback* aCallback)
    {
      MOZ_ASSERT(NS_IsMainThread());
      LOG("UpdataBlob\n");
      nsCOMPtr<nsIRSUCallback> callback = new RSUServiceCallback(aCallback);
      nsCOMPtr<nsIRSURequestService> service = do_GetService(RSU_SERVICE_CONTRACTID);

      nsresult rv = service->UpdateBlob(data, callback);
      return rv;
    }

    NS_IMETHODIMP nsRSUManager::ResetBlob(nsIRSUManagerCallback* aCallback)
    {
      MOZ_ASSERT(NS_IsMainThread());
      LOG("ResetBlob\n");
      nsCOMPtr<nsIRSUCallback> callback = new RSUServiceCallback(aCallback);
      nsCOMPtr<nsIRSURequestService> service = do_GetService(RSU_SERVICE_CONTRACTID);

      nsresult rv = service->ResetBlob(callback);
      return rv;
    }

    NS_IMETHODIMP nsRSUManager::GetVersion(nsIRSUManagerCallback* aCallback)
    {
      MOZ_ASSERT(NS_IsMainThread());
      LOG("GetVersion\n");
      nsCOMPtr<nsIRSUCallback> callback = new RSUServiceCallback(aCallback);
      nsCOMPtr<nsIRSURequestService> service = do_GetService(RSU_SERVICE_CONTRACTID);

      nsresult rv = service->GetSimblobVersion(callback);
      return rv;
    }

    NS_IMETHODIMP nsRSUManager::OpenRF(nsIRSUManagerCallback* aCallback)
    {
      MOZ_ASSERT(NS_IsMainThread());
      LOG("GetVersion\n");
      nsCOMPtr<nsIRSUCallback> callback = new RSUServiceCallback(aCallback);
      nsCOMPtr<nsIRSURequestService> service = do_GetService(RSU_SERVICE_CONTRACTID);

      nsresult rv = service->OpenRF(callback);
      return rv;
    }

    NS_IMETHODIMP nsRSUManager::CloseRF(nsIRSUManagerCallback* aCallback)
    {
      MOZ_ASSERT(NS_IsMainThread());
      LOG("GetVersion\n");
      nsCOMPtr<nsIRSUCallback> callback = new RSUServiceCallback(aCallback);
      nsCOMPtr<nsIRSURequestService> service = do_GetService(RSU_SERVICE_CONTRACTID);

      nsresult rv = service->CloseRF(callback);
      return rv;        
    }

  }// dom
}// mozilla

