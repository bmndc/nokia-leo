/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * The origin of this IDL file is
 * http://notifications.spec.whatwg.org/
 *
 * Copyright:
 * To the extent possible under law, the editors have waived all copyright and
 * related or neighboring rights to this work.
 */

[Constructor(DOMString title, optional NotificationOptions options),
 Exposed=(Window,Worker),
 Func="mozilla::dom::Notification::PrefEnabled",
 UnsafeInPrerendering]
interface Notification : EventTarget {
  [GetterThrows]
  static readonly attribute NotificationPermission permission;

  [Throws, Func="mozilla::dom::Notification::RequestPermissionEnabledForScope"]
  static Promise<NotificationPermission> requestPermission(optional NotificationPermissionCallback permissionCallback);

  [Throws, Func="mozilla::dom::Notification::IsGetEnabled"]
  static Promise<sequence<Notification>> get(optional GetNotificationOptions filter);

  static readonly attribute unsigned long maxActions;

  attribute EventHandler onclick;

  attribute EventHandler onshow;

  attribute EventHandler onerror;

  attribute EventHandler onclose;

  [Pure]
  readonly attribute DOMString title;

  [Pure]
  readonly attribute NotificationDirection dir;

  [Pure]
  readonly attribute DOMString? lang;

  [Pure]
  readonly attribute DOMString? body;

  [Constant]
  readonly attribute DOMString? tag;

  [Pure]
  readonly attribute DOMString? icon;

  [Pure]
  readonly attribute DOMString? image;

  [Constant]
  readonly attribute any data;

  readonly attribute boolean requireInteraction;

  [Frozen, Cached, Pure]
  readonly attribute sequence<NotificationAction> actions;

  readonly attribute boolean silent;

  void close();
};

dictionary NotificationOptions {
  NotificationDirection dir = "auto";
  DOMString lang = "";
  DOMString body = "";
  DOMString tag = "";
  DOMString icon = "";
  DOMString image = "";
  any data = null;
  NotificationBehavior mozbehavior = null;
  boolean requireInteraction = false;
  sequence<NotificationAction> actions = [];
  boolean silent = false;
};

dictionary GetNotificationOptions {
  DOMString tag = "";
};

dictionary NotificationLoopControl {
  boolean sound;
  unsigned long soundMaxDuration;
  boolean vibration;
  unsigned long vibrationMaxDuration;
};

dictionary NotificationBehavior {
  boolean noscreen = false;
  boolean noclear = false;
  boolean showOnlyOnce = false;
  DOMString soundFile = "";
  sequence<unsigned long> vibrationPattern;
  NotificationLoopControl loopControl = null;
};

enum NotificationPermission {
  "default",
  "denied",
  "granted"
};

callback NotificationPermissionCallback = void (NotificationPermission permission);

enum NotificationDirection {
  "auto",
  "ltr",
  "rtl"
};

dictionary NotificationAction {
  required DOMString action;
  required DOMString title;
};

partial interface ServiceWorkerRegistration {
  [Throws, Func="mozilla::dom::ServiceWorkerNotificationAPIVisible"]
  Promise<void> showNotification(DOMString title, optional NotificationOptions options);
  [Throws, Func="mozilla::dom::ServiceWorkerNotificationAPIVisible"]
  Promise<sequence<Notification>> getNotifications(optional GetNotificationOptions filter);
};
