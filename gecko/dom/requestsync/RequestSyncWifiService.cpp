/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "RequestSyncWifiService.h"
#include "mozilla/Services.h"
#include "mozilla/StaticPtr.h"
#include "nsIObserverService.h"
#include "mozilla/dom/NetworkInformationBinding.h"

namespace mozilla {
namespace dom {

NS_IMPL_ISUPPORTS(RequestSyncWifiService, nsIObserver);

namespace {

StaticRefPtr<RequestSyncWifiService> sService;

} // namespace

/* static */ void
RequestSyncWifiService::Init()
{
  RefPtr<RequestSyncWifiService> service = GetInstance();
  if (!service) {
    NS_WARNING("Failed to initialize RequestSyncWifiService.");
  }
}

/* static */ already_AddRefed<RequestSyncWifiService>
RequestSyncWifiService::GetInstance()
{
  if (!sService) {
    sService = new RequestSyncWifiService();
    nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
    if (obs) {
      obs->AddObserver(sService, "network-active-changed", false);
    }
  }

  RefPtr<RequestSyncWifiService> service = sService.get();
  return service.forget();
}

/* static */ void
RequestSyncWifiService::Shutdown()
{
  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  if (sService && obs) {
    obs->RemoveObserver(sService, "network-active-changed");
  }
  sService = nullptr;
}

NS_IMETHODIMP
RequestSyncWifiService::Observe(nsISupports* aSubject,
                                const char* aTopic,
                                const char16_t* aData)
{
  if (!strcmp(aTopic, "network-active-changed")) {
    NS_ConvertUTF16toUTF8 type(aData);
    nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
    if (!type.IsEmpty() && obs) {
      bool isWifi = (static_cast<ConnectionType>(atoi(type.get())) == ConnectionType::Wifi);

      if (isWifi == mIsWifi) {
        return NS_OK;
      }

      mIsWifi = isWifi;

      obs->NotifyObservers(nullptr, "wifi-state-changed", mIsWifi ?
                           MOZ_UTF16("enabled") : MOZ_UTF16("disabled"));
    }
  }
  return NS_OK;
}

} // namespace dom
} // namespace mozilla
