/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

/**
 * Whole File Download related are TBD.
 * Something need to be resolved before go further
 * 1 File management
 * 2 Can we leverage WebAPI DOMDownload
 */

enum LteBroadcastDownloadStatus {
  "notdownloaded",
  "downloading",
  "succeeded",
};

interface LteBroadcastDownloadHandle : LteBroadcastHandle {
  /**
   * To retrieve available LteBroadcastFiles in this handle.
   */
  Promise<sequence<LteBroadcastFile>> getFiles();

  /**
   * To start a download task.
   * @param fileUri The file uri you are going to start download.
   *                If not given, to download all files within this handle.
   */
  Promise<void> startCapture(optional DOMString fileUri);

  /**
   * To cancal download task.
   * @param fileUri The file uri you are going to cancel download.
   *                If not given, to stop all download task withinnder this handle.
   */
  Promise<void> stopCapture(optional DOMString fileUri);

  /**
   * To ask MW to refresh status/download progress immediately.
   * The Promise only a success/error indicator, it does not provide any info.
   * The result would be notify via onmessage 'downloaded', 'downloading', 'progressupdated'...etc.
   * After onmessage is notified, app could retrieve file detail via status, fileSize, receivedFileSize ...etc.
   * @param fileUri The file to get information.
   */
  Promise<void> requestState(DOMString fileUri);
};
