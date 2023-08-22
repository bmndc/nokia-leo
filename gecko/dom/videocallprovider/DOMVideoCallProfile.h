/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#ifndef mozilla_dom_domvideocallprofile_h__
#define mozilla_dom_domvideocallprofile_h__

#include "mozilla/dom/VideoCallProviderBinding.h"
#include "nsIVideoCallProvider.h"

namespace mozilla {
namespace dom {

class DOMVideoCallProfile final : public nsIVideoCallProfile
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIVIDEOCALLPROFILE

  DOMVideoCallProfile(VideoCallQuality aQuality, VideoCallState aState);

  void
  Update(nsIVideoCallProfile* aProfile);

private:
  // Don't try to use the default constructor.
  DOMVideoCallProfile() {}

  ~DOMVideoCallProfile() {}

private:
  VideoCallQuality mQuality;
  VideoCallState mState;
};

} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_domvideocallprofile_h__
