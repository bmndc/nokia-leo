/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DOM_CAMERA_DOMFACEANALYSISIMAGE_H
#define DOM_CAMERA_DOMFACEANALYSISIMAGE_H

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

class DOMFaceAnalysisImage final : public nsISupports
                                 , public nsWrapperCache
{
public:
  //NS_DECL_ISUPPORTS
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(DOMFaceAnalysisImage)

  // Because this header's filename doesn't match its C++ or DOM-facing
  // classname, we can't rely on the [Func="..."] WebIDL tag to implicitly
  // include the right header for us; instead we must explicitly include a
  // HasSupport() method in each header. We can get rid of these with the
  // Great Renaming proposed in bug 983177.
  static bool HasSupport(JSContext* aCx, JSObject* aGlobal);

  static already_AddRefed<DOMFaceAnalysisImage> Create(nsISupports* aParent);

  nsISupports* GetParentObject() const        { return mParent; }

  DOMFaceAnalysisImage(nsISupports* aParent, uint32_t aWidth, uint32_t aHeight, int32_t aType, uint32_t aSize,
                        std::shared_ptr<uint8_t> aData);

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  uint32_t Width()                { return mWidth; }
  uint32_t Height()               { return mHeight; }
  uint32_t Type()                 { return mType; }
  dom::Blob* GetImageData()       { return mImageData; }

protected:
  DOMFaceAnalysisImage(nsISupports* aParent);
  virtual ~DOMFaceAnalysisImage() {}

  nsCOMPtr<nsISupports> mParent;
  uint32_t mWidth = 0;
  uint32_t mHeight = 0;
  int32_t mType = 0;
  RefPtr<dom::Blob> mImageData;
};

} // namespace dom

} // namespace mozilla

#endif // DOM_CAMERA_DOMFACEANALYSISIMAGE_H
