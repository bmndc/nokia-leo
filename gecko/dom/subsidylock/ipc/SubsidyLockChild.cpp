/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#include "mozilla/dom/subsidylock/SubsidyLockChild.h"

using namespace mozilla::dom;
using namespace mozilla::dom::subsidylock;

NS_IMPL_ISUPPORTS(SubsidyLockChild, nsISubsidyLock)

SubsidyLockChild::SubsidyLockChild(uint32_t aServiceId)
  : mServiceId(aServiceId)
  , mLive(true)
{
  MOZ_COUNT_CTOR(SubsidyLockChild);
}

void
SubsidyLockChild::Shutdown()
{
  if (mLive) {
    mLive = false;
    Send__delete__(this);
  }
}

// nsISubsidyLock

NS_IMETHODIMP
SubsidyLockChild::GetServiceId(uint32_t *aServiceId)
{
  *aServiceId = mServiceId;
  return NS_OK;
}

NS_IMETHODIMP
SubsidyLockChild::GetSubsidyLockStatus(nsISubsidyLockCallback* aCallback)
{
  return SendRequest(GetSubsidyLockStatusRequest(), aCallback)
    ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
SubsidyLockChild::UnlockSubsidyLock(uint32_t aLockType,
                                    const nsAString& aPassword,
                                    nsISubsidyLockCallback* aCallback)
{
  return SendRequest(UnlockSubsidyLockRequest(aLockType,
                                              nsAutoString(aPassword)),
                     aCallback)
    ? NS_OK : NS_ERROR_FAILURE;
}

bool
SubsidyLockChild::SendRequest(const SubsidyLockRequest& aRequest,
                              nsISubsidyLockCallback* aCallback)
{
  NS_ENSURE_TRUE(mLive, false);

  SubsidyLockRequestChild* actor = new SubsidyLockRequestChild(aCallback);
  SendPSubsidyLockRequestConstructor(actor, aRequest);

  return true;
}

void
SubsidyLockChild::ActorDestroy(ActorDestroyReason why)
{
  mLive = false;
}

PSubsidyLockRequestChild*
SubsidyLockChild::AllocPSubsidyLockRequestChild(const SubsidyLockRequest& request)
{
  MOZ_CRASH("Caller is supposed to manually construct a request!");
}

bool
SubsidyLockChild::DeallocPSubsidyLockRequestChild(PSubsidyLockRequestChild* aActor)
{
  delete aActor;
  return true;
}

/******************************************************************************
 * SubsidyLockRequestChild
 ******************************************************************************/

void
SubsidyLockRequestChild::ActorDestroy(ActorDestroyReason why)
{
  mRequestCallback = nullptr;
}

bool
SubsidyLockRequestChild::DoReply(const SubsidyLockGetStatusSuccess& aReply)
{
  uint32_t len = aReply.types().Length();
  uint32_t types[len];
  for (uint32_t i = 0; i < len; i++) {
    types[i] = aReply.types()[i];
  }

  return NS_SUCCEEDED(
    mRequestCallback->NotifyGetSubsidyLockStatusSuccess(len, types));
}

bool
SubsidyLockRequestChild::DoReply(const SubsidyLockReplySuccess& aReply)
{
  return NS_SUCCEEDED(mRequestCallback->NotifySuccess());
}

bool
SubsidyLockRequestChild::DoReply(const SubsidyLockReplyError& aReply)
{
  return NS_SUCCEEDED(mRequestCallback->NotifyError(aReply.message()));
}

bool
SubsidyLockRequestChild::DoReply(const SubsidyLockUnlockError& aReply)
{
  return NS_SUCCEEDED(mRequestCallback->NotifyUnlockSubsidyLockError(aReply.message(),
                                                                     aReply.remainingRetry()));
}

bool
SubsidyLockRequestChild::Recv__delete__(const SubsidyLockReply& aReply)
{
  MOZ_ASSERT(mRequestCallback);

  switch (aReply.type()) {
    case SubsidyLockReply::TSubsidyLockGetStatusSuccess:
      return DoReply(aReply.get_SubsidyLockGetStatusSuccess());
    case SubsidyLockReply::TSubsidyLockReplySuccess:
      return DoReply(aReply.get_SubsidyLockReplySuccess());
    case SubsidyLockReply::TSubsidyLockReplyError:
      return DoReply(aReply.get_SubsidyLockReplyError());
    case SubsidyLockReply::TSubsidyLockUnlockError:
      return DoReply(aReply.get_SubsidyLockUnlockError());
    default:
      MOZ_CRASH("Received invalid response type!");
  }

  return false;

}