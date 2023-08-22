/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DOM_CAMERA_DOMFACEANALYSISMANAGER_H
#define DOM_CAMERA_DOMFACEANALYSISMANAGER_H

#include "mozilla/dom/CameraControlBinding.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/dom/Promise.h"
#include <list>

class AnalysisResults;
class FrService;
class ImageContent;

namespace mozilla {

namespace layers {
  class Image;
}

namespace dom {

class DOMFaceAnalysisImage;
class FaceAnalysisResults;

class DOMFaceAnalysisManager final : public DOMEventTargetHelper
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(DOMFaceAnalysisManager,
                                           DOMEventTargetHelper)

  IMPL_EVENT_HANDLER(faceanalysisresults)

  // Because this header's filename doesn't match its C++ or DOM-facing
  // classname, we can't rely on the [Func="..."] WebIDL tag to implicitly
  // include the right header for us; instead we must explicitly include a
  // HasSupport() method in each header. We can get rid of these with the
  // Great Renaming proposed in bug 983177.

  DOMFaceAnalysisManager(nsISupports* aParent);

  static bool HasSupport(JSContext* aCx, JSObject* aGlobal);

  already_AddRefed<dom::Promise> StartFaceAnalysis(int32_t aType, const Optional<Sequence<OwningNonNull<DOMFaceAnalysisImage> > >& aRefImages, ErrorResult& aRv);

  already_AddRefed<dom::Promise> StopFaceAnalysis(ErrorResult& aRv);

  nsISupports* GetParentObject() const { return mParent; }

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  void SetFrService(std::shared_ptr<FrService>& aFrService) { mFrService = aFrService; }
  std::shared_ptr<FrService> GetFrService() { return mFrService; }

  bool AddFrameToQueue(mozilla::layers::Image* aImage);

  void SendEvent(AnalysisResults* aResult);

  nsMainThreadPtrHandle<nsISupports> GetMainThreadObject() { return mMainDOMFaceAnalysisManager; }

protected:
  virtual ~DOMFaceAnalysisManager();
  static void* PushFrameThread(void* aData);

  nsCOMPtr<nsISupports> mParent;

  std::shared_ptr<FrService> mFrService;

  pthread_t mThread;
  std::list<std::shared_ptr<ImageContent>> mFrQueue;
  pthread_mutex_t mMutex;
  pthread_cond_t mCond;
  const uint32_t mMaxQueueSize = 2;
  class DOMCallback;

  nsMainThreadPtrHandle<nsISupports> mMainDOMFaceAnalysisManager;
};

} // namespace dom

} // namespace mozilla

#endif // DOM_CAMERA_DOMFACEANALYSISMANAGER_H
