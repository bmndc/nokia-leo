/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "DOMFaceAnalysisResults.h"
#include "Navigator.h"

using namespace mozilla;
using namespace mozilla::dom;

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(DOMFaceAnalysisResults, mParent, mPayload, mBestImage)

NS_IMPL_CYCLE_COLLECTING_ADDREF(DOMFaceAnalysisResults)
NS_IMPL_CYCLE_COLLECTING_RELEASE(DOMFaceAnalysisResults)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DOMFaceAnalysisResults)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

/* static */
bool
DOMFaceAnalysisResults::HasSupport(JSContext* aCx, JSObject* aGlobal)
{
  return Navigator::HasCameraSupport(aCx, aGlobal);
}

already_AddRefed<DOMFaceAnalysisResults>
DOMFaceAnalysisResults::Create(nsISupports* aParent)
{
  MOZ_ASSERT(aParent);

  RefPtr<DOMFaceAnalysisResults> result =
    new DOMFaceAnalysisResults(aParent);

  return result.forget();
}

JSObject*
DOMFaceAnalysisResults::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return FaceAnalysisResultsBinding::Wrap(aCx, this, aGivenProto);
}

DOMFaceAnalysisResults::DOMFaceAnalysisResults(nsISupports* aParent)
: mParent(aParent)
{
}

void
DOMFaceAnalysisResults::SetValue(const AnalysisResults& aAnalysisResult)
{
  mAnalysisResult = (uint32_t) aAnalysisResult.GetFaceAnalysisResult();
  mAnalysisInfo = (uint32_t) aAnalysisResult.GetFaceAnalysisInfo();
  mMatchScore = aAnalysisResult.GetMatchScore();

  std::vector<uint8_t> payload = aAnalysisResult.GetPayload();
  if (payload.size() > 0) {
    nsCOMPtr<nsIDOMBlob> payloadBlob =
      Blob::CreateMemoryBlob(mParent.get(),
                            static_cast<void*>(&*payload.begin()),
                            static_cast<uint64_t>(payload.size()),
                            NS_LITERAL_STRING(""));
    mPayload = static_cast<Blob*>(payloadBlob.get());
  } else {
    mPayload = nullptr;
  }

  std::shared_ptr<ImageContent> bestImage = aAnalysisResult.GetBestImage();
  if (bestImage) {
    mBestImage = new DOMFaceAnalysisImage(mParent.get(), bestImage->GetWidth(), bestImage->GetHeight(), (int32_t)bestImage->GetType(),
                                          bestImage->GetSize(), bestImage->GetData());
  } else {
    mBestImage = nullptr;
  }
}
