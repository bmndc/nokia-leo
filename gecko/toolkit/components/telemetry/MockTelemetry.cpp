/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "Telemetry.h"
#include "TelemetryCommon.h"
#include "WebrtcTelemetry.h"
#include "mozilla/ProcessedStack.h"
#include "mozilla/ThreadHangStats.h"

namespace mozilla {
namespace Telemetry {

/**
 * Initialize the Telemetry service on the main thread at startup.
 */
void Init() {}

/**
 * Adds sample to a histogram defined in TelemetryHistograms.h
 *
 * @param id - histogram id
 * @param sample - value to record.
 */
void Accumulate(ID id, uint32_t sample) {}

/**
 * Adds sample to a keyed histogram defined in TelemetryHistograms.h
 *
 * @param id - keyed histogram id
 * @param key - the string key
 * @param sample - (optional) value to record, defaults to 1.
 */
void Accumulate(ID id, const nsCString& key, uint32_t sample) {}

/**
 * Adds a sample to a histogram defined in TelemetryHistograms.h.
 * This function is here to support telemetry measurements from Java,
 * where we have only names and not numeric IDs.  You should almost
 * certainly be using the by-enum-id version instead of this one.
 *
 * @param name - histogram name
 * @param sample - value to record
 */
void Accumulate(const char* name, uint32_t sample) {}

/**
 * Adds a sample to a histogram defined in TelemetryHistograms.h.
 * This function is here to support telemetry measurements from Java,
 * where we have only names and not numeric IDs.  You should almost
 * certainly be using the by-enum-id version instead of this one.
 *
 * @param name - histogram name
 * @param key - the string key
 * @param sample - sample - (optional) value to record, defaults to 1.
 */
void Accumulate(const char *name, const nsCString& key, uint32_t sample) {}

/**
 * Adds time delta in milliseconds to a histogram defined in TelemetryHistograms.h
 *
 * @param id - histogram id
 * @param start - start time
 * @param end - end time
 */
void AccumulateTimeDelta(ID id, TimeStamp start, TimeStamp end) {}

/**
 * Enable/disable recording for this histogram at runtime.
 * Recording is enabled by default, unless listed at kRecordingInitiallyDisabledIDs[].
 * id must be a valid telemetry enum, otherwise an assertion is triggered.
 *
 * @param id - histogram id
 * @param enabled - whether or not to enable recording from now on.
 */
void SetHistogramRecordingEnabled(ID id, bool enabled) {}

/**
 * Return a raw Histogram for direct manipulation for users who can not use Accumulate().
 */
base::Histogram* GetHistogramById(ID id) { return nullptr; }

const char* GetHistogramName(ID id) { return nullptr; }

/**
 * Return a raw histogram for keyed histograms.
 */
base::Histogram* GetKeyedHistogramById(ID id, const nsAString&) { return nullptr; }

/**
 * Indicates whether Telemetry base data recording is turned on. Added for future uses.
 */
bool CanRecordBase() { return false; }

/**
 * Indicates whether Telemetry extended data recording is turned on.  This is intended
 * to guard calls to Accumulate when the statistic being recorded is expensive to compute.
 */
bool CanRecordExtended() { return false; }

/**
 * Records slow SQL statements for Telemetry reporting.
 *
 * @param statement - offending SQL statement to record
 * @param dbName - DB filename
 * @param delay - execution time in milliseconds
 */
void RecordSlowSQLStatement(const nsACString &statement,
                            const nsACString &dbName,
                            uint32_t delay) { }

/**
 * Record Webrtc ICE candidate type combinations in a 17bit bitmask
 *
 * @param iceCandidateBitmask - the bitmask representing local and remote ICE
 *                              candidate types present for the connection
 * @param success - did the peer connection connected
 * @param loop - was this a Firefox Hello AKA Loop call
 */
void
RecordWebrtcIceCandidates(const uint32_t iceCandidateBitmask,
                          const bool success,
                          const bool loop) { }
/**
 * Initialize I/O Reporting
 * Initially this only records I/O for files in the binary directory.
 *
 * @param aXreDir - XRE directory
 */
void InitIOReporting(nsIFile* aXreDir) { }

/**
 * Set the profile directory. Once called, files in the profile directory will
 * be included in I/O reporting. We can't use the directory
 * service to obtain this information because it isn't running yet.
 */
void SetProfileDir(nsIFile* aProfD) { }

/**
 * Called to inform Telemetry that startup has completed.
 */
void LeavingStartupStage() { }

/**
 * Called to inform Telemetry that shutdown is commencing.
 */
void EnteringShutdownStage() { }

/**
 * Record the main thread's call stack after it hangs.
 *
 * @param aDuration - Approximate duration of main thread hang, in seconds
 * @param aStack - Array of PCs from the hung call stack
 * @param aSystemUptime - System uptime at the time of the hang, in minutes
 * @param aFirefoxUptime - Firefox uptime at the time of the hang, in minutes
 * @param aAnnotations - Any annotations to be added to the report
 */
#if defined(MOZ_ENABLE_PROFILER_SPS)
void RecordChromeHang(uint32_t aDuration,
                      ProcessedStack &aStack,
                      int32_t aSystemUptime,
                      int32_t aFirefoxUptime,
                      mozilla::UniquePtr<mozilla::HangMonitor::HangAnnotations>
                              aAnnotations) { }
#endif

/**
 * Move a ThreadHangStats to Telemetry storage. Normally Telemetry queries
 * for active ThreadHangStats through BackgroundHangMonitor, but once a
 * thread exits, the thread's copy of ThreadHangStats needs to be moved to
 * inside Telemetry using this function.
 *
 * @param aStats ThreadHangStats to save; the data inside aStats
 *               will be moved and aStats should be treated as
 *               invalid after this function returns
 */
void RecordThreadHangStats(ThreadHangStats& aStats) { }

/**
 * Record a failed attempt at locking the user's profile.
 *
 * @param aProfileDir The profile directory whose lock attempt failed
 */
void WriteFailedProfileLock(nsIFile* aProfileDir) { }

ProcessedStack::ProcessedStack()
{
}

ProcessedStack
GetStackAndModules(const std::vector<uintptr_t>& aPCs)
{
  ProcessedStack Ret;
  return Ret;
}

ProcessedStack::Module dummyModule;
ProcessedStack::Frame dummyFrame;

const ProcessedStack::Module &ProcessedStack::GetModule(unsigned aIndex) const
{
  return dummyModule;
}

size_t ProcessedStack::GetNumModules() const {
  return 0;
}

size_t ProcessedStack::GetStackSize() const
{
  return 0;
}

const ProcessedStack::Frame &ProcessedStack::GetFrame(unsigned aIndex) const
{
  return dummyFrame;
}

void
TimeHistogram::Add(PRIntervalTime aTime)
{
  return;
}

const char*
HangStack::InfallibleAppendViaBuffer(const char* aText, size_t aLength)
{
  return nullptr;
}

const char*
HangStack::AppendViaBuffer(const char* aText, size_t aLength)
{
  return nullptr;
}

uint32_t
HangHistogram::GetHash(const HangStack& aStack)
{
  return 0;
}

bool
HangHistogram::operator==(const HangHistogram& aOther) const
{
  if (mHash != aOther.mHash) {
    return false;
  }
  if (mStack.length() != aOther.mStack.length()) {
    return false;
  }
  return mStack == aOther.mStack;
}

} // namespace Telemetry

void
RecordShutdownStartTimeStamp() { }

void
RecordShutdownEndTimeStamp() { }

} // namespace mozilla

// void
// XRE_TelemetryAccumulate(int aID, uint32_t aSample) { }
