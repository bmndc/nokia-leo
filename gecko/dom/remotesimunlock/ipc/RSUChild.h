/* (c) 2020 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#ifndef mozilla_dom_RSUChild_h
#define mozilla_dom_RSUChild_h

#include "mozilla/dom/PRSURequestChild.h"
#include "mozilla/dom/PRSUChild.h"
#include "nsIRSUManager.h"
//class nsITVServiceCallback;

namespace mozilla {
namespace dom {

class RSUIPCService;

class RSUChild final : public PRSUChild
{
public:
  explicit RSUChild(RSUIPCService* aService);

  virtual void ActorDestroy(ActorDestroyReason aWhy) override;

  virtual PRSURequestChild*
  AllocPRSURequestChild(const RSUIPCRequest& aRequest) override;

  virtual bool DeallocPRSURequestChild(PRSURequestChild* aActor) override;

  /*virtual bool
  RecvNotifyStatusChange(const uint16_t aStatus) override;*/

  virtual ~RSUChild();

private:
  bool mActorDestroyed;
  RefPtr<RSUIPCService> mService;
};

class RSURequestChild final : public PRSURequestChild
{
public:
  explicit RSURequestChild(nsIRSUManagerCallback* aCallback);

  virtual void ActorDestroy(ActorDestroyReason aWhy) override;

  virtual bool Recv__delete__(const RSUIPCResponse& aResponse) override;

private:
  virtual ~RSURequestChild();

  void DoResponse(const RSUGetStatusResponse& aResponse);

  void DoResponse(const RSUSuccessResponse& aResponse);

  void DoResponse(const RSUErrorResponse& aResponse);
  
  void DoResponse(const RSUGetBlobResponse& aResponse);

  void DoResponse(const RSUGetVersionResponse& aResponse);
  
  void DoResponse(const RSUOpenRFResponse& aResponse);

  bool mActorDestroyed;
  nsCOMPtr<nsIRSUManagerCallback> mCallback;
};

} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_RSUChild_h
