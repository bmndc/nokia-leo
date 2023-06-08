/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#include "mozilla/dom/videocallprovider/VideoCallProviderChild.h"

#include <android/log.h>
#undef LOG
#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "VideoCallProviderChild" , ## args)

using namespace mozilla::dom;
using namespace mozilla::dom::videocallprovider;

// VideoCallProviderChild

VideoCallProviderChild::VideoCallProviderChild(uint32_t aClientId, uint32_t aCallIndex)
  : mLive(true),
    mClientId(aClientId),
    mCallIndex(aCallIndex)
{
  LOG("constructor");
  MOZ_COUNT_CTOR(VideoCallProviderChild);
}

void
VideoCallProviderChild::Shutdown()
{
  LOG("Shutdown");
  if (mLive) {
    LOG("Send__delete__");
    Send__delete__(this);
    mLive = false;
  }

  mCallbacks.Clear();
}

void
VideoCallProviderChild::ActorDestroy(ActorDestroyReason aWhy)
{
  LOG("ActorDestroy");
  mLive = false;
}

bool
VideoCallProviderChild::RecvNotifyReceiveSessionModifyRequest(nsIVideoCallProfile* const& aRequest)
{
  LOG("RecvNotifyReceiveSessionModifyRequest");
  nsCOMPtr<nsIVideoCallProfile> request = dont_AddRef(aRequest);

  for (int32_t i = 0; i < mCallbacks.Count(); i++) {
    mCallbacks[i]->OnReceiveSessionModifyRequest(request);
  }

  return true;
}

bool
VideoCallProviderChild::RecvNotifyReceiveSessionModifyResponse(const uint16_t& status,
                                                               nsIVideoCallProfile* const& aRequest,
                                                               nsIVideoCallProfile* const& aResponse)
{
  LOG("RecvNotifyReceiveSessionModifyResponse");

  nsCOMPtr<nsIVideoCallProfile> request = dont_AddRef(aRequest);
  nsCOMPtr<nsIVideoCallProfile> response = dont_AddRef(aResponse);

  for (int32_t i = 0; i < mCallbacks.Count(); i++) {
    mCallbacks[i]->OnReceiveSessionModifyResponse(status, request, response);
  }

  return true;
}

bool
VideoCallProviderChild::RecvNotifyHandleCallSessionEvent(const uint16_t& aEvent)
{
  LOG("RecvNotifyHandleCallSessionEvent");
  for (int32_t i = 0; i < mCallbacks.Count(); i++) {
    mCallbacks[i]->OnHandleCallSessionEvent(aEvent);
  }
  return true;
}

bool
VideoCallProviderChild::RecvNotifyChangePeerDimensions(const uint16_t& aWidth, const uint16_t& aHeight)
{
  LOG("RecvNotifyChangePeerDimensions");
  for (int32_t i = 0; i < mCallbacks.Count(); i++) {
    mCallbacks[i]->OnChangePeerDimensions(aWidth, aHeight);
  }
  return true;
}

bool
VideoCallProviderChild::RecvNotifyChangeCameraCapabilities(nsIVideoCallCameraCapabilities* const& aCapabilities)
{
  LOG("RecvNotifyChangeCameraCapabilities");
  nsCOMPtr<nsIVideoCallCameraCapabilities> capabilities = dont_AddRef(aCapabilities);
  for (int32_t i = 0; i < mCallbacks.Count(); i++) {
    mCallbacks[i]->OnChangeCameraCapabilities(capabilities);
  }
  return true;
}

bool
VideoCallProviderChild::RecvNotifyChangeVideoQuality(const uint16_t& aQuality)
{
  LOG("RecvNotifyChangeVideoQuality");
  for (int32_t i = 0; i < mCallbacks.Count(); i++) {
    mCallbacks[i]->OnChangeVideoQuality(aQuality);
  }
  return true;
}

// nsIVideoCallProvider
NS_IMPL_ISUPPORTS(VideoCallProviderChild, nsIVideoCallProvider)

NS_IMETHODIMP
VideoCallProviderChild::SetCamera(int16_t cameraId)
{
  LOG("SetCamera:%d", cameraId);
  SendSetCamera(cameraId);
  return NS_OK;
}

NS_IMETHODIMP
VideoCallProviderChild::SetPreviewSurface(android::sp<android::IGraphicBufferProducer> & producer,
                                          uint16_t aWidth, uint16_t aHeight)
{
  LOG("SetPreviewSurface");
  // TODO to pass producer
  SendSetPreviewSurface(aWidth, aHeight);
  return NS_OK;
}

NS_IMETHODIMP
VideoCallProviderChild::SetDisplaySurface(android::sp<android::IGraphicBufferProducer> & producer,
                                          uint16_t aWidth, uint16_t aHeight)
{
  LOG("SetDisplaySurface");
  // TODO to pass producer
  SendSetDisplaySurface(aWidth, aHeight);
  return NS_OK;
}

NS_IMETHODIMP
VideoCallProviderChild::SetDeviceOrientation(uint16_t orientation)
{
  LOG("SetDeviceOrientation");
  SendSetDeviceOrientation(orientation);
  return NS_OK;
}

NS_IMETHODIMP
VideoCallProviderChild::SetZoom(float value)
{
  LOG("SetZoom: %f", value);
  SendSetZoom(value);
  return NS_OK;
}

NS_IMETHODIMP
VideoCallProviderChild::SendSessionModifyRequest(nsIVideoCallProfile *fromProfile, nsIVideoCallProfile *toProfile)
{
  SendSendSessionModifyRequest(fromProfile, toProfile);
  return NS_OK;
}

NS_IMETHODIMP
VideoCallProviderChild::SendSessionModifyResponse(nsIVideoCallProfile *aResponse)
{
  SendSendSessionModifyResponse(aResponse);
  return NS_OK;
}

NS_IMETHODIMP
VideoCallProviderChild::RequestCameraCapabilities(void)
{
  LOG("RequestCameraCapabilities");
  SendRequestCameraCapabilities();
  return NS_OK;
}

NS_IMETHODIMP
VideoCallProviderChild::RegisterCallback(nsIVideoCallCallback *aCallback)
{
  LOG("RegisterCallback");
  NS_ENSURE_TRUE(!mCallbacks.Contains(aCallback), NS_ERROR_UNEXPECTED);
  mCallbacks.AppendObject(aCallback);
  return NS_OK;
}

NS_IMETHODIMP
VideoCallProviderChild::UnregisterCallback(nsIVideoCallCallback *aCallback)
{
  LOG("UnregisterCallback");
  NS_ENSURE_TRUE(mCallbacks.Contains(aCallback), NS_ERROR_UNEXPECTED);
  mCallbacks.RemoveObject(aCallback);
  return NS_OK;
}