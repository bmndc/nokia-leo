/* (c) 2020 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#ifndef mozilla_dom_RSUIPCService_h
#define mozilla_dom_RSUIPCService_h

#include "mozilla/Tuple.h"
#include "nsTArray.h"
#include "nsIRSUManager.h"

namespace mozilla {
namespace dom {

class RSUIPCRequest;

class RSUIPCService final : public nsIRSUManager
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRSUMANAGER

  RSUIPCService();

  //nsresult NotifyStatusChange(uint16_t status);
  //void Shutdown();

private:
  virtual ~RSUIPCService();

  nsresult SendRequest(nsIRSUManagerCallback* aCallback,
                       const RSUIPCRequest& aRequest);

  //nsCOMPtr<nsIRSUManagerListener> mListener;
};

} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_RSUIPCService_h
