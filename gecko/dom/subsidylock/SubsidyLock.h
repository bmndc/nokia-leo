/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#ifndef mozilla_dom_SubsidyLock_h
#define mozilla_dom_SubsidyLock_h

#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/dom/SubsidyLockBinding.h"
#include "nsCycleCollectionParticipant.h"
#include "nsISubsidyLockService.h"
#include "nsWeakPtr.h"

namespace mozilla {
namespace dom {

class DOMRequest;

class SubsidyLock final : public nsISupports
                        , public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(SubsidyLock)

  SubsidyLock(nsPIDOMWindowInner* aWindow, uint32_t aClientId);

  void
  Shutdown();

  nsPIDOMWindowInner*
  GetParentObject() const;

  // WrapperCache

  virtual JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  // WebIDL interfaces

  already_AddRefed<DOMRequest>
  GetSubsidyLockStatus(ErrorResult& aRv);

  already_AddRefed<DOMRequest>
  UnlockSubsidyLock(const UnlockOptions& aOptions, ErrorResult& aRv);

private:
  ~SubsidyLock();
  uint32_t mClientId;
  nsCOMPtr<nsPIDOMWindowInner> mWindow;
  nsCOMPtr<nsISubsidyLock> mSubsidyLock;

  bool
  CheckPermission(const char* aType) const;
};

} // namespace dom
} // namespace mozillaalready_AddRefed<DOMRequest>

#endif // mozilla_dom_SubsidyLock_h