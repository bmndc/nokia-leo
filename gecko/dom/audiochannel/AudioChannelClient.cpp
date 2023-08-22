/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* (c) 2018 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#include "AudioChannelClient.h"
#include "AudioChannelAgent.h"
#include "AudioChannelService.h"
#include "nsIPermissionManager.h"
#include "nsIScriptObjectPrincipal.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_INHERITED(AudioChannelClient, DOMEventTargetHelper,
                                   mAgent)

NS_IMPL_ADDREF_INHERITED(AudioChannelClient, DOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(AudioChannelClient, DOMEventTargetHelper)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(AudioChannelClient)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(nsIAudioChannelAgentCallback)
NS_INTERFACE_MAP_END_INHERITING(DOMEventTargetHelper)

/* static */ already_AddRefed<AudioChannelClient>
AudioChannelClient::Constructor(const GlobalObject& aGlobal,
                                AudioChannel aChannel,
                                ErrorResult& aRv)
{
  nsCOMPtr<nsPIDOMWindowInner> window = do_QueryInterface(aGlobal.GetAsSupports());
  if (!window) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  if (!CheckAudioChannelPermissions(window, aChannel)) {
    aRv.ThrowDOMException(NS_ERROR_FAILURE, NS_LITERAL_CSTRING("Permission denied"));
    return nullptr;
  }

  RefPtr<AudioChannelClient> object = new AudioChannelClient(window, aChannel);
  aRv = NS_OK;
  return object.forget();
}

AudioChannelClient::AudioChannelClient(nsPIDOMWindowInner* aWindow,
                                       AudioChannel aChannel)
  : DOMEventTargetHelper(aWindow)
  , mChannel(aChannel)
  , mMuted(true)
{
  MOZ_LOG(AudioChannelService::GetAudioChannelLog(), LogLevel::Debug,
         ("AudioChannelClient, created with channel type %d", (int)aChannel));
}

AudioChannelClient::~AudioChannelClient()
{
  MOZ_LOG(AudioChannelService::GetAudioChannelLog(), LogLevel::Debug,
         ("AudioChannelClient, destroyed"));
}

/* static */ bool
AudioChannelClient::CheckAudioChannelPermissions(nsPIDOMWindowInner* aWindow, AudioChannel aChannel)
{
  // Only normal channel doesn't need permission.
  if (aChannel == AudioChannel::Normal) {
    return true;
  }

  // Maybe this audio channel is equal to the default one.
  if (aChannel == AudioChannelService::GetDefaultAudioChannel()) {
    return true;
  }

  nsCOMPtr<nsIPermissionManager> permissionManager =
    services::GetPermissionManager();
  if (!permissionManager) {
    return false;
  }

  nsCOMPtr<nsIScriptObjectPrincipal> sop = do_QueryInterface(aWindow);
  NS_ASSERTION(sop, "Window didn't QI to nsIScriptObjectPrincipal!");
  nsCOMPtr<nsIPrincipal> principal = sop->GetPrincipal();

  uint32_t perm = nsIPermissionManager::UNKNOWN_ACTION;

  nsCString channel;
  channel.AssignASCII(AudioChannelValues::strings[uint32_t(aChannel)].value,
                      AudioChannelValues::strings[uint32_t(aChannel)].length);
  permissionManager->TestExactPermissionFromPrincipal(principal,
    nsCString(NS_LITERAL_CSTRING("audio-channel-") + channel).get(),
    &perm);

  return perm == nsIPermissionManager::ALLOW_ACTION;
}

JSObject*
AudioChannelClient::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return AudioChannelClientBinding::Wrap(aCx, this, aGivenProto);
}

void
AudioChannelClient::RequestChannel(ErrorResult& aRv)
{
  if (mAgent) {
    return;
  }

  MOZ_LOG(AudioChannelService::GetAudioChannelLog(), LogLevel::Debug,
         ("AudioChannelClient, request channel"));

  mAgent = new AudioChannelAgent();
  aRv = mAgent->InitWithWeakCallback(GetOwner(),
                                     static_cast<int32_t>(mChannel),
                                     this);
  if (NS_WARN_IF(aRv.Failed())) {
    mAgent = nullptr;
    return;
  }

  float volume = 0.0;
  bool muted = true;
  aRv = mAgent->NotifyStartedPlaying(&volume, &muted);
  if (NS_WARN_IF(aRv.Failed())) {
    mAgent = nullptr;
    return;
  }

  WindowVolumeChanged(volume, muted);
}

void
AudioChannelClient::AbandonChannel(ErrorResult& aRv)
{
  if (!mAgent) {
    return;
  }

  MOZ_LOG(AudioChannelService::GetAudioChannelLog(), LogLevel::Debug,
         ("AudioChannelClient, abandon channel"));

  mAgent->NotifyStoppedPlaying();
  mAgent = nullptr;
  mMuted = true;
}

NS_IMETHODIMP
AudioChannelClient::WindowVolumeChanged(float aVolume, bool aMuted)
{
  if (mMuted != aMuted) {
    mMuted = aMuted;
    MOZ_LOG(AudioChannelService::GetAudioChannelLog(), LogLevel::Debug,
           ("AudioChannelClient, state changed, muted %d", mMuted));
    DispatchTrustedEvent(NS_LITERAL_STRING("statechange"));
  }
  return NS_OK;
}

NS_IMETHODIMP
AudioChannelClient::WindowAudioCaptureChanged(bool aCapture)
{
  return NS_OK;
}

} // namespace dom
} // namespace mozilla
