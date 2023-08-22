/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "SpeakerManager.h"

#include "mozilla/Preferences.h"
#include "mozilla/Services.h"

#include "mozilla/dom/Event.h"

#include "AudioChannelService.h"
#include "nsIAppsService.h"
#include "nsIDocShell.h"
#include "nsIDOMClassInfo.h"
#include "nsIDOMEventListener.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIPermissionManager.h"
#include "SpeakerManagerService.h"

namespace mozilla {
namespace dom {

NS_IMPL_QUERY_INTERFACE_INHERITED(SpeakerManager, DOMEventTargetHelper,
                                  nsIDOMEventListener)
NS_IMPL_ADDREF_INHERITED(SpeakerManager, DOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(SpeakerManager, DOMEventTargetHelper)

SpeakerManager::SpeakerManager(SpeakerPolicy aPolicy)
  : mForcespeaker(false)
  , mVisible(false)
  , mAudioChannelActive(false)
  , mIsSystemApp(false)
  , mSystemAppId(nsIScriptSecurityManager::NO_APP_ID)
  , mPolicy(aPolicy)
{
}

SpeakerManager::~SpeakerManager()
{
  SpeakerManagerService *service = SpeakerManagerService::GetOrCreateSpeakerManagerService();
  MOZ_ASSERT(service);

  service->ForceSpeaker(false, false, false, WindowID());
  service->UnRegisterSpeakerManager(this);
  nsCOMPtr<EventTarget> target = do_QueryInterface(GetOwner());
  NS_ENSURE_TRUE_VOID(target);

  target->RemoveSystemEventListener(NS_LITERAL_STRING("visibilitychange"),
                                    this,
                                    /* useCapture = */ true);
}

bool
SpeakerManager::Speakerforced()
{
  SpeakerManagerService *service = SpeakerManagerService::GetOrCreateSpeakerManagerService();
  MOZ_ASSERT(service);
  return service->GetSpeakerStatus();

}

bool
SpeakerManager::Forcespeaker()
{
  return mForcespeaker;
}

void
SpeakerManager::SetForcespeaker(bool aEnable)
{
  if (mForcespeaker != aEnable) {
    MOZ_LOG(SpeakerManagerService::GetSpeakerManagerLog(), LogLevel::Debug,
           ("SpeakerManager, SetForcespeaker, enable %d", aEnable));
    mForcespeaker = aEnable;
    UpdateStatus();
  }
}

void
SpeakerManager::DispatchSimpleEvent(const nsAString& aStr)
{
  MOZ_ASSERT(NS_IsMainThread(), "Not running on main thread");
  nsresult rv = CheckInnerWindowCorrectness();
  if (NS_FAILED(rv)) {
    return;
  }

  RefPtr<Event> event = NS_NewDOMEvent(this, nullptr, nullptr);
  event->InitEvent(aStr, false, false);
  event->SetTrusted(true);

  rv = DispatchDOMEvent(nullptr, event, nullptr, nullptr);
  if (NS_FAILED(rv)) {
    NS_ERROR("Failed to dispatch the event!!!");
    return;
  }
}

static nsresult
GetAppId(nsPIDOMWindowInner* aWindow, uint32_t* aAppId)
{
  *aAppId = nsIScriptSecurityManager::NO_APP_ID;

  nsCOMPtr<nsIDocument> doc = aWindow->GetExtantDoc();
  NS_ENSURE_TRUE(doc, NS_ERROR_FAILURE);

  nsCOMPtr<nsIPrincipal> principal = doc->NodePrincipal();

  nsresult rv = principal->GetAppId(aAppId);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

static nsresult
GetSystemAppId(uint32_t* aSystemAppId)
{
  *aSystemAppId = nsIScriptSecurityManager::NO_APP_ID;

  nsCOMPtr<nsIAppsService> appsService = do_GetService(APPS_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(appsService, NS_ERROR_FAILURE);

  nsAdoptingString systemAppManifest =
    mozilla::Preferences::GetString("b2g.system_manifest_url");
  NS_ENSURE_TRUE(systemAppManifest, NS_ERROR_FAILURE);

  nsresult rv = appsService->GetAppLocalIdByManifestURL(systemAppManifest, aSystemAppId);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

nsresult
SpeakerManager::FindCorrectWindow(nsPIDOMWindowInner* aWindow)
{
  MOZ_ASSERT(aWindow->IsInnerWindow());

  mWindow = aWindow->GetScriptableTop();
  if (NS_WARN_IF(!mWindow)) {
    return NS_OK;
  }

  // From here we do an hack for nested iframes.
  // The system app doesn't have access to the nested iframe objects so it
  // cannot control the volume of the agents running in nested apps. What we do
  // here is to assign those Agents to the top scriptable window of the parent
  // iframe (what is controlled by the system app).
  // For doing this we go recursively back into the chain of windows until we
  // find apps that are not the system one.
  nsCOMPtr<nsPIDOMWindowOuter> outerParent = mWindow->GetParent();
  if (!outerParent || outerParent == mWindow) {
    return NS_OK;
  }

  nsCOMPtr<nsPIDOMWindowInner> parent = outerParent->GetCurrentInnerWindow();
  if (!parent) {
    return NS_OK;
  }

  uint32_t appId;
  nsresult rv = GetAppId(parent, &appId);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (appId == nsIScriptSecurityManager::NO_APP_ID ||
      appId == nsIScriptSecurityManager::UNKNOWN_APP_ID ||
      appId == mSystemAppId) {
    return NS_OK;
  }

  return FindCorrectWindow(parent);
}

nsresult
SpeakerManager::Init(nsPIDOMWindowInner* aWindow)
{
  BindToOwner(aWindow);

  nsCOMPtr<nsIDocShell> docshell = do_GetInterface(GetOwner());
  NS_ENSURE_TRUE(docshell, NS_ERROR_FAILURE);
  docshell->GetIsActive(&mVisible);

  nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(GetOwner());
  NS_ENSURE_TRUE(target, NS_ERROR_FAILURE);

  target->AddSystemEventListener(NS_LITERAL_STRING("visibilitychange"),
                                 this,
                                 /* useCapture = */ true,
                                 /* wantsUntrusted = */ false);

  // Cache System APP ID
  nsresult rv = GetSystemAppId(&mSystemAppId);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  // Get our APP ID
  uint32_t appId;
  rv = GetAppId(aWindow, &appId);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  // System APP uses SpeakerManager to query speakerforced status, and its window
  // may always be visible. Therefore force its policy to be "Query", so it won't
  // compete with other APPs.
  if (appId == mSystemAppId) {
    mIsSystemApp = true;
    mPolicy = SpeakerPolicy::Query;
  }

  rv = FindCorrectWindow(aWindow);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  MOZ_LOG(SpeakerManagerService::GetSpeakerManagerLog(), LogLevel::Debug,
         ("SpeakerManager, Init, window ID %llu, policy %d, is system APP %d",
          WindowID(), mPolicy, mIsSystemApp));

  SpeakerManagerService *service = SpeakerManagerService::GetOrCreateSpeakerManagerService();
  MOZ_ASSERT(service);
  rv = service->RegisterSpeakerManager(this);
  NS_WARN_IF(NS_FAILED(rv));

  // APP may want to keep its forceSpeaker setting same as current global forceSpeaker
  // state, so sync |mForcespeaker| with the global state by default. This can prevent
  // the global state from being disabled and enabled again if APP wants to enable
  // forceSpeaker, and the global state is already on.
  mForcespeaker = Speakerforced();
  UpdateStatus();
  return NS_OK;
}

nsPIDOMWindowInner*
SpeakerManager::GetParentObject() const
{
  return GetOwner();
}

/* static */ already_AddRefed<SpeakerManager>
SpeakerManager::Constructor(const GlobalObject& aGlobal,
                            const Optional<SpeakerPolicy>& aPolicy,
                            ErrorResult& aRv)
{
  nsCOMPtr<nsIScriptGlobalObject> sgo = do_QueryInterface(aGlobal.GetAsSupports());
  if (!sgo) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  nsCOMPtr<nsPIDOMWindowInner> ownerWindow = do_QueryInterface(aGlobal.GetAsSupports());
  if (!ownerWindow) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  nsCOMPtr<nsIPermissionManager> permMgr = services::GetPermissionManager();
  NS_ENSURE_TRUE(permMgr, nullptr);

  uint32_t permission = nsIPermissionManager::DENY_ACTION;
  nsresult rv =
    permMgr->TestPermissionFromWindow(ownerWindow, "speaker-control",
                                      &permission);
  NS_ENSURE_SUCCESS(rv, nullptr);

  if (permission != nsIPermissionManager::ALLOW_ACTION) {
    aRv.Throw(NS_ERROR_DOM_SECURITY_ERR);
    return nullptr;
  }

  // If APP did not specify a policy, set default value as Foreground_or_playing.
  SpeakerPolicy policy = SpeakerPolicy::Foreground_or_playing;
  if (aPolicy.WasPassed()) {
    policy = aPolicy.Value();
  }

  RefPtr<SpeakerManager> object = new SpeakerManager(policy);
  rv = object->Init(ownerWindow);
  if (NS_FAILED(rv)) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }
  return object.forget();
}

JSObject*
SpeakerManager::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return MozSpeakerManagerBinding::Wrap(aCx, this, aGivenProto);
}

NS_IMETHODIMP
SpeakerManager::HandleEvent(nsIDOMEvent* aEvent)
{
  nsAutoString type;
  aEvent->GetType(type);

  if (!type.EqualsLiteral("visibilitychange")) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIDocShell> docshell = do_GetInterface(GetOwner());
  NS_ENSURE_TRUE(docshell, NS_ERROR_FAILURE);

  bool visible = false;
  docshell->GetIsActive(&visible);
  if (mVisible != visible) {
    MOZ_LOG(SpeakerManagerService::GetSpeakerManagerLog(), LogLevel::Debug,
           ("SpeakerManager, HandleEvent, visible %d", visible));
    mVisible = visible;
    UpdateStatus();
  }
  return NS_OK;
}

void
SpeakerManager::SetAudioChannelActive(bool isActive)
{
  if (mAudioChannelActive != isActive) {
    MOZ_LOG(SpeakerManagerService::GetSpeakerManagerLog(), LogLevel::Debug,
           ("SpeakerManager, SetAudioChannelActive, active %d", isActive));
    mAudioChannelActive = isActive;
    UpdateStatus();
  }
}

void SpeakerManager::UpdateStatus()
{
  bool visible = mVisible;
  switch (mPolicy) {
    case SpeakerPolicy::Foreground_or_playing:
      break;
    case SpeakerPolicy::Playing:
      // Expect SpeakerManagerService to respect our setting only when
      // audio channel is active, so let our visibility be always false.
      visible = false;
      break;
    default:
      // SpeakerPolicy::Query. Don't send status to SpeakerManagerService.
      return;
  }

  SpeakerManagerService *service = SpeakerManagerService::GetOrCreateSpeakerManagerService();
  MOZ_ASSERT(service);
  service->ForceSpeaker(mForcespeaker, visible, mAudioChannelActive, WindowID());
}

} // namespace dom
} // namespace mozilla
