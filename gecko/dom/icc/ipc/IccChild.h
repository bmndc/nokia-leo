/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_icc_IccChild_h
#define mozilla_dom_icc_IccChild_h

#include "mozilla/dom/icc/PIccChild.h"
#include "mozilla/dom/icc/PIccRequestChild.h"
#include "nsIIccService.h"

namespace mozilla {
namespace dom {

class IccInfo;

namespace icc {

class IccChild final : public PIccChild
                     , public nsIIcc
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIICC

  explicit IccChild();

void
  Init();

  void
  Shutdown();

protected:
  virtual void
  ActorDestroy(ActorDestroyReason why) override;

  virtual PIccRequestChild*
  AllocPIccRequestChild(const IccRequest& aRequest) override;

  virtual bool
  DeallocPIccRequestChild(PIccRequestChild* aActor) override;

  virtual bool
  RecvNotifyCardStateChanged(const uint32_t& aCardState) override;

  virtual bool
  RecvNotifyIccInfoChanged(const OptionalIccInfoData& aInfoData) override;

  virtual bool
  RecvNotifyStkCommand(const nsString& aStkProactiveCmd) override;

  virtual bool
  RecvNotifyStkSessionEnd() override;

private:
  ~IccChild();

  void
  UpdateIccInfo(const OptionalIccInfoData& aInfoData);

  bool
  SendRequest(const IccRequest& aRequest, nsIIccCallback* aRequestReply);

  bool
  SendChannelRequest(const IccRequest& aRequest, nsIIccChannelCallback* aChannelRequestReply);

  nsCOMArray<nsIIccListener> mListeners;
  RefPtr<IccInfo> mIccInfo;
  uint32_t mCardState;
  bool mIsAlive;
};

class IccRequestChild final : public PIccRequestChild
{
public:
  explicit IccRequestChild(nsIIccCallback* aRequestReply);
  explicit IccRequestChild(nsIIccChannelCallback* aChannelRequestReply);

protected:
  virtual bool
  Recv__delete__(const IccReply& aReply) override;

private:
  virtual ~IccRequestChild() {
    MOZ_COUNT_DTOR(IccRequestChild);
  }

  nsCOMPtr<nsIIccCallback> mRequestReply;
  nsCOMPtr<nsIIccChannelCallback> mChannelRequestReply;
};

} // namespace icc
} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_icc_IccChild_h
