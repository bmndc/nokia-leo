/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_CopyOrMoveToTask_h
#define mozilla_dom_CopyOrMoveToTask_h

#include "mozilla/dom/FileSystemTaskBase.h"
#include "nsAutoPtr.h"
#include "mozilla/ErrorResult.h"

namespace mozilla {
namespace dom {

class FileSystemCopyOrMoveToParams;
class BlobImpl;
class Promise;

class CopyOrMoveToTaskChild final : public FileSystemTaskChildBase
{
public:
  static already_AddRefed<CopyOrMoveToTaskChild>
  Create(FileSystemBase* aFileSystem,
         nsIFile* aSrcDir,
         nsIFile* aDstDir,
         nsIFile* aSrcPath,
         nsIFile* aDstPath,
         bool aKeepBoth,
         bool aIsCopy,
         ErrorResult& aRv);

  virtual
  ~CopyOrMoveToTaskChild();

  already_AddRefed<Promise>
  GetPromise();

  virtual void
  GetPermissionAccessType(nsCString& aAccess) const override;

protected:
  virtual FileSystemParams
  GetRequestParams(const nsString& aSerializedDOMPath,
                   ErrorResult& aRv) const override;

  virtual void
  SetSuccessRequestResult(const FileSystemResponseValue& aValue,
                          ErrorResult& aRv) override;

  virtual void
  HandlerCallback() override;

private:
  CopyOrMoveToTaskChild(FileSystemBase* aFileSystem,
                        nsIFile* aSrcDir,
                        nsIFile* aDstDir,
                        nsIFile* aSrcPath,
                        nsIFile* aDstPath,
                        bool aKeepBoth,
                        bool aIsCopy);

  RefPtr<Promise> mPromise;

  // This path is the Directory::mFile.
  nsCOMPtr<nsIFile> mSrcDir;

  // Directory::mFile, or the root directory of given DeviceStorage
  nsCOMPtr<nsIFile> mDstDir;

  nsCOMPtr<nsIFile> mSrcPath;
  nsCOMPtr<nsIFile> mDstPath;

  bool mKeepBoth;
  bool mIsCopy;
  bool mReturnValue;
};

class CopyOrMoveToTaskParent final : public FileSystemTaskParentBase
{
public:
  static already_AddRefed<CopyOrMoveToTaskParent>
  Create(FileSystemBase* aFileSystem,
         const FileSystemCopyOrMoveToParams& aParam,
         FileSystemRequestParent* aParent,
         ErrorResult& aRv);

  virtual void
  GetPermissionAccessType(nsCString& aAccess) const override;

protected:
  virtual FileSystemResponseValue
  GetSuccessRequestResult(ErrorResult& aRv) const override;

  virtual nsresult
  IOWork() override;

private:
  CopyOrMoveToTaskParent(FileSystemBase* aFileSystem,
                         const FileSystemCopyOrMoveToParams& aParam,
                         FileSystemRequestParent* aParent);

  // This path is the Directory::mFile.
  nsCOMPtr<nsIFile> mSrcDir;

  // Directory::mFile, or the root directory of given DeviceStorage
  nsCOMPtr<nsIFile> mDstDir;

  nsCOMPtr<nsIFile> mSrcPath;
  nsCOMPtr<nsIFile> mDstPath;

  bool mKeepBoth;
  bool mIsCopy;
  bool mReturnValue;
};

} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_CopyOrMoveToTask_h