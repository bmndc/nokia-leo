/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et ft=cpp : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_TVLog_h
#define mozilla_dom_TVLog_h

#include "mozilla/Logging.h"

namespace mozilla {
namespace dom {

mozilla::LogModule*
GetTVLog()
{
  static mozilla::LazyLogModule sTVLog("tv");
  return sTVLog;
}

#define TV_LOG(...) MOZ_LOG(GetTVLog(), LogLevel::Debug, (__VA_ARGS__))

} // namespace dom
} // namespace mozilla

#endif
