/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

interface FaceAnalysisImage
{
  readonly attribute unsigned long width;
  readonly attribute unsigned long height;
  readonly attribute unsigned long type;
  readonly attribute Blob? imageData;
};

dictionary FaceAnalysisImageInit
{
  unsigned long width = 0;
  unsigned long height = 0;
  unsigned long type = 0;
  Blob imageData;
};

interface FaceAnalysisResults
{
  readonly attribute unsigned long analysisResult;
  readonly attribute unsigned long analysisInfo;
  readonly attribute Blob? payload;
  readonly attribute unsigned long matchScore;
  readonly attribute FaceAnalysisImage? bestImage;
};

dictionary FaceAnalysisResultsInit
{
  unsigned long analysisResult = 0;
  unsigned long analysisInfo = 0;
  Blob payload;
  unsigned long matchScore = 0;
  FaceAnalysisImage bestImage;
};

[Constructor(DOMString type, optional FaceAnalysisResultsEventInit eventInit)]
interface FaceAnalysisResultsEvent : Event
{
  readonly attribute FaceAnalysisResults? faceAnalysisResults;
};

dictionary FaceAnalysisResultsEventInit : EventInit
{
  FaceAnalysisResults? faceAnalysisResults = null;
};
