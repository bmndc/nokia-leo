/* bsd.cc -- Functions for loading and manipulating legacy BSD disklabel
   data. */

/* By Rod Smith, initial coding August, 2009 */

/* This program is copyright (c) 2009 by Roderick W. Smith. It is distributed
  under the terms of the GNU GPL version 2, as detailed in the COPYING file. */

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include <stdio.h>
//#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <iostream>
#include <string>
#include "support.h"
#include "bsd.h"

using namespace std;


BSDData::BSDData(void) {
   state = unknown;
   signature = UINT32_C(0);
   signature2 = UINT32_C(0);
   sectorSize = 512;
   numParts = 0;
   labelFirstLBA = 0;
   labelLastLBA = 0;
   labelStart = LABEL_OFFSET1; // assume raw disk format
   partitions = NULL;
} // default constructor

BSDData::~BSDData(void) {
   delete[] partitions;
} // destructor

// Read BSD disklabel data from the specified device filename. This function
// just opens the device file and then calls an overloaded function to do
// the bulk of the work. Returns 1 on success, 0 on failure.
int BSDData::ReadBSDData(const string & device, uint64_t startSector, uint64_t endSector) {
   int allOK = 1;
   DiskIO myDisk;

   if (device != "") {
      if (myDisk.OpenForRead(device)) {
         allOK = ReadBSDData(&myDisk, startSector, endSector);
      } else {
         allOK = 0;
      } // if/else

      myDisk.Close();
   } else {
      allOK = 0;
   } // if/else
   return allOK;
} // BSDData::ReadBSDData() (device filename version)

// Load the BSD disklabel data from an already-opened disk
// file, starting with the specified sector number.
int BSDData::ReadBSDData(DiskIO *theDisk, uint64_t startSector, uint64_t endSector) {
   int allOK = 1;
   int i, foundSig = 0, bigEnd = 0;
   int relative = 0; // assume absolute partition sector numbering
   uint8_t buffer[4096]; // I/O buffer
   uint32_t realSig;
   uint32_t* temp32;
   uint16_t* temp16;
   BSDRecord* tempRecords;
   int offset[NUM_OFFSETS] = { LABEL_OFFSET1, LABEL_OFFSET2 };

   labelFirstLBA = startSector;
   labelLastLBA = endSector;
   offset[1] = theDisk->GetBlockSize();

   // Read 4096 bytes (eight 512-byte sectors or equivalent)
   // into memory; we'll extract data from this buffer.
   // (Done to work around FreeBSD limitation on size of reads
   // from block devices.)
   allOK = theDisk->Seek(startSector);
   if (allOK) allOK = theDisk->Read(buffer, 4096);

   // Do some strangeness to support big-endian architectures...
   bigEnd = (IsLittleEndian() == 0);
   realSig = BSD_SIGNATURE;
   if (bigEnd && allOK)
      ReverseBytes(&realSig, 4);

   // Look for the signature at any of two locations.
   // Note that the signature is repeated at both the original
   // offset and 132 bytes later, so we need two checks....
   if (allOK) {
      i = 0;
      do {
         temp32 = (uint32_t*) &buffer[offset[i]];
         signature = *temp32;
         if (signature == realSig) { // found first, look for second
            temp32 = (uint32_t*) &buffer[offset[i] + 132];
            signature2 = *temp32;
            if (signature2 == realSig) {
               foundSig = 1;
               labelStart = offset[i];
            } // if found signature
         } // if/else
         i++;
      } while ((!foundSig) && (i < NUM_OFFSETS));
      allOK = foundSig;
   } // if

   // Load partition metadata from the buffer....
   if (allOK) {
      temp32 = (uint32_t*) &buffer[labelStart + 40];
      sectorSize = *temp32;
      temp16 = (uint16_t*) &buffer[labelStart + 138];
      numParts = *temp16;
   } // if

   // Make it big-endian-aware....
   if ((IsLittleEndian() == 0) && allOK)
      ReverseMetaBytes();

   // Check validity of the data and flag it appropriately....
   if (foundSig && (numParts <= MAX_BSD_PARTS) && allOK) {
      state = bsd;
   } else {
      state = bsd_invalid;
   } // if/else

   // If the state is good, go ahead and load the main partition data....
   if (state == bsd) {
      partitions = new struct BSDRecord[numParts * sizeof(struct BSDRecord)];
      if (partitions == NULL) {
         cerr << "Unable to allocate memory in BSDData::ReadBSDData()! Terminating!\n";
         exit(1);
      } // if
      for (i = 0; i < numParts; i++) {
         // Once again, we use the buffer, but index it using a BSDRecord
         // pointer (dangerous, but effective)....
         tempRecords = (BSDRecord*) &buffer[labelStart + 148];
         partitions[i].lengthLBA = tempRecords[i].lengthLBA;
         partitions[i].firstLBA = tempRecords[i].firstLBA;
         partitions[i].fsType = tempRecords[i].fsType;
         if (bigEnd) { // reverse data (fsType is a single byte)
            ReverseBytes(&partitions[i].lengthLBA, 4);
            ReverseBytes(&partitions[i].firstLBA, 4);
         } // if big-endian
         // Check for signs of relative sector numbering: A "0" first sector
         // number on a partition with a non-zero length -- but ONLY if the
         // length is less than the disk size, since NetBSD has a habit of
         // creating a disk-sized partition within a carrier MBR partition
         // that's too small to house it, and this throws off everything....
         if ((partitions[i].firstLBA == 0) && (partitions[i].lengthLBA > 0)
             && (partitions[i].lengthLBA < labelLastLBA))
            relative = 1;
      } // for
      // Some disklabels use sector numbers relative to the enclosing partition's
      // start, others use absolute sector numbers. If relative numbering was
      // detected above, apply a correction to all partition start sectors....
      if (relative) {
         for (i = 0; i < numParts; i++) {
            partitions[i].firstLBA += (uint32_t) startSector;
         } // for
      } // if
   } // if signatures OK
//   DisplayBSDData();
   return allOK;
} // BSDData::ReadBSDData(DiskIO* theDisk, uint64_t startSector)

// Reverse metadata's byte order; called only on big-endian systems
void BSDData::ReverseMetaBytes(void) {
   ReverseBytes(&signature, 4);
   ReverseBytes(&sectorSize, 4);
   ReverseBytes(&signature2, 4);
   ReverseBytes(&numParts, 2);
} // BSDData::ReverseMetaByteOrder()

// Display basic BSD partition data. Used for debugging.
void BSDData::DisplayBSDData(void) {
   int i;

   if (state == bsd) {
      cout << "BSD partitions:\n";
      for (i = 0; i < numParts; i++) {
         cout.width(4);
         cout << i + 1 << "\t";
         cout.width(13);
         cout << partitions[i].firstLBA << "\t";
         cout.width(15);
         cout << partitions[i].lengthLBA << " \t0x";
         cout.width(2);
         cout.fill('0');
         cout.setf(ios::uppercase);
         cout << hex << (int) partitions[i].fsType << "\n" << dec;
         cout.fill(' ');
      } // for
   } // if
} // BSDData::DisplayBSDData()

// Displays the BSD disklabel state. Called during program launch to inform
// the user about the partition table(s) status
int BSDData::ShowState(void) {
   int retval = 0;

   switch (state) {
      case bsd_invalid:
         cout << "  BSD: not present\n";
         break;
      case bsd:
         cout << "  BSD: present\n";
         retval = 1;
         break;
      default:
         cout << "\a  BSD: unknown -- bug!\n";
         break;
   } // switch
   return retval;
} // BSDData::ShowState()

// Weirdly, this function has stopped working when defined inline,
// but it's OK here....
int BSDData::IsDisklabel(void) {
   return (state == bsd);
} // BSDData::IsDiskLabel()

// Returns the BSD table's partition type code
uint8_t BSDData::GetType(int i) {
   uint8_t retval = 0; // 0 = "unused"

   if ((i < numParts) && (i >= 0) && (state == bsd) && (partitions != 0))
      retval = partitions[i].fsType;

   return(retval);
} // BSDData::GetType()

// Returns the number of the first sector of the specified partition
uint64_t BSDData::GetFirstSector(int i) {
   uint64_t retval = UINT64_C(0);

   if ((i < numParts) && (i >= 0) && (state == bsd) && (partitions != 0))
      retval = (uint64_t) partitions[i].firstLBA;

   return retval;
} // BSDData::GetFirstSector

// Returns the length (in sectors) of the specified partition
uint64_t BSDData::GetLength(int i) {
   uint64_t retval = UINT64_C(0);

   if ((i < numParts) && (i >= 0) && (state == bsd) && (partitions != 0))
      retval = (uint64_t) partitions[i].lengthLBA;

   return retval;
} // BSDData::GetLength()

// Returns the number of partitions defined in the current table
int BSDData::GetNumParts(void) {
   return numParts;
} // BSDData::GetNumParts()

// Returns the specified partition as a GPT partition. Used in BSD-to-GPT
// conversion process
GPTPart BSDData::AsGPT(int i) {
   GPTPart guid;                  // dump data in here, then return it
   uint64_t sectorOne, sectorEnd; // first & last sectors of partition
   int passItOn = 1;              // Set to 0 if partition is empty or invalid

   guid.BlankPartition();
   sectorOne = (uint64_t) partitions[i].firstLBA;
   sectorEnd = sectorOne + (uint64_t) partitions[i].lengthLBA;
   if (sectorEnd > 0) sectorEnd--;
   // Note on above: BSD partitions sometimes have a length of 0 and a start
   // sector of 0. With unsigned ints, the usual way (start + length - 1) to
   // find the end will result in a huge number, which will be confusing.
   // Thus, apply the "-1" part only if it's reasonable to do so.

   // Do a few sanity checks on the partition before we pass it on....
   // First, check that it falls within the bounds of its container
   // and that it starts before it ends....
   if ((sectorOne < labelFirstLBA) || (sectorEnd > labelLastLBA) || (sectorOne > sectorEnd))
      passItOn = 0;
   // Some disklabels include a pseudo-partition that's the size of the entire
   // disk or containing partition. Don't return it.
   if ((sectorOne <= labelFirstLBA) && (sectorEnd >= labelLastLBA) &&
       (GetType(i) == 0))
      passItOn = 0;
   // If the end point is 0, it's not a valid partition.
   if ((sectorEnd == 0) || (sectorEnd == labelFirstLBA))
      passItOn = 0;

   if (passItOn) {
      guid.SetFirstLBA(sectorOne);
      guid.SetLastLBA(sectorEnd);
      // Now set a random unique GUID for the partition....
      guid.RandomizeUniqueGUID();
      // ... zero out the attributes and name fields....
      guid.SetAttributes(UINT64_C(0));
      // Most BSD disklabel type codes seem to be archaic or rare.
      // They're also ambiguous; a FreeBSD filesystem is impossible
      // to distinguish from a NetBSD one. Thus, these code assignment
      // are going to be rough to begin with. For a list of meanings,
      // see http://fxr.watson.org/fxr/source/sys/dtype.h?v=DFBSD,
      // or Google it.
      switch (GetType(i)) {
         case 1: // BSD swap
            guid.SetType(0xa502); break;
         case 7: // BSD FFS
            guid.SetType(0xa503); break;
         case 8: case 11: // MS-DOS or HPFS
            guid.SetType(0x0700); break;
         case 9: // log-structured fs
            guid.SetType(0xa903); break;
         case 13: // bootstrap
            guid.SetType(0xa501); break;
         case 14: // vinum
            guid.SetType(0xa505); break;
         case 15: // RAID
            guid.SetType(0xa903); break;
         case 27: // FreeBSD ZFS
            guid.SetType(0xa504); break;
         default:
            guid.SetType(0xa503); break;
      } // switch
      // Set the partition name to the name of the type code....
      guid.SetName(guid.GetTypeName());
   } // if
   return guid;
} // BSDData::AsGPT()
