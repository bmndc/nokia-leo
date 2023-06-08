/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#ifndef mozilla_dom_subsidy_SubsidyLockCallback_h
#define mozilla_dom_subsidy_SubsidyLockCallback_h

#include "mozilla/dom/DOMRequest.h"
#include "nsCOMPtr.h"
#include "nsISubsidyLockService.h"

namespace mozilla {
namespace dom {
namespace subsidylock {

class SubsidyLockCallback final : public nsISubsidyLockCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISUBSIDYLOCKCALLBACK

  SubsidyLockCallback(nsPIDOMWindowInner* aWindow, DOMRequest* aRequest);

private:
  ~SubsidyLockCallback() {}

  nsresult
  NotifySuccess(JS::Handle<JS::Value> aResult);

  nsCOMPtr<nsPIDOMWindowInner> mWindow;
  RefPtr<DOMRequest> mRequest;
};

} // namespace subsidylock
} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_subsidy_SubsidyLockCallback_h