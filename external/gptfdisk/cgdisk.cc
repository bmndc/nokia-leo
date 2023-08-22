/*
    Copyright (C) 2011  <Roderick W. Smith>

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

/* This class implements an interactive curses-based interface atop the
   GPTData class */

#include <string.h>
#include "gptcurses.h"

using namespace std;

#define MAX_OPTIONS 50

int main(int argc, char *argv[]) {
   string device = "";
   int displayType = USE_CURSES;

   if (!SizesOK())
      exit(1);

   switch (argc) {
      case 1:
         cout << "Type device filename, or press <Enter> to exit: ";
         device = ReadString();
         if (device.length() == 0)
            exit(0);
         break;
      case 2: // basic usage
         device = (string) argv[1];
         break;
      case 3: // "-a" usage or illegal
         if (strcmp(argv[1], "-a") == 0) {
            device = (string) argv[2];
         } else if (strcmp(argv[2], "-a") == 0) {
            device = (string) argv[1];
         } else {
            cerr << "Usage: " << argv[0] << " [-a] device_file\n";
            exit(1);
         } // if/elseif/else
         displayType = USE_ARROW;
         break;
      default:
         cerr << "Usage: " << argv[0] << " [-a] device_file\n";
         exit(1);
         break;
   } // switch

   GPTDataCurses theGPT;

   theGPT.SetDisplayType(displayType);
   if (theGPT.LoadPartitions(device)) {
      if (theGPT.GetState() != use_gpt) {
         Report("Warning! Non-GPT or damaged disk detected! This program will attempt to\n"
                "convert to GPT form or repair damage to GPT data structures, but may not\n"
                "succeed. Use gdisk or another disk repair tool if you have a damaged GPT\n"
                "disk.");
      } // if
      theGPT.MainMenu();
   } else {
      Report("Could not load partitions from '" + device + "'! Aborting!");
   } // if/else
   return 0;
} // main
