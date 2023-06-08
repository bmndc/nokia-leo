#include "nsThreadUtils.h"
#include "nsXPCOMCIDInternal.h"

#include "SpeechRecognitionAdaptorService.h"

#include "SpeechRecognition.h"
#include "SpeechRecognitionAlternative.h"
#include "SpeechRecognitionResult.h"
#include "SpeechRecognitionResultList.h"
#include "nsIObserverService.h"
#include "mozilla/Services.h"
#include "nsServiceManagerUtils.h"


using namespace voiceEngineAdaptor;

namespace mozilla {

using namespace dom;
static const uint16_t skBytesPerSample = 2;
static const uint16_t skFrequency = 16000;
static const uint16_t skChannels = 1;

// using a static object first, if we have memory problem. We may need to
// initialize it on the fly.
voiceEngineApi sApi;

class DecodeResultTask : public nsRunnable
{
public:
  DecodeResultTask(const nsString& hypstring,
                   SpeechRecognitionAdaptorService* aService,
                   nsIThread* thread,
                   int aError)
      : mResult(hypstring),
        mService(aService),
        mWorkerThread(thread),
        mError(aError)
  {
    MOZ_ASSERT(
      !NS_IsMainThread()); // This should be running on the worker thread
  }

  NS_IMETHOD
  Run()
  {
    MOZ_ASSERT(NS_IsMainThread()); // This method is supposed to run on the main
                                   // thread!

    WeakPtr<dom::SpeechRecognition> recognition = mService->getRecognition();

    if (mError < 0) {
      recognition->DispatchError(SpeechRecognition::EVENT_RECOGNITIONSERVICE_ERROR,
          SpeechRecognitionErrorCode::Cannot_recognize,
          NS_LITERAL_STRING("cannot recognize the audio"));
    } else {
      // Declare javascript result events
      RefPtr<SpeechEvent> event = new SpeechEvent(
        recognition, SpeechRecognition::EVENT_RECOGNITIONSERVICE_FINAL_RESULT);
      SpeechRecognitionResultList* resultList =
        new SpeechRecognitionResultList(recognition);
      SpeechRecognitionResult* result = new SpeechRecognitionResult(recognition);
      SpeechRecognitionAlternative* alternative =
        new SpeechRecognitionAlternative(recognition);

      alternative->mTranscript = mResult;
      alternative->mConfidence = 100;

      result->mItems.AppendElement(alternative);
      resultList->mItems.AppendElement(result);

      event->mRecognitionResultList = resultList;
      NS_DispatchToMainThread(event);
    }

    // reset the service for making sure the status of recogntion engine in the
    // next round is clean.
    mService->reset();
    // If we don't destroy the thread when we're done with it, it will hang
    // around forever... bad!
    // But thread->Shutdown must be called from the main thread, not from the
    // thread itself.
    return mWorkerThread->Shutdown();
  }

private:
  nsString mResult;
  //SpeechRecognitionAdaptorService should live longer than the task
  SpeechRecognitionAdaptorService* mService;
  nsCOMPtr<nsIThread> mWorkerThread;
  // if mError < 0, error happens.
  // TODO: define the error codes
  int mError;
};

class DecodeTask : public nsRunnable
{
public:
  DecodeTask(SpeechRecognitionAdaptorService* aService,
             const nsTArray<int16_t>& audiovector, engineAdaptor aAdaptor)
      : mService(aService), mAudiovector(audiovector), mAdaptor(aAdaptor)
  {
    mWorkerThread = do_GetCurrentThread();
  }

  NS_IMETHOD
  Run()
  {
    const uint32_t bufSize = 2048;
    char output[bufSize] = "";
    if (sApi.mReady) {
      recogResult ret = sApi.processAudio(mAdaptor, (char* )&mAudiovector[0],
                                     mAudiovector.Length(), output, bufSize);
      nsCOMPtr<nsIRunnable> resultrunnable;
      if (ret == eRecogSuccess) {
        resultrunnable = new DecodeResultTask(NS_ConvertUTF8toUTF16(output),
                               mService, mWorkerThread, 0);
      } else {
          resultrunnable = new DecodeResultTask(NS_ConvertUTF8toUTF16(output),
                                 mService, mWorkerThread, -1);
      }

      NS_DispatchToMainThread(resultrunnable);
    } else {
      MOZ_ASSERT("sApi should be ready here!!!!");
    }

    mAudiovector.Clear();
    return NS_OK;
  }

private:
  //SpeechRecognitionAdaptorService should live longer than the task
  SpeechRecognitionAdaptorService* mService;
  nsTArray<int16_t> mAudiovector;
  engineAdaptor mAdaptor;
  nsCOMPtr<nsIThread> mWorkerThread;
};

NS_IMPL_ISUPPORTS(SpeechRecognitionAdaptorService, nsISpeechRecognitionService, nsIObserver)

SpeechRecognitionAdaptorService::SpeechRecognitionAdaptorService() :
  mSpeexState(nullptr),
  mStarted(false)
{
  if (!sApi.mReady) {
    sApi.Init();
  }

  if (sApi.mReady) {
  }
}

SpeechRecognitionAdaptorService::~SpeechRecognitionAdaptorService()
{
  reset();
}

void
SpeechRecognitionAdaptorService::reset()
{
  if (!mStarted) {
    return;
  }

  mSpeexState = nullptr;
  if (mAdaptor) {
    if (sApi.mReady) {
      sApi.disableEngine(&mAdaptor);
    } else {
      MOZ_ASSERT("sApi should be ready here !!!");
    }
  }
  mStarted = false;
}

bool
SpeechRecognitionAdaptorService::internalInit()
{
  if (!sApi.mReady) {
    return false;
  }

  if (mStarted) {
    return true;
  }

  if (eRecogSuccess != sApi.enableEngine(&mAdaptor, skChannels, skFrequency,
                                         skBytesPerSample)) {
    return false;
  }

  nsAdoptingCString prefValue;
  nsCString value;

  prefValue = Preferences::GetCString("media.webspeech.adaptor.acousticModule");
  if (prefValue) {
    value.Assign(prefValue);
    sApi.addContextFile(mAdaptor, eAcousticFile, (char*)value.get());
  }

  prefValue = Preferences::GetCString("media.webspeech.adaptor.dialerContextFile");
  if (prefValue) {
    value.Assign(prefValue);
    sApi.addContextFile(mAdaptor, eContextFile, (char*)value.get());
  }

  prefValue = Preferences::GetCString("media.webspeech.adaptor.launcherContextFile");
  if (prefValue) {
    value.Assign(prefValue);
    sApi.addContextFile(mAdaptor, eContextFile, (char*)value.get());
  }

  prefValue = Preferences::GetCString("media.webspeech.adaptor.clcFile");
  if (prefValue) {
    value.Assign(prefValue);
    sApi.addContextFile(mAdaptor, eClcFile, (char*)value.get());
  }

  mStarted = true;
  return true;
}

NS_IMETHODIMP
SpeechRecognitionAdaptorService::Initialize(WeakPtr<SpeechRecognition> aSpeechRecognition)
{
  if (mAdaptor == nullptr) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  if (internalInit() == false) {
    return NS_ERROR_FAILURE;
  }

  mRecognition = aSpeechRecognition;
  mAudioVector.Clear();
  return NS_OK;
}

NS_IMETHODIMP
SpeechRecognitionAdaptorService::ProcessAudioSegment(AudioSegment* aAudioSegment, int32_t aSampleRate)
{
  if (!mSpeexState) {
    mSpeexState = speex_resampler_init(skChannels, aSampleRate, skFrequency,
                                       SPEEX_RESAMPLER_QUALITY_MAX, nullptr);
  }
  aAudioSegment->ResampleChunks(mSpeexState, aSampleRate, skFrequency);

  AudioSegment::ChunkIterator iterator(*aAudioSegment);

  while (!iterator.IsEnded()) {
    mozilla::AudioChunk& chunk = *(iterator);
    MOZ_ASSERT(chunk.mBuffer);
    const int16_t* buf = static_cast<const int16_t*>(chunk.mChannelData[0]);

    for (int i = 0; i < iterator->mDuration; i++) {
      mAudioVector.AppendElement((int16_t)buf[i]);
    }
    iterator.Next();
  }
  return NS_OK;
}

NS_IMETHODIMP
SpeechRecognitionAdaptorService::SoundEnd()
{
  speex_resampler_destroy(mSpeexState);
  mSpeexState = nullptr;

  // To create a new thread, get the thread manager
  nsCOMPtr<nsIThreadManager> tm = do_GetService(NS_THREADMANAGER_CONTRACTID);
  nsCOMPtr<nsIThread> decodethread;
  nsresult rv = tm->NewThread(0, 0, getter_AddRefs(decodethread));
  if (NS_FAILED(rv)) {
    // In case of failure, call back immediately with an empty string which
    // indicates failure
    return NS_OK;
  }

  nsCOMPtr<nsIRunnable> r =
    new DecodeTask(this, mAudioVector, mAdaptor);
  decodethread->Dispatch(r, nsIEventTarget::DISPATCH_NORMAL);
  return NS_OK;
}

NS_IMETHODIMP
SpeechRecognitionAdaptorService::ValidateAndSetGrammarList(
  mozilla::dom::SpeechGrammar* aSpeechGrammar,
  nsISpeechGrammarCompilationCallback*)
{
  if (!internalInit()) {
    return NS_ERROR_FAILURE;
  }

  if (aSpeechGrammar) {
    nsAutoString grammar;
    ErrorResult rv;
    aSpeechGrammar->GetSrc(grammar, rv);

    if (eRecogSuccess != sApi.setGrammar(mAdaptor,
        (char*)NS_ConvertUTF16toUTF8(grammar).get(), grammar.Length())) {
      return NS_ERROR_FAILURE;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
SpeechRecognitionAdaptorService::Abort()
{
  return NS_OK;
}

NS_IMETHODIMP
SpeechRecognitionAdaptorService::Observe(nsISupports* aSubject, const char* aTopic, const char16_t* aData)
{
  return NS_OK;
}
} // namespace mozilla
