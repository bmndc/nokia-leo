/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et ft=cpp : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

namespace mozilla {
namespace hal_impl {

bool
GetScreenEnabled()
{
  return true;
}

void
SetScreenEnabled(bool aEnabled)
{}

bool
GetExtScreenEnabled()
{
  return true;
}

void
SetExtScreenEnabled(bool aEnabled)
{}

bool
GetKeyLightEnabled()
{
  return true;
}

void
SetKeyLightEnabled(bool aEnabled)
{}

double
GetScreenBrightness()
{
  return 1;
}

void
SetScreenBrightness(double aBrightness)
{}

double
GetExtScreenBrightness()
{
  return 1;
}

void
SetExtScreenBrightness(double aBrightness)
{}

double
GetKeyLightBrightness()
{
  return 1;
}

void
SetKeyLightBrightness(double aBrightness)
{}

} // namespace hal_impl
} // namespace mozilla
