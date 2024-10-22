/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

interface LteBroadcastService : EventTarget {

  /**
   * To indicate current service type.
   */
  readonly attribute LteBroadcastServiceType serviceType;

  /**
   * The identifier of this service.
   * RD note: Since we don't support multiple running app now,
   *  this appID can be generated by LteBroadcastManager with windowID.
   */
  //readonly attribute DOMString appId;

  /**
   * I separate setServiceClass and getHandles into to apis because
   * 1 Basically, setServiceClass should be invoked one time during app run time.
   * 2 APP may getHandles multiple times during app run time.
   */
  Promise<sequence<LteBroadcastHandle>> getHandles();

  /**
   * TBD.
   * To enable middleware statistics report.
   */
  //Promise<void> enableStatisticReport(boolean enabled);

  /**
   * All eMBMS service related state change events will go through this callback.
   * Please refert for possible events.
   */
  attribute EventHandler onmessage;

  /**
   * All eMBMS service related Error/Warning events will go through this callback.
   * The detail is TBD.
   */
  //attribute EventHandler onerror;
};
