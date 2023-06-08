/*
 * Copyright (C) 2012 Mozilla Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <binder/IPermissionController.h>
#include <cutils/properties.h>
#include <private/android_filesystem_config.h>
#include "GonkPermission.h"

#include "mozilla/dom/ContentParent.h"
#include "mozilla/dom/TabParent.h"
#include "mozilla/SyncRunnable.h"
#include "nsIAppsService.h"
#include "mozIApplication.h"
#include "nsThreadUtils.h"

#undef LOG
#include <android/log.h>
#undef ALOGE
#define ALOGE(args...)  __android_log_print(ANDROID_LOG_ERROR, "gonkperm" , ## args)

using namespace android;
using namespace mozilla;

// Checking permissions needs to happen on the main thread, but the
// binder callback is called on a special binder thread, so we use
// this runnable for that.
class GonkPermissionChecker : public nsRunnable {
  int32_t mPid;
  bool mCanUseCamera;

  explicit GonkPermissionChecker(int32_t pid)
    : mPid(pid)
    , mCanUseCamera(false)
  {
  }

public:
  static already_AddRefed<GonkPermissionChecker> Inspect(int32_t pid)
  {
    RefPtr<GonkPermissionChecker> that = new GonkPermissionChecker(pid);
    nsCOMPtr<nsIThread> mainThread = do_GetMainThread();
    MOZ_ASSERT(mainThread);
    SyncRunnable::DispatchToThread(mainThread, that);
    return that.forget();
  }

  bool CanUseCamera()
  {
    return mCanUseCamera;
  }

  NS_IMETHOD Run();
};

NS_IMETHODIMP
GonkPermissionChecker::Run()
{
  MOZ_ASSERT(NS_IsMainThread());

  // Find our ContentParent.
  dom::ContentParent *contentParent = nullptr;
  {
    nsTArray<dom::ContentParent*> parents;
    dom::ContentParent::GetAll(parents);
    for (uint32_t i = 0; i < parents.Length(); ++i) {
      if (parents[i]->Pid() == mPid) {
	contentParent = parents[i];
	break;
      }
    }
  }
  if (!contentParent) {
    ALOGE("pid=%d denied: can't find ContentParent", mPid);
    return NS_OK;
  }

  // Now iterate its apps...
  const ManagedContainer<dom::PBrowserParent>& browsers =
    contentParent->ManagedPBrowserParent();
  for (auto iter = browsers.ConstIter(); !iter.Done(); iter.Next()) {
    dom::TabParent *tabParent =
      static_cast<dom::TabParent*>(iter.Get()->GetKey());
    nsCOMPtr<mozIApplication> mozApp = tabParent->GetOwnOrContainingApp();
    if (!mozApp) {
      continue;
    }

    // ...and check if any of them has camera access.
    bool appCanUseCamera;
    nsresult rv = mozApp->HasPermission("camera", &appCanUseCamera);
    if (NS_SUCCEEDED(rv) && appCanUseCamera) {
      mCanUseCamera = true;
      return NS_OK;
    }
  }
  return NS_OK;
}

bool
GonkPermissionService::checkPermission(const String16& permission, int32_t pid,
                                     int32_t uid)
{
  // root can do anything.
  if (0 == uid) {
    return true;
  }

  // Check if this the external media process spawned from the api daemon.
  char prop[PROPERTY_VALUE_MAX];
  int length = property_get("kaios.media-daemon.pid", prop, NULL);
  if (length > 0) {
    long mpid = strtol(prop, nullptr, 10);
    if ((errno == ERANGE && (mpid == LONG_MAX || mpid == LONG_MIN))
          || (errno != 0 && mpid == 0)) {
        ALOGE("Failed to parse pid value: kaios.media-daemon.pid=%s", prop);
    } else {
      if (pid == mpid) {
        ALOGI("Granting media permissions to pid %d", pid);
        return true;
      }
    }

  }

  String8 perm8(permission);

  // Some ril implementations need android.permission.MODIFY_AUDIO_SETTINGS
  if ((uid == AID_SYSTEM || uid == AID_RADIO || uid == AID_BLUETOOTH) &&
      perm8 == "android.permission.MODIFY_AUDIO_SETTINGS") {
    return true;
  }

#ifdef ENABLE_SOUNDTRIGGER
  // Sound trigger authentication is done in Gecko's permission manager
  if (perm8 == "android.permission.CAPTURE_AUDIO_HOTWORD") {
    return true;
  }
#endif

  // No other permissions apply to non-app processes.
  if (uid < AID_APP) {
    ALOGE("%s for pid=%d,uid=%d denied: not an app",
      String8(permission).string(), pid, uid);
    return false;
  }

#if ANDROID_VERSION >= 23
  // We grant this permission to adapt to AOSP's foreground user check for camera, as
  // in H5OS the permission is sent from the process of camera app instead of system server
  if (perm8 == "android.permission.CAMERA_SEND_SYSTEM_EVENTS") {
    return true;
  }
#endif

  // Only these permissions can be granted to apps through this service.
  if (perm8 != "android.permission.CAMERA" &&
    perm8 != "android.permission.RECORD_AUDIO" &&
    perm8 != "android.permission.CAPTURE_AUDIO_OUTPUT") {
    ALOGE("%s for pid=%d,uid=%d denied: unsupported permission",
      String8(permission).string(), pid, uid);
    return false;
  }

  // Users granted the permission through a prompt dialog.
  // Before permission managment of gUM is done, app cannot remember the
  // permission.
  PermissionGrant permGrant(perm8.string(), pid);
  if (nsTArray<PermissionGrant>::NoIndex != mGrantArray.IndexOf(permGrant)) {
    mGrantArray.RemoveElement(permGrant);
    return true;
  }

  // Camera/audio record permissions are allowed for apps with the
  // "camera" permission.
  RefPtr<GonkPermissionChecker> checker =
    GonkPermissionChecker::Inspect(pid);
  bool canUseCamera = checker->CanUseCamera();
  if (!canUseCamera) {
    ALOGE("%s for pid=%d,uid=%d denied: not granted by user or app manifest",
      String8(permission).string(), pid, uid);
  }
  return canUseCamera;
}

static GonkPermissionService* gGonkPermissionService = NULL;

/* static */
void
GonkPermissionService::instantiate()
{
  defaultServiceManager()->addService(String16(getServiceName()),
    GetInstance());
}

/* static */
GonkPermissionService*
GonkPermissionService::GetInstance()
{
  if (!gGonkPermissionService) {
    gGonkPermissionService = new GonkPermissionService();
  }
  return gGonkPermissionService;
}

#if ANDROID_VERSION >= 23
void
GonkPermissionService::getPackagesForUid(
  const uid_t uid, android::Vector<android::String16>& name)
{
  // In Android AudioFlinger::openRecord(), recordingAllowed(in ServiceUtilities.cpp)
  // checks the the package name(added in M).
  // In H5OS, the only one to open the AudioRecord is b2g process,
  // and the permissionController in GonkPermissionService should return something.
  name.add(String16("b2g"));
  return;
}

bool
GonkPermissionService::isRuntimePermission(const android::String16& permission)
{
  return true;
}
#endif

void
GonkPermissionService::addGrantInfo(const char* permission, int32_t pid)
{
  mGrantArray.AppendElement(PermissionGrant(permission, pid));
}
