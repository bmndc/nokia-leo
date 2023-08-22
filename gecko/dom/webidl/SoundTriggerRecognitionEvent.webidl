/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

enum SoundTriggerEventStatus {
  "success",
  "fail",
  "abort"
};

/*
 * This interface contains the information of recognition results from onrecognitionresult.
 */
[Constructor(DOMString type, optional SoundTriggerRecognitionEventInit init)]
interface SoundTriggerRecognitionEvent : Event {
  /*
   * The status of the event
   */
  readonly attribute SoundTriggerEventStatus status;
  /*
   * detected key phrase
   */
  readonly attribute DOMString text;
  /*
   * confidence level (0~100)
   */
  readonly attribute long confidenceLevel;
  /**
  * start time of the wake words
  */
  readonly attribute long start;
  /**
   * end time of the wake words
   */
  readonly attribute long end;
  /**
   * the user's voice for the wake words
   */
  readonly attribute ArrayBuffer? audioData;
};

dictionary SoundTriggerRecognitionEventInit : EventInit
{
  SoundTriggerEventStatus status = "success";
  DOMString text = "";
  long confidenceLevel = 0;
  long start = 0;
  long end = 0;
  ArrayBuffer? audioData = null;
};
