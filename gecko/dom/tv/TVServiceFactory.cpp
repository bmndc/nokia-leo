/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "TVServiceFactory.h"

#ifdef MOZ_WIDGET_GONK
#include "gonk/TVGonkService.h"
#endif
#include "ipc/TVIPCService.h"
#include "nsITVService.h"
#include "nsITVSimulatorService.h"
#include "nsServiceManagerUtils.h"
#include "mozilla/Preferences.h"

namespace mozilla {
namespace dom {

#ifdef MOZ_WIDGET_GONK
static bool
HasDaemon()
{
  static bool tested;
  static bool hasDaemon;

  if (MOZ_UNLIKELY(!tested)) {
    hasDaemon = !access("/system/bin/tvd", X_OK);
    tested = true;
  }

  return hasDaemon;
}
#endif

/* static */ already_AddRefed<nsITVService>
TVServiceFactory::AutoCreateTVService()
{
  nsCOMPtr<nsITVService> service;

  if (GeckoProcessType_Default != XRE_GetProcessType()) {
    service = new TVIPCService();
    return service.forget();
  }

#ifdef MOZ_WIDGET_GONK
  if (HasDaemon()) {
    service = new TVGonkService();
  }
#endif

  if (!service) {
    // Fallback to TV simulator service, especially for TV simulator on WebIDE.
    if (mozilla::Preferences::GetBool("dom.tv.simulator.enabled", true)) {
      service = do_CreateInstance(TV_SIMULATOR_SERVICE_CONTRACTID);
    } else {
      return nullptr;
    }
  }

  return service.forget();
}

} // namespace dom
} // namespace mozilla
