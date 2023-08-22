/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#include "mozilla/dom/subsidylock/SubsidyLockParent.h"

#include "mozilla/AppProcessChecker.h"
#include "mozilla/dom/BindingUtils.h"

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::dom::subsidylock;

SubsidyLockParent::SubsidyLockParent(uint32_t aClientId)
  : mLive(true)
{
  MOZ_COUNT_CTOR(SubsidyLockParent);

  nsCOMPtr<nsISubsidyLockService> service =
    do_GetService(NS_SUBSIDY_LOCK_SERVICE_CONTRACTID);
  NS_ASSERTION(service, "This shouldn't fail!");

  service->GetItemByServiceId(aClientId, getter_AddRefs(mSubsidyLock));
}

void
SubsidyLockParent::ActorDestroy(ActorDestroyReason why)
{
  mLive = false;
  if (mSubsidyLock) {
     mSubsidyLock = nullptr;
  }
}

bool
SubsidyLockParent::RecvPSubsidyLockRequestConstructor(PSubsidyLockRequestParent* aActor,
                                                      const SubsidyLockRequest& aRequest)
{
  SubsidyLockRequestParent* actor = static_cast<SubsidyLockRequestParent*>(aActor);

  switch (aRequest.type()) {
    case SubsidyLockRequest::TGetSubsidyLockStatusRequest:
      return actor->DoRequest(aRequest.get_GetSubsidyLockStatusRequest());
    case SubsidyLockRequest::TUnlockSubsidyLockRequest:
      return actor->DoRequest(aRequest.get_UnlockSubsidyLockRequest());
    default:
      MOZ_CRASH("Received invalid request type!");
  }

  return false;
}

PSubsidyLockRequestParent*
SubsidyLockParent::AllocPSubsidyLockRequestParent(const SubsidyLockRequest& aRequest)
{
  // We leverage mobileconnection permission for content process permission check.
  if (!AssertAppProcessPermission(Manager(), "mobileconnection")) {
    return nullptr;
  }

  SubsidyLockRequestParent* actor = new SubsidyLockRequestParent(mSubsidyLock);
  // Add an extra ref for IPDL. Will be released in
  // SubsidyLockParent::DeallocPSubsidyLockRequestParent().
  actor->AddRef();
  return actor;
}

bool
SubsidyLockParent::DeallocPSubsidyLockRequestParent(PSubsidyLockRequestParent* aActor)
{
  // SubsidyLockRequestParent is refcounted, must not be freed manually.
  static_cast<SubsidyLockRequestParent*>(aActor)->Release();
  return true;
}

/******************************************************************************
 * PSubsidyLockRequestParent
 ******************************************************************************/

void
SubsidyLockRequestParent::ActorDestroy(ActorDestroyReason aWhy)
{
  mLive = false;
  mSubsidyLock = nullptr;
}

bool
SubsidyLockRequestParent::DoRequest(const GetSubsidyLockStatusRequest& aRequest)
{
  NS_ENSURE_TRUE(mSubsidyLock, false);

  return NS_SUCCEEDED(mSubsidyLock->GetSubsidyLockStatus(this));
}

bool
SubsidyLockRequestParent::DoRequest(const UnlockSubsidyLockRequest& aRequest)
{
  NS_ENSURE_TRUE(mSubsidyLock, false);

  return NS_SUCCEEDED(mSubsidyLock->UnlockSubsidyLock(aRequest.lockType(),
                                                      aRequest.password(),
                                                      this));
}

nsresult
SubsidyLockRequestParent::SendReply(const SubsidyLockReply& aReply)
{
  NS_ENSURE_TRUE(mLive, NS_ERROR_FAILURE);

  return Send__delete__(this, aReply) ? NS_OK : NS_ERROR_FAILURE;
}

// nsISubsidyLockCallback Implementation.

NS_IMPL_ISUPPORTS(SubsidyLockRequestParent, nsISubsidyLockCallback)

NS_IMETHODIMP
SubsidyLockRequestParent::NotifyGetSubsidyLockStatusSuccess(uint32_t aCount,
                                                            uint32_t* aTypes)
{
  nsTArray<uint32_t> types;
  for (uint32_t i = 0; i < aCount; i++) {
    types.AppendElement(aTypes[i]);
  }

  return SendReply(SubsidyLockGetStatusSuccess(types));
}

NS_IMETHODIMP
SubsidyLockRequestParent::NotifyError(const nsAString& aName)
{
  return SendReply(SubsidyLockReplyError(nsAutoString(aName)));
}

NS_IMETHODIMP
SubsidyLockRequestParent::NotifySuccess()
{
  return SendReply(SubsidyLockReplySuccess());
}

NS_IMETHODIMP
SubsidyLockRequestParent::NotifyUnlockSubsidyLockError(const nsAString& aName,
                                                       int32_t aRetryCount)
{
  return SendReply(SubsidyLockUnlockError(aRetryCount, nsAutoString(aName)));
}