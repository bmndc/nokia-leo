/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cutils/properties.h>

#include "NfcMessageHandler.h"
#include <binder/Parcel.h>
#include "mozilla/dom/MozNDEFRecordBinding.h"
#include "nsDebug.h"
#include "NfcOptions.h"
#include "mozilla/unused.h"
#include "nsNetUtil.h"
#include "nsIInputStream.h"

#include <android/log.h>
#define NMH_LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "NfcMessageHandler", args)

#define NFCD_MAJOR_VERSION 1
#define NFCD_MINOR_VERSION 22

#define TRANSIT_FILE_SIZE "nfc.transit.size"

using namespace android;
using namespace mozilla;
using namespace mozilla::dom;

bool
NfcMessageHandler::Marshall(Parcel& aParcel, const CommandOptions& aOptions)
{
  bool result = false;
  NMH_LOG("Marshall type %d\n", (int)aOptions.mType);
  switch (aOptions.mType) {
    case NfcRequestType::ChangeRFState:
      result = ChangeRFStateRequest(aParcel, aOptions);
      break;
    case NfcRequestType::ReadNDEF:
      result = ReadNDEFRequest(aParcel, aOptions);
      break;
    case NfcRequestType::WriteNDEF:
      result = WriteNDEFRequest(aParcel, aOptions);
      break;
    case NfcRequestType::MakeReadOnly:
      result = MakeReadOnlyRequest(aParcel, aOptions);
      break;
    case NfcRequestType::Format:
      result = FormatRequest(aParcel, aOptions);
      break;
    case NfcRequestType::Transceive:
      result = TransceiveRequest(aParcel, aOptions);
      break;
    case NfcRequestType::OpenConnection:
      result = OpenConnectionRequest(aParcel, aOptions);
      break;
    case NfcRequestType::Transmit:
      result = TransmitRequest(aParcel, aOptions);
      break;
    case NfcRequestType::CloseConnection:
      result = CloseConnectionRequest(aParcel, aOptions);
      break;
    case NfcRequestType::ResetSecureElement:
      result = ResetSecureElementRequest(aParcel, aOptions);
      break;
    case NfcRequestType::GetAtr:
      result = GetAtrRequest(aParcel, aOptions);
      break;
    case NfcRequestType::LsExecuteScript:
      result = LsExecuteScriptRequest(aParcel, aOptions);
      break;
    case NfcRequestType::LsGetVersion:
      result = LsGetVersionRequest(aParcel, aOptions);
      break;
    case NfcRequestType::MPOSReaderMode:
      result = MPOSReaderModeRequest(aParcel, aOptions);
      break;
    case NfcRequestType::NfcSelfTest:
      result = NfcSelfTestRequest(aParcel, aOptions);
      break;
    case NfcRequestType::SetConfig:
      result = SetConfigRequest(aParcel, aOptions);
      break;
    case NfcRequestType::TagReaderMode:
      result = TagReaderModeRequest(aParcel, aOptions);
      break;
    default:
      result = false;
      break;
  };

  return result;
}

bool
NfcMessageHandler::Unmarshall(const Parcel& aParcel, EventOptions& aOptions)
{
  mozilla::Unused << htonl(aParcel.readInt32());  // parcel size
  int32_t type = aParcel.readInt32();
  bool isNtf = type >> 31;
  aOptions.mIsNtf = isNtf;
  int32_t msgType = type & ~(1 << 31);
  NMH_LOG("Unmarshall type %d isNtf %d", type, isNtf);
  return isNtf ? ProcessNotification(msgType, aParcel, aOptions) :
                 ProcessResponse(msgType, aParcel, aOptions);
}

bool
NfcMessageHandler::ProcessResponse(int32_t aType, const Parcel& aParcel, EventOptions& aOptions)
{
  bool result = false;
  aOptions.mRspType = static_cast<NfcResponseType>(aType);
  switch (aOptions.mRspType) {
    case NfcResponseType::ChangeRFStateRsp:
      result = ChangeRFStateResponse(aParcel, aOptions);
      break;
    case NfcResponseType::ReadNDEFRsp:
      result = ReadNDEFResponse(aParcel, aOptions);
      break;
    case NfcResponseType::WriteNDEFRsp: // Fall through.
    case NfcResponseType::MakeReadOnlyRsp:
    case NfcResponseType::FormatRsp:
      result = GeneralResponse(aParcel, aOptions);
      break;
    case NfcResponseType::TransceiveRsp:
      result = TransceiveResponse(aParcel, aOptions);
      break;
    case NfcResponseType::OpenConnectionRsp:
      result = OpenConnectionResponse(aParcel, aOptions);
      break;
    case NfcResponseType::TransmitRsp:
      result = TransmitResponse(aParcel, aOptions);
      break;
    case NfcResponseType::CloseConnectionRsp:
      result = CloseConnectionResponse(aParcel, aOptions);
      break;
    case NfcResponseType::ResetSecureElementRsp:
      result = ResetSecureElementResponse(aParcel, aOptions);
      break;
    case NfcResponseType::GetAtrRsp:
      result = GetAtrResponse(aParcel, aOptions);
      break;
    case NfcResponseType::LsExecuteScriptRsp:
      result = LsExecuteScriptResponse(aParcel, aOptions);
      break;
    case NfcResponseType::LsGetVersionRsp:
      result = LsGetVersionResponse(aParcel, aOptions);
      break;
    case NfcResponseType::MPOSReaderModeRsp:
      result = MPOSReaderModeResponse(aParcel, aOptions);
      break;
    case NfcResponseType::NfcSelfTestRsp:
      result = NfcSelfTestResponse(aParcel, aOptions);
      break;
    case NfcResponseType::SetConfigRsp:
      result = SetConfigResponse(aParcel, aOptions);
      break;
    case NfcResponseType::TagReaderModeRsp:
      result = TagReaderModeResponse(aParcel, aOptions);
      break;
    default:
      result = false;
  }

  return result;
}

bool
NfcMessageHandler::ProcessNotification(int32_t aType, const Parcel& aParcel, EventOptions& aOptions)
{
  bool result = false;
  aOptions.mNtfType = static_cast<NfcNotificationType>(aType);
  NMH_LOG("ProcessNotification %d", aType);
  switch (aOptions.mNtfType) {
    case NfcNotificationType::Initialized:
      result = InitializeNotification(aParcel, aOptions);
      break;
    case NfcNotificationType::TechDiscovered:
      result = TechDiscoveredNotification(aParcel, aOptions);
      break;
    case NfcNotificationType::TechLost:
      result = TechLostNotification(aParcel, aOptions);
      break;
    case NfcNotificationType::HciEventTransaction:
      result = HCIEventTransactionNotification(aParcel, aOptions);
      break;
    case NfcNotificationType::NdefReceived:
      result = NDEFReceivedNotification(aParcel, aOptions);
      break;
    case NfcNotificationType::MPOSReaderModeEvent:
      result = MPOSReaderModeEventNotification(aParcel, aOptions);
      break;
    case NfcNotificationType::RfFieldActivateEvent:
    case NfcNotificationType::RfFieldDeActivateEvent:
    case NfcNotificationType::ListenModeActivateEvent:
    case NfcNotificationType::ListenModeDeActivateEvent:
      result = true;
      break;

    case NfcNotificationType::UnInitialized:
    case NfcNotificationType::EnableTimeout:
    case NfcNotificationType::DisableTimeout:
      if (!mRequestIdQueue.IsEmpty()) {
        aOptions.mRequestId = mRequestIdQueue[0];
        mRequestIdQueue.RemoveElementAt(0);
        result = true;
      }
      break;

    default:
    NMH_LOG("ProcessNotification %d", aType);
      result = false;
      break;
  }

  return result;
}

bool
NfcMessageHandler::GeneralResponse(const Parcel& aParcel, EventOptions& aOptions)
{
  aOptions.mErrorCode = aParcel.readInt32();
  aOptions.mSessionId = aParcel.readInt32();

  NS_ENSURE_TRUE(!mRequestIdQueue.IsEmpty(), false);
  aOptions.mRequestId = mRequestIdQueue[0];
  mRequestIdQueue.RemoveElementAt(0);
  return true;
}

bool
NfcMessageHandler::ChangeRFStateRequest(Parcel& aParcel, const CommandOptions& aOptions)
{
  aParcel.writeInt32(static_cast<int32_t>(NfcRequestType::ChangeRFState));
  aParcel.writeInt32(aOptions.mRfState);
  aParcel.writeInt32(aOptions.mPowerMode);
  mRequestIdQueue.AppendElement(aOptions.mRequestId);
  return true;
}

bool
NfcMessageHandler::ChangeRFStateResponse(const Parcel& aParcel, EventOptions& aOptions)
{
  aOptions.mErrorCode = aParcel.readInt32();
  NS_ENSURE_TRUE(!mRequestIdQueue.IsEmpty(), false);
  aOptions.mRequestId = mRequestIdQueue[0];
  mRequestIdQueue.RemoveElementAt(0);

  aOptions.mRfState = aParcel.readInt32();
  return true;
}

bool
NfcMessageHandler::ReadNDEFRequest(Parcel& aParcel, const CommandOptions& aOptions)
{
  aParcel.writeInt32(static_cast<int32_t>(NfcRequestType::ReadNDEF));
  aParcel.writeInt32(aOptions.mSessionId);
  mRequestIdQueue.AppendElement(aOptions.mRequestId);
  return true;
}

bool
NfcMessageHandler::ReadNDEFResponse(const Parcel& aParcel, EventOptions& aOptions)
{
  aOptions.mErrorCode = aParcel.readInt32();
  aOptions.mSessionId = aParcel.readInt32();

  NS_ENSURE_TRUE(!mRequestIdQueue.IsEmpty(), false);
  aOptions.mRequestId = mRequestIdQueue[0];
  mRequestIdQueue.RemoveElementAt(0);

  if (aOptions.mErrorCode == 0) {
    ReadNDEFMessage(aParcel, aOptions);
  }

  return true;
}

bool
NfcMessageHandler::TransceiveRequest(Parcel& aParcel, const CommandOptions& aOptions)
{
  aParcel.writeInt32(static_cast<int32_t>(NfcRequestType::Transceive));
  aParcel.writeInt32(aOptions.mSessionId);
  aParcel.writeInt32(aOptions.mTechnology);

  uint32_t length = aOptions.mCommand.Length();
  aParcel.writeInt32(length);

  void* data = aParcel.writeInplace(length);
  memcpy(data, aOptions.mCommand.Elements(), length);

  mRequestIdQueue.AppendElement(aOptions.mRequestId);
  return true;
}

bool
NfcMessageHandler::TransceiveResponse(const Parcel& aParcel, EventOptions& aOptions)
{
  aOptions.mErrorCode = aParcel.readInt32();
  aOptions.mSessionId = aParcel.readInt32();

  NS_ENSURE_TRUE(!mRequestIdQueue.IsEmpty(), false);
  aOptions.mRequestId = mRequestIdQueue[0];
  mRequestIdQueue.RemoveElementAt(0);

  if (aOptions.mErrorCode == 0) {
    ReadTransceiveResponse(aParcel, aOptions);
  }

  return true;
}

bool
NfcMessageHandler::WriteNDEFRequest(Parcel& aParcel, const CommandOptions& aOptions)
{
  aParcel.writeInt32(static_cast<int32_t>(NfcRequestType::WriteNDEF));
  aParcel.writeInt32(aOptions.mSessionId);
  aParcel.writeInt32(aOptions.mIsP2P);
  WriteNDEFMessage(aParcel, aOptions);
  mRequestIdQueue.AppendElement(aOptions.mRequestId);
  return true;
}

bool
NfcMessageHandler::MakeReadOnlyRequest(Parcel& aParcel, const CommandOptions& aOptions)
{
  aParcel.writeInt32(static_cast<int32_t>(NfcRequestType::MakeReadOnly));
  aParcel.writeInt32(aOptions.mSessionId);
  mRequestIdQueue.AppendElement(aOptions.mRequestId);
  return true;
}

bool
NfcMessageHandler::FormatRequest(Parcel& aParcel, const CommandOptions& aOptions)
{
  aParcel.writeInt32(static_cast<int32_t>(NfcRequestType::Format));
  aParcel.writeInt32(aOptions.mSessionId);
  mRequestIdQueue.AppendElement(aOptions.mRequestId);
  return true;
}
static int nfcdUID = 0;
static int nfcdGID = 0;
bool
NfcMessageHandler::InitializeNotification(const Parcel& aParcel, EventOptions& aOptions)
{
  aOptions.mStatus = aParcel.readInt32();
  aOptions.mMajorVersion = aParcel.readInt32();
  aOptions.mMinorVersion = aParcel.readInt32();
  nfcdUID = aParcel.readInt32();
  nfcdGID = aParcel.readInt32();

  NMH_LOG("Get nfcd uid %d gid %d", nfcdUID, nfcdGID);
  if (aOptions.mMajorVersion != NFCD_MAJOR_VERSION ||
      aOptions.mMinorVersion != NFCD_MINOR_VERSION) {
    NMH_LOG("NFCD version mismatched. majorVersion: %d, minorVersion: %d",
            aOptions.mMajorVersion, aOptions.mMinorVersion);
  }

  return true;
}

bool
NfcMessageHandler::TechDiscoveredNotification(const Parcel& aParcel, EventOptions& aOptions)
{
  aOptions.mSessionId = aParcel.readInt32();
  aOptions.mIsP2P = aParcel.readInt32();

  int32_t techCount = aParcel.readInt32();
  aOptions.mTechList.AppendElements(
    static_cast<const uint8_t*>(aParcel.readInplace(techCount)), techCount);

  int32_t idCount = aParcel.readInt32();
  aOptions.mTagId.AppendElements(
    static_cast<const uint8_t*>(aParcel.readInplace(idCount)), idCount);

  int32_t ndefMsgCount = aParcel.readInt32();
  if (ndefMsgCount != 0) {
    ReadNDEFMessage(aParcel, aOptions);
  }

  int32_t ndefInfo = aParcel.readInt32();
  if (ndefInfo) {
    aOptions.mTagType = aParcel.readInt32();
    aOptions.mMaxNDEFSize = aParcel.readInt32();
    aOptions.mIsReadOnly = aParcel.readInt32();
    aOptions.mIsFormatable = aParcel.readInt32();
  }

  return true;
}

bool
NfcMessageHandler::TechLostNotification(const Parcel& aParcel, EventOptions& aOptions)
{
  aOptions.mSessionId = aParcel.readInt32();
  return true;
}

bool
NfcMessageHandler::HCIEventTransactionNotification(const Parcel& aParcel, EventOptions& aOptions)
{
  aOptions.mOriginType = aParcel.readInt32();
  aOptions.mOriginIndex = aParcel.readInt32();

  int32_t aidLength = aParcel.readInt32();
  aOptions.mAid.AppendElements(
    static_cast<const uint8_t*>(aParcel.readInplace(aidLength)), aidLength);

  int32_t payloadLength = aParcel.readInt32();
  aOptions.mPayload.AppendElements(
    static_cast<const uint8_t*>(aParcel.readInplace(payloadLength)), payloadLength);

  return true;
}

bool
NfcMessageHandler::MPOSReaderModeEventNotification(const Parcel& aParcel, EventOptions& aOptions)
{
  aOptions.mMPOSReaderModeEvent = aParcel.readInt32();
  NMH_LOG("MPOSReaderModeEventNotification %d", aOptions.mMPOSReaderModeEvent);
  return true;
}

bool
NfcMessageHandler::NDEFReceivedNotification(const Parcel& aParcel, EventOptions& aOptions)
{
  aOptions.mSessionId = aParcel.readInt32();
  int32_t ndefMsgCount = aParcel.readInt32();
  if (ndefMsgCount != 0) {
    ReadNDEFMessage(aParcel, aOptions);
  }

  return true;
}

bool
NfcMessageHandler::ReadNDEFMessage(const Parcel& aParcel, EventOptions& aOptions)
{
  int32_t recordCount = aParcel.readInt32();
  aOptions.mRecords.SetCapacity(recordCount);

  for (int i = 0; i < recordCount; i++) {
    int32_t tnf = aParcel.readInt32();
    NDEFRecordStruct record;
    record.mTnf = static_cast<TNF>(tnf);

    int32_t typeLength = aParcel.readInt32();
    record.mType.AppendElements(
      static_cast<const uint8_t*>(aParcel.readInplace(typeLength)), typeLength);

    int32_t idLength = aParcel.readInt32();
    record.mId.AppendElements(
      static_cast<const uint8_t*>(aParcel.readInplace(idLength)), idLength);

    int32_t payloadLength = aParcel.readInt32();
    record.mPayload.AppendElements(
      static_cast<const uint8_t*>(aParcel.readInplace(payloadLength)), payloadLength);

    aOptions.mRecords.AppendElement(record);
  }

  return true;
}

bool
NfcMessageHandler::WriteNDEFMessage(Parcel& aParcel, const CommandOptions& aOptions)
{
  int recordCount = aOptions.mRecords.Length();
  aParcel.writeInt32(recordCount);
  for (int i = 0; i < recordCount; i++) {
    const NDEFRecordStruct& record = aOptions.mRecords[i];
    aParcel.writeInt32(static_cast<int32_t>(record.mTnf));

    void* data;

    aParcel.writeInt32(record.mType.Length());
    data = aParcel.writeInplace(record.mType.Length());
    memcpy(data, record.mType.Elements(), record.mType.Length());

    aParcel.writeInt32(record.mId.Length());
    data = aParcel.writeInplace(record.mId.Length());
    memcpy(data, record.mId.Elements(), record.mId.Length());

    aParcel.writeInt32(record.mPayload.Length());
    data = aParcel.writeInplace(record.mPayload.Length());
    memcpy(data, record.mPayload.Elements(), record.mPayload.Length());
  }

  return true;
}

bool
NfcMessageHandler::ReadTransceiveResponse(const Parcel& aParcel, EventOptions& aOptions)
{
  uint32_t length = aParcel.readInt32();

  aOptions.mResponse.AppendElements(
    static_cast<const uint8_t*>(aParcel.readInplace(length)), length);

  return true;
}

bool
NfcMessageHandler::OpenConnectionRequest(Parcel& aParcel, const CommandOptions& aOptions)
{
  aParcel.writeInt32(static_cast<int32_t>(NfcRequestType::OpenConnection));
  mRequestIdQueue.AppendElement(aOptions.mRequestId);
  return true;
}

bool
NfcMessageHandler::OpenConnectionResponse(const Parcel& aParcel, EventOptions& aOptions)
{
  aOptions.mErrorCode = aParcel.readInt32();
  NS_ENSURE_TRUE(!mRequestIdQueue.IsEmpty(), false);
  aOptions.mRequestId = mRequestIdQueue[0];
  mRequestIdQueue.RemoveElementAt(0);
  aOptions.mHandle = aParcel.readInt32();
  return true;
}

bool
NfcMessageHandler::TransmitRequest(Parcel& aParcel, const CommandOptions& aOptions)
{
  aParcel.writeInt32(static_cast<int32_t>(NfcRequestType::Transmit));
  aParcel.writeInt32(aOptions.mHandle);

  uint32_t length = aOptions.mApduCommand.Length();
  aParcel.writeInt32(length);

  void* data = aParcel.writeInplace(length);
  memcpy(data, aOptions.mApduCommand.Elements(), length);

  mRequestIdQueue.AppendElement(aOptions.mRequestId);
  return true;
}

bool
NfcMessageHandler::TransmitResponse(const Parcel& aParcel, EventOptions& aOptions)
{
  aOptions.mErrorCode = aParcel.readInt32();
  NS_ENSURE_TRUE(!mRequestIdQueue.IsEmpty(), false);
  aOptions.mRequestId = mRequestIdQueue[0];
  mRequestIdQueue.RemoveElementAt(0);
  aOptions.mHandle = aParcel.readInt32();

  uint32_t length = aParcel.readInt32();

  aOptions.mApduResponse.AppendElements(
    static_cast<const uint8_t*>(aParcel.readInplace(length)), length);

  return true;
}

bool
NfcMessageHandler::CloseConnectionRequest(Parcel& aParcel, const CommandOptions& aOptions)
{
  aParcel.writeInt32(static_cast<int32_t>(NfcRequestType::CloseConnection));
  aParcel.writeInt32(aOptions.mHandle);
  mRequestIdQueue.AppendElement(aOptions.mRequestId);
  return true;
}

bool
NfcMessageHandler::CloseConnectionResponse(const Parcel& aParcel, EventOptions& aOptions)
{
  aOptions.mErrorCode = aParcel.readInt32();
  NS_ENSURE_TRUE(!mRequestIdQueue.IsEmpty(), false);
  aOptions.mRequestId = mRequestIdQueue[0];
  mRequestIdQueue.RemoveElementAt(0);
  return true;
}

bool
NfcMessageHandler::ResetSecureElementRequest(Parcel& aParcel, const CommandOptions& aOptions)
{
  aParcel.writeInt32(static_cast<int32_t>(NfcRequestType::ResetSecureElement));
  aParcel.writeInt32(aOptions.mHandle);
  mRequestIdQueue.AppendElement(aOptions.mRequestId);
  return true;
}

bool
NfcMessageHandler::ResetSecureElementResponse(const Parcel& aParcel, EventOptions& aOptions)
{
  aOptions.mErrorCode = aParcel.readInt32();
  NS_ENSURE_TRUE(!mRequestIdQueue.IsEmpty(), false);
  aOptions.mRequestId = mRequestIdQueue[0];
  mRequestIdQueue.RemoveElementAt(0);
  aOptions.mHandle = aParcel.readInt32();
  return true;
}

bool
NfcMessageHandler::GetAtrRequest(Parcel& aParcel, const CommandOptions& aOptions)
{
  aParcel.writeInt32(static_cast<int32_t>(NfcRequestType::GetAtr));
  aParcel.writeInt32(aOptions.mHandle);
  mRequestIdQueue.AppendElement(aOptions.mRequestId);
  return true;
}

bool
NfcMessageHandler::GetAtrResponse(const Parcel& aParcel, EventOptions& aOptions)
{
  aOptions.mErrorCode = aParcel.readInt32();
  NS_ENSURE_TRUE(!mRequestIdQueue.IsEmpty(), false);
  aOptions.mRequestId = mRequestIdQueue[0];
  mRequestIdQueue.RemoveElementAt(0);
  aOptions.mHandle = aParcel.readInt32();

  uint32_t length = aParcel.readInt32();

  aOptions.mResponse.AppendElements(
    static_cast<const uint8_t*>(aParcel.readInplace(length)), length);

  return true;
}

status_t WriteString16(Parcel &p, const char16_t* str, size_t len) {
  if (str == nullptr)
    return p.writeInt32(-1);

  status_t err = p.writeInt32(len);
  if (err == NO_ERROR) {
    len *= sizeof(char16_t);
    uint8_t* data = (uint8_t*) p.writeInplace(len + sizeof(char16_t));
    if (data) {
      memcpy(data, str, len);
      *reinterpret_cast<char16_t*>(data + len) = 0;
      return NO_ERROR;
    }
    err = p.errorCheck();
  }
  return err;
}

bool
NfcMessageHandler::LsExecuteScriptRequest(Parcel& aParcel, const CommandOptions& aOptions)
{
  aParcel.writeInt32(static_cast<int32_t>(NfcRequestType::LsExecuteScript));
  WriteString16(aParcel, aOptions.mLsScriptFile.get(), aOptions.mLsScriptFile.Length());
  WriteString16(aParcel, aOptions.mLsResponseFile.get(), aOptions.mLsResponseFile.Length());
  WriteString16(aParcel, aOptions.mUniqueApplicationID.get(), aOptions.mUniqueApplicationID.Length());

  // Modify file mode and owner to allow nfcd to access the files.
  chmod(NS_LossyConvertUTF16toASCII(aOptions.mLsScriptFile).get(), S_IRWXU|S_IRWXG|S_IRWXO);
  chown(NS_LossyConvertUTF16toASCII(aOptions.mLsResponseFile).get(), nfcdUID, nfcdUID);
  chmod(NS_LossyConvertUTF16toASCII(aOptions.mLsResponseFile).get(), S_IRWXU|S_IRWXG|S_IRWXO);
  mRequestIdQueue.AppendElement(aOptions.mRequestId);
  return true;
}

bool
NfcMessageHandler::LsExecuteScriptResponse(const Parcel& aParcel, EventOptions& aOptions)
{
  aOptions.mErrorCode = aParcel.readInt32();
  NS_ENSURE_TRUE(!mRequestIdQueue.IsEmpty(), false);
  aOptions.mRequestId = mRequestIdQueue[0];
  mRequestIdQueue.RemoveElementAt(0);

  uint32_t length = aParcel.readInt32();

  aOptions.mResponse.AppendElements(
    static_cast<const uint8_t*>(aParcel.readInplace(length)), length);

  return true;
}

bool
NfcMessageHandler::LsGetVersionRequest(Parcel& aParcel, const CommandOptions& aOptions)
{
  aParcel.writeInt32(static_cast<int32_t>(NfcRequestType::LsGetVersion));
  mRequestIdQueue.AppendElement(aOptions.mRequestId);
  return true;
}

bool
NfcMessageHandler::LsGetVersionResponse(const Parcel& aParcel, EventOptions& aOptions)
{
  aOptions.mErrorCode = aParcel.readInt32();
  NS_ENSURE_TRUE(!mRequestIdQueue.IsEmpty(), false);
  aOptions.mRequestId = mRequestIdQueue[0];
  mRequestIdQueue.RemoveElementAt(0);

  uint32_t length = aParcel.readInt32();

  aOptions.mResponse.AppendElements(
    static_cast<const uint8_t*>(aParcel.readInplace(length)), length);

  return true;
}

bool
NfcMessageHandler::MPOSReaderModeRequest(Parcel& aParcel, const CommandOptions& aOptions)
{
  NMH_LOG("MPOSReaderModeRequest %d\n", aOptions.mMPOSReaderMode);
  aParcel.writeInt32(static_cast<int32_t>(NfcRequestType::MPOSReaderMode));
  aParcel.writeInt32(aOptions.mMPOSReaderMode);
  mRequestIdQueue.AppendElement(aOptions.mRequestId);
  return true;
}

bool
NfcMessageHandler::MPOSReaderModeResponse(const Parcel& aParcel, EventOptions& aOptions)
{
  NMH_LOG("MPOSReaderModeResponse\n");
  aOptions.mErrorCode = aParcel.readInt32();
  NS_ENSURE_TRUE(!mRequestIdQueue.IsEmpty(), false);
  aOptions.mRequestId = mRequestIdQueue[0];
  mRequestIdQueue.RemoveElementAt(0);
  return true;
}

bool
NfcMessageHandler::NfcSelfTestRequest(Parcel& aParcel, const CommandOptions& aOptions)
{
  NMH_LOG("NfcSelfTestRequest %d\n", aOptions.mSelfTestType);
  aParcel.writeInt32(static_cast<int32_t>(NfcRequestType::NfcSelfTest));
  aParcel.writeInt32(aOptions.mSelfTestType);
  mRequestIdQueue.AppendElement(aOptions.mRequestId);
  return true;
}

bool
NfcMessageHandler::NfcSelfTestResponse(const Parcel& aParcel, EventOptions& aOptions)
{
  NMH_LOG("NfcSelfTestResponse\n");
  aOptions.mErrorCode = aParcel.readInt32();
  NS_ENSURE_TRUE(!mRequestIdQueue.IsEmpty(), false);
  aOptions.mRequestId = mRequestIdQueue[0];
  mRequestIdQueue.RemoveElementAt(0);
  return true;
}

bool
NfcMessageHandler::SetConfigRequest(Parcel& aParcel, const CommandOptions& aOptions)
{
  int64_t maxSize;
  char propValue[PROPERTY_VALUE_MAX];
  property_get(TRANSIT_FILE_SIZE, propValue, "16384");
  maxSize = atol(propValue);

  ErrorResult rv;
  uint64_t size = aOptions.mConfBlob->GetSize(rv);
  if (NS_WARN_IF(rv.Failed())) {
    return true;
  }

  NMH_LOG("SetConfigRequest %llu %llu\n", size, (uint64_t)maxSize);
  if (size > (uint64_t)maxSize) {
    return false;
  }

  nsCOMPtr<nsIInputStream> inputStream;
  aOptions.mConfBlob->GetInternalStream(getter_AddRefs(inputStream), rv);
  if (NS_WARN_IF(rv.Failed())) {
    return false;
  }

  nsCString blobBuf;
  rv = NS_ReadInputStreamToString(inputStream, blobBuf, (uint32_t)size);
  if (NS_WARN_IF(rv.Failed())) {
    return false;
  }
  NMH_LOG("Blob buffer size = %llu ", size);

  aParcel.writeInt32(static_cast<int32_t>(NfcRequestType::SetConfig));
  WriteString16(aParcel, aOptions.mRfConfType.get(), aOptions.mRfConfType.Length());

  nsString blob = NS_ConvertUTF8toUTF16(blobBuf);
  WriteString16(aParcel, blob.get(), size);
  mRequestIdQueue.AppendElement(aOptions.mRequestId);
  return true;
}

bool
NfcMessageHandler::SetConfigResponse(const Parcel& aParcel, EventOptions& aOptions)
{
  aOptions.mErrorCode = aParcel.readInt32();
  aOptions.mSetConfigResult = aParcel.readInt32();

  NMH_LOG("SetConfigResponse code= %d result= %d\n", aOptions.mErrorCode, aOptions.mSetConfigResult);

  NS_ENSURE_TRUE(!mRequestIdQueue.IsEmpty(), false);
  aOptions.mRequestId = mRequestIdQueue[0];
  mRequestIdQueue.RemoveElementAt(0);
  return true;
}

bool
NfcMessageHandler::TagReaderModeRequest(Parcel& aParcel, const CommandOptions& aOptions)
{
  NMH_LOG("TagReaderModeRequest %d\n", aOptions.mTagReaderMode);
  aParcel.writeInt32(static_cast<int32_t>(NfcRequestType::TagReaderMode));
  aParcel.writeInt32(aOptions.mTagReaderMode);
  mRequestIdQueue.AppendElement(aOptions.mRequestId);
  return true;
}

bool
NfcMessageHandler::TagReaderModeResponse(const Parcel& aParcel, EventOptions& aOptions)
{
  NMH_LOG("TagReaderModeResponse\n");
  aOptions.mErrorCode = aParcel.readInt32();
  NS_ENSURE_TRUE(!mRequestIdQueue.IsEmpty(), false);
  aOptions.mRequestId = mRequestIdQueue[0];
  mRequestIdQueue.RemoveElementAt(0);
  return true;
}