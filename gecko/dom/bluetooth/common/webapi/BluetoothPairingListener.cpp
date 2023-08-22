/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "BluetoothService.h"
#include "BluetoothUtils.h"
#include "nsIDocument.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsIPrincipal.h"

#include "mozilla/dom/bluetooth/BluetoothPairingListener.h"
#include "mozilla/dom/bluetooth/BluetoothPairingHandle.h"
#include "mozilla/dom/bluetooth/BluetoothTypes.h"
#include "mozilla/dom/BluetoothPairingEvent.h"
#include "mozilla/dom/BluetoothPairingListenerBinding.h"

USING_BLUETOOTH_NAMESPACE

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(BluetoothPairingListener)
NS_INTERFACE_MAP_END_INHERITING(DOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(BluetoothPairingListener, DOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(BluetoothPairingListener, DOMEventTargetHelper)

BluetoothPairingListener::BluetoothPairingListener(nsPIDOMWindowInner* aWindow)
  : DOMEventTargetHelper(aWindow)
  , mHasListenedToSignal(false)
{
  MOZ_ASSERT(aWindow);

  TryListeningToBluetoothSignal();
}

already_AddRefed<BluetoothPairingListener>
BluetoothPairingListener::Create(nsPIDOMWindowInner* aWindow)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aWindow);

  RefPtr<BluetoothPairingListener> handle =
    new BluetoothPairingListener(aWindow);

  return handle.forget();
}

BluetoothPairingListener::~BluetoothPairingListener()
{
  UnregisterBluetoothSignalHandler(NS_LITERAL_STRING(KEY_PAIRING_LISTENER),
                                   this);
}

void
BluetoothPairingListener::DispatchPairingEvent(
  const BluetoothRemoteName& aName,
  const BluetoothAddress& aAddress,
  const nsAString& aPasskey,
  const nsAString& aType)
{
  MOZ_ASSERT(!aAddress.IsCleared());
  MOZ_ASSERT(!aName.IsCleared() && !aType.IsEmpty());

  nsString nameStr;
  RemoteNameToString(aName, nameStr);

  nsString addressStr;
  AddressToString(aAddress, addressStr);

  BluetoothPairingEventInit init;
  init.mAddress = addressStr;
  init.mDeviceName = nameStr;

  // Only allow certified bluetooth application to access BluetoothPairingHandle
  if (IsBluetoothCertifiedApp()) {
    RefPtr<BluetoothPairingHandle> handle =
      BluetoothPairingHandle::Create(GetOwner(), addressStr, aType, aPasskey);
    init.mHandle = handle;
  }

  RefPtr<BluetoothPairingEvent> event =
    BluetoothPairingEvent::Constructor(this,
                                       aType,
                                       init);

  DispatchTrustedEvent(event);
}

void
BluetoothPairingListener::Notify(const BluetoothSignal& aData)
{
  BT_LOGD("[A] %s", NS_ConvertUTF16toUTF8(aData.name()).get());

  InfallibleTArray<BluetoothNamedValue> arr;

  BluetoothValue value = aData.value();
  if (aData.name().EqualsLiteral("PairingRequest")) {

    MOZ_ASSERT(value.type() == BluetoothValue::TArrayOfBluetoothNamedValue);

    const InfallibleTArray<BluetoothNamedValue>& arr =
      value.get_ArrayOfBluetoothNamedValue();

    MOZ_ASSERT(arr.Length() == 4 &&
               arr[0].value().type() == BluetoothValue::TBluetoothAddress && // address
               arr[1].value().type() == BluetoothValue::TBluetoothRemoteName && // name
               arr[2].value().type() == BluetoothValue::TnsString && // passkey
               arr[3].value().type() == BluetoothValue::TnsString);  // type

    BluetoothAddress address = arr[0].value().get_BluetoothAddress();
    const BluetoothRemoteName& name = arr[1].value().get_BluetoothRemoteName();
    nsString passkey = arr[2].value().get_nsString();
    nsString type = arr[3].value().get_nsString();

    // Notify pairing listener of pairing requests
    DispatchPairingEvent(name, address, passkey, type);
  } else {
    BT_WARNING("Not handling pairing listener signal: %s",
               NS_ConvertUTF16toUTF8(aData.name()).get());
  }
}

JSObject*
BluetoothPairingListener::WrapObject(JSContext* aCx,
                                     JS::Handle<JSObject*> aGivenProto)
{
  return BluetoothPairingListenerBinding::Wrap(aCx, this, aGivenProto);
}

void
BluetoothPairingListener::DisconnectFromOwner()
{
  DOMEventTargetHelper::DisconnectFromOwner();
  UnregisterBluetoothSignalHandler(NS_LITERAL_STRING(KEY_PAIRING_LISTENER),
                                   this);
}

void
BluetoothPairingListener::EventListenerAdded(nsIAtom* aType)
{
  DOMEventTargetHelper::EventListenerAdded(aType);

  TryListeningToBluetoothSignal();
}

void
BluetoothPairingListener::TryListeningToBluetoothSignal()
{
  if (mHasListenedToSignal) {
    // We've handled prior pending pairing requests
    return;
  }

  // Listen to bluetooth signal only if all pairing event handlers have been
  // attached. All pending pairing requests queued in BluetoothService would
  // be fired when pairing listener starts listening to bluetooth signal.
  if (!HasListenersFor(nsGkAtoms::ondisplaypasskeyreq) ||
      !HasListenersFor(nsGkAtoms::onenterpincodereq) ||
      !HasListenersFor(nsGkAtoms::onpairingconfirmationreq) ||
      !HasListenersFor(nsGkAtoms::onpairingconsentreq)) {
    BT_LOGD("Pairing listener is not ready to handle pairing requests!");
    return;
  }

  // Start listening to bluetooth signal to handle pairing requests
  RegisterBluetoothSignalHandler(NS_LITERAL_STRING(KEY_PAIRING_LISTENER),
                                 this);

  mHasListenedToSignal = true;
}

bool
BluetoothPairingListener::IsBluetoothCertifiedApp()
{
  NS_ENSURE_TRUE(GetOwner(), false);

  // Retrieve the app status and origin for permission checking
  nsCOMPtr<nsIDocument> doc = GetOwner()->GetExtantDoc();
  NS_ENSURE_TRUE(doc, false);

  uint16_t appStatus = nsIPrincipal::APP_STATUS_NOT_INSTALLED;
  doc->NodePrincipal()->GetAppStatus(&appStatus);
  if (appStatus != nsIPrincipal::APP_STATUS_CERTIFIED) {
   return false;
  }

  // Get the app origin of Bluetooth app from PrefService.
  nsCOMPtr<nsIPrefBranch> prefs(do_GetService(NS_PREFSERVICE_CONTRACTID));
  if (!prefs) {
    BT_WARNING("Failed to get preference service");
    return false;
  }

  nsAutoCString prefOrigin;
  nsresult rv = prefs->GetCharPref(PREF_BLUETOOTH_APP_ORIGIN,
                                   getter_Copies(prefOrigin));
  if (NS_FAILED(rv)) {
    BT_WARNING("Failed to get the pref value '" PREF_BLUETOOTH_APP_ORIGIN "'");
    return false;
  }

  nsAutoCString appOrigin;
  doc->NodePrincipal()->GetOriginNoSuffix(appOrigin);

  // Don't use 'Equals()' since the origin might contain a postfix for appId.
  return appOrigin.Find(prefOrigin) != -1;
}
