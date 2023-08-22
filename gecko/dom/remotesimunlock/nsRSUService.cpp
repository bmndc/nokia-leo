/* (c) 2020 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */
#include "nsRSUService.h"
#include "dlfcn.h"
#include <fcntl.h>

#include "mozilla/ClearOnShutdown.h"
#include "mozilla/ModuleUtils.h"
#include "mozilla/RefPtr.h"
#include "nsCOMPtr.h"
#include "mozilla/dom/ToJSValue.h"
#include "nsXULAppAPI.h"
#include "prinit.h"

#include "mozilla/Base64.h"

//wangsong
//#include "mozilla/dom/RSUIPCService.h"
#include "nsRSUManager.h"

//#include "SystemProperty.h"
#include <cutils/properties.h>

#include <android/log.h>
#undef LOG
#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "RSUService" , ## args)

#define  PROPERTY_LIB_PATH  "ro.rsu.lib"
#define  BUFFER_SIZE        4096

using namespace mozilla;
using namespace mozilla::dom;

namespace mozilla {
  namespace dom {

    StaticRefPtr<nsRSUService> gRSUService;
    static UniquePtr<RSUApiService> sRSUApiService;

    void * sRSULib = NULL;
    static PRCallOnceType sInitRsuLib;

    static PRStatus
    InitRsuLib()
    {
      char prop[PROPERTY_VALUE_MAX];
      //property_get(PROPERTY_LIB_PATH, prop, "/system/lib/librsu.so");
      //property_get(PROPERTY_LIB_PATH, prop, "/system/vendor/lib/lib_remote_simlock.so");
      property_get(PROPERTY_LIB_PATH, prop, "/system/vendor/lib/lib_rsu.so");
      sRSULib = dlopen(prop, RTLD_LAZY);
      if(!sRSULib) {
        LOG("dlerror = %s", dlerror());
      }
      return PR_SUCCESS;
    }

    static void *
    GetSharedLibrary()
    {
      PR_CallOnce(&sInitRsuLib, InitRsuLib);
      return sRSULib;
    }

    // Helper to check we have loaded the hardware shared library.
    #define CHECK_HWLIB(ret)                      \
      void* hwLib = GetSharedLibrary();           \
      if (!hwLib) {                               \
        NS_WARNING("Can't find Target library");  \
        return ret;                               \
      }                                           \

    // Defines a function type with the right arguments and return type.
    #define DEFINE_DLFUNC(name, ret, args...) typedef ret (*FUNC##name)(args);

    // Set up a dlsymed function ready to use.
    //MOZ_CRASH
    #define USE_DLFUNC(name)                                                      \
      FUNC##name name = (FUNC##name) dlsym(GetSharedLibrary(), #name);            \
      if (!name) {                                                                \
        NS_WARNING("Symbol not found in shared library : " #name);                \
      }

    class BORQSRSUServiceImpl: public RSUServiceImpl
    {
    public:
      DEFINE_DLFUNC(tsd_invoke, char *, char *)
      char * do_tsd_invoke(char* request) {
        USE_DLFUNC(tsd_invoke)
        return tsd_invoke(request);
      }

      DEFINE_DLFUNC(ModemWrapper_Send_request, int32_t, uint32_t, uint8_t *, uint32_t, uint32_t)
      int32_t do_ModemWrapper_Send_request(uint32_t   request_type,
                                           uint8_t  * buffer_ptr,
                                           uint32_t   buffer_len,
                                           uint32_t   payload_len) {
        USE_DLFUNC(ModemWrapper_Send_request)
        return ModemWrapper_Send_request(request_type, buffer_ptr, buffer_len, payload_len);
      }
    };

    RSUApiService::RSUApiService()
    {
      mImpl = mozilla::MakeUnique<BORQSRSUServiceImpl>();
    }

    bool
    RSUApiService::SendRSURequest(ReqOptions & aOption, ResultOptions & aResult)
    {
      CHECK_HWLIB(false)
      aResult.mType = aOption.mType;

      LOG("enter normal code, try to invoke rsu");
      uint32_t payload_len = 0;
      uint32_t buffer_len = 0;
      uint8_t * buffer = NULL;
      LOG("rsu request type = %d", aOption.mType);
      switch(aOption.mType) {
        case GET_SHARED_KEY:
        case UPDATE_SIMLOCK_SETTINGS:
        case GENERATE_BLOB:
          {
            buffer_len = BUFFER_SIZE;
            buffer = new uint8_t[BUFFER_SIZE];

            //nsAutoCString input;
            nsAutoCString aResult;
            NS_ConvertUTF16toUTF8 request(aOption.mData);

            Base64Decode(request, aResult);
            LOG("decode result length = %d", aResult.Length());
            payload_len = aResult.Length();
            memcpy(buffer, aResult.get(), aResult.Length());
            for(uint32_t i = 0; i < aResult.Length(); i++) {
              LOG("buffer[%d] is %d", i, buffer[i]);
            }
          }
          break;
        case GET_SIMLOCK_VERSION:
          {
            LOG("rsu GET_SIMLOCK_VERSION");
            buffer_len = 4;
            buffer = new uint8_t[buffer_len];
          }
          break;
        case GET_MODEM_STATUS:
          {
            LOG("rsu GET_MODEM_STATUS");
            buffer_len = 16;
            buffer = new uint8_t[buffer_len];
          }
          break;
        case START_DELAY_TIMER:
          {
            buffer_len = 2;
            buffer = new uint8_t[buffer_len];
          }
          break;
        case RESET_SIMLOCK_SETTINGS:
        case STOP_DELAY_TIMER:
          break;
        default:
          return false;
      }

      aResult.mStatus = mImpl->do_ModemWrapper_Send_request(aOption.mType,
                                                           (uint8_t*)buffer,
                                                           buffer_len,
                                                           payload_len);

      LOG("rsu result = %d", aResult.mStatus);
      if(buffer) {
        switch(aOption.mType) {
          case GET_SHARED_KEY:
            {
              nsAutoCString data;
              nsAutoCString result;
              for(int i = 1; i < aResult.mStatus; i++) {
                LOG("buffer[%d] is %d", i, buffer[i]);
                data.Append(buffer[i]);
              }
              Base64Encode(data, result);
              aResult.mResponse.AppendASCII(result.get());
            }
            break;
          case UPDATE_SIMLOCK_SETTINGS:
          case GENERATE_BLOB:
            {
              nsAutoCString data;
              nsAutoCString result;
              for(int i = 0; i < aResult.mStatus; i++) {
                LOG("buffer[%d] is %d", i, buffer[i]);
                data.Append(buffer[i]);
              }
              Base64Encode(data, result);
              aResult.mResponse.AppendASCII(result.get());
            }
            break;
          case GET_SIMLOCK_VERSION:
            {
              aResult.mMinorVersion = (buffer[0] << 8) + buffer[1];
              aResult.mMaxVersion = (buffer[2] << 8) + buffer[3];
              LOG("minorVersion = %x", aResult.mMinorVersion);
              LOG("minorVersion = %x", aResult.mMaxVersion);
            }
            break;
          case GET_MODEM_STATUS:
            {
              LOG("modem lock status = %d", buffer[3]);
              aResult.mLockStatus = buffer[3];
              for(int i = 0; i < 16; i++) {
                LOG("buffer[%d] is %d", i, buffer[i]);
                aResult.mResponse.Append(buffer[i]);
              }
              int status = aResult.mResponse.CharAt(3);
              LOG("aResult.mResponse status = %d", status);
              if(status == 1) {
                uint32_t timer = ((aResult.mResponse.CharAt(4)) << 24)
                           + ((aResult.mResponse.CharAt(5)) << 16)
                           + ((aResult.mResponse.CharAt(6)) << 8)
                           + aResult.mResponse.CharAt(7);
                LOG("timer = %d", timer);
                aResult.mTimer = timer;
              }
            }
            break;
          case START_DELAY_TIMER:
            if(aResult.mStatus >= 0) {
              uint32_t timer = (buffer[0] << 8) + buffer[1];
              LOG("timer = %d", timer);
              aResult.mTimer = timer;
            }
            break;
          default:
            aResult.mResponse.AssignASCII((char*)buffer);
            break;
        }
        free(buffer);
      }
      return true;
    }

    class RSUResultDispatcher : public nsRunnable
    {
    public:
      RSUResultDispatcher(ResultOptions & aResult, int32_t aId, void* aCallback/*nsIRSUCallback * aCallback*/)
        : mResult(aResult)
        , mId(aId)
        , mCallback(aCallback)
      {
        LOG("RSUResultDispatcher");
        MOZ_ASSERT(!NS_IsMainThread());
      }

      NS_IMETHOD Run()
      {
        MOZ_ASSERT(NS_IsMainThread());
        LOG("RSUResultDispatcher Run");        
        gRSUService->DispatchResult(mResult, mId, (nsIRSUCallback*)mCallback);
        return NS_OK;
      }

    private:
      ResultOptions mResult;
      int32_t mId;
      //nsCOMPtr<nsIRSUCallback> mCallback;
      void* mCallback;
    };

    class ControlRunnable : public nsRunnable
    {
    public:
      ControlRunnable(ReqOptions aOptions, int32_t aId, nsIRSUCallback* aCallback)
        : mOptions(aOptions)
        , mId(aId)
        , mCallback(aCallback)
      {
        MOZ_ASSERT(NS_IsMainThread());
        LOG("ControlRunnable constructor");
      }

      NS_IMETHOD Run()
      {
        LOG("ControlRunnable run");
        ResultOptions mResult;
        if (sRSUApiService->SendRSURequest(mOptions, mResult)) {
          LOG("sRSUApiService->SendRSURequest, get result");
          //nsCOMPtr<nsIRunnable> runnable = new RSUResultDispatcher(mResult, mId, mCallback);
          NS_DispatchToMainThread(new RSUResultDispatcher(mResult, mId, mCallback));
          return NS_OK;
        }

        LOG("ControlRunnable error");
        mResult.mResponse.AssignASCII("");
        mResult.mStatus = -1;
        NS_DispatchToMainThread(new RSUResultDispatcher(mResult, mId, mCallback));
        return NS_OK;
      }
    private:
       ReqOptions mOptions;
       int32_t mId;
       //nsCOMPtr<nsIRSUCallback> mCallback;
       void* mCallback;
    };


    NS_IMPL_ISUPPORTS(nsRSUService, nsIRSURequestService)

    nsRSUService::nsRSUService()
    {
      MOZ_COUNT_CTOR(nsRSUService);
      LOG("nsRSUService constructor\n");
    }

    nsRSUService::~nsRSUService()
    {
      MOZ_COUNT_DTOR(nsRSUService);
      LOG("nsRSUService destructor\n");
      //Shutdown();
    }

    int32_t nsRSUService::addCallback(nsIRSUCallback* aCallback) {
      if (aCallback && mRSUCallbacks.IndexOf(aCallback) == -1) {
        mRSUCallbacks.AppendObject(aCallback);
        LOG("After append, callback list number is %d", mRSUCallbacks.Length());
      }
      return mRSUCallbacks.IndexOf(aCallback);
    }

    void nsRSUService::removeCallback(nsIRSUCallback* aCallback) {
      if(aCallback && mRSUCallbacks.IndexOf(aCallback) != -1) {
        LOG("remove callback from mRSUCallbacks");
        mRSUCallbacks.RemoveObject(aCallback);
        LOG("After remove, callback list number is %d", mRSUCallbacks.Length());
      }
    }

    void nsRSUService::DispatchResult(ResultOptions & aResult, int32_t aId, nsCOMPtr<nsIRSUCallback> aCallback) 
    {
      MOZ_ASSERT(NS_IsMainThread());
      
      /*if(mRSUCallbacks[aId]) {
        LOG("DispatchResult with id %d", aId);
        mRSUCallbacks[aId]->NotifySuccess(aId);
      }*/

      if (mListener) {
        mListener->OnCommand(aId, aResult.mResponse);
      }

      if(aCallback) {
        LOG("aCallback is not NULL");
        if(aResult.mStatus >= 0) {
          switch(aResult.mType) {
            case GET_SHARED_KEY:
              aCallback->NotifySharedKey(aResult.mResponse);
              break;
            case GENERATE_BLOB:
            case UPDATE_SIMLOCK_SETTINGS:
              aCallback->NotifyGetBlob(aResult.mResponse);
              break;
            case GET_SIMLOCK_VERSION:
              aCallback->NotifyBlobVersion(aResult.mMinorVersion, aResult.mMaxVersion);
              break;
            case GET_MODEM_STATUS:
              aCallback->NotifyLockStatus(aResult.mLockStatus, aResult.mTimer);
              break;
            case START_DELAY_TIMER:
              aCallback->NotifyOpenRF(aResult.mTimer);
              break;
            default:
              aCallback->NotifySuccess(aResult.mType);
              break;
          }
          
        } else {
          switch(aResult.mStatus) {
            case CONNECTION_FAILED:
              aCallback->NotifyError(NS_LITERAL_STRING("CONNECTION_FAILED"), aResult.mType);
              break;
            case UNSUPPORTED_COMMAND:
              aCallback->NotifyError(NS_LITERAL_STRING("UNSUPPORTED_COMMAND"), aResult.mType);
              break;
            case VERIFICATION_FAILED:
              aCallback->NotifyError(NS_LITERAL_STRING("VERIFICATION_FAILED"), aResult.mType);
              break;
            case BUFFER_TOO_SHORT:
              aCallback->NotifyError(NS_LITERAL_STRING("BUFFER_TOO_SHORT"), aResult.mType);
              break;
            case COMMAND_FAILED:
              if(aResult.mType == GET_MODEM_STATUS) {
                aCallback->NotifyLockStatus(aResult.mLockStatus, aResult.mTimer);
              } else {
                aCallback->NotifyError(NS_LITERAL_STRING("COMMAND_FAILED"), aResult.mType);
              }
              break;
            case GET_TIME_FAILED:
              aCallback->NotifyError(NS_LITERAL_STRING("GET_TIME_FAILED"), aResult.mType);
              break;
            case INVALID_CMD_ARGS:
              aCallback->NotifyError(NS_LITERAL_STRING("INVALID_CMD_ARGS"), aResult.mType);
              break;
            case SIMLOCK_RESP_TIMER_EXPIRED:
              aCallback->NotifyError(NS_LITERAL_STRING("TIMER_EXPIRED"), aResult.mType);
              break;
            default:
              aCallback->NotifyError(NS_LITERAL_STRING("unexpected"), aResult.mType);      
          }
        }
        removeCallback(aCallback);
        aCallback = NULL;//New modification
      } else {
        LOG("aCallback is NULL");
      }
    }

    already_AddRefed<nsIRSURequestService> nsRSUService::FactoryCreate()
    {
      LOG("nsRSUService::FactoryCreate");
      if (XRE_GetProcessType() != GeckoProcessType_Default) {
        LOG("nsRSUService::FactoryCreate exception");
        return nullptr;
      }
  
      MOZ_ASSERT(NS_IsMainThread());
  
      if (!gRSUService) {
	gRSUService = new nsRSUService();
	ClearOnShutdown(&gRSUService);
      }

      sRSUApiService = MakeUnique<RSUApiService>();
      ClearOnShutdown(&sRSUApiService);
      LOG("FactoryCreate try to return service");
      RefPtr<nsIRSURequestService> service = gRSUService.get();
      LOG("FactoryCreate OK");
      return service.forget();
    }

    NS_IMETHODIMP nsRSUService::Init(nsIRSUEventListener * aListener)
    {
      MOZ_ASSERT(NS_IsMainThread());
      MOZ_ASSERT(aListener);

      if (!mRsuThread) {
        nsresult rv = NS_NewThread(getter_AddRefs(mRsuThread));
        if (NS_FAILED(rv)) {
          NS_WARNING("Can't create RSU thread");
          Shutdown();
          return NS_ERROR_FAILURE;
        }
      }

      mListener = aListener;
      return NS_OK;
    }

    NS_IMETHODIMP nsRSUService::Shutdown()
    {
      LOG("Shutdown");
      MOZ_ASSERT(NS_IsMainThread());

      if (mRsuThread) {
        mRsuThread->Shutdown();
        mRsuThread = nullptr;
      }

      mListener = nullptr;

      return NS_OK;
    }

    NS_IMETHODIMP nsRSUService::GetSharedKey2(nsIRSUCallback* aCallback)
    {
      MOZ_ASSERT(NS_IsMainThread());
      LOG("GetSharedKey2, aCallback = %p", aCallback);
      if (!mRsuThread) {
        nsresult rv = NS_NewThread(getter_AddRefs(mRsuThread));
        if (NS_FAILED(rv)) {
          NS_WARNING("Can't create RSU thread");
          return NS_ERROR_FAILURE;
        }
      }

      int32_t id = addCallback(aCallback);
      LOG("aCallback id = %d", id);
      nsString request;
      request.AssignASCII("");
      ReqOptions options(GET_SHARED_KEY, request);
      nsCOMPtr<nsIRunnable> runnable = new ControlRunnable(options, id, aCallback);
      mRsuThread->Dispatch(runnable, nsIEventTarget::DISPATCH_NORMAL);
      return NS_OK;
    }

    NS_IMETHODIMP nsRSUService::GetSimblobVersion(nsIRSUCallback* aCallback)
    {
      MOZ_ASSERT(NS_IsMainThread());
      LOG("GetSimblobVersion, aCallback = %p", aCallback);
      if (!mRsuThread) {
        nsresult rv = NS_NewThread(getter_AddRefs(mRsuThread));
        if (NS_FAILED(rv)) {
          NS_WARNING("Can't create RSU thread");
          return NS_ERROR_FAILURE;
        }
      }

      int32_t id = addCallback(aCallback);
      LOG("aCallback id = %d", id);
      nsString request;
      request.AssignASCII("");
      ReqOptions options(GET_SIMLOCK_VERSION, request);
      nsCOMPtr<nsIRunnable> runnable = new ControlRunnable(options, id, aCallback);
      mRsuThread->Dispatch(runnable, nsIEventTarget::DISPATCH_NORMAL);
      return NS_OK;
    }

    NS_IMETHODIMP nsRSUService::UpdateBlob(const nsAString & aBlob, nsIRSUCallback* aCallback)
    {
      MOZ_ASSERT(NS_IsMainThread());
      LOG("UpdateBlob, aCallback = %p", aCallback);
      if (!mRsuThread) {
        nsresult rv = NS_NewThread(getter_AddRefs(mRsuThread));
        if (NS_FAILED(rv)) {
          NS_WARNING("Can't create RSU thread");
          return NS_ERROR_FAILURE;
        }
      }

      int32_t id = addCallback(aCallback);
      LOG("aCallback id = %d", id);

      char * aTempRequest = ToNewUTF8String(aBlob);
      LOG("UpdateBlob, aTempRequest = %s", aTempRequest);
      nsString request;
      request.AssignASCII(aTempRequest);
      ReqOptions options(UPDATE_SIMLOCK_SETTINGS, request);
      nsCOMPtr<nsIRunnable> runnable = new ControlRunnable(options, id, aCallback);
      mRsuThread->Dispatch(runnable, nsIEventTarget::DISPATCH_NORMAL);

      free(aTempRequest);
      return NS_OK;
    }

    NS_IMETHODIMP nsRSUService::ResetBlob(nsIRSUCallback* aCallback)
    {
      MOZ_ASSERT(NS_IsMainThread());
      LOG("ResetBlob, aCallback = %p", aCallback);
      if (!mRsuThread) {
        nsresult rv = NS_NewThread(getter_AddRefs(mRsuThread));
        if (NS_FAILED(rv)) {
          NS_WARNING("Can't create RSU thread");
          return NS_ERROR_FAILURE;
        }
      }

      int32_t id = addCallback(aCallback);
      LOG("aCallback id = %d", id);
      nsString request;
      request.AssignASCII("");
      ReqOptions options(RESET_SIMLOCK_SETTINGS, request);
      nsCOMPtr<nsIRunnable> runnable = new ControlRunnable(options, id, aCallback);
      mRsuThread->Dispatch(runnable, nsIEventTarget::DISPATCH_NORMAL);
      return NS_OK;
    }

    NS_IMETHODIMP nsRSUService::GetSimlockStatus(nsIRSUCallback* aCallback)
    {
      MOZ_ASSERT(NS_IsMainThread());
      LOG("GetSimlockStatus, aCallback = %p", aCallback);
      if (!mRsuThread) {
        nsresult rv = NS_NewThread(getter_AddRefs(mRsuThread));
        if (NS_FAILED(rv)) {
          NS_WARNING("Can't create RSU thread");
          return NS_ERROR_FAILURE;
        }
      }

      int32_t id = addCallback(aCallback);
      LOG("aCallback id = %d", id);
      nsString request;
      request.AssignASCII("");
      ReqOptions options(GET_MODEM_STATUS, request);
      nsCOMPtr<nsIRunnable> runnable = new ControlRunnable(options, id, aCallback);
      mRsuThread->Dispatch(runnable, nsIEventTarget::DISPATCH_NORMAL);
      return NS_OK;
    }

    NS_IMETHODIMP nsRSUService::GenerateBlob(nsIRSUCallback* aCallback)
    {
      MOZ_ASSERT(NS_IsMainThread());
      LOG("GenerateBlob, aCallback = %p", aCallback);
      if (!mRsuThread) {
        nsresult rv = NS_NewThread(getter_AddRefs(mRsuThread));
        if (NS_FAILED(rv)) {
          NS_WARNING("Can't create RSU thread");
          return NS_ERROR_FAILURE;
        }
      }

      int32_t id = addCallback(aCallback);
      LOG("aCallback id = %d", id);
      nsString request;
      request.AssignASCII("");
      ReqOptions options(GENERATE_BLOB, request);
      nsCOMPtr<nsIRunnable> runnable = new ControlRunnable(options, id, aCallback);
      mRsuThread->Dispatch(runnable, nsIEventTarget::DISPATCH_NORMAL);
      return NS_OK;
    }

    NS_IMETHODIMP nsRSUService::OpenRF(nsIRSUCallback* aCallback)
    {
      MOZ_ASSERT(NS_IsMainThread());
      LOG("OpenRF, aCallback = %p", aCallback);
      if (!mRsuThread) {
        nsresult rv = NS_NewThread(getter_AddRefs(mRsuThread));
        if (NS_FAILED(rv)) {
          NS_WARNING("Can't create RSU thread");
          return NS_ERROR_FAILURE;
        }
      }

      int32_t id = addCallback(aCallback);
      LOG("aCallback id = %d", id);
      nsString request;
      request.AssignASCII("");
      ReqOptions options(START_DELAY_TIMER, request);
      nsCOMPtr<nsIRunnable> runnable = new ControlRunnable(options, id, aCallback);
      mRsuThread->Dispatch(runnable, nsIEventTarget::DISPATCH_NORMAL);
      return NS_OK;
    }

    NS_IMETHODIMP nsRSUService::CloseRF(nsIRSUCallback* aCallback)
    {
      MOZ_ASSERT(NS_IsMainThread());
      LOG("CloseRF, aCallback = %p", aCallback);
      if (!mRsuThread) {
        nsresult rv = NS_NewThread(getter_AddRefs(mRsuThread));
        if (NS_FAILED(rv)) {
          NS_WARNING("Can't create RSU thread");
          return NS_ERROR_FAILURE;
        }
      }

      int32_t id = addCallback(aCallback);
      LOG("aCallback id = %d", id);
      nsString request;
      request.AssignASCII("");
      ReqOptions options(STOP_DELAY_TIMER, request);
      nsCOMPtr<nsIRunnable> runnable = new ControlRunnable(options, id, aCallback);
      mRsuThread->Dispatch(runnable, nsIEventTarget::DISPATCH_NORMAL);
      return NS_OK;
    }

  }// dom

  NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(nsIRSUManager, nsRSUManager::FactoryCreate)
  NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(nsIRSURequestService, nsRSUService::FactoryCreate)

  NS_DEFINE_NAMED_CID(RSU_MANAGER_CID);
  NS_DEFINE_NAMED_CID(RSU_SERVICE_CID);


  static const mozilla::Module::CIDEntry kRSUServiceCIDs[] = {
    { &kRSU_MANAGER_CID, false, nullptr, nsIRSUManagerConstructor },
    { &kRSU_SERVICE_CID, false, nullptr, nsIRSURequestServiceConstructor },
    { nullptr }
  };

  static const mozilla::Module::ContractIDEntry kRSUServiceContracts[] = {
    { RSU_MANAGER_CONTRACTID, &kRSU_MANAGER_CID },
    { RSU_SERVICE_CONTRACTID, &kRSU_SERVICE_CID },
    { nullptr }
  };

  static const mozilla::Module kRSUServiceModule = {
    mozilla::Module::kVersion,
    kRSUServiceCIDs,
    kRSUServiceContracts,
    nullptr
  };
}// mozilla
NSMODULE_DEFN(RSUServiceModule) = &kRSUServiceModule;
