/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_RSUManagerCallback_h
#define mozilla_dom_RSUManagerCallback_h

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsIRSUManager.h"

namespace mozilla {
namespace dom {

class Promise;

//namespace rsu {

class RSUManagerCallback : public nsIRSUManagerCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRSUMANAGERCALLBACK

  explicit RSUManagerCallback(Promise* aPromise);

protected:
  virtual ~RSUManagerCallback() {}

protected:
  RefPtr<Promise> mPromise;
};

//} // namespace rsu
} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_RSUManagerCallback_h
