/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "NfcService.h"
#include <binder/Parcel.h>
#include <cutils/properties.h>
#include "mozilla/ClearOnShutdown.h"
#include "mozilla/dom/NfcOptionsBinding.h"
#include "mozilla/dom/MozNFCBinding.h"
#include "mozilla/dom/ToJSValue.h"
#include "mozilla/dom/RootedDictionary.h"
#include "mozilla/Endian.h"
#include "mozilla/Hal.h"
#include "mozilla/ipc/ListenSocket.h"
#include "mozilla/ipc/ListenSocketConsumer.h"
#include "mozilla/ipc/NfcConnector.h"
#include "mozilla/ipc/StreamSocket.h"
#include "mozilla/ipc/StreamSocketConsumer.h"
#include "mozilla/ModuleUtils.h"
#include "mozilla/UniquePtr.h"
#include "mozilla/unused.h"
#include "NfcMessageHandler.h"
#include "NfcOptions.h"
#include "nsString.h"
#include "nsXULAppAPI.h"

#define NS_NFCSERVICE_CID \
  { 0x584c9a21, 0x4e17, 0x43b7, {0xb1, 0x6a, 0x87, 0xa0, 0x42, 0xef, 0xd4, 0x64} }
#define NS_NFCSERVICE_CONTRACTID "@mozilla.org/nfc/service;1"

using namespace android;
using namespace mozilla::dom;
using namespace mozilla::ipc;
using namespace mozilla::hal;

namespace mozilla {

static StaticRefPtr<NfcService> gNfcService;

NS_IMPL_ISUPPORTS(NfcService, nsINfcService)

static const int sRetryInterval = 100; // ms
//
// NfcConsumer
//

/**
 * |NfcConsumer| implements the details of the connection to an NFC daemon
 * as well as the message passing.
 */
class NfcConsumer
  : public ListenSocketConsumer
  , public StreamSocketConsumer
{
public:
  NfcConsumer(NfcService* aNfcService);

  nsresult Start();
  void Shutdown();

  nsresult Send(const CommandOptions& aCommandOptions);
  nsresult Receive(UnixSocketBuffer* aBuffer);

  // Methods for |StreamSocketConsumer| and |ListenSocketConsumer|
  //

  void ReceiveSocketData(
    int aIndex, UniquePtr<mozilla::ipc::UnixSocketBuffer>& aBuffer) override;

  void OnConnectSuccess(int aIndex) override;
  void OnConnectError(int aIndex) override;
  void OnDisconnect(int aIndex) override;

  nsresult StartNfcService(const char* aSvcName, const char* aArgs);
  void StopNfcService(const char* aSvcName);
  bool NfcServiceIsStopped(const char* aSvcName);
  bool NfcServiceIsRunning(const char* aSvcName);

private:
  class DispatchNfcEventRunnable;
  class ShutdownServiceRunnable;

  enum SocketType {
    LISTEN_SOCKET,
    STREAM_SOCKET
  };

  bool IsNfcServiceThread() const;

  RefPtr<NfcService> mNfcService;
  nsCOMPtr<nsIThread> mThread;
  RefPtr<mozilla::ipc::ListenSocket> mListenSocket;
  RefPtr<mozilla::ipc::StreamSocket> mStreamSocket;
  UniquePtr<NfcMessageHandler> mHandler;
  nsCString mListenSocketName;
};

NfcConsumer::NfcConsumer(NfcService* aNfcService)
  : mNfcService(aNfcService)
{
  MOZ_ASSERT(mNfcService);
}

class StartNfcServiceTimerCallback final : public nsITimerCallback
{
  NS_DECL_THREADSAFE_ISUPPORTS;

public:
  StartNfcServiceTimerCallback(NfcConsumer *consumer, const char* aSvcName, const char* aArgs)
    : mConsumer(consumer)
    , mSvcName(aSvcName)
    , mArgs(aArgs)
  {
    MOZ_COUNT_CTOR_INHERITED(StartNfcServiceTimerCallback,
                             nsITimerCallback);
  }

  NS_IMETHOD Notify(nsITimer* aTimer) override
  {
    MOZ_ASSERT(NS_IsMainThread());

    return mConsumer->StartNfcService(mSvcName.get(), mArgs.get());
  }

protected:
  ~StartNfcServiceTimerCallback()
  {
    MOZ_COUNT_DTOR_INHERITED(StartNfcServiceTimerCallback,
                             nsITimerCallback);
  }

private:
  NfcConsumer *mConsumer;
  nsCString mSvcName;
  nsCString mArgs;
};

NS_IMPL_ISUPPORTS0(StartNfcServiceTimerCallback);

nsresult
NfcConsumer::StartNfcService(const char* aSvcName, const char* aArgs)
{
  MOZ_ASSERT(IsNfcServiceThread());

  char value[PROPERTY_VALUE_MAX];
  auto res = snprintf(value, sizeof(value), "%s:%s", aSvcName, aArgs);

  if (res < 0) {
    return NS_ERROR_FAILURE;
  } else if (static_cast<size_t>(res) >= sizeof(value)) {
    NS_WARNING("trunctated service name ");
    return NS_ERROR_OUT_OF_MEMORY;
  }

  if (NS_WARN_IF(property_set("ctl.start", value) < 0)) {
    return NS_ERROR_FAILURE;
  }

  /* If the system service is not running, re-try later to start it.
   *
   * This condition happens when we restart a service immediately
   * after it crashed, as the service state remains 'stopping'
   * instead of 'stopped'. Due to the limitation of property service,
   * hereby add delay. See Bug 1143925 Comment 41.
   */
  if (!NfcServiceIsRunning(aSvcName)) {
    nsCOMPtr<nsITimer> timer = do_CreateInstance("@mozilla.org/timer;1");
    if (!timer) {
      return NS_ERROR_FAILURE;
    }

    RefPtr<StartNfcServiceTimerCallback> timerCallback =
      new StartNfcServiceTimerCallback(this, aSvcName, aArgs);

    timer->InitWithCallback(timerCallback,
                            sRetryInterval,
                            nsITimer::TYPE_ONE_SHOT);
  }

  return NS_OK;
}

void
NfcConsumer::StopNfcService(const char* aSvcName)
{
  MOZ_ASSERT(IsNfcServiceThread());

  Unused << NS_WARN_IF(property_set("ctl.stop", aSvcName));
}

bool
NfcConsumer::NfcServiceIsRunning(const char* aSvcName)
{
  MOZ_ASSERT(IsNfcServiceThread());

  char key[PROPERTY_KEY_MAX];
  auto res = snprintf(key, sizeof(key), "init.svc.%s", aSvcName);

  if (res < 0) {
    return false;
  } else if (static_cast<size_t>(res) >= sizeof(key)) {
    NS_WARNING("trunctated service name ");
    return false;
  }

  char value[PROPERTY_VALUE_MAX];
  NS_WARN_IF(property_get(key, value, "") < 0);

  return !strcmp(value, "running");
}

bool
NfcConsumer::NfcServiceIsStopped(const char* aSvcName)
{
  MOZ_ASSERT(IsNfcServiceThread());

  char key[PROPERTY_KEY_MAX];
  auto res = snprintf(key, sizeof(key), "init.svc.%s", aSvcName);

  if (res < 0) {
    return false;
  } else if (static_cast<size_t>(res) >= sizeof(key)) {
    NS_WARNING("trunctated service name ");
    return false;
  }

  char value[PROPERTY_VALUE_MAX];
  NS_WARN_IF(property_get(key, value, "") < 0);

  return !strcmp(value, "stopped");
}

nsresult
NfcConsumer::Start()
{
  static const char BASE_SOCKET_NAME[] = "nfcd";

  MOZ_ASSERT(!mThread); // already started

  // Store a pointer of the consumer's NFC thread for
  // use with |IsNfcServiceThread|.
  mThread = do_GetCurrentThread();

  // If we could not cleanup properly before and an old
  // instance of the daemon is still running, we kill it
  // here.
  StopNfcService("nfcd");

  bool stopped = NfcServiceIsStopped("nfcd");
  int count = 0;
  // Make sure nfcd is not in middle of stop stage.
  // Android's init service creates a new process only when
  // the process is in stopped state.
  while (!stopped && count++ < 5) {
    PR_Sleep(PR_MillisecondsToInterval(100));
    stopped = NfcServiceIsStopped("nfcd");
  }

  mHandler = MakeUnique<NfcMessageHandler>();

  mStreamSocket = new StreamSocket(this, STREAM_SOCKET);

  mListenSocketName = BASE_SOCKET_NAME;

  mListenSocket = new ListenSocket(this, LISTEN_SOCKET);
  nsresult rv = mListenSocket->Listen(new NfcConnector(mListenSocketName),
                                      mStreamSocket);
  if (NS_FAILED(rv)) {
    mStreamSocket = nullptr;
    mHandler = nullptr;
    mThread = nullptr;
    return rv;
  }

  return NS_OK;
}

void
NfcConsumer::Shutdown()
{
  MOZ_ASSERT(IsNfcServiceThread());

  mListenSocket->Close();
  mListenSocket = nullptr;
  mStreamSocket->Close();
  mStreamSocket = nullptr;

  mHandler = nullptr;

  mThread = nullptr;
}

// Runnable used dispatch the NfcEventOptions on the main thread.
class NfcConsumer::DispatchNfcEventRunnable final : public nsRunnable
{
public:
  DispatchNfcEventRunnable(NfcService* aNfcService, const EventOptions& aEvent)
    : mNfcService(aNfcService)
    , mEvent(aEvent)
  {
    MOZ_ASSERT(mNfcService);
  }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());

    mozilla::AutoSafeJSContext cx;
    RootedDictionary<NfcEventOptions> event(cx);

    // the copy constructor is private.
#define COPY_FIELD(prop) event.prop = mEvent.prop;

#define COPY_OPT_FIELD(prop, defaultValue)           \
    if (mEvent.prop != defaultValue) {               \
      event.prop.Construct();                        \
      event.prop.Value() = mEvent.prop;              \
    }

    COPY_OPT_FIELD(mRspType, NfcResponseType::EndGuard_)
    COPY_OPT_FIELD(mNtfType, NfcNotificationType::EndGuard_)

    // Only one of rspType and ntfType should be used.
    MOZ_ASSERT(((mEvent.mRspType != NfcResponseType::EndGuard_) ||
                (mEvent.mNtfType != NfcNotificationType::EndGuard_)) &&
               ((mEvent.mRspType == NfcResponseType::EndGuard_) ||
                (mEvent.mNtfType == NfcNotificationType::EndGuard_)));

    COPY_OPT_FIELD(mRequestId, EmptyString())
    COPY_OPT_FIELD(mStatus, -1)
    COPY_OPT_FIELD(mSessionId, -1)
    COPY_OPT_FIELD(mMajorVersion, -1)
    COPY_OPT_FIELD(mMinorVersion, -1)

    if (mEvent.mRfState != -1) {
      event.mRfState.Construct();
      RFState rfState = static_cast<RFState>(mEvent.mRfState);
      MOZ_ASSERT(rfState < RFState::EndGuard_);
      event.mRfState.Value() = rfState;
    }

    if (mEvent.mErrorCode != -1) {
      event.mErrorMsg.Construct();
      event.mErrorMsg.Value() = static_cast<NfcErrorMessage>(mEvent.mErrorCode);
    }

    if (mEvent.mTechList.Length() > 0) {
      int length = mEvent.mTechList.Length();
      event.mTechList.Construct();

      if (!event.mTechList.Value().SetCapacity(length, fallible)) {
        return NS_ERROR_FAILURE;
      }

      for (int i = 0; i < length; i++) {
        NFCTechType tech = static_cast<NFCTechType>(mEvent.mTechList[i]);
        MOZ_ASSERT(tech < NFCTechType::EndGuard_);
        // FIXME: Make this infallible after bug 968520 is done.
        *event.mTechList.Value().AppendElement(fallible) = tech;
      }
    }

    if (mEvent.mTagId.Length() > 0) {
      event.mTagId.Construct();
      event.mTagId.Value().Init(Uint8Array::Create(cx, mEvent.mTagId.Length(), mEvent.mTagId.Elements()));
    }

    if (mEvent.mRecords.Length() > 0) {
      int length = mEvent.mRecords.Length();
      event.mRecords.Construct();
      if (!event.mRecords.Value().SetCapacity(length, fallible)) {
        return NS_ERROR_FAILURE;
      }

      for (int i = 0; i < length; i++) {
        NDEFRecordStruct& recordStruct = mEvent.mRecords[i];
        // FIXME: Make this infallible after bug 968520 is done.
        MozNDEFRecordOptions& record =
          *event.mRecords.Value().AppendElement(fallible);

        record.mTnf = recordStruct.mTnf;
        MOZ_ASSERT(record.mTnf < TNF::EndGuard_);

        if (recordStruct.mType.Length() > 0) {
          record.mType.Construct();
          record.mType.Value().SetValue().Init(Uint8Array::Create(cx, recordStruct.mType.Length(), recordStruct.mType.Elements()));
        }

        if (recordStruct.mId.Length() > 0) {
          record.mId.Construct();
          record.mId.Value().SetValue().Init(Uint8Array::Create(cx, recordStruct.mId.Length(), recordStruct.mId.Elements()));
        }

        if (recordStruct.mPayload.Length() > 0) {
          record.mPayload.Construct();
          record.mPayload.Value().SetValue().Init(Uint8Array::Create(cx, recordStruct.mPayload.Length(), recordStruct.mPayload.Elements()));
        }
      }
    }

    COPY_OPT_FIELD(mIsP2P, -1)

    if (mEvent.mTagType != -1) {
      event.mTagType.Construct();
      event.mTagType.Value() = static_cast<NFCTagType>(mEvent.mTagType);
    }

    COPY_OPT_FIELD(mMaxNDEFSize, -1)
    COPY_OPT_FIELD(mIsReadOnly, -1)
    COPY_OPT_FIELD(mIsFormatable, -1)

    // HCI Event Transaction parameters.
    if (mEvent.mOriginType != -1) {
      MOZ_ASSERT(static_cast<HCIEventOrigin>(mEvent.mOriginType) < HCIEventOrigin::EndGuard_);

      event.mOrigin.Construct();
      event.mOrigin.Value().AssignASCII(HCIEventOriginValues::strings[mEvent.mOriginType].value);
      event.mOrigin.Value().AppendInt(mEvent.mOriginIndex, 16 /* radix */);
    }

    if (mEvent.mAid.Length() > 0) {
      event.mAid.Construct();
      event.mAid.Value().Init(Uint8Array::Create(cx, mEvent.mAid.Length(), mEvent.mAid.Elements()));
    }

    if (mEvent.mPayload.Length() > 0) {
      event.mPayload.Construct();
      event.mPayload.Value().Init(Uint8Array::Create(cx, mEvent.mPayload.Length(), mEvent.mPayload.Elements()));
    }

    if (mEvent.mResponse.Length() > 0) {
      event.mResponse.Construct();
      event.mResponse.Value().Init(
        Uint8Array::Create(cx, mEvent.mResponse.Length(), mEvent.mResponse.Elements()));
    }

    COPY_OPT_FIELD(mHandle, -1)

    if (mEvent.mApduResponse.Length() > 0) {
      event.mApduResponse.Construct();
      event.mApduResponse.Value().Init(
        Uint8Array::Create(cx, mEvent.mApduResponse.Length(), mEvent.mApduResponse.Elements()));
    }

    COPY_OPT_FIELD(mMPOSReaderModeEvent, -1)

    COPY_OPT_FIELD(mSetConfigResult, -1)
#undef COPY_FIELD
#undef COPY_OPT_FIELD

    JS::RootedValue val(cx);

    if (!ToJSValue(cx, event, &val)) {
      return NS_ERROR_FAILURE;
    }

    mNfcService->DispatchNfcEvent(val);
    return NS_OK;
  }

private:
  RefPtr<NfcService> mNfcService;
  EventOptions mEvent;
};

NfcResponseType
ConvertToResponse(EventOptions& event, NfcRequestType reqquestType)
{
  NfcResponseType returnType;

  switch (reqquestType) {
    case NfcRequestType::ChangeRFState:
      returnType = NfcResponseType::ChangeRFStateRsp;
      event.mRfState = (int32_t)RFState::Idle;
      break;
    case NfcRequestType::ReadNDEF:
      returnType = NfcResponseType::ReadNDEFRsp;
      break;
    case NfcRequestType::WriteNDEF:
      returnType = NfcResponseType::WriteNDEFRsp;
      break;
    case NfcRequestType::MakeReadOnly:
      returnType = NfcResponseType::MakeReadOnlyRsp;
      break;
    case NfcRequestType::Format:
      returnType = NfcResponseType::FormatRsp;
      break;
    case NfcRequestType::Transceive:
      returnType = NfcResponseType::TransceiveRsp;
      break;
    case NfcRequestType::OpenConnection:
      returnType = NfcResponseType::OpenConnectionRsp;
      break;
    case NfcRequestType::Transmit:
      returnType = NfcResponseType::TransmitRsp;
      break;
    case NfcRequestType::CloseConnection:
      returnType = NfcResponseType::CloseConnectionRsp;
      break;
    case NfcRequestType::ResetSecureElement:
      returnType = NfcResponseType::ResetSecureElementRsp;
      break;
    case NfcRequestType::GetAtr:
      returnType = NfcResponseType::GetAtrRsp;
      break;
    case NfcRequestType::LsExecuteScript:
      returnType = NfcResponseType::LsExecuteScriptRsp;
      break;
    case NfcRequestType::LsGetVersion:
      returnType = NfcResponseType::LsGetVersionRsp;
      break;
    case NfcRequestType::MPOSReaderMode:
      returnType = NfcResponseType::MPOSReaderModeRsp;
      break;
    case NfcRequestType::NfcSelfTest:
      returnType = NfcResponseType::NfcSelfTestRsp;
      break;
    case NfcRequestType::SetConfig:
      returnType = NfcResponseType::SetConfigRsp;
      break;
    case NfcRequestType::TagReaderMode:
      returnType = NfcResponseType::TagReaderModeRsp;
      break;

    default:
      returnType = NfcResponseType::EndGuard_;
      break;
  };

  return returnType;
}

nsresult
NfcConsumer::Send(const CommandOptions& aOptions)
{
  MOZ_ASSERT(IsNfcServiceThread());

  if (NS_WARN_IF(!mStreamSocket) ||
    NS_WARN_IF(mStreamSocket->GetConnectionStatus() != SOCKET_CONNECTED)) {

    EventOptions event;
    event.mErrorCode = (int32_t)NfcErrorMessage::ErrorConnect;
    event.mRspType = ConvertToResponse(event, aOptions.mType);
    event.mRequestId = aOptions.mRequestId;
    NS_DispatchToMainThread(new DispatchNfcEventRunnable(mNfcService, event));
    return NS_OK; // Probably shutting down.
  }

  Parcel parcel;
  parcel.writeInt32(0); // Parcel Size.
  mHandler->Marshall(parcel, aOptions);
  parcel.setDataPosition(0);
  uint32_t sizeBE = htonl(parcel.dataSize() - sizeof(int));
  parcel.writeInt32(sizeBE);

  // TODO: Zero-copy buffer transfers
  mStreamSocket->SendSocketData(
    new UnixSocketRawData(parcel.data(), parcel.dataSize()));

  return NS_OK;
}

nsresult
NfcConsumer::Receive(UnixSocketBuffer* aBuffer)
{
  MOZ_ASSERT(IsNfcServiceThread());
  MOZ_ASSERT(mHandler);
  MOZ_ASSERT(aBuffer);

  while (aBuffer->GetSize()) {
    const uint8_t* data = aBuffer->GetData();
    uint32_t parcelSize = BigEndian::readUint32(data);

    MOZ_ASSERT(parcelSize <= aBuffer->GetSize());

    // TODO: Zero-copy buffer transfers
    Parcel parcel;
    parcel.setData(aBuffer->GetData(), parcelSize + sizeof(parcelSize));
    aBuffer->Consume(parcelSize + sizeof(parcelSize));

    EventOptions event;
    mHandler->Unmarshall(parcel, event);

    NS_DispatchToMainThread(new DispatchNfcEventRunnable(mNfcService, event));
  }

  return NS_OK;
}

bool
NfcConsumer::IsNfcServiceThread() const
{
  return nsCOMPtr<nsIThread>(do_GetCurrentThread()) == mThread;
}

// |StreamSocketConsumer|, |ListenSocketConsumer|

void
NfcConsumer::ReceiveSocketData(
  int aIndex, UniquePtr<mozilla::ipc::UnixSocketBuffer>& aBuffer)
{
  MOZ_ASSERT(IsNfcServiceThread());
  MOZ_ASSERT(aIndex == STREAM_SOCKET);

  Receive(aBuffer.get());
}

void
NfcConsumer::OnConnectSuccess(int aIndex)
{
  MOZ_ASSERT(IsNfcServiceThread());

  switch (aIndex) {
    case LISTEN_SOCKET: {
      nsCString args("-S -a ");
      args.Append(mListenSocketName);
      nsresult rv = StartNfcService("nfcd", args.get());
      if (NS_FAILED(rv)) {
        OnConnectError(STREAM_SOCKET);
      }
      break;
    }
    case STREAM_SOCKET:
      /* nothing to do */
      break;
  }
}

class NfcConsumer::ShutdownServiceRunnable final : public nsRunnable
{
public:
  ShutdownServiceRunnable(NfcService* aNfcService)
    : mNfcService(aNfcService)
  {
    MOZ_ASSERT(mNfcService);
  }

  NS_IMETHOD Run() override
  {
    MOZ_ASSERT(NS_IsMainThread());

    mNfcService->Shutdown();

    return NS_OK;
  }

private:
  RefPtr<NfcService> mNfcService;
};

void
NfcConsumer::OnConnectError(int aIndex)
{
  MOZ_ASSERT(IsNfcServiceThread());

  NS_DispatchToMainThread(new ShutdownServiceRunnable(mNfcService));
}

void
NfcConsumer::OnDisconnect(int aIndex)
{
  MOZ_ASSERT(IsNfcServiceThread());
}

//
// NfcService
//

NfcService::NfcService()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!gNfcService);
}

NfcService::~NfcService()
{
  MOZ_ASSERT(!gNfcService);
}

already_AddRefed<NfcService>
NfcService::FactoryCreate()
{
  if (!XRE_IsParentProcess()) {
    return nullptr;
  }

  MOZ_ASSERT(NS_IsMainThread());

  if (!gNfcService) {
    gNfcService = new NfcService();
    ClearOnShutdown(&gNfcService);
  }

  RefPtr<NfcService> service(gNfcService);
  return service.forget();
}

/**
 * |StartConsumerRunnable| calls |NfcConsumer::Start| on the NFC thread.
 */
class NfcService::StartConsumerRunnable final : public nsRunnable
{
public:
  StartConsumerRunnable(NfcConsumer* aNfcConsumer)
    : mNfcConsumer(aNfcConsumer)
  {
    MOZ_ASSERT(mNfcConsumer);
  }

  NS_IMETHOD Run() override
  {
    mNfcConsumer->Start();

    return NS_OK;
  }

private:
  NfcConsumer* mNfcConsumer;
};

NS_IMETHODIMP
NfcService::Start(nsINfcGonkEventListener* aListener)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aListener);
  MOZ_ASSERT(!mThread);
  MOZ_ASSERT(!mNfcConsumer);

  auto nfcConsumer = MakeUnique<NfcConsumer>(this);

  nsresult rv = NS_NewNamedThread("NfcThread", getter_AddRefs(mThread));
  if (NS_FAILED(rv)) {
    NS_WARNING("Can't create Nfc worker thread.");
    return rv;
  }

  rv = mThread->Dispatch(new StartConsumerRunnable(nfcConsumer.get()),
                         nsIEventTarget::DISPATCH_NORMAL);
  if (NS_FAILED(rv)) {
    return rv;
  }

  mListener = aListener;
  mNfcConsumer = Move(nfcConsumer);

  return NS_OK;
}

// static
void
NfcService::TimerCallback(nsITimer* aTimer, void* aClosure)
{
  RefPtr<nsIThread> nfcThread = static_cast<nsIThread*>(aClosure);
  nfcThread->Shutdown();
  nfcThread = nullptr;
  gNfcService->mTimer->Cancel();
  gNfcService->mTimer = nullptr;
}

/**
 * |CleanupRunnable| deletes instances of the NFC consumer and
 * thread on the main thread. This has to be down after shutting
 * down the NFC consumer on the NFC thread.
 */
class NfcService::CleanupRunnable final : public nsRunnable
{
public:
  CleanupRunnable(NfcConsumer* aNfcConsumer,
                  already_AddRefed<nsIThread> aThread)
    : mNfcConsumer(aNfcConsumer)
    , mThread(aThread)
  {
    MOZ_ASSERT(mNfcConsumer);
    MOZ_ASSERT(mThread);
  }

  NS_IMETHOD Run() override
  {
    MOZ_ASSERT(NS_IsMainThread());

    mNfcConsumer = nullptr; // deletes NFC consumer

    if (!gNfcService->mTimer) {
      gNfcService->mTimer = do_CreateInstance("@mozilla.org/timer;1");
      gNfcService->mTimer->InitWithFuncCallback(TimerCallback, mThread,
                                   500,
                                   nsITimer::TYPE_ONE_SHOT);
    }
    return NS_OK;
  }

private:
  UniquePtr<NfcConsumer> mNfcConsumer;
  nsCOMPtr<nsIThread> mThread;
};

/**
 * |ShutdownConsumerRunnable| calls |NfcConsumer::Shutdown| on the
 * NFC thread. Optionally, it can dispatch a |CleanupRunnable| to
 * the main thread for cleaning up the NFC resources.
 */
class NfcService::ShutdownConsumerRunnable final : public nsRunnable
{
public:
  ShutdownConsumerRunnable(NfcConsumer* aNfcConsumer, bool aCleanUp)
    : mNfcConsumer(aNfcConsumer)
    , mCleanUp(aCleanUp)
  {
    MOZ_ASSERT(mNfcConsumer);
  }

  NS_IMETHOD Run() override
  {
    mNfcConsumer->Shutdown();

    if (mCleanUp) {
      NS_DispatchToMainThread(
        new CleanupRunnable(mNfcConsumer, do_GetCurrentThread()));
    }

    return NS_OK;
  }

private:
  NfcConsumer* mNfcConsumer;
  bool mCleanUp;
};

NS_IMETHODIMP
NfcService::Shutdown()
{
  if (!mNfcConsumer) {
    return NS_OK; // NFC was shut down meanwhile; not an error
  }

  nsresult rv = mThread->Dispatch(
    new ShutdownConsumerRunnable(mNfcConsumer.get(), true),
    nsIEventTarget::DISPATCH_NORMAL);

  if (NS_FAILED(rv)) {
    return rv;
  }

  // |CleanupRunnable| will take care of these pointers
  Unused << mNfcConsumer.release();
  Unused << mThread.forget();

  return NS_OK;
}

/**
 * |SendRunnable| calls |NfcConsumer::Send| on the NFC thread.
 */
class NfcService::SendRunnable final : public nsRunnable
{
public:
  SendRunnable(NfcConsumer* aNfcConsumer, const CommandOptions& aOptions)
    : mNfcConsumer(aNfcConsumer)
    , mOptions(aOptions)
  {
    MOZ_ASSERT(mNfcConsumer);
  }

  NS_IMETHOD Run() override
  {
    mNfcConsumer->Send(mOptions);

    return NS_OK;
  }

private:
   NfcConsumer* mNfcConsumer;
   CommandOptions mOptions;
};

NS_IMETHODIMP
NfcService::SendCommand(JS::HandleValue aOptions, JSContext* aCx)
{
  MOZ_ASSERT(NS_IsMainThread());

  NfcCommandOptions options;

  if (!options.Init(aCx, aOptions)) {
    NS_WARNING("Bad dictionary passed to NfcService::SendCommand");
    return NS_ERROR_FAILURE;
  }

  // Dispatch the command to the NFC thread.
  nsresult rv = mThread->Dispatch(
    new SendRunnable(mNfcConsumer.get(), CommandOptions(options)),
    nsIEventTarget::DISPATCH_NORMAL);

  if (NS_FAILED(rv)) {
    return rv;
  }

  return NS_OK;
}

void
NfcService::DispatchNfcEvent(JS::RootedValue& optionsValue)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mListener);

  if (!mNfcConsumer) {
    return; // NFC has been shutdown meanwhile; not en error
  }

  mListener->OnEvent(optionsValue);
}

NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(NfcService,
                                         NfcService::FactoryCreate)

NS_DEFINE_NAMED_CID(NS_NFCSERVICE_CID);

static const mozilla::Module::CIDEntry kNfcServiceCIDs[] = {
  { &kNS_NFCSERVICE_CID, false, nullptr, NfcServiceConstructor },
  { nullptr }
};

static const mozilla::Module::ContractIDEntry kNfcServiceContracts[] = {
  { NS_NFCSERVICE_CONTRACTID, &kNS_NFCSERVICE_CID },
  { nullptr }
};

static const mozilla::Module kNfcServiceModule = {
  mozilla::Module::kVersion,
  kNfcServiceCIDs,
  kNfcServiceContracts,
  nullptr
};

} // namespace mozilla

NSMODULE_DEFN(NfcServiceModule) = &mozilla::kNfcServiceModule;
