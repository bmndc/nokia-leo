/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

/*
 * The registry interface gives yo access to the TV daemon's Registry
 * service. The purpose of the service is to register and setup all other
 * services, and make them available.
 *
 * All public methods and callback methods run on the main thread.
 */

#ifndef mozilla_ipc_RegistryInterface_h
#define mozilla_ipc_RegistryInterface_h

#include <mozilla/ipc/DaemonRunnables.h>
#include <mozilla/ipc/DaemonSocketMessageHandlers.h>
#include "TVHelpers.h"

namespace mozilla {
namespace ipc {

class DaemonSocketPDU;
class DaemonSocketPDUHeader;

}
}

namespace mozilla {
namespace dom {

class TVInterface;

using mozilla::ipc::DaemonSocketPDU;
using mozilla::ipc::DaemonSocketPDUHeader;
using mozilla::ipc::DaemonSocketResultHandler;

/**
 * This class is the result-handler interface for the TV
 * Registry interface. Methods always run on the main thread.
 */
class RegistryResultHandler : public DaemonSocketResultHandler
{
public:
  /**
   * Called if a registry command failed.
   *
   * @param aError The error code.
   */
  virtual void OnError(TVStatus aError);

  /**
   * The callback method for |RegistryInterface::RegisterModule|.
   */
  virtual void RegisterModule(void);

  /**
   * The callback method for |TVRegsitryInterface::UnregisterModule|.
   */
  virtual void UnregisterModule();

protected:
  virtual ~RegistryResultHandler();
};

/**
 * This is the module class for the TV registry component. It handles
 * PDU packing and unpacking. Methods are either executed on the main thread
 * or the I/O thread.
 *
 * This is an internal class, use |RegistryInterface| instead.
 */
class RegistryModule
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(RegistryModule)

public:
  enum { SERVICE_ID = 0x00 };

  enum {
    OPCODE_ERROR = 0x00,
    OPCODE_REGISTER_MODULE = 0x01,
    OPCODE_UNREGISTER_MODULE = 0x02
  };

  virtual nsresult Send(DaemonSocketPDU* aPDU,
                        DaemonSocketResultHandler* aRes) = 0;

  //
  // Commands
  //
  nsresult RegisterModuleCmd(uint8_t aId, RegistryResultHandler* aRes);

  nsresult UnregisterModuleCmd(uint8_t aId, RegistryResultHandler* aRes);

protected:
  virtual ~RegistryModule();

  void HandleSvc(const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
                 DaemonSocketResultHandler* aRes);

  //
  // Responses
  //
  typedef mozilla::ipc::DaemonResultRunnable0<RegistryResultHandler, void>
  ResultRunnable;

  typedef mozilla::ipc::DaemonResultRunnable1<RegistryResultHandler, void,
                                              TVStatus, TVStatus> ErrorRunnable;

  void ErrorRsp(const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
                RegistryResultHandler* aRes);

  void RegisterModuleRsp(const DaemonSocketPDUHeader& aHeader,
                         DaemonSocketPDU& aPDU, RegistryResultHandler* aRes);

  void UnregisterModuleRsp(const DaemonSocketPDUHeader& aHeader,
                           DaemonSocketPDU& aPDU, RegistryResultHandler* aRes);
};

/**
 * This class implements the public interface to the TV Registry
 * component. Use |TVInterface::GetRegistryInterface| to retrieve
 * an instance. All methods run on the main thread.
 */
class RegistryInterface final
{
public:
  friend class TVDaemonInterface;

  RegistryInterface(already_AddRefed<RegistryModule> aModule);

  ~RegistryInterface();

  /**
   * Sends a RegisterModule command to the TV daemon. When the
   * result handler's |RegisterModule| method gets called, the service
   * has been registered successfully and can be used.
   *
   * @param aId The id of the service that is to be registered.
   * @param aRes The result handler.
   */
  void RegisterModule(uint8_t aId, RegistryResultHandler* aRes);

  /**
   * Sends an UnregisterModule command to the TV daemon. The service
   * should not be used afterwards until it has been registered again.
   *
   * @param aId The id of the service that is to be unregistered.
   * @param aRes The result handler.
   */
  void UnregisterModule(uint8_t aId, RegistryResultHandler* aRes);

private:
  void DispatchError(RegistryResultHandler* aRes, TVStatus aError);

  void DispatchError(RegistryResultHandler* aRes, nsresult aRv);

  typedef mozilla::ipc::DaemonResultRunnable1<RegistryResultHandler, void,
                                              TVStatus, TVStatus> ErrorRunnable;

  RefPtr<RegistryModule> mModule;
};

} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_RegistryInterface_h
