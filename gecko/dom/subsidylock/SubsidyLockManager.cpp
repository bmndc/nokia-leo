/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#include "mozilla/dom/SubsidyLockManager.h"
#include "mozilla/dom/SubsidyLockManagerBinding.h"
#include "mozilla/Preferences.h"
#include "ipc/SubsidyLockIPCService.h"
#include "nsIGonkSubsidyLockService.h"
#include "nsXULAppAPI.h" // For XRE_GetProcessType()

using namespace mozilla::dom;
using namespace mozilla::dom::subsidylock;

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(SubsidyLockManager,
                                      mWindow,
                                      mSubsidyLocks)

NS_IMPL_CYCLE_COLLECTING_ADDREF(SubsidyLockManager)
NS_IMPL_CYCLE_COLLECTING_RELEASE(SubsidyLockManager)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(SubsidyLockManager)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

SubsidyLockManager::SubsidyLockManager(nsPIDOMWindowInner* aWindow)
  : mLengthInitialized(false)
  , mWindow(aWindow)
{
}

SubsidyLockManager::~SubsidyLockManager()
{
  uint32_t len = mSubsidyLocks.Length();
  for (uint32_t i = 0; i < len; ++i) {
    if (!mSubsidyLocks[i]) {
      continue;
    }

    mSubsidyLocks[i]->Shutdown();
    mSubsidyLocks[i] = nullptr;
  }
}

nsPIDOMWindowInner*
SubsidyLockManager::GetParentObject() const
{
  MOZ_ASSERT(mWindow);
  return mWindow;
}

JSObject*
SubsidyLockManager::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return SubsidyLockManagerBinding::Wrap(aCx, this, aGivenProto);
}

SubsidyLock*
SubsidyLockManager::Item(uint32_t aIndex)
{
  bool unused;
  return IndexedGetter(aIndex, unused);
}

uint32_t
SubsidyLockManager::Length()
{
  if (!mLengthInitialized) {
    mLengthInitialized = true;

    nsCOMPtr<nsISubsidyLockService> service =
      do_GetService(NS_SUBSIDY_LOCK_SERVICE_CONTRACTID);
    NS_ENSURE_TRUE(service, 0);

    uint32_t length = 0;
    nsresult rv = service->GetNumItems(&length);
    NS_ENSURE_SUCCESS(rv, 0);

    mSubsidyLocks.SetLength(length);
  }

  return mSubsidyLocks.Length();
}

SubsidyLock*
SubsidyLockManager::IndexedGetter(uint32_t aIndex, bool& aFound)
{
  aFound = aIndex < Length();
  if (!aFound) {
    return nullptr;
  }

  if (!mSubsidyLocks[aIndex]) {
    mSubsidyLocks[aIndex] = new SubsidyLock(mWindow, aIndex);
  }

  return mSubsidyLocks[aIndex];
}

already_AddRefed<nsISubsidyLockService>
NS_CreateSubsidyLockService()
{
  nsCOMPtr<nsISubsidyLockService> service;

  if (XRE_IsContentProcess()) {
    service = new SubsidyLockIPCService();
  } else {
#if defined(MOZ_WIDGET_GONK) && defined(MOZ_B2G_RIL)
    service = do_GetService(GONK_SUBSIDY_LOCK_SERVICE_CONTRACTID);
#endif
  }

  return service.forget();
}