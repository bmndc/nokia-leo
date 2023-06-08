/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "DTVInterface.h"

#include "mozilla/dom/TVServiceRunnables.h"
#include "mozilla/ipc/DaemonRunnables.h"
#include "mozilla/layers/GonkNativeHandle.h"
#include "nsIMutableArray.h"
#include "TVLog.h"

namespace mozilla {
namespace dom {

using mozilla::ipc::DaemonSocketPDUHelpers::UnpackPDU;
using mozilla::ipc::DaemonSocketPDUHelpers::UnpackPDUInitOp;
using mozilla::ipc::DaemonSocketPDUHelpers::UnpackString0;
using mozilla::ipc::DaemonSocketPDUHelpers::PackConversion;
using mozilla::ipc::DaemonSocketPDUHelpers::PackCString0;
using mozilla::ipc::DaemonSocketPDUHelpers::ConstantInitOp1;

//
// DTVResultHandler
//

DTVResultHandler::DTVResultHandler(nsITVServiceCallback* aCallback)
  : mCallback(aCallback)
{
}

void
DTVResultHandler::OnError(TVStatus aStatus)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!mCallback) {
    return;
  }

  uint16_t errorCode;
  switch (aStatus) {
    case TVStatus::STATUS_INVALID_ARG:
      errorCode = nsITVServiceCallback::TV_ERROR_INVALID_ARG;
      break;
    case TVStatus::STATUS_NO_SIGNAL:
      errorCode = nsITVServiceCallback::TV_ERROR_NO_SIGNAL;
      break;
    case TVStatus::STATUS_NOT_SUPPORTED:
      errorCode = nsITVServiceCallback::TV_ERROR_NOT_SUPPORTED;
      break;
    default:
      errorCode = nsITVServiceCallback::TV_ERROR_FAILURE;
      break;
  }

  RefPtr<nsIRunnable> runnable =
    new TVServiceNotifyRunnable(mCallback, nullptr, errorCode);
  NS_WARN_IF(NS_FAILED(NS_DispatchToCurrentThread(runnable)));
}

void
DTVResultHandler::OnGetTunersResponse(
  const nsTArray<struct TunerData>& aTunerMsgDataList)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!mCallback) {
    return;
  }

  nsCOMPtr<nsIMutableArray> tunerDataList =
    do_CreateInstance(NS_ARRAY_CONTRACTID);
  if (NS_WARN_IF(!tunerDataList)) {
    return;
  }

  for (uint32_t i = 0; i < aTunerMsgDataList.Length(); i++) {
    nsCOMPtr<nsITVTunerData> tunerData;
    tunerData = do_CreateInstance(TV_TUNER_DATA_CONTRACTID);
    PackTunerData(tunerData, aTunerMsgDataList[i]);
    tunerDataList->AppendElement(tunerData, false);
  }

  RefPtr<nsIRunnable> runnable =
    new TVServiceNotifyRunnable(mCallback, tunerDataList);
  NS_WARN_IF(NS_FAILED(NS_DispatchToCurrentThread(runnable)));
}

void
DTVResultHandler::OnSetSourceResponse(native_handle_t* aHandle)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!mCallback) {
    return;
  }

  GonkNativeHandle handle(new GonkNativeHandle::NhObj(aHandle));

  nsCOMPtr<nsITVGonkNativeHandleData> handleData =
    do_CreateInstance(TV_GONK_NATIVE_HANDLE_DATA_CONTRACTID);
  handleData->SetHandle(handle);

  nsCOMPtr<nsIMutableArray> handleDataList =
    do_CreateInstance(NS_ARRAY_CONTRACTID);
  if (NS_WARN_IF(!handleDataList)) {
    return;
  }
  handleDataList->AppendElement(handleData, false);

  RefPtr<nsIRunnable> runnable =
    new TVServiceNotifyRunnable(mCallback, handleDataList);
  NS_WARN_IF(NS_FAILED(NS_DispatchToCurrentThread(runnable)));
}

void
DTVResultHandler::OnStartScanningChannelsResponse()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!mCallback) {
    return;
  }

  RefPtr<nsIRunnable> runnable =
    new TVServiceNotifyRunnable(mCallback, nullptr);
  NS_WARN_IF(NS_FAILED(NS_DispatchToCurrentThread(runnable)));
}

void
DTVResultHandler::OnStopScanningChannelsResponse()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!mCallback) {
    return;
  }

  RefPtr<nsIRunnable> runnable =
    new TVServiceNotifyRunnable(mCallback, nullptr);
  NS_WARN_IF(NS_FAILED(NS_DispatchToCurrentThread(runnable)));
}

void
DTVResultHandler::OnSetChannelResponse(struct ChannelData aChannelMsgData)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!mCallback) {
    return;
  }

  nsCOMPtr<nsIMutableArray> channelDataList =
    do_CreateInstance(NS_ARRAY_CONTRACTID);
  if (NS_WARN_IF(!channelDataList)) {
    return;
  }

  nsCOMPtr<nsITVChannelData> channelData;
  channelData = do_CreateInstance(TV_CHANNEL_DATA_CONTRACTID);
  PackChannelData(channelData, aChannelMsgData);
  channelDataList->AppendElement(channelData.get(), false);

  RefPtr<nsIRunnable> runnable =
    new TVServiceNotifyRunnable(mCallback, channelDataList);
  NS_WARN_IF(NS_FAILED(NS_DispatchToCurrentThread(runnable)));
}

void
DTVResultHandler::OnGetChannelsResponse(
  const nsTArray<struct ChannelData>& aChannelMsgDataList)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!mCallback) {
    return;
  }

  nsCOMPtr<nsIMutableArray> channelDataList =
    do_CreateInstance(NS_ARRAY_CONTRACTID);
  if (NS_WARN_IF(!channelDataList)) {
    return;
  }

  for (uint32_t i = 0; i < aChannelMsgDataList.Length(); i++) {
    nsCOMPtr<nsITVChannelData> channelData;
    channelData = do_CreateInstance(TV_CHANNEL_DATA_CONTRACTID);
    PackChannelData(channelData, aChannelMsgDataList[i]);
    channelDataList->AppendElement(channelData, false);
  }

  RefPtr<nsIRunnable> runnable =
    new TVServiceNotifyRunnable(mCallback, channelDataList);
  NS_WARN_IF(NS_FAILED(NS_DispatchToCurrentThread(runnable)));
}

void
DTVResultHandler::OnGetProgramsResponse(
  const nsTArray<struct ProgramData>& aProgramMsgDataList)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!mCallback) {
    return;
  }

  nsCOMPtr<nsIMutableArray> programDataList =
    do_CreateInstance(NS_ARRAY_CONTRACTID);
  if (NS_WARN_IF(!programDataList)) {
    return;
  }

  for (uint32_t i = 0; i < aProgramMsgDataList.Length(); i++) {
    nsCOMPtr<nsITVProgramData> programData;
    programData = do_CreateInstance(TV_PROGRAM_DATA_CONTRACTID);
    PackProgramData(programData, aProgramMsgDataList[i]);
    programDataList->AppendElement(programData, false);
  }

  RefPtr<nsIRunnable> runnable =
    new TVServiceNotifyRunnable(mCallback, programDataList);
  NS_WARN_IF(NS_FAILED(NS_DispatchToCurrentThread(runnable)));
}

//
// DTVNotificationHandler
//

void
DTVNotificationHandler::OnError(TVStatus& aError)
{
  TV_LOG("Received error code %d", static_cast<int>(aError));
}

void
DTVNotificationHandler::OnChannelScanned(const nsAString& aTunerId,
                                         const nsAString& aSourceType,
                                         struct ChannelData aChannelMsgData)
{
}

void
DTVNotificationHandler::OnChannelScanComplete(const nsAString& aTunerId,
                                              const nsAString& aSourceType)
{
}

void
DTVNotificationHandler::OnChannelScanStopped(const nsAString& aTunerId,
                                             const nsAString& aSourceType)
{
}

void
DTVNotificationHandler::OnEITBroadcasted(
  const nsAString& aTunerId, const nsAString& aSourceType,
  struct ChannelData aChannelMsgData,
  const nsTArray<struct ProgramData>& aProgramMsgDataList)
{
}

DTVNotificationHandler::~DTVNotificationHandler() {}

//
// DTVModule
//

DTVModule::DTVModule() {}

DTVModule::~DTVModule() {}

void
DTVModule::HandleSvc(const DaemonSocketPDUHeader& aHeader,
                     DaemonSocketPDU& aPDU, DaemonSocketResultHandler* aRes)
{
  static void (DTVModule::*const HandleOp[])(
    const DaemonSocketPDUHeader&, DaemonSocketPDU&,
    DaemonSocketResultHandler*) = {[0] = &DTVModule::HandleRsp,
                                   [1] = &DTVModule::HandleNtf };

  MOZ_ASSERT(!NS_IsMainThread()); // I/O thread

  // Negate twice to map bit to 0/1
  unsigned long isNtf = !!(aHeader.mOpcode & OPCODE_NTF_FILTER);

  (this->*(HandleOp[isNtf]))(aHeader, aPDU, aRes);
}

nsresult
DTVModule::UnpackTVDataInitOp::
operator()(nsTArray<struct TunerData>& aArg1) const
{
  DaemonSocketPDU& pdu = GetPDU();

  // Read number of tuners.
  uint32_t count;
  nsresult rv = UnpackPDU(pdu, count);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  // Read tuners.
  aArg1.SetLength(count);
  for (uint32_t i = 0; i < count; i++) {
    rv = UnpackPDU(pdu, aArg1[i]);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  WarnAboutTrailingData();
  return NS_OK;
}

nsresult
DTVModule::UnpackTVDataInitOp::
operator()(struct ChannelData& aArg1) const
{
  DaemonSocketPDU& pdu = GetPDU();

  nsresult rv = UnpackPDU(pdu, aArg1);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  WarnAboutTrailingData();
  return NS_OK;
}

nsresult
DTVModule::UnpackTVDataInitOp::
operator()(nsTArray<struct ChannelData>& aArg1) const
{
  DaemonSocketPDU& pdu = GetPDU();

  // Read number of channels.
  uint32_t count;
  nsresult rv = UnpackPDU(pdu, count);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  // Read channels.
  aArg1.SetLength(count);
  for (uint32_t i = 0; i < count; i++) {
    rv = UnpackPDU(pdu, aArg1[i]);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  WarnAboutTrailingData();
  return NS_OK;
}

nsresult
DTVModule::UnpackTVDataInitOp::
operator()(nsTArray<struct ProgramData>& aArg1) const
{
  DaemonSocketPDU& pdu = GetPDU();

  // Read number of programs.
  uint32_t count;
  nsresult rv = UnpackPDU(pdu, count);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  // Read programs.
  aArg1.SetLength(count);
  for (uint32_t i = 0; i < count; i++) {
    rv = UnpackPDU(pdu, aArg1[i]);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  WarnAboutTrailingData();
  return NS_OK;
}

nsresult
DTVModule::UnpackTVDataInitOp::
operator()(native_handle_t*& aArg1) const
{
  DaemonSocketPDU& pdu = GetPDU();

  // Read native handle.
  nsresult rv = UnpackPDU(pdu, aArg1);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  WarnAboutTrailingData();
  return NS_OK;
}

nsresult
DTVModule::UnpackTVNotificationInitOp::
operator()(TVStatus& aArg1) const
{
  DaemonSocketPDU& pdu = GetPDU();

  // Read error message.
  nsresult rv = UnpackPDU(pdu, aArg1);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  WarnAboutTrailingData();
  return NS_OK;
}

nsresult
DTVModule::UnpackTVNotificationInitOp::
operator()(nsString& aArg1, nsString& aArg2) const
{
  DaemonSocketPDU& pdu = GetPDU();

  // Read tuner ID.
  nsresult rv = UnpackPDU(pdu, UnpackString0(aArg1));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  // Read source type.
  uint8_t sourceType;
  rv = UnpackPDU(pdu, sourceType);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  aArg2 = ToTVSourceTypeStr(static_cast<TVSourceType>(sourceType));

  WarnAboutTrailingData();
  return NS_OK;
}

nsresult
DTVModule::UnpackTVNotificationInitOp::
operator()(nsString& aArg1, nsString& aArg2, struct ChannelData& aArg3) const
{
  DaemonSocketPDU& pdu = GetPDU();

  // Read tuner ID.
  nsresult rv = UnpackPDU(pdu, UnpackString0(aArg1));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  // Read source type.
  uint8_t sourceType;
  rv = UnpackPDU(pdu, sourceType);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  aArg2 = ToTVSourceTypeStr(static_cast<TVSourceType>(sourceType));

  // Read channel.
  rv = UnpackPDU(pdu, aArg3);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  WarnAboutTrailingData();
  return NS_OK;
}

nsresult
DTVModule::UnpackTVNotificationInitOp::
operator()(nsString& aArg1, nsString& aArg2, struct ChannelData& aArg3,
           nsTArray<struct ProgramData>& aArg4) const
{
  DaemonSocketPDU& pdu = GetPDU();

  // Read tuner ID.
  nsresult rv = UnpackPDU(pdu, UnpackString0(aArg1));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  // Read source type.
  uint8_t sourceType;
  rv = UnpackPDU(pdu, sourceType);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  aArg2 = ToTVSourceTypeStr(static_cast<TVSourceType>(sourceType));

  // Read channel.
  rv = UnpackPDU(pdu, aArg3);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  // Read number of programs.
  uint32_t count;
  rv = UnpackPDU(pdu, count);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  // Read programs.
  aArg4.SetLength(count);
  for (uint32_t i = 0; i < count; i++) {
    rv = UnpackPDU(pdu, aArg4[i]);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  WarnAboutTrailingData();
  return NS_OK;
}

// Commands
//

nsresult
DTVModule::GetTunersCommand(DTVResultHandler* aResultHandler)
{
  MOZ_ASSERT(NS_IsMainThread());

  UniquePtr<DaemonSocketPDU> pdu =
    MakeUnique<DaemonSocketPDU>(SERVICE_ID, OPCODE_GET_TUNERS, 0);

  nsresult rv = Send(pdu.get(), aResultHandler);
  if (NS_FAILED(rv)) {
    return rv;
  }
  Unused << pdu.release();
  return NS_OK;
}

nsresult
DTVModule::SetSourceCommand(const nsAString& aTunerId,
                            const nsAString& aSourceType,
                            DTVResultHandler* aResultHandler)
{
  MOZ_ASSERT(NS_IsMainThread());

  NS_ConvertUTF16toUTF8 tunerId(aTunerId);
  uint16_t payloadSize = tunerId.Length() + 1 + // Tuner ID string + '\0'
                         sizeof(uint8_t);       // Source type

  UniquePtr<DaemonSocketPDU> pdu =
    MakeUnique<DaemonSocketPDU>(SERVICE_ID, OPCODE_SET_SOURCE, payloadSize);

  nsresult rv;
  rv = PackPDU(
    PackCString0(tunerId),
    PackConversion<TVSourceType, uint8_t>(ToTVSourceType(aSourceType)), *pdu);

  if (NS_FAILED(rv)) {
    return rv;
  }

  rv = Send(pdu.get(), aResultHandler);
  if (NS_FAILED(rv)) {
    return rv;
  }
  Unused << pdu.release();
  return NS_OK;
}

nsresult
DTVModule::StartScanningChannelsCommand(const nsAString& aTunerId,
                                        const nsAString& aSourceType,
                                        DTVResultHandler* aResultHandler)
{
  MOZ_ASSERT(NS_IsMainThread());

  NS_ConvertUTF16toUTF8 tunerId(aTunerId);
  uint16_t payloadSize = tunerId.Length() + 1 + // Tuner ID string + '\0'
                         sizeof(uint8_t);       // Source type

  UniquePtr<DaemonSocketPDU> pdu = MakeUnique<DaemonSocketPDU>(
    SERVICE_ID, OPCODE_START_SCANNING_CHANNELS, payloadSize);

  nsresult rv;
  rv = PackPDU(
    PackCString0(tunerId),
    PackConversion<TVSourceType, uint8_t>(ToTVSourceType(aSourceType)), *pdu);

  if (NS_FAILED(rv)) {
    return rv;
  }

  rv = Send(pdu.get(), aResultHandler);
  if (NS_FAILED(rv)) {
    return rv;
  }
  Unused << pdu.release();
  return NS_OK;
}

nsresult
DTVModule::StopScanningChannelsCommand(const nsAString& aTunerId,
                                       const nsAString& aSourceType,
                                       DTVResultHandler* aResultHandler)
{
  MOZ_ASSERT(NS_IsMainThread());

  NS_ConvertUTF16toUTF8 tunerId(aTunerId);
  uint16_t payloadSize = tunerId.Length() + 1 + // Tuner ID string + '\0'
                         sizeof(uint8_t);       // Source type

  UniquePtr<DaemonSocketPDU> pdu = MakeUnique<DaemonSocketPDU>(
    SERVICE_ID, OPCODE_STOP_SCANNING_CHANNELS, payloadSize);

  nsresult rv;
  rv = PackPDU(
    PackCString0(tunerId),
    PackConversion<TVSourceType, uint8_t>(ToTVSourceType(aSourceType)), *pdu);

  if (NS_FAILED(rv)) {
    return rv;
  }

  rv = Send(pdu.get(), aResultHandler);
  if (NS_FAILED(rv)) {
    return rv;
  }
  Unused << pdu.release();
  return NS_OK;
}

nsresult
DTVModule::ClearScannedChannelsCacheCommand(DTVResultHandler* aResultHandler)
{
  MOZ_ASSERT(NS_IsMainThread());

  UniquePtr<DaemonSocketPDU> pdu = MakeUnique<DaemonSocketPDU>(
    SERVICE_ID, OPCODE_CLEAR_SCANNED_CHANNELS_CACHE, 0);

  nsresult rv = Send(pdu.get(), aResultHandler);
  if (NS_FAILED(rv)) {
    return rv;
  }
  Unused << pdu.release();
  return NS_OK;
}

nsresult
DTVModule::SetChannelCommand(const nsAString& aTunerId,
                             const nsAString& aSourceType,
                             const nsAString& aChannelNumber,
                             DTVResultHandler* aResultHandler)
{
  MOZ_ASSERT(NS_IsMainThread());

  NS_ConvertUTF16toUTF8 tunerId(aTunerId);
  NS_ConvertUTF16toUTF8 channelNumber(aChannelNumber);
  uint16_t payloadSize = tunerId.Length() + 1 + // Tuner ID string + '\0'
                         sizeof(uint8_t) +      // Source type
                         channelNumber.Length() +
                         1; // Channel number string + '\0'

  UniquePtr<DaemonSocketPDU> pdu =
    MakeUnique<DaemonSocketPDU>(SERVICE_ID, OPCODE_SET_CHANNEL, payloadSize);

  nsresult rv;
  rv =
    PackPDU(PackCString0(tunerId),
            PackConversion<TVSourceType, uint8_t>(ToTVSourceType(aSourceType)),
            PackCString0(channelNumber), *pdu);

  if (NS_FAILED(rv)) {
    return rv;
  }

  rv = Send(pdu.get(), aResultHandler);
  if (NS_FAILED(rv)) {
    return rv;
  }
  Unused << pdu.release();
  return NS_OK;
}

nsresult
DTVModule::GetChannelsCommand(const nsAString& aTunerId,
                              const nsAString& aSourceType,
                              DTVResultHandler* aResultHandler)
{
  MOZ_ASSERT(NS_IsMainThread());

  NS_ConvertUTF16toUTF8 tunerId(aTunerId);
  uint16_t payloadSize = tunerId.Length() + 1 + // Tuner ID string + '\0'
                         sizeof(uint8_t);       // Source type

  UniquePtr<DaemonSocketPDU> pdu =
    MakeUnique<DaemonSocketPDU>(SERVICE_ID, OPCODE_GET_CHANNELS, payloadSize);

  nsresult rv;
  rv = PackPDU(
    PackCString0(tunerId),
    PackConversion<TVSourceType, uint8_t>(ToTVSourceType(aSourceType)), *pdu);

  if (NS_FAILED(rv)) {
    return rv;
  }

  rv = Send(pdu.get(), aResultHandler);
  if (NS_FAILED(rv)) {
    return rv;
  }
  Unused << pdu.release();
  return NS_OK;
}

nsresult
DTVModule::GetProgramsCommand(const nsAString& aTunerId,
                              const nsAString& aSourceType,
                              const nsAString& aChannelNumber,
                              uint64_t aStartTime, uint64_t aEndTime,
                              DTVResultHandler* aResultHandler)
{
  MOZ_ASSERT(NS_IsMainThread());

  NS_ConvertUTF16toUTF8 tunerId(aTunerId);
  NS_ConvertUTF16toUTF8 channelNumber(aChannelNumber);
  uint16_t payloadSize = tunerId.Length() + 1 + // Tuner ID string + '\0'
                         sizeof(uint8_t) +      // Source type
                         channelNumber.Length() +
                         1 +                // Channel number string + '\0'
                         sizeof(uint64_t) + // Start time
                         sizeof(uint64_t);  // End time

  UniquePtr<DaemonSocketPDU> pdu =
    MakeUnique<DaemonSocketPDU>(SERVICE_ID, OPCODE_GET_PROGRAMS, payloadSize);

  nsresult rv =
    PackPDU(PackCString0(tunerId),
            PackConversion<TVSourceType, uint8_t>(ToTVSourceType(aSourceType)),
            PackCString0(channelNumber), aStartTime, aEndTime, *pdu);
  if (NS_FAILED(rv)) {
    return rv;
  }

  rv = Send(pdu.get(), aResultHandler);
  if (NS_FAILED(rv)) {
    return rv;
  }
  Unused << pdu.release();
  return NS_OK;
}

// Responses
//
void
DTVModule::ErrorResponse(const DaemonSocketPDUHeader& aHeader,
                         DaemonSocketPDU& aPDU,
                         DTVResultHandler* aResultHandler)
{
  TVErrorRunnable::Dispatch(aResultHandler, &DTVResultHandler::OnError,
                            UnpackPDUInitOp(aPDU));
}

void
DTVModule::GetTunersResponse(const DaemonSocketPDUHeader& aHeader,
                             DaemonSocketPDU& aPDU,
                             DTVResultHandler* aResultHandler)
{
  GetTunersResultRunnable::Dispatch(aResultHandler,
                                    &DTVResultHandler::OnGetTunersResponse,
                                    UnpackTVDataInitOp(aPDU));
}

void
DTVModule::SetSourceResponse(const DaemonSocketPDUHeader& aHeader,
                             DaemonSocketPDU& aPDU,
                             DTVResultHandler* aResultHandler)
{
  SetSourceResultRunnable::Dispatch(aResultHandler,
                                    &DTVResultHandler::OnSetSourceResponse,
                                    UnpackTVDataInitOp(aPDU));
}

void
DTVModule::StartScanningChannelsResponse(const DaemonSocketPDUHeader& aHeader,
                                         DaemonSocketPDU& aPDU,
                                         DTVResultHandler* aResultHandler)
{
  TVResultRunnable::Dispatch(aResultHandler,
                             &DTVResultHandler::OnStartScanningChannelsResponse,
                             UnpackPDUInitOp(aPDU));
}

void
DTVModule::StopScanningChannelsResponse(const DaemonSocketPDUHeader& aHeader,
                                        DaemonSocketPDU& aPDU,
                                        DTVResultHandler* aResultHandler)
{
  TVResultRunnable::Dispatch(aResultHandler,
                             &DTVResultHandler::OnStopScanningChannelsResponse,
                             UnpackPDUInitOp(aPDU));
}

void
DTVModule::ClearScannedChannelsCacheResponse(
  const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
  DTVResultHandler* aResultHandler)
{
  // Nothing to do.
}

void
DTVModule::SetChannelResponse(const DaemonSocketPDUHeader& aHeader,
                              DaemonSocketPDU& aPDU,
                              DTVResultHandler* aResultHandler)
{
  SetChannelResultRunnable::Dispatch(aResultHandler,
                                     &DTVResultHandler::OnSetChannelResponse,
                                     UnpackTVDataInitOp(aPDU));
}

void
DTVModule::GetChannelsResponse(const DaemonSocketPDUHeader& aHeader,
                               DaemonSocketPDU& aPDU,
                               DTVResultHandler* aResultHandler)
{
  GetChannelsResultRunnable::Dispatch(aResultHandler,
                                      &DTVResultHandler::OnGetChannelsResponse,
                                      UnpackTVDataInitOp(aPDU));
}

void
DTVModule::GetProgramsResponse(const DaemonSocketPDUHeader& aHeader,
                               DaemonSocketPDU& aPDU,
                               DTVResultHandler* aResultHandler)
{
  GetProgramsResultRunnable::Dispatch(aResultHandler,
                                      &DTVResultHandler::OnGetProgramsResponse,
                                      UnpackTVDataInitOp(aPDU));
}

void
DTVModule::HandleRsp(const DaemonSocketPDUHeader& aHeader,
                     DaemonSocketPDU& aPDU, DaemonSocketResultHandler* aRes)
{
  static void (DTVModule::*const sHandleRsp[])(
    const DaemonSocketPDUHeader&, DaemonSocketPDU&, DTVResultHandler*) =
    {[OPCODE_ERROR] = &DTVModule::ErrorResponse,
     [OPCODE_GET_TUNERS] = &DTVModule::GetTunersResponse,
     [OPCODE_SET_SOURCE] = &DTVModule::SetSourceResponse,
     [OPCODE_START_SCANNING_CHANNELS] =
       &DTVModule::StartScanningChannelsResponse,
     [OPCODE_STOP_SCANNING_CHANNELS] = &DTVModule::StopScanningChannelsResponse,
     [OPCODE_CLEAR_SCANNED_CHANNELS_CACHE] =
       &DTVModule::ClearScannedChannelsCacheResponse,
     [OPCODE_SET_CHANNEL] = &DTVModule::SetChannelResponse,
     [OPCODE_GET_CHANNELS] = &DTVModule::GetChannelsResponse,
     [OPCODE_GET_PROGRAMS] = &DTVModule::GetProgramsResponse, };

  MOZ_ASSERT(!NS_IsMainThread()); // I/O thread

  if (!(aHeader.mOpcode < MOZ_ARRAY_LENGTH(sHandleRsp)) ||
      !sHandleRsp[aHeader.mOpcode]) {
    TV_LOG("TV DTV response opcode %d unknown", aHeader.mOpcode);
    return;
  }

  RefPtr<DTVResultHandler> res = static_cast<DTVResultHandler*>(aRes);

  if (!res) {
    printf_stderr("no response");
    return; // Return early if no result handler has been set for response
  }

  (this->*(sHandleRsp[aHeader.mOpcode]))(aHeader, aPDU, res);
}

// Notifications
//

// Returns the current notification handler to a notification runnable
class DTVModule::NotificationHandlerWrapper final
{
public:
  typedef DTVNotificationHandler ObjectType;

  static ObjectType* GetInstance()
  {
    MOZ_ASSERT(NS_IsMainThread());
    return sNotificationHandler.get();
  }

  static RefPtr<DTVNotificationHandler> sNotificationHandler;
};

RefPtr<DTVNotificationHandler>
DTVModule::NotificationHandlerWrapper::sNotificationHandler;

void
DTVModule::ErrorNotification(const DaemonSocketPDUHeader& aHeader,
                             DaemonSocketPDU& aPDU)
{
  ErrorNotificationRunnable::Dispatch(&DTVNotificationHandler::OnError,
                                      UnpackTVNotificationInitOp(aPDU));
}

void
DTVModule::ChannelScannedNotification(const DaemonSocketPDUHeader& aHeader,
                                      DaemonSocketPDU& aPDU)
{
  ScannedChannelNotificationRunnable::Dispatch(
    &DTVNotificationHandler::OnChannelScanned,
    UnpackTVNotificationInitOp(aPDU));
}

void
DTVModule::ChannelScanCompleteNotification(const DaemonSocketPDUHeader& aHeader,
                                           DaemonSocketPDU& aPDU)
{
  ChannelScanNotificationRunnable::Dispatch(
    &DTVNotificationHandler::OnChannelScanComplete,
    UnpackTVNotificationInitOp(aPDU));
}

void
DTVModule::ChannelScanStoppedNotification(const DaemonSocketPDUHeader& aHeader,
                                          DaemonSocketPDU& aPDU)
{
  ChannelScanNotificationRunnable::Dispatch(
    &DTVNotificationHandler::OnChannelScanStopped,
    UnpackTVNotificationInitOp(aPDU));
}

void
DTVModule::EITBroadcastedNotification(const DaemonSocketPDUHeader& aHeader,
                                      DaemonSocketPDU& aPDU)
{
  EITBroadcastedNotificationRunnable::Dispatch(
    &DTVNotificationHandler::OnEITBroadcasted,
    UnpackTVNotificationInitOp(aPDU));
}

void
DTVModule::HandleNtf(const DaemonSocketPDUHeader& aHeader,
                     DaemonSocketPDU& aPDU, DaemonSocketResultHandler* aRes)
{
  static void (DTVModule::*const sHandleNtf[])(
    const DaemonSocketPDUHeader&,
    DaemonSocketPDU&) = {[0] = &DTVModule::ErrorNotification,
                         [1] = &DTVModule::ChannelScannedNotification,
                         [2] = &DTVModule::ChannelScanCompleteNotification,
                         [3] = &DTVModule::ChannelScanStoppedNotification,
                         [4] = &DTVModule::EITBroadcastedNotification };

  MOZ_ASSERT(!NS_IsMainThread());

  uint8_t index = aHeader.mOpcode - OPCODE_NTF_FILTER;

  if (!(index < MOZ_ARRAY_LENGTH(sHandleNtf)) || !sHandleNtf[index]) {
    TV_LOG("TV DTV response opcode %d unknown", aHeader.mOpcode);
    return;
  }

  (this->*(sHandleNtf[index]))(aHeader, aPDU);
}

//
// DTVInterface
//

DTVInterface::DTVInterface(already_AddRefed<DTVModule> aModule)
  : mModule(aModule) {}

DTVInterface::~DTVInterface() {}

void
DTVInterface::SetNotificationHandler(
  DTVNotificationHandler* aNotificationHandler)
{
  MOZ_ASSERT(NS_IsMainThread());

  DTVModule::NotificationHandlerWrapper::sNotificationHandler =
    aNotificationHandler;
}

void
DTVInterface::GetTuners(DTVResultHandler* aResultHandler)
{
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->GetTunersCommand(aResultHandler);
  if (NS_FAILED(rv)) {
    DispatchError(aResultHandler, rv);
  }
}

void
DTVInterface::SetSource(const nsAString& aTunerId, const nsAString& aSourceType,
                        DTVResultHandler* aResultHandler)
{
  MOZ_ASSERT(mModule);

  nsresult rv =
    mModule->SetSourceCommand(aTunerId, aSourceType, aResultHandler);
  if (NS_FAILED(rv)) {
    DispatchError(aResultHandler, rv);
  }
}

void
DTVInterface::StartScanningChannels(const nsAString& aTunerId,
                                    const nsAString& aSourceType,
                                    DTVResultHandler* aResultHandler)
{
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->StartScanningChannelsCommand(aTunerId, aSourceType,
                                                      aResultHandler);
  if (NS_FAILED(rv)) {
    DispatchError(aResultHandler, rv);
  }
}

void
DTVInterface::StopScanningChannels(const nsAString& aTunerId,
                                   const nsAString& aSourceType,
                                   DTVResultHandler* aResultHandler)
{
  MOZ_ASSERT(mModule);

  nsresult rv =
    mModule->StopScanningChannelsCommand(aTunerId, aSourceType, aResultHandler);
  if (NS_FAILED(rv)) {
    DispatchError(aResultHandler, rv);
  }
}

void
DTVInterface::ClearScannedChannelsCache(DTVResultHandler* aResultHandler)
{
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->ClearScannedChannelsCacheCommand(aResultHandler);
  if (NS_FAILED(rv)) {
    DispatchError(aResultHandler, rv);
  }
}

void
DTVInterface::SetChannel(const nsAString& aTunerId,
                         const nsAString& aSourceType,
                         const nsAString& aChannelNumber,
                         DTVResultHandler* aResultHandler)
{
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->SetChannelCommand(aTunerId, aSourceType,
                                           aChannelNumber, aResultHandler);
  if (NS_FAILED(rv)) {
    DispatchError(aResultHandler, rv);
  }
}

void
DTVInterface::GetChannels(const nsAString& aTunerId,
                          const nsAString& aSourceType,
                          DTVResultHandler* aResultHandler)
{
  MOZ_ASSERT(mModule);

  nsresult rv =
    mModule->GetChannelsCommand(aTunerId, aSourceType, aResultHandler);
  if (NS_FAILED(rv)) {
    DispatchError(aResultHandler, rv);
  }
}

void
DTVInterface::GetPrograms(const nsAString& aTunerId,
                          const nsAString& aSourceType,
                          const nsAString& aChannelNumber, uint64_t startTime,
                          uint64_t endTime, DTVResultHandler* aResultHandler)
{
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->GetProgramsCommand(
    aTunerId, aSourceType, aChannelNumber, startTime, endTime, aResultHandler);
  if (NS_FAILED(rv)) {
    DispatchError(aResultHandler, rv);
  }
}

void
DTVInterface::DispatchError(DTVResultHandler* aRes, TVStatus aError)
{
  TVErrorRunnable::Dispatch(aRes, &DTVResultHandler::OnError,
                            ConstantInitOp1<TVStatus>(aError));
}

void
DTVInterface::DispatchError(DTVResultHandler* aRes, nsresult aRv)
{
  TVStatus status;

  if (NS_SUCCEEDED(aRv)) {
    status = TVStatus::STATUS_OK;
  } else if (aRv == NS_ERROR_INVALID_ARG) {
    status = TVStatus::STATUS_INVALID_ARG;
  } else if (aRv == NS_ERROR_NOT_AVAILABLE) {
    status = TVStatus::STATUS_NO_SIGNAL;
  } else if (aRv == NS_ERROR_NOT_IMPLEMENTED) {
    status = TVStatus::STATUS_NOT_SUPPORTED;
  } else {
    status = TVStatus::STATUS_FAILURE;
  }

  DispatchError(aRes, status);
}

} // namespace dom
} // namespace mozilla
