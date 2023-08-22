/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "RenameToTask.h"

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

namespace mozilla {

using namespace ipc;

namespace dom {

/**
 * RenameToTaskChild
 */

/* static */ already_AddRefed<RenameToTaskChild>
RenameToTaskChild::Create(FileSystemBase* aFileSystem,
                          nsIFile* aDirPath,
                          nsIFile* aOldPath,
                          const nsAString& aNewName,
                          ErrorResult& aRv)

{
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");
  MOZ_ASSERT(aFileSystem);
  MOZ_ASSERT(aDirPath);
  MOZ_ASSERT(aOldPath);

  RefPtr<RenameToTaskChild> task =
    new RenameToTaskChild(aFileSystem, aDirPath, aOldPath, aNewName);

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

RenameToTaskChild::RenameToTaskChild(FileSystemBase* aFileSystem,
                                     nsIFile* aDirPath,
                                     nsIFile* aOldPath,
                                     const nsAString& aNewName)
  : FileSystemTaskChildBase(aFileSystem)
  , mDirPath(aDirPath)
  , mOldPath(aOldPath)
  , mNewName(aNewName)
  , mReturnValue(false)
{
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");
  MOZ_ASSERT(aFileSystem);
  MOZ_ASSERT(aDirPath);
  MOZ_ASSERT(aOldPath);
}

RenameToTaskChild::~RenameToTaskChild()
{
  MOZ_ASSERT(NS_IsMainThread());
}

already_AddRefed<Promise>
RenameToTaskChild::GetPromise()
{
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");
  return RefPtr<Promise>(mPromise).forget();
}

FileSystemParams
RenameToTaskChild::GetRequestParams(const nsString& aSerializedDOMPath,
                                    ErrorResult& aRv) const
{
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");
  FileSystemRenameToParams param;
  param.filesystem() = aSerializedDOMPath;

  aRv = mDirPath->GetPath(param.directory());
  if (NS_WARN_IF(aRv.Failed())) {
    return param;
  }

  param.newName() = mNewName;

  nsAutoString path;
  aRv = mOldPath->GetPath(path);
  if (NS_WARN_IF(aRv.Failed())) {
    return param;
  }

  param.oldRealPath() = path;

  return param;
}

void
RenameToTaskChild::SetSuccessRequestResult(const FileSystemResponseValue& aValue,
                                           ErrorResult& aRv)
{
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");

  FileSystemBooleanResponse r = aValue;
  mReturnValue = r.success();
}

void
RenameToTaskChild::HandlerCallback()
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
RenameToTaskChild::GetPermissionAccessType(nsCString& aAccess) const
{
  aAccess.AssignLiteral(DIRECTORY_WRITE_PERMISSION);
}

/**
 * RenameToTaskParent
 */

/* static */ already_AddRefed<RenameToTaskParent>
RenameToTaskParent::Create(FileSystemBase* aFileSystem,
                           const FileSystemRenameToParams& aParam,
                           FileSystemRequestParent* aParent,
                           ErrorResult& aRv)
{
  MOZ_ASSERT(XRE_IsParentProcess(), "Only call from parent process!");
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aFileSystem);

  RefPtr<RenameToTaskParent> task =
    new RenameToTaskParent(aFileSystem, aParam, aParent);

  NS_ConvertUTF16toUTF8 directoryPath(aParam.directory());
  aRv = NS_NewNativeLocalFile(directoryPath, true,
                              getter_AddRefs(task->mDirPath));
  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }

  task->mNewName = aParam.newName();

  NS_ConvertUTF16toUTF8 path(aParam.oldRealPath());
  aRv = NS_NewNativeLocalFile(path, true, getter_AddRefs(task->mOldPath));
  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }

  if (!FileSystemUtils::IsDescendantPath(task->mDirPath, task->mOldPath)) {
    aRv.Throw(NS_ERROR_DOM_FILESYSTEM_NO_MODIFICATION_ALLOWED_ERR);
    return nullptr;
  }

  return task.forget();
}

RenameToTaskParent::RenameToTaskParent(
        FileSystemBase* aFileSystem,
        const FileSystemRenameToParams& aParam,
        FileSystemRequestParent* aParent)
  : FileSystemTaskParentBase(aFileSystem, aParam, aParent)
  , mReturnValue(false)
{
  MOZ_ASSERT(XRE_IsParentProcess(), "Only call from parent process!");
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aFileSystem);
}

FileSystemResponseValue
RenameToTaskParent::GetSuccessRequestResult(ErrorResult& aRv) const
{
  AssertIsOnBackgroundThread();

  return FileSystemBooleanResponse(mReturnValue);
}

nsresult
RenameToTaskParent::IOWork()
{
  MOZ_ASSERT(XRE_IsParentProcess(),
             "Only call from parent process!");
  MOZ_ASSERT(!NS_IsMainThread(), "Only call on worker thread!");

  if (mFileSystem->IsShutdown()) {
    return NS_ERROR_FAILURE;
  }

  MOZ_ASSERT(FileSystemUtils::IsDescendantPath(mDirPath, mOldPath));

  bool exists = false;
  nsresult rv = mOldPath->Exists(&exists);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (!exists) {
    mReturnValue = false;
    return NS_OK;
  }

  bool isFile = false;
  rv = mOldPath->IsFile(&isFile);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (isFile && !mFileSystem->IsSafeFile(mOldPath)) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  nsCOMPtr<nsIFile> parentDir;
  rv = mOldPath->GetParent(getter_AddRefs(parentDir));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = mOldPath->MoveTo(parentDir, mNewName);

  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  mReturnValue = true;
  return NS_OK;
}

void
RenameToTaskParent::GetPermissionAccessType(nsCString& aAccess) const
{
  aAccess.AssignLiteral(DIRECTORY_WRITE_PERMISSION);
}

} // namespace dom
} // namespace mozilla
