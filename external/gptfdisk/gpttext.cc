/*
    Copyright (C) 2010-2013  <Roderick W. Smith>

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

/* This class implements an interactive text-mode interface atop the
   GPTData class */

#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <limits.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>
#include "attributes.h"
#include "gpttext.h"
#include "support.h"

using namespace std;

/********************************************
 *                                          *
 * GPTDataText class and related structures *
 *                                          *
 ********************************************/

GPTDataTextUI::GPTDataTextUI(void) : GPTData() {
} // default constructor

GPTDataTextUI::GPTDataTextUI(string filename) : GPTData(filename) {
} // constructor passing filename

GPTDataTextUI::~GPTDataTextUI(void) {
} // default destructor

/*********************************************************************
 *                                                                   *
 * The following functions are extended (interactive) versions of    *
 * simpler functions in the base class....                           *
 *                                                                   *
 *********************************************************************/

// Overridden function; calls base-class function and then makes
// additional queries of the user, if the base-class function can't
// decide what to do.
WhichToUse GPTDataTextUI::UseWhichPartitions(void) {
   WhichToUse which;
   MBRValidity mbrState;
   int answer;

   which = GPTData::UseWhichPartitions();
   if ((which != use_abort) || beQuiet)
      return which;

   // If we get past here, it means that the non-interactive tests were
   // inconclusive, so we must ask the user which table to use....
   mbrState = protectiveMBR.GetValidity();

   if ((state == gpt_valid) && (mbrState == mbr)) {
      cout << "Found valid MBR and GPT. Which do you want to use?\n";
      answer = GetNumber(1, 3, 2, " 1 - MBR\n 2 - GPT\n 3 - Create blank GPT\n\nYour answer: ");
      if (answer == 1) {
         which = use_mbr;
      } else if (answer == 2) {
         which = use_gpt;
         cout << "Using GPT and creating fresh protective MBR.\n";
      } else which = use_new;
   } // if

   // Nasty decisions here -- GPT is present, but corrupt (bad CRCs or other
   // problems)
   if (state == gpt_corrupt) {
      if ((mbrState == mbr) || (mbrState == hybrid)) {
         cout << "Found valid MBR and corrupt GPT. Which do you want to use? (Using the\n"
              << "GPT MAY permit recovery of GPT data.)\n";
         answer = GetNumber(1, 3, 2, " 1 - MBR\n 2 - GPT\n 3 - Create blank GPT\n\nYour answer: ");
         if (answer == 1) {
            which = use_mbr;
         } else if (answer == 2) {
            which = use_gpt;
         } else which = use_new;
      } else if (mbrState == invalid) {
         cout << "Found invalid MBR and corrupt GPT. What do you want to do? (Using the\n"
              << "GPT MAY permit recovery of GPT data.)\n";
         answer = GetNumber(1, 2, 1, " 1 - Use current GPT\n 2 - Create blank GPT\n\nYour answer: ");
         if (answer == 1) {
            which = use_gpt;
         } else which = use_new;
      } // if/else/else
   } // if (corrupt GPT)

   return which;
} // UseWhichPartitions()

// Ask the user for a partition number; and prompt for verification
// if the requested partition isn't of a known BSD type.
// Lets the base-class function do the work, and returns its value (the
// number of converted partitions).
int GPTDataTextUI::XFormDisklabel(void) {
   uint32_t partNum;
   uint16_t hexCode;
   int goOn = 1, numDone = 0;
   BSDData disklabel;

   partNum = GetPartNum();

   // Now see if the specified partition has a BSD type code....
   hexCode = partitions[partNum].GetHexType();
   if ((hexCode != 0xa500) && (hexCode != 0xa900)) {
      cout << "Specified partition doesn't have a disklabel partition type "
           << "code.\nContinue anyway? ";
      goOn = (GetYN() == 'Y');
   } // if

   if (goOn)
      numDone = GPTData::XFormDisklabel(partNum);

   return numDone;
} // GPTData::XFormDisklabel(void)


/*********************************************************************
 *                                                                   *
 * Begin functions that obtain information from the users, and often *
 * do something with that information (call other functions)         *
 *                                                                   *
 *********************************************************************/

// Prompts user for partition number and returns the result. Returns "0"
// (the first partition) if none are currently defined.
uint32_t GPTDataTextUI::GetPartNum(void) {
   uint32_t partNum;
   uint32_t low, high;
   ostringstream prompt;

   if (GetPartRange(&low, &high) > 0) {
      prompt << "Partition number (" << low + 1 << "-" << high + 1 << "): ";
      partNum = GetNumber(low + 1, high + 1, low, prompt.str());
   } else partNum = 1;
   return (partNum - 1);
} // GPTDataTextUI::GetPartNum()

// What it says: Resize the partition table. (Default is 128 entries.)
void GPTDataTextUI::ResizePartitionTable(void) {
   int newSize;
   ostringstream prompt;
   uint32_t curLow, curHigh;

   cout << "Current partition table size is " << numParts << ".\n";
   GetPartRange(&curLow, &curHigh);
   curHigh++; // since GetPartRange() returns numbers starting from 0...
   // There's no point in having fewer than four partitions....
   if (curHigh < (blockSize / GPT_SIZE))
      curHigh = blockSize / GPT_SIZE;
   prompt << "Enter new size (" << curHigh << " up, default " << NUM_GPT_ENTRIES << "): ";
   newSize = GetNumber(4, 65535, 128, prompt.str());
   if (newSize < 128) {
      cout << "Caution: The partition table size should officially be 16KB or larger,\n"
           << "which works out to 128 entries. In practice, smaller tables seem to\n"
           << "work with most OSes, but this practice is risky. I'm proceeding with\n"
           << "the resize, but you may want to reconsider this action and undo it.\n\n";
   } // if
   SetGPTSize(newSize);
} // GPTDataTextUI::ResizePartitionTable()

// Interactively create a partition
void GPTDataTextUI::CreatePartition(void) {
   uint64_t firstBlock, firstInLargest, lastBlock, sector, origSector;
   uint32_t firstFreePart = 0;
   ostringstream prompt1, prompt2, prompt3;
   int partNum;

   // Find first free partition...
   while (partitions[firstFreePart].GetFirstLBA() != 0) {
      firstFreePart++;
   } // while

   if (((firstBlock = FindFirstAvailable()) != 0) &&
       (firstFreePart < numParts)) {
      lastBlock = FindLastAvailable();
      firstInLargest = FindFirstInLargest();
      Align(&firstInLargest);

      // Get partition number....
      prompt1 << "Partition number (" << firstFreePart + 1 << "-" << numParts
              << ", default " << firstFreePart + 1 << "): ";
      do {
         partNum = GetNumber(firstFreePart + 1, numParts,
                             firstFreePart + 1, prompt1.str()) - 1;
         if (partitions[partNum].GetFirstLBA() != 0)
            cout << "partition " << partNum + 1 << " is in use.\n";
      } while (partitions[partNum].GetFirstLBA() != 0);

      // Get first block for new partition...
      prompt2 << "First sector (" << firstBlock << "-" << lastBlock << ", default = "
              << firstInLargest << ") or {+-}size{KMGTP}: ";
      do {
         sector = GetSectorNum(firstBlock, lastBlock, firstInLargest, blockSize, prompt2.str());
      } while (IsFree(sector) == 0);
      origSector = sector;
      if (Align(&sector)) {
         cout << "Information: Moved requested sector from " << origSector << " to "
              << sector << " in\norder to align on " << sectorAlignment
              << "-sector boundaries.\n";
         if (!beQuiet)
            cout << "Use 'l' on the experts' menu to adjust alignment\n";
      } // if
      //      Align(&sector); // Align sector to correct multiple
      firstBlock = sector;

      // Get last block for new partitions...
      lastBlock = FindLastInFree(firstBlock);
      prompt3 << "Last sector (" << firstBlock << "-" << lastBlock << ", default = "
            << lastBlock << ") or {+-}size{KMGTP}: ";
      do {
         sector = GetSectorNum(firstBlock, lastBlock, lastBlock, blockSize, prompt3.str());
      } while (IsFree(sector) == 0);
      lastBlock = sector;

      firstFreePart = GPTData::CreatePartition(partNum, firstBlock, lastBlock);
      partitions[partNum].ChangeType();
      partitions[partNum].SetDefaultDescription();
   } else {
      if (firstFreePart >= numParts)
         cout << "No table partition entries left\n";
      else
         cout << "No free sectors available\n";
   } // if/else
} // GPTDataTextUI::CreatePartition()

// Interactively delete a partition (duh!)
void GPTDataTextUI::DeletePartition(void) {
   int partNum;
   uint32_t low, high;
   ostringstream prompt;

   if (GetPartRange(&low, &high) > 0) {
      prompt << "Partition number (" << low + 1 << "-" << high + 1 << "): ";
      partNum = GetNumber(low + 1, high + 1, low, prompt.str());
      GPTData::DeletePartition(partNum - 1);
   } else {
      cout << "No partitions\n";
   } // if/else
} // GPTDataTextUI::DeletePartition()

// Prompt user for a partition number, then change its type code
void GPTDataTextUI::ChangePartType(void) {
   int partNum;
   uint32_t low, high;

   if (GetPartRange(&low, &high) > 0) {
      partNum = GetPartNum();
      partitions[partNum].ChangeType();
   } else {
      cout << "No partitions\n";
   } // if/else
} // GPTDataTextUI::ChangePartType()

// Prompt user for a partition number, then change its unique
// GUID.
void GPTDataTextUI::ChangeUniqueGuid(void) {
   int partNum;
   uint32_t low, high;
   string guidStr;

   if (GetPartRange(&low, &high) > 0) {
      partNum = GetPartNum();
      cout << "Enter the partition's new unique GUID ('R' to randomize): ";
      guidStr = ReadString();
      if ((guidStr.length() >= 32) || (guidStr[0] == 'R') || (guidStr[0] == 'r')) {
         SetPartitionGUID(partNum, (GUIDData) guidStr);
         cout << "New GUID is " << partitions[partNum].GetUniqueGUID() << "\n";
      } else {
         cout << "GUID is too short!\n";
      } // if/else
   } else
      cout << "No partitions\n";
} // GPTDataTextUI::ChangeUniqueGuid()

// Partition attributes seem to be rarely used, but I want a way to
// adjust them for completeness....
void GPTDataTextUI::SetAttributes(uint32_t partNum) {
   partitions[partNum].SetAttributes();
} // GPTDataTextUI::SetAttributes()

// Prompts the user for a partition name and sets the partition's
// name. Returns 1 on success, 0 on failure (invalid partition
// number). (Note that the function skips prompting when an
// invalid partition number is detected.)
int GPTDataTextUI::SetName(uint32_t partNum) {
   UnicodeString theName = "";
   int retval = 1;

   if (IsUsedPartNum(partNum)) {
      cout << "Enter name: ";
#ifdef USE_UTF16
      theName = ReadUString();
#else
      theName = ReadString();
#endif
      partitions[partNum].SetName(theName);
   } else {
      cerr << "Invalid partition number (" << partNum << ")\n";
      retval = 0;
   } // if/else

   return retval;
} // GPTDataTextUI::SetName()

// Ask user for two partition numbers and swap them in the table. Note that
// this just reorders table entries; it doesn't adjust partition layout on
// the disk.
// Returns 1 if successful, 0 if not. (If user enters identical numbers, it
// counts as successful.)
int GPTDataTextUI::SwapPartitions(void) {
   int partNum1, partNum2, didIt = 0;
   uint32_t low, high;
   ostringstream prompt;
   GPTPart temp;

   if (GetPartRange(&low, &high) > 0) {
      partNum1 = GetPartNum();
      if (high >= numParts - 1)
         high = 0;
      prompt << "New partition number (1-" << numParts
             << ", default " << high + 2 << "): ";
      partNum2 = GetNumber(1, numParts, high + 2, prompt.str()) - 1;
      didIt = GPTData::SwapPartitions(partNum1, partNum2);
   } else {
      cout << "No partitions\n";
   } // if/else
   return didIt;
} // GPTDataTextUI::SwapPartitionNumbers()

// This function destroys the on-disk GPT structures. Returns 1 if the user
// confirms destruction, 0 if the user aborts or if there's a disk error.
int GPTDataTextUI::DestroyGPTwPrompt(void) {
   int allOK = 1;

   if ((apmFound) || (bsdFound)) {
      cout << "WARNING: APM or BSD disklabel structures detected! This operation could\n"
           << "damage any APM or BSD partitions on this disk!\n";
   } // if APM or BSD
   cout << "\a\aAbout to wipe out GPT on " << device << ". Proceed? ";
   if (GetYN() == 'Y') {
      if (DestroyGPT()) {
         // Note on below: Touch the MBR only if the user wants it completely
         // blanked out. Version 0.4.2 deleted the 0xEE partition and re-wrote
         // the MBR, but this could wipe out a valid MBR that the program
         // had subsequently discarded (say, if it conflicted with older GPT
         // structures).
         cout << "Blank out MBR? ";
         if (GetYN() == 'Y') {
            DestroyMBR();
         } else {
            cout << "MBR is unchanged. You may need to delete an EFI GPT (0xEE) partition\n"
                 << "with fdisk or another tool.\n";
         } // if/else
      } else allOK = 0; // if GPT structures destroyed
   } else allOK = 0; // if user confirms destruction
   return (allOK);
} // GPTDataTextUI::DestroyGPTwPrompt()

// Get partition number from user and then call ShowPartDetails(partNum)
// to show its detailed information
void GPTDataTextUI::ShowDetails(void) {
   int partNum;
   uint32_t low, high;

   if (GetPartRange(&low, &high) > 0) {
      partNum = GetPartNum();
      ShowPartDetails(partNum);
   } else {
      cout << "No partitions\n";
   } // if/else
} // GPTDataTextUI::ShowDetails()

// Create a hybrid MBR -- an ugly, funky thing that helps GPT work with
// OSes that don't understand GPT.
void GPTDataTextUI::MakeHybrid(void) {
   uint32_t partNums[3] = {0, 0, 0};
   string line;
   int numPartsToCvt = 0, numConverted = 0, i, j, mbrNum = 0;
   unsigned int hexCode = 0;
   MBRPart hybridPart;
   MBRData hybridMBR;
   char eeFirst = 'Y'; // Whether EFI GPT (0xEE) partition comes first in table

   cout << "\nWARNING! Hybrid MBRs are flaky and dangerous! If you decide not to use one,\n"
        << "just hit the Enter key at the below prompt and your MBR partition table will\n"
        << "be untouched.\n\n\a";

   // Use a local MBR structure, copying from protectiveMBR to keep its
   // boot loader code intact....
   hybridMBR = protectiveMBR;
   hybridMBR.EmptyMBR(0);

   // Now get the numbers of up to three partitions to add to the
   // hybrid MBR....
   cout << "Type from one to three GPT partition numbers, separated by spaces, to be\n"
        << "added to the hybrid MBR, in sequence: ";
   line = ReadString();
   istringstream inLine(line);
   do {
      inLine >> partNums[numPartsToCvt];
      if (partNums[numPartsToCvt] > 0)
         numPartsToCvt++;
   } while (!inLine.eof() && (numPartsToCvt < 3));

   if (numPartsToCvt > 0) {
      cout << "Place EFI GPT (0xEE) partition first in MBR (good for GRUB)? ";
      eeFirst = GetYN();
   } // if

   for (i = 0; i < numPartsToCvt; i++) {
      j = partNums[i] - 1;
      if (partitions[j].IsUsed() && (partitions[j].IsSizedForMBR() != MBR_SIZED_BAD)) {
         mbrNum = i + (eeFirst == 'Y');
         cout << "\nCreating entry for GPT partition #" << j + 1
              << " (MBR partition #" << mbrNum + 1 << ")\n";
         hybridPart.SetType(GetMBRTypeCode(partitions[j].GetHexType() / 256));
         hybridPart.SetLocation(partitions[j].GetFirstLBA(), partitions[j].GetLengthLBA());
         hybridPart.SetInclusion(PRIMARY);
         cout << "Set the bootable flag? ";
         if (GetYN() == 'Y')
            hybridPart.SetStatus(0x80);
         else
            hybridPart.SetStatus(0x00);
         hybridPart.SetInclusion(PRIMARY);
         if (partitions[j].IsSizedForMBR() == MBR_SIZED_IFFY)
            WarnAboutIffyMBRPart(j + 1);
         numConverted++;
      } else {
         cerr << "\nGPT partition #" << j + 1 << " does not exist or is too big; skipping.\n";
      } // if/else
      hybridMBR.AddPart(mbrNum, hybridPart);
   } // for

   if (numConverted > 0) { // User opted to create a hybrid MBR....
      // Create EFI protective partition that covers the start of the disk.
      // If this location (covering the main GPT data structures) is omitted,
      // Linux won't find any partitions on the disk.
      hybridPart.SetLocation(1, hybridMBR.FindLastInFree(1));
      hybridPart.SetStatus(0);
      hybridPart.SetType(0xEE);
      hybridPart.SetInclusion(PRIMARY);
      // newNote firstLBA and lastLBA are computed later...
      if (eeFirst == 'Y') {
         hybridMBR.AddPart(0, hybridPart);
      } else {
         hybridMBR.AddPart(numConverted, hybridPart);
      } // else
      hybridMBR.SetHybrid();

      // ... and for good measure, if there are any partition spaces left,
      // optionally create another protective EFI partition to cover as much
      // space as possible....
      if (hybridMBR.CountParts() < 4) { // unused entry....
         cout << "\nUnused partition space(s) found. Use one to protect more partitions? ";
         if (GetYN() == 'Y') {
            cout << "Note: Default is 0xEE, but this may confuse Mac OS X.\n";
            // Comment on above: Mac OS treats disks with more than one
            // 0xEE MBR partition as MBR disks, not as GPT disks.
            hexCode = GetMBRTypeCode(0xEE);
            hybridMBR.MakeBiggestPart(3, hexCode);
         } // if (GetYN() == 'Y')
      } // if unused entry
      protectiveMBR = hybridMBR;
   } else {
      cout << "\nNo partitions converted; original protective/hybrid MBR is unmodified!\n";
   } // if/else (numConverted > 0)
} // GPTDataTextUI::MakeHybrid()

// Convert the GPT to MBR form, storing partitions in the protectiveMBR
// variable. This function is necessarily limited; it may not be able to
// convert all partitions, depending on the disk size and available space
// before each partition (one free sector is required to create a logical
// partition, which are necessary to convert more than four partitions).
// Returns the number of converted partitions; if this value
// is over 0, the calling function should call DestroyGPT() to destroy
// the GPT data, call SaveMBR() to save the MBR, and then exit.
int GPTDataTextUI::XFormToMBR(void) {
   uint32_t i;

   protectiveMBR.EmptyMBR(0);
   for (i = 0; i < numParts; i++) {
      if (partitions[i].IsUsed()) {
         if (partitions[i].IsSizedForMBR() == MBR_SIZED_IFFY)
            WarnAboutIffyMBRPart(i + 1);
         // Note: MakePart() checks for oversized partitions, so don't
         // bother checking other IsSizedForMBR() return values....
         protectiveMBR.MakePart(i, partitions[i].GetFirstLBA(),
                                partitions[i].GetLengthLBA(),
                                partitions[i].GetHexType() / 0x0100, 0);
      } // if
   } // for
   protectiveMBR.MakeItLegal();
   return protectiveMBR.DoMenu();
} // GPTDataTextUI::XFormToMBR()

/******************************************************
 *                                                    *
 * Display informational messages for the user....    *
 *                                                    *
 ******************************************************/

// Although an MBR partition that begins below sector 2^32 and is less than 2^32 sectors in
// length is technically legal even if it ends above the 2^32-sector mark, such a partition
// tends to confuse a lot of OSes, so warn the user about such partitions. This function is
// called by XFormToMBR() and MakeHybrid(); it's a separate function just to consolidate the
// lengthy message in one place.
void GPTDataTextUI::WarnAboutIffyMBRPart(int partNum) {
   cout << "\a\nWarning! GPT partition " << partNum << " ends after the 2^32 sector mark! The partition\n"
        << "begins before this point, and is smaller than 2^32 sectors. This is technically\n"
        << "legal, but will confuse some OSes. The partition IS being added to the MBR, but\n"
        << "if your OS misbehaves or can't see the partition, the partition may simply be\n"
        << "unusable in that OS and may need to be resized or omitted from the MBR.\n\n";
} // GPTDataTextUI::WarnAboutIffyMBRPart()

/*********************************************************************
 *                                                                   *
 * The following functions provide the main menus for the gdisk      *
 * program....                                                       *
 *                                                                   *
 *********************************************************************/

// Accept a command and execute it. Returns only when the user
// wants to exit (such as after a 'w' or 'q' command).
void GPTDataTextUI::MainMenu(string filename) {
   int goOn = 1;
   PartType typeHelper;
   uint32_t temp1, temp2;

   do {
      cout << "\nCommand (? for help): ";
      switch (ReadString()[0]) {
         case '\0':
            goOn = cin.good();
            break;
         case 'b': case 'B':
            cout << "Enter backup filename to save: ";
            SaveGPTBackup(ReadString());
            break;
         case 'c': case 'C':
            if (GetPartRange(&temp1, &temp2) > 0)
               SetName(GetPartNum());
            else
               cout << "No partitions\n";
            break;
         case 'd': case 'D':
            DeletePartition();
            break;
         case 'i': case 'I':
            ShowDetails();
            break;
         case 'l': case 'L':
            typeHelper.ShowAllTypes();
            break;
         case 'n': case 'N':
            CreatePartition();
            break;
         case 'o': case 'O':
            cout << "This option deletes all partitions and creates a new protective MBR.\n"
                 << "Proceed? ";
            if (GetYN() == 'Y') {
               ClearGPTData();
               MakeProtectiveMBR();
            } // if
            break;
         case 'p': case 'P':
            DisplayGPTData();
            break;
         case 'q': case 'Q':
            goOn = 0;
            break;
         case 'r': case 'R':
            RecoveryMenu(filename);
            goOn = 0;
            break;
         case 's': case 'S':
            SortGPT();
            cout << "You may need to edit /etc/fstab and/or your boot loader configuration!\n";
            break;
         case 't': case 'T':
            ChangePartType();
            break;
         case 'v': case 'V':
            Verify();
            break;
         case 'w': case 'W':
            if (SaveGPTData() == 1)
               goOn = 0;
            break;
         case 'x': case 'X':
            ExpertsMenu(filename);
            goOn = 0;
            break;
         default:
            ShowCommands();
            break;
      } // switch
   } while (goOn);
} // GPTDataTextUI::MainMenu()

void GPTDataTextUI::ShowCommands(void) {
   cout << "b\tback up GPT data to a file\n";
   cout << "c\tchange a partition's name\n";
   cout << "d\tdelete a partition\n";
   cout << "i\tshow detailed information on a partition\n";
   cout << "l\tlist known partition types\n";
   cout << "n\tadd a new partition\n";
   cout << "o\tcreate a new empty GUID partition table (GPT)\n";
   cout << "p\tprint the partition table\n";
   cout << "q\tquit without saving changes\n";
   cout << "r\trecovery and transformation options (experts only)\n";
   cout << "s\tsort partitions\n";
   cout << "t\tchange a partition's type code\n";
   cout << "v\tverify disk\n";
   cout << "w\twrite table to disk and exit\n";
   cout << "x\textra functionality (experts only)\n";
   cout << "?\tprint this menu\n";
} // GPTDataTextUI::ShowCommands()

// Accept a recovery & transformation menu command. Returns only when the user
// issues an exit command, such as 'w' or 'q'.
void GPTDataTextUI::RecoveryMenu(string filename) {
   uint32_t numParts;
   int goOn = 1, temp1;
   
   do {
      cout << "\nRecovery/transformation command (? for help): ";
      switch (ReadString()[0]) {
         case '\0':
            goOn = cin.good();
            break;
         case 'b': case 'B':
            RebuildMainHeader();
            break;
         case 'c': case 'C':
            cout << "Warning! This will probably do weird things if you've converted an MBR to\n"
            << "GPT form and haven't yet saved the GPT! Proceed? ";
            if (GetYN() == 'Y')
               LoadSecondTableAsMain();
            break;
         case 'd': case 'D':
            RebuildSecondHeader();
            break;
         case 'e': case 'E':
            cout << "Warning! This will probably do weird things if you've converted an MBR to\n"
            << "GPT form and haven't yet saved the GPT! Proceed? ";
            if (GetYN() == 'Y')
               LoadMainTable();
            break;
         case 'f': case 'F':
            cout << "Warning! This will destroy the currently defined partitions! Proceed? ";
            if (GetYN() == 'Y') {
               if (LoadMBR(filename) == 1) { // successful load
                  XFormPartitions();
               } else {
                  cout << "Problem loading MBR! GPT is untouched; regenerating protective MBR!\n";
                  MakeProtectiveMBR();
               } // if/else
            } // if
            break;
         case 'g': case 'G':
            numParts = GetNumParts();
            temp1 = XFormToMBR();
            if (temp1 > 0)
               cout << "\nConverted " << temp1 << " partitions. Finalize and exit? ";
            if ((temp1 > 0) && (GetYN() == 'Y')) {
               if ((DestroyGPT() > 0) && (SaveMBR())) {
                  goOn = 0;
               } // if
            } else {
               MakeProtectiveMBR();
               SetGPTSize(numParts, 0);
               cout << "Note: New protective MBR created\n\n";
            } // if/else
            break;
         case 'h': case 'H':
            MakeHybrid();
            break;
         case 'i': case 'I':
            ShowDetails();
            break;
         case 'l': case 'L':
            cout << "Enter backup filename to load: ";
            LoadGPTBackup(ReadString());
            break;
         case 'm': case 'M':
            MainMenu(filename);
            goOn = 0;
            break;
         case 'o': case 'O':
            DisplayMBRData();
            break;
         case 'p': case 'P':
            DisplayGPTData();
            break;
         case 'q': case 'Q':
            goOn = 0;
            break;
         case 't': case 'T':
            XFormDisklabel();
            break;
         case 'v': case 'V':
            Verify();
            break;
         case 'w': case 'W':
            if (SaveGPTData() == 1) {
               goOn = 0;
            } // if
            break;
         case 'x': case 'X':
            ExpertsMenu(filename);
            goOn = 0;
            break;
         default:
            ShowRecoveryCommands();
            break;
      } // switch
   } while (goOn);
} // GPTDataTextUI::RecoveryMenu()

void GPTDataTextUI::ShowRecoveryCommands(void) {
   cout << "b\tuse backup GPT header (rebuilding main)\n";
   cout << "c\tload backup partition table from disk (rebuilding main)\n";
   cout << "d\tuse main GPT header (rebuilding backup)\n";
   cout << "e\tload main partition table from disk (rebuilding backup)\n";
   cout << "f\tload MBR and build fresh GPT from it\n";
   cout << "g\tconvert GPT into MBR and exit\n";
   cout << "h\tmake hybrid MBR\n";
   cout << "i\tshow detailed information on a partition\n";
   cout << "l\tload partition data from a backup file\n";
   cout << "m\treturn to main menu\n";
   cout << "o\tprint protective MBR data\n";
   cout << "p\tprint the partition table\n";
   cout << "q\tquit without saving changes\n";
   cout << "t\ttransform BSD disklabel partition\n";
   cout << "v\tverify disk\n";
   cout << "w\twrite table to disk and exit\n";
   cout << "x\textra functionality (experts only)\n";
   cout << "?\tprint this menu\n";
} // GPTDataTextUI::ShowRecoveryCommands()

// Accept an experts' menu command. Returns only after the user
// selects an exit command, such as 'w' or 'q'.
void GPTDataTextUI::ExpertsMenu(string filename) {
   GPTData secondDevice;
   uint32_t temp1, temp2;
   int goOn = 1;
   string guidStr, device;
   GUIDData aGUID;
   ostringstream prompt;
   
   do {
      cout << "\nExpert command (? for help): ";
      switch (ReadString()[0]) {
         case '\0':
            goOn = cin.good();
            break;
         case 'a': case 'A':
            if (GetPartRange(&temp1, &temp2) > 0)
               SetAttributes(GetPartNum());
            else
               cout << "No partitions\n";
            break;
         case 'c': case 'C':
            ChangeUniqueGuid();
            break;
         case 'd': case 'D':
            cout << "Partitions will begin on " << GetAlignment()
            << "-sector boundaries.\n";
            break;
         case 'e': case 'E':
            cout << "Relocating backup data structures to the end of the disk\n";
            MoveSecondHeaderToEnd();
            break;
         case 'f': case 'F':
            RandomizeGUIDs();
            break;
         case 'g': case 'G':
            cout << "Enter the disk's unique GUID ('R' to randomize): ";
            guidStr = ReadString();
            if ((guidStr.length() >= 32) || (guidStr[0] == 'R') || (guidStr[0] == 'r')) {
               SetDiskGUID((GUIDData) guidStr);
               cout << "The new disk GUID is " << GetDiskGUID() << "\n";
            } else {
               cout << "GUID is too short!\n";
            } // if/else
            break;
         case 'h': case 'H':
            RecomputeCHS();
            break;
         case 'i': case 'I':
            ShowDetails();
            break;
         case 'l': case 'L':
            prompt.seekp(0);
            prompt << "Enter the sector alignment value (1-" << MAX_ALIGNMENT << ", default = "
                   << DEFAULT_ALIGNMENT << "): ";
            temp1 = GetNumber(1, MAX_ALIGNMENT, DEFAULT_ALIGNMENT, prompt.str());
            SetAlignment(temp1);
            break;
         case 'm': case 'M':
            MainMenu(filename);
            goOn = 0;
            break;
         case 'n': case 'N':
            MakeProtectiveMBR();
            break;
         case 'o': case 'O':
            DisplayMBRData();
            break;
         case 'p': case 'P':
            DisplayGPTData();
            break;
         case 'q': case 'Q':
            goOn = 0;
            break;
         case 'r': case 'R':
            RecoveryMenu(filename);
            goOn = 0;
            break;
         case 's': case 'S':
            ResizePartitionTable();
            break;
         case 't': case 'T':
            SwapPartitions();
            break;
         case 'u': case 'U':
            cout << "Type device filename, or press <Enter> to exit: ";
            device = ReadString();
            if (device.length() > 0) {
               secondDevice = *this;
               secondDevice.SetDisk(device);
               secondDevice.SaveGPTData(0);
            } // if
            break;
         case 'v': case 'V':
            Verify();
            break;
         case 'w': case 'W':
            if (SaveGPTData() == 1) {
               goOn = 0;
            } // if
            break;
         case 'z': case 'Z':
            if (DestroyGPTwPrompt() == 1) {
               goOn = 0;
            }
            break;
         default:
            ShowExpertCommands();
            break;
      } // switch
   } while (goOn);
} // GPTDataTextUI::ExpertsMenu()

void GPTDataTextUI::ShowExpertCommands(void) {
   cout << "a\tset attributes\n";
   cout << "c\tchange partition GUID\n";
   cout << "d\tdisplay the sector alignment value\n";
   cout << "e\trelocate backup data structures to the end of the disk\n";
   cout << "g\tchange disk GUID\n";
   cout << "h\trecompute CHS values in protective/hybrid MBR\n";
   cout << "i\tshow detailed information on a partition\n";
   cout << "l\tset the sector alignment value\n";
   cout << "m\treturn to main menu\n";
   cout << "n\tcreate a new protective MBR\n";
   cout << "o\tprint protective MBR data\n";
   cout << "p\tprint the partition table\n";
   cout << "q\tquit without saving changes\n";
   cout << "r\trecovery and transformation options (experts only)\n";
   cout << "s\tresize partition table\n";
   cout << "t\ttranspose two partition table entries\n";
   cout << "u\treplicate partition table on new device\n";
   cout << "v\tverify disk\n";
   cout << "w\twrite table to disk and exit\n";
   cout << "z\tzap (destroy) GPT data structures and exit\n";
   cout << "?\tprint this menu\n";
} // GPTDataTextUI::ShowExpertCommands()



/********************************
 *                              *
 * Non-class support functions. *
 *                              *
 ********************************/

// GetMBRTypeCode() doesn't really belong in the class, since it's MBR-
// specific, but it's also user I/O-related, so I want to keep it in
// this file....

// Get an MBR type code from the user and return it
int GetMBRTypeCode(int defType) {
   string line;
   int typeCode;

   cout.setf(ios::uppercase);
   cout.fill('0');
   do {
      cout << "Enter an MBR hex code (default " << hex;
      cout.width(2);
      cout << defType << "): " << dec;
      line = ReadString();
      if (line[0] == '\0')
         typeCode = defType;
      else
         typeCode = StrToHex(line, 0);
   } while ((typeCode <= 0) || (typeCode > 255));
   cout.fill(' ');
   return typeCode;
} // GetMBRTypeCode

#ifdef USE_UTF16
// Note: ReadUString() is here rather than in support.cc so that the ICU
// libraries need not be linked to fixparts.

// Reads a Unicode string from stdin, returning it as an ICU-style string.
// Note that the returned string will NOT include the carriage return
// entered by the user. Relies on the ICU constructor from a string
// encoded in the current codepage to work.
UnicodeString ReadUString(void) {
   return ReadString().c_str();
} // ReadUString()
#endif
   
