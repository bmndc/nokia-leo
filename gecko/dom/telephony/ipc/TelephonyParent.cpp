/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/dom/telephony/TelephonyParent.h"
#include "nsServiceManagerUtils.h"
#ifdef MOZ_WIDGET_GONK
#include "mozilla/dom/videocallprovider/VideoCallProviderParent.h"
#endif
 
#include <android/log.h>
#define DHCUI_DEBUG(args...) __android_log_print(ANDROID_LOG_DEBUG, "DHCUI-debug", ## args)


USING_TELEPHONY_NAMESPACE
USING_VIDEOCALLPROVIDER_NAMESPACE

/*******************************************************************************
 * TelephonyParent
 ******************************************************************************/

NS_IMPL_ISUPPORTS(TelephonyParent, nsITelephonyListener)

TelephonyParent::TelephonyParent()
  : mActorDestroyed(false)
  , mRegistered(false)
{
  DHCUI_DEBUG("TelephonyParent::TelephonyParent %p",this);
}

void
TelephonyParent::ActorDestroy(ActorDestroyReason why)
{
  // The child process could die before this asynchronous notification, in which
  // case ActorDestroy() was called and mActorDestroyed is set to true. Return
  // an error here to avoid sending a message to the dead process.
  mActorDestroyed = true;
  DHCUI_DEBUG("TelephonyParent::ActorDestroy %p",this);
  // Try to unregister listener if we're still registered.
  RecvUnregisterListener();
}

bool
TelephonyParent::RecvPTelephonyRequestConstructor(PTelephonyRequestParent* aActor,
                                                  const IPCTelephonyRequest& aRequest)
{
  TelephonyRequestParent* actor = static_cast<TelephonyRequestParent*>(aActor);
  nsCOMPtr<nsITelephonyService> service = do_GetService(TELEPHONY_SERVICE_CONTRACTID);

  DHCUI_DEBUG("TelephonyParent::RecvPTelephonyRequestConstructor");
  if (!service) {
    return NS_SUCCEEDED(actor->GetCallback()->NotifyError(NS_LITERAL_STRING("InvalidStateError")));
  }

  switch (aRequest.type()) {
    case IPCTelephonyRequest::TEnumerateCallsRequest: {
      service->EnumerateCalls(actor);
      return true;
    }

    case IPCTelephonyRequest::TDialRequest: {
      const DialRequest& request = aRequest.get_DialRequest();
      service->Dial(request.clientId(), request.number(),
                    request.isEmergency(), request.type(),
                    request.rttMode(), actor->GetDialCallback());
      return true;
    }

    case IPCTelephonyRequest::TSendUSSDRequest: {
      const SendUSSDRequest& request = aRequest.get_SendUSSDRequest();
      service->SendUSSD(request.clientId(), request.ussd(), actor->GetCallback());
      return true;
    }

    case IPCTelephonyRequest::TCancelUSSDRequest: {
      const CancelUSSDRequest& request = aRequest.get_CancelUSSDRequest();
      service->CancelUSSD(request.clientId(), actor->GetCallback());
      return true;
    }

    case IPCTelephonyRequest::TConferenceCallRequest: {
      const ConferenceCallRequest& request = aRequest.get_ConferenceCallRequest();
      service->ConferenceCall(request.clientId(), actor->GetCallback());
      return true;
    }

    case IPCTelephonyRequest::TSeparateCallRequest: {
      const SeparateCallRequest& request = aRequest.get_SeparateCallRequest();
      service->SeparateCall(request.clientId(), request.callIndex(), actor->GetCallback());
      return true;
    }

    case IPCTelephonyRequest::THangUpConferenceRequest: {
      const HangUpConferenceRequest& request = aRequest.get_HangUpConferenceRequest();
      service->HangUpConference(request.clientId(), actor->GetCallback());
      return true;
    }

    case IPCTelephonyRequest::THoldConferenceRequest: {
      const HoldConferenceRequest& request = aRequest.get_HoldConferenceRequest();
      service->HoldConference(request.clientId(), actor->GetCallback());
      return true;
    }

    case IPCTelephonyRequest::TResumeConferenceRequest: {
      const ResumeConferenceRequest& request = aRequest.get_ResumeConferenceRequest();
      service->ResumeConference(request.clientId(), actor->GetCallback());
      return true;
    }

    case IPCTelephonyRequest::TAnswerCallRequest: {
      const AnswerCallRequest& request = aRequest.get_AnswerCallRequest();
      service->AnswerCall(request.clientId(), request.callIndex(), request.type(),
                          request.rttMode(), actor->GetCallback());
      return true;
    }

    case IPCTelephonyRequest::THangUpAllCallsRequest: {
      const HangUpAllCallsRequest& request = aRequest.get_HangUpAllCallsRequest();
      service->HangUpAllCalls(request.clientId(), actor->GetCallback());
      return true;
    }

    case IPCTelephonyRequest::THangUpCallRequest: {
      const HangUpCallRequest& request = aRequest.get_HangUpCallRequest();
      service->HangUpCall(request.clientId(), request.callIndex(), request.reason(), actor->GetCallback());
      return true;
    }

    case IPCTelephonyRequest::TRejectCallRequest: {
      const RejectCallRequest& request = aRequest.get_RejectCallRequest();
      service->RejectCall(request.clientId(), request.callIndex(), request.reason(), actor->GetCallback());
      return true;
    }

    case IPCTelephonyRequest::THoldCallRequest: {
      const HoldCallRequest& request = aRequest.get_HoldCallRequest();
      service->HoldCall(request.clientId(), request.callIndex(), actor->GetCallback());
      return true;
    }

    case IPCTelephonyRequest::TResumeCallRequest: {
      const ResumeCallRequest& request = aRequest.get_ResumeCallRequest();
      service->ResumeCall(request.clientId(), request.callIndex(), actor->GetCallback());
      return true;
    }

    case IPCTelephonyRequest::TSendTonesRequest: {
      const SendTonesRequest& request = aRequest.get_SendTonesRequest();
      service->SendTones(request.clientId(),
                         request.dtmfChars(),
                         request.pauseDuration(),
                         request.toneDuration(),
                         actor->GetCallback());
      return true;
    }

    case IPCTelephonyRequest::TSendRttModifyRequest: {
      const SendRttModifyRequest& request = aRequest.get_SendRttModifyRequest();
      service->SendRttModify(request.clientId(), request.callIndex(),
                             request.rttMode(), actor->GetCallback());
      return true;
    }

    case IPCTelephonyRequest::TSendRttModifyResponseRequest: {
      const SendRttModifyResponseRequest& request = aRequest.get_SendRttModifyResponseRequest();
      service->SendRttModifyResponse(request.clientId(), request.callIndex(),
                                     request.status(), actor->GetCallback());
      return true;
    }

    case IPCTelephonyRequest::TSendRttMessageRequest: {
      const SendRttMessageRequest& request = aRequest.get_SendRttMessageRequest();
      service->SendRttMessage(request.clientId(), request.callIndex(),
                              request.message(), actor->GetCallback());
      return true;
    }

    default:
      MOZ_CRASH("Unknown type!");
  }

  return false;
}

PTelephonyRequestParent*
TelephonyParent::AllocPTelephonyRequestParent(const IPCTelephonyRequest& aRequest)
{
  DHCUI_DEBUG("TelephonyParent::AllocPTelephonyRequestParent %p",this);
  TelephonyRequestParent* actor = new TelephonyRequestParent();
  // Add an extra ref for IPDL. Will be released in
  // TelephonyParent::DeallocPTelephonyRequestParent().
  NS_ADDREF(actor);

  return actor;
}

bool
TelephonyParent::DeallocPTelephonyRequestParent(PTelephonyRequestParent* aActor)
{
  // TelephonyRequestParent is refcounted, must not be freed manually.
  DHCUI_DEBUG("TelephonyParent::DeallocPTelephonyRequestParent, %p",this);
  static_cast<TelephonyRequestParent*>(aActor)->Release();
  return true;
}

#ifdef MOZ_WIDGET_GONK
PVideoCallProviderParent*
TelephonyParent::AllocPVideoCallProviderParent(const uint32_t& aClientId,
                                               const uint32_t& aCallindex)
{
  LOG("AllocPVideoCallProviderParent");
  RefPtr<VideoCallProviderParent> parent = new VideoCallProviderParent(aClientId, aCallindex);
  parent->AddRef();

  return parent;
}

bool
TelephonyParent::DeallocPVideoCallProviderParent(PVideoCallProviderParent* aActor)
{
  LOG("DeallocPVideoCallProviderParent");
  static_cast<VideoCallProviderParent*>(aActor)->Release();
  return true;
}
#endif

bool
TelephonyParent::Recv__delete__()
{
  DHCUI_DEBUG("TelephonyParent::Recv__delete__ %p",this);
  return true; // Unregister listener in TelephonyParent::ActorDestroy().
}

bool
TelephonyParent::RecvRegisterListener()
{
  NS_ENSURE_TRUE(!mRegistered, true);

  nsCOMPtr<nsITelephonyService> service =
    do_GetService(TELEPHONY_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(service, true);

  mRegistered = NS_SUCCEEDED(service->RegisterListener(this));
  return true;
}

bool
TelephonyParent::RecvUnregisterListener()
{
  NS_ENSURE_TRUE(mRegistered, true);

  nsCOMPtr<nsITelephonyService> service =
    do_GetService(TELEPHONY_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(service, true);

  mRegistered = !NS_SUCCEEDED(service->UnregisterListener(this));
  return true;
}

bool
TelephonyParent::RecvStartTone(const uint32_t& aClientId, const nsString& aTone)
{
  nsCOMPtr<nsITelephonyService> service =
    do_GetService(TELEPHONY_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(service, true);

  service->StartTone(aClientId, aTone);
  return true;
}

bool
TelephonyParent::RecvStopTone(const uint32_t& aClientId)
{
  nsCOMPtr<nsITelephonyService> service =
    do_GetService(TELEPHONY_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(service, true);

  service->StopTone(aClientId);
  return true;
}

bool
TelephonyParent::RecvGetHacMode(bool* aEnabled)
{
  *aEnabled = false;

  nsCOMPtr<nsITelephonyService> service =
    do_GetService(TELEPHONY_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(service, true);

  service->GetHacMode(aEnabled);
  return true;
}

bool
TelephonyParent::RecvSetHacMode(const bool& aEnabled)
{
  nsCOMPtr<nsITelephonyService> service =
    do_GetService(TELEPHONY_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(service, true);

  service->SetHacMode(aEnabled);
  return true;
}

//[BTV-2476]Merge from argon task5198590-xuemingli@t2mobile.com-begin
bool
TelephonyParent::RecvGetMicMode(uint16_t* aMicMode)
{
  *aMicMode = 0;

  nsCOMPtr<nsITelephonyService> service =
    do_GetService(TELEPHONY_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(service, true);

  service->GetMicMode(aMicMode);
  return true;
}

bool
TelephonyParent::RecvSetMicMode(const uint16_t& aMicMode)
{
  nsCOMPtr<nsITelephonyService> service =
    do_GetService(TELEPHONY_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(service, true);

  service->SetMicMode(aMicMode);
  return true;
}
//[BTV-2476]Merge from argon task5198590-xuemingli@t2mobile.com-end


bool
TelephonyParent::RecvGetMicrophoneMuted(bool* aMuted)
{
  *aMuted = false;

  nsCOMPtr<nsITelephonyService> service =
    do_GetService(TELEPHONY_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(service, true);

  service->GetMicrophoneMuted(aMuted);
  return true;
}

bool
TelephonyParent::RecvSetMicrophoneMuted(const bool& aMuted)
{
  nsCOMPtr<nsITelephonyService> service =
    do_GetService(TELEPHONY_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(service, true);

  service->SetMicrophoneMuted(aMuted);
  return true;
}

bool
TelephonyParent::RecvGetSpeakerEnabled(bool* aEnabled)
{
  *aEnabled = false;

  nsCOMPtr<nsITelephonyService> service =
    do_GetService(TELEPHONY_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(service, true);

  service->GetSpeakerEnabled(aEnabled);
  return true;
}

bool
TelephonyParent::RecvSetSpeakerEnabled(const bool& aEnabled)
{
  nsCOMPtr<nsITelephonyService> service =
    do_GetService(TELEPHONY_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(service, true);

  service->SetSpeakerEnabled(aEnabled);
  return true;
}

bool
TelephonyParent::RecvGetTtyMode(uint16_t* aMode)
{
  *aMode = nsITelephonyService::TTY_MODE_OFF;

  nsCOMPtr<nsITelephonyService> service =
    do_GetService(TELEPHONY_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(service, true);

  service->GetTtyMode(aMode);
  return true;
}

bool
TelephonyParent::RecvSetTtyMode(const uint16_t& aMode)
{
  nsCOMPtr<nsITelephonyService> service =
    do_GetService(TELEPHONY_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(service, true);

  service->SetTtyMode(aMode);
  return true;
}

bool
TelephonyParent::RecvSetMaxTransmitPower(const int32_t& index)
{
  nsCOMPtr<nsITelephonyService> service =
    do_GetService(TELEPHONY_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(service, true);

  //LOG("to call TelephonyParent::RecvSetMaxTransmitPower");

  service->SetMaxTransmitPower(index);
  return true;
}

// nsITelephonyListener

NS_IMETHODIMP
TelephonyParent::CallStateChanged(uint32_t aLength, nsITelephonyCallInfo** aAllInfo)
{
  DHCUI_DEBUG("TelephonyParent::CallStateChanged,%p",this);
  NS_ENSURE_TRUE(!mActorDestroyed, NS_ERROR_FAILURE);

  nsTArray<nsITelephonyCallInfo*> allInfo;
  for (uint32_t i = 0; i < aLength; i++) {
    allInfo.AppendElement(aAllInfo[i]);
  }

  return SendNotifyCallStateChanged(allInfo) ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
TelephonyParent::EnumerateCallStateComplete()
{
  MOZ_CRASH("Not a EnumerateCalls request!");
}

NS_IMETHODIMP
TelephonyParent::EnumerateCallState(nsITelephonyCallInfo* aInfo)
{
  MOZ_CRASH("Not a EnumerateCalls request!");
}

NS_IMETHODIMP
TelephonyParent::NotifyCdmaCallWaiting(uint32_t aClientId,
                                       const nsAString& aNumber,
                                       uint16_t aNumberPresentation,
                                       const nsAString& aName,
                                       uint16_t aNamePresentation)
{
  NS_ENSURE_TRUE(!mActorDestroyed, NS_ERROR_FAILURE);
  DHCUI_DEBUG("TelephonyParent::NotifyCdmaCallWaiting");
  IPCCdmaWaitingCallData data(nsString(aNumber), aNumberPresentation,
                              nsString(aName), aNamePresentation);
  return SendNotifyCdmaCallWaiting(aClientId, data) ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
TelephonyParent::NotifyConferenceError(const nsAString& aName,
                                       const nsAString& aMessage)
{
  NS_ENSURE_TRUE(!mActorDestroyed, NS_ERROR_FAILURE);
  DHCUI_DEBUG("TelephonyParent::NotifyConferenceError");
  return SendNotifyConferenceError(nsString(aName), nsString(aMessage)) ? NS_OK
                                                                        : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
TelephonyParent::NotifyRingbackTone(bool aPlayRingbackTone)
{
  NS_ENSURE_TRUE(!mActorDestroyed, NS_ERROR_FAILURE);
  DHCUI_DEBUG("TelephonyParent::NotifyRingbackTone");
  return SendNotifyRingbackTone(aPlayRingbackTone) ? NS_OK
                                                   : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
TelephonyParent::NotifyTtyModeReceived(uint16_t mode)
{
  NS_ENSURE_TRUE(!mActorDestroyed, NS_ERROR_FAILURE);
  DHCUI_DEBUG("TelephonyParent::NotifyTtyModeReceived");
  return SendNotifyTtyModeReceived(mode) ? NS_OK
                                         : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
TelephonyParent::NotifyTelephonyCoverageLosing(uint16_t aType)
{
  NS_ENSURE_TRUE(!mActorDestroyed, NS_ERROR_FAILURE);
  DHCUI_DEBUG("TelephonyParent::NotifyTelephonyCoverageLosing");
  return SendNotifyTelephonyCoverageLosing(aType) ? NS_OK
                                                  : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
TelephonyParent::SupplementaryServiceNotification(uint32_t aClientId,
                                                  int32_t aNotificationType,
                                                  int32_t aCode,
                                                  int32_t aIndex,
                                                  int32_t aType,
                                                  const nsAString& aNumber)
{
  NS_ENSURE_TRUE(!mActorDestroyed, NS_ERROR_FAILURE);
  DHCUI_DEBUG("TelephonyParent::SupplementaryServiceNotification");
  return SendNotifySupplementaryService(aClientId, aNotificationType, aCode,
      aIndex, aType, nsString(aNumber)) ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
TelephonyParent::NotifyRttModifyRequestReceived(uint32_t aClientId,
                                                int32_t aCallIndex,
                                                uint16_t aRttMode)
{
  NS_ENSURE_TRUE(!mActorDestroyed, NS_ERROR_FAILURE);
  DHCUI_DEBUG("TelephonyParent::NotifyRttModifyRequestReceived");
  return SendNotifyRttModifyRequestReceived(aClientId, aCallIndex, aRttMode)
      ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
TelephonyParent::NotifyRttModifyResponseReceived(uint32_t aClientId,
                                                 int32_t aCallIndex,
                                                 uint16_t aStatus)
{
  NS_ENSURE_TRUE(!mActorDestroyed, NS_ERROR_FAILURE);
  DHCUI_DEBUG("TelephonyParent::NotifyRttModifyResponseReceived");
  return SendNotifyRttModifyResponseReceived(aClientId, aCallIndex, aStatus)
      ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
TelephonyParent::NotifyRttMessageReceived(uint32_t aClientId,
                                          int32_t aCallIndex,
                                          const nsAString& aMessage)
{
  NS_ENSURE_TRUE(!mActorDestroyed, NS_ERROR_FAILURE);
  DHCUI_DEBUG("TelephonyParent::NotifyRttMessageReceived");
  return SendNotifyRttMessageReceived(aClientId, aCallIndex, nsString(aMessage))
      ? NS_OK : NS_ERROR_FAILURE;
}

/*******************************************************************************
 * TelephonyRequestParent
 ******************************************************************************/

NS_IMPL_ISUPPORTS(TelephonyRequestParent,
                  nsITelephonyListener)

TelephonyRequestParent::TelephonyRequestParent()
  : mActorDestroyed(false),
    mCallback(new Callback(this)),
    mDialCallback(new DialCallback(this))
{
  DHCUI_DEBUG("TelephonyRequestParent::TelephonyRequestParent this is %p",this);
}

void
TelephonyRequestParent::ActorDestroy(ActorDestroyReason why)
{
  // The child process could die before this asynchronous notification, in which
  // case ActorDestroy() was called and mActorDestroyed is set to true. Return
  // an error here to avoid sending a message to the dead process.
  DHCUI_DEBUG("TelephonyRequestParent::ActorDestroy %p,mCallback is %p ,mDialCallback is %p",this,mCallback.get(),mDialCallback.get());
  mActorDestroyed = true;
}

nsresult
TelephonyRequestParent::SendResponse(const IPCTelephonyResponse& aResponse)
{
  NS_ENSURE_TRUE(!mActorDestroyed, NS_ERROR_FAILURE);

  DHCUI_DEBUG("TelephonyRequestParent::SendResponse %p",this);
  return Send__delete__(this, aResponse) ? NS_OK : NS_ERROR_FAILURE;
}

// nsITelephonyListener

NS_IMETHODIMP
TelephonyRequestParent::CallStateChanged(uint32_t aLength, nsITelephonyCallInfo** aAllInfo)
{
  MOZ_CRASH("Not a TelephonyParent!");
}

NS_IMETHODIMP
TelephonyRequestParent::EnumerateCallStateComplete()
{
  NS_ENSURE_TRUE(!mActorDestroyed, NS_ERROR_FAILURE);

  DHCUI_DEBUG("TelephonyRequestParent::EnumerateCallStateComplete %p",this);
  return Send__delete__(this, EnumerateCallsResponse()) ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
TelephonyRequestParent::EnumerateCallState(nsITelephonyCallInfo* aInfo)
{
  NS_ENSURE_TRUE(!mActorDestroyed, NS_ERROR_FAILURE);

  DHCUI_DEBUG("TelephonyRequestParent::EnumerateCallState %p",this);
  return SendNotifyEnumerateCallState(aInfo) ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
TelephonyRequestParent::NotifyCdmaCallWaiting(uint32_t aClientId,
                                              const nsAString& aNumber,
                                              uint16_t aNumberPresentation,
                                              const nsAString& aName,
                                              uint16_t aNamePresentation)
{
  MOZ_CRASH("Not a TelephonyParent!");
}

NS_IMETHODIMP
TelephonyRequestParent::NotifyConferenceError(const nsAString& aName,
                                              const nsAString& aMessage)
{
  MOZ_CRASH("Not a TelephonyParent!");
}

NS_IMETHODIMP
TelephonyRequestParent::NotifyRingbackTone(bool aPlayRingbackTone)
{
  MOZ_CRASH("Not a TelephonyParent!");
}

NS_IMETHODIMP
TelephonyRequestParent::NotifyTtyModeReceived(uint16_t aMode)
{
  MOZ_CRASH("Not a TelephonyParent!");
}

NS_IMETHODIMP
TelephonyRequestParent::NotifyTelephonyCoverageLosing(uint16_t aType)
{
  MOZ_CRASH("Not a TelephonyParent!");
}

NS_IMETHODIMP
TelephonyRequestParent::SupplementaryServiceNotification(uint32_t aClientId,
                                                         int32_t aNotificationType,
                                                         int32_t aCode,
                                                         int32_t aIndex,
                                                         int32_t aType,
                                                         const nsAString& aNumber)
{
  MOZ_CRASH("Not a TelephonyParent!");
}

NS_IMETHODIMP
TelephonyRequestParent::NotifyRttModifyRequestReceived(uint32_t aClientId,
                                                       int32_t aCallIndex,
                                                       uint16_t aRttMode)
{
  MOZ_CRASH("Not a TelephonyParent!");
}

NS_IMETHODIMP
TelephonyRequestParent::NotifyRttModifyResponseReceived(uint32_t aClientId,
                                                        int32_t aCallIndex,
                                                        uint16_t aStatus)
{
  MOZ_CRASH("Not a TelephonyParent!");
}

NS_IMETHODIMP
TelephonyRequestParent::NotifyRttMessageReceived(uint32_t aClientId,
                                                 int32_t aCallIndex,
                                                 const nsAString& aMessage)
{
  MOZ_CRASH("Not a TelephonyParent!");
}

/*******************************************************************************
 * TelephonyRequestParent::Callback
 ******************************************************************************/

NS_IMPL_ISUPPORTS(TelephonyRequestParent::Callback,
                  nsITelephonyCallback)

nsresult TelephonyRequestParent::Callback::SendResponse(const IPCTelephonyResponse& aResponse)
{
  if(mParent && !mParent->mActorDestroyed)
  { //ASYNC operation feedback from ril message
    return mParent->SendResponse(aResponse);
  }else{
    LOG("TelephonyRequestParent::Callback::SendResponse return with child process already exit");
    return NS_ERROR_FAILURE;
  }
}

NS_IMETHODIMP
TelephonyRequestParent::Callback::NotifySuccess()
{
  DHCUI_DEBUG("TelephonyRequestParent::Callback::NotifySuccess");
  return SendResponse(SuccessResponse());
}

NS_IMETHODIMP
TelephonyRequestParent::Callback::NotifyError(const nsAString& aError)
{
  DHCUI_DEBUG("TelephonyRequestParent::Callback::NotifyError");
  return SendResponse(ErrorResponse(nsAutoString(aError)));
}

/*******************************************************************************
 * TelephonyRequestParent::DialCallback
 ******************************************************************************/

NS_IMPL_ISUPPORTS_INHERITED(TelephonyRequestParent::DialCallback,
                            TelephonyRequestParent::Callback,
                            nsITelephonyDialCallback)

NS_IMETHODIMP
TelephonyRequestParent::DialCallback::NotifyDialMMI(const nsAString& aServiceCode)
{
  DHCUI_DEBUG("TelephonyRequestParent::DialCallback::NotifyDialMMI");
  if(mParent && !mParent->mActorDestroyed)
  {
    return mParent->SendNotifyDialMMI(nsAutoString(aServiceCode)) ? NS_OK : NS_ERROR_FAILURE;
  }else{
    //ASYNC operation feedback from ril message
    LOG("DialCallback::NotifyDialMMI return with child process already exit");
    return NS_ERROR_FAILURE;
  }
}

NS_IMETHODIMP
TelephonyRequestParent::DialCallback::NotifyDialCallSuccess(uint32_t aClientId,
                                                            uint32_t aCallIndex,
                                                            const nsAString& aNumber,
                                                            bool aIsEmergency,
                                                            uint16_t aRttMode,
                                                            uint16_t aVoiceQuality,
                                                            uint16_t aVideoCallState,
                                                            uint32_t aCapabilities,
                                                            uint16_t aRadioTech)
{
  DHCUI_DEBUG("TelephonyRequestParent::DialCallback::NotifyDialCallSuccess");
  return SendResponse(DialResponseCallSuccess(aClientId, aCallIndex,
                                              nsAutoString(aNumber),
                                              aIsEmergency,
                                              aRttMode,
                                              aVoiceQuality,
                                              aVideoCallState,
                                              aCapabilities,
                                              aRadioTech));
}

NS_IMETHODIMP
TelephonyRequestParent::DialCallback::NotifyDialMMISuccess(const nsAString& aStatusMessage)
{
  DHCUI_DEBUG("TelephonyRequestParent::DialCallback::NotifyDialMMISuccess");
  return SendResponse(DialResponseMMISuccess(nsAutoString(aStatusMessage),
                                             AdditionalInformation(mozilla::void_t())));
}

NS_IMETHODIMP
TelephonyRequestParent::DialCallback::NotifyDialMMISuccessWithInteger(const nsAString& aStatusMessage,
                                                                      uint16_t aAdditionalInformation)
{
  DHCUI_DEBUG("TelephonyRequestParent::DialCallback::NotifyDialMMISuccessWithInteger");
  return SendResponse(DialResponseMMISuccess(nsAutoString(aStatusMessage),
                                             AdditionalInformation(aAdditionalInformation)));
}

NS_IMETHODIMP
TelephonyRequestParent::DialCallback::NotifyDialMMISuccessWithStrings(const nsAString& aStatusMessage,
                                                                      uint32_t aCount,
                                                                      const char16_t** aAdditionalInformation)
{
  nsTArray<nsString> additionalInformation;
  nsString* infos = additionalInformation.AppendElements(aCount);
  DHCUI_DEBUG("TelephonyRequestParent::DialCallback::NotifyDialMMISuccessWithStrings");
  for (uint32_t i = 0; i < aCount; i++) {
    infos[i].Rebind(aAdditionalInformation[i],
                    nsCharTraits<char16_t>::length(aAdditionalInformation[i]));
  }
  DHCUI_DEBUG("TelephonyRequestParent::DialCallback::NotifyDialMMISuccessWithCallForwardingOptions");
  return SendResponse(DialResponseMMISuccess(nsAutoString(aStatusMessage),
                                             AdditionalInformation(additionalInformation)));
}

NS_IMETHODIMP
TelephonyRequestParent::DialCallback::NotifyDialMMISuccessWithCallForwardingOptions(const nsAString& aStatusMessage,
                                                                                    uint32_t aCount,
                                                                                    nsIMobileCallForwardingOptions** aAdditionalInformation)
{
  nsTArray<nsIMobileCallForwardingOptions*> additionalInformation;
  for (uint32_t i = 0; i < aCount; i++) {
    additionalInformation.AppendElement(aAdditionalInformation[i]);
  }

  return SendResponse(DialResponseMMISuccess(nsAutoString(aStatusMessage),
                                             AdditionalInformation(additionalInformation)));
}

NS_IMETHODIMP
TelephonyRequestParent::DialCallback::NotifyDialMMIError(const nsAString& aError)
{
  DHCUI_DEBUG("TelephonyRequestParent::DialCallback::NotifyDialMMIError");
  return SendResponse(DialResponseMMIError(nsAutoString(aError),
                                           AdditionalInformation(mozilla::void_t())));
}

NS_IMETHODIMP
TelephonyRequestParent::DialCallback::NotifyDialMMIErrorWithInfo(const nsAString& aError,
                                                                 uint16_t aInfo)
{
  DHCUI_DEBUG("TelephonyRequestParent::DialCallback::NotifyDialMMIErrorWithInfo");
  return SendResponse(DialResponseMMIError(nsAutoString(aError),
                                           AdditionalInformation(aInfo)));
}
