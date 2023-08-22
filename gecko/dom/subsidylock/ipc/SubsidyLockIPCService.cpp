/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#include "SubsidyLockIPCService.h"
#include "mozilla/dom/ContentChild.h"
#include "mozilla/Preferences.h"

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::dom::subsidylock;

NS_IMPL_ISUPPORTS(SubsidyLockIPCService, nsISubsidyLockService)

SubsidyLockIPCService::SubsidyLockIPCService()
{
  int32_t numRil = Preferences::GetInt("ril.numRadioInterfaces", 1);
  mItems.SetLength(numRil);
}

SubsidyLockIPCService::~SubsidyLockIPCService()
{
  uint32_t count = mItems.Length();
  for (uint32_t i = 0; i < count; i++) {
    if (mItems[i]) {
      mItems[i]->Shutdown();
    }
  }
}

// nsISubsidyLockService

NS_IMETHODIMP
SubsidyLockIPCService::GetNumItems(uint32_t* aNumItems)
{
  *aNumItems = mItems.Length();
  return NS_OK;
}

NS_IMETHODIMP
SubsidyLockIPCService::GetItemByServiceId(uint32_t aServiceId,
                                          nsISubsidyLock** aItem)
{
  NS_ENSURE_TRUE(aServiceId < mItems.Length(), NS_ERROR_INVALID_ARG);

  if (!mItems[aServiceId]) {
    RefPtr<SubsidyLockChild> child = new SubsidyLockChild(aServiceId);

    // |SendPMobileConnectionConstructor| adds another reference to the child
    // actor and removes in |DeallocPMobileConnectionChild|.
    ContentChild::GetSingleton()->SendPSubsidyLockConstructor(child,
                                                              aServiceId);

    mItems[aServiceId] = child;
  }

  RefPtr<nsISubsidyLock> item(mItems[aServiceId]);
  item.forget(aItem);
  return NS_OK;

}