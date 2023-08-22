/* (c) 2020 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#include "mozilla/dom/RemoteSimUnlockBinding.h"
#include "RSUChild.h"

#include "mozilla/unused.h"
#include "nsIMutableArray.h"
#include "RSUIPCService.h"

using namespace mozilla;

namespace mozilla {
namespace dom {

/*
 * Implementation of RSUChild
 */

RSUChild::RSUChild(RSUIPCService* aService)
  : mActorDestroyed(false), mService(aService)
{
  MOZ_ASSERT(mService);

  MOZ_COUNT_CTOR(RSUChild);
}

RSUChild::~RSUChild()
{
  MOZ_COUNT_DTOR(RSUChild);

  if (!mActorDestroyed) {
    Send__delete__(this);
  }
  mService = nullptr;
}

/* virtual */ void
RSUChild::ActorDestroy(ActorDestroyReason aWhy)
{
  mActorDestroyed = true;
  mService = nullptr;
}

/* virtual */ PRSURequestChild*
RSUChild::AllocPRSURequestChild(const RSUIPCRequest& aRequest)
{
  NS_NOTREACHED(
    "We should never be manually allocating PRSURequestChild actors");
  return nullptr;
}

/* virtual */ bool
RSUChild::DeallocPRSURequestChild(PRSURequestChild* aActor)
{
  delete aActor;
  return true;
}

///* virtual */ bool
/*RSUChild::RecvNotifyStatusChange(const uint16_t aStatus)
{
  if(mService) {
    mService->NotifyStatusChange(aStatus);
  }
  return true;
}*/

/*
 * Implementation of RSURequestChild
 */

RSURequestChild::RSURequestChild(nsIRSUManagerCallback* aCallback)
  : mActorDestroyed(false), mCallback(aCallback)
{
  MOZ_COUNT_CTOR(RSURequestChild);
}

RSURequestChild::~RSURequestChild()
{
  MOZ_COUNT_DTOR(RSURequestChild);
  mCallback = nullptr;
}

/* virtual */ void
RSURequestChild::ActorDestroy(ActorDestroyReason aWhy)
{
  mActorDestroyed = true;
  mCallback = nullptr;
}

/* virtual */ bool
RSURequestChild::Recv__delete__(const RSUIPCResponse& aResponse)
{
  if (mActorDestroyed) {
    return true;
  }

  if (!mCallback) {
    return true;
  }

  switch (aResponse.type()) {
    case RSUIPCResponse::TRSUGetStatusResponse:
      DoResponse(aResponse.get_RSUGetStatusResponse());
      break;
    case RSUIPCResponse::TRSUSuccessResponse:
      DoResponse(aResponse.get_RSUSuccessResponse());
      break;
    case RSUIPCResponse::TRSUErrorResponse:
      DoResponse(aResponse.get_RSUErrorResponse());
      break;
    case RSUIPCResponse::TRSUGetBlobResponse:
      DoResponse(aResponse.get_RSUGetBlobResponse());
      break;
    case RSUIPCResponse::TRSUGetVersionResponse:
      DoResponse(aResponse.get_RSUGetVersionResponse());
      break;
    case RSUIPCResponse::TRSUOpenRFResponse:
      DoResponse(aResponse.get_RSUOpenRFResponse());
      break;

    default:
      MOZ_CRASH("Unknown RSUIPCResponse type");
  }

  return true;
}

void
RSURequestChild::DoResponse(const RSUGetStatusResponse& aResponse)
{
  if (mCallback) {
    Unused << mCallback->NotifyGetStatusResponse(aResponse.status(), aResponse.timer());
  }
}

void
RSURequestChild::DoResponse(const RSUSuccessResponse& aResponse)
{
  if (mCallback) {
    Unused << mCallback->NotifySuccess();
  }
}

void
RSURequestChild::DoResponse(const RSUErrorResponse& aResponse)
{
  if (mCallback) {
    Unused << mCallback->NotifyError(aResponse.name());
  }
}

void
RSURequestChild::DoResponse(const RSUGetBlobResponse& aResponse)
{
  if (mCallback) {
    Unused << mCallback->NotifyGetBlobResponse(aResponse.blob());
  }
}

void
RSURequestChild::DoResponse(const RSUGetVersionResponse& aResponse)
{
  if (mCallback) {
    Unused << mCallback->NotifyGetVersionResponse(aResponse.minorVersion(),
                                                            aResponse.maxVersion());
  }
}

void
RSURequestChild::DoResponse(const RSUOpenRFResponse& aResponse)
{
  if (mCallback) {
    Unused << mCallback->NotifyOpenRFResponse(aResponse.timer());
  }
}
} // namespace dom
} // namespace mozilla
