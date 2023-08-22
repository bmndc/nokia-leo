/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "RSUManagerCallback.h"
#include "mozilla/dom/RemoteSimUnlockBinding.h"
#include "mozilla/dom/Promise.h"
#include "nsJSUtils.h"

using namespace mozilla::dom;
//using namespace mozilla::dom::rsu;

NS_IMPL_ISUPPORTS(RSUManagerCallback, nsIRSUManagerCallback)

RSUManagerCallback::RSUManagerCallback(Promise* aPromise)
  : mPromise(aPromise)
{
}

// nsIRSUManagerCallback

NS_IMETHODIMP
RSUManagerCallback::NotifySuccess()
{
  mPromise->MaybeResolve(JS::UndefinedHandleValue);
  return NS_OK;
}

NS_IMETHODIMP
RSUManagerCallback::NotifyError(const nsAString& aError)
{
  mPromise->MaybeRejectBrokenly(aError);
  return NS_OK;
}

NS_IMETHODIMP
RSUManagerCallback::NotifyGetStatusResponse(uint16_t type, uint32_t timer)
{
  RsuStatus status;
  status.mType.Construct(type);
  status.mTimer.Construct(timer);
  mPromise->MaybeResolve(status);
  return NS_OK;
}

//Not complete at present.
NS_IMETHODIMP
RSUManagerCallback::NotifyGetBlobResponse(const nsAString& data)
{
  RsuBlob blob;
  blob.mData.Construct(nsString(data));
  mPromise->MaybeResolve(blob);
  return NS_OK;
}

NS_IMETHODIMP
RSUManagerCallback::NotifyGetVersionResponse(uint32_t minorVersion, uint32_t maxVersion)
{
  RsuVersion version;
  version.mMinorVersion.Construct(minorVersion);
  version.mMaxVersion.Construct(maxVersion);
  mPromise->MaybeResolve(version);
  return NS_OK;
}

NS_IMETHODIMP
RSUManagerCallback::NotifyOpenRFResponse(uint32_t aTimer)
{
  RsuTimer timer;
  timer.mTimer.Construct(aTimer);
  mPromise->MaybeResolve(timer);
  return NS_OK;
}
