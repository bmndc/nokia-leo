/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "RemotePrintJobChild.h"

#include "mozilla/unused.h"
#include "nsPagePrintTimer.h"
#include "nsPrintEngine.h"

namespace mozilla {
namespace layout {

RemotePrintJobChild::RemotePrintJobChild()
{
  MOZ_COUNT_CTOR(RemotePrintJobChild);
}

nsresult
RemotePrintJobChild::InitializePrint(const nsString& aDocumentTitle,
                                     const nsString& aPrintToFile,
                                     const int32_t& aStartPage,
                                     const int32_t& aEndPage)
{
  // Print initialization can sometimes display a dialog in the parent, so we
  // need to spin a nested event loop until initialization completes.
  Unused << SendInitializePrint(aDocumentTitle, aPrintToFile, aStartPage,
                                aEndPage);
  while (!mPrintInitialized) {
    Unused << NS_ProcessNextEvent();
  }

  return mInitializationResult;
}

bool
RemotePrintJobChild::RecvPrintInitializationResult(const nsresult& aRv)
{
  mPrintInitialized = true;
  mInitializationResult = aRv;
  return true;
}

void
RemotePrintJobChild::ProcessPage(Shmem& aStoredPage)
{
  MOZ_ASSERT(mPagePrintTimer);

  mPagePrintTimer->WaitForRemotePrint();
  Unused << SendProcessPage(aStoredPage);
}

bool
RemotePrintJobChild::RecvPageProcessed()
{
  MOZ_ASSERT(mPagePrintTimer);

  mPagePrintTimer->RemotePrintFinished();
  return true;
}

bool
RemotePrintJobChild::RecvAbortPrint(const nsresult& aRv)
{
  MOZ_ASSERT(mPrintEngine);

  mPrintEngine->CleanupOnFailure(aRv, true);
  return true;
}

void
RemotePrintJobChild::SetPagePrintTimer(nsPagePrintTimer* aPagePrintTimer)
{
  MOZ_ASSERT(aPagePrintTimer);

  mPagePrintTimer = aPagePrintTimer;
}

void
RemotePrintJobChild::SetPrintEngine(nsPrintEngine* aPrintEngine)
{
  MOZ_ASSERT(aPrintEngine);

  mPrintEngine = aPrintEngine;
}

RemotePrintJobChild::~RemotePrintJobChild()
{
  MOZ_COUNT_DTOR(RemotePrintJobChild);
}

void
RemotePrintJobChild::ActorDestroy(ActorDestroyReason aWhy)
{
  mPagePrintTimer = nullptr;
  mPrintEngine = nullptr;
}

} // namespace layout
} // namespace mozilla
