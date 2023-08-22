/* (c) 2020 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#ifndef mozilla_dom_RSU_h
#define mozilla_dom_RSU_h

#include "mozilla/dom/RemoteSimUnlockBinding.h"
#include "mozilla/dom/BindingDeclarations.h"
#include "mozilla/dom/Promise.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "nsPIDOMWindow.h"
#include "nsIRSUManager.h"

namespace mozilla {
namespace dom {

class RemoteSimUnlock final : public DOMEventTargetHelper,
                  private nsIRSUManagerListener
{
  class Listener;
  ~RemoteSimUnlock();
  //nsCOMPtr<nsIRSUManager> mService;

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIRSUMANAGERLISTENER
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(RemoteSimUnlock, DOMEventTargetHelper)
  NS_REALLY_FORWARD_NSIDOMEVENTTARGET(DOMEventTargetHelper)

  RemoteSimUnlock(nsPIDOMWindowInner* aWin);

  nsPIDOMWindowInner* GetParentObject() const
  {
    return GetOwner();
  }
  //WrapperCache
  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aGivenProto) override;

  //static already_AddRefed<RemoteSimUnlock> GetInstance(nsPIDOMWindowInner* aWindow);

  bool IsCalledBySystemOrTestApp();


  //webidl
  already_AddRefed<Promise> GetStatus(ErrorResult& aRv);

  already_AddRefed<Promise> GenerateRequest(ErrorResult& aRv);
  
  already_AddRefed<Promise> UpdataBlob(const nsAString& aData, ErrorResult& aRv);

  already_AddRefed<Promise> GetVersion(ErrorResult& aRv);

  already_AddRefed<Promise> OpenRF(ErrorResult& aRv);

  already_AddRefed<Promise> CloseRF(ErrorResult& aRv);

  //reserved API
  already_AddRefed<Promise> ResetBlob(ErrorResult& aRv);
  already_AddRefed<Promise> Unlock(const Optional<uint16_t>& aRsuType, ErrorResult& aRv);
  already_AddRefed<Promise> GetAllowedType(ErrorResult& aRv);
  already_AddRefed<Promise> RegisterDevice(ErrorResult& aRv);
  already_AddRefed<Promise> GetSLCVersion(ErrorResult& aRv);

  IMPL_EVENT_HANDLER(unlockstatuschange)


private:
  //bool Init();
  void Shutdown();
  RefPtr<Listener> mListener;
  nsPIDOMWindowInner* MOZ_NON_OWNING_REF mWindow;
};

} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_RSU_h

