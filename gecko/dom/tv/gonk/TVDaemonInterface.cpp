/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "TVDaemonInterface.h"

#include <cutils/properties.h>

#include "DTVInterface.h"
#include "mozilla/Hal.h"
#include "mozilla/ipc/DaemonSocket.h"
#include "mozilla/ipc/DaemonSocketConnector.h"
#include "mozilla/ipc/DaemonSocketConsumer.h"
#include "mozilla/ipc/DaemonSocketPDUHelpers.h"
#include "mozilla/ipc/ListenSocket.h"
#include "RegistryInterface.h"
#include "TVHelpers.h"
#include "TVLog.h"

namespace mozilla {
namespace dom {

using mozilla::dom::TVStatus;
using mozilla::ipc::DaemonSocket;
using mozilla::ipc::DaemonSocketConnector;
using mozilla::ipc::DaemonSocketIOConsumer;
using mozilla::ipc::DaemonSocketPDUHelpers::ConstantInitOp1;
using mozilla::ipc::DaemonSocketPDUHelpers::UnpackPDU;
using mozilla::ipc::ListenSocket;
using mozilla::ipc::SocketConnectionStatus;

//
// TVDaemonResultHandler
//
void
TVDaemonResultHandler::OnError(TVStatus aError)
{
  TV_LOG("Received error code %d", static_cast<int>(aError));
}

void
TVDaemonResultHandler::Connect() {}

void
TVDaemonResultHandler::Disconnect() {}

TVDaemonResultHandler::~TVDaemonResultHandler() {}

//
// TVDaemonNotificationHandler
//
void
TVDaemonNotificationHandler::BackendErrorNotification(bool aCrashed)
{
  if (aCrashed) {
    NS_WARNING("TV backend crashed");
  } else {
    NS_WARNING("Error in TV backend");
  }
}

TVDaemonNotificationHandler::~TVDaemonNotificationHandler() {}

//
// TVDaemonProtocol
//
class TVDaemonProtocol final : public DaemonSocketIOConsumer,
                               public RegistryModule,
                               public DTVModule
{
public:
  TVDaemonProtocol();

  void SetConnection(DaemonSocket* aConnection);

  already_AddRefed<DaemonSocketResultHandler> FetchResultHandler(
    const DaemonSocketPDUHeader& aHeader);

  // Methods for |TVRegistryModule| and |TVDTVModule|
  //

  nsresult Send(DaemonSocketPDU* aPDU,
                DaemonSocketResultHandler* aRes) override;

  // Methods for |DaemonSocketIOConsumer|
  //

  void Handle(DaemonSocketPDU& aPDU) override;
  void StoreResultHandler(const DaemonSocketPDU& aPDU) override;

private:
  void HandleRegistrySvc(const DaemonSocketPDUHeader& aHeader,
                         DaemonSocketPDU& aPDU,
                         DaemonSocketResultHandler* aRes);
  void HandleDTVSvc(const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
                    DaemonSocketResultHandler* aRes);

  RefPtr<DaemonSocket> mConnection;
  nsTArray<RefPtr<DaemonSocketResultHandler>> mResultHandlerQ;
};

TVDaemonProtocol::TVDaemonProtocol() {}

void
TVDaemonProtocol::SetConnection(DaemonSocket* aConnection)
{
  mConnection = aConnection;
}

already_AddRefed<DaemonSocketResultHandler>
TVDaemonProtocol::FetchResultHandler(const DaemonSocketPDUHeader& aHeader)
{
  MOZ_ASSERT(!NS_IsMainThread());

  if (aHeader.mOpcode & OPCODE_NTF_FILTER) {
    return nullptr; // Ignore notifications
  }

  RefPtr<DaemonSocketResultHandler> res = mResultHandlerQ.ElementAt(0);
  mResultHandlerQ.RemoveElementAt(0);

  return res.forget();
}

void
TVDaemonProtocol::HandleRegistrySvc(const DaemonSocketPDUHeader& aHeader,
                                    DaemonSocketPDU& aPDU,
                                    DaemonSocketResultHandler* aRes)
{
  RegistryModule::HandleSvc(aHeader, aPDU, aRes);
}

void
TVDaemonProtocol::HandleDTVSvc(const DaemonSocketPDUHeader& aHeader,
                               DaemonSocketPDU& aPDU,
                               DaemonSocketResultHandler* aRes)
{
  DTVModule::HandleSvc(aHeader, aPDU, aRes);
}

// |TVRegistryModule|, |TVDTVModule|

nsresult
TVDaemonProtocol::Send(DaemonSocketPDU* aPDU, DaemonSocketResultHandler* aRes)
{
  MOZ_ASSERT(mConnection);
  MOZ_ASSERT(aPDU);

  if (mConnection->GetConnectionStatus() ==
      SocketConnectionStatus::SOCKET_DISCONNECTED) {
    NS_WARNING("TV socket is disconnected");
    return NS_ERROR_FAILURE;
  }

  aPDU->SetConsumer(this);
  aPDU->SetResultHandler(aRes);
  aPDU->UpdateHeader();

  mConnection->SendSocketData(aPDU); // Forward PDU to data channel

  return NS_OK;
}

// |DaemonSocketIOConsumer|

void
TVDaemonProtocol::Handle(DaemonSocketPDU& aPDU)
{
  static void (TVDaemonProtocol::*const HandleSvc[])(
    const DaemonSocketPDUHeader&, DaemonSocketPDU&,
    DaemonSocketResultHandler*) = {[RegistryModule::SERVICE_ID] =
                                     &TVDaemonProtocol::HandleRegistrySvc,
                                   [DTVModule::SERVICE_ID] =
                                     &TVDaemonProtocol::HandleDTVSvc };

  DaemonSocketPDUHeader header;

  if (NS_FAILED(UnpackPDU(aPDU, header))) {
    return;
  }
  if (!(header.mService < MOZ_ARRAY_LENGTH(HandleSvc)) ||
      !HandleSvc[header.mService]) {
    TV_LOG("TV service %d unknown", header.mService);
    return;
  }

  RefPtr<DaemonSocketResultHandler> res = FetchResultHandler(header);

  (this->*(HandleSvc[header.mService]))(header, aPDU, res);
}

void
TVDaemonProtocol::StoreResultHandler(const DaemonSocketPDU& aPDU)
{
  MOZ_ASSERT(!NS_IsMainThread());

  mResultHandlerQ.AppendElement(aPDU.GetResultHandler());
}

//
// TVDaemonInterface
//

#define BASE_SOCKET_NAME "tvd"
#define POSTFIX_LENGTH 16

/*static*/
TVDaemonInterface*
TVDaemonInterface::GetInstance()
{
  NS_ASSERTION(NS_IsMainThread(), "Main thread only");

  static UniquePtr<TVDaemonInterface> sInterface;

  if (!sInterface) {
    sInterface = MakeUnique<TVDaemonInterface>();
  }

  return sInterface.get();
}

void
TVDaemonInterface::SetNotificationHandler(
  TVDaemonNotificationHandler* aNotificationHandler)
{
  MOZ_ASSERT(NS_IsMainThread());

  mNotificationHandler = aNotificationHandler;
}

/*
 * The connect procedure consists of several steps.
 *
 *  (1) Start listening for the command channel's socket connection: We
 *      do this before anything else, so that we don't miss connection
 *      requests from the TV daemon. This step will create a listen
 *      socket.
 *
 *  (2) Start the TV daemon: When the daemon starts up it will open
 *      a socket connection to Gecko and thus create the data channel.
 *      Gecko already opened the listen socket in step (1). Step (2) ends
 *      with the creation of the data channel.
 *
 *  (3) Signal success to the caller.
 *
 * If any step fails, we roll-back the procedure and signal an error to the
 * caller.
 */
void
TVDaemonInterface::Connect(TVDaemonNotificationHandler* aNotificationHandler,
                           TVDaemonResultHandler* aRes)
{
  // If we could not cleanup properly before and an old
  // instance of the daemon is still running, we kill it
  // here.
  mozilla::hal::StopSystemService(BASE_SOCKET_NAME);

  mNotificationHandler = aNotificationHandler;

  mResultHandlerQ.AppendElement(aRes);

  if (!mProtocol) {
    mProtocol = MakeUnique<TVDaemonProtocol>();
  }

  if (!mListenSocket) {
    mListenSocket = new ListenSocket(this, LISTEN_SOCKET);
  }

  // Init, step 1: Listen for data channel... */

  if (!mDataSocket) {
    mDataSocket = new DaemonSocket(mProtocol.get(), this, DATA_SOCKET);
  } else if (mDataSocket->GetConnectionStatus() ==
             SocketConnectionStatus::SOCKET_CONNECTED) {
    // Command channel should not be open; let's close it.
    mDataSocket->Close();
  }

  // The listen socket's name is generated with a random postfix. This
  // avoids naming collisions if we still have a listen socket from a
  // previously failed cleanup. It also makes it hard for malicious
  // external programs to capture the socket name or connect before
  // the daemon can do so. If no random postfix can be generated, we
  // simply use the base name as-is.
  nsresult rv = DaemonSocketConnector::CreateRandomAddressString(
    NS_LITERAL_CSTRING(BASE_SOCKET_NAME), POSTFIX_LENGTH, mListenSocketName);
  if (NS_FAILED(rv)) {
    mListenSocketName.AssignLiteral(BASE_SOCKET_NAME);
  }

  rv = mListenSocket->Listen(new DaemonSocketConnector(mListenSocketName),
                             mDataSocket);
  if (NS_FAILED(rv)) {
    OnConnectError(DATA_SOCKET);
    return;
  }

  // The protocol implementation needs a data channel for
  // sending commands to the daemon. We set it here, because
  // this is the earliest time when it's available.
  mProtocol->SetConnection(mDataSocket);
}

/*
 * Disconnecting is inverse to connecting.
 *
 *  (1) Close data socket: We close the data channel and the daemon will
 *      will notice. Once we see the socket's disconnect, we continue with
 *      the cleanup.
 *
 *  (2) Close listen socket: The listen socket is not active any longer
 *      and we simply close it.
 *
 *  (3) Signal success to the caller.
 *
 * We don't have to stop the daemon explicitly. It will cleanup and quit
 * after it noticed the closing of the data channel
 *
 * Rolling back half-completed cleanups is not possible. In the case of
 * an error, we simply push forward and try to recover during the next
 * initialization.
 */
void
TVDaemonInterface::Disconnect(TVDaemonResultHandler* aRes)
{
  mNotificationHandler = nullptr;

  // Cleanup, step 1: Close data channel
  mDataSocket->Close();

  mResultHandlerQ.AppendElement(aRes);
}

RegistryInterface*
TVDaemonInterface::GetTVRegistryInterface()
{
  NS_ASSERTION(NS_IsMainThread(), "Main thread only");

  if (!mRegistryInterface) {
    RefPtr<RegistryModule> module = mProtocol.get();
    mRegistryInterface = MakeUnique<RegistryInterface>(module.forget());
  }

  return mRegistryInterface.get();
}

DTVInterface*
TVDaemonInterface::GetTVDTVInterface()
{
  NS_ASSERTION(NS_IsMainThread(), "Main thread only");

  if (!mDTVInterface) {
    RefPtr<DTVModule> module = mProtocol.get();
    mDTVInterface = MakeUnique<DTVInterface>(module.forget());
  }

  return mDTVInterface.get();
}

TVDaemonInterface::TVDaemonInterface() : mNotificationHandler(nullptr) {}

TVDaemonInterface::~TVDaemonInterface() {}

void
TVDaemonInterface::DispatchError(TVDaemonResultHandler* aRes, TVStatus aError)
{
  ErrorRunnable::Dispatch(aRes, &TVDaemonResultHandler::OnError,
                          ConstantInitOp1<TVStatus>(aError));
}

void
TVDaemonInterface::DispatchError(TVDaemonResultHandler* aRes, nsresult aRv)
{
  TVStatus status;

  if (NS_WARN_IF(NS_FAILED(Convert(aRv, status)))) {
    status = TVStatus::STATUS_FAILURE;
  }

  DispatchError(aRes, status);
}

// |DaemonSocketConsumer|, |ListenSocketConsumer|
void
TVDaemonInterface::OnConnectSuccess(int aIndex)
{
  nsresult rv;
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!mResultHandlerQ.IsEmpty());

  switch (aIndex) {
    case LISTEN_SOCKET: {
      // Init, step 2: Start TV daemon
      nsCString args("-a ");
      args.Append(mListenSocketName);
      rv = mozilla::hal::StartSystemService(BASE_SOCKET_NAME, args.get());
      if (NS_WARN_IF(rv != NS_OK)) {
        OnConnectError(DATA_SOCKET);
      }
      break;
    }
    case DATA_SOCKET:
      if (!mResultHandlerQ.IsEmpty()) {
        // Init, step 3: Signal success
        RefPtr<TVDaemonResultHandler> res = mResultHandlerQ.ElementAt(0);
        mResultHandlerQ.RemoveElementAt(0);
        if (res) {
          res->Connect();
        }
      }
      break;
  }
}

void
TVDaemonInterface::OnConnectError(int aIndex)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!mResultHandlerQ.IsEmpty());

  switch (aIndex) {
    case DATA_SOCKET:
      // Stop daemon and close listen socket
      mozilla::hal::StopSystemService(BASE_SOCKET_NAME);
      mListenSocket->Close();
      // Fall through
    case LISTEN_SOCKET:
      if (!mResultHandlerQ.IsEmpty()) {
        // Signal error to caller
        RefPtr<TVDaemonResultHandler> res = mResultHandlerQ.ElementAt(0);
        mResultHandlerQ.RemoveElementAt(0);
        if (res) {
          DispatchError(res, TVStatus::STATUS_FAILURE);
        }
      }
      break;
  }
}

/*
 * Disconnects can happend
 *
 *  (a) during startup,
 *  (b) during regular service, or
 *  (c) during shutdown.
 *
 * For cases (a) and (c), |mResultHandlerQ| contains an element. For
 * case (b) |mResultHandlerQ| will be empty. This distinguishes a crash in
 * the daemon. The following procedure to recover from crashes consists of
 * several steps for case (b).
 *
 *  (1) Close listen socket.
 *  (2) Wait for all sockets to be disconnected and inform caller about
 *      the crash.
 *  (3) After all resources have been cleaned up, let the caller restart
 *      the daemon.
 */
void
TVDaemonInterface::OnDisconnect(int aIndex)
{
  MOZ_ASSERT(NS_IsMainThread());

  switch (aIndex) {
    case DATA_SOCKET:
      // Cleanup, step 2 (Recovery, step 1): Close listen socket
      mListenSocket->Close();
      break;
    case LISTEN_SOCKET:
      // Cleanup, step 3: Signal success to caller
      if (!mResultHandlerQ.IsEmpty()) {
        RefPtr<TVDaemonResultHandler> res = mResultHandlerQ.ElementAt(0);
        mResultHandlerQ.RemoveElementAt(0);
        if (res) {
          res->Disconnect();
        }
      }
      break;
  }

  /* For recovery make sure all sockets disconnected, in order to avoid
   * the remaining disconnects interfere with the restart procedure.
   */
  if (mNotificationHandler && mResultHandlerQ.IsEmpty()) {
    if (mListenSocket->GetConnectionStatus() ==
          SocketConnectionStatus::SOCKET_DISCONNECTED &&
        mDataSocket->GetConnectionStatus() ==
          SocketConnectionStatus::SOCKET_DISCONNECTED) {
      // Recovery, step 2: Notify the caller to prepare the restart procedure.
      mNotificationHandler->BackendErrorNotification(true);
      mNotificationHandler = nullptr;
    }
  }
}

} // namespace dom
} // namespace mozilla
