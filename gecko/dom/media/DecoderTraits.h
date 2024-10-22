/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DecoderTraits_h_
#define DecoderTraits_h_

#include "nsCOMPtr.h"

class nsAString;
class nsACString;
class nsIChannel;
class nsIStreamListener;

namespace mozilla {

class AbstractMediaDecoder;
class DecoderDoctorDiagnostics;
class MediaDecoder;
class MediaDecoderOwner;
class MediaDecoderReader;

enum CanPlayStatus {
  CANPLAY_NO,
  CANPLAY_MAYBE,
  CANPLAY_YES
};

class DecoderTraits {
public:
  // Returns the CanPlayStatus indicating if we can handle this
  // MIME type. The MIME type should not include the codecs parameter.
  // That parameter should be passed in aRequestedCodecs, and will only be
  // used if whether a given MIME type being handled depends on the
  // codec that will be used.  If the codecs parameter has not been
  // specified, pass false in aHaveRequestedCodecs.
  static CanPlayStatus CanHandleMediaType(const char* aMIMEType,
                                          bool aHaveRequestedCodecs,
                                          const nsAString& aRequestedCodecs,
                                          DecoderDoctorDiagnostics* aDiagnostics);

  // Returns the CanPlayStatus indicating if we can handle this MIME type and
  // codecs type natively, excluding any plugins-based reader (such as
  // GStreamer)
  // The MIME type should not include the codecs parameter.
  // That parameter is passed in aRequestedCodecs.
  static CanPlayStatus CanHandleCodecsType(const char* aMIMEType,
                                           const nsAString& aRequestedCodecs,
                                           DecoderDoctorDiagnostics* aDiagnostics);

  // Returns true if we should handle this MIME type when it appears
  // as an <object> or as a toplevel page. If, in practice, our support
  // for the type is more limited than appears in the wild, we should return
  // false here even if CanHandleMediaType would return true.
  static bool ShouldHandleMediaType(const char* aMIMEType,
                                    DecoderDoctorDiagnostics* aDiagnostics);

  // Create a decoder for the given aType. Returns null if we
  // were unable to create the decoder.
  static already_AddRefed<MediaDecoder> CreateDecoder(const nsACString& aType,
                                                      MediaDecoderOwner* aOwner,
                                                      DecoderDoctorDiagnostics* aDiagnostics,
                                                      bool aIsMSE,
                                                      nsIChannel* aChannel = nullptr,
                                                      nsIStreamListener** aStreamListener = nullptr);

  // Create a reader for thew given MIME type aType. Returns null
  // if we were unable to create the reader.
  static MediaDecoderReader* CreateReader(const nsACString& aType,
                                          AbstractMediaDecoder* aDecoder);

  // Returns true if MIME type aType is supported in video documents,
  // or false otherwise. Not all platforms support all MIME types, and
  // vice versa.
  static bool IsSupportedInVideoDocument(const nsACString& aType);

  // Returns true if we should not start decoder until we receive
  // OnConnected signal. (currently RTSP only)
  static bool DecoderWaitsForOnConnected(const nsACString& aType);

  static bool IsWebMTypeAndEnabled(const nsACString& aType);
  static bool IsWebMAudioType(const nsACString& aType);
  static bool IsMP4TypeAndEnabled(const nsACString& aType,
                                  DecoderDoctorDiagnostics* aDiagnostics);
#ifdef MOZ_WIDGET_GONK
  //For VideoOffloadPlayer
  static bool IsVideoOffloadPlayerSupportedType(const nsACString& aType);
#endif
};

} // namespace mozilla

#endif

