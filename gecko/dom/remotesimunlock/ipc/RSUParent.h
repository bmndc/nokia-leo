/* (c) 2020 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#ifndef mozilla_dom_RSUParent_h__
#define mozilla_dom_RSUParent_h__

#include "mozilla/dom/PRSUParent.h"
#include "mozilla/dom/PRSURequestParent.h"
#include "mozilla/dom/PRSU.h"
#include "nsIRSUManager.h"

namespace mozilla {
namespace dom {

class RSUParent final : public PRSUParent,
                        public nsISupports
                      /*, public nsIRSUManagerListener*/
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  //NS_DECL_NSIRSUMANAGERLISTENER

  RSUParent();

  //bool Init();

  virtual void ActorDestroy(ActorDestroyReason aWhy) override;

  virtual bool RecvPRSURequestConstructor(PRSURequestParent* aActor,
                                         const RSUIPCRequest& aRequest) override;

  virtual PRSURequestParent*
  AllocPRSURequestParent(const RSUIPCRequest& aRequest) override;

  virtual bool DeallocPRSURequestParent(PRSURequestParent* aActor) override;

  //virtual bool RecvRegisterListener() override;

  //virtual bool RecvUnregisterListener() override;

private:
  virtual ~RSUParent();

  nsCOMPtr<nsIRSUManager> mService;
};

class RSURequestParent final : public PRSURequestParent,
                              public nsISupports
{
  friend class RSUParent;

public:
  NS_DECL_THREADSAFE_ISUPPORTS

  explicit RSURequestParent(/*nsIRSUManager* aService*/);

  virtual void ActorDestroy(ActorDestroyReason aWhy) override;

  nsresult SendResponse(const RSUIPCResponse& aResponse);

private:
  virtual ~RSURequestParent();

  void DoRequest(const RSUGetStatusRequest& aRequest);
  void DoRequest(const RSUUnlockRequest& aRequest);
  void DoRequest(const RSUGenerateBlobRequest& aRequest);
  void DoRequest(const RSUUpdataBlobRequest& aRequest);
  void DoRequest(const RSUResetBlobRequest& aRequest);
  void DoRequest(const RSUGetVersionRequest& aRequest);
  void DoRequest(const RSUOpenRFRequest& aRequest);
  void DoRequest(const RSUCloseRFRequest& aRequest);

  nsCOMPtr<nsIRSUManager> mService;
};

} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_RSUParent_h__
