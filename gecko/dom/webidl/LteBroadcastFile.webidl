/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

interface LteBroadcastFile {
  /**
   * The identifier of service handle.
   */
  readonly attribute DOMString handleId;

  /**
   * The file uri used to identify the file.
   */
  readonly attribute DOMString fileUri;

  /**
   * Current status.
   */
  readonly attribute LteBroadcastDownloadStatus? status;

  /**
   * Please refer WebAPI Downloads.storageName.
   * Since device storage type is fixed to sdcard, FE needs storageName
   * to distinguish internal from external sdcard. acquire correct deviceStorage via
   * Navigator.getDeviceStorageByNameAndType.
   */
  readonly attribute DOMString storageName;

  /**
   * Please refer WebAPI Downloads.storagePath.
   * FE could access/manage files via DeviceStorage.get()
   */
  readonly attribute DOMString storagePath;

  /**
   * The MIME type of this file.
   */
  readonly attribute DOMString contentType;

  /**
   * File size.
   */
  readonly attribute unsigned long fileSize;

  /**
   * Used to calculate download progress.
   */
  readonly attribute unsigned long receivedFileSize;
};