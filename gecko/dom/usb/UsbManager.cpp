/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <limits>
#include "UsbManager.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/Hal.h"
#include "mozilla/dom/UsbEvent.h"
#include "mozilla/dom/UsbManagerBinding.h"
#include "nsIDOMClassInfo.h"

/**
 * We have to use macros here because our leak analysis tool things we are
 * leaking strings when we have |static const nsString|. Sad :(
 */
#define USB_STATUS_CHANGE_NAME NS_LITERAL_STRING("usbstatuschange")

namespace mozilla {
namespace dom {
namespace usb {

UsbManager::UsbManager(nsPIDOMWindowInner* aWindow)
  : DOMEventTargetHelper(aWindow)
  , mDeviceAttached(false)
  , mDeviceConfigured(false)
  , mDebounce(false)
{
}

void
UsbManager::Init()
{
  hal::RegisterUsbObserver(this);

  hal::UsbStatus usbStatus;
  hal::GetCurrentUsbStatus(&usbStatus);

  UpdateFromUsbStatus(usbStatus);
}

void
UsbManager::Shutdown()
{
  hal::UnregisterUsbObserver(this);
}

JSObject*
UsbManager::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return UsbManagerBinding::Wrap(aCx, this, aGivenProto);
}

void
UsbManager::UpdateFromUsbStatus(const hal::UsbStatus& aUsbStatus)
{
  mDeviceAttached = aUsbStatus.deviceAttached();
  mDeviceConfigured = aUsbStatus.deviceConfigured();
}

void
UsbManager::Notify(const hal::UsbStatus& aUsbStatus)
{
  bool previousDeviceAttached = mDeviceAttached;
  bool previousDeviceConfigured = mDeviceConfigured;

  UpdateFromUsbStatus(aUsbStatus);

  if (previousDeviceAttached != mDeviceAttached ||
      previousDeviceConfigured != mDeviceConfigured) {
    UsbEventInit init;
    init.mDeviceAttached = aUsbStatus.deviceAttached();
    init.mDeviceConfigured = aUsbStatus.deviceConfigured();

    RefPtr<UsbEvent> event = UsbEvent::Constructor(this,
                                                   USB_STATUS_CHANGE_NAME,
                                                   init);
    // Delay for debouncing USB disconnects.
    // We often get rapid connect/disconnect events
    // when enabling USB functions which need debouncing.
    if (!mDeviceAttached && !mDebounce) {
      MessageLoopForIO::current()->PostDelayedTask(FROM_HERE,
          NewRunnableMethod(this, &UsbManager::DebounceEvent), 1000);
      mDebounce = true;
      return;
    }
    mDebounce = false;
    DispatchTrustedEvent(event);
  }
}

void
UsbManager::DebounceEvent()
{
  if (!mDebounce)
    return;
  UsbEventInit init;
  init.mDeviceAttached = mDeviceAttached;
  init.mDeviceConfigured = mDeviceConfigured;

  RefPtr<UsbEvent> event = UsbEvent::Constructor(this,
                                                 USB_STATUS_CHANGE_NAME,
                                                 init);
  mDebounce = false;
  DispatchTrustedEvent(event);
}

} // namespace usb
} // namespace dom
} // namespace mozilla
