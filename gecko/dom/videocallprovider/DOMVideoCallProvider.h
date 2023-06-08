/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#ifndef mozilla_dom_domvideocallprovider_h__
#define mozilla_dom_domvideocallprovider_h__

#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/dom/Promise.h"
#include "mozilla/dom/VideoCallProviderBinding.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIVideoCallCallback.h"
#include "nsIVideoCallProvider.h"

class nsPIDOMWindowInner;

namespace mozilla {

class ErrorResult;
class nsDOMCameraControl;
class nsDOMSurfaceControl;

namespace dom {

struct SurfaceConfiguration;

class DOMVideoCallProfile;
class Promise;
class SurfaceControlBack;

class DOMVideoCallProvider final : public DOMEventTargetHelper
                                 , private nsIVideoCallCallback
{
  /**
   * Class DOMVideoCallProvider doesn't actually expose
   * nsIVideoCallCallback. Instead, it owns an
   * nsIVideoCallCallback derived instance mListener and passes it to
   * nsIVideoCallProvider. The onreceived events are first delivered to
   * mListener and then forwarded to its owner, DOMVideoCallProvider.
   * See also bug 775997 comment #51.
   */
  class Listener;

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIVIDEOCALLCALLBACK
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(DOMVideoCallProvider, DOMEventTargetHelper)
  NS_REALLY_FORWARD_NSIDOMEVENTTARGET(DOMEventTargetHelper)

  DOMVideoCallProvider(nsPIDOMWindowInner *aWindow, nsIVideoCallProvider *aHandler);

  virtual void
  DisconnectFromOwner() override;

  // WrapperCache
  virtual JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  // WebIDL APIs
  already_AddRefed<Promise>
  SetCamera(const Optional<nsAString>& aCamera, ErrorResult& aRv);

  already_AddRefed<Promise>
  GetPreviewStream(const SurfaceConfiguration& aOptions, ErrorResult& aRv);

  already_AddRefed<Promise>
  GetDisplayStream(const SurfaceConfiguration& aOptions, ErrorResult& aRv);

  already_AddRefed<Promise>
  SetOrientation(uint16_t aOrientation, ErrorResult& aRv);

  already_AddRefed<Promise>
  SetZoom(float aZoom, ErrorResult& aRv);

  already_AddRefed<Promise>
  SendSessionModifyRequest(const VideoCallProfile& aFrom,
                           const VideoCallProfile& aTo,
                           ErrorResult& aRv);

  already_AddRefed<Promise>
  SendSessionModifyResponse(const VideoCallProfile& aResponse, ErrorResult& aRv);

  already_AddRefed<Promise>
  RequestCameraCapabilities(ErrorResult& aRv);

  IMPL_EVENT_HANDLER(sessionmodifyrequest)
  IMPL_EVENT_HANDLER(sessionmodifyresponse)
  IMPL_EVENT_HANDLER(callsessionevent)
  IMPL_EVENT_HANDLER(changepeerdimensions)
  IMPL_EVENT_HANDLER(changecameracapabilities)
  IMPL_EVENT_HANDLER(changevideoquality)

  // Class API
  void
  SetSurface(const int16_t type, android::sp<android::IGraphicBufferProducer>& aProducer,
             const uint16_t aWidth, const uint16_t aHeight);
  void
  SetDataSourceSize(const int16_t aType, const uint16_t aWidth, const uint16_t aHeight);

  void
  Shutdown();

private:
  ~DOMVideoCallProvider();

  already_AddRefed<Promise>
  GetStream(const int16_t aType, const SurfaceConfiguration& aOptions, ErrorResult& aRv);

  bool
  IsValidSurfaceSize(const uint16_t aWidth, const uint16_t aHeight);

  bool
  ValidOrientation(uint16_t aOrientation);

  bool
  ValidZoom(uint16_t aZoom);

  nsresult
  DispatchSessionModifyRequestEvent(const nsAString& aType, nsIVideoCallProfile *request);

  nsresult
  DispatchSessionModifyResponseEvent(const nsAString& aType,
                                     const uint16_t aStatus,
                                     DOMVideoCallProfile* aRequest,
                                     DOMVideoCallProfile* aResponse);

  nsresult
  ConvertToJsValue(nsIVideoCallProfile *aProfile, JS::Rooted<JS::Value>* jsResult);


  nsresult
  DispatCallSessionEvent(const nsAString& aType, const int16_t aEvent);

  nsresult
  DispatchChangePeerDimensionsEvent(const nsAString& aType, const uint16_t aWidth, const uint16_t aHeight);

  /**
   * TODO
   */
  nsresult
  DispatchCameraCapabilitiesEvent(const nsAString& aType, nsIVideoCallCameraCapabilities* capabilities);

  nsresult
  DispatchVideoQualityChangeEvent(const nsAString& aType, const uint16_t aQuality);

  nsString mCamera;
  uint16_t mOrientation;
  float mZoom;
  float mMaxZoom;
  bool mZoomSupported;

  nsCOMPtr<nsIVideoCallProvider> mProvider;
  RefPtr<Listener> mListener;

  RefPtr<nsDOMSurfaceControl> mDisplayControl;
  RefPtr<nsDOMSurfaceControl> mPreviewControl;
  RefPtr<SurfaceControlBack> mDisplayCallback;
  RefPtr<SurfaceControlBack> mPreviewCallback;
};

} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_domvideocallprovider_h__
