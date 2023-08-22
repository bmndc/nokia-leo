/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 tw=80 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsMediaSniffer.h"
#include "nsIHttpChannel.h"
#include "nsString.h"
#include "nsMimeTypes.h"
#include "mozilla/ArrayUtils.h"
#include "mozilla/ModuleUtils.h"
#include "mp3sniff.h"
#include "nestegg/nestegg.h"

#include "nsIClassInfoImpl.h"
#include <algorithm>

// The minimum number of bytes that are needed to attempt to sniff an mp4 file.
static const unsigned MP4_MIN_BYTES_COUNT = 12;
// The maximum number of bytes to consider when attempting to sniff a file.
static const uint32_t MAX_BYTES_SNIFFED = 512;
// The maximum number of bytes to consider when attempting to sniff for a mp3
// bitstream.
// This was 320kbps * 144 / 32kHz + 1 padding byte + 4 bytes of capture pattern, 
// but still too small for some mp3 files, enlarge to 5KB
static const uint32_t MAX_BYTES_SNIFFED_MP3 = 5 * 1024;

NS_IMPL_ISUPPORTS(nsMediaSniffer, nsIContentSniffer)

nsMediaSnifferEntry nsMediaSniffer::sSnifferEntries[] = {
  // The string OggS, followed by the null byte.
  PATTERN_ENTRY("\xFF\xFF\xFF\xFF\xFF", "OggS", APPLICATION_OGG),
  // The string RIFF, followed by four bytes, followed by the string WAVE
  PATTERN_ENTRY("\xFF\xFF\xFF\xFF\x00\x00\x00\x00\xFF\xFF\xFF\xFF", "RIFF\x00\x00\x00\x00WAVE", AUDIO_WAV),
  // mp3 with ID3 tags, the string "ID3".
  PATTERN_ENTRY("\xFF\xFF\xFF", "ID3", AUDIO_MP3),
  //The string "MThd" followed by four bytes representing the number 6 in 32 bits (big-endian), the MIDI signature.
  //https://mimesniff.spec.whatwg.org/#matching-an-audio-or-video-type-pattern
  PATTERN_ENTRY("\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF", "MThd\x00\x00\x00\x06", AUDIO_MIDI),
  //The string "#!AMR-WB" followed by the null byte, the AMR-WB signature.
  PATTERN_ENTRY("\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF", "#!AMR-WB", AUDIO_AMR),
  //The string "#!AMR." followed by the null byte, the AMR-NB signature.
  PATTERN_ENTRY("\xFF\xFF\xFF\xFF\xFF\xFF", "#!AMR\x0A", AUDIO_AMR)
};

// For a complete list of file types, see http://www.ftyps.com/index.html
nsMediaSnifferEntry sFtypEntries[] = {
  PATTERN_ENTRY("\xFF\xFF\xFF", "mp4", VIDEO_MP4), // Could be mp41 or mp42.
  PATTERN_ENTRY("\xFF\xFF\xFF", "avc", VIDEO_MP4), // Could be avc1, avc2, ...
  PATTERN_ENTRY("\xFF\xFF\xFF", "3gp", VIDEO_3GPP), // Could be 3gp4, 3gp5, ...
  PATTERN_ENTRY("\xFF\xFF\xFF\xFF", "M4A ", AUDIO_MP4),
  PATTERN_ENTRY("\xFF\xFF\xFF\xFF", "M4P ", AUDIO_MP4),
  PATTERN_ENTRY("\xFF\xFF\xFF\xFF", "qt  ", VIDEO_QUICKTIME),
};

static bool MatchesBrands(const uint8_t aData[4], nsACString& aSniffedType)
{
  for (size_t i = 0; i < mozilla::ArrayLength(sFtypEntries); ++i) {
    const auto& currentEntry = sFtypEntries[i];
    bool matched = true;
    MOZ_ASSERT(currentEntry.mLength <= 4, "Pattern is too large to match brand strings.");
    for (uint32_t j = 0; j < currentEntry.mLength; ++j) {
      if ((currentEntry.mMask[j] & aData[j]) != currentEntry.mPattern[j]) {
        matched = false;
        break;
      }
    }
    if (matched) {
      aSniffedType.AssignASCII(currentEntry.mContentType);
      return true;
    }
  }

  return false;
}

// This function implements sniffing algorithm for MP4 family file types,
// including MP4 (described at http://mimesniff.spec.whatwg.org/#signature-for-mp4),
// M4A (Apple iTunes audio), and 3GPP.
static bool MatchesMP4(const uint8_t* aData, const uint32_t aLength, nsACString& aSniffedType)
{
  if (aLength <= MP4_MIN_BYTES_COUNT) {
    return false;
  }
  // Conversion from big endian to host byte order.
  uint32_t boxSize = (uint32_t)(aData[3] | aData[2] << 8 | aData[1] << 16 | aData[0] << 24);

  // Boxsize should be evenly divisible by 4.
  if (boxSize % 4 || aLength < boxSize) {
    return false;
  }
  // The string "ftyp".
  if (aData[4] != 0x66 ||
      aData[5] != 0x74 ||
      aData[6] != 0x79 ||
      aData[7] != 0x70) {
    return false;
  }
  if (MatchesBrands(&aData[8], aSniffedType)) {
    return true;
  }
  // Skip minor_version (bytes 12-15).
  uint32_t bytesRead = 16;
  while (bytesRead < boxSize) {
    if (MatchesBrands(&aData[bytesRead], aSniffedType)) {
      return true;
    }
    bytesRead += 4;
  }

  return false;
}

static bool MatchesWebM(const uint8_t* aData, const uint32_t aLength)
{
  return nestegg_sniff((uint8_t*)aData, aLength) ? true : false;
}

// This function implements mp3 sniffing based on parsing
// packet headers and looking for expected boundaries.
static bool MatchesMP3(const uint8_t* aData, const uint32_t aLength)
{
  return mp3_sniff(aData, (long)aLength);
}

static bool MatchesAAC(const uint8_t* aData, const uint32_t aLength)
{
  uint32_t pos = 0;

  //Some aac files will use ID3v2 tag to storage metadata,
  //such as album, artist. If this file has ID3v2 tag, need to skip it.
  //Additionally, the length of ID3v2 header is 10 bytes. If this file
  //has ID3v2 tag, aLength need to be great than or equal to 10, or we
  //we can not get the length of the ID3v2 tag correctly.
  if (aLength >= 10 && !memcmp("ID3", &aData[pos], 3)) {
    size_t len = ((aData[pos + 6] & 0x7f) << 21) |
                 ((aData[pos + 7] & 0x7f) << 14) |
                 ((aData[pos + 8] & 0x7f) << 7)  |
                 (aData[pos + 9] & 0x7f);

    len += 10;
    pos += len;

    //If pos is great than or equal to aLength, return false directly.
    if (pos >= aLength) {
      return false;
    }
  }

  //Using 2 bytes of ADTS header as identifier.
  //-----------------------------------------------------------------------------
  //|       Fields    | Bits | Description
  //|-----------------|------|---------------------------------------------------
  //|      Syncword   |  12  | all bits must be 1
  //|   MPEG version  |   1  | 0 for MPEG-4, 1 for MPEG-2
  //|      Layer      |   2  | always 0
  //|Protection Absent|   1  | 1 if there is no CRC and 0 if there is CRC
  //|-----------------|------|---------------------------------------------------

  if (pos + 2 > aLength) {
    return false;
  }

  if ((aData[pos] == 0xff) && ((aData[pos + 1] & 0xf6) == 0xf0)) {
    return true;
  }

  return false;
}

NS_IMETHODIMP
nsMediaSniffer::GetMIMETypeFromContent(nsIRequest* aRequest,
                                       const uint8_t* aData,
                                       const uint32_t aLength,
                                       nsACString& aSniffedType)
{
  nsCOMPtr<nsIChannel> channel = do_QueryInterface(aRequest);
  if (channel) {
    nsLoadFlags loadFlags = 0;
    channel->GetLoadFlags(&loadFlags);
    if (!(loadFlags & nsIChannel::LOAD_MEDIA_SNIFFER_OVERRIDES_CONTENT_TYPE)) {
      // For media, we want to sniff only if the Content-Type is unknown, or if it
      // is application/octet-stream.
      nsAutoCString contentType;
      nsresult rv = channel->GetContentType(contentType);
      NS_ENSURE_SUCCESS(rv, rv);
      if (!contentType.IsEmpty() &&
          !contentType.EqualsLiteral(APPLICATION_OCTET_STREAM) &&
          !contentType.EqualsLiteral(UNKNOWN_CONTENT_TYPE)) {
        return NS_ERROR_NOT_AVAILABLE;
      }
    }
  }

  const uint32_t clampedLength = std::min(aLength, MAX_BYTES_SNIFFED);

  for (size_t i = 0; i < mozilla::ArrayLength(sSnifferEntries); ++i) {
    const nsMediaSnifferEntry& currentEntry = sSnifferEntries[i];
    if (clampedLength < currentEntry.mLength || currentEntry.mLength == 0) {
      continue;
    }
    bool matched = true;
    for (uint32_t j = 0; j < currentEntry.mLength; ++j) {
      if ((currentEntry.mMask[j] & aData[j]) != currentEntry.mPattern[j]) {
        matched = false;
        break;
      }
    }
    if (matched) {
      aSniffedType.AssignASCII(currentEntry.mContentType);
      return NS_OK;
    }
  }

  if (MatchesMP4(aData, clampedLength, aSniffedType)) {
    return NS_OK;
  }

  if (MatchesWebM(aData, clampedLength)) {
    aSniffedType.AssignLiteral(VIDEO_WEBM);
    return NS_OK;
  }

  // Bug 950023: 512 bytes are often not enough to sniff for mp3.
  if (MatchesMP3(aData, std::min(aLength, MAX_BYTES_SNIFFED_MP3))) {
    aSniffedType.AssignLiteral(AUDIO_MP3);
    return NS_OK;
  }

  if (MatchesAAC(aData, aLength)) {
    aSniffedType.AssignLiteral(AUDIO_AAC);
    return NS_OK;
  }

  // Could not sniff the media type, we are required to set it to
  // application/octet-stream.
  aSniffedType.AssignLiteral(APPLICATION_OCTET_STREAM);
  return NS_ERROR_NOT_AVAILABLE;
}
