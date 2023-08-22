/*
    MBRPart class, part of GPT fdisk program family.
    Copyright (C) 2011  Roderick W. Smith

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include <stddef.h>
#include <stdint.h>
#include <iostream>
#include "support.h"
#include "mbrpart.h"

using namespace std;

uint32_t MBRPart::numHeads = MAX_HEADS;
uint32_t MBRPart::numSecspTrack = MAX_SECSPERTRACK;
uint64_t MBRPart::diskSize = 0;
uint32_t MBRPart::blockSize = 512;
int MBRPart::numInstances = 0;

MBRPart::MBRPart() {
   int i;

   status = 0;
   for (i = 0; i < 3; i++) {
      firstSector[i] = 0;
      lastSector[i] = 0;
   } // for
   partitionType = 0x00;
   firstLBA = 0;
   lengthLBA = 0;
   includeAs = NONE;
   canBePrimary = 0;
   canBeLogical = 0;
   if (numInstances == 0) {
      numHeads = MAX_HEADS;
      numSecspTrack = MAX_SECSPERTRACK;
      diskSize = 0;
      blockSize = 512;
   } // if
   numInstances++;
}

MBRPart::MBRPart(const MBRPart& orig) {
   numInstances++;
   operator=(orig);
}

MBRPart::~MBRPart() {
   numInstances--;
}

MBRPart& MBRPart::operator=(const MBRPart& orig) {
   int i;

   status = orig.status;
   for (i = 0; i < 3; i++) {
      firstSector[i] = orig.firstSector[i];
      lastSector[i] = orig.lastSector[i];
   } // for
   partitionType = orig.partitionType;
   firstLBA = orig.firstLBA;
   lengthLBA = orig.lengthLBA;
   includeAs = orig.includeAs;
   canBePrimary = orig.canBePrimary;
   canBeLogical = orig.canBeLogical;
   return *this;
} // MBRPart::operator=(const MBRPart& orig)

// Set partition data from packed MBRRecord structure.
MBRPart& MBRPart::operator=(const struct MBRRecord& orig) {
   int i;

   status = orig.status;
   for (i = 0; i < 3; i++) {
      firstSector[i] = orig.firstSector[i];
      lastSector[i] = orig.lastSector[i];
   } // for
   partitionType = orig.partitionType;
   firstLBA = orig.firstLBA;
   lengthLBA = orig.lengthLBA;
   if (lengthLBA > 0)
      includeAs = PRIMARY;
   else
      includeAs = NONE;
   return *this;
} // MBRPart::operator=(const struct MBRRecord& orig)

// Compare the values, and return a bool result.
// Because this is intended for sorting and a lengthLBA value of 0 denotes
// a partition that's not in use and so that should be sorted upwards,
// we return the opposite of the usual arithmetic result when either
// lengthLBA value is 0.
bool MBRPart::operator<(const MBRPart &other) const {
   if (lengthLBA && other.lengthLBA)
      return (firstLBA < other.firstLBA);
   else
      return (other.firstLBA < firstLBA);
} // operator<()

/**************************************************
 *                                                *
 * Set information on partitions or disks without *
 * interacting with the user....                  *
 *                                                *
 **************************************************/

void MBRPart::SetGeometry(uint32_t heads, uint32_t sectors, uint64_t ds, uint32_t bs) {
   numHeads = heads;
   numSecspTrack = sectors;
   diskSize = ds;
   blockSize = bs;
} // MBRPart::SetGeometry

// Empty the partition (zero out all values).
void MBRPart::Empty(void) {
   status = UINT8_C(0);
   firstSector[0] = UINT8_C(0);
   firstSector[1] = UINT8_C(0);
   firstSector[2] = UINT8_C(0);
   partitionType = UINT8_C(0);
   lastSector[0] = UINT8_C(0);
   lastSector[1] = UINT8_C(0);
   lastSector[2] = UINT8_C(0);
   firstLBA = UINT32_C(0);
   lengthLBA = UINT32_C(0);
   includeAs = NONE;
} // MBRPart::Empty()

// Sets the type code, but silently refuses to change it to an extended type
// code.
// Returns 1 on success, 0 on failure (extended type code)
int MBRPart::SetType(uint8_t typeCode, int isExtended) {
   int allOK = 0;

   if ((isExtended == 1) || ((typeCode != 0x05) && (typeCode != 0x0f) && (typeCode != 0x85))) {
      partitionType = typeCode;
      allOK = 1;
   } // if
   return allOK;
} // MBRPart::SetType()

void MBRPart::SetStartLBA(uint64_t start) {
   if (start > UINT32_MAX)
      cerr << "Partition start out of range! Continuing, but problems now likely!\n";
   firstLBA = (uint32_t) start;
   RecomputeCHS();
} // MBRPart::SetStartLBA()

void MBRPart::SetLengthLBA(uint64_t length) {
   if (length > UINT32_MAX)
      cerr << "Partition length out of range! Continuing, but problems now likely!\n";
   lengthLBA = (uint32_t) length;
   RecomputeCHS();
} // MBRPart::SetLengthLBA()

// Set the start point and length of the partition. This function takes LBA
// values, sets them directly, and sets the CHS values based on the LBA
// values and the current geometry settings.
void MBRPart::SetLocation(uint64_t start, uint64_t length) {
   int validCHS;

   if ((start > UINT32_MAX) || (length > UINT32_MAX)) {
      cerr << "Partition values out of range in MBRPart::SetLocation()!\n"
           << "Continuing, but strange problems are now likely!\n";
   } // if
   firstLBA = (uint32_t) start;
   lengthLBA = (uint32_t) length;
   validCHS = RecomputeCHS();

   // If this is a complete 0xEE protective MBR partition, max out its
   // CHS last sector value, as per the GPT spec. (Set to 0xffffff,
   // although the maximum legal MBR value is 0xfeffff, which is
   // actually what GNU Parted and Apple's Disk Utility use, in
   // violation of the GPT spec.)
   if ((partitionType == 0xEE) && (!validCHS) && (firstLBA == 1) &&
       ((lengthLBA == diskSize - 1) || (lengthLBA == UINT32_MAX))) {
      lastSector[0] = lastSector[1] = lastSector[2] = 0xFF;
   } // if
} // MBRPart::SetLocation()

// Store the MBR data in the packed structure used for disk I/O...
void MBRPart::StoreInStruct(MBRRecord* theStruct) {
   int i;
   
   theStruct->firstLBA = firstLBA;
   theStruct->lengthLBA = lengthLBA;
   theStruct->partitionType = partitionType;
   theStruct->status = status;
   for (i = 0; i < 3; i++) {
      theStruct->firstSector[i] = firstSector[i];
      theStruct->lastSector[i] = lastSector[i];
   } // for
} // MBRPart::StoreInStruct()

/**********************************************
*                                            *
* Get information on partitions or disks.... *
*                                            *
**********************************************/

// Returns the last LBA value. Note that this can theoretically be a 33-bit
// value, so we return a 64-bit value. If lengthLBA == 0, returns 0, even if
// firstLBA is non-0.
uint64_t MBRPart::GetLastLBA(void) const {
   if (lengthLBA > 0)
      return (uint64_t) firstLBA + (uint64_t) lengthLBA - UINT64_C(1);
   else
      return 0;
} // MBRPart::GetLastLBA()

// Returns 1 if other overlaps with the current partition, 0 if they don't
// overlap
int MBRPart::DoTheyOverlap (const MBRPart& other) {
   return lengthLBA && other.lengthLBA &&
          (firstLBA <= other.GetLastLBA()) != (GetLastLBA() < other.firstLBA);
} // MBRPart::DoTheyOverlap()

/*************************************************
 *                                               *
 * Adjust information on partitions or disks.... *
 *                                               *
 *************************************************/

// Recompute the CHS values for the start and end points.
// Returns 1 if both computed values are within the range
// that can be expressed by that CHS, 0 otherwise.
int MBRPart::RecomputeCHS(void) {
   int retval = 1;

   if (lengthLBA > 0) {
      retval = LBAtoCHS(firstLBA, firstSector);
      retval *= LBAtoCHS(firstLBA + lengthLBA - 1, lastSector);
   } // if
   return retval;
} // MBRPart::RecomputeCHS()

// Converts 32-bit LBA value to MBR-style CHS value. Returns 1 if conversion
// was within the range that can be expressed by CHS (including 0, for an
// empty partition), 0 if the value is outside that range, and -1 if chs is
// invalid.
int MBRPart::LBAtoCHS(uint32_t lba, uint8_t * chs) {
   uint64_t cylinder, head, sector; // all numbered from 0
   uint64_t remainder;
   int retval = 1;
   int done = 0;

   if (chs != NULL) {
      // Special case: In case of 0 LBA value, zero out CHS values....
      if (lba == 0) {
         chs[0] = chs[1] = chs[2] = UINT8_C(0);
         done = 1;
      } // if
      // If LBA value is too large for CHS, max out CHS values....
      if ((!done) && (lba >= (numHeads * numSecspTrack * MAX_CYLINDERS))) {
         chs[0] = 254;
         chs[1] = chs[2] = 255;
         done = 1;
         retval = 0;
      } // if
      // If neither of the above applies, compute CHS values....
      if (!done) {
         cylinder = lba / (numHeads * numSecspTrack);
         remainder = lba - (cylinder * numHeads * numSecspTrack);
         head = remainder / numSecspTrack;
         remainder -= head * numSecspTrack;
         sector = remainder;
         if (head < numHeads)
            chs[0] = (uint8_t) head;
         else
            retval = 0;
         if (sector < numSecspTrack) {
            chs[1] = (uint8_t) ((sector + 1) + (cylinder >> 8) * 64);
            chs[2] = (uint8_t) (cylinder & UINT32_C(0xFF));
         } else {
            retval = 0;
         } // if/else
      } // if value is expressible and non-0
   } else { // Invalid (NULL) chs pointer
      retval = -1;
   } // if CHS pointer valid
   return (retval);
} // MBRPart::LBAtoCHS()

// Reverses the byte order, but only if we're on a big-endian platform.
// Note that most data come in 8-bit structures, so don't need reversing;
// only the LBA data needs to be reversed....
void MBRPart::ReverseByteOrder(void) {
   if (IsLittleEndian() == 0) {
      ReverseBytes(&firstLBA, 4);
      ReverseBytes(&lengthLBA, 4);
   } // if
} // MBRPart::ReverseByteOrder()

/**************************
 *                        *
 * User I/O functions.... *
 *                        *
 **************************/

// Show MBR data. Should update canBeLogical flags before calling.
// If isGpt == 1, omits the "can be logical" and "can be primary" columns.
void MBRPart::ShowData(int isGpt) {
   char bootCode = ' ';

   if (status & 0x80) // it's bootable
      bootCode = '*';
   cout.fill(' ');
   cout << bootCode << "  ";
   cout.width(13);
   cout << firstLBA;
   cout.width(13);
   cout << GetLastLBA() << "   ";
   switch (includeAs) {
      case PRIMARY:
         cout << "primary";
         break;
      case LOGICAL:
         cout << "logical";
         break;
      case NONE:
         cout << "omitted";
         break;
      default:
         cout << "error  ";
         break;
   } // switch
   cout.width(7);
   if (!isGpt) {
      if (canBeLogical)
         cout << "     Y      ";
      else
         cout << "            ";
      if (canBePrimary)
         cout << "  Y      ";
      else
         cout << "         ";
   } // if
   cout << "0x";
   cout.width(2);
   cout.fill('0');
   cout << hex << (int) partitionType << dec << "\n";
} // MBRPart::ShowData()
