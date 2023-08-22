/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/**
 * Provide an Android-Surface-like Surface to JS APP, this will create a correspond Android Surface in native side.
 */
[Func="nsDOMSurfaceControl::HasSupport"]
interface SurfaceControl : MediaStream
{
};

/* Used for the dimensions of a preview stream */
dictionary SurfaceSize
{
  unsigned long width = 0;
  unsigned long height = 0;
};

/* Pre-emptive surface configuration options.
   To determine 'previewSize', one should generally provide the size of the
   element which will contain the preview rather than guess which supported
   preview size is the best. If not specified, 'previewSize' defaults to the
   inner window size. */
dictionary SurfaceConfiguration
{
  SurfaceSize previewSize = null;
};