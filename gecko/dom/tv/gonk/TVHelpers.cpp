/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "TVHelpers.h"

#include "mozilla/dom/TVUtils.h"
#include "mozilla/ipc/DaemonSocketPDUHelpers.h"

namespace mozilla {
namespace dom {

using mozilla::ipc::DaemonSocketPDUHelpers::UnpackArray;
using mozilla::ipc::DaemonSocketPDUHelpers::UnpackConversion;
using mozilla::ipc::DaemonSocketPDUHelpers::UnpackPDU;
using mozilla::ipc::DaemonSocketPDUHelpers::UnpackString0;

nsresult
PackTunerData(nsCOMPtr<nsITVTunerData>& aTunerData,
              const struct TunerData& aTunerMsgData)
{
  MOZ_ASSERT(aTunerData);
  MOZ_ASSERT(&aTunerMsgData);

  aTunerData->SetId(aTunerMsgData.id);
  aTunerData->SetStreamType(nsITVTunerData::TV_STREAM_TYPE_HW);
  aTunerData->SetSupportedSourceTypesByString(aTunerMsgData.sourceTypes);

  return NS_OK;
}

nsresult
PackChannelData(nsCOMPtr<nsITVChannelData>& aChannelData,
                const struct ChannelData& aChannelMsgData)
{
  MOZ_ASSERT(aChannelData);
  MOZ_ASSERT(&aChannelMsgData);

  aChannelData->SetNetworkId(aChannelMsgData.networkId);
  aChannelData->SetTransportStreamId(aChannelMsgData.transportStreamId);
  aChannelData->SetServiceId(aChannelMsgData.serviceId);

  aChannelData->SetType(
    ToTVChannelTypeStr(static_cast<TVChannelType>(aChannelMsgData.type)));

  aChannelData->SetNumber(aChannelMsgData.number);
  aChannelData->SetName(aChannelMsgData.name);
  aChannelData->SetIsEmergency(aChannelMsgData.isEmergency);
  aChannelData->SetIsFree(aChannelMsgData.isFree);

  return NS_OK;
}

nsresult
PackProgramData(nsCOMPtr<nsITVProgramData>& aProgramData,
                const struct ProgramData& aProgramMsgData)
{
  MOZ_ASSERT(aProgramData);
  MOZ_ASSERT(&aProgramMsgData);

  aProgramData->SetEventId(aProgramMsgData.eventId);
  aProgramData->SetTitle(aProgramMsgData.title);
  aProgramData->SetStartTime(aProgramMsgData.startTime);
  aProgramData->SetDuration(aProgramMsgData.duration);
  aProgramData->SetDescription(aProgramMsgData.description);
  aProgramData->SetRating(aProgramMsgData.rating);
  aProgramData->SetAudioLanguagesByString(aProgramMsgData.audioLanguages);
  aProgramData->SetSubtitleLanguagesByString(
    aProgramMsgData.subtitleLanguages);

  return NS_OK;
}

nsresult
Convert(nsresult aIn, TVStatus& aOut)
{
  if (NS_SUCCEEDED(aIn)) {
    aOut = TVStatus::STATUS_OK;
  } else if (aIn == NS_ERROR_INVALID_ARG) {
    aOut = TVStatus::STATUS_INVALID_ARG;
  } else if (aIn == NS_ERROR_NOT_AVAILABLE) {
    aOut = TVStatus::STATUS_NO_SIGNAL;
  } else if (aIn == NS_ERROR_NOT_IMPLEMENTED) {
    aOut = TVStatus::STATUS_NOT_SUPPORTED;
  } else {
    aOut = TVStatus::STATUS_FAILURE;
  }
  return NS_OK;
}

nsresult
UnpackPDU(DaemonSocketPDU& aPDU, struct TunerData& aOut)
{
  // Read tuner ID.
  nsresult rv = UnpackPDU(aPDU, UnpackString0(aOut.id));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  // Read number of supported types.
  rv = UnpackPDU(aPDU, aOut.sourceTypeNum);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  // Read supported source types.
  uint8_t sourceTypes[aOut.sourceTypeNum];
  rv = UnpackPDU(aPDU, UnpackArray<uint8_t>(sourceTypes, aOut.sourceTypeNum));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  for (uint32_t i = 0; i < aOut.sourceTypeNum; i++) {
    nsString sourceStr =
      ToTVSourceTypeStr(static_cast<TVSourceType>(sourceTypes[i]));
    aOut.sourceTypes.AppendElement(NS_ConvertUTF16toUTF8(sourceStr));
  }

  return NS_OK;
}

nsresult
UnpackPDU(DaemonSocketPDU& aPDU, struct ChannelData& aOut)
{
  // Read network ID.
  nsresult rv = UnpackPDU(aPDU, UnpackString0(aOut.networkId));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  // Read transport stream ID.
  rv = UnpackPDU(aPDU, UnpackString0(aOut.transportStreamId));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  // Read service ID.
  rv = UnpackPDU(aPDU, UnpackString0(aOut.serviceId));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  // Read channel type.
  rv = UnpackPDU(aPDU, aOut.type);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  // Read number.
  rv = UnpackPDU(aPDU, UnpackString0(aOut.number));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  // Read name.
  rv = UnpackPDU(aPDU, UnpackString0(aOut.name));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  // Read isEmergency.
  rv = UnpackPDU(aPDU, aOut.isEmergency);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  // Read isFree.
  rv = UnpackPDU(aPDU, aOut.isFree);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

nsresult
UnpackPDU(DaemonSocketPDU& aPDU, struct ProgramData& aOut)
{
  // Read event ID.
  nsresult rv = UnpackPDU(aPDU, UnpackString0(aOut.eventId));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  // Read title.
  rv = UnpackPDU(aPDU, UnpackString0(aOut.title));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  // Read start time.
  rv = UnpackPDU(aPDU, aOut.startTime);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  // Read duration.
  rv = UnpackPDU(aPDU, aOut.duration);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  // Read description.
  rv = UnpackPDU(aPDU, UnpackString0(aOut.description));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  // Read rating.
  rv = UnpackPDU(aPDU, UnpackString0(aOut.rating));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  // Read number of audio languages.
  rv = UnpackPDU(aPDU, aOut.audioLanguageCount);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  // Read audio languages.
  nsDependentCString audioLanguages[aOut.audioLanguageCount];
  rv = UnpackPDU(aPDU, UnpackArray<nsDependentCString>(
                         audioLanguages, aOut.audioLanguageCount));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  aOut.audioLanguages.AppendElements(audioLanguages, aOut.audioLanguageCount);

  // Read number of subtitle languages.
  rv = UnpackPDU(aPDU, aOut.subtitleLanguageCount);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  // Read subtitle languages.
  nsDependentCString subtitleLanguages[aOut.subtitleLanguageCount];
  rv = UnpackPDU(aPDU, UnpackArray<nsDependentCString>(
                         subtitleLanguages, aOut.subtitleLanguageCount));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  aOut.subtitleLanguages.AppendElements(subtitleLanguages,
                                        aOut.subtitleLanguageCount);

  return NS_OK;
}

nsresult
UnpackPDU(DaemonSocketPDU& aPDU, native_handle_t*& aOut)
{
  // Read size of native_handle.
  uint32_t version;
  nsresult rv = UnpackPDU(aPDU, version);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  // Read number of file descriptors.
  uint32_t numFds;
  rv = UnpackPDU(aPDU, numFds);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  // Read number of ints.
  uint32_t numInts;
  rv = UnpackPDU(aPDU, numInts);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  aOut = native_handle_create(numFds, numInts);

  // Read filer descriptors.
  nsTArray<int> fds;
  if (numFds != 0) {
    fds = aPDU.AcquireFds();
  }

  for (uint32_t i = 0; i < numFds + numInts; i++) {
    if (i < numFds) {
      aOut->data[i] = fds[i];
      continue;
    }
    uint32_t data;
    rv = UnpackPDU(aPDU, data);
    aOut->data[i] = data;
  }

  return NS_OK;
}

nsresult
UnpackPDU(DaemonSocketPDU& aPDU, TVStatus& aOut)
{
  return UnpackPDU(aPDU, UnpackConversion<uint8_t, TVStatus>(aOut));
}

nsresult
Convert(uint8_t aIn, TVStatus& aOut)
{
  static const TVStatus sStatus[] = {[0x00] = TVStatus::STATUS_OK,
                                     [0x01] = TVStatus::STATUS_FAILURE,
                                     [0x02] = TVStatus::STATUS_INVALID_ARG,
                                     [0x03] = TVStatus::STATUS_NO_SIGNAL,
                                     [0x04] = TVStatus::STATUS_NOT_SUPPORTED };
  if (MOZ_HAL_IPC_CONVERT_WARN_IF(aIn >= MOZ_ARRAY_LENGTH(sStatus), uint8_t,
                                  TVStatus)) {
    return NS_ERROR_ILLEGAL_VALUE;
  }
  aOut = sStatus[aIn];
  return NS_OK;
}

nsresult
Convert(TVSourceType aIn, uint8_t& aOut)
{
  aOut = static_cast<uint8_t>(aIn);
  return NS_OK;
}

} // namespace dom
} // namespace mozilla
