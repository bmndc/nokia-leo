/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

/*
 * The TV interface gives you access to the low-level TV code in a
 * platform-independent manner. The interfaces in this file allow for starting
 * an stopping the TV driver. Specific functionality is implemented in
 * sub-interfaces.
 */

#ifndef mozilla_ipc_TVDaemonInterface_h
#define mozilla_ipc_TVDaemonInterface_h

#include <mozilla/ipc/DaemonRunnables.h>
#include <mozilla/ipc/DaemonSocketConsumer.h>
#include <mozilla/ipc/DaemonSocketMessageHandlers.h>
#include <mozilla/ipc/ListenSocketConsumer.h>
#include "mozilla/UniquePtr.h"
#include "TVHelpers.h"

namespace mozilla {
namespace ipc {

class DaemonSocket;
class ListenSocket;

}
}

namespace mozilla {
namespace dom {

using mozilla::UniquePtr;

class TVDaemonProtocol;
class RegistryInterface;
class DTVInterface;

/**
 * This class is the result-handler interface for the TV
 * interface. Methods always run on the main thread.
 */
class TVDaemonResultHandler : public mozilla::ipc::DaemonSocketResultHandler
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(TVDaemonResultHandler)

public:
  /**
   * Called if a command failed.
   *
   * @param aError The error code.
   */
  virtual void OnError(TVStatus aError);

  /**
   * The callback method for |TVDaemonInterface::Connect|.
   */
  virtual void Connect();

  /**
   * The callback method for |TVDaemonInterface::Connect|.
   */
  virtual void Disconnect();

protected:
  virtual ~TVDaemonResultHandler();
};

/**
 * This is the notification-handler interface. Implement this classes
 * methods to handle event and notifications from the tv daemon.
 * All methods run on the main thread.
 */
class TVDaemonNotificationHandler
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(TVDaemonNotificationHandler)

public:
  /**
   * This notification is called when the backend code fails
   * unexpectedly. Save state in the high-level code and restart
   * the driver.
   *
   * @param aCrash True is the tv driver crashed.
   */
  virtual void BackendErrorNotification(bool aCrashed);

protected:
  virtual ~TVDaemonNotificationHandler();
};

/**
 * This class implements the public interface to the TV functionality and
 * driver. Use |TVDaemonInterface::GetInstance| to retrieve an instance.
 * All methods run on the main thread.
 */
class TVDaemonInterface final : public mozilla::ipc::DaemonSocketConsumer,
                                public mozilla::ipc::ListenSocketConsumer
{
public:
  TVDaemonInterface();

  ~TVDaemonInterface();

  /**
   * Returns an instance of the TV backend. This code can return |nullptr|
   * if no TV backend is available.
   *
   * @return An instance of |TVDaemonInterface|.
   */
  static TVDaemonInterface* GetInstance();

  /**
   * This method sets the notification handler for TV notifications. Call
   * this method immediately after retreiving an instance of the class, or you
   * won't be able able to receive notifications. You may not free the handler
   * class while the TV backend is connected.
   *
   * @param aNotificationHandler An instance of a notification handler.
   */
  void SetNotificationHandler(
    TVDaemonNotificationHandler* aNotificationHandler);

  /**
   * This method starts the TV backend and establishes ad connection with
   * Gecko. This is a multi-step process and errors are signalled by
   * |TVDaemonNotificationHandler::BackendErrorNotification|. If you see
   * this notification before the connection has been established, it's
   * certainly best to assume the TV backend to be not evailable.
   *
   * @param aRes The result handler.
   */
  void Connect(TVDaemonNotificationHandler* aNotificationHandler,
               TVDaemonResultHandler* aRes);

  /**
   * This method disconnects Gecko from the TV backend and frees the
   * backend's resources. This will invalidate all interfaces and
   * state. Don't use any TV functionality without reconnecting first.
   *
   * @param aRes The result handler.
   */
  void Disconnect(TVDaemonResultHandler* aRes);

  /**
   * Returns the Registry interface for the connected TV backend.
   *
   * @return An instance of the TV Registry interface.
   */
  RegistryInterface* GetTVRegistryInterface();

  /**
   * Returns the DTV interface for the connected TV backend.
   *
   * @return An instance of the TV DTV interface.
   */
  DTVInterface* GetTVDTVInterface();

private:
  class StartDaemonTask;

  enum Channel { LISTEN_SOCKET, DATA_SOCKET };

  typedef mozilla::ipc::DaemonResultRunnable1<TVDaemonResultHandler, void,
                                              TVStatus, TVStatus> ErrorRunnable;

  void DispatchError(TVDaemonResultHandler* aRes, TVStatus aError);
  void DispatchError(TVDaemonResultHandler* aRes, nsresult aRv);

  static bool IsDaemonRunning();

  // Methods for |DaemonSocketConsumer| and |ListenSocketConsumer|
  //
  void OnConnectSuccess(int aIndex) override;
  void OnConnectError(int aIndex) override;
  void OnDisconnect(int aIndex) override;

  nsCString mListenSocketName;
  RefPtr<mozilla::ipc::ListenSocket> mListenSocket;
  RefPtr<mozilla::ipc::DaemonSocket> mDataSocket;

  UniquePtr<TVDaemonProtocol> mProtocol;

  UniquePtr<RegistryInterface> mRegistryInterface;
  UniquePtr<DTVInterface> mDTVInterface;

  nsTArray<RefPtr<TVDaemonResultHandler> > mResultHandlerQ;
  RefPtr<TVDaemonNotificationHandler> mNotificationHandler;
};

} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_TVDaemonInterface_h
