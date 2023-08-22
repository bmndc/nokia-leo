/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "DOMFaceAnalysisImage.h"
#include "Navigator.h"

using namespace mozilla;
using namespace mozilla::dom;

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(DOMFaceAnalysisImage, mParent, mImageData)

NS_IMPL_CYCLE_COLLECTING_ADDREF(DOMFaceAnalysisImage)
NS_IMPL_CYCLE_COLLECTING_RELEASE(DOMFaceAnalysisImage)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DOMFaceAnalysisImage)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

/* static */
bool
DOMFaceAnalysisImage::HasSupport(JSContext* aCx, JSObject* aGlobal)
{
  return Navigator::HasCameraSupport(aCx, aGlobal);
}

already_AddRefed<DOMFaceAnalysisImage>
DOMFaceAnalysisImage::Create(nsISupports* aParent)
{
  MOZ_ASSERT(aParent);

  RefPtr<DOMFaceAnalysisImage> result =
    new DOMFaceAnalysisImage(aParent);

  return result.forget();
}

JSObject*
DOMFaceAnalysisImage::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return FaceAnalysisImageBinding::Wrap(aCx, this, aGivenProto);
}

DOMFaceAnalysisImage::DOMFaceAnalysisImage(nsISupports* aParent)
: mParent(aParent)
{}

DOMFaceAnalysisImage::DOMFaceAnalysisImage(nsISupports* aParent, uint32_t aWidth, uint32_t aHeight,
                                           int32_t aType, uint32_t aSize,
                                           std::shared_ptr<uint8_t> aData)
: mParent(aParent)
, mWidth(aWidth)
, mHeight(aHeight)
, mType(aType)
{
  if (aData && aSize > 0) {
    const nsAString& type = NS_LITERAL_STRING("image/nv21");
    nsCOMPtr<nsIDOMBlob> imageBlob =
      Blob::CreateMemoryBlob(mParent.get(),
                             static_cast<void*>(aData.get()),
                             static_cast<uint64_t>(aSize),
                             NS_LITERAL_STRING(""));
    mImageData = static_cast<Blob*>(imageBlob.get());
  }
}

