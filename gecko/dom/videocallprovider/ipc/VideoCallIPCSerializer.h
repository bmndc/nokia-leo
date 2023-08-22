/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#ifndef mozilla_dom_videocallprovider_videocallprovideripcserializer_h__
#define mozilla_dom_videocallprovider_videocallprovideripcserializer_h__

#include "ipc/IPCMessageUtils.h"
#include "mozilla/dom/DOMVideoCallCameraCapabilities.h"
#include "mozilla/dom/DOMVideoCallProfile.h"
#include "mozilla/dom/VideoCallProviderBinding.h"
#include "nsIVideoCallProvider.h"
#include "nsIVideoCallCallback.h"

using mozilla::dom::VideoCallQuality;
using mozilla::dom::VideoCallState;
using mozilla::dom::DOMVideoCallProfile;
using mozilla::dom::DOMVideoCallCameraCapabilities;

typedef nsIVideoCallProfile* nsVideoCallProfile;
typedef nsIVideoCallCameraCapabilities* nsVideoCallCameraCapabilities;

namespace IPC {

/**
 * nsIVideoCallProfile Serialize/De-serialize.
 */
template <>
struct ParamTraits<nsIVideoCallProfile*>
{
  typedef nsIVideoCallProfile* paramType;

  // Function to serialize a DOMVideoCallProfile.
  static void Write(Message* aMsg, const paramType& aParam)
  {
    bool isNull = !aParam;
    WriteParam(aMsg, isNull);
    // If it is a null object, then we are don.
    if (isNull) {
      return;
    }

    uint16_t quality;
    uint16_t state;

    aParam->GetQuality(&quality);
    aParam->GetState(&state);

    WriteParam(aMsg, quality);
    WriteParam(aMsg, state);
    aParam->Release();
  }

  // Function to de-serialize a DOMVideoCallProfile.
  static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
  {
    // Check if is the null pointer we have transferred.
    bool isNull;
    if (!ReadParam(aMsg, aIter, &isNull)) {
      return false;
    }

    if (isNull) {
      *aResult = nullptr;
    }

    uint16_t quality;
    uint16_t state;

    if (!(ReadParam(aMsg, aIter, &quality) &&
          ReadParam(aMsg, aIter, &state))) {
      return false;
    }

    *aResult = new DOMVideoCallProfile(static_cast<VideoCallQuality>(quality),
                                       static_cast<VideoCallState>(state));
    // We release this ref after receiver finishes processing.
    NS_ADDREF(*aResult);

    return true;
  }
};

/**
 * nsIVideoCallCameraCapabilities Serialize/De-serialize.
 */
template <>
struct ParamTraits<nsIVideoCallCameraCapabilities*>
{
  typedef nsIVideoCallCameraCapabilities* paramType;

  // Function to serialize a DOMVideoCallCameraCapabilities.
  static void Write(Message* aMsg, const paramType& aParam)
  {
    bool isNull = !aParam;
    WriteParam(aMsg, isNull);
    // If it is a null object, then we are don.
    if (isNull) {
      return;
    }

    uint16_t width;
    uint16_t height;
    bool zoomSupported;
    float maxZoom;

    aParam->GetWidth(&width);
    aParam->GetHeight(&height);
    aParam->GetZoomSupported(&zoomSupported);
    aParam->GetMaxZoom(&maxZoom);

    WriteParam(aMsg, width);
    WriteParam(aMsg, height);
    WriteParam(aMsg, zoomSupported);
    WriteParam(aMsg, maxZoom);
    aParam->Release();
  }

  // Function to de-serialize a DOMVideoCallCameraCapabilities.
  static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
  {
    // Check if is the null pointer we have transferred.
    bool isNull;
    if (!ReadParam(aMsg, aIter, &isNull)) {
      return false;
    }

    if (isNull) {
      *aResult = nullptr;
    }

    uint16_t width;
    uint16_t height;
    bool zoomSupported;
    float maxZoom;

    if (!(ReadParam(aMsg, aIter, &width) &&
          ReadParam(aMsg, aIter, &height) &&
          ReadParam(aMsg, aIter, &zoomSupported) &&
          ReadParam(aMsg, aIter, &maxZoom))) {
      return false;
    }

    *aResult = new DOMVideoCallCameraCapabilities(width, height, zoomSupported, maxZoom);
    // We release this ref after receiver finishes processing.
    NS_ADDREF(*aResult);

    return true;
  }
};

} // namespace IPC

#endif // mozilla_dom_videocallprovider_videocallprovideripcserializer_h__