/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef dom_src_geolocation_IPC_serialiser
#define dom_src_geolocation_IPC_serialiser

#include "ipc/IPCMessageUtils.h"
#include "nsGeoPosition.h"
#include "nsIDOMGeoPosition.h"
#include "nsIDOMGeoPositionExtended.h"

typedef nsIDOMGeoPosition* GeoPosition;

namespace IPC {

template <>
struct ParamTraits<nsIDOMGeoPositionCoords*>
{
  typedef nsIDOMGeoPositionCoords* paramType;

  // Function to serialize a geoposition
  static void Write(Message *aMsg, const paramType& aParam)
  {
    bool isNull = !aParam;
    WriteParam(aMsg, isNull);
    // If it is a null object, then we are done
    if (isNull) return;

    double coordData;

    aParam->GetLatitude(&coordData);
    WriteParam(aMsg, coordData);

    aParam->GetLongitude(&coordData);
    WriteParam(aMsg, coordData);

    aParam->GetAltitude(&coordData);
    WriteParam(aMsg, coordData);

    aParam->GetAccuracy(&coordData);
    WriteParam(aMsg, coordData);

    aParam->GetAltitudeAccuracy(&coordData);
    WriteParam(aMsg, coordData);

    aParam->GetHeading(&coordData);
    WriteParam(aMsg, coordData);

    aParam->GetSpeed(&coordData);
    WriteParam(aMsg, coordData);
  }

  // Function to de-serialize a geoposition
  static bool Read(const Message* aMsg, void **aIter, paramType* aResult)
  {
    // Check if it is the null pointer we have transfered
    bool isNull;
    if (!ReadParam(aMsg, aIter, &isNull)) return false;

    if (isNull) {
      *aResult = 0;
      return true;
    }

    double latitude;
    double longitude;
    double altitude;
    double accuracy;
    double altitudeAccuracy;
    double heading;
    double speed;

    // It's not important to us where it fails, but rather if it fails
    if (!(   ReadParam(aMsg, aIter, &latitude         )
          && ReadParam(aMsg, aIter, &longitude        )
          && ReadParam(aMsg, aIter, &altitude         )
          && ReadParam(aMsg, aIter, &accuracy         )
          && ReadParam(aMsg, aIter, &altitudeAccuracy )
          && ReadParam(aMsg, aIter, &heading          )
          && ReadParam(aMsg, aIter, &speed            ))) return false;

    // We now have all the data
    *aResult = new nsGeoPositionCoords(latitude,         /* aLat     */
                                       longitude,        /* aLong    */
                                       altitude,         /* aAlt     */
                                       accuracy,         /* aHError  */
                                       altitudeAccuracy, /* aVError  */
                                       heading,          /* aHeading */
                                       speed             /* aSpeed   */
                                      );
    return true;

  }

};

template <>
struct ParamTraits<nsIDOMGeoPosition*>
{
  typedef nsIDOMGeoPosition* paramType;

  // Function to serialize a geoposition
  static void Write(Message *aMsg, const paramType& aParam)
  {
    bool isNull = !aParam;
    WriteParam(aMsg, isNull);
    // If it is a null object, then we are done
    if (isNull) return;

    DOMTimeStamp timeStamp;
    aParam->GetTimestamp(&timeStamp);
    WriteParam(aMsg, timeStamp);

    nsCOMPtr<nsIDOMGeoPositionCoords> coords;
    aParam->GetCoords(getter_AddRefs(coords));
    WriteParam(aMsg, coords.get());

    nsCOMPtr<nsIDOMGeoPositionExtended> positionEx = do_QueryInterface(aParam);
    if (positionEx) {
      nsString positioningMethod;
      positionEx->GetPositioningMethod(positioningMethod);
      WriteParam(aMsg, positioningMethod);

      DOMTimeStamp gnssUtcTime;
      positionEx->GetGnssUtcTime(&gnssUtcTime);
      WriteParam(aMsg, gnssUtcTime);
    }
  }

  // Function to de-serialize a geoposition
  static bool Read(const Message* aMsg, void **aIter, paramType* aResult)
  {
    // Check if it is the null pointer we have transfered
    bool isNull;
    if (!ReadParam(aMsg, aIter, &isNull)) return false;

    if (isNull) {
      *aResult = 0;
      return true;
    }

    DOMTimeStamp timeStamp;
    nsIDOMGeoPositionCoords* coords = nullptr;

    // It's not important to us where it fails, but rather if it fails
    if (!ReadParam(aMsg, aIter, &timeStamp) ||
        !ReadParam(aMsg, aIter, &coords)) {
      nsCOMPtr<nsIDOMGeoPositionCoords> tmpcoords = coords;
      return false;
    }

    nsString positioningMethod;
    DOMTimeStamp gnssUtcTime;
    if (!ReadParam(aMsg, aIter, &positioningMethod) ||
        !ReadParam(aMsg, aIter, &gnssUtcTime)) {
      // The position implements nsIDOMGeoPosition instead of
      // nsIDOMGeoPositionExtended.
      *aResult = new nsGeoPosition(coords, timeStamp);
      return true;
    }

    mozilla::dom::PositionSource method =
      nsGeoPosition::ConvertStringToPositionSource(positioningMethod);

    *aResult = new nsGeoPosition(coords, timeStamp, method, gnssUtcTime);
    return true;
  };

};

} // namespace IPC

#endif
