/* -*- Mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; tab-width: 40 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef STUMBLERLOGGING_H
#define STUMBLERLOGGING_H

#if !defined(MOZ_WIDGET_GONK)

#include "mozilla/Logging.h"
mozilla::LogModule* GetLog();

#define STUMBLER_DBG(arg, ...)                                   \
  MOZ_LOG(GetLog(), mozilla::LogLevel::Debug,                    \
          ("Stumbler_GEO - %s: " arg, __func__, ##__VA_ARGS__))

#define STUMBLER_LOG(arg, ...)                                   \
  MOZ_LOG(GetLog(), mozilla::LogLevel::Info,                     \
          ("Stumbler_GEO - %s: " arg, __func__, ##__VA_ARGS__))

#define STUMBLER_ERR(arg, ...)                                   \
  MOZ_LOG(GetLog(), mozilla::LogLevel::Error,                    \
          ("Stumbler_GEO - %s: " arg, __func__, ##__VA_ARGS__))

#else

#include <android/log.h>
#define STUMBLER_DBG(msg, ...)                                   \
  __android_log_print(ANDROID_LOG_DEBUG, "Stumbler_GEO",         \
                      "%s: " msg, __func__, ##__VA_ARGS__)

#define STUMBLER_LOG(msg, ...)                                   \
  __android_log_print(ANDROID_LOG_INFO, "Stumbler_GEO",          \
                      "%s: " msg, __func__, ##__VA_ARGS__)

#define STUMBLER_ERR(msg, ...)                                   \
  __android_log_print(ANDROID_LOG_WARN, "Stumbler_GEO",          \
                      "%s: " msg, __func__, ##__VA_ARGS__)

#endif // !defined(MOZ_WIDGET_GONK)

#endif // STUMBLERLOGGING_H
