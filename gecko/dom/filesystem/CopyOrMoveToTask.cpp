/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "CopyOrMoveToTask.h"

#include "mozilla/dom/File.h"
#include "mozilla/dom/FileSystemBase.h"
#include "mozilla/dom/FileSystemUtils.h"
#include "mozilla/dom/PFileSystemParams.h"
#include "mozilla/dom/Promise.h"
#include "mozilla/dom/ipc/BlobChild.h"
#include "mozilla/dom/ipc/BlobParent.h"
#include "mozilla/ipc/BackgroundParent.h"
#include "nsIFile.h"
#include "nsStringGlue.h"
#include "nsPrintfCString.h"

namespace mozilla {
namespace dom {

/**
 * CopyOrMoveToTaskChild
 */

/* static */ already_AddRefed<CopyOrMoveToTaskChild>
CopyOrMoveToTaskChild::Create(FileSystemBase* aFileSystem,
                              nsIFile* aSrcDir,
                              nsIFile* aDirDir,
                              nsIFile* aSrcPath,
                              nsIFile* aDstPath,
                              bool aKeepBoth,
                              bool aIsCopy,
                              ErrorResult& aRv)

{
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");
  MOZ_ASSERT(aFileSystem);
  MOZ_ASSERT(aSrcDir);
  MOZ_ASSERT(aDirDir);
  MOZ_ASSERT(aSrcPath);
  MOZ_ASSERT(aDstPath);

  RefPtr<CopyOrMoveToTaskChild> task =
    new CopyOrMoveToTaskChild(aFileSystem, aSrcDir, aDirDir, aSrcPath, aDstPath,
                              aKeepBoth, aIsCopy);

  nsCOMPtr<nsIGlobalObject> globalObject =
    do_QueryInterface(aFileSystem->GetParentObject());
  if (NS_WARN_IF(!globalObject)) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  task->mPromise = Promise::Create(globalObject, aRv);
  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }

  return task.forget();
}

CopyOrMoveToTaskChild::CopyOrMoveToTaskChild(FileSystemBase* aFileSystem,
                                            nsIFile* aSrcDir,
                                            nsIFile* aDstDir,
                                            nsIFile* aSrcPath,
                                            nsIFile* aDstPath,
                                            bool aKeepBoth,
                                            bool aIsCopy)
  : FileSystemTaskChildBase(aFileSystem)
  , mSrcDir(aSrcDir)
  , mDstDir(aDstDir)
  , mSrcPath(aSrcPath)
  , mDstPath(aDstPath)
  , mKeepBoth(aKeepBoth)
  , mIsCopy(aIsCopy)
  , mReturnValue(false)
{
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");
  MOZ_ASSERT(aFileSystem);
  MOZ_ASSERT(aSrcDir);
  MOZ_ASSERT(aDstDir);
  MOZ_ASSERT(aSrcPath);
  MOZ_ASSERT(aDstPath);
}

CopyOrMoveToTaskChild::~CopyOrMoveToTaskChild()
{
  MOZ_ASSERT(NS_IsMainThread());
}

already_AddRefed<Promise>
CopyOrMoveToTaskChild::GetPromise()
{
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");
  return RefPtr<Promise>(mPromise).forget();
}

FileSystemParams
CopyOrMoveToTaskChild::GetRequestParams(const nsString& aSerializedDOMPath,
                                        ErrorResult& aRv) const
{
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");
  FileSystemCopyOrMoveToParams param;
  param.filesystem() = aSerializedDOMPath;

  aRv = mSrcDir->GetPath(param.srcDirectory());
  if (NS_WARN_IF(aRv.Failed())) {
    return param;
  }

  aRv = mDstDir->GetPath(param.dstDirectory());
  if (NS_WARN_IF(aRv.Failed())) {
    return param;
  }

  param.keepBoth() = mKeepBoth;
  param.isCopy() = mIsCopy;

  nsAutoString srcPath;
  aRv = mSrcPath->GetPath(srcPath);
  if (NS_WARN_IF(aRv.Failed())) {
    return param;
  }
  param.srcRealPath() = srcPath;

  nsAutoString dstPath;
  aRv = mDstPath->GetPath(dstPath);
  if (NS_WARN_IF(aRv.Failed())) {
    return param;
  }
  param.dstRealPath() = dstPath;


  return param;
}

void
CopyOrMoveToTaskChild::SetSuccessRequestResult(
      const FileSystemResponseValue& aValue,
      ErrorResult& aRv)
{
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");

  FileSystemBooleanResponse r = aValue;
  mReturnValue = r.success();
}

void
CopyOrMoveToTaskChild::HandlerCallback()
{
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");

  if (mFileSystem->IsShutdown()) {
    mPromise = nullptr;
    return;
  }

  if (HasError()) {
    mPromise->MaybeReject(mErrorValue);
    mPromise = nullptr;
    return;
  }

  mPromise->MaybeResolve(mReturnValue);
  mPromise = nullptr;
}

void
CopyOrMoveToTaskChild::GetPermissionAccessType(nsCString& aAccess) const
{
  aAccess.AssignLiteral(DIRECTORY_WRITE_PERMISSION);
}

/**
 * CopyOrMoveToTaskParent
 */

/* static */ already_AddRefed<CopyOrMoveToTaskParent>
CopyOrMoveToTaskParent::Create(FileSystemBase* aFileSystem,
                              const FileSystemCopyOrMoveToParams& aParam,
                              FileSystemRequestParent* aParent,
                              ErrorResult& aRv)
{
  MOZ_ASSERT(XRE_IsParentProcess(), "Only call from parent process!");
  mozilla::ipc::AssertIsOnBackgroundThread();
  MOZ_ASSERT(aFileSystem);

  RefPtr<CopyOrMoveToTaskParent> task =
    new CopyOrMoveToTaskParent(aFileSystem, aParam, aParent);

  NS_ConvertUTF16toUTF8 srcDirectoryPath(aParam.srcDirectory());
  aRv = NS_NewNativeLocalFile(srcDirectoryPath, true,
                              getter_AddRefs(task->mSrcDir));
  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }

  NS_ConvertUTF16toUTF8 dstDirectoryPath(aParam.dstDirectory());
  aRv = NS_NewNativeLocalFile(dstDirectoryPath, true,
                              getter_AddRefs(task->mDstDir));
  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }

  task->mKeepBoth = aParam.keepBoth();
  task->mIsCopy = aParam.isCopy();

  NS_ConvertUTF16toUTF8 srcPath(aParam.srcRealPath());
  aRv = NS_NewNativeLocalFile(srcPath, true, getter_AddRefs(task->mSrcPath));
  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }

  NS_ConvertUTF16toUTF8 dstPath(aParam.dstRealPath());
  aRv = NS_NewNativeLocalFile(dstPath, true, getter_AddRefs(task->mDstPath));
  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }

  if (!FileSystemUtils::IsDescendantPath(task->mSrcDir, task->mSrcPath) ||
      !FileSystemUtils::IsDescendantPath(task->mDstDir, task->mDstPath, true) ) {
    aRv.Throw(NS_ERROR_DOM_FILESYSTEM_NO_MODIFICATION_ALLOWED_ERR);
    return nullptr;
  }

  return task.forget();
}

CopyOrMoveToTaskParent::CopyOrMoveToTaskParent(
        FileSystemBase* aFileSystem,
        const FileSystemCopyOrMoveToParams& aParam,
        FileSystemRequestParent* aParent)
  : FileSystemTaskParentBase(aFileSystem, aParam, aParent)
  , mKeepBoth(false)
  , mIsCopy(false)
  , mReturnValue(false)
{
  MOZ_ASSERT(XRE_IsParentProcess(), "Only call from parent process!");
  mozilla::ipc::AssertIsOnBackgroundThread();
  MOZ_ASSERT(aFileSystem);
}

FileSystemResponseValue
CopyOrMoveToTaskParent::GetSuccessRequestResult(ErrorResult& aRv) const
{
  mozilla::ipc::AssertIsOnBackgroundThread();

  return FileSystemBooleanResponse(mReturnValue);
}

nsresult
CopyOrMoveToTaskParent::IOWork()
{
  MOZ_ASSERT(XRE_IsParentProcess(),
             "Only call from parent process!");
  MOZ_ASSERT(!NS_IsMainThread(), "Only call on worker thread!");

  if (mFileSystem->IsShutdown()) {
    return NS_ERROR_FAILURE;
  }

  MOZ_ASSERT(FileSystemUtils::IsDescendantPath(mSrcDir, mSrcPath));
  MOZ_ASSERT(FileSystemUtils::IsDescendantPath(mDstDir, mDstPath, true));

  nsString fileName;

  bool exists = false;
  nsresult rv = mSrcPath->Exists(&exists);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (!exists) {
    mReturnValue = false;
    return NS_OK;
  }

  bool isFile = false;
  rv = mSrcPath->IsFile(&isFile);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (isFile && !mFileSystem->IsSafeFile(mSrcPath)) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  rv = mDstPath->Exists(&exists);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (!exists) {
    mReturnValue = false;
    return NS_OK;
  }

  rv = mDstPath->IsFile(&isFile);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  if (isFile) {
    return NS_ERROR_DOM_FILESYSTEM_NO_MODIFICATION_ALLOWED_ERR;
  }

  rv = mSrcPath->GetLeafName(fileName);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (mKeepBoth) {
    nsString destPathStr;
    nsString predictPathStr;
    nsCOMPtr<nsIFile> predictPath;

    rv = mDstPath->GetPath(destPathStr);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    predictPathStr = destPathStr + NS_ConvertASCIItoUTF16("/") + fileName;
    rv = NS_NewLocalFile(predictPathStr, false, getter_AddRefs(predictPath));
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    rv = predictPath->Exists(&exists);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    if (exists) {
      nsString rootName, suffix;
      const int32_t lastDot = fileName.RFindChar('.');
      if (lastDot == kNotFound || lastDot == 0) {
        rootName = fileName;
      } else {
        suffix = Substring(fileName, lastDot);
        rootName = Substring(fileName, 0, lastDot);
      }

      for (int indx = 1; indx < 10000; indx++) {
        predictPathStr = destPathStr + NS_ConvertASCIItoUTF16("/") + rootName +
          NS_ConvertASCIItoUTF16(nsPrintfCString("(%d)", indx)) + suffix;
        rv = NS_NewLocalFile(predictPathStr, false, getter_AddRefs(predictPath));
        if (NS_WARN_IF(NS_FAILED(rv))) {
          return rv;
        }

        rv = predictPath->Exists(&exists);
        if (NS_WARN_IF(NS_FAILED(rv))) {
          return rv;
        }

        if (!exists) {
          fileName = rootName +
            NS_ConvertASCIItoUTF16(nsPrintfCString("(%d)", indx)) + suffix;
          break;
        }
      }
    }
  }

  if (mIsCopy) {
    rv = mSrcPath->CopyTo(mDstPath, fileName);
  } else {
    rv = mSrcPath->MoveTo(mDstPath, fileName);
  }

  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  mReturnValue = true;
  return NS_OK;
}

void
CopyOrMoveToTaskParent::GetPermissionAccessType(nsCString& aAccess) const
{
  aAccess.AssignLiteral(DIRECTORY_WRITE_PERMISSION);
}

} // namespace dom
} // namespace mozilla