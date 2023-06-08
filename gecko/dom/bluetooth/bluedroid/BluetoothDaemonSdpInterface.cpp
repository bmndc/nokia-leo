/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#include "BluetoothDaemonSdpInterface.h"
#include "mozilla/UniquePtr.h"
#include "mozilla/unused.h"

BEGIN_BLUETOOTH_NAMESPACE

using namespace mozilla::ipc;

//
// SDP module
//

BluetoothSdpNotificationHandler*
  BluetoothDaemonSdpModule::sNotificationHandler;


void
BluetoothDaemonSdpModule::SetNotificationHandler(
  BluetoothSdpNotificationHandler* aNotificationHandler)
{
  sNotificationHandler = aNotificationHandler;
}

void
BluetoothDaemonSdpModule::HandleSvc(const DaemonSocketPDUHeader& aHeader,
                                    DaemonSocketPDU& aPDU,
                                    DaemonSocketResultHandler* aRes)
{
  static void (BluetoothDaemonSdpModule::* const HandleOp[])(
    const DaemonSocketPDUHeader&, DaemonSocketPDU&,
    DaemonSocketResultHandler*) = {
    [0] = &BluetoothDaemonSdpModule::HandleRsp,
    [1] = &BluetoothDaemonSdpModule::HandleNtf
  };

  MOZ_ASSERT(!NS_IsMainThread());
  // negate twice to map bit to 0/1
  unsigned int isNtf = !!(aHeader.mOpcode & 0x80);

  (this->*(HandleOp[isNtf]))(aHeader, aPDU, aRes);
}

// Commands
//

nsresult
BluetoothDaemonSdpModule::SdpSearchCmd(
  const BluetoothAddress& aRemoteAddr, const BluetoothUuid& aUuid,
  BluetoothSdpResultHandler* aRes)
{
  MOZ_ASSERT(NS_IsMainThread());

  UniquePtr<DaemonSocketPDU> pdu =
    MakeUnique<DaemonSocketPDU>(SERVICE_ID, OPCODE_SDP_SEARCH,
                                6 + 16); // address + UUID

  nsresult rv;
  rv = PackPDU(aRemoteAddr, *pdu);
  if (NS_FAILED(rv)) {
    return rv;
  }
  rv = PackPDU(aUuid, *pdu);
  if (NS_FAILED(rv)) {
    return rv;
  }

  rv = Send(pdu.get(), aRes);
  if (NS_FAILED(rv)) {
    return rv;
  }
  Unused << pdu.release();

  return NS_OK;
}

nsresult
BluetoothDaemonSdpModule::CreateSdpRecordCmd(
  const BluetoothSdpRecord& aRecord, int& aRecordHandle,
  BluetoothSdpResultHandler* aRes)
{
  MOZ_ASSERT(NS_IsMainThread());

  UniquePtr<DaemonSocketPDU> pdu =
    MakeUnique<DaemonSocketPDU>(SERVICE_ID, OPCODE_CREATE_SDP_RECORD,
                                44); // Bluetooth SDP record

  nsresult rv = PackPDU(aRecord, *pdu);
  if (NS_FAILED(rv)) {
    return rv;
  }

  rv = Send(pdu.get(), aRes);
  if (NS_FAILED(rv)) {
    return rv;
  }
  Unused << pdu.release();

  return NS_OK;
}

nsresult
BluetoothDaemonSdpModule::RemoveSdpRecordCmd(
  int aSdpHandle, BluetoothSdpResultHandler* aRes)
{
  MOZ_ASSERT(NS_IsMainThread());

  UniquePtr<DaemonSocketPDU> pdu =
    MakeUnique<DaemonSocketPDU>(SERVICE_ID, OPCODE_REMOVE_SDP_RECORD,
                                4); // SDP handle

  nsresult rv = PackPDU(aSdpHandle, *pdu);
  if (NS_FAILED(rv)) {
    return rv;
  }
  rv = Send(pdu.get(), aRes);
  if (NS_FAILED(rv)) {
    return rv;
  }
  Unused << pdu.release();

  return NS_OK;
}

// Responses
//

void
BluetoothDaemonSdpModule::ErrorRsp(
  const DaemonSocketPDUHeader& aHeader,
  DaemonSocketPDU& aPDU, BluetoothSdpResultHandler* aRes)
{
  ErrorRunnable::Dispatch(
    aRes, &BluetoothSdpResultHandler::OnError, UnpackPDUInitOp(aPDU));
}

void
BluetoothDaemonSdpModule::SdpSearchRsp(
  const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
  BluetoothSdpResultHandler* aRes)
{
  ResultRunnable::Dispatch(
    aRes, &BluetoothSdpResultHandler::SdpSearch, UnpackPDUInitOp(aPDU));
}

void
BluetoothDaemonSdpModule::CreateSdpRecordRsp(
  const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
  BluetoothSdpResultHandler* aRes)
{
  ResultRunnable::Dispatch(
    aRes, &BluetoothSdpResultHandler::CreateSdpRecord, UnpackPDUInitOp(aPDU));
}

void
BluetoothDaemonSdpModule::RemoveSdpRecordRsp(
  const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
  BluetoothSdpResultHandler* aRes)
{
  ResultRunnable::Dispatch(
    aRes, &BluetoothSdpResultHandler::RemoveSdpRecord, UnpackPDUInitOp(aPDU));
}

void
BluetoothDaemonSdpModule::HandleRsp(
  const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
  DaemonSocketResultHandler* aRes)
{
  static void (BluetoothDaemonSdpModule::* const HandleRsp[])(
    const DaemonSocketPDUHeader&,
    DaemonSocketPDU&,
    BluetoothSdpResultHandler*) = {
    [OPCODE_ERROR] = &BluetoothDaemonSdpModule::ErrorRsp,
    [OPCODE_SDP_SEARCH] = &BluetoothDaemonSdpModule::SdpSearchRsp,
    [OPCODE_CREATE_SDP_RECORD] = &BluetoothDaemonSdpModule::CreateSdpRecordRsp,
    [OPCODE_REMOVE_SDP_RECORD] = &BluetoothDaemonSdpModule::RemoveSdpRecordRsp
  };

  MOZ_ASSERT(!NS_IsMainThread()); // I/O thread

  if (NS_WARN_IF(!(aHeader.mOpcode < MOZ_ARRAY_LENGTH(HandleRsp))) ||
      NS_WARN_IF(!HandleRsp[aHeader.mOpcode])) {
    return;
  }

  RefPtr<BluetoothSdpResultHandler> res =
    static_cast<BluetoothSdpResultHandler*>(aRes);

  if (!res) {
    return; // Return early if no result handler has been set for response
  }

  (this->*(HandleRsp[aHeader.mOpcode]))(aHeader, aPDU, res);
}

// Notifications
//

// Returns the current notification handler to a notification runnable
class BluetoothDaemonSdpModule::NotificationHandlerWrapper final
{
public:
  typedef BluetoothSdpNotificationHandler ObjectType;

  static ObjectType* GetInstance()
  {
    MOZ_ASSERT(NS_IsMainThread());

    return sNotificationHandler;
  }
};

void
BluetoothDaemonSdpModule::SdpSearchNtf(
  const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU)
{
  SdpSearchNotification::Dispatch(
    &BluetoothSdpNotificationHandler::SdpSearchNotification,
    UnpackPDUInitOp(aPDU));
}

void
BluetoothDaemonSdpModule::HandleNtf(
  const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
  DaemonSocketResultHandler* aRes)
{
  static void (BluetoothDaemonSdpModule::* const HandleNtf[])(
    const DaemonSocketPDUHeader&, DaemonSocketPDU&) = {
    [0] = &BluetoothDaemonSdpModule::SdpSearchNtf
  };

  MOZ_ASSERT(!NS_IsMainThread());

  uint8_t index = aHeader.mOpcode - 0x81;

  if (NS_WARN_IF(!(index < MOZ_ARRAY_LENGTH(HandleNtf))) ||
      NS_WARN_IF(!HandleNtf[index])) {
    return;
  }

  (this->*(HandleNtf[index]))(aHeader, aPDU);
}


//
// SDP interface
//

BluetoothDaemonSdpInterface::BluetoothDaemonSdpInterface(
  BluetoothDaemonSdpModule* aModule)
  : mModule(aModule)
{
}

BluetoothDaemonSdpInterface::~BluetoothDaemonSdpInterface()
{
}

void
BluetoothDaemonSdpInterface::SetNotificationHandler(
  BluetoothSdpNotificationHandler* aNotificationHandler)
{
  MOZ_ASSERT(mModule);

  mModule->SetNotificationHandler(aNotificationHandler);
}

/* SdpSearch / CreateSdpRecord / RemoveSdpRecord */

void
BluetoothDaemonSdpInterface::SdpSearch(
  const BluetoothAddress& aBdAddr, const BluetoothUuid& aUuid,
  BluetoothSdpResultHandler* aRes)
{
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->SdpSearchCmd(aBdAddr, aUuid, aRes);
  if (NS_FAILED(rv)) {
    DispatchError(aRes, rv);
  }
}

void
BluetoothDaemonSdpInterface::CreateSdpRecord(
  const BluetoothSdpRecord& aRecord, int& aRecordHandle,
  BluetoothSdpResultHandler* aRes)
{
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->CreateSdpRecordCmd(aRecord, aRecordHandle, aRes);
  if (NS_FAILED(rv)) {
    DispatchError(aRes, rv);
  }
}

void
BluetoothDaemonSdpInterface::RemoveSdpRecord(
  int aSdpHandle, BluetoothSdpResultHandler* aRes)
{
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->RemoveSdpRecordCmd(aSdpHandle, aRes);
  if (NS_FAILED(rv)) {
    DispatchError(aRes, rv);
  }
}

void
BluetoothDaemonSdpInterface::DispatchError(
  BluetoothSdpResultHandler* aRes, BluetoothStatus aStatus)
{
  DaemonResultRunnable1<BluetoothSdpResultHandler, void,
                        BluetoothStatus, BluetoothStatus>::Dispatch(
    aRes, &BluetoothSdpResultHandler::OnError,
    ConstantInitOp1<BluetoothStatus>(aStatus));
}

void
BluetoothDaemonSdpInterface::DispatchError(
  BluetoothSdpResultHandler* aRes, nsresult aRv)
{
  BluetoothStatus status;

  if (NS_WARN_IF(NS_FAILED(Convert(aRv, status)))) {
    status = STATUS_FAIL;
  }
  DispatchError(aRes, status);
}

END_BLUETOOTH_NAMESPACE
