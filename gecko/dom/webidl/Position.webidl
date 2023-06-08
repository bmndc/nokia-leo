/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * The origin of this IDL file is
 * http://www.w3.org/TR/geolocation-API
 *
 * Copyright © 2012 W3C® (MIT, ERCIM, Keio), All Rights Reserved. W3C
 * liability, trademark and document use rules apply.
 */

// non-standard: The possible positioning methods (sources) of the position.
enum PositionSource { "gnss", "wifi", "cell", "wifi-and-cell", "unknown" };

[NoInterfaceObject]
interface Position {
  readonly attribute Coordinates coords;
  readonly attribute DOMTimeStamp timestamp;

  // non-standard: The positioning method used to determine the position.
  [AvailableIn=CertifiedApps]
  readonly attribute PositionSource positioningMethod;

  // non-standard: The UTC time in ms from GNSS position.
  // The value is 0 if the position isn't from GNSS
  [AvailableIn=CertifiedApps]
  readonly attribute DOMTimeStamp gnssUtcTime;
};
