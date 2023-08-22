/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#ifndef mozilla_dom_subsidylock_SubsidyLockParent_h
#define mozilla_dom_subsidylock_SubsidyLockParent_h

#include "mozilla/dom/subsidylock/PSubsidyLockParent.h"
#include "mozilla/dom/subsidylock/PSubsidyLockRequestParent.h"
#include "nsISubsidyLockService.h"
#include "nsServiceManagerUtils.h"

namespace mozilla {
namespace dom {
namespace subsidylock {

/**
 * Parent actor of PSubsidyLock. This object is created/destroyed along
 * with child actor.
 */
class SubsidyLockParent : public PSubsidyLockParent
{
  NS_INLINE_DECL_REFCOUNTING(SubsidyLockParent)

public:
  explicit SubsidyLockParent(uint32_t aClientId);

protected:
  virtual
  ~SubsidyLockParent()
  {
    MOZ_COUNT_DTOR(SubsidyLockParent);
  }

  virtual void
  ActorDestroy(ActorDestroyReason aWhy) override;

  virtual bool
  RecvPSubsidyLockRequestConstructor(PSubsidyLockRequestParent* actor,
                                     const SubsidyLockRequest& aRequest) override;

  virtual PSubsidyLockRequestParent*
  AllocPSubsidyLockRequestParent(const SubsidyLockRequest& aRequest) override;

  virtual bool
  DeallocPSubsidyLockRequestParent(PSubsidyLockRequestParent* aActor) override;

private:
  nsCOMPtr<nsISubsidyLock> mSubsidyLock;
  bool mLive;
};

/******************************************************************************
 * PSubsidyLockRequestParent
 ******************************************************************************/

/**
 * Parent actor of PSubsidyLockRequestParent. The object is created along
 * with child actor and destroyed after the callback function of
 * nsIXXXXXXXXXXXCallback is called. Child actor might be destroyed before
 * any callback is triggered. So we use mLive to maintain the status of child
 * actor in order to present sending data to a dead one.
 */
class SubsidyLockRequestParent : public PSubsidyLockRequestParent
                               , public nsISubsidyLockCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISUBSIDYLOCKCALLBACK

  explicit SubsidyLockRequestParent(nsISubsidyLock* aSubsidyLock)
    : mSubsidyLock(aSubsidyLock)
    , mLive(true)
  {
    MOZ_COUNT_CTOR(SubsidyLockRequestParent);
  }

  bool
  DoRequest(const GetSubsidyLockStatusRequest& aRequest);

  bool
  DoRequest(const UnlockSubsidyLockRequest& aRequest);

protected:
  virtual
  ~SubsidyLockRequestParent()
  {
    MOZ_COUNT_DTOR(SubsidyLockRequestParent);
  }

  virtual void
  ActorDestroy(ActorDestroyReason aWhy) override;

  nsresult
  SendReply(const SubsidyLockReply& aReply);

private:
  nsCOMPtr<nsISubsidyLock> mSubsidyLock;
  bool mLive;
};

} // namespace subsidylock
} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_subsidylock_SubsidyLockParent_h