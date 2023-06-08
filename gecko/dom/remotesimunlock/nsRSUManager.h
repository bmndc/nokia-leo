/* (c) 2020 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#ifndef __NSRSUMANGER__
#define __NSRSUMANGER__

#include "nsIRSUManager.h"
#include "nsIRSURequestService.h"
#include "nsCOMPtr.h"
#include "nsThread.h"

namespace mozilla {
  namespace dom {
    class nsRSUManager final: public nsIRSUManager
    {
    public:
      NS_DECL_ISUPPORTS
      NS_DECL_NSIRSUMANAGER
      nsRSUManager();
      static already_AddRefed<nsIRSUManager> FactoryCreate();

    private:
      virtual ~nsRSUManager();
      nsCOMPtr<nsIRSURequestService> mService;
    protected:
      /* additional members */
    };
  } // dom
} // mozilla

#endif //__NSRSUMANGER__
