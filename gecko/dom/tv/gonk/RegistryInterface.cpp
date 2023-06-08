/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "RegistryInterface.h"

#include "mozilla/ipc/DaemonSocketPDUHelpers.h"
#include "TVLog.h"

namespace mozilla {
namespace dom {

using mozilla::dom::TVStatus;
using mozilla::ipc::DaemonSocketPDUHelpers::ConstantInitOp1;
using mozilla::ipc::DaemonSocketPDUHelpers::PackPDU;
using mozilla::ipc::DaemonSocketPDUHelpers::UnpackPDUInitOp;

//
// RegistryResultHandler
//

void
RegistryResultHandler::OnError(TVStatus aError)
{
  TV_LOG("Received error code %d", static_cast<int>(aError));
}

void
RegistryResultHandler::RegisterModule() {}

void
RegistryResultHandler::UnregisterModule() {}

RegistryResultHandler::~RegistryResultHandler() {}

//
// RegistryModule
//

RegistryModule::~RegistryModule() {}

void
RegistryModule::HandleSvc(const DaemonSocketPDUHeader& aHeader,
                          DaemonSocketPDU& aPDU,
                          DaemonSocketResultHandler* aRes)
{
  static void (RegistryModule::*const HandleRsp[])(
    const DaemonSocketPDUHeader&, DaemonSocketPDU&,
    RegistryResultHandler*) = {[OPCODE_ERROR] = &RegistryModule::ErrorRsp,
                               [OPCODE_REGISTER_MODULE] =
                                 &RegistryModule::RegisterModuleRsp,
                               [OPCODE_UNREGISTER_MODULE] =
                                 &RegistryModule::UnregisterModuleRsp };

  if ((aHeader.mOpcode >= MOZ_ARRAY_LENGTH(HandleRsp)) ||
      !HandleRsp[aHeader.mOpcode]) {
    TV_LOG("TV registry response opcode %d unknown", aHeader.mOpcode);
    return;
  }

  RefPtr<RegistryResultHandler> res = static_cast<RegistryResultHandler*>(aRes);

  if (!res) {
    return; // Return early if no result handler has been set
  }

  (this->*(HandleRsp[aHeader.mOpcode]))(aHeader, aPDU, res);
}

//
// Commands
//

nsresult
RegistryModule::RegisterModuleCmd(uint8_t aId, RegistryResultHandler* aRes)
{
  MOZ_ASSERT(NS_IsMainThread());

  UniquePtr<DaemonSocketPDU> pdu =
    MakeUnique<DaemonSocketPDU>(SERVICE_ID, OPCODE_REGISTER_MODULE, 0);

  nsresult rv = PackPDU(aId, *pdu);
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
RegistryModule::UnregisterModuleCmd(uint8_t aId, RegistryResultHandler* aRes)
{
  MOZ_ASSERT(NS_IsMainThread());

  UniquePtr<DaemonSocketPDU> pdu =
    MakeUnique<DaemonSocketPDU>(SERVICE_ID, OPCODE_UNREGISTER_MODULE, 0);

  nsresult rv = PackPDU(aId, *pdu);
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

//
// Responses
//

void
RegistryModule::ErrorRsp(const DaemonSocketPDUHeader& aHeader,
                         DaemonSocketPDU& aPDU, RegistryResultHandler* aRes)
{
  ErrorRunnable::Dispatch(aRes, &RegistryResultHandler::OnError,
                          UnpackPDUInitOp(aPDU));
}

void
RegistryModule::RegisterModuleRsp(const DaemonSocketPDUHeader& aHeader,
                                  DaemonSocketPDU& aPDU,
                                  RegistryResultHandler* aRes)
{
  ResultRunnable::Dispatch(aRes, &RegistryResultHandler::RegisterModule,
                           UnpackPDUInitOp(aPDU));
}

void
RegistryModule::UnregisterModuleRsp(const DaemonSocketPDUHeader& aHeader,
                                    DaemonSocketPDU& aPDU,
                                    RegistryResultHandler* aRes)
{
  ResultRunnable::Dispatch(aRes, &RegistryResultHandler::UnregisterModule,
                           UnpackPDUInitOp(aPDU));
}

//
// RegistryInterface
//

RegistryInterface::RegistryInterface(already_AddRefed<RegistryModule> aModule)
  : mModule(aModule)
{
}

RegistryInterface::~RegistryInterface() {}

void
RegistryInterface::RegisterModule(uint8_t aId, RegistryResultHandler* aRes)
{
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->RegisterModuleCmd(aId, aRes);
  if (NS_FAILED(rv)) {
    DispatchError(aRes, rv);
  }
}

void
RegistryInterface::UnregisterModule(uint8_t aId, RegistryResultHandler* aRes)
{
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->UnregisterModuleCmd(aId, aRes);
  if (NS_FAILED(rv)) {
    DispatchError(aRes, rv);
  }
}

void
RegistryInterface::DispatchError(RegistryResultHandler* aRes, TVStatus aError)
{
  ErrorRunnable::Dispatch(aRes, &RegistryResultHandler::OnError,
                          ConstantInitOp1<TVStatus>(aError));
}

void
RegistryInterface::DispatchError(RegistryResultHandler* aRes, nsresult aRv)
{
  TVStatus status;

  if (NS_WARN_IF(NS_FAILED(Convert(aRv, status)))) {
    status = TVStatus::STATUS_FAILURE;
  }

  DispatchError(aRes, status);
}

} // namespace dom
} // namespace mozilla
