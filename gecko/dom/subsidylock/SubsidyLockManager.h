/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#ifndef mozilla_dom_subsidylock_SubsidyLockManager_h__
#define mozilla_dom_subsidylock_SubsidyLockManager_h__

#include "mozilla/dom/SubsidyLock.h"
#include "nsWrapperCache.h"

namespace mozilla {
namespace dom {

class SubsidyLockManager final : public nsISupports
                               , public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(SubsidyLockManager)

  explicit SubsidyLockManager(nsPIDOMWindowInner* aWindow);

  nsPIDOMWindowInner*
  GetParentObject() const;

  // WrapperCache

  virtual JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  //  WebIDL

  SubsidyLock*
  Item(uint32_t aIndex);

  uint32_t
  Length();

  SubsidyLock*
  IndexedGetter(uint32_t aIndex, bool& aFound);

private:
  ~SubsidyLockManager();

  bool mLengthInitialized;

  nsCOMPtr<nsPIDOMWindowInner> mWindow;
  nsTArray<RefPtr<SubsidyLock>> mSubsidyLocks;
};

} // namespace dom
} // namespace mozilla


#endif // mozilla_dom_subsidylock_SubsidyLockManager_h__