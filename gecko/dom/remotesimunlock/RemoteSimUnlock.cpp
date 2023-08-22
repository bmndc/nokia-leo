/* (c) 2020 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#include "mozilla/dom/RemoteSimUnlock.h"
#include "nsCOMPtr.h"
#include "RSUManagerCallback.h"

//For system only
#include "nsIAppsService.h"
#include "mozilla/Preferences.h"

namespace mozilla {
namespace dom {
class RemoteSimUnlock::Listener final : public nsIRSUManagerListener
{
private:
  RemoteSimUnlock* mRSU;

public:
  NS_DECL_ISUPPORTS
  NS_FORWARD_SAFE_NSIRSUMANAGERLISTENER(mRSU)

  explicit Listener(RemoteSimUnlock* aRSU)
    : mRSU(aRSU)
  {
    MOZ_ASSERT(mRSU);
  }

  void Disconnect()
  {
    MOZ_ASSERT(mRSU);
    mRSU = nullptr;
  }

private:
  ~Listener()
  {
    MOZ_ASSERT(!mRSU);
  }
};

NS_IMPL_CYCLE_COLLECTION_CLASS(RemoteSimUnlock)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(RemoteSimUnlock,
                                                  DOMEventTargetHelper)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(RemoteSimUnlock,
                                                DOMEventTargetHelper)
  tmp->Shutdown();

NS_IMPL_CYCLE_COLLECTION_UNLINK_END

//QueryInterface implementation for RemoteSimUnlock
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(RemoteSimUnlock)
NS_INTERFACE_MAP_END_INHERITING(DOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(RemoteSimUnlock, DOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(RemoteSimUnlock, DOMEventTargetHelper)

NS_IMPL_ISUPPORTS(RemoteSimUnlock::Listener, nsIRSUManagerListener)

/*NS_IMPL_CYCLE_COLLECTION_INHERITED(RemoteSimUnlock, DOMEventTargetHelper)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(RemoteSimUnlock)
NS_INTERFACE_MAP_END_INHERITING(DOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(RemoteSimUnlock, DOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(RemoteSimUnlock, DOMEventTargetHelper)*/

RemoteSimUnlock::RemoteSimUnlock(nsPIDOMWindowInner* aWindow)
  : DOMEventTargetHelper(aWindow)
{
  BindToOwner(aWindow);
  mListener = new Listener(this);
}
RemoteSimUnlock::~RemoteSimUnlock()
{
  Shutdown();
}

void
RemoteSimUnlock::Shutdown()
{
  if (mListener) {
    mListener->Disconnect();

    /*if (mService) {
      mService->UnregisterListener(mListener);
      mService = nullptr;
    }*/

    mListener = nullptr;
  }
}

JSObject*
RemoteSimUnlock::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return RemoteSimUnlockBinding::Wrap(aCx, this, aGivenProto);
}

//To make API as system only
static nsresult
  GetAppId(nsPIDOMWindowInner* aWindow, uint32_t* aAppId)
{
  *aAppId = nsIScriptSecurityManager::NO_APP_ID;

  nsCOMPtr<nsIDocument> doc = aWindow->GetExtantDoc();
  NS_ENSURE_TRUE(doc, NS_ERROR_FAILURE);

  nsCOMPtr<nsIPrincipal> principal = doc->NodePrincipal();

  nsresult rv = principal->GetAppId(aAppId);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

static nsresult
  GetSystemAppId(uint32_t* aSystemAppId)
{
  *aSystemAppId = nsIScriptSecurityManager::NO_APP_ID;

  nsCOMPtr<nsIAppsService> appsService = do_GetService(APPS_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(appsService, NS_ERROR_FAILURE);

  nsAdoptingString systemAppManifest =
    mozilla::Preferences::GetString("b2g.system_manifest_url");
  NS_ENSURE_TRUE(systemAppManifest, NS_ERROR_FAILURE);

  nsresult rv = appsService->GetAppLocalIdByManifestURL(systemAppManifest, aSystemAppId);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

static nsresult
  GetTestAppId(uint32_t* aTestAppId)
{
  *aTestAppId = nsIScriptSecurityManager::NO_APP_ID;

  nsCOMPtr<nsIAppsService> appsService = do_GetService(APPS_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(appsService, NS_ERROR_FAILURE);

  nsAdoptingString testAppManifest;
  testAppManifest.AssignASCII("app://calculator.gaiamobile.org/manifest.webapp");//For test

  nsresult rv = appsService->GetAppLocalIdByManifestURL(testAppManifest, aTestAppId);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

bool RemoteSimUnlock::IsCalledBySystemOrTestApp()
{
  uint32_t systemAppId;
  nsresult rv = GetSystemAppId(&systemAppId);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return false;
  }

  nsCOMPtr<nsPIDOMWindowInner> window = GetOwner();
  if (!window) {
    return false;
  }

  uint32_t appId;
  rv = GetAppId(window, &appId);
  if (NS_WARN_IF(NS_FAILED(rv))) {
     return false;
  }

  if(appId != systemAppId) {
    uint32_t testAppId;
    nsresult rv = GetTestAppId(&testAppId);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return false;
    }
    return (appId == testAppId);
  } else {
    return true;
  }
}

already_AddRefed<Promise>
  RemoteSimUnlock::GetStatus(ErrorResult& aRv)
{
  /*if (!IsCalledBySystemOrTestApp()) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }*/

  nsCOMPtr<nsIGlobalObject> global = do_QueryInterface(GetOwner());
  if (!global) {
    return nullptr;
  }
  RefPtr<Promise> promise = Promise::Create(global, aRv);
  NS_ENSURE_TRUE(!aRv.Failed(), nullptr);

  nsCOMPtr<nsIRSUManager> service = do_GetService(RSU_MANAGER_CONTRACTID);
  if (NS_WARN_IF(!service)) {
    promise->MaybeReject(NS_ERROR_UNEXPECTED);
    return promise.forget();
  }

  nsCOMPtr<nsIRSUManagerCallback> callback = new RSUManagerCallback(promise);
  nsresult rv = service->GetStatus(callback);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    promise->MaybeReject(NS_ERROR_UNEXPECTED);
    return promise.forget();
  }
  return promise.forget();
}

already_AddRefed<Promise>
  RemoteSimUnlock::GenerateRequest(ErrorResult& aRv)
{
  /*if (!IsCalledBySystemOrTestApp()) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }*/
  
  nsCOMPtr<nsIGlobalObject> global = do_QueryInterface(GetOwner());
  if (!global) {
    return nullptr;
  }
  RefPtr<Promise> promise = Promise::Create(global, aRv);
  NS_ENSURE_TRUE(!aRv.Failed(), nullptr);

  nsCOMPtr<nsIRSUManager> service = do_GetService(RSU_MANAGER_CONTRACTID);
  if (NS_WARN_IF(!service)) {
    promise->MaybeReject(NS_ERROR_UNEXPECTED);
    return promise.forget();
  }

  nsCOMPtr<nsIRSUManagerCallback> callback = new RSUManagerCallback(promise);
  nsresult rv = service->GenerateBlob(callback);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    promise->MaybeReject(NS_ERROR_UNEXPECTED);
    return promise.forget();
  }
  return promise.forget();
}

already_AddRefed<Promise>
  RemoteSimUnlock::UpdataBlob(const nsAString& aData, ErrorResult& aRv)
{
  /*if (!IsCalledBySystemOrTestApp()) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }*/
  
  nsCOMPtr<nsIGlobalObject> global = do_QueryInterface(GetOwner());
  if (!global) {
    return nullptr;
  }
  RefPtr<Promise> promise = Promise::Create(global, aRv);
  NS_ENSURE_TRUE(!aRv.Failed(), nullptr);

  nsCOMPtr<nsIRSUManager> service = do_GetService(RSU_MANAGER_CONTRACTID);
  if (NS_WARN_IF(!service)) {
    promise->MaybeReject(NS_ERROR_UNEXPECTED);
    return promise.forget();
  }

  nsCOMPtr<nsIRSUManagerCallback> callback = new RSUManagerCallback(promise);
  nsresult rv = service->UpdataBlob(aData, callback);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    promise->MaybeReject(NS_ERROR_UNEXPECTED);
    return promise.forget();
  }
  return promise.forget();
}

already_AddRefed<Promise>
  RemoteSimUnlock::GetVersion(ErrorResult& aRv)
{
  /*if (!IsCalledBySystemOrTestApp()) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }*/
  
  nsCOMPtr<nsIGlobalObject> global = do_QueryInterface(GetOwner());
  if (!global) {
    return nullptr;
  }
  RefPtr<Promise> promise = Promise::Create(global, aRv);
  NS_ENSURE_TRUE(!aRv.Failed(), nullptr);

  nsCOMPtr<nsIRSUManager> service = do_GetService(RSU_MANAGER_CONTRACTID);
  if (NS_WARN_IF(!service)) {
    promise->MaybeReject(NS_ERROR_UNEXPECTED);
    return promise.forget();
  }

  nsCOMPtr<nsIRSUManagerCallback> callback = new RSUManagerCallback(promise);
  nsresult rv = service->GetVersion(callback);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    promise->MaybeReject(NS_ERROR_UNEXPECTED);
    return promise.forget();
  }
  return promise.forget();
}

already_AddRefed<Promise>
  RemoteSimUnlock::OpenRF(ErrorResult& aRv)
{
  /*if (!IsCalledBySystemOrTestApp()) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }*/
  
  nsCOMPtr<nsIGlobalObject> global = do_QueryInterface(GetOwner());
  if (!global) {
    return nullptr;
  }
  RefPtr<Promise> promise = Promise::Create(global, aRv);
  NS_ENSURE_TRUE(!aRv.Failed(), nullptr);

  nsCOMPtr<nsIRSUManager> service = do_GetService(RSU_MANAGER_CONTRACTID);
  if (NS_WARN_IF(!service)) {
    promise->MaybeReject(NS_ERROR_UNEXPECTED);
    return promise.forget();
  }

  nsCOMPtr<nsIRSUManagerCallback> callback = new RSUManagerCallback(promise);
  nsresult rv = service->OpenRF(callback);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    promise->MaybeReject(NS_ERROR_UNEXPECTED);
    return promise.forget();
  }
  return promise.forget();
}

already_AddRefed<Promise>
  RemoteSimUnlock::CloseRF(ErrorResult& aRv)
{
  /*if (!IsCalledBySystemOrTestApp()) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }*/
  
  nsCOMPtr<nsIGlobalObject> global = do_QueryInterface(GetOwner());
  if (!global) {
    return nullptr;
  }
  RefPtr<Promise> promise = Promise::Create(global, aRv);
  NS_ENSURE_TRUE(!aRv.Failed(), nullptr);

  nsCOMPtr<nsIRSUManager> service = do_GetService(RSU_MANAGER_CONTRACTID);
  if (NS_WARN_IF(!service)) {
    promise->MaybeReject(NS_ERROR_UNEXPECTED);
    return promise.forget();
  }

  nsCOMPtr<nsIRSUManagerCallback> callback = new RSUManagerCallback(promise);
  nsresult rv = service->CloseRF(callback);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    promise->MaybeReject(NS_ERROR_UNEXPECTED);
    return promise.forget();
  }
  return promise.forget();
}

//Reserv API for future
already_AddRefed<Promise>
  RemoteSimUnlock::ResetBlob(ErrorResult& aRv)
{
  nsCOMPtr<nsIGlobalObject> global = do_QueryInterface(GetOwner());
  if (!global) {
    return nullptr;
  }
  RefPtr<Promise> promise = Promise::Create(global, aRv);
  NS_ENSURE_TRUE(!aRv.Failed(), nullptr);

  nsCOMPtr<nsIRSUManager> service = do_GetService(RSU_MANAGER_CONTRACTID);
  if (NS_WARN_IF(!service)) {
    promise->MaybeReject(NS_ERROR_UNEXPECTED);
    return promise.forget();
  }

  nsCOMPtr<nsIRSUManagerCallback> callback = new RSUManagerCallback(promise);
  nsresult rv = service->ResetBlob(callback);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    promise->MaybeReject(NS_ERROR_UNEXPECTED);
    return promise.forget();
  }
  return promise.forget();
}

already_AddRefed<Promise>
  RemoteSimUnlock::Unlock(const Optional<uint16_t>& aRsuType, ErrorResult& aRv)
{
  nsCOMPtr<nsIGlobalObject> global = do_QueryInterface(GetOwner());
  if (!global) {
    return nullptr;
  }
  RefPtr<Promise> promise = Promise::Create(global, aRv);
  NS_ENSURE_TRUE(!aRv.Failed(), nullptr);
  nsCOMPtr<nsIRSUManager> service = do_GetService(RSU_MANAGER_CONTRACTID);
  if (NS_WARN_IF(!service)) {
    promise->MaybeReject(NS_ERROR_UNEXPECTED);
    return promise.forget();
  }
  /*uint16_t rsuType = 3;
  if (aRsuType.WasPassed()) {
    rsuType = aRsuType.Value();
  }*/
  nsCOMPtr<nsIRSUManagerCallback> callback = new RSUManagerCallback(promise);
  nsresult rv = service->Unlock(callback);//To be filled with unlock type
  if (NS_WARN_IF(NS_FAILED(rv))) {
    promise->MaybeReject(NS_ERROR_UNEXPECTED);
    return promise.forget();
  }
  return promise.forget();
}

already_AddRefed<Promise>
  RemoteSimUnlock::GetAllowedType(ErrorResult& aRv)
{
  nsCOMPtr<nsIGlobalObject> global = do_QueryInterface(GetOwner());
  if (!global) {
    return nullptr;
  }
  RefPtr<Promise> promise = Promise::Create(global, aRv);
  promise->MaybeReject(NS_ERROR_UNEXPECTED);
  return promise.forget();
}

already_AddRefed<Promise>
  RemoteSimUnlock::RegisterDevice(ErrorResult& aRv)
{
  nsCOMPtr<nsIGlobalObject> global = do_QueryInterface(GetOwner());
  if (!global) {
    return nullptr;
  }
  RefPtr<Promise> promise = Promise::Create(global, aRv);
  promise->MaybeReject(NS_ERROR_UNEXPECTED);
  return promise.forget();
}

already_AddRefed<Promise>
  RemoteSimUnlock::GetSLCVersion(ErrorResult& aRv)
{
  nsCOMPtr<nsIGlobalObject> global = do_QueryInterface(GetOwner());
  if (!global) {
    return nullptr;
  }
  RefPtr<Promise> promise = Promise::Create(global, aRv);
  promise->MaybeReject(NS_ERROR_UNEXPECTED);
  return promise.forget();
}

NS_IMETHODIMP
RemoteSimUnlock::NotifyStatusChange(uint16_t status)
{
  return NS_OK;
}

} // namespace dom
} // namespace mozilla


