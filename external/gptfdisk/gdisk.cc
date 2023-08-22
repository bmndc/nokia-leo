// gdisk.cc
// Program modelled after Linux fdisk, but it manipulates GPT partitions
// rather than MBR partitions.
//
// by Rod Smith, project began February 2009

/* This program is copyright (c) 2009-2013 by Roderick W. Smith. It is distributed
  under the terms of the GNU GPL version 2, as detailed in the COPYING file. */

#include <string.h>
#include <iostream>
#include "gpttext.h"

int main(int argc, char* argv[]) {
   GPTDataTextUI theGPT;
   string device = "";
   UnicodeString uString;
   int isError = 0;

#ifndef EFI
   cout << "GPT fdisk (gdisk) version " << GPTFDISK_VERSION << "\n\n";
#endif /*EFI*/

   if (!SizesOK())
      exit(1);

   switch (argc) {
      case 1:
         cout << "Type device filename, or press <Enter> to exit: ";
         device = ReadString();
         if (device.length() == 0)
            exit(0);
         else if (theGPT.LoadPartitions(device)) {
            if (theGPT.GetState() != use_gpt)
               WinWarning();
            theGPT.MainMenu(device);
         } // if/elseif
         break;
      case 2: // basic usage
         if (theGPT.LoadPartitions(argv[1])) {
            if (theGPT.GetState() != use_gpt)
               WinWarning();
            theGPT.MainMenu(argv[1]);
         } // if
         break;
      case 3: // usage with "-l" option
         if (strcmp(argv[1], "-l") == 0) {
            device = (string) argv[2];
         } else if (strcmp(argv[2], "-l") == 0) {
            device = (string) argv[1];
         } else { // 3 arguments, but none is "-l"
            cerr << "Usage: " << argv[0] << " [-l] device_file\n";
            isError = 1;
         } // if/elseif/else
         if (device != "") {
            theGPT.JustLooking();
            if (theGPT.LoadPartitions(device))
               theGPT.DisplayGPTData();
            else
               isError = 1;
         } // if
         break;
      default:
         cerr << "Usage: " << argv[0] << " [-l] device_file\n";
         isError = 1;
         break;
   } // switch
   return (isError);
} // main
