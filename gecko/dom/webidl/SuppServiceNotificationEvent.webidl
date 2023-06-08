/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* (c) 2018 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

/**
 * The possible values of notification type.
 */
enum NotifyType{"mo", "mt", "unknown"};

/**
 * The possible values of code type.
 * @see 3GPP TS 27.007 7.17 "code1"(MO) and "code2"(MT).
 */
enum CodeType{
//3GPP TS 27.007 7.17 "code1"(MO).
  "mo-unconditional_cf_active",   //0 unconditional call forwarding is active
  "mo-some_cf_active",            //1	some of the conditional call forwardings are active
  "mo-call_forwarded",            //2	call has been forwarded
  "mo-call_is_waiting",           //3	call is waiting
  "mo-cug_call",                  //4 this is a CUG call
  "mo-outgoing_calls_barred",     //5	outgoing calls are barred
  "mo-incoming_calls_barred",     //6	incoming calls are barred
  "mo-clir_suppression_rejected", //7 CLIR suppression rejected
  "mo-call_deflected",            //8 call has been deflected

//3GPP TS 27.007 7.17 "code2"(MT),the length of code1 is 9.
  "mt-forwarded_call",            //9 this is a forwarded call
  "mt-cug_call",                  //10 this is a CUG call
  "mt-call_on_hold",              //11 call has been put on hold
  "mt-call_retrieved",            //12 call has been retrieved
  "mt-multi_party_call",          //13 multiparty call entered
  "mt-on_hold_call_released",     //14 call on hold has been released
  "mt-forward_check_received",    //15 forward check SS message received
  "mt-call_connecting_ect",       //16 call is being connected with the remote party in ECT
  "mt-call_connected_ect",        //17 call has been connected with the remote party in ECT
  "mt-deflected_call",            //18 this is a deflected call
  "mt-additional_call_forwarded", //19 additional incoming call forwarded
  "unknown"
};

[Constructor(DOMString type, optional SuppServiceNotificationEventInit eventInitDict), Pref="dom.telephony.enabled"]

interface SuppServiceNotificationEvent : Event
{
  readonly attribute NotifyType notificationType;
  readonly attribute CodeType code;
  readonly attribute long index;
  readonly attribute long type;
  readonly attribute DOMString number;
};

dictionary SuppServiceNotificationEventInit : EventInit
{
  NotifyType notificationType = "unknown";
  CodeType code = "unknown";
  long index = 0;
  long type = 0;
  DOMString number = "";
};

