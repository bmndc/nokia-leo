/* basicmbr.cc -- Functions for loading, saving, and manipulating legacy MBR partition
   data. */

/* Initial coding by Rod Smith, January to February, 2009 */

/* This program is copyright (c) 2009-2013 by Roderick W. Smith. It is distributed
  under the terms of the GNU GPL version 2, as detailed in the COPYING file. */

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>
#include <iostream>
#include <algorithm>
#include "mbr.h"
#include "support.h"

using namespace std;

/****************************************
 *                                      *
 * MBRData class and related structures *
 *                                      *
 ****************************************/

BasicMBRData::BasicMBRData(void) {
   blockSize = SECTOR_SIZE;
   diskSize = 0;
   device = "";
   state = invalid;
   numHeads = MAX_HEADS;
   numSecspTrack = MAX_SECSPERTRACK;
   myDisk = NULL;
   canDeleteMyDisk = 0;
//   memset(&EbrLocations, 0, MAX_MBR_PARTS * sizeof(uint32_t));
   EmptyMBR();
} // BasicMBRData default constructor

BasicMBRData::BasicMBRData(string filename) {
   blockSize = SECTOR_SIZE;
   diskSize = 0;
   device = filename;
   state = invalid;
   numHeads = MAX_HEADS;
   numSecspTrack = MAX_SECSPERTRACK;
   myDisk = NULL;
   canDeleteMyDisk = 0;
//   memset(&EbrLocations, 0, MAX_MBR_PARTS * sizeof(uint32_t));
   
   // Try to read the specified partition table, but if it fails....
   if (!ReadMBRData(filename)) {
      EmptyMBR();
      device = "";
   } // if
} // BasicMBRData(string filename) constructor

// Free space used by myDisk only if that's OK -- sometimes it will be
// copied from an outside source, in which case that source should handle
// it!
BasicMBRData::~BasicMBRData(void) {
   if (canDeleteMyDisk)
      delete myDisk;
} // BasicMBRData destructor

// Assignment operator -- copy entire set of MBR data.
BasicMBRData & BasicMBRData::operator=(const BasicMBRData & orig) {
   int i;

   memcpy(code, orig.code, 440);
   diskSignature = orig.diskSignature;
   nulls = orig.nulls;
   MBRSignature = orig.MBRSignature;
   blockSize = orig.blockSize;
   diskSize = orig.diskSize;
   numHeads = orig.numHeads;
   numSecspTrack = orig.numSecspTrack;
   canDeleteMyDisk = orig.canDeleteMyDisk;
   device = orig.device;
   state = orig.state;

   myDisk = new DiskIO;
   if (myDisk == NULL) {
      cerr << "Unable to allocate memory in BasicMBRData::operator=()! Terminating!\n";
      exit(1);
   } // if
   if (orig.myDisk != NULL)
      myDisk->OpenForRead(orig.myDisk->GetName());

   for (i = 0; i < MAX_MBR_PARTS; i++) {
      partitions[i] = orig.partitions[i];
   } // for
   return *this;
} // BasicMBRData::operator=()

/**********************
 *                    *
 * Disk I/O functions *
 *                    *
 **********************/

// Read data from MBR. Returns 1 if read was successful (even if the
// data isn't a valid MBR), 0 if the read failed.
int BasicMBRData::ReadMBRData(const string & deviceFilename) {
   int allOK = 1;

   if (myDisk == NULL) {
      myDisk = new DiskIO;
      if (myDisk == NULL) {
         cerr << "Unable to allocate memory in BasicMBRData::ReadMBRData()! Terminating!\n";
         exit(1);
      } // if
      canDeleteMyDisk = 1;
   } // if
   if (myDisk->OpenForRead(deviceFilename)) {
      allOK = ReadMBRData(myDisk);
   } else {
      allOK = 0;
   } // if

   if (allOK)
      device = deviceFilename;

   return allOK;
} // BasicMBRData::ReadMBRData(const string & deviceFilename)

// Read data from MBR. If checkBlockSize == 1 (the default), the block
// size is checked; otherwise it's set to the default (512 bytes).
// Note that any extended partition(s) present will be omitted from
// in the partitions[] array; these partitions must be re-created when
// the partition table is saved in MBR format.
int BasicMBRData::ReadMBRData(DiskIO * theDisk, int checkBlockSize) {
   int allOK = 1, i, logicalNum = 3;
   int err = 1;
   TempMBR tempMBR;

   if ((myDisk != NULL) && (myDisk != theDisk) && (canDeleteMyDisk)) {
      delete myDisk;
      canDeleteMyDisk = 0;
   } // if

   myDisk = theDisk;

   // Empty existing MBR data, including the logical partitions...
   EmptyMBR(0);

   if (myDisk->Seek(0))
     if (myDisk->Read(&tempMBR, 512))
        err = 0;
   if (err) {
      cerr << "Problem reading disk in BasicMBRData::ReadMBRData()!\n";
   } else {
      for (i = 0; i < 440; i++)
         code[i] = tempMBR.code[i];
      diskSignature = tempMBR.diskSignature;
      nulls = tempMBR.nulls;
      for (i = 0; i < 4; i++) {
         partitions[i] = tempMBR.partitions[i];
         if (partitions[i].GetLengthLBA() > 0)
            partitions[i].SetInclusion(PRIMARY);
      } // for i... (reading all four partitions)
      MBRSignature = tempMBR.MBRSignature;
      ReadCHSGeom();

      // Reverse the byte order, if necessary
      if (IsLittleEndian() == 0) {
         ReverseBytes(&diskSignature, 4);
         ReverseBytes(&nulls, 2);
         ReverseBytes(&MBRSignature, 2);
         for (i = 0; i < 4; i++) {
            partitions[i].ReverseByteOrder();
         } // for
      } // if

      if (MBRSignature != MBR_SIGNATURE) {
         allOK = 0;
         state = invalid;
      } // if

      // Find disk size
      diskSize = myDisk->DiskSize(&err);

      // Find block size
      if (checkBlockSize) {
         blockSize = myDisk->GetBlockSize();
      } // if (checkBlockSize)

      // Load logical partition data, if any is found....
      if (allOK) {
         for (i = 0; i < 4; i++) {
            if ((partitions[i].GetType() == 0x05) || (partitions[i].GetType() == 0x0f)
                || (partitions[i].GetType() == 0x85)) {
               // Found it, so call a function to load everything from them....
               logicalNum = ReadLogicalParts(partitions[i].GetStartLBA(), abs(logicalNum) + 1);
               if (logicalNum < 0) {
                  cerr << "Error reading logical partitions! List may be truncated!\n";
               } // if maxLogicals valid
               DeletePartition(i);
            } // if primary partition is extended
         } // for primary partition loop
         if (allOK) { // Loaded logicals OK
            state = mbr;
         } else {
            state = invalid;
         } // if
      } // if

      // Check to see if it's in GPT format....
      if (allOK) {
         for (i = 0; i < 4; i++) {
            if (partitions[i].GetType() == UINT8_C(0xEE)) {
               state = gpt;
            } // if
         } // for
      } // if

      // If there's an EFI GPT partition, look for other partition types,
      // to flag as hybrid
      if (state == gpt) {
         for (i = 0 ; i < 4; i++) {
            if ((partitions[i].GetType() != UINT8_C(0xEE)) &&
                (partitions[i].GetType() != UINT8_C(0x00)))
               state = hybrid;
            if (logicalNum != 3)
               cerr << "Warning! MBR Logical partitions found on a hybrid MBR disk! This is an\n"
                    << "EXTREMELY dangerous configuration!\n\a";
         } // for
      } // if (hybrid detection code)
   } // no initial error
   return allOK;
} // BasicMBRData::ReadMBRData(DiskIO * theDisk, int checkBlockSize)

// This is a function to read all the logical partitions, following the
// logical partition linked list from the disk and storing the basic data in the
// partitions[] array. Returns last index to partitions[] used, or -1 times the
// that index if there was a problem. (Some problems can leave valid logical
// partition data.)
// Parameters:
// extendedStart = LBA of the start of the extended partition
// partNum = number of first partition in extended partition (normally 4).
int BasicMBRData::ReadLogicalParts(uint64_t extendedStart, int partNum) {
   struct TempMBR ebr;
   int i, another = 1, allOK = 1;
   uint8_t ebrType;
   uint64_t offset;
   uint64_t EbrLocations[MAX_MBR_PARTS];

   offset = extendedStart;
   memset(&EbrLocations, 0, MAX_MBR_PARTS * sizeof(uint64_t));
   while (another && (partNum < MAX_MBR_PARTS) && (partNum >= 0) && (allOK > 0)) {
      for (i = 0; i < MAX_MBR_PARTS; i++) {
         if (EbrLocations[i] == offset) { // already read this one; infinite logical partition loop!
            cerr << "Logical partition infinite loop detected! This is being corrected.\n";
            allOK = -1;
            partNum -= 1;
         } // if
      } // for
      EbrLocations[partNum] = offset;
      if (myDisk->Seek(offset) == 0) { // seek to EBR record
         cerr << "Unable to seek to " << offset << "! Aborting!\n";
         allOK = -1;
      }
      if (myDisk->Read(&ebr, 512) != 512) { // Load the data....
         cerr << "Error seeking to or reading logical partition data from " << offset
              << "!\nSome logical partitions may be missing!\n";
         allOK = -1;
      } else if (IsLittleEndian() != 1) { // Reverse byte ordering of some data....
         ReverseBytes(&ebr.MBRSignature, 2);
         ReverseBytes(&ebr.partitions[0].firstLBA, 4);
         ReverseBytes(&ebr.partitions[0].lengthLBA, 4);
         ReverseBytes(&ebr.partitions[1].firstLBA, 4);
         ReverseBytes(&ebr.partitions[1].lengthLBA, 4);
      } // if/else/if

      if (ebr.MBRSignature != MBR_SIGNATURE) {
         allOK = -1;
         cerr << "EBR signature for logical partition invalid; read 0x";
         cerr.fill('0');
         cerr.width(4);
         cerr.setf(ios::uppercase);
         cerr << hex << ebr.MBRSignature << ", but should be 0x";
         cerr.width(4);
         cerr << MBR_SIGNATURE << dec << "\n";
         cerr.fill(' ');
      } // if

      if ((partNum >= 0) && (partNum < MAX_MBR_PARTS) && (allOK > 0)) {
         // Sometimes an EBR points directly to another EBR, rather than defining
         // a logical partition and then pointing to another EBR. Thus, we skip
         // the logical partition when this is the case....
         ebrType = ebr.partitions[0].partitionType;
         if ((ebrType == 0x05) || (ebrType == 0x0f) || (ebrType == 0x85)) {
            cout << "EBR describes a logical partition!\n";
            offset = extendedStart + ebr.partitions[0].firstLBA;
         } else {
            // Copy over the basic data....
            partitions[partNum] = ebr.partitions[0];
            // Adjust the start LBA, since it's encoded strangely....
            partitions[partNum].SetStartLBA(ebr.partitions[0].firstLBA + offset);
            partitions[partNum].SetInclusion(LOGICAL);
            
            // Find the next partition (if there is one)
            if ((ebr.partitions[1].firstLBA != UINT32_C(0)) && (partNum < (MAX_MBR_PARTS - 1))) {
               offset = extendedStart + ebr.partitions[1].firstLBA;
               partNum++;
            } else {
               another = 0;
            } // if another partition
         } // if/else
      } // if
   } // while()
   return (partNum * allOK);
} // BasicMBRData::ReadLogicalPart()

// Write the MBR data to the default defined device. This writes both the
// MBR itself and any defined logical partitions, provided there's an
// MBR extended partition.
int BasicMBRData::WriteMBRData(void) {
   int allOK = 1;

   if (myDisk != NULL) {
      if (myDisk->OpenForWrite() != 0) {
         allOK = WriteMBRData(myDisk);
         cout << "Done writing data!\n";
      } else {
         allOK = 0;
      } // if/else
      myDisk->Close();
   } else allOK = 0;
   return allOK;
} // BasicMBRData::WriteMBRData(void)

// Save the MBR data to a file. This writes both the
// MBR itself and any defined logical partitions.
int BasicMBRData::WriteMBRData(DiskIO *theDisk) {
   int i, j, partNum, next, allOK = 1, moreLogicals = 0;
   uint64_t extFirstLBA = 0;
   uint64_t writeEbrTo; // 64-bit because we support extended in 2-4TiB range
   TempMBR tempMBR;

   allOK = CreateExtended();
   if (allOK) {
      // First write the main MBR data structure....
      memcpy(tempMBR.code, code, 440);
      tempMBR.diskSignature = diskSignature;
      tempMBR.nulls = nulls;
      tempMBR.MBRSignature = MBRSignature;
      for (i = 0; i < 4; i++) {
         partitions[i].StoreInStruct(&tempMBR.partitions[i]);
         if (partitions[i].GetType() == 0x0f) {
            extFirstLBA = partitions[i].GetStartLBA();
            moreLogicals = 1;
         } // if
      } // for i...
   } // if
   allOK = allOK && WriteMBRData(tempMBR, theDisk, 0);

   // Set up tempMBR with some constant data for logical partitions...
   tempMBR.diskSignature = 0;
   for (i = 2; i < 4; i++) {
      tempMBR.partitions[i].firstLBA = tempMBR.partitions[i].lengthLBA = 0;
      tempMBR.partitions[i].partitionType = 0x00;
      for (j = 0; j < 3; j++) {
         tempMBR.partitions[i].firstSector[j] = 0;
         tempMBR.partitions[i].lastSector[j] = 0;
      } // for j
   } // for i

   partNum = FindNextInUse(4);
   writeEbrTo = (uint64_t) extFirstLBA;
   // Write logicals...
   while (allOK && moreLogicals && (partNum < MAX_MBR_PARTS) && (partNum >= 0)) {
      partitions[partNum].StoreInStruct(&tempMBR.partitions[0]);
      tempMBR.partitions[0].firstLBA = 1;
      // tempMBR.partitions[1] points to next EBR or terminates EBR linked list...
      next = FindNextInUse(partNum + 1);
      if ((next < MAX_MBR_PARTS) && (next > 0) && (partitions[next].GetStartLBA() > 0)) {
         tempMBR.partitions[1].partitionType = 0x0f;
         tempMBR.partitions[1].firstLBA = (uint32_t) (partitions[next].GetStartLBA() - extFirstLBA - 1);
         tempMBR.partitions[1].lengthLBA = (uint32_t) (partitions[next].GetLengthLBA() + 1);
         LBAtoCHS((uint64_t) tempMBR.partitions[1].firstLBA,
                  (uint8_t *) &tempMBR.partitions[1].firstSector);
         LBAtoCHS(tempMBR.partitions[1].lengthLBA - extFirstLBA,
                  (uint8_t *) &tempMBR.partitions[1].lastSector);
      } else {
         tempMBR.partitions[1].partitionType = 0x00;
         tempMBR.partitions[1].firstLBA = 0;
         tempMBR.partitions[1].lengthLBA = 0;
         moreLogicals = 0;
      } // if/else
      allOK = WriteMBRData(tempMBR, theDisk, writeEbrTo);
      writeEbrTo = (uint64_t) tempMBR.partitions[1].firstLBA + (uint64_t) extFirstLBA;
      partNum = next;
   } // while
   DeleteExtendedParts();
   return allOK;
} // BasicMBRData::WriteMBRData(DiskIO *theDisk)

int BasicMBRData::WriteMBRData(const string & deviceFilename) {
   device = deviceFilename;
   return WriteMBRData();
} // BasicMBRData::WriteMBRData(const string & deviceFilename)

// Write a single MBR record to the specified sector. Used by the like-named
// function to write both the MBR and multiple EBR (for logical partition)
// records.
// Returns 1 on success, 0 on failure
int BasicMBRData::WriteMBRData(struct TempMBR & mbr, DiskIO *theDisk, uint64_t sector) {
   int i, allOK;

   // Reverse the byte order, if necessary
   if (IsLittleEndian() == 0) {
      ReverseBytes(&mbr.diskSignature, 4);
      ReverseBytes(&mbr.nulls, 2);
      ReverseBytes(&mbr.MBRSignature, 2);
      for (i = 0; i < 4; i++) {
         ReverseBytes(&mbr.partitions[i].firstLBA, 4);
         ReverseBytes(&mbr.partitions[i].lengthLBA, 4);
      } // for
   } // if

   // Now write the data structure...
   allOK = theDisk->OpenForWrite();
   if (allOK && theDisk->Seek(sector)) {
      if (theDisk->Write(&mbr, 512) != 512) {
         allOK = 0;
         cerr << "Error " << errno << " when saving MBR!\n";
      } // if
   } else {
      allOK = 0;
      cerr << "Error " << errno << " when seeking to MBR to write it!\n";
   } // if/else
   theDisk->Close();

   // Reverse the byte order back, if necessary
   if (IsLittleEndian() == 0) {
      ReverseBytes(&mbr.diskSignature, 4);
      ReverseBytes(&mbr.nulls, 2);
      ReverseBytes(&mbr.MBRSignature, 2);
      for (i = 0; i < 4; i++) {
         ReverseBytes(&mbr.partitions[i].firstLBA, 4);
         ReverseBytes(&mbr.partitions[i].lengthLBA, 4);
      } // for
   }// if
   return allOK;
} // BasicMBRData::WriteMBRData(uint64_t sector)

// Set a new disk device; used in copying one disk's partition
// table to another disk.
void BasicMBRData::SetDisk(DiskIO *theDisk) {
   int err;

   myDisk = theDisk;
   diskSize = theDisk->DiskSize(&err);
   canDeleteMyDisk = 0;
   ReadCHSGeom();
} // BasicMBRData::SetDisk()

/********************************************
 *                                          *
 * Functions that display data for the user *
 *                                          *
 ********************************************/

// Show the MBR data to the user, up to the specified maximum number
// of partitions....
void BasicMBRData::DisplayMBRData(void) {
   int i;

   cout << "\nDisk size is " << diskSize << " sectors ("
        << BytesToIeee(diskSize, blockSize) << ")\n";
   cout << "MBR disk identifier: 0x";
   cout.width(8);
   cout.fill('0');
   cout.setf(ios::uppercase);
   cout << hex << diskSignature << dec << "\n";
   cout << "MBR partitions:\n\n";
   if ((state == gpt) || (state == hybrid)) {
      cout << "Number  Boot  Start Sector   End Sector   Status      Code\n";
   } else {
      cout << "                                                   Can Be   Can Be\n";
      cout << "Number  Boot  Start Sector   End Sector   Status   Logical  Primary   Code\n";
      UpdateCanBeLogical();
   } // 
   for (i = 0; i < MAX_MBR_PARTS; i++) {
      if (partitions[i].GetLengthLBA() != 0) {
         cout.fill(' ');
         cout.width(4);
         cout << i + 1 << "      ";
         partitions[i].ShowData((state == gpt) || (state == hybrid));
      } // if
      cout.fill(' ');
   } // for
} // BasicMBRData::DisplayMBRData()

// Displays the state, as a word, on stdout. Used for debugging & to
// tell the user about the MBR state when the program launches....
void BasicMBRData::ShowState(void) {
   switch (state) {
      case invalid:
         cout << "  MBR: not present\n";
         break;
      case gpt:
         cout << "  MBR: protective\n";
         break;
      case hybrid:
         cout << "  MBR: hybrid\n";
         break;
      case mbr:
         cout << "  MBR: MBR only\n";
         break;
      default:
         cout << "\a  MBR: unknown -- bug!\n";
         break;
   } // switch
} // BasicMBRData::ShowState()

/************************
 *                      *
 * GPT Checks and fixes *
 *                      *
 ************************/

// Perform a very rudimentary check for GPT data on the disk; searches for
// the GPT signature in the main and backup metadata areas.
// Returns 0 if GPT data not found, 1 if main data only is found, 2 if
// backup only is found, 3 if both main and backup data are found, and
// -1 if a disk error occurred.
int BasicMBRData::CheckForGPT(void) {
   int retval = 0, err;
   char signature1[9], signature2[9];

   if (myDisk != NULL) {
      if (myDisk->OpenForRead() != 0) {
         if (myDisk->Seek(1)) {
            myDisk->Read(signature1, 8);
            signature1[8] = '\0';
         } else retval = -1;
         if (myDisk->Seek(myDisk->DiskSize(&err) - 1)) {
            myDisk->Read(signature2, 8);
            signature2[8] = '\0';
         } else retval = -1;
         if ((retval >= 0) && (strcmp(signature1, "EFI PART") == 0))
            retval += 1;
         if ((retval >= 0) && (strcmp(signature2, "EFI PART") == 0))
            retval += 2;
      } else {
         retval = -1;
      } // if/else
      myDisk->Close();
   } else retval = -1;
   return retval;
} // BasicMBRData::CheckForGPT()

// Blanks the 2nd (sector #1, numbered from 0) and last sectors of the disk,
// but only if GPT data are verified on the disk, and only for the sector(s)
// with GPT signatures.
// Returns 1 if operation completes successfully, 0 if not (returns 1 if
// no GPT data are found on the disk).
int BasicMBRData::BlankGPTData(void) {
   int allOK = 1, err;
   uint8_t blank[512];

   memset(blank, 0, 512);
   switch (CheckForGPT()) {
      case -1:
         allOK = 0;
         break;
      case 0:
         break;
      case 1:
         if ((myDisk != NULL) && (myDisk->OpenForWrite())) {
            if (!((myDisk->Seek(1)) && (myDisk->Write(blank, 512) == 512)))
               allOK = 0;
            myDisk->Close();
         } else allOK = 0;
         break;
      case 2:
         if ((myDisk != NULL) && (myDisk->OpenForWrite())) {
            if (!((myDisk->Seek(myDisk->DiskSize(&err) - 1)) &&
               (myDisk->Write(blank, 512) == 512)))
               allOK = 0;
            myDisk->Close();
         } else allOK = 0;
         break;
      case 3:
         if ((myDisk != NULL) && (myDisk->OpenForWrite())) {
            if (!((myDisk->Seek(1)) && (myDisk->Write(blank, 512) == 512)))
               allOK = 0;
            if (!((myDisk->Seek(myDisk->DiskSize(&err) - 1)) &&
                (myDisk->Write(blank, 512) == 512)))
                allOK = 0;
            myDisk->Close();
         } else allOK = 0;
         break;
      default:
         break;
   } // switch()
   return allOK;
} // BasicMBRData::BlankGPTData

/*********************************************************************
 *                                                                   *
 * Functions that set or get disk metadata (CHS geometry, disk size, *
 * etc.)                                                             *
 *                                                                   *
 *********************************************************************/

// Read the CHS geometry using OS calls, or if that fails, set to
// the most common value for big disks (255 heads, 63 sectors per
// track, & however many cylinders that computes to).
void BasicMBRData::ReadCHSGeom(void) {
   int err;

   numHeads = myDisk->GetNumHeads();
   numSecspTrack = myDisk->GetNumSecsPerTrack();
   diskSize = myDisk->DiskSize(&err);
   blockSize = myDisk->GetBlockSize();
   partitions[0].SetGeometry(numHeads, numSecspTrack, diskSize, blockSize);
} // BasicMBRData::ReadCHSGeom()

// Find the low and high used partition numbers (numbered from 0).
// Return value is the number of partitions found. Note that the
// *low and *high values are both set to 0 when no partitions
// are found, as well as when a single partition in the first
// position exists. Thus, the return value is the only way to
// tell when no partitions exist.
int BasicMBRData::GetPartRange(uint32_t *low, uint32_t *high) {
   uint32_t i;
   int numFound = 0;

   *low = MAX_MBR_PARTS + 1; // code for "not found"
   *high = 0;
   for (i = 0; i < MAX_MBR_PARTS; i++) {
      if (partitions[i].GetStartLBA() != UINT32_C(0)) { // it exists
         *high = i; // since we're counting up, set the high value
         // Set the low value only if it's not yet found...
         if (*low == (MAX_MBR_PARTS + 1))
            *low = i;
         numFound++;
      } // if
   } // for

   // Above will leave *low pointing to its "not found" value if no partitions
   // are defined, so reset to 0 if this is the case....
   if (*low == (MAX_MBR_PARTS + 1))
      *low = 0;
   return numFound;
} // GPTData::GetPartRange()

// Converts 64-bit LBA value to MBR-style CHS value. Returns 1 if conversion
// was within the range that can be expressed by CHS (including 0, for an
// empty partition), 0 if the value is outside that range, and -1 if chs is
// invalid.
int BasicMBRData::LBAtoCHS(uint64_t lba, uint8_t * chs) {
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
      if ((!done) && (lba >= ((uint64_t) numHeads * numSecspTrack * MAX_CYLINDERS))) {
         chs[0] = 254;
         chs[1] = chs[2] = 255;
         done = 1;
         retval = 0;
      } // if
      // If neither of the above applies, compute CHS values....
      if (!done) {
         cylinder = lba / (uint64_t) (numHeads * numSecspTrack);
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
            chs[2] = (uint8_t) (cylinder & UINT64_C(0xFF));
         } else {
            retval = 0;
         } // if/else
      } // if value is expressible and non-0
   } else { // Invalid (NULL) chs pointer
      retval = -1;
   } // if CHS pointer valid
   return (retval);
} // BasicMBRData::LBAtoCHS()

// Look for overlapping partitions. Also looks for a couple of non-error
// conditions that the user should be told about.
// Returns the number of problems found
int BasicMBRData::FindOverlaps(void) {
   int i, j, numProbs = 0, numEE = 0, ProtectiveOnOne = 0;

   for (i = 0; i < MAX_MBR_PARTS; i++) {
      for (j = i + 1; j < MAX_MBR_PARTS; j++) {
         if ((partitions[i].GetInclusion() != NONE) && (partitions[j].GetInclusion() != NONE) &&
             (partitions[i].DoTheyOverlap(partitions[j]))) {
            numProbs++;
            cout << "\nProblem: MBR partitions " << i + 1 << " and " << j + 1
                 << " overlap!\n";
         } // if
      } // for (j...)
      if (partitions[i].GetType() == 0xEE) {
         numEE++;
         if (partitions[i].GetStartLBA() == 1)
            ProtectiveOnOne = 1;
      } // if
   } // for (i...)

   if (numEE > 1)
      cout << "\nCaution: More than one 0xEE MBR partition found. This can cause problems\n"
           << "in some OSes.\n";
   if (!ProtectiveOnOne && (numEE > 0))
      cout << "\nWarning: 0xEE partition doesn't start on sector 1. This can cause "
           << "problems\nin some OSes.\n";

   return numProbs;
} // BasicMBRData::FindOverlaps()

// Returns the number of primary partitions, including the extended partition
// required to hold any logical partitions found.
int BasicMBRData::NumPrimaries(void) {
   int i, numPrimaries = 0, logicalsFound = 0;

   for (i = 0; i < MAX_MBR_PARTS; i++) {
      if (partitions[i].GetLengthLBA() > 0) {
         if (partitions[i].GetInclusion() == PRIMARY)
            numPrimaries++;
         if (partitions[i].GetInclusion() == LOGICAL)
            logicalsFound = 1;
      } // if
   } // for
   return (numPrimaries + logicalsFound);
} // BasicMBRData::NumPrimaries()

// Returns the number of logical partitions.
int BasicMBRData::NumLogicals(void) {
   int i, numLogicals = 0;

   for (i = 0; i < MAX_MBR_PARTS; i++) {
      if (partitions[i].GetInclusion() == LOGICAL)
         numLogicals++;
   } // for
   return numLogicals;
} // BasicMBRData::NumLogicals()

// Returns the number of partitions (primaries plus logicals), NOT including
// the extended partition required to house the logicals.
int BasicMBRData::CountParts(void) {
   int i, num = 0;

   for (i = 0; i < MAX_MBR_PARTS; i++) {
      if ((partitions[i].GetInclusion() == LOGICAL) ||
          (partitions[i].GetInclusion() == PRIMARY))
         num++;
   } // for
   return num;
} // BasicMBRData::CountParts()

// Updates the canBeLogical and canBePrimary flags for all the partitions.
void BasicMBRData::UpdateCanBeLogical(void) {
   int i, j, sectorBefore, numPrimaries, numLogicals, usedAsEBR;
   uint64_t firstLogical, lastLogical, lStart, pStart;

   numPrimaries = NumPrimaries();
   numLogicals = NumLogicals();
   firstLogical = FirstLogicalLBA() - 1;
   lastLogical = LastLogicalLBA();
   for (i = 0; i < MAX_MBR_PARTS; i++) {
      usedAsEBR = (SectorUsedAs(partitions[i].GetLastLBA()) == EBR);
      if (usedAsEBR) {
         partitions[i].SetCanBeLogical(0);
         partitions[i].SetCanBePrimary(0);
      } else if (partitions[i].GetLengthLBA() > 0) {
         // First determine if it can be logical....
         sectorBefore = SectorUsedAs(partitions[i].GetStartLBA() - 1);
         lStart = partitions[i].GetStartLBA(); // start of potential logical part.
         if ((lastLogical > 0) &&
             ((sectorBefore == EBR) || (sectorBefore == NONE))) {
            // Assume it can be logical, then search for primaries that make it
            // not work and, if found, flag appropriately.
            partitions[i].SetCanBeLogical(1);
            for (j = 0; j < MAX_MBR_PARTS; j++) {
               if ((i != j) && (partitions[j].GetInclusion() == PRIMARY)) {
                  pStart = partitions[j].GetStartLBA();
                  if (((pStart < lStart) && (firstLogical < pStart)) ||
                      ((pStart > lStart) && (firstLogical > pStart))) {
                     partitions[i].SetCanBeLogical(0);
                  } // if/else
               } // if
            } // for
         } else {
            if ((sectorBefore != EBR) && (sectorBefore != NONE))
               partitions[i].SetCanBeLogical(0);
            else
               partitions[i].SetCanBeLogical(lastLogical == 0); // can be logical only if no logicals already
         } // if/else
         // Now determine if it can be primary. Start by assuming it can be...
         partitions[i].SetCanBePrimary(1);
         if ((numPrimaries >= 4) && (partitions[i].GetInclusion() != PRIMARY)) {
            partitions[i].SetCanBePrimary(0);
            if ((partitions[i].GetInclusion() == LOGICAL) && (numLogicals == 1) &&
                (numPrimaries == 4))
               partitions[i].SetCanBePrimary(1);
         } // if
         if ((partitions[i].GetStartLBA() > (firstLogical + 1)) &&
             (partitions[i].GetLastLBA() < lastLogical))
            partitions[i].SetCanBePrimary(0);
      } // else if
   } // for
} // BasicMBRData::UpdateCanBeLogical()

// Returns the first sector occupied by any logical partition. Note that
// this does NOT include the logical partition's EBR! Returns UINT32_MAX
// if there are no logical partitions defined.
uint64_t BasicMBRData::FirstLogicalLBA(void) {
   int i;
   uint64_t firstFound = UINT32_MAX;

   for (i = 0; i < MAX_MBR_PARTS; i++) {
      if ((partitions[i].GetInclusion() == LOGICAL) &&
          (partitions[i].GetStartLBA() < firstFound)) {
         firstFound = partitions[i].GetStartLBA();
      } // if
   } // for
   return firstFound;
} // BasicMBRData::FirstLogicalLBA()

// Returns the last sector occupied by any logical partition, or 0 if
// there are no logical partitions defined.
uint64_t BasicMBRData::LastLogicalLBA(void) {
   int i;
   uint64_t lastFound = 0;

   for (i = 0; i < MAX_MBR_PARTS; i++) {
      if ((partitions[i].GetInclusion() == LOGICAL) &&
          (partitions[i].GetLastLBA() > lastFound))
         lastFound = partitions[i].GetLastLBA();
   } // for
   return lastFound;
} // BasicMBRData::LastLogicalLBA()

// Returns 1 if logical partitions are contiguous (have no primaries
// in their midst), or 0 if one or more primaries exist between
// logicals.
int BasicMBRData::AreLogicalsContiguous(void) {
   int allOK = 1, i = 0;
   uint64_t firstLogical, lastLogical;

   firstLogical = FirstLogicalLBA() - 1; // subtract 1 for EBR
   lastLogical = LastLogicalLBA();
   if (lastLogical > 0) {
      do {
         if ((partitions[i].GetInclusion() == PRIMARY) &&
             (partitions[i].GetStartLBA() >= firstLogical) &&
             (partitions[i].GetStartLBA() <= lastLogical)) {
            allOK = 0;
         } // if
         i++;
      } while ((i < MAX_MBR_PARTS) && allOK);
   } // if
   return allOK;
} // BasicMBRData::AreLogicalsContiguous()

// Returns 1 if all partitions fit on the disk, given its size; 0 if any
// partition is too big.
int BasicMBRData::DoTheyFit(void) {
   int i, allOK = 1;

   for (i = 0; i < MAX_MBR_PARTS; i++) {
      if ((partitions[i].GetStartLBA() > diskSize) || (partitions[i].GetLastLBA() > diskSize)) {
         allOK = 0;
      } // if
   } // for
   return allOK;
} // BasicMBRData::DoTheyFit(void)

// Returns 1 if there's at least one free sector immediately preceding
// all partitions flagged as logical; 0 if any logical partition lacks
// this space.
int BasicMBRData::SpaceBeforeAllLogicals(void) {
   int i = 0, allOK = 1;

   do {
      if ((partitions[i].GetStartLBA() > 0) && (partitions[i].GetInclusion() == LOGICAL)) {
         allOK = allOK && (SectorUsedAs(partitions[i].GetStartLBA() - 1) == EBR);
      } // if
      i++;
   } while (allOK && (i < MAX_MBR_PARTS));
   return allOK;
} // BasicMBRData::SpaceBeforeAllLogicals()

// Returns 1 if the partitions describe a legal layout -- all logicals
// are contiguous and have at least one preceding empty sector,
// the number of primaries is under 4 (or under 3 if there are any
// logicals), there are no overlapping partitions, etc.
// Does NOT assume that primaries are numbered 1-4; uses the
// IsItPrimary() function of the MBRPart class to determine
// primary status. Also does NOT consider partition order; there
// can be gaps and it will still be considered legal.
int BasicMBRData::IsLegal(void) {
   int allOK = 1;

   allOK = (FindOverlaps() == 0);
   allOK = (allOK && (NumPrimaries() <= 4));
   allOK = (allOK && AreLogicalsContiguous());
   allOK = (allOK && DoTheyFit());
   allOK = (allOK && SpaceBeforeAllLogicals());
   return allOK;
} // BasicMBRData::IsLegal()

// Returns 1 if the 0xEE partition in the protective/hybrid MBR is marked as
// active/bootable.
int BasicMBRData::IsEEActive(void) {
   int i, IsActive = 0;

   for (i = 0; i < MAX_MBR_PARTS; i++) {
      if ((partitions[i].GetStatus() & 0x80) && (partitions[i].GetType() == 0xEE))
         IsActive = 1;
   }
   return IsActive;
} // BasicMBRData::IsEEActive()

// Finds the next in-use partition, starting with start (will return start
// if it's in use). Returns -1 if no subsequent partition is in use.
int BasicMBRData::FindNextInUse(int start) {
   if (start >= MAX_MBR_PARTS)
      start = -1;
   while ((start < MAX_MBR_PARTS) && (start >= 0) && (partitions[start].GetInclusion() == NONE))
      start++;
   if ((start < 0) || (start >= MAX_MBR_PARTS))
      start = -1;
   return start;
} // BasicMBRData::FindFirstLogical();

/*****************************************************
 *                                                   *
 * Functions to create, delete, or change partitions *
 *                                                   *
 *****************************************************/

// Empty all data. Meant mainly for calling by constructors, but it's also
// used by the hybrid MBR functions in the GPTData class.
void BasicMBRData::EmptyMBR(int clearBootloader) {
   int i;

   // Zero out the boot loader section, the disk signature, and the
   // 2-byte nulls area only if requested to do so. (This is the
   // default.)
   if (clearBootloader == 1) {
      EmptyBootloader();
   } // if

   // Blank out the partitions
   for (i = 0; i < MAX_MBR_PARTS; i++) {
      partitions[i].Empty();
   } // for
   MBRSignature = MBR_SIGNATURE;
   state = mbr;
} // BasicMBRData::EmptyMBR()

// Blank out the boot loader area. Done with the initial MBR-to-GPT
// conversion, since MBR boot loaders don't understand GPT, and so
// need to be replaced....
void BasicMBRData::EmptyBootloader(void) {
   int i;

   for (i = 0; i < 440; i++)
      code[i] = 0;
   nulls = 0;
} // BasicMBRData::EmptyBootloader

// Create a partition of the specified number based on the passed
// partition. This function does *NO* error checking, so it's possible
// to seriously screw up a partition table using this function!
// Note: This function should NOT be used to create the 0xEE partition
// in a conventional GPT configuration, since that partition has
// specific size requirements that this function won't handle. It may
// be used for creating the 0xEE partition(s) in a hybrid MBR, though,
// since those toss the rulebook away anyhow....
void BasicMBRData::AddPart(int num, const MBRPart& newPart) {
   partitions[num] = newPart;
} // BasicMBRData::AddPart()

// Create a partition of the specified number, starting LBA, and
// length. This function does almost no error checking, so it's possible
// to seriously screw up a partition table using this function!
// Note: This function should NOT be used to create the 0xEE partition
// in a conventional GPT configuration, since that partition has
// specific size requirements that this function won't handle. It may
// be used for creating the 0xEE partition(s) in a hybrid MBR, though,
// since those toss the rulebook away anyhow....
void BasicMBRData::MakePart(int num, uint64_t start, uint64_t length, int type, int bootable) {
   if ((num >= 0) && (num < MAX_MBR_PARTS) && (start <= UINT32_MAX) && (length <= UINT32_MAX)) {
      partitions[num].Empty();
      partitions[num].SetType(type);
      partitions[num].SetLocation(start, length);
      if (num < 4)
         partitions[num].SetInclusion(PRIMARY);
      else
         partitions[num].SetInclusion(LOGICAL);
      SetPartBootable(num, bootable);
   } // if valid partition number & size
} // BasicMBRData::MakePart()

// Set the partition's type code.
// Returns 1 if successful, 0 if not (invalid partition number)
int BasicMBRData::SetPartType(int num, int type) {
   int allOK = 1;

   if ((num >= 0) && (num < MAX_MBR_PARTS)) {
      if (partitions[num].GetLengthLBA() != UINT32_C(0)) {
         allOK = partitions[num].SetType(type);
      } else allOK = 0;
   } else allOK = 0;
   return allOK;
} // BasicMBRData::SetPartType()

// Set (or remove) the partition's bootable flag. Setting it is the
// default; pass 0 as bootable to remove the flag.
// Returns 1 if successful, 0 if not (invalid partition number)
int BasicMBRData::SetPartBootable(int num, int bootable) {
   int allOK = 1;

   if ((num >= 0) && (num < MAX_MBR_PARTS)) {
      if (partitions[num].GetLengthLBA() != UINT32_C(0)) {
         if (bootable == 0)
            partitions[num].SetStatus(UINT8_C(0x00));
         else
            partitions[num].SetStatus(UINT8_C(0x80));
      } else allOK = 0;
   } else allOK = 0;
   return allOK;
} // BasicMBRData::SetPartBootable()

// Create a partition that fills the most available space. Returns
// 1 if partition was created, 0 otherwise. Intended for use in
// creating hybrid MBRs.
int BasicMBRData::MakeBiggestPart(int i, int type) {
   uint64_t start = UINT64_C(1); // starting point for each search
   uint64_t firstBlock; // first block in a segment
   uint64_t lastBlock; // last block in a segment
   uint64_t segmentSize; // size of segment in blocks
   uint64_t selectedSegment = UINT64_C(0); // location of largest segment
   uint64_t selectedSize = UINT64_C(0); // size of largest segment in blocks
   int found = 0;
   string anything;

   do {
      firstBlock = FindFirstAvailable(start);
      if (firstBlock > UINT64_C(0)) { // something's free...
         lastBlock = FindLastInFree(firstBlock);
         segmentSize = lastBlock - firstBlock + UINT64_C(1);
         if (segmentSize > selectedSize) {
            selectedSize = segmentSize;
            selectedSegment = firstBlock;
         } // if
         start = lastBlock + 1;
      } // if
   } while (firstBlock != 0);
   if ((selectedSize > UINT64_C(0)) && (selectedSize < diskSize)) {
      found = 1;
      MakePart(i, selectedSegment, selectedSize, type, 0);
   } else {
      found = 0;
   } // if/else
   return found;
} // BasicMBRData::MakeBiggestPart(int i)

// Delete partition #i
void BasicMBRData::DeletePartition(int i) {
   partitions[i].Empty();
} // BasicMBRData::DeletePartition()

// Set the inclusion status (PRIMARY, LOGICAL, or NONE) with some sanity
// checks to ensure the table remains legal.
// Returns 1 on success, 0 on failure.
int BasicMBRData::SetInclusionwChecks(int num, int inclStatus) {
   int allOK = 1, origValue;

   if (IsLegal()) {
      if ((inclStatus == PRIMARY) || (inclStatus == LOGICAL) || (inclStatus == NONE)) {
         origValue = partitions[num].GetInclusion();
         partitions[num].SetInclusion(inclStatus);
         if (!IsLegal()) {
            partitions[num].SetInclusion(origValue);
            cerr << "Specified change is not legal! Aborting change!\n";
         } // if
      } else {
         cerr << "Invalid partition inclusion code in BasicMBRData::SetInclusionwChecks()!\n";
      } // if/else
   } else {
      cerr << "Partition table is not currently in a valid state. Aborting change!\n";
      allOK = 0;
   } // if/else
   return allOK;
} // BasicMBRData::SetInclusionwChecks()

// Recomputes the CHS values for the specified partition and adjusts the value.
// Note that this will create a technically incorrect CHS value for EFI GPT (0xEE)
// protective partitions, but this is required by some buggy BIOSes, so I'm
// providing a function to do this deliberately at the user's command.
// This function does nothing if the partition's length is 0.
void BasicMBRData::RecomputeCHS(int partNum) {
   partitions[partNum].RecomputeCHS();
} // BasicMBRData::RecomputeCHS()

// Sorts the partitions starting with partition #start. This function
// does NOT pay attention to primary/logical assignment, which is
// critical when writing the partitions.
void BasicMBRData::SortMBR(int start) {
   if ((start < MAX_MBR_PARTS) && (start >= 0))
      sort(partitions + start, partitions + MAX_MBR_PARTS);
} // BasicMBRData::SortMBR()

// Delete any partitions that are too big to fit on the disk
// or that are too big for MBR (32-bit limits).
// This deletes the partitions by setting values to 0, not just
// by setting them as being omitted.
// Returns the number of partitions deleted in this way.
int BasicMBRData::DeleteOversizedParts() {
   int num = 0, i;

   for (i = 0; i < MAX_MBR_PARTS; i++) {
      if ((partitions[i].GetStartLBA() > diskSize) || (partitions[i].GetLastLBA() > diskSize) ||
          (partitions[i].GetStartLBA() > UINT32_MAX) || (partitions[i].GetLengthLBA() > UINT32_MAX)) {
         cerr << "\aWarning: Deleting oversized partition #" << i + 1 << "! Start = "
              << partitions[i].GetStartLBA() << ", length = " << partitions[i].GetLengthLBA() << "\n";
         partitions[i].Empty();
         num++;
      } // if
   } // for
   return num;
} // BasicMBRData::DeleteOversizedParts()

// Search for and delete extended partitions.
// Returns the number of partitions deleted.
int BasicMBRData::DeleteExtendedParts() {
   int i, numDeleted = 0;
   uint8_t type;

   for (i = 0; i < MAX_MBR_PARTS; i++) {
      type = partitions[i].GetType();
      if (((type == 0x05) || (type == 0x0f) || (type == (0x85))) &&
          (partitions[i].GetLengthLBA() > 0)) {
         partitions[i].Empty();
         numDeleted++;
      } // if
   } // for
   return numDeleted;
} // BasicMBRData::DeleteExtendedParts()

// Finds any overlapping partitions and omits the smaller of the two.
void BasicMBRData::OmitOverlaps() {
   int i, j;

   for (i = 0; i < MAX_MBR_PARTS; i++) {
      for (j = i + 1; j < MAX_MBR_PARTS; j++) {
         if ((partitions[i].GetInclusion() != NONE) &&
             partitions[i].DoTheyOverlap(partitions[j])) {
            if (partitions[i].GetLengthLBA() < partitions[j].GetLengthLBA())
               partitions[i].SetInclusion(NONE);
            else
               partitions[j].SetInclusion(NONE);
         } // if
      } // for (j...)
   } // for (i...)
} // BasicMBRData::OmitOverlaps()

// Convert as many partitions into logicals as possible, except for
// the first partition, if possible.
void BasicMBRData::MaximizeLogicals() {
   int earliestPart = 0, earliestPartWas = NONE, i;

   for (i = MAX_MBR_PARTS - 1; i >= 0; i--) {
      UpdateCanBeLogical();
      earliestPart = i;
      if (partitions[i].CanBeLogical()) {
         partitions[i].SetInclusion(LOGICAL);
      } else if (partitions[i].CanBePrimary()) {
         partitions[i].SetInclusion(PRIMARY);
      } else {
         partitions[i].SetInclusion(NONE);
      } // if/elseif/else
   } // for
   // If we have spare primaries, convert back the earliest partition to
   // its original state....
   if ((NumPrimaries() < 4) && (partitions[earliestPart].GetInclusion() == LOGICAL))
      partitions[earliestPart].SetInclusion(earliestPartWas);
} // BasicMBRData::MaximizeLogicals()

// Add primaries up to the maximum allowed, from the omitted category.
void BasicMBRData::MaximizePrimaries() {
   int num, i = 0;

   num = NumPrimaries();
   while ((num < 4) && (i < MAX_MBR_PARTS)) {
      if ((partitions[i].GetInclusion() == NONE) && (partitions[i].CanBePrimary())) {
         partitions[i].SetInclusion(PRIMARY);
         num++;
         UpdateCanBeLogical();
      } // if
      i++;
   } // while
} // BasicMBRData::MaximizePrimaries()

// Remove primary partitions in excess of 4, starting with the later ones,
// in terms of the array location....
void BasicMBRData::TrimPrimaries(void) {
   int numToDelete, i = MAX_MBR_PARTS - 1;

   numToDelete = NumPrimaries() - 4;
   while ((numToDelete > 0) && (i >= 0)) {
      if (partitions[i].GetInclusion() == PRIMARY) {
         partitions[i].SetInclusion(NONE);
         numToDelete--;
      } // if
      i--;
   } // while (numToDelete > 0)
} // BasicMBRData::TrimPrimaries()

// Locates primary partitions located between logical partitions and
// either converts the primaries into logicals (if possible) or omits
// them.
void BasicMBRData::MakeLogicalsContiguous(void) {
   uint64_t firstLogicalLBA, lastLogicalLBA;
   int i;

   firstLogicalLBA = FirstLogicalLBA();
   lastLogicalLBA = LastLogicalLBA();
   for (i = 0; i < MAX_MBR_PARTS; i++) {
      if ((partitions[i].GetInclusion() == PRIMARY) &&
          (partitions[i].GetStartLBA() >= firstLogicalLBA) &&
          (partitions[i].GetLastLBA() <= lastLogicalLBA)) {
         if (SectorUsedAs(partitions[i].GetStartLBA() - 1) == NONE)
            partitions[i].SetInclusion(LOGICAL);
         else
            partitions[i].SetInclusion(NONE);
      } // if
   } // for
} // BasicMBRData::MakeLogicalsContiguous()

// If MBR data aren't legal, adjust primary/logical assignments and,
// if necessary, drop partitions, to make the data legal.
void BasicMBRData::MakeItLegal(void) {
   if (!IsLegal()) {
      DeleteOversizedParts();
      MaximizeLogicals();
      MaximizePrimaries();
      if (!AreLogicalsContiguous())
         MakeLogicalsContiguous();
      if (NumPrimaries() > 4)
         TrimPrimaries();
      OmitOverlaps();
   } // if
} // BasicMBRData::MakeItLegal()

// Removes logical partitions and deactivated partitions from first four
// entries (primary space).
// Returns the number of partitions moved.
int BasicMBRData::RemoveLogicalsFromFirstFour(void) {
   int i, j = 4, numMoved = 0, swapped = 0;
   MBRPart temp;

   for (i = 0; i < 4; i++) {
      if ((partitions[i].GetInclusion() != PRIMARY) && (partitions[i].GetLengthLBA() > 0)) {
         j = 4;
         swapped = 0;
         do {
            if ((partitions[j].GetInclusion() == NONE) && (partitions[j].GetLengthLBA() == 0)) {
               temp = partitions[j];
               partitions[j] = partitions[i];
               partitions[i] = temp;
               swapped = 1;
               numMoved++;
            } // if
            j++;
         } while ((j < MAX_MBR_PARTS) && !swapped);
         if (j >= MAX_MBR_PARTS)
            cerr << "Warning! Too many partitions in BasicMBRData::RemoveLogicalsFromFirstFour()!\n";
      } // if
   } // for i...
   return numMoved;
} // BasicMBRData::RemoveLogicalsFromFirstFour()

// Move all primaries into the first four partition spaces
// Returns the number of partitions moved.
int BasicMBRData::MovePrimariesToFirstFour(void) {
   int i, j = 0, numMoved = 0, swapped = 0;
   MBRPart temp;

   for (i = 4; i < MAX_MBR_PARTS; i++) {
      if (partitions[i].GetInclusion() == PRIMARY) {
         j = 0;
         swapped = 0;
         do {
            if (partitions[j].GetInclusion() != PRIMARY) {
               temp = partitions[j];
               partitions[j] = partitions[i];
               partitions[i] = temp;
               swapped = 1;
               numMoved++;
            } // if
            j++;
         } while ((j < 4) && !swapped);
      } // if
   } // for
   return numMoved;
} // BasicMBRData::MovePrimariesToFirstFour()

// Create an extended partition, if necessary, to hold the logical partitions.
// This function also sorts the primaries into the first four positions of
// the table.
// Returns 1 on success, 0 on failure.
int BasicMBRData::CreateExtended(void) {
   int allOK = 1, i = 0, swapped = 0;
   MBRPart temp;

   if (IsLegal()) {
      // Move logicals out of primary space...
      RemoveLogicalsFromFirstFour();
      // Move primaries out of logical space...
      MovePrimariesToFirstFour();

      // Create the extended partition
      if (NumLogicals() > 0) {
         SortMBR(4); // sort starting from 4 -- that is, logicals only
         temp.Empty();
         temp.SetStartLBA(FirstLogicalLBA() - 1);
         temp.SetLengthLBA(LastLogicalLBA() - FirstLogicalLBA() + 2);
         temp.SetType(0x0f, 1);
         temp.SetInclusion(PRIMARY);
         do {
            if ((partitions[i].GetInclusion() == NONE) || (partitions[i].GetLengthLBA() == 0)) {
               partitions[i] = temp;
               swapped = 1;
            } // if
            i++;
         } while ((i < 4) && !swapped);
         if (!swapped) {
            cerr << "Could not create extended partition; no room in primary table!\n";
            allOK = 0;
         } // if
      } // if (NumLogicals() > 0)
   } else allOK = 0;
   // Do a final check for EFI GPT (0xEE) partitions & flag as a problem if found
   // along with an extended partition
   for (i = 0; i < MAX_MBR_PARTS; i++)
      if (swapped && partitions[i].GetType() == 0xEE)
         allOK = 0;
   return allOK;
} // BasicMBRData::CreateExtended()

/****************************************
 *                                      *
 * Functions to find data on free space *
 *                                      *
 ****************************************/

// Finds the first free space on the disk from start onward; returns 0
// if none available....
uint64_t BasicMBRData::FindFirstAvailable(uint64_t start) {
   uint64_t first;
   uint64_t i;
   int firstMoved;

   if ((start >= (UINT32_MAX - 1)) || (start >= (diskSize - 1)))
      return 0;

   first = start;

   // ...now search through all partitions; if first is within an
   // existing partition, move it to the next sector after that
   // partition and repeat. If first was moved, set firstMoved
   // flag; repeat until firstMoved is not set, so as to catch
   // cases where partitions are out of sequential order....
   do {
      firstMoved = 0;
      for (i = 0; i < 4; i++) {
         // Check if it's in the existing partition
         if ((first >= partitions[i].GetStartLBA()) &&
             (first < (partitions[i].GetStartLBA() + partitions[i].GetLengthLBA()))) {
            first = partitions[i].GetStartLBA() + partitions[i].GetLengthLBA();
            firstMoved = 1;
         } // if
      } // for
   } while (firstMoved == 1);
   if ((first >= diskSize) || (first > UINT32_MAX))
      first = 0;
   return (first);
} // BasicMBRData::FindFirstAvailable()

// Finds the last free sector on the disk from start forward.
uint64_t BasicMBRData::FindLastInFree(uint64_t start) {
   uint64_t nearestStart;
   uint64_t i;

   if ((diskSize <= UINT32_MAX) && (diskSize > 0))
      nearestStart = diskSize - 1;
   else
      nearestStart = UINT32_MAX - 1;

   for (i = 0; i < 4; i++) {
      if ((nearestStart > partitions[i].GetStartLBA()) &&
          (partitions[i].GetStartLBA() > start)) {
         nearestStart = partitions[i].GetStartLBA() - 1;
      } // if
   } // for
   return (nearestStart);
} // BasicMBRData::FindLastInFree()

// Finds the first free sector on the disk from start backward.
uint64_t BasicMBRData::FindFirstInFree(uint64_t start) {
   uint64_t bestLastLBA, thisLastLBA;
   int i;

   bestLastLBA = 1;
   for (i = 0; i < 4; i++) {
      thisLastLBA = partitions[i].GetLastLBA() + 1;
      if (thisLastLBA > 0)
         thisLastLBA--;
      if ((thisLastLBA > bestLastLBA) && (thisLastLBA < start))
         bestLastLBA = thisLastLBA + 1;
   } // for
   return (bestLastLBA);
} // BasicMBRData::FindFirstInFree()

// Returns NONE (unused), PRIMARY, LOGICAL, EBR (for EBR or MBR), or INVALID.
// Note: If the sector immediately before a logical partition is in use by
// another partition, this function returns PRIMARY or LOGICAL for that
// sector, rather than EBR.
int BasicMBRData::SectorUsedAs(uint64_t sector, int topPartNum) {
   int i = 0, usedAs = NONE;

   do {
      if ((partitions[i].GetStartLBA() <= sector) && (partitions[i].GetLastLBA() >= sector))
         usedAs = partitions[i].GetInclusion();
      if ((partitions[i].GetStartLBA() == (sector + 1)) && (partitions[i].GetInclusion() == LOGICAL))
         usedAs = EBR;
      if (sector == 0)
         usedAs = EBR;
      if (sector >= diskSize)
         usedAs = INVALID;
      i++;
   } while ((i < topPartNum) && ((usedAs == NONE) || (usedAs == EBR)));
   return usedAs;
} // BasicMBRData::SectorUsedAs()

/******************************************************
 *                                                    *
 * Functions that extract data on specific partitions *
 *                                                    *
 ******************************************************/

uint8_t BasicMBRData::GetStatus(int i) {
   MBRPart* thePart;
   uint8_t retval;

   thePart = GetPartition(i);
   if (thePart != NULL)
      retval = thePart->GetStatus();
   else
      retval = UINT8_C(0);
   return retval;
} // BasicMBRData::GetStatus()

uint8_t BasicMBRData::GetType(int i) {
   MBRPart* thePart;
   uint8_t retval;

   thePart = GetPartition(i);
   if (thePart != NULL)
      retval = thePart->GetType();
   else
      retval = UINT8_C(0);
   return retval;
} // BasicMBRData::GetType()

uint64_t BasicMBRData::GetFirstSector(int i) {
   MBRPart* thePart;
   uint64_t retval;

   thePart = GetPartition(i);
   if (thePart != NULL) {
      retval = thePart->GetStartLBA();
   } else
      retval = UINT32_C(0);
      return retval;
} // BasicMBRData::GetFirstSector()

uint64_t BasicMBRData::GetLength(int i) {
   MBRPart* thePart;
   uint64_t retval;

   thePart = GetPartition(i);
   if (thePart != NULL) {
      retval = thePart->GetLengthLBA();
   } else
      retval = UINT64_C(0);
      return retval;
} // BasicMBRData::GetLength()

/***********************
 *                     *
 * Protected functions *
 *                     *
 ***********************/

// Return a pointer to a primary or logical partition, or NULL if
// the partition is out of range....
MBRPart* BasicMBRData::GetPartition(int i) {
   MBRPart* thePart = NULL;

   if ((i >= 0) && (i < MAX_MBR_PARTS))
      thePart = &partitions[i];
   return thePart;
} // GetPartition()

/*******************************************
 *                                         *
 * Functions that involve user interaction *
 *                                         *
 *******************************************/

// Present the MBR operations menu. Note that the 'w' option does not
// immediately write data; that's handled by the calling function.
// Returns the number of partitions defined on exit, or -1 if the
// user selected the 'q' option. (Thus, the caller should save data
// if the return value is >0, or possibly >=0 depending on intentions.)
int BasicMBRData::DoMenu(const string& prompt) {
   int goOn = 1, quitting = 0, retval, num, haveShownInfo = 0;
   unsigned int hexCode;
   string tempStr;

   do {
      cout << prompt;
      switch (ReadString()[0]) {
         case '\0':
            goOn = cin.good();
            break;
         case 'a': case 'A':
            num = GetNumber(1, MAX_MBR_PARTS, 1, "Toggle active flag for partition: ") - 1;
            if (partitions[num].GetInclusion() != NONE)
               partitions[num].SetStatus(partitions[num].GetStatus() ^ 0x80);
            break;
         case 'c': case 'C':
            for (num = 0; num < MAX_MBR_PARTS; num++)
               RecomputeCHS(num);
            break;
         case 'l': case 'L':
            num = GetNumber(1, MAX_MBR_PARTS, 1, "Partition to set as logical: ") - 1;
            SetInclusionwChecks(num, LOGICAL);
            break;
         case 'o': case 'O':
            num = GetNumber(1, MAX_MBR_PARTS, 1, "Partition to omit: ") - 1;
            SetInclusionwChecks(num, NONE);
            break;
         case 'p': case 'P':
            if (!haveShownInfo) {
               cout << "\n** NOTE: Partition numbers do NOT indicate final primary/logical "
                    << "status,\n** unlike in most MBR partitioning tools!\n\a";
               cout << "\n** Extended partitions are not displayed, but will be generated "
                    << "as required.\n";
               haveShownInfo = 1;
            } // if
            DisplayMBRData();
            break;
         case 'q': case 'Q':
            cout << "This will abandon your changes. Are you sure? ";
            if (GetYN() == 'Y') {
               goOn = 0;
               quitting = 1;
            } // if
            break;
         case 'r': case 'R':
            num = GetNumber(1, MAX_MBR_PARTS, 1, "Partition to set as primary: ") - 1;
            SetInclusionwChecks(num, PRIMARY);
            break;
         case 's': case 'S':
            SortMBR();
            break;
         case 't': case 'T':
            num = GetNumber(1, MAX_MBR_PARTS, 1, "Partition to change type code: ") - 1;
            hexCode = 0x00;
            if (partitions[num].GetLengthLBA() > 0) {
               while ((hexCode <= 0) || (hexCode > 255)) {
                  cout << "Enter an MBR hex code: ";
                  tempStr = ReadString();
                  if (IsHex(tempStr))
                     sscanf(tempStr.c_str(), "%x", &hexCode);
               } // while
               partitions[num].SetType(hexCode);
            } // if
            break;
         case 'w': case 'W':
            goOn = 0;
            break;
         default:
            ShowCommands();
            break;
      } // switch
   } while (goOn);
   if (quitting)
      retval = -1;
   else
      retval = CountParts();
   return (retval);
} // BasicMBRData::DoMenu()

void BasicMBRData::ShowCommands(void) {
   cout << "a\ttoggle the active/boot flag\n";
   cout << "c\trecompute all CHS values\n";
   cout << "l\tset partition as logical\n";
   cout << "o\tomit partition\n";
   cout << "p\tprint the MBR partition table\n";
   cout << "q\tquit without saving changes\n";
   cout << "r\tset partition as primary\n";
   cout << "s\tsort MBR partitions\n";
   cout << "t\tchange partition type code\n";
   cout << "w\twrite the MBR partition table to disk and exit\n";
} // BasicMBRData::ShowCommands()
