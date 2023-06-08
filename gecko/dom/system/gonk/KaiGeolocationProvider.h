/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* Copyright (C) 2016 Kai OS Technologies. All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef KaiGeolocationProvider_h
#define KaiGeolocationProvider_h

#include "nsCOMPtr.h"
#include "nsIDOMGeoPosition.h"
#include "nsIGeolocationProvider.h"
#include "nsIGeolocationNetworkProvider.h"

#define KAI_GEOLOCATION_PROVIDER_CID \
{ 0x82e77d58, 0x9ce5, 0x11e6, { 0xa5, 0x5b, 0x89, 0x33, 0x11, 0xfc, 0x8c, 0x42 } }

#define KAI_GEOLOCATION_PROVIDER_CONTRACTID \
"@mozilla.org/kai-geolocation-provider;1"

class KaiGeolocationProvider : public nsIGeolocationNetworkProvider
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIGEOLOCATIONPROVIDER
  NS_DECL_NSIGEOLOCATIONNETWORKPROVIDER

  static already_AddRefed<KaiGeolocationProvider> GetSingleton();

private:
  // Client should use GetSingleton() to get the provider instance.
  KaiGeolocationProvider();
  KaiGeolocationProvider(const KaiGeolocationProvider &);
  KaiGeolocationProvider & operator = (const KaiGeolocationProvider &);
  virtual ~KaiGeolocationProvider();

  // Find the proper position updatea from two location providers
  nsCOMPtr<nsIDOMGeoPosition> FindProperNetworkPos();

#if !defined(DISABLE_MOZ_RIL_GEOLOC) && !defined(DISABLE_MOZ_GEOLOC)
  NS_METHOD SetGpsDeleteType(uint16_t gpsMode);
#endif

  static void AcquireWakelockCallback();
  static void ReleaseWakelockCallback();

  static KaiGeolocationProvider* sSingleton;

  bool mStarted;
  bool mE911;

  nsCOMPtr<nsIGeolocationUpdate> mLocationCallback;

  // holds an instane of provider only when it's positioning
  nsCOMPtr<nsIGeolocationNetworkProvider> mNetworkLocationProvider;

  // always holds the service of GPS location provider
  nsCOMPtr<nsIGeolocationProvider> mGpsLocationProvider;

  nsCOMPtr<nsIDOMGeoPosition> mLastGPSPosition;
  nsCOMPtr<nsIDOMGeoPosition> mLastNetworkPosition;
  nsCOMPtr<nsIDOMGeoPosition> mLastQcPosition;

  class NetworkLocationUpdate : public nsIGeolocationUpdate
  {
    public:
      NS_DECL_ISUPPORTS
      NS_DECL_NSIGEOLOCATIONUPDATE

      NetworkLocationUpdate() {}

    private:
      virtual ~NetworkLocationUpdate() {}
  };

  class GpsLocationUpdate : public nsIGeolocationUpdate
  {
    public:
      NS_DECL_ISUPPORTS
      NS_DECL_NSIGEOLOCATIONUPDATE

      GpsLocationUpdate() {}

    private:
      virtual ~GpsLocationUpdate() {}
  };
};

#endif /* KaiGeolocationProvider_h */
