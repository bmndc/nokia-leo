/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef AudioDestinationNode_h_
#define AudioDestinationNode_h_

#include "mozilla/dom/AudioChannelBinding.h"
#include "AudioNode.h"
#include "nsIAudioChannelAgent.h"

namespace mozilla {
namespace dom {

class AudioContext;
class AudioChannelHelper;

class AudioDestinationNode final : public AudioNode
                                 , public nsIAudioChannelAgentCallback
                                 , public MainThreadMediaStreamListener
{
public:
  // This node type knows what MediaStreamGraph to use based on
  // whether it's in offline mode.
  AudioDestinationNode(AudioContext* aContext,
                       bool aIsOffline,
                       AudioChannel aChannel = AudioChannel::Normal,
                       uint32_t aNumberOfChannels = 0,
                       uint32_t aLength = 0,
                       float aSampleRate = 0.0f);

  void DestroyMediaStream() override;

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(AudioDestinationNode, AudioNode)
  NS_DECL_NSIAUDIOCHANNELAGENTCALLBACK

  JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  uint16_t NumberOfOutputs() const final override
  {
    return 0;
  }

  uint32_t MaxChannelCount() const;
  void SetChannelCount(uint32_t aChannelCount,
                       ErrorResult& aRv) override;

  // Returns the stream or null after unlink.
  AudioNodeStream* Stream() { return mStream; }

  void Mute();
  void Unmute();

  void Suspend();
  void Resume();

  void StartRendering(Promise* aPromise);

  void OfflineShutdown();

  AudioChannel MozAudioChannelType() const;

  void NotifyMainThreadStreamFinished() override;
  void FireOfflineCompletionEvent();

  // An amount that should be added to the MediaStream's current time to
  // get the AudioContext.currentTime.
  StreamTime ExtraCurrentTime();

  // When aIsOnlyNode is true, this is the only node for the AudioContext.
  void SetIsOnlyNodeForContext(bool aIsOnlyNode);

  void EnsureAudioChannelAgent();
  void DestroyAudioChannelAgent();

  // Called when AudioChannelAgent::NotifyStartedPlaying() or
  // AudioChannelAgent::NotifyStoppedPlaying() is called.
  void AudioChannelStateChanged(bool aPlaying);

  // Force the agent to grab audio channel immediately and stay at playing
  // state for |aDurationMs|. This function can be called multiple times and
  // the duration of them can be overlapped with each other. After the
  // duration is finished, as long as it's still in the duration of another
  // call, or the input of AudioDestinationNode is not muted, the agent stays
  // at playing state.
  void ForceAudioChannelPlaying(uint32_t aDurationMs);

  bool IsAudioChannelPlaying();

  const char* NodeType() const override
  {
    return "AudioDestinationNode";
  }

  size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const override;
  size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const override;

  void InputMuted(bool aInputMuted);
  void ResolvePromise(AudioBuffer* aRenderedBuffer);

protected:
  virtual ~AudioDestinationNode();

private:
  void SetMozAudioChannelType(AudioChannel aValue, ErrorResult& aRv);
  bool CheckAudioChannelPermissions(AudioChannel aValue);

  void SetCanPlay(float aVolume, bool aMuted);

  void NotifyStableState();
  void ScheduleStableStateNotification();

  SelfReference<AudioDestinationNode> mOfflineRenderingRef;
  uint32_t mFramesToProduce;

  RefPtr<AudioChannelHelper> mAudioChannelHelper;
  RefPtr<MediaInputPort> mCaptureStreamPort;

  RefPtr<Promise> mOfflineRenderingPromise;

  // Audio Channel Type.
  AudioChannel mAudioChannel;
  bool mIsOffline;
  bool mAudioChannelMuted;

  TimeStamp mStartedBlockingDueToBeingOnlyNode;
  StreamTime mExtraCurrentTimeSinceLastStartedBlocking;
  bool mExtraCurrentTimeUpdatedSinceLastStableState;
  bool mCaptured;
};

} // namespace dom
} // namespace mozilla

#endif

