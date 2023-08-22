/* -*- Mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; tab-width: 40 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_bluetooth_bluedroid_BluetoothMapSmsManager_h
#define mozilla_dom_bluetooth_bluedroid_BluetoothMapSmsManager_h

#include "BluetoothCommon.h"
#include "BluetoothInterface.h"
#include "BluetoothMapBMessage.h"
#include "BluetoothMapFolder.h"
#include "BluetoothProfileManagerBase.h"
#include "BluetoothSocketObserver.h"
#include "mozilla/ipc/SocketBase.h"
#include "mozilla/UniquePtr.h"
#include "ObexBase.h"

class nsIInputStream;

namespace mozilla {
  namespace dom {
    class Blob;
    class BlobParent;
  }
}

BEGIN_BLUETOOTH_NAMESPACE

struct Map {
  enum AppParametersTagId {
    MaxListCount                  = 0x1,
    StartOffset                   = 0x2,
    FilterMessageType             = 0x3,
    FilterPeriodBegin             = 0x4,
    FilterPeriodEnd               = 0x5,
    FilterReadStatus              = 0x6,
    FilterRecipient               = 0x7,
    FilterOriginator              = 0x8,
    FilterPriority                = 0x9,
    Attachment                    = 0x0A,
    Transparent                   = 0x0B,
    Retry                         = 0x0C,
    NewMessage                    = 0x0D,
    NotificationStatus            = 0x0E,
    MASInstanceId                 = 0x0F,
    ParameterMask                 = 0x10,
    FolderListingSize             = 0x11,
    MessagesListingSize           = 0x12,
    SubjectLength                 = 0x13,
    Charset                       = 0x14,
    FractionRequest               = 0x15,
    FractionDeliver               = 0x16,
    StatusIndicator               = 0x17,
    StatusValue                   = 0x18,
    MSETime                       = 0x19
  };
};

class BluetoothNamedValue;
class BluetoothSocket;
class ObexHeaderSet;

/*
 * BluetoothMapSmsManager acts as Message Server Equipment (MSE) and runs both
 * MAS server and MNS client to exchange SMS/MMS message.
 */

class BluetoothMapSmsManager : public BluetoothSocketObserver
                             , public BluetoothProfileManagerBase
                             , public BluetoothSdpNotificationHandler
{
public:
  BT_DECL_PROFILE_MGR_BASE
  BT_DECL_SOCKET_OBSERVER
  virtual void GetName(nsACString& aName)
  {
    aName.AssignLiteral("MapSms");
  }

  static const int MAX_PACKET_LENGTH = 0xFFFE;
  static const int MAX_INSTANCE_ID = 255;
  // SDP record for SupportedMessageTypes
  static const int SDP_MESSAGE_TYPE_EMAIL = 0x01;
  static const int SDP_MESSAGE_TYPE_SMS_GSM = 0x02;
  static const int SDP_MESSAGE_TYPE_SMS_CDMA = 0x04;
  static const int SDP_MESSAGE_TYPE_MMS = 0x08;
  // By defualt SMS/MMS is default supported
  static const int SDP_SMS_MMS_INSTANCE_ID = 0;

  static void InitMapSmsInterface(BluetoothProfileResultHandler* aRes);
  static void DeinitMapSmsInterface(BluetoothProfileResultHandler* aRes);
  static BluetoothMapSmsManager* Get();

  bool Listen();

  /**
   * Reply folder-listing object to the *IPC* 'folderlisting'
   *
   * @param aMasId [in]          MAS id
   * @param aFolderlists [in]    folder listing object
   *
   * @return true if the response packet has been packed correctly and started
   *         to be sent to the remote device; false otherwise.
   */
  bool ReplyToFolderListing(uint8_t aMasId, const nsAString& aFolderlists);

  /**
   * Reply message-listing object to the *IPC* 'messageslisting'
   *
   * @param aActor [in]          a blob actor containing message-listing objects
   * @param aMasId [in]          MAS id
   * @param aNewMessage [in]     indicate whether there are unread messages
   * @param aTimestamp [in]      time stamp
   * @param aSize [in]           total number of messages
   *
   * @return true if the response packet has been packed correctly and started
   *         to be sent to the remote device; false otherwise.
   */
  bool ReplyToMessagesListing(
    BlobParent* aActor, uint8_t aMasId, bool aNewMessage,
    const nsAString& aTimestamp, int aSize);

  /**
   * Reply messages-listing object to the *in-process* 'messageslisting' request
   *
   * @param aBlob [in]           a blob contained the vCard objects
   * @param aMasId [in]          MAS id
   * @param aNewMessage [in]     indicate whether there are unread messages
   * @param aTimestamp [in]      time stamp
   * @param aSize [in]           total number of messages
   *
   * @return true if the response packet has been packed correctly and started
   *         to be sent to the remote device; false otherwise.
   */
  bool ReplyToMessagesListing(
    Blob* aBlob, uint8_t aMasId, bool aNewMessage, const nsAString& aTimestamp,
    int aSize);

  /**
   * Reply bMessage object to the *IPC* 'getmessage' request.
   *
   * @param aActor [in]          a blob actor containing the bMessage object
   * @param aMasId [in]          MAS id
   *
   * @return true if the response packet has been packed correctly and started
   *         to be sent to the remote device; false otherwise.
   */
  bool ReplyToGetMessage(BlobParent* aActor, uint8_t aMasId);

  /**
   * Reply bMessage to the *in-process* 'getmessage' request.
   *
   * @param aBlob [in]          a blob containing the bMessage object
   * @param aMasId [in]         the number of vCard indexes in the blob
   *
   * @return true if the response packet has been packed correctly and started
   *         to be sent to the remote device; false otherwise.
   */
  bool ReplyToGetMessage(Blob* aBlob, uint8_t aMasId);

  /**
   * Reply to the *IPC* 'setmessage' request.
   *
   * @param aMasId [in]         MAS id
   * @param aStatus [in]        success or failure
   *
   * @return true if the response packet has been packed correctly and started
   *         to be sent to the remote device; false otherwise.
   */
  bool ReplyToSetMessageStatus(uint8_t aMasId, bool aStatus);

  /**
   * Reply to the *in-process* 'sendmessage' request.
   *
   * @param aMasId [in]         MAS id
   * @param aHandleId [in]      Handle id
   * @param aStatus [in]        success or failure
   *
   * @return true if the response packet has been packed correctly and started
   *         to be sent to the remote device; false otherwise.
   */
  bool ReplyToSendMessage(
    uint8_t aMasId, const nsAString& aHandleId , bool aStatus);

  /**
   * Reply to the *in-process* 'messageupdate' request.
   *
   * @param aMasId [in]         MAS id
   * @param aStatus [in]        success or failure
   *
   * @return true if the response packet has been packed correctly and started
   *         to be sent to the remote device; false otherwise.
   */
  bool ReplyToMessageUpdate(uint8_t aMasId, bool aStatus);

 /**
  * SendEvent to MCE device. MSE shall use the SendEvent function to notify the
  * MCE on events affecting the messages listings within the MSE's folders
  * structure. The MSE shall notify the MCE of the sending status of these
  * messages using the SendEvent function.
  *
  * @param aMasId [in]          MAS id
  * @param aBlob [in]           a blob containing the MAP-event-report objects
  *
  * @return true if the blob has started to be sent to the remote device; false
  * otherwise.
  */
 bool SendMessageEvent(uint8_t aMasId, Blob* aBlob);

 /**
  * SendEvent to MCE device. MSE shall use the SendEvent function to notify the
  * MCE on events affecting the messages listings within the MSE's folders
  * structure. The MSE shall notify the MCE of the sending status of these
  * messages using the SendEvent function.
  *
  * @param aMasId [in]          MAS id
  * @param aActor [in]          a blob actor containing the MAP-event-report
  *                             objects
  *
  * @return true if the blob has started to be sent to the remote device; false
  * otherwise.
  */
 bool SendMessageEvent(uint8_t aMasId, BlobParent* aActor);


protected:
  virtual ~BluetoothMapSmsManager();

private:
  class CreateSdpRecordResultHandler;
  class RemoveSdpRecordResultHandler;
  class InitProfileResultHandlerRunnable;
  class DeinitProfileResultHandlerRunnable;
  class RegisterModuleResultHandler;
  class UnregisterModuleResultHandler;

  BluetoothMapSmsManager();

  nsresult Init();
  void Uninit();
  void HandleShutdown();

  bool ReplyToConnect();
  void ReplyToDisconnectOrAbort();

  /*
   * This function replies to Get request with Header Body, in case of a GET
   * operation returning an object that is too big to fit in one response
   * packet. If the operation requires multiple response packets to complete
   * after the Final bit is set in the request.
   */
  bool ReplyToGetWithHeaderBody(UniquePtr<uint8_t[]> aResponse, unsigned int aIndex);
  /*
   * This function sends multiple response packets, in case of a PUT operation
   * returning an object larger than one response packet.
   * If the operation requires multiple request packets to complete after the
   * Final bit is set in the request.
   */
  bool MnsPutMultiRequest();
  void ReplyToSetPath();
  void ReplyToPut(uint8_t aResponse);
  bool SendReply(uint8_t aResponse);

  void HandleNotificationRegistration(const ObexHeaderSet& aHeader);
  void HandleSetMessageStatus(const ObexHeaderSet& aHeader);
  void HandleSmsMmsFolderListing(const ObexHeaderSet& aHeader);
  void HandleSmsMmsMsgListing(const ObexHeaderSet& aHeader);
  void HandleSmsMmsGetMessage(const ObexHeaderSet& aHeader);
  void HandleSmsMmsPushMessage(const ObexHeaderSet& aHeader);

  void AppendBtNamedValueByTagId(const ObexHeaderSet& aHeader,
    InfallibleTArray<BluetoothNamedValue>& aValues,
    const Map::AppParametersTagId aTagId);
  bool SendMasObexData(uint8_t* aData, uint8_t aOpcode, int aSize);
  bool SendMasObexData(UniquePtr<uint8_t[]> aData, uint8_t aOpcode, int aSize);
  void SendMnsObexData(uint8_t* aData, uint8_t aOpcode, int aSize);
  bool StatusResponse(bool aStatus);

  ObexResponseCode NotifyConnectionRequest();

  uint8_t SetPath(uint8_t flags, const ObexHeaderSet& aHeader);
  bool CompareHeaderTarget(const ObexHeaderSet& aHeader);
  void AfterMapSmsConnected();
  void AfterMapSmsDisconnected();
  void CreateMnsObexConnection();
  void DestroyMnsObexConnection();
  void SendPutFinalRequest();
  void SendMnsConnectRequest();
  void SendMnsDisconnectRequest();
  void MnsDataHandler(mozilla::ipc::UnixSocketBuffer* aMessage);
  void MasDataHandler(mozilla::ipc::UnixSocketBuffer* aMessage);
  bool GetInputStreamFromBlob(Blob* aBlob, bool aIsMas);
  InfallibleTArray<uint32_t> PackParameterMask(uint8_t* aData, int aSize);

  /**
   * Usually we won't get a full PUT packet in one operation, which means that
   * a packet may be devided into several parts and BluetoothMapSmsManager
   * should be in charge of assembling.
   *
   * @param aOpCode  [in] the opCode of the PUT packet (usually is Put/PutFinal).
   * @param aMessage [in] the content of the PUT packet.
   *
   * @return true if a packet has been fully received.
   *         false if the received length exceeds/not reaches the expected
   *         length.
   */
  bool ComposePacket(uint8_t aOpCode,
                     mozilla::ipc::UnixSocketBuffer* aMessage);

  /**
   * Parse the headers from the full PUT packet.
   *
   * @param aOpCode  [in] the opCode of the PUT packet (usually is Put/PutFinal).
   * @param aMessage [in] the content of the PUT packet.
   * @param aPktHeaders [out] the headers of the PUT packet.
   *
   * @return true if a headers is successfully parsed.
   *         false if a put packet has not been fully received, or a headers is
   *                  parsed with any error.
   */
  bool ParsePutPacketHeaders(uint8_t aOpCode,
                             mozilla::ipc::UnixSocketBuffer* aMessage,
                             ObexHeaderSet* aPktHeaders);
  /*
   * Build mandatory folders
   */
  void BuildDefaultFolderStructure();
  /**
   * Current virtual folder path
   */
  BluetoothMapFolder* mCurrentFolder;
  RefPtr<BluetoothMapFolder> mRootFolder;

  // Whether header body is required for the current MessagesListing response.
  bool mBodyRequired;
  // Whether FractionDeliver is required for the current GetMessage response
  bool mFractionDeliverRequired;
  // MAS OBEX session status. Set when MAS OBEX session is established.
  bool mMasConnected;
  // MNS OBEX session status. Set when MNS OBEX session is established.
  bool mMnsConnected;
  bool mNtfRequired;
  // The connection ID is received during the connection establishment,
  // in order to signal the recipient of the request which OBEX connection
  // this request belongs to.
  uint32_t mConnectionId;
  // Record the last command
  int mLastCommand;

  BluetoothAddress mDeviceAddress;
  unsigned int mRemoteMaxPacketLength;

  // If a connection has been established, mMasSocket will be the socket
  // communicating with the remote socket. We maintain the invariant that if
  // mMasSocket is non-null, mServerSocket must be null (and vice versa).
  RefPtr<BluetoothSocket> mMasSocket;

  // Server socket. Once an inbound connection is established, it will hand
  // over the ownership to mMasSocket, and get a new server socket while Listen()
  // is called.
  RefPtr<BluetoothSocket> mMasServerSocket;

  // Message notification service client socket
  RefPtr<BluetoothSocket> mMnsSocket;

  int mBodySegmentLength;
  UniquePtr<uint8_t[]> mBodySegment;

  // The getMessage/messagesListing data stream for current processing response
  nsCOMPtr<nsIInputStream> mMasDataStream;
  // The notification data stream
  nsCOMPtr<nsIInputStream> mMnsDataStream;

  // Set when receiving a PutFinal packet
  bool mPutFinalFlag;
  int mPutPacketLength;
  int mPutReceivedPacketLength;
  UniquePtr<uint8_t[]> mPutReceivedDataBuffer;
};

END_BLUETOOTH_NAMESPACE

#endif //mozilla_dom_bluetooth_bluedroid_BluetoothMapSmsManager_h
