/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#include "DOMVideoCallProfile.h"

namespace mozilla {
namespace dom {

// nsIVideoCallProfile
NS_IMPL_ISUPPORTS(DOMVideoCallProfile, nsIVideoCallProfile)

DOMVideoCallProfile::DOMVideoCallProfile(VideoCallQuality aQuality,
                                         VideoCallState aState)
  : mQuality(aQuality)
  , mState(aState)
{
}

void
DOMVideoCallProfile::Update(nsIVideoCallProfile* aProfile)
{
  uint16_t quality;
  uint16_t state;
  aProfile->GetQuality(&quality);
  aProfile->GetState(&state);

  mQuality = static_cast<VideoCallQuality>(quality);
  mState = static_cast<VideoCallState>(state);
}

NS_IMETHODIMP
DOMVideoCallProfile::GetQuality(uint16_t *aQuality)
{
  *aQuality = static_cast<uint16_t>(mQuality);
  return NS_OK;
}

NS_IMETHODIMP
DOMVideoCallProfile::GetState(uint16_t *aState)
{
  *aState = static_cast<uint16_t>(mState);
  return NS_OK;
}

} // namespace mozilla
} // namespace dom
