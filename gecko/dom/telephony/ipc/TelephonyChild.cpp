/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "TelephonyChild.h"

#include "mozilla/dom/telephony/TelephonyDialCallback.h"
#ifdef MOZ_WIDGET_GONK
#include "mozilla/dom/videocallprovider/VideoCallProviderChild.h"
#endif
#include "mozilla/UniquePtr.h"
#include "TelephonyIPCService.h"

#include <android/log.h>
#define DHCUI_DEBUG(args...) __android_log_print(ANDROID_LOG_DEBUG, "DHCUI-debug", ## args)


USING_TELEPHONY_NAMESPACE
USING_VIDEOCALLPROVIDER_NAMESPACE

/*******************************************************************************
 * TelephonyChild
 ******************************************************************************/

TelephonyChild::TelephonyChild(TelephonyIPCService* aService)
  : mService(aService)
{
  MOZ_ASSERT(aService);
  DHCUI_DEBUG("TelephonyChild::TelephonyChild %p",this);
}

TelephonyChild::~TelephonyChild()
{
  DHCUI_DEBUG("TelephonyChild::~~~TelephonyChild %p",this);
}

void
TelephonyChild::ActorDestroy(ActorDestroyReason aWhy)
{
  DHCUI_DEBUG("TelephonyChild::ActorDestroy %p",this);
  if (mService) {
    mService->NoteActorDestroyed();
    mService = nullptr;
  }
}

PTelephonyRequestChild*
TelephonyChild::AllocPTelephonyRequestChild(const IPCTelephonyRequest& aRequest)
{
  MOZ_CRASH("Caller is supposed to manually construct a request!");
}

bool
TelephonyChild::DeallocPTelephonyRequestChild(PTelephonyRequestChild* aActor)
{
  delete aActor;
  return true;
}

#ifdef MOZ_WIDGET_GONK
PVideoCallProviderChild*
TelephonyChild::AllocPVideoCallProviderChild(const uint32_t& clientId, const uint32_t& callIndex)
{
  NS_NOTREACHED("No one should be allocating PVideoCallProviderChild actors");
  return nullptr;
}

bool
TelephonyChild::DeallocPVideoCallProviderChild(PVideoCallProviderChild* aActor)
{
  // To release a child.
  delete aActor;
  return true;
}
#endif

bool
TelephonyChild::RecvNotifyCallStateChanged(nsTArray<nsITelephonyCallInfo*>&& aAllInfo)
{
  uint32_t length = aAllInfo.Length();
  nsTArray<nsCOMPtr<nsITelephonyCallInfo>> results;
  for (uint32_t i = 0; i < length; ++i) {
    // Use dont_AddRef here because this instance has already been AddRef-ed in
    // TelephonyIPCSerializer.h
    nsCOMPtr<nsITelephonyCallInfo> info = dont_AddRef(aAllInfo[i]);
    results.AppendElement(info);
  }

  MOZ_ASSERT(mService);

  mService->CallStateChanged(length, const_cast<nsITelephonyCallInfo**>(aAllInfo.Elements()));

  return true;
}

bool
TelephonyChild::RecvNotifyCdmaCallWaiting(const uint32_t& aClientId,
                                          const IPCCdmaWaitingCallData& aData)
{
  MOZ_ASSERT(mService);

  mService->NotifyCdmaCallWaiting(aClientId,
                                  aData.number(),
                                  aData.numberPresentation(),
                                  aData.name(),
                                  aData.namePresentation());
  return true;
}

bool
TelephonyChild::RecvNotifyConferenceError(const nsString& aName,
                                          const nsString& aMessage)
{
  MOZ_ASSERT(mService);

  mService->NotifyConferenceError(aName, aMessage);
  return true;
}

bool
TelephonyChild::RecvNotifyRingbackTone(const bool& aPlayRingbackTone)
{
  MOZ_ASSERT(mService);

  mService->NotifyRingbackTone(aPlayRingbackTone);
  return true;
}

bool
TelephonyChild::RecvNotifyTtyModeReceived(const uint16_t& aMode)
{
  MOZ_ASSERT(mService);

  mService->NotifyTtyModeReceived(aMode);
  return true;
}

bool
TelephonyChild::RecvNotifyTelephonyCoverageLosing(const uint16_t& aType)
{
  MOZ_ASSERT(mService);

  mService->NotifyTelephonyCoverageLosing(aType);
  return true;
}

bool
TelephonyChild::RecvNotifySupplementaryService(const uint32_t& aClientId,
                                               const int32_t& aNotificationType,
                                               const int32_t& aCode,
                                               const int32_t& aIndex,
                                               const int32_t& aType,
                                               const nsString& aNumber)
{
  MOZ_ASSERT(mService);

  mService->SupplementaryServiceNotification(aClientId, aNotificationType,
                                             aCode, aIndex, aType, aNumber);
  return true;
}

bool
TelephonyChild::RecvNotifyRttModifyRequestReceived(const uint32_t& aClientId,
                                                   const int32_t& aCallIndex,
                                                   const uint16_t& aRttMode)
{
  MOZ_ASSERT(mService);

  mService->NotifyRttModifyRequestReceived(aClientId, aCallIndex, aRttMode);
  return true;
}

bool
TelephonyChild::RecvNotifyRttModifyResponseReceived(const uint32_t& aClientId,
                                                    const int32_t& aCallIndex,
                                                    const uint16_t& aStatus)
{
  MOZ_ASSERT(mService);

  mService->NotifyRttModifyResponseReceived(aClientId, aCallIndex, aStatus);
  return true;
}

bool
TelephonyChild::RecvNotifyRttMessageReceived(const uint32_t& aClientId,
                                             const int32_t& aCallIndex,
                                             const nsString& aMessage)
{
  MOZ_ASSERT(mService);

  mService->NotifyRttMessageReceived(aClientId, aCallIndex, aMessage);
  return true;
}

/*******************************************************************************
 * TelephonyRequestChild
 ******************************************************************************/

TelephonyRequestChild::TelephonyRequestChild(nsITelephonyListener* aListener,
                                             nsITelephonyCallback* aCallback)
  : mListener(aListener), mCallback(aCallback)
{
  DHCUI_DEBUG("TelephonyRequestChild::TelephonyRequestChild %p",this);
}

void
TelephonyRequestChild::ActorDestroy(ActorDestroyReason aWhy)
{
  mListener = nullptr;
  mCallback = nullptr;
  DHCUI_DEBUG("TelephonyRequestChild::ActorDestroy %p",this);
}

bool
TelephonyRequestChild::Recv__delete__(const IPCTelephonyResponse& aResponse)
{
  DHCUI_DEBUG("TelephonyRequestChild::Recv__delete__ %p",this);
  switch (aResponse.type()) {
    case IPCTelephonyResponse::TEnumerateCallsResponse:
      mListener->EnumerateCallStateComplete();
      break;
    case IPCTelephonyResponse::TSuccessResponse:
      return DoResponse(aResponse.get_SuccessResponse());
    case IPCTelephonyResponse::TErrorResponse:
      return DoResponse(aResponse.get_ErrorResponse());
    case IPCTelephonyResponse::TDialResponseCallSuccess:
      return DoResponse(aResponse.get_DialResponseCallSuccess());
    case IPCTelephonyResponse::TDialResponseMMISuccess:
      return DoResponse(aResponse.get_DialResponseMMISuccess());
    case IPCTelephonyResponse::TDialResponseMMIError:
      return DoResponse(aResponse.get_DialResponseMMIError());
    default:
      MOZ_CRASH("Unknown type!");
  }

  return true;
}

bool
TelephonyRequestChild::RecvNotifyEnumerateCallState(nsITelephonyCallInfo* const& aInfo)
{
  // Use dont_AddRef here because this instances has already been AddRef-ed in
  // TelephonyIPCSerializer.h
  nsCOMPtr<nsITelephonyCallInfo> info = dont_AddRef(aInfo);

  MOZ_ASSERT(mListener);

  mListener->EnumerateCallState(aInfo);

  return true;
}

bool
TelephonyRequestChild::RecvNotifyDialMMI(const nsString& aServiceCode)
{
  MOZ_ASSERT(mCallback);
  DHCUI_DEBUG("TelephonyRequestChild::RecvNotifyDialMMI %p",this);
  nsCOMPtr<nsITelephonyDialCallback> callback = do_QueryInterface(mCallback);
  callback->NotifyDialMMI(aServiceCode);
  return true;
}

bool
TelephonyRequestChild::DoResponse(const SuccessResponse& aResponse)
{
  DHCUI_DEBUG("TelephonyRequestChild::DoResponse 1 %p",this);
  MOZ_ASSERT(mCallback);
  mCallback->NotifySuccess();
  return true;
}

bool
TelephonyRequestChild::DoResponse(const ErrorResponse& aResponse)
{
  DHCUI_DEBUG("TelephonyRequestChild::DoResponse 2 %p",this);
  MOZ_ASSERT(mCallback);
  mCallback->NotifyError(aResponse.name());
  return true;
}

bool
TelephonyRequestChild::DoResponse(const DialResponseCallSuccess& aResponse)
{
  DHCUI_DEBUG("TelephonyRequestChild::DoResponse 3 %p",this);
  MOZ_ASSERT(mCallback);
  nsCOMPtr<nsITelephonyDialCallback> callback = do_QueryInterface(mCallback);
  callback->NotifyDialCallSuccess(aResponse.clientId(), aResponse.callIndex(),
                                  aResponse.number(),
                                  aResponse.isEmergency(),
                                  aResponse.rttMode(),
                                  aResponse.voiceQuality(),
                                  aResponse.videoCallState(),
                                  aResponse.capabilities(),
                                  aResponse.radioTech());
  return true;
}

bool
TelephonyRequestChild::DoResponse(const DialResponseMMISuccess& aResponse)
{
  DHCUI_DEBUG("TelephonyRequestChild::DoResponse 4 %p",this);
  MOZ_ASSERT(mCallback);
  nsCOMPtr<nsITelephonyDialCallback> callback = do_QueryInterface(mCallback);

  nsAutoString statusMessage(aResponse.statusMessage());
  AdditionalInformation info(aResponse.additionalInformation());

  switch (info.type()) {
    case AdditionalInformation::Tvoid_t:
      callback->NotifyDialMMISuccess(statusMessage);
      break;
    case AdditionalInformation::Tuint16_t:
      callback->NotifyDialMMISuccessWithInteger(statusMessage, info.get_uint16_t());
      break;
    case AdditionalInformation::TArrayOfnsString: {
      uint32_t count = info.get_ArrayOfnsString().Length();
      const nsTArray<nsString>& additionalInformation = info.get_ArrayOfnsString();

      auto additionalInfoPtrs = MakeUnique<const char16_t*[]>(count);
      for (size_t i = 0; i < count; ++i) {
        additionalInfoPtrs[i] = additionalInformation[i].get();
      }

      callback->NotifyDialMMISuccessWithStrings(statusMessage, count,
                                                additionalInfoPtrs.get());
      break;
    }
    case AdditionalInformation::TArrayOfnsMobileCallForwardingOptions: {
      uint32_t count = info.get_ArrayOfnsMobileCallForwardingOptions().Length();

      nsTArray<nsCOMPtr<nsIMobileCallForwardingOptions>> results;
      for (uint32_t i = 0; i < count; i++) {
        // Use dont_AddRef here because these instances are already AddRef-ed in
        // MobileConnectionIPCSerializer.h
        nsCOMPtr<nsIMobileCallForwardingOptions> item = dont_AddRef(
          info.get_ArrayOfnsMobileCallForwardingOptions()[i]);
        results.AppendElement(item);
      }

      callback->NotifyDialMMISuccessWithCallForwardingOptions(statusMessage, count,
        const_cast<nsIMobileCallForwardingOptions**>(info.get_ArrayOfnsMobileCallForwardingOptions().Elements()));
      break;
    }
    default:
      MOZ_CRASH("Received invalid type!");
      break;
  }

  return true;
}

bool
TelephonyRequestChild::DoResponse(const DialResponseMMIError& aResponse)
{
  DHCUI_DEBUG("TelephonyRequestChild::DoResponse 5 %p",this);
  MOZ_ASSERT(mCallback);
  nsCOMPtr<nsITelephonyDialCallback> callback = do_QueryInterface(mCallback);

  nsAutoString name(aResponse.name());
  AdditionalInformation info(aResponse.additionalInformation());

  switch (info.type()) {
    case AdditionalInformation::Tvoid_t:
      callback->NotifyDialMMIError(name);
      break;
    case AdditionalInformation::Tuint16_t:
      callback->NotifyDialMMIErrorWithInfo(name, info.get_uint16_t());
      break;
    default:
      MOZ_CRASH("Received invalid type!");
      break;
  }

  return true;
}
