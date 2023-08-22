/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

[CheckAnyPermissions="bluetooth"]
interface BluetoothMapRequestHandle
{
  /**
   * Reply to Folder-Listing object for MAP request. The Promise will be
   * rejected if the MAP request operation fails.
   */
  [NewObject, Throws, AvailableIn=CertifiedApps]
  Promise<void> replyToFolderListing(octet masId, DOMString folders);

  /**
   * Reply the Messages-Listing object to the MAP request. The Promise will
   * be rejected if the MAP request operation fails.
   */
  [NewObject, Throws, AvailableIn=CertifiedApps]
  Promise<void> replyToMessagesListing(
    octet masId,
    Blob messageslisting,
    boolean newmessage,
    DOMString timestamp,
    unsigned long size);

  /**
   * Reply GetMessage object to the MAP request. The Promise will be rejected
   * if the MAP request operation fails.
   */
  [NewObject, Throws, AvailableIn=CertifiedApps]
  Promise<void> replyToGetMessage(octet masId, Blob bmessage);

  /**
   * Reply SetMessage object to the MAP request. The Promise will be rejected
   * if the MAP request operation fails.
   */
  [NewObject, Throws, AvailableIn=CertifiedApps]
  Promise<void> replyToSetMessageStatus(octet masId, boolean status);

  /**
   * Reply SendMessage request to the MAP request. The Promise will be rejected
   * if the MAP request operation fails.
   */
  [NewObject, Throws, AvailableIn=CertifiedApps]
  Promise<void> replyToSendMessage(octet masId, DOMString handleId, boolean status);

  /**
   * Reply Message-Update object to the MAP request. The Promise will be
   * rejected if the MAP request operation fails.
   */
  [NewObject, Throws, AvailableIn=CertifiedApps]
  Promise<void> replyToMessageUpdate(octet masId, boolean status);
};
