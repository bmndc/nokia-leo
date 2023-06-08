/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DOM_CAMERA_DOMFACEANALYSISRESULTS_H
#define DOM_CAMERA_DOMFACEANALYSISRESULTS_H

#include "mozilla/dom/FaceAnalysisResultsEventBinding.h"
#include "mozilla/dom/CameraControlBinding.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"
#include "mozilla/dom/DOMRect.h"
#include "mozilla/dom/DOMPoint.h"
#include "ICameraControl.h"
#include "mozilla/dom/fr/FrService.h"

namespace mozilla {

namespace dom {

class DOMFaceAnalysisImage;

class DOMFaceAnalysisResults final : public nsISupports
                                   , public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(DOMFaceAnalysisResults)

  // Because this header's filename doesn't match its C++ or DOM-facing
  // classname, we can't rely on the [Func="..."] WebIDL tag to implicitly
  // include the right header for us; instead we must explicitly include a
  // HasSupport() method in each header. We can get rid of these with the
  // Great Renaming proposed in bug 983177.
  static bool HasSupport(JSContext* aCx, JSObject* aGlobal);

  static already_AddRefed<DOMFaceAnalysisResults> Create(nsISupports* aParent);

  nsISupports* GetParentObject() const        { return mParent; }

  DOMFaceAnalysisResults(nsISupports* aParent);

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  uint32_t AnalysisResult()             { return mAnalysisResult; }
  uint32_t AnalysisInfo()               { return mAnalysisInfo; }
  dom::Blob* GetPayload()               { return mPayload; }
  uint32_t MatchScore()                 { return mMatchScore; }
  DOMFaceAnalysisImage* GetBestImage()  { return mBestImage; }

  void SetValue(const AnalysisResults& aAnalysisResult);

protected:
  virtual ~DOMFaceAnalysisResults() { }

  nsCOMPtr<nsISupports> mParent;
  uint32_t mAnalysisResult = 0;
  uint32_t mAnalysisInfo = 0;
  RefPtr<dom::Blob> mPayload;
  int32_t mMatchScore = 0;
  RefPtr<DOMFaceAnalysisImage> mBestImage;
};

} // namespace dom

} // namespace mozilla

#endif // DOM_CAMERA_DOMFACEANALYSISRESULTS_H
