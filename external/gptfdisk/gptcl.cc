/*
    Implementation of GPTData class derivative with popt-based command
    line processing
    Copyright (C) 2010-2014 Roderick W. Smith

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

#include <string.h>
#include <string>
#include <iostream>
#include <sstream>
#include <errno.h>
#include "gptcl.h"

GPTDataCL::GPTDataCL(void) {
   attributeOperation = backupFile = partName = hybrids = newPartInfo = NULL;
   mbrParts = twoParts = outDevice = typeCode = partGUID = diskGUID = NULL;
   alignment = DEFAULT_ALIGNMENT;
   deletePartNum = infoPartNum = largestPartNum = bsdPartNum = 0;
   tableSize = GPT_SIZE;
} // GPTDataCL constructor

GPTDataCL::GPTDataCL(string filename) {
} // GPTDataCL constructor with filename

GPTDataCL::~GPTDataCL(void) {
} // GPTDataCL destructor

void GPTDataCL::LoadBackupFile(string backupFile, int &saveData, int &neverSaveData) {
   if (LoadGPTBackup(backupFile) == 1) {
      JustLooking(0);
      saveData = 1;
   } else {
      saveData = 0;
      neverSaveData = 1;
      cerr << "Error loading backup file!\n";
   } // else
} // GPTDataCL::LoadBackupFile()

// Perform the actions specified on the command line. This is necessarily one
// monster of a function!
// Returns values:
// 0 = success
// 1 = too few arguments
// 2 = error when reading partition table
// 3 = non-GPT disk and no -g option
// 4 = unable to save changes
// 8 = disk replication operation (-R) failed
int GPTDataCL::DoOptions(int argc, char* argv[]) {
   GPTData secondDevice;
   int opt, numOptions = 0, saveData = 0, neverSaveData = 0;
   int partNum = 0, newPartNum = -1, saveNonGPT = 1, retval = 0, pretend = 0;
   uint64_t low, high, startSector, endSector, sSize;
   uint64_t temp; // temporary variable; free to use in any case
   char *device;
   string cmd, typeGUID, name;
   PartType typeHelper;

   struct poptOption theOptions[] =
   {
      {"attributes", 'A', POPT_ARG_STRING, &attributeOperation, 'A', "operate on partition attributes", "list|[partnum:show|or|nand|xor|=|set|clear|toggle|get[:bitnum|hexbitmask]]"},
      {"set-alignment", 'a', POPT_ARG_INT, &alignment, 'a', "set sector alignment", "value"},
      {"backup", 'b', POPT_ARG_STRING, &backupFile, 'b', "backup GPT to file", "file"},
      {"change-name", 'c', POPT_ARG_STRING, &partName, 'c', "change partition's name", "partnum:name"},
      {"recompute-chs", 'C', POPT_ARG_NONE, NULL, 'C', "recompute CHS values in protective/hybrid MBR", ""},
      {"delete", 'd', POPT_ARG_INT, &deletePartNum, 'd', "delete a partition", "partnum"},
      {"display-alignment", 'D', POPT_ARG_NONE, NULL, 'D', "show number of sectors per allocation block", ""},
      {"move-second-header", 'e', POPT_ARG_NONE, NULL, 'e', "move second header to end of disk", ""},
      {"end-of-largest", 'E', POPT_ARG_NONE, NULL, 'E', "show end of largest free block", ""},
      {"first-in-largest", 'f', POPT_ARG_NONE, NULL, 'f', "show start of the largest free block", ""},
      {"first-aligned-in-largest", 'F', POPT_ARG_NONE, NULL, 'F', "show start of the largest free block, aligned", ""},
      {"mbrtogpt", 'g', POPT_ARG_NONE, NULL, 'g', "convert MBR to GPT", ""},
      {"randomize-guids", 'G', POPT_ARG_NONE, NULL, 'G', "randomize disk and partition GUIDs", ""},
      {"hybrid", 'h', POPT_ARG_STRING, &hybrids, 'h', "create hybrid MBR", "partnum[:partnum...]"},
      {"info", 'i', POPT_ARG_INT, &infoPartNum, 'i', "show detailed information on partition", "partnum"},
      {"load-backup", 'l', POPT_ARG_STRING, &backupFile, 'l', "load GPT backup from file", "file"},
      {"list-types", 'L', POPT_ARG_NONE, NULL, 'L', "list known partition types", ""},
      {"gpttombr", 'm', POPT_ARG_STRING, &mbrParts, 'm', "convert GPT to MBR", "partnum[:partnum...]"},
      {"new", 'n', POPT_ARG_STRING, &newPartInfo, 'n', "create new partition", "partnum:start:end"},
      {"largest-new", 'N', POPT_ARG_INT, &largestPartNum, 'N', "create largest possible new partition", "partnum"},
      {"clear", 'o', POPT_ARG_NONE, NULL, 'o', "clear partition table", ""},
      {"print", 'p', POPT_ARG_NONE, NULL, 'p', "print partition table", ""},
      {"pretend", 'P', POPT_ARG_NONE, NULL, 'P', "make changes in memory, but don't write them", ""},
      {"transpose", 'r', POPT_ARG_STRING, &twoParts, 'r', "transpose two partitions", "partnum:partnum"},
      {"replicate", 'R', POPT_ARG_STRING, &outDevice, 'R', "replicate partition table", "device_filename"},
      {"sort", 's', POPT_ARG_NONE, NULL, 's', "sort partition table entries", ""},
      {"resize-table", 'S', POPT_ARG_INT, &tableSize, 'S', "resize partition table", "numparts"},
      {"typecode", 't', POPT_ARG_STRING, &typeCode, 't', "change partition type code", "partnum:{hexcode|GUID}"},
      {"transform-bsd", 'T', POPT_ARG_INT, &bsdPartNum, 'T', "transform BSD disklabel partition to GPT", "partnum"},
      {"partition-guid", 'u', POPT_ARG_STRING, &partGUID, 'u', "set partition GUID", "partnum:guid"},
      {"disk-guid", 'U', POPT_ARG_STRING, &diskGUID, 'U', "set disk GUID", "guid"},
      {"verify", 'v', POPT_ARG_NONE, NULL, 'v', "check partition table integrity", ""},
      {"version", 'V', POPT_ARG_NONE, NULL, 'V', "display version information", ""},
      {"zap", 'z', POPT_ARG_NONE, NULL, 'z', "zap (destroy) GPT (but not MBR) data structures", ""},
      {"zap-all", 'Z', POPT_ARG_NONE, NULL, 'Z', "zap (destroy) GPT and MBR data structures", ""},
      POPT_AUTOHELP { NULL, 0, 0, NULL, 0 }
   };

   // Create popt context...
   poptCon = poptGetContext(NULL, argc, (const char**) argv, theOptions, 0);

   poptSetOtherOptionHelp(poptCon, " [OPTION...] <device>");

   if (argc < 2) {
      poptPrintUsage(poptCon, stderr, 0);
      return 1;
   }

   // Do one loop through the options to find the device filename and deal
   // with options that don't require a device filename, to flag destructive
   // (o, z, or Z) options, and to flag presence of a --pretend/-P option
   while ((opt = poptGetNextOpt(poptCon)) > 0) {
      switch (opt) {
         case 'A':
            cmd = GetString(attributeOperation, 1);
            if (cmd == "list")
               Attributes::ListAttributes();
            break;
         case 'L':
            typeHelper.ShowAllTypes(0);
            break;
         case 'P':
            pretend = 1;
            break;
         case 'V':
            cout << "GPT fdisk (sgdisk) version " << GPTFDISK_VERSION << "\n\n";
            break;
         default:
            break;
      } // switch
      numOptions++;
   } // while

   // Assume first non-option argument is the device filename....
   device = (char*) poptGetArg(poptCon);
   poptResetContext(poptCon);

   if (device != NULL) {
      JustLooking(); // reset as necessary
      BeQuiet(); // Tell called functions to be less verbose & interactive
      if (LoadPartitions((string) device)) {
         if ((WhichWasUsed() == use_mbr) || (WhichWasUsed() == use_bsd))
            saveNonGPT = 0; // flag so we don't overwrite unless directed to do so
            sSize = GetBlockSize();
         while ((opt = poptGetNextOpt(poptCon)) > 0) {
            switch (opt) {
               case 'A': {
                  if (cmd != "list") {
                     partNum = (int) GetInt(attributeOperation, 1) - 1;
                     if (partNum < 0)
                        partNum = newPartNum;
                     if ((partNum >= 0) && (partNum < (int) GetNumParts())) {
                        switch (ManageAttributes(partNum, GetString(attributeOperation, 2),
                           GetString(attributeOperation, 3))) {
                           case -1:
                              saveData = 0;
                              neverSaveData = 1;
                              break;
                           case 1:
                              JustLooking(0);
                              saveData = 1;
                              break;
                           default:
                              break;
                        } // switch
                     } else {
                        cerr << "Error: Invalid partition number " << partNum + 1 << "\n";
                        saveData = 0;
                        neverSaveData = 1;
                     } // if/else reasonable partition #
                  } // if (cmd != "list")
                  break;
               } // case 'A':
               case 'a':
                  SetAlignment(alignment);
                  break;
               case 'b':
                  SaveGPTBackup(backupFile);
                  free(backupFile);
                  break;
               case 'c':
                  cout << "Setting name!\n";
                  JustLooking(0);
                  partNum = (int) GetInt(partName, 1) - 1;
                  if (partNum < 0)
                     partNum = newPartNum;
                  cout << "partNum is " << partNum << "\n";
                  if ((partNum >= 0) && (partNum < (int) GetNumParts())) {
                     cout << "REALLY setting name!\n";
                     name = GetString(partName, 2);
                     if (SetName(partNum, (UnicodeString) name.c_str())) {
                        saveData = 1;
                     } else {
                        cerr << "Unable to set partition " << partNum + 1
                             << "'s name to '" << GetString(partName, 2) << "'!\n";
                        neverSaveData = 1;
                     } // if/else
                     free(partName);
                  }
                  break;
               case 'C':
                  JustLooking(0);
                  RecomputeCHS();
                  saveData = 1;
                  break;
               case 'd':
                  JustLooking(0);
                  if (DeletePartition(deletePartNum - 1) == 0) {
                     cerr << "Error " << errno << " deleting partition!\n";
                     neverSaveData = 1;
                  } else saveData = 1;
                                                      break;
               case 'D':
                  cout << GetAlignment() << "\n";
                  break;
               case 'e':
                  JustLooking(0);
                  MoveSecondHeaderToEnd();
                  saveData = 1;
                  break;
               case 'E':
                  cout << FindLastInFree(FindFirstInLargest()) << "\n";
                  break;
               case 'f':
                  cout << FindFirstInLargest() << "\n";
                  break;
               case 'F':
                  temp = FindFirstInLargest();
                  Align(&temp);
                  cout << temp << "\n";
                  break;
               case 'g':
                  JustLooking(0);
                  saveData = 1;
                  saveNonGPT = 1;
                  break;
               case 'G':
                  JustLooking(0);
                  saveData = 1;
                  RandomizeGUIDs();
                  break;
               case 'h':
                  JustLooking(0);
                  if (BuildMBR(hybrids, 1) == 1)
                     saveData = 1;
                  break;
               case 'i':
                  ShowPartDetails(infoPartNum - 1);
                  break;
               case 'l':
                  LoadBackupFile(backupFile, saveData, neverSaveData);
                  free(backupFile);
                  break;
               case 'L':
                  break;
               case 'm':
                  JustLooking(0);
                  if (BuildMBR(mbrParts, 0) == 1) {
                     if (!pretend) {
                        if (SaveMBR()) {
                           DestroyGPT();
                        } else
                           cerr << "Problem saving MBR!\n";
                     } // if
                     saveNonGPT = 0;
                     pretend = 1; // Not really, but works around problem if -g is used with this...
                     saveData = 0;
                  } // if
                  break;
               case 'n':
                  JustLooking(0);
                  newPartNum = (int) GetInt(newPartInfo, 1) - 1;
                  if (newPartNum < 0)
                     newPartNum = FindFirstFreePart();
                  low = FindFirstInLargest();
                  Align(&low);
                  high = FindLastInFree(low);
                  startSector = IeeeToInt(GetString(newPartInfo, 2), sSize, low, high, low);
                  endSector = IeeeToInt(GetString(newPartInfo, 3), sSize, startSector, high, high);
                  if (CreatePartition(newPartNum, startSector, endSector)) {
                     saveData = 1;
                  } else {
                     cerr << "Could not create partition " << newPartNum + 1 << " from "
                          << startSector << " to " << endSector << "\n";
                     neverSaveData = 1;
                  } // if/else
                  free(newPartInfo);
                  break;
               case 'N':
                  JustLooking(0);
                  startSector = FindFirstInLargest();
                  Align(&startSector);
                  endSector = FindLastInFree(startSector);
                  if (largestPartNum < 0)
                     largestPartNum = FindFirstFreePart();
                  if (CreatePartition(largestPartNum - 1, startSector, endSector)) {
                     saveData = 1;
                  } else {
                     cerr << "Could not create partition " << largestPartNum << " from "
                     << startSector << " to " << endSector << "\n";
                     neverSaveData = 1;
                  } // if/else
                  break;
               case 'o':
                  JustLooking(0);
                  ClearGPTData();
                  saveData = 1;
                  break;
               case 'p':
                  DisplayGPTData();
                  break;
               case 'P':
                  pretend = 1;
                  break;
               case 'r':
                  JustLooking(0);
                  uint64_t p1, p2;
                  p1 = GetInt(twoParts, 1) - 1;
                  p2 = GetInt(twoParts, 2) - 1;
                  if (SwapPartitions((uint32_t) p1, (uint32_t) p2) == 0) {
                     neverSaveData = 1;
                     cerr << "Cannot swap partitions " << p1 + 1 << " and " << p2 + 1 << "\n";
                  } else saveData = 1;
                                                      break;
               case 'R':
                  secondDevice = *this;
                  secondDevice.SetDisk(outDevice);
                  secondDevice.JustLooking(0);
                  if (!secondDevice.SaveGPTData(1))
                     retval = 8;
                  break;
               case 's':
                  JustLooking(0);
                  SortGPT();
                  saveData = 1;
                  break;
               case 'S':
                  JustLooking(0);
                  if (SetGPTSize(tableSize) == 0)
                     neverSaveData = 1;
                  else
                     saveData = 1;
                  break;
               case 't':
                  JustLooking(0);
                  partNum = (int) GetInt(typeCode, 1) - 1;
                  if (partNum < 0)
                     partNum = newPartNum;
                  if ((partNum >= 0) && (partNum < (int) GetNumParts())) {
                     typeHelper = GetString(typeCode, 2);
                     if ((typeHelper != (GUIDData) "00000000-0000-0000-0000-000000000000") &&
                         (ChangePartType(partNum, typeHelper))) {
                        saveData = 1;
                        } else {
                           cerr << "Could not change partition " << partNum + 1
                           << "'s type code to " << GetString(typeCode, 2) << "!\n";
                           neverSaveData = 1;
                        } // if/else
                     free(typeCode);
                  }
                  break;
               case 'T':
                  JustLooking(0);
                  XFormDisklabel(bsdPartNum - 1);
                  saveData = 1;
                  break;
               case 'u':
                  JustLooking(0);
                  saveData = 1;
                  partNum = (int) GetInt(partGUID, 1) - 1;
                  if (partNum < 0)
                     partNum = newPartNum;
                  if ((partNum >= 0) && (partNum < (int) GetNumParts())) {
                     SetPartitionGUID(partNum, GetString(partGUID, 2).c_str());
                  }
                  break;
               case 'U':
                  JustLooking(0);
                  saveData = 1;
                  SetDiskGUID(diskGUID);
                  break;
               case 'v':
                  Verify();
                  break;
               case 'z':
                  if (!pretend) {
                     DestroyGPT();
                  } // if
                  saveNonGPT = 0;
                  saveData = 0;
                  break;
               case 'Z':
                  if (!pretend) {
                     DestroyGPT();
                     DestroyMBR();
                  } // if
                  saveNonGPT = 0;
                  saveData = 0;
                  break;
               default:
                  cerr << "Unknown option (-" << opt << ")!\n";
                  break;
               } // switch
         } // while
      } else { // if loaded OK
         poptResetContext(poptCon);
         // Do a few types of operations even if there are problems....
         while ((opt = poptGetNextOpt(poptCon)) > 0) {
            switch (opt) {
               case 'l':
                  LoadBackupFile(backupFile, saveData, neverSaveData);
                  cout << "Information: Loading backup partition table; will override earlier problems!\n";
                  free(backupFile);
                  retval = 0;
                  break;
               case 'o':
                  JustLooking(0);
                  ClearGPTData();
                  saveData = 1;
                  cout << "Information: Creating fresh partition table; will override earlier problems!\n";
                  retval = 0;
                  break;
               case 'v':
                  cout << "Verification may miss some problems or report too many!\n";
                  Verify();
                  break;
               case 'z':
                  if (!pretend) {
                     DestroyGPT();
                  } // if
                  saveNonGPT = 0;
                  saveData = 0;
                  break;
               case 'Z':
                  if (!pretend) {
                     DestroyGPT();
                     DestroyMBR();
                  } // if
                  saveNonGPT = 0;
                  saveData = 0;
                  break;
            } // switch
         } // while
         retval = 2;
      } // if/else loaded OK
      if ((saveData) && (!neverSaveData) && (saveNonGPT) && (!pretend)) {
         SaveGPTData(1);
      }
      if (saveData && (!saveNonGPT)) {
         cout << "Non-GPT disk; not saving changes. Use -g to override.\n";
         retval = 3;
      } // if
      if (neverSaveData) {
         cerr << "Error encountered; not saving changes.\n";
         retval = 4;
      } // if
   } // if (device != NULL)
   poptFreeContext(poptCon);
   return retval;
} // GPTDataCL::DoOptions()

// Create a hybrid or regular MBR from GPT data structures
int GPTDataCL::BuildMBR(char* argument, int isHybrid) {
   int numParts, allOK = 1, i, origPartNum;
   MBRPart newPart;
   BasicMBRData newMBR;

   if (argument != NULL) {
      numParts = CountColons(argument) + 1;
      if (numParts <= (4 - isHybrid)) {
         newMBR.SetDisk(GetDisk());
         for (i = 0; i < numParts; i++) {
            origPartNum = GetInt(argument, i + 1) - 1;
            if (IsUsedPartNum(origPartNum) && (partitions[origPartNum].IsSizedForMBR() == MBR_SIZED_GOOD)) {
               newPart.SetInclusion(PRIMARY);
               newPart.SetLocation(operator[](origPartNum).GetFirstLBA(),
                                   operator[](origPartNum).GetLengthLBA());
               newPart.SetStatus(0);
               newPart.SetType((uint8_t)(operator[](origPartNum).GetHexType() / 0x0100));
               newMBR.AddPart(i + isHybrid, newPart);
            } else {
               cerr << "Original partition " << origPartNum + 1 << " does not exist or is too big! Aborting operation!\n";
               allOK = 0;
            } // if/else
         } // for
         if (isHybrid) {
            newPart.SetInclusion(PRIMARY);
            newPart.SetLocation(1, newMBR.FindLastInFree(1));
            newPart.SetStatus(0);
            newPart.SetType(0xEE);
            newMBR.AddPart(0, newPart);
         } // if
         if (allOK)
            SetProtectiveMBR(newMBR);
      } else allOK = 0;
   } else allOK = 0;
   if (!allOK)
      cerr << "Problem creating MBR!\n";
   return allOK;
} // GPTDataCL::BuildMBR()

// Returns the number of colons in argument string, ignoring the
// first character (thus, a leading colon is ignored, as GetString()
// does).
int CountColons(char* argument) {
   int num = 0;

   while ((argument[0] != '\0') && (argument = strchr(&argument[1], ':')))
      num++;

   return num;
} // GPTDataCL::CountColons()

// Extract integer data from argument string, which should be colon-delimited
uint64_t GetInt(const string & argument, int itemNum) {
   uint64_t retval;

   istringstream inString(GetString(argument, itemNum));
   inString >> retval;
   return retval;
} // GPTDataCL::GetInt()

// Extract string data from argument string, which should be colon-delimited
// If string begins with a colon, that colon is skipped in the counting. If an
// invalid itemNum is specified, returns an empty string.
string GetString(string argument, int itemNum) {
   size_t startPos = 0, endPos = 0;
   string retVal = "";
   int foundLast = 0;
   int numFound = 0;

   if (argument[0] == ':')
      argument.erase(0, 1);
   while ((numFound < itemNum) && (!foundLast)) {
      endPos = argument.find(':', startPos);
      numFound++;
      if (endPos == string::npos) {
         foundLast = 1;
         endPos = argument.length();
      } else if (numFound < itemNum) {
         startPos = endPos + 1;
      } // if/elseif
   } // while
   if ((numFound == itemNum) && (numFound > 0))
      retVal = argument.substr(startPos, endPos - startPos);

   return retVal;
} // GetString()
