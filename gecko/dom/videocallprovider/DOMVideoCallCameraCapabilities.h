/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#ifndef mozilla_dom_domvideocallcameracapabilities_h__
#define mozilla_dom_domvideocallcameracapabilities_h__

#include "nsIVideoCallProvider.h"
#include "nsIVideoCallCallback.h"

namespace mozilla {
namespace dom {

class DOMVideoCallCameraCapabilities final : public nsIVideoCallCameraCapabilities
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIVIDEOCALLCAMERACAPABILITIES

  DOMVideoCallCameraCapabilities(uint16_t aWidth, uint16_t aHeight, bool aZoomSupported, float aMaxZoom);

  void
  Update(nsIVideoCallCameraCapabilities* aCapabilities);

private:
  DOMVideoCallCameraCapabilities() {}

  ~DOMVideoCallCameraCapabilities() {}

private:
  uint16_t mWidth;
  uint16_t mHeight;
  bool mZoomSupported;
  float mMaxZoom;
};

} // dom
}  // mozilla

#endif // mozilla_dom_domvideocallcameracapabilities_h__