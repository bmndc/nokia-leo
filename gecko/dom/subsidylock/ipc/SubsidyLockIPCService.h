/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#ifndef mozilla_dom_subsidy_SubsidyLockIPCService_h
#define mozilla_dom_subsidy_SubsidyLockIPCService_h

#include "nsCOMPtr.h"
#include "nsISubsidyLockService.h"

namespace mozilla {
namespace dom {
namespace subsidylock {

class SubsidyLockChild;

class SubsidyLockIPCService final : public nsISubsidyLockService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISUBSIDYLOCKSERVICE

  SubsidyLockIPCService();

private:
  // final suppresses -Werror,-Wdelete-non-virtual-dtor
  ~SubsidyLockIPCService();

  nsTArray<RefPtr<SubsidyLockChild>> mItems;
};

} // namespace subsidylock
} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_subsidy_SubsidyLockIPCService_h