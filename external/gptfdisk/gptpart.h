//
// C++ Interface: gptpart
//
// Description: Class to implement a single GPT partition
//
//
// Author: Rod Smith <rodsmith@rodsbooks.com>, (C) 2009
//
// Copyright: See COPYING file that comes with this distribution
//
//
// This program is copyright (c) 2009 by Roderick W. Smith. It is distributed
// under the terms of the GNU GPL version 2, as detailed in the COPYING file.

#ifndef __GPTPART_H
#define __GPTPART_H

#include <stdint.h>
#include <string>
#include <sys/types.h>
#include "support.h"
#include "parttypes.h"
#include "guid.h"
#include "attributes.h"

using namespace std;

// Values returned by GPTPart::IsSizedForMBR()
#define MBR_SIZED_GOOD 0 /* Whole partition under 2^32 sectors */
#define MBR_SIZED_IFFY 1 /* Partition starts under 2^32 & is less than 2^32, but ends over 2^32 */
#define MBR_SIZED_BAD  2 /* Partition starts over 2^32, is bigger than 2^32, or otherwise bad */

/****************************************
 *                                      *
 * GPTPart class and related structures *
 *                                      *
 ****************************************/

class GPTPart {
   protected:
      // Caution: The non-static data in GPTPart is precisely the right size
      // to enable easy loading of the data directly from disk. If any
      // non-static variables are added to the below, the data size will
      // change and the program will stop working. This can be corrected by
      // adjusting the data-load operation in GPTData::LoadMainTable() and
      // GPTData::LoadSecondTableAsMain() and then removing the GPTPart
      // size check in SizesOK() (in gpt.cc file).
      PartType partitionType;
      GUIDData uniqueGUID;
      uint64_t firstLBA;
      uint64_t lastLBA;
      Attributes attributes;
      uint16_t name[NAME_SIZE];
   public:
      GPTPart(void);
      ~GPTPart(void);

      // Simple data retrieval:
      PartType & GetType(void) {return partitionType;}
      uint16_t GetHexType(void) const;
      string GetTypeName(void);
      UnicodeString GetUTypeName(void);
      const GUIDData GetUniqueGUID(void) const {return uniqueGUID;}
      uint64_t GetFirstLBA(void) const {return firstLBA;}
      uint64_t GetLastLBA(void) const {return lastLBA;}
      uint64_t GetLengthLBA(void) const;
      Attributes GetAttributes(void) {return attributes;}
      void ShowAttributes(uint32_t partNum) {attributes.ShowAttributes(partNum);}
      UnicodeString GetDescription(void);
      int IsUsed(void);
      int IsSizedForMBR(void);

      // Simple data assignment:
      void SetType(PartType t);
      void SetType(uint16_t hex) {partitionType = hex;}
      void SetUniqueGUID(GUIDData u) {uniqueGUID = u;}
      void RandomizeUniqueGUID(void) {uniqueGUID.Randomize();}
      void SetFirstLBA(uint64_t f) {firstLBA = f;}
      void SetLastLBA(uint64_t l) {lastLBA = l;}
      void SetAttributes(uint64_t a) {attributes = a;}
      void SetAttributes(void) {attributes.ChangeAttributes();}
      void SetName(const string & theName);
#ifdef USE_UTF16
      void SetName(const UnicodeString & theName);
#endif
      void SetDefaultDescription(void);

      // Additional functions
      GPTPart & operator=(const GPTPart & orig);
      bool operator<(const GPTPart &other) const;
      void ShowSummary(int partNum, uint32_t blockSize); // display summary information (1-line)
      void ShowDetails(uint32_t blockSize); // display detailed information (multi-line)
      void BlankPartition(void); // empty partition of data
      int DoTheyOverlap(const GPTPart & other); // returns 1 if there's overlap
      void ReversePartBytes(void); // reverse byte order of all integer fields

      // Functions requiring user interaction
      void ChangeType(void); // Change the type code
}; // struct GPTPart

#endif
