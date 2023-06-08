/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

/*
 * The DTV interface gives yo access to the TV daemon's DTV service,
 * which handles TV. The DTV service will inform you when TV are
 * detected or removed from the system. You can activate (or deactivate)
 * existing TV and DTV will deliver the TV' events.
 *
 * All public methods and callback methods run on the main thread.
 */

#ifndef mozilla_dom_DTVInterface_h
#define mozilla_dom_DTVInterface_h

#include "mozilla/ipc/DaemonRunnables.h"
#include "mozilla/ipc/DaemonSocketMessageHandlers.h"
#include "mozilla/ipc/DaemonSocketPDUHelpers.h"
#include "TVHelpers.h"

namespace mozilla {
namespace ipc {

class DaemonSocketPDU;
class DaemonSocketPDUHeader;

}
}

namespace mozilla {
namespace dom {

using mozilla::ipc::DaemonSocketPDU;
using mozilla::ipc::DaemonSocketPDUHeader;
using mozilla::ipc::DaemonSocketPDUHelpers::PDUInitOp;
using mozilla::ipc::DaemonSocketResultHandler;

class TVInterface;

/**
 * This class is the result-handler interface for the TV
 * DTV interface. Methods always run on the main thread.
 */
class DTVResultHandler : public DaemonSocketResultHandler
{
public:
  explicit DTVResultHandler(nsITVServiceCallback* aCallback);

  /**
   * Called if a DTV command failed.
   *
   * @param aError The error code.
   */
  void OnError(TVStatus aStatus);

  void OnGetTunersResponse(const nsTArray<struct TunerData>& aTunerDataList);

  void OnSetSourceResponse(native_handle_t* aHandle);

  void OnStartScanningChannelsResponse();

  void OnStopScanningChannelsResponse();

  void OnSetChannelResponse(struct ChannelData aChannelData);

  void OnGetChannelsResponse(
    const nsTArray<struct ChannelData>& aChannelDataList);

  void OnGetProgramsResponse(
    const nsTArray<struct ProgramData>& aProgramDataList);

protected:
  ~DTVResultHandler() {}

  nsCOMPtr<nsITVServiceCallback> mCallback;
};

/**
 * This is the notification-handler interface. Implement this classes
 * methods to handle event and notifications from the TV daemon.
 */
class DTVNotificationHandler
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(DTVNotificationHandler)

public:

  //
  // Notifications
  //

  virtual void OnError(TVStatus& aError);

  virtual void OnChannelScanned(const nsAString& aTunerId,
                                const nsAString& aSourceType,
                                struct ChannelData aChannelData);

  virtual void OnChannelScanComplete(const nsAString& aTunerId,
                                     const nsAString& aSourceType);

  virtual void OnChannelScanStopped(const nsAString& aTunerId,
                                    const nsAString& aSourceType);

  virtual void OnEITBroadcasted(
    const nsAString& aTunerId, const nsAString& aSourceType,
    struct ChannelData aChannelData,
    const nsTArray<struct ProgramData>& aProgramDataList);

protected:
  virtual ~DTVNotificationHandler();
};

/**
 * This is the module class for the TV DTV component. It handles PDU
 * packing and unpacking. Methods are either executed on the main thread or
 * the I/O thread.
 *
 * This is an internal class, use |DTVInterface| instead.
 */
class DTVModule
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(DTVModule)
public:
  class UnpackTVDataInitOp final : private PDUInitOp
  {
  public:
    UnpackTVDataInitOp(DaemonSocketPDU& aPDU) : PDUInitOp(aPDU) {}

    nsresult operator()(nsTArray<struct TunerData>& aArg1) const;

    nsresult operator()(struct ChannelData& aArg1) const;

    nsresult operator()(nsTArray<struct ChannelData>& aArg1) const;

    nsresult operator()(nsTArray<struct ProgramData>& aArg1) const;

    nsresult operator()(native_handle_t*& aArg1) const;
  };

  class UnpackTVNotificationInitOp final : private PDUInitOp
  {
  public:
    UnpackTVNotificationInitOp(DaemonSocketPDU& aPDU) : PDUInitOp(aPDU) {}
    nsresult operator()(TVStatus& aArg1) const;

    nsresult operator()(nsString& aArg1, nsString& aArg2) const;

    nsresult operator()(nsString& aArg1, nsString& aArg2,
                        struct ChannelData& aArg3) const;

    nsresult operator()(nsString& aArg1, nsString& aArg2,
                        struct ChannelData& aArg3,
                        nsTArray<struct ProgramData>& aArg4) const;
  };

  class NotificationHandlerWrapper;

  enum { SERVICE_ID = 0x01 };

  enum {
    OPCODE_ERROR = 0x00,
    OPCODE_GET_TUNERS = 0x01,
    OPCODE_SET_SOURCE = 0x02,
    OPCODE_START_SCANNING_CHANNELS = 0x03,
    OPCODE_STOP_SCANNING_CHANNELS = 0x04,
    OPCODE_CLEAR_SCANNED_CHANNELS_CACHE = 0x05,
    OPCODE_SET_CHANNEL = 0x06,
    OPCODE_GET_CHANNELS = 0x07,
    OPCODE_GET_PROGRAMS = 0x08,
    OPCODE_ERROR_NOTIFICATION = 0x80,
    OPCODE_CHANNEL_SCANNED_NOTIFICATION = 0x81,
    OPCODE_CHANNEL_SCAN_COMPLETE_NOTIFICATION = 0x82,
    OPCODE_CHANNEL_SCAN_STOPPED_NOTIFICATION = 0x83,
    OPCODE_EIT_BROADCASTED_NOTIFICATION = 0x84,
  };

  virtual nsresult Send(DaemonSocketPDU* aPDU,
                        DaemonSocketResultHandler* aRes) = 0;

  //
  // Commands
  //

  nsresult GetTunersCommand(DTVResultHandler* aResultHandler);

  nsresult SetSourceCommand(const nsAString& aTunerId,
                            const nsAString& aSourceType,
                            DTVResultHandler* aResultHandler);

  nsresult StartScanningChannelsCommand(const nsAString& aTunerId,
                                        const nsAString& aSourceType,
                                        DTVResultHandler* aResultHandler);

  nsresult StopScanningChannelsCommand(const nsAString& aTunerId,
                                       const nsAString& aSourceType,
                                       DTVResultHandler* aResultHandler);

  nsresult ClearScannedChannelsCacheCommand(DTVResultHandler* aResultHandler);

  nsresult SetChannelCommand(const nsAString& aTunerId,
                             const nsAString& aSourceType,
                             const nsAString& aChannelNumber,
                             DTVResultHandler* aResultHandler);

  nsresult GetChannelsCommand(const nsAString& aTunerId,
                              const nsAString& aSourceType,
                              DTVResultHandler* aResultHandler);

  nsresult GetProgramsCommand(const nsAString& aTunerId,
                              const nsAString& aSourceType,
                              const nsAString& aChannelNumber,
                              uint64_t aStartTime, uint64_t aEndTime,
                              DTVResultHandler* aResultHandler);

protected:
  DTVModule();
  virtual ~DTVModule();

  void HandleSvc(const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
                 DaemonSocketResultHandler* aRes);

private:

  //
  // Responses
  //

  typedef mozilla::ipc::DaemonResultRunnable1<DTVResultHandler, void, TVStatus,
                                              TVStatus> TVErrorRunnable;

  typedef mozilla::ipc::DaemonResultRunnable0<DTVResultHandler, void>
  TVResultRunnable;

  typedef mozilla::ipc::DaemonResultRunnable1<
    DTVResultHandler, void, nsTArray<struct TunerData>,
    const nsTArray<struct TunerData>&> GetTunersResultRunnable;

  typedef mozilla::ipc::DaemonResultRunnable1<
    DTVResultHandler, void, native_handle_t*, native_handle_t*>
  SetSourceResultRunnable;

  typedef mozilla::ipc::DaemonResultRunnable1<
    DTVResultHandler, void, struct ChannelData, struct ChannelData>
  SetChannelResultRunnable;

  typedef mozilla::ipc::DaemonResultRunnable1<
    DTVResultHandler, void, nsTArray<struct ChannelData>,
    const nsTArray<struct ChannelData>&> GetChannelsResultRunnable;

  typedef mozilla::ipc::DaemonResultRunnable1<
    DTVResultHandler, void, nsTArray<struct ProgramData>,
    const nsTArray<struct ProgramData>&> GetProgramsResultRunnable;

  void ErrorResponse(const DaemonSocketPDUHeader& aHeader,
                     DaemonSocketPDU& aPDU, DTVResultHandler* aResultHandler);

  void GetTunersResponse(const DaemonSocketPDUHeader& aHeader,
                         DaemonSocketPDU& aPDU,
                         DTVResultHandler* aResultHandler);

  void SetSourceResponse(const DaemonSocketPDUHeader& aHeader,
                         DaemonSocketPDU& aPDU,
                         DTVResultHandler* aResultHandler);

  void StartScanningChannelsResponse(const DaemonSocketPDUHeader& aHeader,
                                     DaemonSocketPDU& aPDU,
                                     DTVResultHandler* aResultHandler);

  void StopScanningChannelsResponse(const DaemonSocketPDUHeader& aHeader,
                                    DaemonSocketPDU& aPDU,
                                    DTVResultHandler* aResultHandler);

  void ClearScannedChannelsCacheResponse(const DaemonSocketPDUHeader& aHeader,
                                         DaemonSocketPDU& aPDU,
                                         DTVResultHandler* aResultHandler);

  void SetChannelResponse(const DaemonSocketPDUHeader& aHeader,
                          DaemonSocketPDU& aPDU,
                          DTVResultHandler* aResultHandler);

  void GetChannelsResponse(const DaemonSocketPDUHeader& aHeader,
                           DaemonSocketPDU& aPDU,
                           DTVResultHandler* aResultHandler);

  void GetProgramsResponse(const DaemonSocketPDUHeader& aHeader,
                           DaemonSocketPDU& aPDU,
                           DTVResultHandler* aResultHandler);

  void HandleRsp(const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
                 DaemonSocketResultHandler* aRes);

  //
  // Notifications
  //

  typedef mozilla::ipc::DaemonNotificationRunnable1<NotificationHandlerWrapper,
                                                    void, TVStatus, TVStatus&>
  ErrorNotificationRunnable;

  typedef mozilla::ipc::DaemonNotificationRunnable3<
    NotificationHandlerWrapper, void, nsString, nsString, struct ChannelData,
    const nsAString&, const nsAString&,
    struct ChannelData> ScannedChannelNotificationRunnable;

  typedef mozilla::ipc::DaemonNotificationRunnable2<
    NotificationHandlerWrapper, void, nsString, nsString, const nsAString&,
    const nsAString&> ChannelScanNotificationRunnable;

  typedef mozilla::ipc::DaemonNotificationRunnable4<
    NotificationHandlerWrapper, void, nsString, nsString, struct ChannelData,
    nsTArray<struct ProgramData>, const nsAString&, const nsAString&,
    struct ChannelData,
    const nsTArray<struct ProgramData>&> EITBroadcastedNotificationRunnable;

  void ErrorNotification(const DaemonSocketPDUHeader& aHeader,
                         DaemonSocketPDU& aPDU);

  void ChannelScannedNotification(const DaemonSocketPDUHeader& aHeader,
                                  DaemonSocketPDU& aPDU);

  void ChannelScanCompleteNotification(const DaemonSocketPDUHeader& aHeader,
                                       DaemonSocketPDU& aPDU);

  void ChannelScanStoppedNotification(const DaemonSocketPDUHeader& aHeader,
                                      DaemonSocketPDU& aPDU);

  void EITBroadcastedNotification(const DaemonSocketPDUHeader& aHeader,
                                  DaemonSocketPDU& aPDU);

  void HandleNtf(const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
                 DaemonSocketResultHandler* aRes);
};

/**
 * This class implements the public interface to the TV DTV
 * component. Use |TVInterface::GetDTVInterface| to retrieve
 * an instance. All methods run on the main thread.
 */
class DTVInterface final
{
public:
  friend class TVDaemonInterface;

  DTVInterface(already_AddRefed<DTVModule> aModule);

  ~DTVInterface();

  /**
   * This method sets the notification handler for DTV notifications. Call
   * this method immediately after registering the module. Otherwise you won't
   * be able able to receive DTV notifications. You may not free the handler
   * class while the DTV component is regsitered.
   *
   * @param aNotificationHandler An instance of a DTV notification handler.
   */
  void SetNotificationHandler(DTVNotificationHandler* aNotificationHandler);

  void GetTuners(DTVResultHandler* aResultHandler);

  void SetSource(const nsAString& aTunerId, const nsAString& aSourceType,
                 DTVResultHandler* aResultHandler);

  void StartScanningChannels(const nsAString& aTunerId,
                             const nsAString& aSourceType,
                             DTVResultHandler* aResultHandler);

  void StopScanningChannels(const nsAString& aTunerId,
                            const nsAString& aSourceType,
                            DTVResultHandler* aResultHandler);

  void ClearScannedChannelsCache(DTVResultHandler* aResultHandler);

  void SetChannel(const nsAString& aTunerId, const nsAString& aSourceType,
                  const nsAString& aChannelNumber,
                  DTVResultHandler* aResultHandler);

  void GetChannels(const nsAString& aTunerId, const nsAString& aSourceType,
                   DTVResultHandler* aResultHandler);

  void GetPrograms(const nsAString& aTunerId, const nsAString& aSourceType,
                   const nsAString& aChannelNumber, uint64_t startTime,
                   uint64_t endTime, DTVResultHandler* aResultHandler);

private:
  typedef mozilla::ipc::DaemonResultRunnable1<DTVResultHandler, void, TVStatus,
                                              TVStatus> TVErrorRunnable;

  void DispatchError(DTVResultHandler* aRes, TVStatus aError);
  void DispatchError(DTVResultHandler* aRes, nsresult aRv);

  RefPtr<DTVModule> mModule;
};

} // dom
} // namespace mozilla

#endif // mozilla_dom_DTVInterface_h
