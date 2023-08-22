/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#ifndef mozilla_dom_videocallprovider_VideoCallProviderParent_h__
#define mozilla_dom_videocallprovider_VideoCallProviderParent_h__

#include "mozilla/dom/videocallprovider/PVideoCallProviderParent.h"
#include "nsIVideoCallProvider.h"
#include "nsIVideoCallCallback.h"

namespace mozilla {
namespace dom {
namespace videocallprovider {

class VideoCallProviderParent : public PVideoCallProviderParent
                              , public nsIVideoCallCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIVIDEOCALLCALLBACK

  explicit VideoCallProviderParent(uint32_t aClientId, uint32_t aCallIndex);

protected:
  virtual
  ~VideoCallProviderParent()
  {
    MOZ_COUNT_DTOR(VideoCallProviderParent);
  }

  virtual void
  ActorDestroy(ActorDestroyReason aWhy) override;

  virtual bool
  RecvSetCamera(const int16_t& cameraId) override;

  virtual bool
  RecvSetPreviewSurface(const uint16_t& aWidth,
                        const uint16_t& aHeight) override;

  virtual bool
  RecvSetDisplaySurface(const uint16_t& aWidth,
                        const uint16_t& aHeight) override;

  virtual bool
  RecvSetDeviceOrientation(const uint16_t& aOrientation) override;

  virtual bool
  RecvSetZoom(const float& aValue) override;

  virtual bool
  RecvSendSessionModifyRequest(const nsVideoCallProfile& aFromProfile,
                               const nsVideoCallProfile& aToProfile) override;

  virtual bool
  RecvSendSessionModifyResponse(const nsVideoCallProfile& aResponse) override;

  virtual bool
  RecvRequestCameraCapabilities() override;

private:
  uint32_t mClientId;
  uint32_t mCallIndex;
  nsCOMPtr<nsIVideoCallProvider> mProvider;
};

} // videocallprovider
} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_videocallprovider_VideoCallProviderParent_h__
