/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* (c) 2018 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#ifndef mozilla_dom_AudioChannelClient_h__
#define mozilla_dom_AudioChannelClient_h__

#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/dom/AudioChannelClientBinding.h"
#include "nsIAudioChannelAgent.h"
#include "nsWeakReference.h"

namespace mozilla {
namespace dom {

class AudioChannelClient final : public DOMEventTargetHelper
                               , public nsSupportsWeakReference
                               , public nsIAudioChannelAgentCallback
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(AudioChannelClient, DOMEventTargetHelper)
  NS_DECL_NSIAUDIOCHANNELAGENTCALLBACK

  static already_AddRefed<AudioChannelClient>
  Constructor(const GlobalObject& aGlobal,
              AudioChannel aChannel,
              ErrorResult& aRv);

  nsPIDOMWindowInner* GetParentObject() const
  {
    return GetOwner();
  }

  JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  void RequestChannel(ErrorResult& aRv);

  void AbandonChannel(ErrorResult& aRv);

  bool ChannelMuted() const
  {
    return mMuted;
  }

  IMPL_EVENT_HANDLER(statechange)

private:
  AudioChannelClient(nsPIDOMWindowInner* aWindow, AudioChannel aChannel);
  ~AudioChannelClient();

  static bool CheckAudioChannelPermissions(nsPIDOMWindowInner* aWindow, AudioChannel aChannel);

  nsCOMPtr<nsIAudioChannelAgent> mAgent;
  AudioChannel mChannel;
  bool mMuted;
};

} // namespace dom
} // namespace mozilla

#endif
