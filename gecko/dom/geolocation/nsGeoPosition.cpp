/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsGeoPosition.h"

#include "mozilla/dom/PositionBinding.h"
#include "mozilla/dom/CoordinatesBinding.h"
#include "prtime.h"

using mozilla::dom::PositionSource;

////////////////////////////////////////////////////
// nsGeoPositionCoords
////////////////////////////////////////////////////
nsGeoPositionCoords::nsGeoPositionCoords(double aLat, double aLong,
                                         double aAlt, double aHError,
                                         double aVError, double aHeading,
                                         double aSpeed)
  : mLat(aLat)
  , mLong(aLong)
  , mAlt(aAlt)
  , mHError(aHError)
  , mVError(aVError)
  , mHeading(aHeading)
  , mSpeed(aSpeed)
{
}

nsGeoPositionCoords::~nsGeoPositionCoords()
{
}

NS_INTERFACE_MAP_BEGIN(nsGeoPositionCoords)
NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMGeoPositionCoords)
NS_INTERFACE_MAP_ENTRY(nsIDOMGeoPositionCoords)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsGeoPositionCoords)
NS_IMPL_RELEASE(nsGeoPositionCoords)

NS_IMETHODIMP
nsGeoPositionCoords::GetLatitude(double *aLatitude)
{
  *aLatitude = mLat;
  return NS_OK;
}

NS_IMETHODIMP
nsGeoPositionCoords::GetLongitude(double *aLongitude)
{
  *aLongitude = mLong;
  return NS_OK;
}

NS_IMETHODIMP
nsGeoPositionCoords::GetAltitude(double *aAltitude)
{
  *aAltitude = mAlt;
  return NS_OK;
}

NS_IMETHODIMP
nsGeoPositionCoords::GetAccuracy(double *aAccuracy)
{
  *aAccuracy = mHError;
  return NS_OK;
}

NS_IMETHODIMP
nsGeoPositionCoords::GetAltitudeAccuracy(double *aAltitudeAccuracy)
{
  *aAltitudeAccuracy = mVError;
  return NS_OK;
}

NS_IMETHODIMP
nsGeoPositionCoords::GetHeading(double *aHeading)
{
  *aHeading = mHeading;
  return NS_OK;
}

NS_IMETHODIMP
nsGeoPositionCoords::GetSpeed(double *aSpeed)
{
  *aSpeed = mSpeed;
  return NS_OK;
}

////////////////////////////////////////////////////
// nsGeoPosition
////////////////////////////////////////////////////

nsGeoPosition::nsGeoPosition(double aLat, double aLong,
                             double aAlt, double aHError,
                             double aVError, double aHeading,
                             double aSpeed, long long aTimestamp,
                             PositionSource aMethod,
                             long long aGnssUtcTime) :
    mTimestamp(aTimestamp),
    mPositioningMethod(aMethod),
    mGnssUtcTime(aGnssUtcTime)
{
  CorrectTimestamp(mTimestamp);
  mCoords = new nsGeoPositionCoords(aLat, aLong,
                                    aAlt, aHError,
                                    aVError, aHeading,
                                    aSpeed);
}

nsGeoPosition::nsGeoPosition(nsIDOMGeoPositionCoords *aCoords,
                             long long aTimestamp,
                             PositionSource aMethod,
                             long long aGnssUtcTime) :
    mTimestamp(aTimestamp),
    mCoords(aCoords),
    mPositioningMethod(aMethod),
    mGnssUtcTime(aGnssUtcTime)
{
  CorrectTimestamp(mTimestamp);
}

nsGeoPosition::nsGeoPosition(nsIDOMGeoPositionCoords *aCoords,
                             DOMTimeStamp aTimestamp,
                             PositionSource aMethod,
                             DOMTimeStamp aGnssUtcTime) :
  mTimestamp(aTimestamp),
  mCoords(aCoords),
  mPositioningMethod(aMethod),
  mGnssUtcTime(aGnssUtcTime)
{
  CorrectTimestamp(mTimestamp);
}

/* static */ PositionSource
nsGeoPosition::ConvertStringToPositionSource(const nsAString& aMethod) {
  if (aMethod.EqualsLiteral("gnss")) {
    return PositionSource::Gnss;
  } else if (aMethod.EqualsLiteral("wifi")) {
    return PositionSource::Wifi;
  } else if (aMethod.EqualsLiteral("cell")) {
    return PositionSource::Cell;
  } else if (aMethod.EqualsLiteral("wifi-and-cell")) {
    return PositionSource::Wifi_and_cell;
  }
  return PositionSource::Unknown;
}

nsGeoPosition::~nsGeoPosition()
{
}

void
nsGeoPosition::CorrectTimestamp(long long aTimestamp)
{
  // Sanity check of the given timestamp
  const int64_t diff_ms = (PR_Now() / PR_USEC_PER_MSEC) - aTimestamp;
  if (diff_ms > 3600000) { // 3600000 ms equals to 1 hour
    mTimestamp = (PR_Now() / PR_USEC_PER_MSEC);
  }
}

NS_INTERFACE_MAP_BEGIN(nsGeoPosition)
NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMGeoPositionExtended)
NS_INTERFACE_MAP_ENTRY(nsIDOMGeoPositionExtended)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsGeoPosition)
NS_IMPL_RELEASE(nsGeoPosition)

NS_IMETHODIMP
nsGeoPosition::GetTimestamp(DOMTimeStamp* aTimestamp)
{
  *aTimestamp = mTimestamp;
  return NS_OK;
}

NS_IMETHODIMP
nsGeoPosition::GetCoords(nsIDOMGeoPositionCoords * *aCoords)
{
  NS_IF_ADDREF(*aCoords = mCoords);
  return NS_OK;
}

NS_IMETHODIMP
nsGeoPosition::GetPositioningMethod(nsAString& aPositioningMethod)
{
  switch (mPositioningMethod) {
    case PositionSource::Gnss:
      aPositioningMethod.AssignLiteral("gnss");
      break;
    case PositionSource::Wifi:
      aPositioningMethod.AssignLiteral("wifi");
      break;
    case PositionSource::Cell:
      aPositioningMethod.AssignLiteral("cell");
      break;
    case PositionSource::Wifi_and_cell:
      aPositioningMethod.AssignLiteral("wifi-and-cell");
      break;
    default:
      aPositioningMethod.AssignLiteral("unknown");
  }
  return NS_OK;
}

NS_IMETHODIMP
nsGeoPosition::GetGnssUtcTime(DOMTimeStamp* aGnssUtcTime)
{
  *aGnssUtcTime = mGnssUtcTime;
  return NS_OK;
}

namespace mozilla {
namespace dom {


NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(Position, mParent, mCoordinates)
NS_IMPL_CYCLE_COLLECTING_ADDREF(Position)
NS_IMPL_CYCLE_COLLECTING_RELEASE(Position)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(Position)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

Position::Position(nsISupports* aParent, nsIDOMGeoPosition* aGeoPosition)
  : mParent(aParent)
  , mGeoPosition(aGeoPosition)
  , mIsHighAccuracy(false)
{
  mGeoPositionEx = do_QueryInterface(mGeoPosition);

  if (Coords()) {
    const double kGpsAccuracyInMeters = 30.000; // meter
    mIsHighAccuracy = Coords()->Accuracy() < kGpsAccuracyInMeters;
  }
}

Position::~Position()
{
}

nsISupports*
Position::GetParentObject() const
{
  return mParent;
}

JSObject*
Position::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return PositionBinding::Wrap(aCx, this, aGivenProto);
}

Coordinates*
Position::Coords()
{
  if (!mCoordinates) {
    nsCOMPtr<nsIDOMGeoPositionCoords> coords;
    mGeoPosition->GetCoords(getter_AddRefs(coords));
    MOZ_ASSERT(coords, "coords should not be null");

    mCoordinates = new Coordinates(this, coords);
  }

  return mCoordinates;
}

uint64_t
Position::Timestamp() const
{
  uint64_t rv;

  mGeoPosition->GetTimestamp(&rv);
  return rv;
}

PositionSource
Position::PositioningMethod() const
{
  if (!mGeoPositionEx) {
    if (mIsHighAccuracy) {
      return PositionSource::Gnss;
    }
    return PositionSource::Unknown;
  }

  nsString methodStr;
  mGeoPositionEx->GetPositioningMethod(methodStr);

  PositionSource method =
    nsGeoPosition::ConvertStringToPositionSource(methodStr);

  return method;
}

uint64_t
Position::GnssUtcTime() const
{
  uint64_t rv;

  if (!mGeoPositionEx) {
    if (mIsHighAccuracy) {
      // Treat the timestamp of GNSS positoin as GNSS UTC time.
      mGeoPosition->GetTimestamp(&rv);
      return rv;
    }
    // 'gnssUtcTime' is 0 since the position isn't from GNSS.
    return 0;
  }

  mGeoPositionEx->GetGnssUtcTime(&rv);
  return rv;
}

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(Coordinates, mPosition)
NS_IMPL_CYCLE_COLLECTING_ADDREF(Coordinates)
NS_IMPL_CYCLE_COLLECTING_RELEASE(Coordinates)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(Coordinates)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

Coordinates::Coordinates(Position* aPosition, nsIDOMGeoPositionCoords* aCoords)
  : mPosition(aPosition)
  , mCoords(aCoords)
{
}

Coordinates::~Coordinates()
{
}

Position*
Coordinates::GetParentObject() const
{
  return mPosition;
}

JSObject*
Coordinates::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return CoordinatesBinding::Wrap(aCx, this, aGivenProto);
}

#define GENERATE_COORDS_WRAPPED_GETTER(name) \
double                                       \
Coordinates::name() const                    \
{                                            \
  double rv;                                 \
  mCoords->Get##name(&rv);                   \
  return rv;                                 \
}

#define GENERATE_COORDS_WRAPPED_GETTER_NULLABLE(name) \
Nullable<double>                                      \
Coordinates::Get##name() const                        \
{                                                     \
  double rv;                                          \
  mCoords->Get##name(&rv);                            \
  return Nullable<double>(rv);                        \
}

GENERATE_COORDS_WRAPPED_GETTER(Latitude)
GENERATE_COORDS_WRAPPED_GETTER(Longitude)
GENERATE_COORDS_WRAPPED_GETTER_NULLABLE(Altitude)
GENERATE_COORDS_WRAPPED_GETTER(Accuracy)
GENERATE_COORDS_WRAPPED_GETTER_NULLABLE(AltitudeAccuracy)
GENERATE_COORDS_WRAPPED_GETTER_NULLABLE(Heading)
GENERATE_COORDS_WRAPPED_GETTER_NULLABLE(Speed)

#undef GENERATE_COORDS_WRAPPED_GETTER
#undef GENERATE_COORDS_WRAPPED_GETTER_NULLABLE

} // namespace dom
} // namespace mozilla
