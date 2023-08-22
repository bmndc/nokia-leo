/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* (c) 2018 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#include "MediaOffloadPlayerBase.h"
#include "nsComponentManagerUtils.h"
#include "nsITimer.h"
#include "mozilla/dom/HTMLMediaElement.h"
#include "VideoUtils.h"
#include "mozilla/dom/power/PowerManagerService.h"
#include "mozilla/dom/WakeLock.h"

#include <binder/IPCThreadState.h>
#include <stagefright/foundation/ADebug.h>
#include <stagefright/foundation/ALooper.h>
#include <stagefright/MediaDefs.h>
#include <stagefright/MediaErrors.h>
#include <stagefright/MetaData.h>
#include <stagefright/Utils.h>

#include <binder/IServiceManager.h>
#include <media/IStreamSource.h>
#include <fcntl.h>

using namespace android;

namespace mozilla {

LazyLogModule gMediaOffloadPlayerLog("MediaOffloadPlayerBase");
#define MEDIA_OFFLOAD_LOG(type, msg) \
  MOZ_LOG(gMediaOffloadPlayerLog, type, msg)

MediaOffloadPlayerBase::MediaOffloadPlayerBase(MediaOmxCommonDecoder* aObserver) :
  mIsElementVisible(true),
  mObserver(aObserver)
{
}

void MediaOffloadPlayerBase::SetElementVisibility(bool aIsVisible)
{
  MOZ_ASSERT(NS_IsMainThread());
  mIsElementVisible = aIsVisible;
  if (mIsElementVisible) {
    MEDIA_OFFLOAD_LOG(LogLevel::Debug, ("Element is visible. Start time update"));
    StartTimeUpdate();
  }
}

static void TimeUpdateCallback(nsITimer* aTimer, void* aClosure)
{
  MediaOffloadPlayerBase* player = static_cast<MediaOffloadPlayerBase*>(aClosure);
  player->TimeUpdate();
}

void MediaOffloadPlayerBase::TimeUpdate()
{
  MOZ_ASSERT(NS_IsMainThread());
  TimeStamp now = TimeStamp::Now();

  // If TIMEUPDATE_MS has passed since the last fire update event fired, fire
  // another timeupdate event.
  if ((mLastFireUpdateTime.IsNull() ||
      now - mLastFireUpdateTime >=
          TimeDuration::FromMilliseconds(TIMEUPDATE_MS))) {
    mLastFireUpdateTime = now;
    NotifyPositionChanged();
  }

  if (mPlayState != MediaDecoder::PLAY_STATE_PLAYING || !mIsElementVisible) {
    StopTimeUpdate();
  }
}

nsresult MediaOffloadPlayerBase::StartTimeUpdate()
{
  MOZ_ASSERT(NS_IsMainThread());
  if (mTimeUpdateTimer) {
    return NS_OK;
  }

  mTimeUpdateTimer = do_CreateInstance("@mozilla.org/timer;1");
  return mTimeUpdateTimer->InitWithFuncCallback(TimeUpdateCallback,
      this,
      TIMEUPDATE_MS,
      nsITimer::TYPE_REPEATING_SLACK);
}

nsresult MediaOffloadPlayerBase::StopTimeUpdate()
{
  MOZ_ASSERT(NS_IsMainThread());
  if (!mTimeUpdateTimer) {
    return NS_OK;
  }

  nsresult rv = mTimeUpdateTimer->Cancel();
  mTimeUpdateTimer = nullptr;
  return rv;
}


void MediaOffloadPlayerBase::WakeLockCreate()
{
  MOZ_ASSERT(NS_IsMainThread());
  MEDIA_OFFLOAD_LOG(LogLevel::Debug, ("%s", __FUNCTION__));
  if (!mWakeLock) {
    RefPtr<dom::power::PowerManagerService> pmService =
      dom::power::PowerManagerService::GetInstance();
    NS_ENSURE_TRUE_VOID(pmService);

    ErrorResult rv;
    mWakeLock = pmService->NewWakeLock(NS_LITERAL_STRING("cpu"), nullptr, rv);
  }
}

void MediaOffloadPlayerBase::WakeLockRelease()
{
  MOZ_ASSERT(NS_IsMainThread());
  MEDIA_OFFLOAD_LOG(LogLevel::Debug, ("%s", __FUNCTION__));
  if (mWakeLock) {
    ErrorResult rv;
    mWakeLock->Unlock(rv);
    mWakeLock = nullptr;
  }
}

void MediaOffloadPlayerBase::NotifyPositionChanged()
{
  nsCOMPtr<nsIRunnable> nsEvent =
    NS_NewRunnableMethod(mObserver, &MediaOmxCommonDecoder::NotifyOffloadPlayerPositionChanged);
  NS_DispatchToMainThread(nsEvent);
}

} // namespace mozilla
