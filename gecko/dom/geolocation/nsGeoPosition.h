/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef nsGeoPosition_h
#define nsGeoPosition_h

#include "nsAutoPtr.h"
#include "nsIDOMGeoPositionCoords.h"
#include "nsIDOMGeoPosition.h"
#include "nsIDOMGeoPositionExtended.h"
#include "nsString.h"
#include "mozilla/Attributes.h"
#include "mozilla/dom/Nullable.h"
#include "mozilla/dom/PositionBinding.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"
#include "js/TypeDecls.h"

////////////////////////////////////////////////////
// nsGeoPositionCoords
////////////////////////////////////////////////////

/**
 * Simple object that holds a single point in space.
 */
class nsGeoPositionCoords final : public nsIDOMGeoPositionCoords
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIDOMGEOPOSITIONCOORDS

  nsGeoPositionCoords(double aLat, double aLong,
                      double aAlt, double aHError,
                      double aVError, double aHeading,
                      double aSpeed);
private:
  ~nsGeoPositionCoords();
  const double mLat, mLong, mAlt, mHError, mVError, mHeading, mSpeed;
};


////////////////////////////////////////////////////
// nsGeoPosition
////////////////////////////////////////////////////

class nsGeoPosition final : public nsIDOMGeoPositionExtended
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIDOMGEOPOSITION
  NS_DECL_NSIDOMGEOPOSITIONEXTENDED

  nsGeoPosition(double aLat, double aLong,
                double aAlt, double aHError,
                double aVError, double aHeading,
                double aSpeed, long long aTimestamp,
                mozilla::dom::PositionSource aMethod
                  = mozilla::dom::PositionSource::Unknown,
                long long aGnssUtcTime = 0);

  nsGeoPosition(nsIDOMGeoPositionCoords *aCoords,
                long long aTimestamp,
                mozilla::dom::PositionSource aMethod
                  = mozilla::dom::PositionSource::Unknown,
                long long aGnssUtcTime = 0);

  nsGeoPosition(nsIDOMGeoPositionCoords *aCoords,
                DOMTimeStamp aTimestamp,
                mozilla::dom::PositionSource aMethod
                  = mozilla::dom::PositionSource::Unknown,
                DOMTimeStamp aGnssUtcTime = 0);

  static mozilla::dom::PositionSource
    ConvertStringToPositionSource(const nsAString& aMethod);

private:
  ~nsGeoPosition();
  void CorrectTimestamp(long long aTimestamp);

  long long mTimestamp;
  RefPtr<nsIDOMGeoPositionCoords> mCoords;
  mozilla::dom::PositionSource mPositioningMethod;
  long long mGnssUtcTime;
};

////////////////////////////////////////////////////
// WebIDL wrappers for the classes above
////////////////////////////////////////////////////

namespace mozilla {
namespace dom {

class Coordinates;

class Position final : public nsISupports,
                       public nsWrapperCache
{
  ~Position();

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(Position)

public:
  Position(nsISupports* aParent, nsIDOMGeoPosition* aGeoPosition);

  nsISupports* GetParentObject() const;

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  Coordinates* Coords();

  uint64_t Timestamp() const;

  PositionSource PositioningMethod() const;

  uint64_t GnssUtcTime() const;

  nsIDOMGeoPosition* GetWrappedGeoPosition() { return mGeoPosition; }

private:
  RefPtr<Coordinates> mCoordinates;
  nsCOMPtr<nsISupports> mParent;
  nsCOMPtr<nsIDOMGeoPosition> mGeoPosition;
  nsCOMPtr<nsIDOMGeoPositionExtended> mGeoPositionEx;
  bool mIsHighAccuracy;
};

class Coordinates final : public nsISupports,
                          public nsWrapperCache
{
  ~Coordinates();

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(Coordinates)

public:
  Coordinates(Position* aPosition, nsIDOMGeoPositionCoords* aCoords);

  Position* GetParentObject() const;

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  double Latitude() const;

  double Longitude() const;

  Nullable<double> GetAltitude() const;

  double Accuracy() const;

  Nullable<double> GetAltitudeAccuracy() const;

  Nullable<double> GetHeading() const;

  Nullable<double> GetSpeed() const;
private:
  RefPtr<Position> mPosition;
  nsCOMPtr<nsIDOMGeoPositionCoords> mCoords;
};

} // namespace dom
} // namespace mozilla

#endif /* nsGeoPosition_h */

