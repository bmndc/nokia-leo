/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "TVGonkService.h"

#include "mozilla/dom/TVServiceRunnables.h"
#include "nsCOMPtr.h"
#include "nsIMutableArray.h"
#include "nsServiceManagerUtils.h"
#include "TVDaemonInterface.h"

namespace mozilla {
namespace dom {

class GonkDTVNotificationHandler final : public DTVNotificationHandler
{
public:
  void OnChannelScanned(const nsAString& aTunerId, const nsAString& aSourceType,
                        struct ChannelData aChannelMsgData) override
  {
    MOZ_ASSERT(NS_IsMainThread());

    nsCOMPtr<nsITVService> service = do_GetService(TV_SERVICE_CONTRACTID);
    if (NS_WARN_IF(!service)) {
      return;
    }

    nsCOMPtr<nsITVChannelData> channelData;
    channelData = do_CreateInstance(TV_CHANNEL_DATA_CONTRACTID);
    PackChannelData(channelData, aChannelMsgData);

    nsresult rv =
      static_cast<TVGonkService*>(service.get())
        ->NotifyChannelScanned(aTunerId, aSourceType, channelData.get());
    NS_WARN_IF(NS_FAILED(rv));
  }

  void OnChannelScanComplete(const nsAString& aTunerId,
                             const nsAString& aSourceType) override
  {
    MOZ_ASSERT(NS_IsMainThread());

    nsCOMPtr<nsITVService> service = do_GetService(TV_SERVICE_CONTRACTID);
    if (NS_WARN_IF(!service)) {
      return;
    }

    nsresult rv = static_cast<TVGonkService*>(service.get())
                    ->NotifyChannelScanComplete(aTunerId, aSourceType);
    NS_WARN_IF(NS_FAILED(rv));
  }

  void OnChannelScanStopped(const nsAString& aTunerId,
                            const nsAString& aSourceType) override
  {
    MOZ_ASSERT(NS_IsMainThread());

    nsCOMPtr<nsITVService> service = do_GetService(TV_SERVICE_CONTRACTID);
    if (NS_WARN_IF(!service)) {
      return;
    }

    nsresult rv = static_cast<TVGonkService*>(service.get())
                    ->NotifyChannelScanStopped(aTunerId, aSourceType);
    NS_WARN_IF(NS_FAILED(rv));
  }

  void OnEITBroadcasted(const nsAString& aTunerId, const nsAString& aSourceType,
                        struct ChannelData aChannelMsgData,
                        const nsTArray<struct ProgramData>& aProgramDataList)
    override
  {
    MOZ_ASSERT(NS_IsMainThread());

    nsCOMPtr<nsITVService> service = do_GetService(TV_SERVICE_CONTRACTID);
    if (NS_WARN_IF(!service)) {
      return;
    }

    nsCOMPtr<nsITVChannelData> channelData;
    channelData = do_CreateInstance(TV_CHANNEL_DATA_CONTRACTID);
    PackChannelData(channelData, aChannelMsgData);

    uint32_t length = aProgramDataList.Length();
    nsCOMArray<nsITVProgramData> programDataList(length);
    for (uint32_t i = 0; i < length; i++) {
      nsCOMPtr<nsITVProgramData> programData;
      programData = do_CreateInstance(TV_PROGRAM_DATA_CONTRACTID);
      PackProgramData(programData, aProgramDataList[i]);
      programDataList.AppendElement(programData);
    }

    nsresult rv =
      static_cast<TVGonkService*>(service.get())->NotifyEITBroadcasted(
        aTunerId, aSourceType, channelData.get(),
        const_cast<nsITVProgramData**>(programDataList.Elements()), length);
    NS_WARN_IF(NS_FAILED(rv));
  }
};


class DisconnectResultHandler final : public TVDaemonResultHandler
{
public:
  DisconnectResultHandler(TVDaemonInterface* aInterface)
    : mInterface(aInterface)
  {
    MOZ_ASSERT(aInterface);
  }

  void OnError(TVStatus aError)
  {
    MOZ_ASSERT(mInterface);

    TVDaemonResultHandler::OnError(aError);
    if (!mInterface) {
      return;
    }
    mInterface->SetNotificationHandler(nullptr);
  }

  void Disconnect() override
  {
    MOZ_ASSERT(mInterface);

    if (!mInterface) {
      return;
    }
    mInterface->SetNotificationHandler(nullptr);
  }

private:
  TVDaemonInterface* mInterface;
};

/**
 * |TVRegisterModuleResultHandler| implements the result-handler
 * callback for registering the services. If an error occures
 * during the process, the result handler
 * disconnects and closes the backend.
 */
class TVRegisterModuleResultHandler final : public RegistryResultHandler
{
public:
  TVRegisterModuleResultHandler(TVDaemonInterface* aInterface)
    : mInterface(aInterface)
  {
    MOZ_ASSERT(aInterface);
  }

  void OnError(TVStatus aError) override
  {
    RegistryResultHandler::OnError(aError);
    Disconnect(); // Registering failed, so close the connection completely
  }

  void RegisterModule() override
  {
    DTVInterface* dTVInterface = mInterface->GetTVDTVInterface();
    if (!dTVInterface) {
      Disconnect();
      return;
    }
    dTVInterface->SetNotificationHandler(new GonkDTVNotificationHandler());
  }

  void UnregisterModule() override
  {
    DTVInterface* dTVInterface = mInterface->GetTVDTVInterface();
    if (!dTVInterface) {
      Disconnect();
      return;
    }
    dTVInterface->SetNotificationHandler(nullptr);
  }

  void Disconnect()
  {
    MOZ_ASSERT(mInterface);
    mInterface->Disconnect(new DisconnectResultHandler(mInterface));
  }

private:
  TVDaemonInterface* mInterface;
};

/**
 * |TVConnectResultHandler| implements the result-handler
 * callback for starting the TV backend.
 */
class TVConnectResultHandler final : public TVDaemonResultHandler
{
public:
  TVConnectResultHandler(TVDaemonInterface* aInterface) : mInterface(aInterface)
  {
    MOZ_ASSERT(aInterface);
  }

  void OnError(TVStatus aError) override
  {
    MOZ_ASSERT(mInterface);

    TVDaemonResultHandler::OnError(aError);
    mInterface->SetNotificationHandler(nullptr);
  }

  void Connect() override
  {
    MOZ_ASSERT(NS_IsMainThread());

    // Init, step 2: register DTV service
    RegistryInterface* registryInterface = mInterface->GetTVRegistryInterface();
    if (!registryInterface) {
      return;
    }
    registryInterface->RegisterModule(
      DTVModule::SERVICE_ID,
      new TVRegisterModuleResultHandler(mInterface));
  }

private:
  TVDaemonInterface* mInterface;
};

/**
 * This is the notifiaction handler for the TV interface. If the backend
 * crashes, we can restart it from here.
 */
class TVNotificationHandler final : public TVDaemonNotificationHandler
{
public:
  TVNotificationHandler(TVDaemonInterface* aInterface) : mInterface(aInterface)
  {
    MOZ_ASSERT(mInterface);
    mInterface->SetNotificationHandler(this);
  }

  void BackendErrorNotification(bool aCrashed) override
  {
    MOZ_ASSERT(mInterface);

    // Force the TV daemon interface to init.
    // Start up
    // Init, step 1: connect to TV backend
    mInterface->Connect(this, new TVConnectResultHandler(mInterface));
  }

private:
  TVDaemonInterface* mInterface;
};

NS_IMPL_ISUPPORTS(TVGonkService, nsITVService)

TVGonkService::TVGonkService()
{
  mInterface = TVDaemonInterface::GetInstance();

  if (!mInterface) {
    return;
  }

  // Force the TV daemon interface to init.
  // Start up
  // Init, step 1: connect to TV backend
  mInterface->Connect(new TVNotificationHandler(mInterface),
                      new TVConnectResultHandler(mInterface));
}

TVGonkService::~TVGonkService()
{
  if (!mInterface) {
    return;
  }
  RegistryInterface* registryInterface = mInterface->GetTVRegistryInterface();
  if (registryInterface) {
    registryInterface->UnregisterModule(
      DTVModule::SERVICE_ID,
      new TVRegisterModuleResultHandler(mInterface));
  }

  mInterface->Disconnect(new DisconnectResultHandler(mInterface));
}

NS_IMETHODIMP
TVGonkService::RegisterSourceListener(const nsAString& aTunerId,
                                      const nsAString& aSourceType,
                                      nsITVSourceListener* aListener)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!aTunerId.IsEmpty());
  MOZ_ASSERT(!aSourceType.IsEmpty());
  MOZ_ASSERT(aListener);

  mSourceListenerTuples.AppendElement(MakeUnique<TVSourceListenerTuple>(
    nsString(aTunerId), nsString(aSourceType), aListener));
  return NS_OK;
}

NS_IMETHODIMP
TVGonkService::UnregisterSourceListener(const nsAString& aTunerId,
                                        const nsAString& aSourceType,
                                        nsITVSourceListener* aListener)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!aTunerId.IsEmpty());
  MOZ_ASSERT(!aSourceType.IsEmpty());
  MOZ_ASSERT(aListener);

  for (uint32_t i = 0; i < mSourceListenerTuples.Length(); i++) {
    const UniquePtr<TVSourceListenerTuple>& tuple = mSourceListenerTuples[i];
    if (aTunerId.Equals(Get<0>(*tuple)) && aSourceType.Equals(Get<1>(*tuple)) &&
        aListener == Get<2>(*tuple)) {
      mSourceListenerTuples.RemoveElementAt(i);
      break;
    }
  }

  return NS_OK;
}

void
TVGonkService::GetSourceListeners(
  const nsAString& aTunerId, const nsAString& aSourceType,
  nsTArray<nsCOMPtr<nsITVSourceListener>>& aListeners) const
{
  aListeners.Clear();

  for (uint32_t i = 0; i < mSourceListenerTuples.Length(); i++) {
    const UniquePtr<TVSourceListenerTuple>& tuple = mSourceListenerTuples[i];
    nsCOMPtr<nsITVSourceListener> listener = Get<2>(*tuple);
    if (aTunerId.Equals(Get<0>(*tuple)) && aSourceType.Equals(Get<1>(*tuple))) {
      aListeners.AppendElement(listener);
      break;
    }
  }
}

/* virtual */ NS_IMETHODIMP
TVGonkService::GetTuners(nsITVServiceCallback* aCallback)
{
  MOZ_ASSERT(aCallback);
  MOZ_ASSERT(mInterface);

  DTVInterface* daemonInterface = mInterface->GetTVDTVInterface();
  if (NS_WARN_IF(!daemonInterface)) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  daemonInterface->GetTuners(new DTVResultHandler(aCallback));
  return NS_OK;
}

/* virtual */ NS_IMETHODIMP
TVGonkService::SetSource(const nsAString& aTunerId,
                         const nsAString& aSourceType,
                         nsITVServiceCallback* aCallback)
{
  MOZ_ASSERT(!aTunerId.IsEmpty());
  MOZ_ASSERT(!aSourceType.IsEmpty());
  MOZ_ASSERT(aCallback);
  MOZ_ASSERT(mInterface);

  DTVInterface* daemonInterface = mInterface->GetTVDTVInterface();
  if (NS_WARN_IF(!daemonInterface)) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  daemonInterface->SetSource(aTunerId, aSourceType,
                             new DTVResultHandler(aCallback));
  return NS_OK;
}

/* virtual */ NS_IMETHODIMP
TVGonkService::StartScanningChannels(const nsAString& aTunerId,
                                     const nsAString& aSourceType,
                                     nsITVServiceCallback* aCallback)
{
  MOZ_ASSERT(!aTunerId.IsEmpty());
  MOZ_ASSERT(!aSourceType.IsEmpty());
  MOZ_ASSERT(aCallback);
  MOZ_ASSERT(mInterface);

  DTVInterface* daemonInterface = mInterface->GetTVDTVInterface();
  if (NS_WARN_IF(!daemonInterface)) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  daemonInterface->StartScanningChannels(aTunerId, aSourceType,
                                         new DTVResultHandler(aCallback));
  return NS_OK;
}

/* virtual */ NS_IMETHODIMP
TVGonkService::StopScanningChannels(const nsAString& aTunerId,
                                    const nsAString& aSourceType,
                                    nsITVServiceCallback* aCallback)
{
  MOZ_ASSERT(!aTunerId.IsEmpty());
  MOZ_ASSERT(!aSourceType.IsEmpty());
  MOZ_ASSERT(aCallback);
  MOZ_ASSERT(mInterface);

  DTVInterface* daemonInterface = mInterface->GetTVDTVInterface();
  if (NS_WARN_IF(!daemonInterface)) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  daemonInterface->StopScanningChannels(aTunerId, aSourceType,
                                        new DTVResultHandler(aCallback));
  return NS_OK;
}

/* virtual */ NS_IMETHODIMP
TVGonkService::ClearScannedChannelsCache()
{
  MOZ_ASSERT(mInterface);

  DTVInterface* daemonInterface = mInterface->GetTVDTVInterface();
  if (NS_WARN_IF(!daemonInterface)) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  daemonInterface->ClearScannedChannelsCache(new DTVResultHandler(nullptr));
  return NS_OK;
}

/* virtual */ NS_IMETHODIMP
TVGonkService::SetChannel(const nsAString& aTunerId,
                          const nsAString& aSourceType,
                          const nsAString& aChannelNumber,
                          nsITVServiceCallback* aCallback)
{
  MOZ_ASSERT(!aTunerId.IsEmpty());
  MOZ_ASSERT(!aSourceType.IsEmpty());
  MOZ_ASSERT(aCallback);
  MOZ_ASSERT(mInterface);

  DTVInterface* daemonInterface = mInterface->GetTVDTVInterface();
  if (NS_WARN_IF(!daemonInterface)) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  daemonInterface->SetChannel(aTunerId, aSourceType, aChannelNumber,
                              new DTVResultHandler(aCallback));
  return NS_OK;
}

/* virtual */ NS_IMETHODIMP
TVGonkService::GetChannels(const nsAString& aTunerId,
                           const nsAString& aSourceType,
                           nsITVServiceCallback* aCallback)
{
  MOZ_ASSERT(!aTunerId.IsEmpty());
  MOZ_ASSERT(!aSourceType.IsEmpty());
  MOZ_ASSERT(aCallback);
  MOZ_ASSERT(mInterface);

  DTVInterface* daemonInterface = mInterface->GetTVDTVInterface();
  if (NS_WARN_IF(!daemonInterface)) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  daemonInterface->GetChannels(aTunerId, aSourceType,
                               new DTVResultHandler(aCallback));
  return NS_OK;
}

/* virtual */ NS_IMETHODIMP
TVGonkService::GetPrograms(const nsAString& aTunerId,
                           const nsAString& aSourceType,
                           const nsAString& aChannelNumber, uint64_t startTime,
                           uint64_t endTime, nsITVServiceCallback* aCallback)
{
  MOZ_ASSERT(!aTunerId.IsEmpty());
  MOZ_ASSERT(!aSourceType.IsEmpty());
  MOZ_ASSERT(!aChannelNumber.IsEmpty());
  MOZ_ASSERT(aCallback);
  MOZ_ASSERT(mInterface);

  DTVInterface* daemonInterface = mInterface->GetTVDTVInterface();
  if (NS_WARN_IF(!daemonInterface)) {
    return NS_ERROR_NOT_AVAILABLE;
  }
  daemonInterface->GetPrograms(aTunerId, aSourceType, aChannelNumber, startTime,
                               endTime, new DTVResultHandler(aCallback));

  return NS_OK;
}

nsresult
TVGonkService::NotifyChannelScanned(const nsAString& aTunerId,
                                    const nsAString& aSourceType,
                                    nsITVChannelData* aChannelData)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!aTunerId.IsEmpty());
  MOZ_ASSERT(!aSourceType.IsEmpty());
  MOZ_ASSERT(aChannelData);

  nsTArray<nsCOMPtr<nsITVSourceListener>> listeners;
  GetSourceListeners(aTunerId, aSourceType, listeners);
  for (uint32_t i = 0; i < listeners.Length(); i++) {
    nsresult rv =
      listeners[i]->NotifyChannelScanned(aTunerId, aSourceType, aChannelData);
    NS_WARN_IF(NS_FAILED(rv));
  }

  return NS_OK;
}

nsresult
TVGonkService::NotifyChannelScanComplete(const nsAString& aTunerId,
                                         const nsAString& aSourceType)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!aTunerId.IsEmpty());
  MOZ_ASSERT(!aSourceType.IsEmpty());

  nsTArray<nsCOMPtr<nsITVSourceListener>> listeners;
  GetSourceListeners(aTunerId, aSourceType, listeners);
  for (uint32_t i = 0; i < listeners.Length(); i++) {
    nsresult rv =
      listeners[i]->NotifyChannelScanComplete(aTunerId, aSourceType);
    NS_WARN_IF(NS_FAILED(rv));
  }

  return NS_OK;
}

nsresult
TVGonkService::NotifyChannelScanStopped(const nsAString& aTunerId,
                                        const nsAString& aSourceType)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!aTunerId.IsEmpty());
  MOZ_ASSERT(!aSourceType.IsEmpty());

  nsTArray<nsCOMPtr<nsITVSourceListener>> listeners;
  GetSourceListeners(aTunerId, aSourceType, listeners);
  for (uint32_t i = 0; i < listeners.Length(); i++) {
    nsresult rv = listeners[i]->NotifyChannelScanStopped(aTunerId, aSourceType);
    NS_WARN_IF(NS_FAILED(rv));
  }

  return NS_OK;
}

nsresult
TVGonkService::NotifyEITBroadcasted(const nsAString& aTunerId,
                                    const nsAString& aSourceType,
                                    nsITVChannelData* aChannelData,
                                    nsITVProgramData** aProgramDataList,
                                    uint32_t aCount)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!aTunerId.IsEmpty());
  MOZ_ASSERT(!aSourceType.IsEmpty());
  MOZ_ASSERT(aChannelData);
  MOZ_ASSERT(aProgramDataList);

  nsTArray<nsCOMPtr<nsITVSourceListener>> listeners;
  GetSourceListeners(aTunerId, aSourceType, listeners);
  for (uint32_t i = 0; i < listeners.Length(); i++) {
    nsresult rv = listeners[i]->NotifyEITBroadcasted(
      aTunerId, aSourceType, aChannelData, aProgramDataList, aCount);
    NS_WARN_IF(NS_FAILED(rv));
  }

  return NS_OK;
}

} // namespace dom
} // namespace mozilla
