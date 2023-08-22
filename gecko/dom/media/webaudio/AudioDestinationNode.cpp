/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "AudioDestinationNode.h"
#include "AlignmentUtils.h"
#include "AudioContext.h"
#include "mozilla/dom/AudioDestinationNodeBinding.h"
#include "mozilla/dom/ScriptSettings.h"
#include "mozilla/Preferences.h"
#include "mozilla/Services.h"
#include "AudioChannelAgent.h"
#include "AudioChannelService.h"
#include "AudioNodeEngine.h"
#include "AudioNodeStream.h"
#include "MediaStreamGraph.h"
#include "OfflineAudioCompletionEvent.h"
#include "nsContentUtils.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIDocShell.h"
#include "nsIPermissionManager.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsITimer.h"
#include "nsServiceManagerUtils.h"
#include "mozilla/dom/Promise.h"

namespace mozilla {
namespace dom {

extern LazyLogModule gAudioContextLog;

#define LOG(msg, ...) \
  MOZ_LOG(gAudioContextLog, LogLevel::Debug, (msg, ##__VA_ARGS__))
#define CTX_LOG(ctx, msg, ...) \
  MOZ_LOG(gAudioContextLog, LogLevel::Debug, ("(%p) " msg, ctx, ##__VA_ARGS__))

static uint8_t gWebAudioOutputKey;

class OfflineDestinationNodeEngine final : public AudioNodeEngine
{
public:
  OfflineDestinationNodeEngine(AudioDestinationNode* aNode,
                               uint32_t aNumberOfChannels,
                               uint32_t aLength,
                               float aSampleRate)
    : AudioNodeEngine(aNode)
    , mWriteIndex(0)
    , mNumberOfChannels(aNumberOfChannels)
    , mLength(aLength)
    , mSampleRate(aSampleRate)
    , mBufferAllocated(false)
  {
  }

  void ProcessBlock(AudioNodeStream* aStream,
                    GraphTime aFrom,
                    const AudioBlock& aInput,
                    AudioBlock* aOutput,
                    bool* aFinished) override
  {
    // Do this just for the sake of political correctness; this output
    // will not go anywhere.
    *aOutput = aInput;

    // The output buffer is allocated lazily, on the rendering thread, when
    // non-null input is received.
    if (!mBufferAllocated && !aInput.IsNull()) {
      // These allocations might fail if content provides a huge number of
      // channels or size, but it's OK since we'll deal with the failure
      // gracefully.
      mBuffer = ThreadSharedFloatArrayBufferList::
        Create(mNumberOfChannels, mLength, fallible);
      if (mBuffer && mWriteIndex) {
        // Zero leading for any null chunks that were skipped.
        for (uint32_t i = 0; i < mNumberOfChannels; ++i) {
          float* channelData = mBuffer->GetDataForWrite(i);
          PodZero(channelData, mWriteIndex);
        }
      }

      mBufferAllocated = true;
    }

    // Skip copying if there is no buffer.
    uint32_t outputChannelCount = mBuffer ? mNumberOfChannels : 0;

    // Record our input buffer
    MOZ_ASSERT(mWriteIndex < mLength, "How did this happen?");
    const uint32_t duration = std::min(WEBAUDIO_BLOCK_SIZE, mLength - mWriteIndex);
    const uint32_t inputChannelCount = aInput.ChannelCount();
    for (uint32_t i = 0; i < outputChannelCount; ++i) {
      float* outputData = mBuffer->GetDataForWrite(i) + mWriteIndex;
      if (aInput.IsNull() || i >= inputChannelCount) {
        PodZero(outputData, duration);
      } else {
        const float* inputBuffer = static_cast<const float*>(aInput.mChannelData[i]);
        if (duration == WEBAUDIO_BLOCK_SIZE && IS_ALIGNED16(inputBuffer)) {
          // Use the optimized version of the copy with scale operation
          AudioBlockCopyChannelWithScale(inputBuffer, aInput.mVolume,
                                         outputData);
        } else {
          if (aInput.mVolume == 1.0f) {
            PodCopy(outputData, inputBuffer, duration);
          } else {
            for (uint32_t j = 0; j < duration; ++j) {
              outputData[j] = aInput.mVolume * inputBuffer[j];
            }
          }
        }
      }
    }
    mWriteIndex += duration;

    if (mWriteIndex >= mLength) {
      NS_ASSERTION(mWriteIndex == mLength, "Overshot length");
      // Go to finished state. When the graph's current time eventually reaches
      // the end of the stream, then the main thread will be notified and we'll
      // shut down the AudioContext.
      *aFinished = true;
    }
  }

  bool IsActive() const override
  {
    // Keep processing to track stream time, which is used for all timelines
    // associated with the same AudioContext.
    return true;
  }


  class OnCompleteTask final : public nsRunnable
  {
  public:
    OnCompleteTask(AudioContext* aAudioContext, AudioBuffer* aRenderedBuffer)
      : mAudioContext(aAudioContext)
      , mRenderedBuffer(aRenderedBuffer)
    {}

    NS_IMETHOD Run() override
    {
      RefPtr<OfflineAudioCompletionEvent> event =
          new OfflineAudioCompletionEvent(mAudioContext, nullptr, nullptr);
      event->InitEvent(mRenderedBuffer);
      mAudioContext->DispatchTrustedEvent(event);

      return NS_OK;
    }
  private:
    RefPtr<AudioContext> mAudioContext;
    RefPtr<AudioBuffer> mRenderedBuffer;
  };

  void FireOfflineCompletionEvent(AudioDestinationNode* aNode)
  {
    AudioContext* context = aNode->Context();
    context->Shutdown();
    // Shutdown drops self reference, but the context is still referenced by aNode,
    // which is strongly referenced by the runnable that called
    // AudioDestinationNode::FireOfflineCompletionEvent.

    // Create the input buffer
    ErrorResult rv;
    RefPtr<AudioBuffer> renderedBuffer =
      AudioBuffer::Create(context, mNumberOfChannels, mLength, mSampleRate,
                          mBuffer.forget(), rv);
    if (rv.Failed()) {
      return;
    }

    aNode->ResolvePromise(renderedBuffer);

    RefPtr<OnCompleteTask> onCompleteTask =
      new OnCompleteTask(context, renderedBuffer);
    NS_DispatchToMainThread(onCompleteTask);

    context->OnStateChanged(nullptr, AudioContextState::Closed);
  }

  size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const override
  {
    size_t amount = AudioNodeEngine::SizeOfExcludingThis(aMallocSizeOf);
    if (mBuffer) {
      amount += mBuffer->SizeOfIncludingThis(aMallocSizeOf);
    }
    return amount;
  }

  size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const override
  {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }

private:
  // The input to the destination node is recorded in mBuffer.
  // When this buffer fills up with mLength frames, the buffered input is sent
  // to the main thread in order to dispatch OfflineAudioCompletionEvent.
  RefPtr<ThreadSharedFloatArrayBufferList> mBuffer;
  // An index representing the next offset in mBuffer to be written to.
  uint32_t mWriteIndex;
  uint32_t mNumberOfChannels;
  // How many frames the OfflineAudioContext intends to produce.
  uint32_t mLength;
  float mSampleRate;
  bool mBufferAllocated;
};

class InputMutedRunnable final : public nsRunnable
{
public:
  InputMutedRunnable(AudioNodeStream* aStream,
                     bool aInputMuted)
    : mStream(aStream)
    , mInputMuted(aInputMuted)
  {
  }

  NS_IMETHOD Run() override
  {
    MOZ_ASSERT(NS_IsMainThread());
    RefPtr<AudioNode> node = mStream->Engine()->NodeMainThread();

    if (node) {
      RefPtr<AudioDestinationNode> destinationNode =
        static_cast<AudioDestinationNode*>(node.get());
      destinationNode->InputMuted(mInputMuted);
    }
    return NS_OK;
  }

private:
  RefPtr<AudioNodeStream> mStream;
  bool mInputMuted;
};

class DestinationNodeEngine final : public AudioNodeEngine
{
public:
  explicit DestinationNodeEngine(AudioDestinationNode* aNode)
    : AudioNodeEngine(aNode)
    , mVolume(1.0f)
    , mLastInputMuted(true)
    , mSuspended(false)
  {
    MOZ_ASSERT(aNode);
  }

  void ProcessBlock(AudioNodeStream* aStream,
                    GraphTime aFrom,
                    const AudioBlock& aInput,
                    AudioBlock* aOutput,
                    bool* aFinished) override
  {
    *aOutput = aInput;
    aOutput->mVolume *= mVolume;

    if (mSuspended) {
      return;
    }

    bool newInputMuted = aInput.IsNull() || aInput.IsMuted();
    if (newInputMuted != mLastInputMuted) {
      mLastInputMuted = newInputMuted;

      LOG("Destination node input is %s", (newInputMuted ? "muted" : "unmuted"));

      RefPtr<InputMutedRunnable> runnable =
        new InputMutedRunnable(aStream, newInputMuted);
      aStream->Graph()->
        DispatchToMainThreadAfterStreamStateUpdate(runnable.forget());
    }
  }

  bool IsActive() const override
  {
    // Keep processing to track stream time, which is used for all timelines
    // associated with the same AudioContext.  If there are no other engines
    // for the AudioContext, then this could return false to suspend the
    // stream, but the stream is blocked anyway through
    // AudioDestinationNode::SetIsOnlyNodeForContext().
    return true;
  }

  void SetDoubleParameter(uint32_t aIndex, double aParam) override
  {
    if (aIndex == VOLUME) {
      mVolume = aParam;
    }
  }

  void SetInt32Parameter(uint32_t aIndex, int32_t aParam) override
  {
    if (aIndex == SUSPENDED) {
      mSuspended = !!aParam;
      if (mSuspended) {
        mLastInputMuted = true;
      }
    }
  }

  enum Parameters {
    VOLUME,
    SUSPENDED,
  };

  size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const override
  {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }

private:
  float mVolume;
  bool mLastInputMuted;
  bool mSuspended;
};

static bool UseAudioChannelAPI()
{
  return Preferences::GetBool("media.useAudioChannelAPI");
}

class AudioChannelHelper
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(AudioChannelHelper);

  static already_AddRefed<AudioChannelHelper>
  Create(AudioDestinationNode* aNode, AudioChannel aChannel)
  {
    MOZ_ASSERT(NS_IsMainThread());

    RefPtr<AudioChannelHelper> helper = new AudioChannelHelper(aNode, aChannel);
    nsresult rv = helper->Init();
    if (NS_FAILED(rv)) {
      return nullptr;
    }
    return helper.forget();
  }

  void SetInputPlaying(bool aPlaying)
  {
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(mAgent);

    LOG("AudioChannelHelper input %s playing", (aPlaying ? "starts" : "stops"));
    mInputPlaying = aPlaying;
    HandlePlayingState();
  }

  void ForcePlaying(uint32_t aDurationMs)
  {
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(mAgent);

    LOG("AudioChannelHelper starts force playing");
    CancelTimerIfArmed();
    nsresult rv = ArmTimer(aDurationMs);
    NS_ENSURE_SUCCESS_VOID(rv);

    mForcedPlaying = true;
    HandlePlayingState();
  }

  bool IsPlaying()
  {
    MOZ_ASSERT(NS_IsMainThread());
    return mAgentPlaying;
  }

private:
  AudioChannelHelper(AudioDestinationNode* aNode, AudioChannel aChannel)
    : mDestinationNode(aNode)
    , mChannel(aChannel)
    , mForcedPlaying(false)
    , mInputPlaying(false)
    , mAgentPlaying(false)
    , mTimerArmed(false)
  {
    LOG("Creating AudioChannelHelper with channel %d", (int)aChannel);
    mTimer = do_CreateInstance("@mozilla.org/timer;1");
  }

  ~AudioChannelHelper()
  {
    CancelTimerIfArmed();
    if (mAgentPlaying) {
      StopPlaying();
    }
    LOG("AudioChannelHelper destroyed");
  }

  nsresult Init()
  {
    mAgent = new AudioChannelAgent();
    nsresult rv = mAgent->InitWithWeakCallback(mDestinationNode->GetOwner(),
                                               static_cast<int32_t>(mChannel),
                                               mDestinationNode);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      mAgent = nullptr;
    }
    return rv;
  }

  void HandlePlayingState()
  {
    bool playing = mForcedPlaying || mInputPlaying;
    if (playing == mAgentPlaying) {
      return;
    }
    if (playing) {
      StartPlaying();
    } else {
      StopPlaying();
    }
  }

  void StartPlaying()
  {
    float volume = 0.0;
    bool muted = true;
    nsresult rv = mAgent->NotifyStartedPlaying(&volume, &muted);
    NS_ENSURE_SUCCESS_VOID(rv);

    mAgentPlaying = true;
    mDestinationNode->AudioChannelStateChanged(mAgentPlaying);
    mDestinationNode->WindowVolumeChanged(volume, muted);
  }

  void StopPlaying()
  {
    nsresult rv = mAgent->NotifyStoppedPlaying();
    NS_ENSURE_SUCCESS_VOID(rv);

    mAgentPlaying = false;
    mDestinationNode->AudioChannelStateChanged(mAgentPlaying);
  }

  void ResetForcePlaying()
  {
    LOG("AudioChannelHelper reset force playing");
    mForcedPlaying = false;
    HandlePlayingState();
  }

  nsresult ArmTimer(uint32_t aDelayMs)
  {
    nsresult rv = mTimer->InitWithNamedFuncCallback(&TimerCallback, this, aDelayMs,
                                                    nsITimer::TYPE_ONE_SHOT,
                                                    "AudioChannelHelper::TimerCallback");
    NS_ENSURE_SUCCESS(rv, rv);
    mTimerArmed = true;
    return rv;
  }

  void CancelTimerIfArmed()
  {
    if (mTimerArmed) {
      mTimer->Cancel();
      mTimerArmed = false;
    }
  }

  static void TimerCallback(nsITimer* aTimer, void* aClosure)
  {
    static_cast<AudioChannelHelper*>(aClosure)->ResetForcePlaying();
  }

  AudioDestinationNode* mDestinationNode;
  nsCOMPtr<nsIAudioChannelAgent> mAgent;
  nsCOMPtr<nsITimer> mTimer;
  AudioChannel mChannel;
  bool mForcedPlaying;
  bool mInputPlaying;
  bool mAgentPlaying;
  bool mTimerArmed;
};


NS_IMPL_CYCLE_COLLECTION_INHERITED(AudioDestinationNode, AudioNode,
                                   mOfflineRenderingPromise)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(AudioDestinationNode)
  NS_INTERFACE_MAP_ENTRY(nsIAudioChannelAgentCallback)
NS_INTERFACE_MAP_END_INHERITING(AudioNode)

NS_IMPL_ADDREF_INHERITED(AudioDestinationNode, AudioNode)
NS_IMPL_RELEASE_INHERITED(AudioDestinationNode, AudioNode)

AudioDestinationNode::AudioDestinationNode(AudioContext* aContext,
                                           bool aIsOffline,
                                           AudioChannel aChannel,
                                           uint32_t aNumberOfChannels,
                                           uint32_t aLength, float aSampleRate)
  : AudioNode(aContext, aIsOffline ? aNumberOfChannels : 2,
              ChannelCountMode::Explicit, ChannelInterpretation::Speakers)
  , mFramesToProduce(aLength)
  , mAudioChannel(AudioChannel::Normal)
  , mIsOffline(aIsOffline)
  , mAudioChannelMuted(false)
  , mExtraCurrentTimeSinceLastStartedBlocking(0)
  , mExtraCurrentTimeUpdatedSinceLastStableState(false)
  , mCaptured(false)
{
  MediaStreamGraph* graph = aIsOffline ?
                            MediaStreamGraph::CreateNonRealtimeInstance(aSampleRate) :
                            MediaStreamGraph::GetInstance(MediaStreamGraph::SYSTEM_THREAD_DRIVER, aChannel);
  AudioNodeEngine* engine = aIsOffline ?
                            new OfflineDestinationNodeEngine(this, aNumberOfChannels,
                                                             aLength, aSampleRate) :
                            static_cast<AudioNodeEngine*>(new DestinationNodeEngine(this));

  AudioNodeStream::Flags flags =
    AudioNodeStream::NEED_MAIN_THREAD_CURRENT_TIME |
    AudioNodeStream::NEED_MAIN_THREAD_FINISHED |
    AudioNodeStream::EXTERNAL_OUTPUT;
  mStream = AudioNodeStream::Create(aContext, engine, flags, graph);
  mStream->AddMainThreadListener(this);
  mStream->AddAudioOutput(&gWebAudioOutputKey);

  if (!aIsOffline) {
    SetCanPlay(1.0, true);
    graph->NotifyWhenGraphStarted(mStream);
  }

  if (aChannel != AudioChannel::Normal) {
    ErrorResult rv;
    SetMozAudioChannelType(aChannel, rv);
  }
}

AudioDestinationNode::~AudioDestinationNode()
{
}

size_t
AudioDestinationNode::SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const
{
  size_t amount = AudioNode::SizeOfExcludingThis(aMallocSizeOf);
  return amount;
}

size_t
AudioDestinationNode::SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const
{
  return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
}

void
AudioDestinationNode::EnsureAudioChannelAgent()
{
  if (mIsOffline || mAudioChannelHelper) {
    return;
  }

  mAudioChannelHelper = AudioChannelHelper::Create(this, mAudioChannel);
}

void
AudioDestinationNode::DestroyAudioChannelAgent()
{
  mAudioChannelHelper = nullptr;
}

void
AudioDestinationNode::DestroyMediaStream()
{
  DestroyAudioChannelAgent();

  if (!mStream)
    return;

  mStream->RemoveMainThreadListener(this);
  MediaStreamGraph* graph = mStream->Graph();
  if (graph->IsNonRealtime()) {
    MediaStreamGraph::DestroyNonRealtimeInstance(graph);
  }
  AudioNode::DestroyMediaStream();
}

void
AudioDestinationNode::NotifyMainThreadStreamFinished()
{
  MOZ_ASSERT(mStream->IsFinished());

  if (mIsOffline) {
    nsCOMPtr<nsIRunnable> runnable =
      NS_NewRunnableMethod(this, &AudioDestinationNode::FireOfflineCompletionEvent);
    NS_DispatchToCurrentThread(runnable);
  }
}

void
AudioDestinationNode::FireOfflineCompletionEvent()
{
  OfflineDestinationNodeEngine* engine =
    static_cast<OfflineDestinationNodeEngine*>(Stream()->Engine());
  engine->FireOfflineCompletionEvent(this);
}

void
AudioDestinationNode::ResolvePromise(AudioBuffer* aRenderedBuffer)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mIsOffline);
  mOfflineRenderingPromise->MaybeResolve(aRenderedBuffer);
}

uint32_t
AudioDestinationNode::MaxChannelCount() const
{
  return Context()->MaxChannelCount();
}

void
AudioDestinationNode::SetChannelCount(uint32_t aChannelCount, ErrorResult& aRv)
{
  if (aChannelCount > MaxChannelCount()) {
    aRv.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return;
  }

  AudioNode::SetChannelCount(aChannelCount, aRv);
}

void
AudioDestinationNode::Mute()
{
  MOZ_ASSERT(Context() && !Context()->IsOffline());
  SendDoubleParameterToStream(DestinationNodeEngine::VOLUME, 0.0f);
}

void
AudioDestinationNode::Unmute()
{
  MOZ_ASSERT(Context() && !Context()->IsOffline());
  SendDoubleParameterToStream(DestinationNodeEngine::VOLUME, 1.0f);
}

void
AudioDestinationNode::Suspend()
{
  DestroyAudioChannelAgent();
  SendInt32ParameterToStream(DestinationNodeEngine::SUSPENDED, 1);
}

void
AudioDestinationNode::Resume()
{
  EnsureAudioChannelAgent();
  SendInt32ParameterToStream(DestinationNodeEngine::SUSPENDED, 0);
}

void
AudioDestinationNode::OfflineShutdown()
{
  MOZ_ASSERT(Context() && Context()->IsOffline(),
             "Should only be called on a valid OfflineAudioContext");

  MediaStreamGraph::DestroyNonRealtimeInstance(mStream->Graph());
  mOfflineRenderingRef.Drop(this);
}

JSObject*
AudioDestinationNode::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return AudioDestinationNodeBinding::Wrap(aCx, this, aGivenProto);
}

void
AudioDestinationNode::StartRendering(Promise* aPromise)
{
  mOfflineRenderingPromise = aPromise;
  mOfflineRenderingRef.Take(this);
  mStream->Graph()->StartNonRealtimeProcessing(mFramesToProduce);
}

void
AudioDestinationNode::SetCanPlay(float aVolume, bool aMuted)
{
  if (!mStream) {
    return;
  }

  CTX_LOG(Context(), "SetCanPlay(), volume %f, muted %d", aVolume, aMuted);
  mStream->SetTrackEnabled(AudioNodeStream::AUDIO_TRACK, !aMuted);
  mStream->SetAudioOutputVolume(&gWebAudioOutputKey, aVolume);
}

void
AudioDestinationNode::AudioChannelStateChanged(bool aPlaying)
{
  // When AudioChannelAgent::NotifyStoppedPlaying() is called, need to
  // disable audio track so MSG can switch to SystemClockDriver and
  // release cubeb stream. Besides this case, only need to call
  // SetCanPlay() in WindowVolumeChanged().
  if (!aPlaying) {
    SetCanPlay(1.0, true);
  }
  Context()->AudioChannelStateChanged(aPlaying);
}

NS_IMETHODIMP
AudioDestinationNode::WindowVolumeChanged(float aVolume, bool aMuted)
{
  if (aMuted != mAudioChannelMuted) {
    mAudioChannelMuted = aMuted;

    if (UseAudioChannelAPI()) {
      Context()->DispatchTrustedEvent(
        !aMuted ? NS_LITERAL_STRING("mozinterruptend")
                : NS_LITERAL_STRING("mozinterruptbegin"));
    }
  }

  SetCanPlay(aVolume, aMuted);
  Context()->AudioChannelMuted(aMuted);
  return NS_OK;
}

NS_IMETHODIMP
AudioDestinationNode::WindowAudioCaptureChanged(bool aCapture)
{
  if (!mStream || Context()->IsOffline()) {
    return NS_OK;
  }

  nsCOMPtr<nsPIDOMWindowInner> ownerWindow = GetOwner();
  if (!ownerWindow) {
    return NS_OK;
  }

  if (aCapture != mCaptured) {
    if (aCapture) {
      nsCOMPtr<nsPIDOMWindowInner> window = Context()->GetParentObject();
      uint64_t id = window->WindowID();
      mCaptureStreamPort =
        mStream->Graph()->ConnectToCaptureStream(id, mStream);
    } else {
      mCaptureStreamPort->Destroy();
    }
    mCaptured = aCapture;
  }

  return NS_OK;
}

AudioChannel
AudioDestinationNode::MozAudioChannelType() const
{
  return mAudioChannel;
}

void
AudioDestinationNode::SetMozAudioChannelType(AudioChannel aValue, ErrorResult& aRv)
{
  if (Context()->IsOffline()) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }

  if (aValue != mAudioChannel &&
      CheckAudioChannelPermissions(aValue)) {
    mAudioChannel = aValue;

    if (mStream) {
      mStream->SetAudioChannelType(mAudioChannel);
    }

    if (mAudioChannelHelper) {
      DestroyAudioChannelAgent();
      EnsureAudioChannelAgent();
    }
  }
}

bool
AudioDestinationNode::CheckAudioChannelPermissions(AudioChannel aValue)
{
  // Only normal channel doesn't need permission.
  if (aValue == AudioChannel::Normal) {
    return true;
  }

  // Maybe this audio channel is equal to the default one.
  if (aValue == AudioChannelService::GetDefaultAudioChannel()) {
    return true;
  }

  nsCOMPtr<nsIPermissionManager> permissionManager =
    services::GetPermissionManager();
  if (!permissionManager) {
    return false;
  }

  nsCOMPtr<nsIScriptObjectPrincipal> sop = do_QueryInterface(GetOwner());
  NS_ASSERTION(sop, "Window didn't QI to nsIScriptObjectPrincipal!");
  nsCOMPtr<nsIPrincipal> principal = sop->GetPrincipal();

  uint32_t perm = nsIPermissionManager::UNKNOWN_ACTION;

  nsCString channel;
  channel.AssignASCII(AudioChannelValues::strings[uint32_t(aValue)].value,
                      AudioChannelValues::strings[uint32_t(aValue)].length);
  permissionManager->TestExactPermissionFromPrincipal(principal,
    nsCString(NS_LITERAL_CSTRING("audio-channel-") + channel).get(),
    &perm);

  return perm == nsIPermissionManager::ALLOW_ACTION;
}

void
AudioDestinationNode::NotifyStableState()
{
  mExtraCurrentTimeUpdatedSinceLastStableState = false;
}

void
AudioDestinationNode::ScheduleStableStateNotification()
{
  nsCOMPtr<nsIRunnable> event =
    NS_NewRunnableMethod(this, &AudioDestinationNode::NotifyStableState);
  // Dispatch will fail if this is called on AudioNode destruction during
  // shutdown, in which case failure can be ignored.
  nsContentUtils::RunInStableState(event.forget());
}

StreamTime
AudioDestinationNode::ExtraCurrentTime()
{
  if (!mStartedBlockingDueToBeingOnlyNode.IsNull() &&
      !mExtraCurrentTimeUpdatedSinceLastStableState) {
    mExtraCurrentTimeUpdatedSinceLastStableState = true;
    // Round to nearest processing block.
    double seconds =
      (TimeStamp::Now() - mStartedBlockingDueToBeingOnlyNode).ToSeconds();
    mExtraCurrentTimeSinceLastStartedBlocking = WEBAUDIO_BLOCK_SIZE *
      StreamTime(seconds * Context()->SampleRate() / WEBAUDIO_BLOCK_SIZE + 0.5);
    ScheduleStableStateNotification();
  }
  return mExtraCurrentTimeSinceLastStartedBlocking;
}

void
AudioDestinationNode::SetIsOnlyNodeForContext(bool aIsOnlyNode)
{
  if (!mStartedBlockingDueToBeingOnlyNode.IsNull() == aIsOnlyNode) {
    // Nothing changed.
    return;
  }

  if (!mStream) {
    // DestroyMediaStream has been called, presumably during CC Unlink().
    return;
  }

  if (mIsOffline) {
    // Don't block the destination stream for offline AudioContexts, since
    // we expect the zero data produced when there are no other nodes to
    // show up in its result buffer. Also, we would get confused by adding
    // ExtraCurrentTime before StartRendering has even been called.
    return;
  }

  if (aIsOnlyNode) {
    mStream->Suspend();
    mStartedBlockingDueToBeingOnlyNode = TimeStamp::Now();
    // Don't do an update of mExtraCurrentTimeSinceLastStartedBlocking until the next stable state.
    mExtraCurrentTimeUpdatedSinceLastStableState = true;
    ScheduleStableStateNotification();
  } else {
    // Force update of mExtraCurrentTimeSinceLastStartedBlocking if necessary
    ExtraCurrentTime();
    mStream->AdvanceAndResume(mExtraCurrentTimeSinceLastStartedBlocking);
    mExtraCurrentTimeSinceLastStartedBlocking = 0;
    mStartedBlockingDueToBeingOnlyNode = TimeStamp();
  }
}

void
AudioDestinationNode::InputMuted(bool aMuted)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(Context() && !Context()->IsOffline());

  EnsureAudioChannelAgent();
  if (mAudioChannelHelper) {
    mAudioChannelHelper->SetInputPlaying(!aMuted);
  }
}

void
AudioDestinationNode::ForceAudioChannelPlaying(uint32_t aDurationMs)
{
  MOZ_ASSERT(NS_IsMainThread());

  EnsureAudioChannelAgent();
  if (mAudioChannelHelper) {
    mAudioChannelHelper->ForcePlaying(aDurationMs);
  }
}

bool
AudioDestinationNode::IsAudioChannelPlaying()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (mAudioChannelHelper) {
    return mAudioChannelHelper->IsPlaying();
  }
  return false;
}

} // namespace dom
} // namespace mozilla
