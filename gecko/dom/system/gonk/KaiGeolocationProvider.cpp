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

#include "KaiGeolocationProvider.h"

#include <cmath> // for sin(), cos()
#include "nsGeoPosition.h"
#include "nsIURLFormatter.h"
#if !defined(DISABLE_MOZ_RIL_GEOLOC) && !defined(DISABLE_MOZ_GEOLOC)
#include "GonkGPSGeolocationProvider.h"
#endif

#undef LOG
#undef ERR
#undef DBG
#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO,  "KAI_GEO", ## args)
#define ERR(args...)  __android_log_print(ANDROID_LOG_ERROR, "KAI_GEO", ## args)
#define DBG(args...)  __android_log_print(ANDROID_LOG_DEBUG, "KAI_GEO", ## args)

using namespace mozilla;
using namespace mozilla::dom;

NS_IMPL_ISUPPORTS(KaiGeolocationProvider,
                  nsIGeolocationProvider,
                  nsIGeolocationNetworkProvider)

/* static */ KaiGeolocationProvider* KaiGeolocationProvider::sSingleton = nullptr;


KaiGeolocationProvider::KaiGeolocationProvider()
  : mStarted(false)
  , mE911(false)
{
  // Normally, we could only initialize mGpsLocationProvider in Startup() as
  // lazy initialization.
  // However, GpsNiCallbacks may be triggered for SUPL NI (Network Initiated)
  // without calling Startup(). Therefore, we have to initialize
  // mGpsLocationProvider here for GPS HAL.
  mGpsLocationProvider =
    do_GetService("@mozilla.org/gonk-gps-geolocation-provider;1");
}

KaiGeolocationProvider::~KaiGeolocationProvider()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!mStarted, "Must call Shutdown before destruction");

  sSingleton = nullptr;
}

already_AddRefed<KaiGeolocationProvider>
KaiGeolocationProvider::GetSingleton()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!sSingleton) {
    sSingleton = new KaiGeolocationProvider();
  }

  RefPtr<KaiGeolocationProvider> provider = sSingleton;
  return provider.forget();
}

NS_IMETHODIMP
KaiGeolocationProvider::Startup()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (mStarted) {
    return NS_OK;
  }

  // Setup NetworkLocationProvider if the API key and server URI are available
  const nsAdoptingString& serverUri = Preferences::GetString("geo.wifi.uri");
  if (!serverUri.IsEmpty()) {
    nsresult rv;
    nsCOMPtr<nsIURLFormatter> formatter =
      do_CreateInstance("@mozilla.org/toolkit/URLFormatterService;1", &rv);
    if (NS_SUCCEEDED(rv)) {
      nsString systemKey, e911Key;
      formatter->FormatURLPref(
        NS_LITERAL_STRING("geo.authorization.key"), systemKey);
      formatter->FormatURLPref(
        NS_LITERAL_STRING("geo.authorization.e911_key"), e911Key);
      if (!systemKey.IsEmpty() || !e911Key.IsEmpty()) {
        mNetworkLocationProvider =
          do_CreateInstance("@mozilla.org/geolocation/mls-provider;1");
        if (mNetworkLocationProvider) {
          rv = mNetworkLocationProvider->Startup();
          if (NS_SUCCEEDED(rv)) {
            RefPtr<NetworkLocationUpdate> update = new NetworkLocationUpdate();
            mNetworkLocationProvider->Watch(update);
          }
        }
      }
    }
  }

  // Although mGpsLocationProvider should have been initialized in constructor,
  // we still assign the same service again for safety.
  // It could recure us from the failure of getting service in constructor.
  mGpsLocationProvider =
    do_GetService("@mozilla.org/gonk-gps-geolocation-provider;1");
  if (mGpsLocationProvider) {
    nsresult rv = mGpsLocationProvider->Startup();
    if (NS_SUCCEEDED(rv)) {
      RefPtr<GpsLocationUpdate> update = new GpsLocationUpdate();
      mGpsLocationProvider->Watch(update);
    }
  }

  mStarted = true;
  return NS_OK;
}

NS_IMETHODIMP
KaiGeolocationProvider::Watch(nsIGeolocationUpdate* aCallback)
{
  MOZ_ASSERT(NS_IsMainThread());

  mLocationCallback = aCallback;
  return NS_OK;
}

NS_IMETHODIMP
KaiGeolocationProvider::Shutdown()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!mStarted) {
    return NS_OK;
  }

  mStarted = false;
  mE911 = false;
  if (mNetworkLocationProvider) {
    mNetworkLocationProvider->Shutdown();
    mNetworkLocationProvider = nullptr;
  }
  if (mGpsLocationProvider) {
    mGpsLocationProvider->Shutdown();
  }

  return NS_OK;
}

NS_IMETHODIMP
KaiGeolocationProvider::SetHighAccuracy(bool highAccuracy)
{
  if (mGpsLocationProvider) {
    mGpsLocationProvider->SetHighAccuracy(highAccuracy);
  }
  return NS_OK;
}

#if !defined(DISABLE_MOZ_RIL_GEOLOC) && !defined(DISABLE_MOZ_GEOLOC)
NS_IMETHODIMP
KaiGeolocationProvider::SetGpsDeleteType(uint16_t gpsMode)
{
  if (mGpsLocationProvider) {
    mGpsLocationProvider->SetGpsDeleteType(gpsMode);
  }
  return NS_OK;
}
#endif

NS_IMETHODIMP
KaiGeolocationProvider::SetE911(bool e911)
{
  mE911 = e911;
  if (mNetworkLocationProvider) {
    mNetworkLocationProvider->SetE911(e911);
  }
  return NS_OK;
}

void
KaiGeolocationProvider::AcquireWakelockCallback()
{
}

void
KaiGeolocationProvider::ReleaseWakelockCallback()
{
}

nsCOMPtr<nsIDOMGeoPosition>
KaiGeolocationProvider::FindProperNetworkPos()
{
  if (!mLastQcPosition) {
    return mLastNetworkPosition;
  }

  if (!mLastNetworkPosition) {
    return mLastQcPosition;
  }

  DOMTimeStamp time_ms = 0;
  if (mLastQcPosition) {
    mLastQcPosition->GetTimestamp(&time_ms);
  }
  const int64_t qc_diff_ms = (PR_Now() / PR_USEC_PER_MSEC) - time_ms;

  time_ms = 0;
  if (mLastNetworkPosition) {
    mLastNetworkPosition->GetTimestamp(&time_ms);
  }
  const int64_t network_diff_ms = (PR_Now() / PR_USEC_PER_MSEC) - time_ms;

  const int kMaxTrustTimeDiff = 10000; // 10,000ms

  if (qc_diff_ms < kMaxTrustTimeDiff &&
      network_diff_ms > kMaxTrustTimeDiff) {
    return mLastQcPosition;
  } else if (qc_diff_ms > kMaxTrustTimeDiff &&
             network_diff_ms < kMaxTrustTimeDiff) {
    return mLastNetworkPosition;
  }

  nsCOMPtr<nsIDOMGeoPositionCoords> coords;
  mLastQcPosition->GetCoords(getter_AddRefs(coords));
  if (!coords) {
    return mLastNetworkPosition;
  }
  double qc_acc;
  coords->GetAccuracy(&qc_acc);

  mLastNetworkPosition->GetCoords(getter_AddRefs(coords));
  if (!coords) {
    return mLastQcPosition;
  }
  double network_acc;
  coords->GetAccuracy(&network_acc);

  return qc_acc < network_acc ? mLastQcPosition : mLastNetworkPosition;
}

NS_IMPL_ISUPPORTS(KaiGeolocationProvider::NetworkLocationUpdate,
                  nsIGeolocationUpdate)

NS_IMETHODIMP
KaiGeolocationProvider::NetworkLocationUpdate::Update(nsIDOMGeoPosition *position)
{
  if (!position) {
    return NS_ERROR_FAILURE;
  }
  RefPtr<KaiGeolocationProvider> provider =
    KaiGeolocationProvider::GetSingleton();

  nsCOMPtr<nsIDOMGeoPositionCoords> coords;
  position->GetCoords(getter_AddRefs(coords));
  if (!coords) {
    return NS_ERROR_FAILURE;
  }

  double lat, lon, acc;
  coords->GetLatitude(&lat);
  coords->GetLongitude(&lon);
  coords->GetAccuracy(&acc);

  double delta = -1.0;

  static double sLastNetworkPosLat = 0;
  static double sLastNetworkPosLon = 0;

  if (0 != sLastNetworkPosLon || 0 != sLastNetworkPosLat) {
    // Use spherical law of cosines to calculate difference
    // Not quite as correct as the Haversine but simpler and cheaper
    // Should the following be a utility function? Others might need this calc.
    const double radsInDeg = M_PI / 180.0;
    const double rNewLat = lat * radsInDeg;
    const double rNewLon = lon * radsInDeg;
    const double rOldLat = sLastNetworkPosLat * radsInDeg;
    const double rOldLon = sLastNetworkPosLon * radsInDeg;
    // WGS84 equatorial radius of earth = 6378137m
    double cosDelta = (sin(rNewLat) * sin(rOldLat)) +
                      (cos(rNewLat) * cos(rOldLat) * cos(rOldLon - rNewLon));
    if (cosDelta > 1.0) {
      cosDelta = 1.0;
    } else if (cosDelta < -1.0) {
      cosDelta = -1.0;
    }
    delta = acos(cosDelta) * 6378137;
  }

  sLastNetworkPosLat = lat;
  sLastNetworkPosLon = lon;

  // if the coord change is smaller than this arbitrarily small value
  // assume the coord is unchanged, and stick with the GPS location
  const double kMinCoordChangeInMeters = 10;

  DOMTimeStamp time_ms = 0;
  if (provider->mLastGPSPosition) {
    provider->mLastGPSPosition->GetTimestamp(&time_ms);
  }
  const int64_t diff_ms = (PR_Now() / PR_USEC_PER_MSEC) - time_ms;

  // We want to distinguish between the GPS being inactive completely
  // and temporarily inactive. In the former case, we would use a low
  // accuracy network location; in the latter, we only want a network
  // location that appears to updating with movement.

  const bool isGPSFullyInactive = diff_ms > 1000 * 60 * 2; // two mins
  const bool isGPSTempInactive = diff_ms > 1000 * 10; // 10 secs

  provider->mLastNetworkPosition = position;

  if (provider->mLocationCallback) {
    if (isGPSFullyInactive ||
       (isGPSTempInactive && delta > kMinCoordChangeInMeters))
    {
      if (gDebug_isLoggingEnabled) {
        DBG("geo: Using network location, GPS age:%fs, network Delta:%fm",
          diff_ms / 1000.0, delta);
      }

      provider->mLocationCallback->Update(provider->FindProperNetworkPos());
    } else if (provider->mLastGPSPosition) {
      if (gDebug_isLoggingEnabled) {
        DBG("geo: Using old GPS age:%fs", diff_ms / 1000.0);
      }

      // This is a fallback case so that the GPS provider responds with its last
      // location rather than waiting for a more recent GPS or network location.
      // The service decides if the location is too old, not the provider.
      provider->mLocationCallback->Update(provider->mLastGPSPosition);
    }
  }

#if !defined(DISABLE_MOZ_RIL_GEOLOC) && !defined(DISABLE_MOZ_GEOLOC)
  RefPtr<GonkGPSGeolocationProvider> gpsProvider =
    GonkGPSGeolocationProvider::GetSingleton();
  if (gpsProvider) {
    gpsProvider->InjectLocation(lat, lon, acc);
  }
#endif

  return NS_OK;
}

NS_IMETHODIMP
KaiGeolocationProvider::NetworkLocationUpdate::NotifyError(uint16_t error)
{
  return NS_OK;
}


NS_IMPL_ISUPPORTS(KaiGeolocationProvider::GpsLocationUpdate,
                  nsIGeolocationUpdate)

NS_IMETHODIMP
KaiGeolocationProvider::GpsLocationUpdate::Update(nsIDOMGeoPosition *position)
{
  RefPtr<KaiGeolocationProvider> provider =
    KaiGeolocationProvider::GetSingleton();

  nsCOMPtr<nsIDOMGeoPositionCoords> coords;
  position->GetCoords(getter_AddRefs(coords));
  if (!coords) {
    return NS_ERROR_FAILURE;
  }

  double lat, lon, acc;
  coords->GetLatitude(&lat);
  coords->GetLongitude(&lon);
  coords->GetAccuracy(&acc);

  const double kGpsAccuracyInMeters = 30.000; // meter

  if (acc <= kGpsAccuracyInMeters) {
    provider->mLastGPSPosition = position;

    if (provider->mLocationCallback) {
      provider->mLocationCallback->Update(position);
    }
  } else {
    provider->mLastQcPosition = position;

    if (provider->mLocationCallback) {
      provider->mLocationCallback->Update(provider->FindProperNetworkPos());
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
KaiGeolocationProvider::GpsLocationUpdate::NotifyError(uint16_t error)
{
  return NS_OK;
}
