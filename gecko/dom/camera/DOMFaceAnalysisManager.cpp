/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "DOMFaceAnalysisManager.h"
#include "Navigator.h"
#include "DOMFaceAnalysisResults.h"
#include "GrallocImages.h"
#include "mozilla/dom/fr/FrService.h"
#include "mozilla/dom/FaceAnalysisResultsEventBinding.h"
#include "mozilla/dom/FaceAnalysisResultsEvent.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "DOMFaceAnalysisImage.h"

using namespace mozilla;
using namespace mozilla::dom;

NS_IMPL_CYCLE_COLLECTION_INHERITED(DOMFaceAnalysisManager, DOMEventTargetHelper, mParent)

NS_IMPL_ADDREF_INHERITED(DOMFaceAnalysisManager, DOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(DOMFaceAnalysisManager, DOMEventTargetHelper)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(DOMFaceAnalysisManager)
NS_INTERFACE_MAP_END_INHERITING(DOMEventTargetHelper)


class DOMFaceAnalysisManager::DOMCallback : public nsRunnable
{
public:
  explicit DOMCallback(nsMainThreadPtrHandle<nsISupports> aDOMFaceAnalysisManager)
    : mDOMFaceAnalysisManager(aDOMFaceAnalysisManager)
  {
    MOZ_COUNT_CTOR(DOMFaceAnalysisManager::DOMCallback);
  }

protected:
  virtual ~DOMCallback()
  {
    MOZ_COUNT_DTOR(DOMFaceAnalysisManager::DOMCallback);
  }

public:
  virtual void RunCallback(DOMFaceAnalysisManager* aDOMFaceAnalysisManager) = 0;

  NS_IMETHOD
  Run() override
  {
    MOZ_ASSERT(NS_IsMainThread());
    RefPtr<DOMFaceAnalysisManager> faceAnalysisManager = static_cast<DOMFaceAnalysisManager*>(mDOMFaceAnalysisManager.get());
    if (!faceAnalysisManager) {
      return NS_ERROR_INVALID_ARG;
    }
    RunCallback(faceAnalysisManager);
    return NS_OK;
  }

protected:
  nsMainThreadPtrHandle<nsISupports> mDOMFaceAnalysisManager;
};

/* static */
bool
DOMFaceAnalysisManager::HasSupport(JSContext* aCx, JSObject* aGlobal)
{
  return Navigator::HasCameraSupport(aCx, aGlobal);
}

JSObject*
DOMFaceAnalysisManager::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return FaceAnalysisManagerBinding::Wrap(aCx, this, aGivenProto);
}

DOMFaceAnalysisManager::DOMFaceAnalysisManager(nsISupports* aParent)
: mParent(aParent)
, mMainDOMFaceAnalysisManager(new nsMainThreadPtrHolder<nsISupports>(static_cast<DOMFaceAnalysisManager*>(this)))
{
  AnalysisResults result;
  SendEvent(&result);

  pthread_mutex_init(&mMutex, nullptr);
  pthread_cond_init(&mCond, nullptr);
}

DOMFaceAnalysisManager::~DOMFaceAnalysisManager()
{
  pthread_cond_signal(&mCond);
  pthread_join(mThread, NULL);
}

already_AddRefed<dom::Promise>
DOMFaceAnalysisManager::StartFaceAnalysis(int32_t aType, const Optional<Sequence<OwningNonNull<DOMFaceAnalysisImage> > >& aRefImages, ErrorResult& aRv)
{
  nsCOMPtr<nsIGlobalObject> globalObject = do_QueryInterface(GetParentObject());
  if (!globalObject) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  RefPtr<dom::Promise> promise = Promise::Create(globalObject, aRv);
  if (aRv.Failed()) {
    return nullptr;
  }

  if (!mFrService) {
    return nullptr;
  }

  std::vector<ImageContent> refContentImages;

  if (aRefImages.WasPassed()) {
    for (const auto& image : aRefImages.Value()) {
      RefPtr<BlobImpl> imageImpl = image->GetImageData()->Impl();
      nsCOMPtr<nsIInputStream> stream;
      imageImpl->GetInternalStream(getter_AddRefs(stream), aRv);
      if (NS_WARN_IF(aRv.Failed())) {
        continue;
      }

      uint64_t blobSize = image->GetImageData()->GetSize(aRv);
      std::shared_ptr<uint8_t> bufferData = std::shared_ptr<uint8_t>(new uint8_t[blobSize]);
      uint32_t numRead = 0;
      uint64_t readSize = 0;
      while (readSize < blobSize) {
        aRv = stream->Read((char*)bufferData.get() + numRead, blobSize, &numRead);
        if (NS_WARN_IF(aRv.Failed())) {
          continue;
        }
        readSize += numRead;
      }

      refContentImages.push_back(ImageContent(image->Width(), image->Height(), (ImageType)image->Type(), blobSize, bufferData));
    }
  }
  FrStatus ret = mFrService->StartFaceAnalysis((FaceThreshold)aType, &refContentImages);
  if (ret == FrStatus::STATUS_OK) {
    pthread_create(&mThread, nullptr, DOMFaceAnalysisManager::PushFrameThread, this);
  }

  promise->MaybeResolve(true);

  return promise.forget();
}

already_AddRefed<dom::Promise>
DOMFaceAnalysisManager::StopFaceAnalysis(ErrorResult& aRv)
{
  nsCOMPtr<nsIGlobalObject> globalObject = do_QueryInterface(GetParentObject());
  if (!globalObject) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }
  RefPtr<dom::Promise> promise = Promise::Create(globalObject, aRv);
  if (aRv.Failed()) {
    return nullptr;
  }

  if (!mFrService) {
    return nullptr;
  }

  mFrService->StopFaceAnalysis();

  promise->MaybeResolve(true);

  return promise.forget();
}

bool
DOMFaceAnalysisManager::AddFrameToQueue(mozilla::layers::Image* aImage)
{
  pthread_mutex_lock(&mMutex);

  if (mFrQueue.size() >= mMaxQueueSize) {
    pthread_mutex_unlock(&mMutex);

    return false;
  }

  mozilla::layers::GrallocImage* grallocImage = aImage->AsGrallocImage();
  android::sp<android::GraphicBuffer> graphicBuffer = grallocImage->GetGraphicBuffer();

  void* basePtr = nullptr;
  int32_t width = graphicBuffer->getWidth();
  int32_t height = graphicBuffer->getHeight();
  int32_t yStride = graphicBuffer->getStride();
  int32_t uvStride = ((yStride / 2) + 15) & ~0x0F; // Align to 16 bytes boundary
  size_t bufferSize = height * yStride + height * uvStride;
  graphicBuffer->lock(android::GraphicBuffer::USAGE_SW_READ_MASK, &basePtr);
  uint32_t format = graphicBuffer->getPixelFormat();
  std::shared_ptr<uint8_t> buffer = std::shared_ptr<uint8_t>(new uint8_t[bufferSize]);
  memcpy(buffer.get(), basePtr, bufferSize);
  graphicBuffer->unlock();

  ImageType imageType = ImageType::TYPE_RGB;
  switch (format) {
  case HAL_PIXEL_FORMAT_RGBA_8888:
    imageType = ImageType::TYPE_RGBA;
    break;
  case HAL_PIXEL_FORMAT_YCrCb_420_SP:
    imageType = ImageType::TYPE_NV21;
    break;
  case HAL_PIXEL_FORMAT_YV12:
    imageType = ImageType::TYPE_NV12;
    break;
  default:
    break;
  }

  std::shared_ptr<ImageContent> imageContent = std::shared_ptr<ImageContent>(new ImageContent(width, height, imageType,
                                                                                              bufferSize, buffer));
  mFrQueue.push_back(imageContent);

  pthread_cond_signal(&mCond);

  pthread_mutex_unlock(&mMutex);

  return true;
}

void
DOMFaceAnalysisManager::SendEvent(AnalysisResults* aResult) {
  FaceAnalysisResultsEventInit eventInit;
  eventInit.mFaceAnalysisResults = dom::DOMFaceAnalysisResults::Create(mParent);
  eventInit.mFaceAnalysisResults->SetValue(*aResult);
  RefPtr<FaceAnalysisResultsEvent> event =
  FaceAnalysisResultsEvent::Constructor(this,
                                        NS_LITERAL_STRING("faceanalysisresults"),
                                        eventInit);
  DispatchTrustedEvent(event);
}

/* static */ void*
DOMFaceAnalysisManager::PushFrameThread(void* aData)
{
  DOMFaceAnalysisManager* faceAnalysisManager = static_cast<DOMFaceAnalysisManager*>(aData);
  std::shared_ptr<FrService> frService = faceAnalysisManager->GetFrService();
  if (!frService) {
    return nullptr;
  }

  class Callback : public DOMCallback
  {
  public:
    Callback(nsMainThreadPtrHandle<nsISupports> aDOMFaceAnalysisManager,
            AnalysisResults& aResult)
      : DOMCallback(aDOMFaceAnalysisManager)
      , mResult(aResult)
    { }

    void
    RunCallback(DOMFaceAnalysisManager* aDOMFaceAnalysisManager) override
    {
      aDOMFaceAnalysisManager->SendEvent(&mResult);
    }

  protected:
    AnalysisResults mResult;
  };

  while (frService->IsAnalyzing()) {
    pthread_mutex_lock(&faceAnalysisManager->mMutex);

    if (faceAnalysisManager->mFrQueue.size() == 0) {
      pthread_cond_wait(&faceAnalysisManager->mCond, &faceAnalysisManager->mMutex);
    }
    if (!frService->IsAnalyzing()) {
      break;
    }

    std::shared_ptr<ImageContent> imageContent;
    if (faceAnalysisManager->mFrQueue.size() > 0) {
      imageContent = faceAnalysisManager->mFrQueue.front();
      faceAnalysisManager->mFrQueue.pop_front();
    }

    pthread_mutex_unlock(&faceAnalysisManager->mMutex);

    if (imageContent) {
      AnalysisResults result;
      frService->PushFrame(*imageContent.get(), result);
      NS_DispatchToMainThread(new Callback(faceAnalysisManager->GetMainThreadObject(), result));
    }

    usleep(10000);
  }

  return nullptr;
}
