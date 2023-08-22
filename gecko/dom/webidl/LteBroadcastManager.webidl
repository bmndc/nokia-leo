/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

enum LteBroadcastServiceType
{
  "streaming",
  "download"
};

enum LteBroadcastCoverageState
{
  "in",
  "out"
};

/**
 * We currently support one lte broadcast service even with dual slot, dual LTE capability.
 * Need to enhance it it if we plan to support services for each slot simultaneously.
 */
[Pref="dom.embms.enabled"]
interface LteBroadcastManager : EventTarget {

  /**
   * LTE broadcast service coverage.
   */
  readonly attribute LteBroadcastCoverageState coverage;

  /**
   * It is a TBD that how to maintain middleware's life cycle.
   * One possible design is:
   *  Middleware is always running after boot up, there is no need for start/stop.
   * Another design would be:
   *  Middleware doing nothing until FE explicitly start/stop it.
   *  However, middleware still need to take care some cases like:
   *    - To maintain its scheduling after reboot, if there were some booked stream/download in previous bootup session.
   *    - Middleware should not stop if FE stop it but there are some booed stream/download task.
   *      Middle can shutdown if there is no booked task.
   */
  Promise<void> start();

  /**
   * TBD. Same reason as stat().
   */
  Promise<void> stop();

  /**
   * To setup classes filter which APP has interest in it.
   * Per current application, all service associate same classFilter within a app.
   * We may move it to LteBroadcastService if we enable differnt classFilter for difference service.
   */
  Promise<void> setServiceClassFilter(sequence<DOMString> serviceClasses);

  /**
   * To retrieve service with specific type.
   * @param serviceType
   *        The service you are going to use.
   * @return LteBroadcastService.
   *         It could be any class which inherited from LteBroadcastService, depends on they type you request.
   *         Currently, we support LteBroadcastStreamingService and LteBroadcastDownloadService.
   * Note: Currently, manager will assign an appId automatically by windowId because we don't support multiple
   *       running app now.
   *       App may need to pass appId if we enabled multiple app support.
   */
  Promise<LteBroadcastService> getService(LteBroadcastServiceType serviceType);

  /**
   * Service Area Identities list.
   */
  Promise<sequence<long>> getSAI();

  /**
   * All eMBMS related event will go through this callback.
   * TBD. To enable this if we need it.
   */
   //attribute EventHandler onstatechange;

  /**
   * Please refer LteBroadcastErrorEvent.
   * All eMBMS related Error/Warning events will go through this callback.
   * Possible errors are:
   *   "stop": if whole eMBMS stopped unexpectedly.
   */
  attribute EventHandler onerror;

  /**
   * The 'message' event is notified whenever some manager related events happen.
   * ex: SAI changed, coverage changed.
   * Please refer LteBroadcastMessageEvent for event structure.
   */
  attribute EventHandler onmessage;
};
