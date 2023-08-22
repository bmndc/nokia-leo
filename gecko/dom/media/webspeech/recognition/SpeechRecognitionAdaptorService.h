

#ifndef kaios_dom_SpeechRecognitionAdaptorService_h
#define kaios_dom_SpeechRecognitionAdaptorService_h

#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsIObserver.h"
#include "nsISpeechRecognitionService.h"
#include "speex/speex_resampler.h"

#include "voiceEngineAdaptor.h"

#include <dlfcn.h>

#define NS_SPEECH_RECOGNITION_ADAPTOR_SERVICE_CID \
  {0xfdeb5ec9, 0x6e58, 0x42c9, {0xbf, 0x2d, 0x16, 0xa1, 0x08, 0x84, 0x04, 0x6a}};

namespace voiceEngineAdaptor {
class voiceEngineApi {
public:
  voiceEngineApi() : mHandle(nullptr),
                     mReady(false) {
  }

  ~voiceEngineApi() {
    if (mHandle) {
      dlclose(mHandle);
      mHandle = nullptr;
    }
  }

  bool Init() {
    mHandle = dlopen("libvoiceEngineAdaptor.so", RTLD_LAZY);
    if (!mHandle) {
      return false;
    }

    enableEngine = (recogResult (*)(engineAdaptor*, uint16_t, uint16_t, uint16_t))dlsym(mHandle, "enableEngine");
    if (!enableEngine) {
      return false;
    }

    processAudio = (recogResult (*)(engineAdaptor, char*, uint32_t, char*, int32_t))dlsym(mHandle, "processAudio");
    if (!processAudio) {
      return false;
    }

    disableEngine = (recogResult (*)(engineAdaptor*))dlsym(mHandle, "disableEngine");
    if (!disableEngine) {
      return false;
    }

    setGrammar = (recogResult(*)(engineAdaptor, char*, uint32_t))dlsym(mHandle, "setGrammar");
    if (!setGrammar) {
      return false;
    }

    addContextFile = (recogResult (*)(engineAdaptor, fileType, char*))dlsym(mHandle, "addContextFile");
    if (!addContextFile) {
      return false;
    }

    mReady = true;
    return true;
  }

  recogResult (* enableEngine) (engineAdaptor* aAdaptor, uint16_t aChannels,
                         uint16_t aFrequency, uint16_t aBytePerSample);
  recogResult (* processAudio) (engineAdaptor aAdaptor, char* aData, uint32_t aSamples,
                         char* aOutput, int32_t aOutputSize);
  recogResult (* disableEngine) (engineAdaptor* aAdaptor);
  recogResult (* setGrammar) (engineAdaptor aAdaptor, char* aGrammar, uint32_t aGrammarSize);
  recogResult (* addContextFile) (engineAdaptor, fileType, char*);

  void* mHandle;
  bool mReady;
};
} //voiceEngineAdaptor

namespace mozilla {
class SpeechRecognitionAdaptorService : public nsISpeechRecognitionService,
                                       public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISPEECHRECOGNITIONSERVICE
  NS_DECL_NSIOBSERVER

  SpeechRecognitionAdaptorService();

  // recognition Services
  voiceEngineAdaptor::engineAdaptor mAdaptor;
  void reset();
  WeakPtr<dom::SpeechRecognition> getRecognition() {
    return mRecognition;
  }
private:
  virtual ~SpeechRecognitionAdaptorService();
  bool setParamters();
  bool internalInit();

  /** Speex state */
  SpeexResamplerState* mSpeexState;

  /** Audio data */
  nsTArray<int16_t> mAudioVector;

  /** The associated SpeechRecognition */
  WeakPtr<dom::SpeechRecognition> mRecognition;
  bool mStarted;
};

} // namespace mozilla

#endif
