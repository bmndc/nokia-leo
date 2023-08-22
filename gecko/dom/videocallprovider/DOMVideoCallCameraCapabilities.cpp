/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#include "DOMVideoCallCameraCapabilities.h"

using namespace mozilla::dom;

// nsIVideoCallCameraCapabilities
NS_IMPL_ISUPPORTS(DOMVideoCallCameraCapabilities, nsIVideoCallCameraCapabilities)

DOMVideoCallCameraCapabilities::DOMVideoCallCameraCapabilities(uint16_t aWidth, uint16_t aHeight,
                                                               bool aZoomSupported, float aMaxZoom)
  : mWidth(aWidth)
  , mHeight(aHeight)
  , mZoomSupported(aZoomSupported)
  , mMaxZoom(aMaxZoom)
{
}

void
DOMVideoCallCameraCapabilities::Update(nsIVideoCallCameraCapabilities* aCapabilities)
{
  aCapabilities->GetWidth(&mWidth);
  aCapabilities->GetHeight(&mHeight);
  aCapabilities->GetZoomSupported(&mZoomSupported);
  aCapabilities->GetMaxZoom(&mMaxZoom);
}

// nsIVideoCallCameraCapabilities

NS_IMETHODIMP
DOMVideoCallCameraCapabilities::GetWidth(uint16_t *aWidth)
{
  *aWidth = mWidth;
  return NS_OK;
}

NS_IMETHODIMP
DOMVideoCallCameraCapabilities::GetHeight(uint16_t *aHeight)
{
  *aHeight = mHeight;
  return NS_OK;
}

NS_IMETHODIMP
DOMVideoCallCameraCapabilities::GetZoomSupported(bool *aZoomSupported)
{
  *aZoomSupported = mZoomSupported;
  return NS_OK;
}

NS_IMETHODIMP
DOMVideoCallCameraCapabilities::GetMaxZoom(float *aMaxZoom)
{
  *aMaxZoom = mMaxZoom;
  return NS_OK;
}