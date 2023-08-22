// fixparts
// Program to fix certain types of damaged Master Boot Record (MBR) partition
// tables
//
// Copyright 2011 by Roderick W. Smith
//
// This program is distributed under the terms of the GNU GPL, as described
// in the COPYING file.
//
// Based on C++ classes originally created for GPT fdisk (gdisk and sgdisk)
// programs

#include <stdio.h>
#include <string.h>
#include <string>
#include <iostream>
#include <sstream>
#include "basicmbr.h"
#include "support.h"

using namespace std;

void DoMBR(BasicMBRData & mbrTable);

int main(int argc, char* argv[]) {
   BasicMBRData mbrTable;
   string device;

   cout << "FixParts " << GPTFDISK_VERSION << "\n";
   
   switch (argc) {
      case 1:
         cout << "Type device filename, or press <Enter> to exit: ";
         device = ReadString();
         if (device.length() == 0)
            exit(0);
         break;
      case 2:
         device = argv[1];
         break;
      default:
         cerr << "Usage: " << argv[0] << " device_filename\n";
         exit(1);
   } // switch

   cout << "\nLoading MBR data from " << device << "\n";
   if (!mbrTable.ReadMBRData(device)) {
      cerr << "\nUnable to read MBR data from '" << device << "'! Exiting!\n\n";
      exit(1);
   } // if

   // This switch() statement weeds out disks with GPT signatures and non-MBR
   // disks so we don't accidentally damage them....
   switch(mbrTable.GetValidity()) {
      case hybrid: case gpt:
         cerr << "\nThis disk appears to be a GPT disk. Use GNU Parted or GPT fdisk on it!\n";
         cerr << "Exiting!\n\n";
         exit(1);
         break;
      case invalid:
         cerr << "\nCannot find valid MBR data on '" << device << "'! Exiting!\n\n";
         exit(1);
         break;
      case mbr:
         DoMBR(mbrTable);
         break;
      default:
         cerr << "\nCannot determine the validity of the disk on '" << device
              << "'! Exiting!\n\n";
         exit(1);
         break;
   } // switch()
   return 0;
} // main()

// Do the bulk of the processing on actual MBR disks. First checks for old
// GPT data (note this is different from the earlier check; this one only
// looks for the GPT signatures in the main and backup GPT area, not for
// a protective partition in the MBR, which we know is NOT present, since
// if it were, this function would NOT be called!) and offers to destroy
// it, if found; then makes sure the partitions are in a consistent and
// legal state; then presents the MBR menu and, if it returns a "1" value
// (meaning the user opted to write changes), writes the table to disk.
void DoMBR(BasicMBRData & mbrTable) {
   int doItAgain;

   if (mbrTable.CheckForGPT() > 0) {
      cout << "\nNOTICE: GPT signatures detected on the disk, but no 0xEE protective "
           << "partition!\nThe GPT signatures are probably left over from a previous "
           << "partition table.\nDo you want to delete them (if you answer 'Y', this "
           << "will happen\nimmediately)? ";
      if (GetYN() == 'Y') {
         cout << "Erasing GPT data!\n";
         if (mbrTable.BlankGPTData() != 1)
            cerr << "GPT signature erasure failed!\n";
      } // if
   } // if

   mbrTable.MakeItLegal();
   do {
      doItAgain = 0;
      if (mbrTable.DoMenu() > 0) {
         cout << "\nFinal checks complete. About to write MBR data. THIS WILL OVERWRITE "
              << "EXISTING\nPARTITIONS!!\n\nDo you want to proceed? ";
         if (GetYN() == 'Y') {
            mbrTable.WriteMBRData();
            mbrTable.DiskSync();
            doItAgain = 0;
         } else {
            doItAgain = 1;
         } // else
      } // if
   } while (doItAgain);
} // DoMBR()
