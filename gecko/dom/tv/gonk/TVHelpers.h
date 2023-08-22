/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_TVDaemonHelpers_h
#define mozilla_dom_TVDaemonHelpers_h

#include "mozilla/ipc/DaemonSocketPDU.h"
#include "nsITVService.h"

#define OPCODE_NTF_FILTER 0x80

namespace mozilla {
namespace dom {

using mozilla::ipc::DaemonSocketPDU;

struct TunerData
{
  nsString id;
  uint32_t sourceTypeNum;
  nsTArray<nsCString> sourceTypes;
};

struct ChannelData
{
  nsString networkId;
  nsString transportStreamId;
  nsString serviceId;
  uint8_t type;
  nsString number;
  nsString name;
  bool isEmergency;
  bool isFree;
};

struct ProgramData
{
  nsString eventId;
  nsString title;
  uint64_t startTime;
  uint64_t duration;
  nsString description;
  nsString rating;
  uint32_t audioLanguageCount;
  nsTArray<nsCString> audioLanguages;
  uint32_t subtitleLanguageCount;
  nsTArray<nsCString> subtitleLanguages;
};

enum class TVStatus : uint8_t {
  STATUS_OK = 0x00,
  STATUS_FAILURE = 0x01,
  STATUS_INVALID_ARG = 0x02,
  STATUS_NO_SIGNAL = 0x03,
  STATUS_NOT_SUPPORTED = 0x04
};

enum class GonkTVChannelType : uint8_t {
  TV = 0x00,
  RADIO = 0x01,
  DATA = 0x02
};

nsresult
PackTunerData(nsCOMPtr<nsITVTunerData>& aTunerData,
              const struct TunerData& aTunerMsgDta);

nsresult
PackChannelData(nsCOMPtr<nsITVChannelData>& aChannelData,
                const struct ChannelData& aChannelMsgDta);

nsresult
PackProgramData(nsCOMPtr<nsITVProgramData>& aProgramData,
                const struct ProgramData& aProgramMsgDta);

nsresult
UnpackPDU(DaemonSocketPDU& aPDU, TVStatus& aOut);

nsresult
UnpackPDU(DaemonSocketPDU& aPDU, struct TunerData& aOut);

nsresult
UnpackPDU(DaemonSocketPDU& aPDU, struct ChannelData& aOut);

nsresult
UnpackPDU(DaemonSocketPDU& aPDU, struct ProgramData& aOut);

nsresult
UnpackPDU(DaemonSocketPDU& aPDU, native_handle_t*& aOut);

nsresult
Convert(nsresult aIn, TVStatus& aOut);

nsresult
Convert(uint8_t aIn, TVStatus& aOut);

nsresult
Convert(TVSourceType aIn, uint8_t& aOut);

} // namespace dom
} // namespace mozilla

#endif
