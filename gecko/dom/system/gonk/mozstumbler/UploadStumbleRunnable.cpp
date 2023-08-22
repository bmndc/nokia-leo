/* -*- Mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; tab-width: 40 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "UploadStumbleRunnable.h"
#include "StumblerLogging.h"
#include "mozilla/dom/Event.h"
#include "mozilla/Preferences.h"
#include "nsContentUtils.h"
#ifdef HAS_KOOST_MODULES
#include "nsIHawkHelper.h"
#endif
#include "nsIInputStream.h"
#include "nsIScriptSecurityManager.h"
#include "nsISeekableStream.h"
#include "nsIURLFormatter.h"
#include "nsIXMLHttpRequest.h"
#include "nsJSUtils.h"
#include "nsNetUtil.h"
#include "nsStreamUtils.h" // for NS_ConsumeStream
#include "nsVariant.h"

// The Hawk credential which is cached when using location HTTP API
static const char GEO_KAI_HAWK_KID [] = "geolocation.kaios.hawk_key_id";
static const char GEO_KAI_HAWK_MAC [] = "geolocation.kaios.hawk_mac_key";

UploadStumbleRunnable::UploadStumbleRunnable(nsIInputStream* aUploadData)
: mUploadInputStream(aUploadData)
, mNeedAuthorization(false)
{
  mNeedAuthorization = Preferences::GetBool("geo.provider.need_authorization");
}

NS_IMETHODIMP
UploadStumbleRunnable::Run()
{
  MOZ_ASSERT(NS_IsMainThread());

  nsresult rv = Upload();
  if (NS_FAILED(rv)) {
    WriteStumbleOnThread::UploadEnded(false);
  }
  return NS_OK;
}

nsresult
UploadStumbleRunnable::Upload()
{
  STUMBLER_DBG("Upload stumble data file.\n");

  nsresult rv;
  RefPtr<nsVariant> variant = new nsVariant();

  rv = variant->SetAsISupports(mUploadInputStream);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIXMLHttpRequest> xhr = do_CreateInstance(NS_XMLHTTPREQUEST_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIScriptSecurityManager> secman =
    do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIPrincipal> systemPrincipal;
  rv = secman->GetSystemPrincipal(getter_AddRefs(systemPrincipal));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = xhr->Init(systemPrincipal, nullptr, nullptr, nullptr, nullptr);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURLFormatter> formatter =
    do_CreateInstance("@mozilla.org/toolkit/URLFormatterService;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsString url;
  rv = formatter->FormatURLPref(NS_LITERAL_STRING("geo.stumbler.url"), url);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = xhr->Open(NS_LITERAL_CSTRING("POST"), NS_ConvertUTF16toUTF8(url), false, EmptyString(), EmptyString());
  NS_ENSURE_SUCCESS(rv, rv);

  // The POST content is pre-compressed gzip file.
  xhr->SetRequestHeader(NS_LITERAL_CSTRING("Content-Type"),
                        NS_LITERAL_CSTRING("application/gzip"));

  xhr->SetResponseType(NS_LITERAL_STRING("text"));

#ifdef HAS_KOOST_MODULES
  if (mNeedAuthorization) {
    nsString hawkHeader = ComputeHawkHeader(url);
    xhr->SetRequestHeader(NS_LITERAL_CSTRING("Authorization"),
                          NS_ConvertUTF16toUTF8(hawkHeader));
  }
#endif
  xhr->SetMozBackgroundRequest(true);
  // 60s timeout
  xhr->SetTimeout(60 * 1000);

  nsCOMPtr<EventTarget> target(do_QueryInterface(xhr));
  RefPtr<nsIDOMEventListener> listener = new UploadEventListener(xhr);

  const char* const sEventStrings[] = {
    // nsIXMLHttpRequestEventTarget event types
    "abort",
    "error",
    "load",
    "timeout"
  };

  for (uint32_t index = 0; index < MOZ_ARRAY_LENGTH(sEventStrings); index++) {
    nsAutoString eventType = NS_ConvertASCIItoUTF16(sEventStrings[index]);
    rv = target->AddEventListener(eventType, listener, false);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = xhr->Send(variant);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsString
UploadStumbleRunnable::ComputeHawkHeader(const nsAString& requestUrl)
{
#ifdef HAS_KOOST_MODULES
  // Read the request body from the input stream
  nsCString streamBuffer;
  nsresult rv = NS_ConsumeStream(mUploadInputStream, UINT32_MAX, streamBuffer);
  if (NS_FAILED(rv)) {
    STUMBLER_ERR("Failed to read the stream of GeoSubmit");
    return EmptyString();
  }
  nsString requestBody = NS_ConvertASCIItoUTF16(streamBuffer);
  streamBuffer = EmptyCString();

  // Set the stream offset to the begining
  nsCOMPtr<nsISeekableStream> seekableStream
    = do_QueryInterface(mUploadInputStream);
  if (seekableStream) {
    seekableStream->Seek(nsISeekableStream::NS_SEEK_SET, 0);
  }

  nsAdoptingString kid = Preferences::GetString(GEO_KAI_HAWK_KID);
  nsAdoptingString mac = Preferences::GetString(GEO_KAI_HAWK_MAC);
  if (!kid || !mac) {
    STUMBLER_ERR("Failed to get cached Hawk ID and MAC");
    return EmptyString();
  }

  nsCOMPtr<nsIHawkHelper> hawkHelper
    = do_CreateInstance("@kaiostech.com/kaiauth/hawk-helper;1");
  if (hawkHelper) {
    nsString hawkHeader;
    rv = hawkHelper->ComputeHawkHeader(NS_LITERAL_STRING("POST"),
                                       kid,
                                       mac,
                                       requestUrl,
                                       requestBody,
                                       hawkHeader);
    if (NS_SUCCEEDED(rv)) {
      return hawkHeader;
    }
  }
#endif
  return EmptyString();
}

NS_IMPL_ISUPPORTS(UploadEventListener, nsIDOMEventListener)

UploadEventListener::UploadEventListener(nsIXMLHttpRequest* aXHR)
: mXHR(aXHR)
{
}

NS_IMETHODIMP
UploadEventListener::HandleEvent(nsIDOMEvent* aEvent)
{
  nsString type;
  if (NS_FAILED(aEvent->GetType(type))) {
    STUMBLER_ERR("Failed to get event type");
    WriteStumbleOnThread::UploadEnded(false);
    return NS_ERROR_FAILURE;
  }

  if (type.EqualsLiteral("load")) {
    STUMBLER_DBG("Got load Event\n");
  } else if (type.EqualsLiteral("error") && mXHR) {
    STUMBLER_ERR("Upload Error");
  } else {
    STUMBLER_DBG("Receive %s Event", NS_ConvertUTF16toUTF8(type).get());
  }

  uint32_t statusCode = 0;
  bool doDelete = false;
  if (!mXHR) {
    return NS_OK;
  }
  nsresult rv = mXHR->GetStatus(&statusCode);
  if (NS_SUCCEEDED(rv)) {
    STUMBLER_DBG("statuscode %d\n", statusCode);
  }

  nsAutoString response;
  rv = mXHR->GetResponseText(response);
  if (NS_SUCCEEDED(rv)) {
    STUMBLER_DBG("XHR response: %s\n", NS_ConvertUTF16toUTF8(response).get());
  }

  if (200 == statusCode || 400 == statusCode) {
    doDelete = true;
  }

  WriteStumbleOnThread::UploadEnded(doDelete);
  nsCOMPtr<EventTarget> target(do_QueryInterface(mXHR));

  const char* const sEventStrings[] = {
    // nsIXMLHttpRequestEventTarget event types
    "abort",
    "error",
    "load",
    "timeout"
  };

  for (uint32_t index = 0; index < MOZ_ARRAY_LENGTH(sEventStrings); index++) {
    nsAutoString eventType = NS_ConvertASCIItoUTF16(sEventStrings[index]);
    rv = target->RemoveEventListener(eventType, this, false);
  }

  mXHR = nullptr;
  return NS_OK;
}
