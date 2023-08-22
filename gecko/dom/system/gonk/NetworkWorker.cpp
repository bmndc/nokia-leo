/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "NetworkWorker.h"
#include "NetworkUtils.h"
#include <nsThreadUtils.h>
#include "mozilla/ModuleUtils.h"
#include "mozilla/ClearOnShutdown.h"
#include "mozilla/dom/ScriptSettings.h"
#include "mozilla/dom/ToJSValue.h"
#include "nsXULAppAPI.h"

#define NS_NETWORKWORKER_CID \
  { 0x6df093e1, 0x8127, 0x4fa7, {0x90, 0x13, 0xa3, 0xaa, 0xa7, 0x79, 0xbb, 0xdd} }

using namespace mozilla;
using namespace mozilla::dom;

namespace mozilla {

// The singleton network worker, to be used on the main thread.
StaticRefPtr<NetworkWorker> gNetworkWorker;

// The singleton networkutils class, that can be used on any thread.
static nsAutoPtr<NetworkUtils> gNetworkUtils;

// Runnable used dispatch command result on the main thread.
class NetworkResultDispatcher : public nsRunnable
{
public:
  NetworkResultDispatcher(const NetworkResultOptions& aResult)
    : mResult(aResult)
  {
    MOZ_ASSERT(!NS_IsMainThread());
  }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());

    if (gNetworkWorker) {
      gNetworkWorker->DispatchNetworkResult(mResult);
    }
    return NS_OK;
  }
private:
  NetworkResultOptions mResult;
};

NS_IMPL_ISUPPORTS(NetworkWorker, nsINetworkWorker, nsIObserver)

NetworkWorker::NetworkWorker()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!gNetworkWorker);
}

NetworkWorker::~NetworkWorker()
{
  MOZ_ASSERT(!gNetworkWorker);
  MOZ_ASSERT(!mListener);
}

already_AddRefed<NetworkWorker>
NetworkWorker::FactoryCreate()
{
  if (!XRE_IsParentProcess()) {
    return nullptr;
  }

  MOZ_ASSERT(NS_IsMainThread());

  if (!gNetworkWorker) {
    gNetworkWorker = new NetworkWorker();
    ClearOnShutdown(&gNetworkWorker);

    gNetworkUtils = new NetworkUtils(NetworkWorker::NotifyResult);
    ClearOnShutdown(&gNetworkUtils);
  }

  RefPtr<NetworkWorker> worker = gNetworkWorker.get();
  return worker.forget();
}

NS_IMETHODIMP
NetworkWorker::Start(nsINetworkEventListener* aListener)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aListener);

  if (mListener) {
    return NS_OK;
  }

  mListener = aListener;
  Preferences::AddStrongObserver(this, PREF_NETWORK_DEBUG_ENABLED);

  return NS_OK;
}

NS_IMETHODIMP
NetworkWorker::Shutdown()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!mListener) {
    return NS_OK;
  }

  mListener = nullptr;
  Preferences::RemoveObserver(this, PREF_NETWORK_DEBUG_ENABLED);

  return NS_OK;
}

NS_IMETHODIMP
NetworkWorker::Observe(nsISupports* aSubject,
                      const char* aTopic,
                      const char16_t* aData)
{
  if (!strcmp(aTopic, "nsPref:changed") && gNetworkUtils &&
      nsDependentString(aData).EqualsASCII(PREF_NETWORK_DEBUG_ENABLED)) {
    gNetworkUtils->updateDebug();
  }
  return NS_OK;
}

// Receive command from main thread (NetworkService.js).
NS_IMETHODIMP
NetworkWorker::PostMessage(JS::Handle<JS::Value> aOptions, JSContext* aCx)
{
  MOZ_ASSERT(NS_IsMainThread());

  NetworkCommandOptions options;
  if (!options.Init(aCx, aOptions)) {
    NS_WARNING("Bad dictionary passed to NetworkWorker::SendCommand");
    return NS_ERROR_FAILURE;
  }

  // Dispatch the command to the control thread.
  NetworkParams NetworkParams(options);
  if (gNetworkUtils) {
    gNetworkUtils->DispatchCommand(NetworkParams);
  }
  return NS_OK;
}

void
NetworkWorker::DispatchNetworkResult(const NetworkResultOptions& aOptions)
{
  MOZ_ASSERT(NS_IsMainThread());

  mozilla::AutoSafeJSContext cx;
  JS::RootedValue val(cx);

  if (!ToJSValue(cx, aOptions, &val)) {
    return;
  }

  // Call the listener with a JS value.
  if (mListener) {
    mListener->OnEvent(val);
  }
}

// Callback function from network worker thread to update result on main thread.
void
NetworkWorker::NotifyResult(NetworkResultOptions& aResult)
{
  MOZ_ASSERT(!NS_IsMainThread());

  nsCOMPtr<nsIRunnable> runnable = new NetworkResultDispatcher(aResult);
  NS_DispatchToMainThread(runnable);
}

NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(NetworkWorker,
                                         NetworkWorker::FactoryCreate)

NS_DEFINE_NAMED_CID(NS_NETWORKWORKER_CID);

static const mozilla::Module::CIDEntry kNetworkWorkerCIDs[] = {
  { &kNS_NETWORKWORKER_CID, false, nullptr, NetworkWorkerConstructor },
  { nullptr }
};

static const mozilla::Module::ContractIDEntry kNetworkWorkerContracts[] = {
  { "@mozilla.org/network/worker;1", &kNS_NETWORKWORKER_CID },
  { nullptr }
};

static const mozilla::Module kNetworkWorkerModule = {
  mozilla::Module::kVersion,
  kNetworkWorkerCIDs,
  kNetworkWorkerContracts,
  nullptr
};

} // namespace mozilla

NSMODULE_DEFN(NetworkWorkerModule) = &kNetworkWorkerModule;
