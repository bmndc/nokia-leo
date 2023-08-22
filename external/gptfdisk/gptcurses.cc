/*
 *    Implementation of GPTData class derivative with curses-based text-mode
 *    interaction
 *    Copyright (C) 2011-2013 Roderick W. Smith
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License along
 *    with this program; if not, write to the Free Software Foundation, Inc.,
 *    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include <iostream>
#include <string>
#include <sstream>
#include <ncurses.h>
#include "gptcurses.h"
#include "support.h"

using namespace std;

// # of lines to reserve for general information and headers (RESERVED_TOP)
// and for options and messages (RESERVED_BOTTOM)
#define RESERVED_TOP 7
#define RESERVED_BOTTOM 5

int GPTDataCurses::numInstances = 0;

GPTDataCurses::GPTDataCurses(void) {
   if (numInstances > 0) {
      refresh();
   } else {
      setlocale( LC_ALL , "" );
      initscr();
      cbreak();
      noecho();
      intrflush(stdscr, false);
      keypad(stdscr, true);
      nonl();
      numInstances++;
   } // if/else
   firstSpace = NULL;
   lastSpace = NULL;
   currentSpace = NULL;
   currentSpaceNum = -1;
   whichOptions = ""; // current set of options
   currentKey = 'b'; // currently selected option
   displayType = USE_CURSES;
} // GPTDataCurses constructor

GPTDataCurses::~GPTDataCurses(void) {
   numInstances--;
   if ((numInstances == 0) && !isendwin())
      endwin();
} // GPTDataCurses destructor

/************************************************
 *                                              *
 * Functions relating to Spaces data structures *
 *                                              *
 ************************************************/

void GPTDataCurses::EmptySpaces(void) {
   Space *trash;

   while (firstSpace != NULL) {
      trash = firstSpace;
      firstSpace = firstSpace->nextSpace;
      delete trash;
   } // if
   numSpaces = 0;
   lastSpace = NULL;
} // GPTDataCurses::EmptySpaces()

// Create Spaces from partitions. Does NOT creates Spaces to represent
// unpartitioned space on the disk.
// Returns the number of Spaces created.
int GPTDataCurses::MakeSpacesFromParts(void) {
   uint i;
   Space *tempSpace;

   EmptySpaces();
   for (i = 0; i < numParts; i++) {
      if (partitions[i].IsUsed()) {
         tempSpace = new Space;
         tempSpace->firstLBA = partitions[i].GetFirstLBA();
         tempSpace->lastLBA = partitions[i].GetLastLBA();
         tempSpace->origPart = &partitions[i];
         tempSpace->partNum = (int) i;
         LinkToEnd(tempSpace);
      } // if
   } // for
   return numSpaces;
} // GPTDataCurses::MakeSpacesFromParts()

// Add a single empty Space to the current Spaces linked list and sort the result....
void GPTDataCurses::AddEmptySpace(uint64_t firstLBA, uint64_t lastLBA) {
   Space *tempSpace;

   tempSpace = new Space;
   tempSpace->firstLBA = firstLBA;
   tempSpace->lastLBA = lastLBA;
   tempSpace->origPart = &emptySpace;
   tempSpace->partNum = -1;
   LinkToEnd(tempSpace);
   SortSpaces();
} // GPTDataCurses::AddEmptySpace();

// Add Spaces to represent the unallocated parts of the partition table.
// Returns the number of Spaces added.
int GPTDataCurses::AddEmptySpaces(void) {
   int numAdded = 0;
   Space *current;

   SortSpaces();
   if (firstSpace == NULL) {
      AddEmptySpace(GetFirstUsableLBA(), GetLastUsableLBA());
      numAdded++;
   } else {
      current = firstSpace;
      while ((current != NULL) /* && (current->partNum != -1) */ ) {
         if ((current == firstSpace) && (current->firstLBA > GetFirstUsableLBA())) {
            AddEmptySpace(GetFirstUsableLBA(), current->firstLBA - 1);
            numAdded++;
         } // if
         if ((current == lastSpace) && (current->lastLBA < GetLastUsableLBA())) {
            AddEmptySpace(current->lastLBA + 1, GetLastUsableLBA());
            numAdded++;
         } // if
         if ((current->prevSpace != NULL) && (current->prevSpace->lastLBA < (current->firstLBA - 1))) {
            AddEmptySpace(current->prevSpace->lastLBA + 1, current->firstLBA - 1);
            numAdded++;
         } // if
         current = current->nextSpace;
      } // while
   } // if/else
   return numAdded;
} // GPTDataCurses::AddEmptySpaces()

// Remove the specified Space from the linked list and set its previous and
// next pointers to NULL.
void GPTDataCurses::UnlinkSpace(Space *theSpace) {
   if (theSpace != NULL) {
      if (theSpace->prevSpace != NULL)
         theSpace->prevSpace->nextSpace = theSpace->nextSpace;
      if (theSpace->nextSpace != NULL)
         theSpace->nextSpace->prevSpace = theSpace->prevSpace;
      if (theSpace == firstSpace)
         firstSpace = theSpace->nextSpace;
      if (theSpace == lastSpace)
         lastSpace = theSpace->prevSpace;
      theSpace->nextSpace = NULL;
      theSpace->prevSpace = NULL;
      numSpaces--;
   } // if
} // GPTDataCurses::UnlinkSpace

// Link theSpace to the end of the current linked list.
void GPTDataCurses::LinkToEnd(Space *theSpace) {
   if (lastSpace == NULL) {
      firstSpace = lastSpace = theSpace;
      theSpace->nextSpace = NULL;
      theSpace->prevSpace = NULL;
   } else {
      theSpace->prevSpace = lastSpace;
      theSpace->nextSpace = NULL;
      lastSpace->nextSpace = theSpace;
      lastSpace = theSpace;
   } // if/else
   numSpaces++;
} // GPTDataCurses::LinkToEnd()

// Sort spaces into ascending order by on-disk position.
void GPTDataCurses::SortSpaces(void) {
   Space *oldFirst, *oldLast, *earliest = NULL, *current = NULL;

   oldFirst = firstSpace;
   oldLast = lastSpace;
   firstSpace = lastSpace = NULL;
   while (oldFirst != NULL) {
      current = earliest = oldFirst;
      while (current != NULL) {
         if (current->firstLBA < earliest->firstLBA)
            earliest = current;
         current = current->nextSpace;
      } // while
      if (oldFirst == earliest)
         oldFirst = earliest->nextSpace;
      if (oldLast == earliest)
         oldLast = earliest->prevSpace;
      UnlinkSpace(earliest);
      LinkToEnd(earliest);
   } // while
} // GPTDataCurses::SortSpaces()

// Identify the spaces on the disk, a "space" being defined as a partition
// or an empty gap between, before, or after partitions. The spaces are
// presented to users in the main menu display.
void GPTDataCurses::IdentifySpaces(void) {
   MakeSpacesFromParts();
   AddEmptySpaces();
} // GPTDataCurses::IdentifySpaces()

/**************************
 *                        *
 * Data display functions *
 *                        *
 **************************/

// Display a single Space on line # lineNum.
// Returns a pointer to the space being displayed
Space* GPTDataCurses::ShowSpace(int spaceNum, int lineNum) {
   Space *space;
   int i = 0;
#ifdef USE_UTF16
   char temp[40];
#endif

   space = firstSpace;
   while ((space != NULL) && (i < spaceNum)) {
      space = space->nextSpace;
      i++;
   } // while
   if ((space != NULL) && (lineNum < (LINES - 5))) {
      ClearLine(lineNum);
      if (space->partNum == -1) { // space is empty
         move(lineNum, 12);
         printw(BytesToIeee((space->lastLBA - space->firstLBA + 1), blockSize).c_str());
         move(lineNum, 24);
         printw("free space");
      } else { // space holds a partition
         move(lineNum, 3);
         printw("%d", space->partNum + 1);
         move(lineNum, 12);
         printw(BytesToIeee((space->lastLBA - space->firstLBA + 1), blockSize).c_str());
         move(lineNum, 24);
         printw(space->origPart->GetTypeName().c_str());
         move(lineNum, 50);
         #ifdef USE_UTF16
         space->origPart->GetDescription().extract(0, 39, temp, 39);
         printw(temp);
         #else
         printw(space->origPart->GetDescription().c_str());
         #endif
      } // if/else
   } // if
   return space;
} // GPTDataCurses::ShowSpace

// Display the partitions, being sure that the space #selected is displayed
// and highlighting that space.
// Returns the number of the space being shown (should be selected, but will
// be -1 if something weird happens)
int GPTDataCurses::DisplayParts(int selected) {
   int lineNum = 5, i = 0, retval = -1, numToShow, pageNum;
   string theLine;

   move(lineNum++, 0);
   theLine = "Part. #     Size        Partition Type            Partition Name";
   printw(theLine.c_str());
   move(lineNum++, 0);
   theLine = "----------------------------------------------------------------";
   printw(theLine.c_str());
   numToShow = LINES - RESERVED_TOP - RESERVED_BOTTOM;
   pageNum = selected / numToShow;
   for (i = pageNum * numToShow; i <= (pageNum + 1) * numToShow - 1; i++) {
      if (i < numSpaces) { // real space; show it
         if (i == selected) {
            currentSpaceNum = i;
            if (displayType == USE_CURSES) {
               attron(A_REVERSE);
               currentSpace = ShowSpace(i, lineNum++);
               attroff(A_REVERSE);
            } else {
               currentSpace = ShowSpace(i, lineNum);
               move(lineNum++, 0);
               printw(">");
            }
            DisplayOptions(i);
            retval = selected;
         } else {
            ShowSpace(i, lineNum++);
         }
      } else { // blank in display
         ClearLine(lineNum++);
      } // if/else
   } // for
   refresh();
   return retval;
} // GPTDataCurses::DisplayParts()

/**********************************************
 *                                            *
 * Functions corresponding to main menu items *
 *                                            *
 **********************************************/

// Delete the specified partition and re-detect partitions and spaces....
void GPTDataCurses::DeletePartition(int partNum) {
   if (!GPTData::DeletePartition(partNum))
      Report("Could not delete partition!");
   IdentifySpaces();
   if (currentSpaceNum >= numSpaces) {
      currentSpaceNum = numSpaces - 1;
      currentSpace = lastSpace;
   } // if
} // GPTDataCurses::DeletePartition()

// Displays information on the specified partition
void GPTDataCurses::ShowInfo(int partNum) {
   uint64_t size;
#ifdef USE_UTF16
   char temp[NAME_SIZE + 1];
#endif

   clear();
   move(2, (COLS - 29) / 2);
   printw("Information for partition #%d\n\n", partNum + 1);
   printw("Partition GUID code: %s (%s)\n", partitions[partNum].GetType().AsString().c_str(),
          partitions[partNum].GetTypeName().c_str());
   printw("Partition unique GUID: %s\n", partitions[partNum].GetUniqueGUID().AsString().c_str());
   printw("First sector: %lld (at %s)\n", partitions[partNum].GetFirstLBA(),
          BytesToIeee(partitions[partNum].GetFirstLBA(), blockSize).c_str());
   printw("Last sector: %lld (at %s)\n", partitions[partNum].GetLastLBA(),
          BytesToIeee(partitions[partNum].GetLastLBA(), blockSize).c_str());
   size = partitions[partNum].GetLastLBA() - partitions[partNum].GetFirstLBA();
   printw("Partition size: %lld sectors (%s)\n", size, BytesToIeee(size, blockSize).c_str());
   printw("Attribute flags: %016x\n", partitions[partNum].GetAttributes().GetAttributes());
   #ifdef USE_UTF16
   partitions[partNum].GetDescription().extract(0, NAME_SIZE , temp, NAME_SIZE );
   printw("Partition name: '%s'\n", temp);
   #else
   printw("Partition name: '%s'\n", partitions[partNum].GetDescription().c_str());
   #endif
   PromptToContinue();
} // GPTDataCurses::ShowInfo()

// Prompt for and change a partition's name....
void GPTDataCurses::ChangeName(int partNum) {
   char temp[NAME_SIZE + 1];

   if (ValidPartNum(partNum)) {
      move(LINES - 4, 0);
      clrtobot();
      move(LINES - 4, 0);
      #ifdef USE_UTF16
      partitions[partNum].GetDescription().extract(0, NAME_SIZE , temp, NAME_SIZE );
      printw("Current partition name is '%s'\n", temp);
      #else
      printw("Current partition name is '%s'\n", partitions[partNum].GetDescription().c_str());
      #endif
      printw("Enter new partition name, or <Enter> to use the current name:\n");
      echo();
      getnstr(temp, NAME_SIZE );
      partitions[partNum].SetName((string) temp);
      noecho();
   } // if
} // GPTDataCurses::ChangeName()

// Change the partition's type code....
void GPTDataCurses::ChangeType(int partNum) {
   char temp[80] = "L\0";
   PartType tempType;

   echo();
   do {
      move(LINES - 4, 0);
      clrtobot();
      move(LINES - 4, 0);
      printw("Current type is %04x (%s)\n", partitions[partNum].GetType().GetHexType(), partitions[partNum].GetTypeName().c_str());
      printw("Hex code or GUID (L to show codes, Enter = %04x): ", partitions[partNum].GetType().GetHexType());
      getnstr(temp, 79);
      if ((temp[0] == 'L') || (temp[0] == 'l')) {
         ShowTypes();
      } else {
         if (temp[0] == '\0')
            tempType = partitions[partNum].GetType().GetHexType();
         tempType = temp;
         partitions[partNum].SetType(tempType);
      } // if
   } while ((temp[0] == 'L') || (temp[0] == 'l') || (partitions[partNum].GetType() == (GUIDData) "0x0000"));
   noecho();
} // GPTDataCurses::ChangeType

// Sets the partition alignment value
void GPTDataCurses::SetAlignment(void) {
   int alignment;

   move(LINES - 4, 0);
   clrtobot();
   printw("Current partition alignment, in sectors, is %d.", GetAlignment());
   do {
      move(LINES - 3, 0);
      printw("Type new alignment value, in sectors: ");
      echo();
      scanw("%d", &alignment);
      noecho();
   } while ((alignment == 0) || (alignment > MAX_ALIGNMENT));
   GPTData::SetAlignment(alignment);
} // GPTDataCurses::SetAlignment()

// Verify the data structures. Note that this function leaves curses mode and
// relies on the underlying GPTData::Verify() function to report on problems
void GPTDataCurses::Verify(void) {
   char junk;

   def_prog_mode();
   endwin();
   GPTData::Verify();
   cout << "\nPress the <Enter> key to continue: ";
   cin.get(junk);
   reset_prog_mode();
   refresh();
} // GPTDataCurses::Verify()

// Create a new partition in the space pointed to by currentSpace.
void GPTDataCurses::MakeNewPart(void) {
   uint64_t size, newFirstLBA = 0, newLastLBA = 0;
   int partNum;
   char inLine[80];

   move(LINES - 4, 0);
   clrtobot();
   while ((newFirstLBA < currentSpace->firstLBA) || (newFirstLBA > currentSpace->lastLBA)) {
      newFirstLBA = currentSpace->firstLBA;
      move(LINES - 4, 0);
      clrtoeol();
      newFirstLBA = currentSpace->firstLBA;
      Align(&newFirstLBA);
      printw("First sector (%lld-%lld, default = %lld): ", newFirstLBA, currentSpace->lastLBA, newFirstLBA);
      echo();
      getnstr(inLine, 79);
      noecho();
      newFirstLBA = IeeeToInt(inLine, blockSize, currentSpace->firstLBA, currentSpace->lastLBA, newFirstLBA);
      Align(&newFirstLBA);
   } // while
   size = currentSpace->lastLBA - newFirstLBA + 1;
   while ((newLastLBA > currentSpace->lastLBA) || (newLastLBA < newFirstLBA)) {
      move(LINES - 3, 0);
      clrtoeol();
      printw("Size in sectors or {KMGTP} (default = %lld): ", size);
      echo();
      getnstr(inLine, 79);
      noecho();
      newLastLBA = newFirstLBA + IeeeToInt(inLine, blockSize, 1, size, size) - 1;
   } // while
   partNum = FindFirstFreePart();
   if (CreatePartition(partNum, newFirstLBA, newLastLBA)) { // created OK; set type code & name....
      ChangeType(partNum);
      ChangeName(partNum);
   } else {
      Report("Error creating partition!");
   } // if/else
} // GPTDataCurses::MakeNewPart()

// Prompt user for permission to save data and, if it's given, do so!
void GPTDataCurses::SaveData(void) {
   string answer = "";
   char inLine[80];

   move(LINES - 4, 0);
   clrtobot();
   move (LINES - 2, 14);
   printw("Warning!! This may destroy data on your disk!");
   echo();
   while ((answer != "yes") && (answer != "no")) {
      move (LINES - 4, 2);
      printw("Are you sure you want to write the partition table to disk? (yes or no): ");
      getnstr(inLine, 79);
      answer = inLine;
      if ((answer != "yes") && (answer != "no")) {
         move(LINES - 2, 0);
         clrtoeol();
         move(LINES - 2, 14);
         printw("Please enter 'yes' or 'no'");
      } // if
   } // while()
   noecho();
   if (answer == "yes") {
      if (SaveGPTData(1)) {
         if (!myDisk.DiskSync())
            Report("The kernel may be using the old partition table. Reboot to use the new\npartition table!");
      } else {
         Report("Problem saving data! Your partition table may be damaged!");
      }
   }
} // GPTDataCurses::SaveData()

// Back up the partition table, prompting user for a filename....
void GPTDataCurses::Backup(void) {
   char inLine[80];

   ClearBottom();
   move(LINES - 3, 0);
   printw("Enter backup filename to save: ");
   echo();
   getnstr(inLine, 79);
   noecho();
   SaveGPTBackup(inLine);
} // GPTDataCurses::Backup()

// Load a GPT backup from a file
void GPTDataCurses::LoadBackup(void) {
   char inLine[80];

   ClearBottom();
   move(LINES - 3, 0);
   printw("Enter backup filename to load: ");
   echo();
   getnstr(inLine, 79);
   noecho();
   if (!LoadGPTBackup(inLine))
      Report("Restoration failed!");
   IdentifySpaces();
} // GPTDataCurses::LoadBackup()

// Display some basic help information
void GPTDataCurses::ShowHelp(void) {
   int i = 0;

   clear();
   move(0, (COLS - 22) / 2);
   printw("Help screen for cgdisk");
   move(2, 0);
   printw("This is cgdisk, a curses-based disk partitioning program. You can use it\n");
   printw("to create, delete, and modify partitions on your hard disk.\n\n");
   attron(A_BOLD);
   printw("Use cgdisk only on GUID Partition Table (GPT) disks!\n");
   attroff(A_BOLD);
   printw("Use cfdisk on Master Boot Record (MBR) disks.\n\n");
   printw("Command      Meaning\n");
   printw("-------      -------\n");
   while (menuMain[i].key != 0) {
      printw("   %c         %s\n", menuMain[i].key, menuMain[i].desc.c_str());
      i++;
   } // while()
   PromptToContinue();
} // GPTDataCurses::ShowHelp()

/************************************
 *                                  *
 * User input and menuing functions *
 *                                  *
 ************************************/

// Change the currently-selected space....
void GPTDataCurses::ChangeSpaceSelection(int delta) {
   if (currentSpace != NULL) {
      while ((delta > 0) && (currentSpace->nextSpace != NULL)) {
         currentSpace = currentSpace->nextSpace;
         delta--;
         currentSpaceNum++;
      } // while
      while ((delta < 0) && (currentSpace->prevSpace != NULL)) {
         currentSpace = currentSpace->prevSpace;
         delta++;
         currentSpaceNum--;
      } // while
   } // if
   // Below will hopefully never be true; bad counting error (bug), so reset to
   // the first Space as a failsafe....
   if (DisplayParts(currentSpaceNum) != currentSpaceNum) {
      currentSpaceNum = 0;
      currentSpace = firstSpace;
      DisplayParts(currentSpaceNum);
   } // if
} // GPTDataCurses

// Move option selection left or right....
void GPTDataCurses::MoveSelection(int delta) {
   int newKeyNum;

   // Begin with a sanity check to ensure a valid key is selected....
   if (whichOptions.find(currentKey) == string::npos)
      currentKey = 'n';
   newKeyNum = whichOptions.find(currentKey);
   newKeyNum += delta;
   if (newKeyNum < 0)
      newKeyNum = whichOptions.length() - 1;
   newKeyNum %= whichOptions.length();
   currentKey = whichOptions[newKeyNum];
   DisplayOptions(currentKey);
} // GPTDataCurses::MoveSelection()

// Show user's options. Refers to currentSpace to determine which options to show.
// Highlights the option with the key selectedKey; or a default if that's invalid.
void GPTDataCurses::DisplayOptions(char selectedKey) {
   uint i, j = 0, firstLine, numPerLine;
   string optionName, optionDesc = "";

   if (currentSpace != NULL) {
      if (currentSpace->partNum == -1) { // empty space is selected
         whichOptions = EMPTY_SPACE_OPTIONS;
         if (whichOptions.find(selectedKey) == string::npos)
            selectedKey = 'n';
      } else { // a partition is selected
         whichOptions = PARTITION_OPTIONS;
         if (whichOptions.find(selectedKey) == string::npos)
            selectedKey = 't';
      } // if/else

      firstLine = LINES - 4;
      numPerLine = (COLS - 8) / 12;
      ClearBottom();
      move(firstLine, 0);
      for (i = 0; i < whichOptions.length(); i++) {
         optionName = "";
         for (j = 0; menuMain[j].key; j++) {
            if (menuMain[j].key == whichOptions[i]) {
               optionName = menuMain[j].name;
               if (whichOptions[i] == selectedKey)
                  optionDesc = menuMain[j].desc;
            } // if
         } // for
         move(firstLine + i / numPerLine, (i % numPerLine) * 12 + 4);
         if (whichOptions[i] == selectedKey) {
            attron(A_REVERSE);
            printw("[ %s ]", optionName.c_str());
            attroff(A_REVERSE);
         } else {
            printw("[ %s ]", optionName.c_str());
         } // if/else
      } // for
      move(LINES - 1, (COLS - optionDesc.length()) / 2);
      printw(optionDesc.c_str());
      currentKey = selectedKey;
   } // if
} // GPTDataCurses::DisplayOptions()

// Accept user input and process it. Returns when the program should terminate.
void GPTDataCurses::AcceptInput() {
   int inputKey, exitNow = 0;

   do {
      refresh();
      inputKey = getch();
      switch (inputKey) {
         case KEY_UP:
            ChangeSpaceSelection(-1);
            break;
         case KEY_DOWN:
            ChangeSpaceSelection(+1);
            break;
         case 339: // page up key
            ChangeSpaceSelection(RESERVED_TOP + RESERVED_BOTTOM - LINES);
            break;
         case 338: // page down key
            ChangeSpaceSelection(LINES - RESERVED_TOP - RESERVED_BOTTOM);
            break;
         case KEY_LEFT:
            MoveSelection(-1);
            break;
         case KEY_RIGHT:
            MoveSelection(+1);
            break;
         case KEY_ENTER: case 13:
            exitNow = Dispatch(currentKey);
            break;
         case 27: // escape key
            exitNow = 1;
            break;
         default:
            exitNow = Dispatch(inputKey);
            break;
      } // switch()
   } while (!exitNow);
} // GPTDataCurses::AcceptInput()

// Operation has been selected, so do it. Returns 1 if the program should
// terminate on return from this program, 0 otherwise.
int GPTDataCurses::Dispatch(char operation) {
   int exitNow = 0;

   switch (operation) {
      case 'a': case 'A':
         SetAlignment();
         break;
      case 'b': case 'B':
         Backup();
         break;
      case 'd': case 'D':
         if (ValidPartNum(currentSpace->partNum))
            DeletePartition(currentSpace->partNum);
         break;
      case 'h': case 'H':
         ShowHelp();
         break;
      case 'i': case 'I':
         if (ValidPartNum(currentSpace->partNum))
            ShowInfo(currentSpace->partNum);
         break;
      case 'l': case 'L':
         LoadBackup();
         break;
      case 'm': case 'M':
         if (ValidPartNum(currentSpace->partNum))
            ChangeName(currentSpace->partNum);
         break;
      case 'n': case 'N':
         if (currentSpace->partNum < 0) {
            MakeNewPart();
            IdentifySpaces();
         } // if
         break;
      case 'q': case 'Q':
         exitNow = 1;
         break;
      case 't': case 'T':
         if (ValidPartNum(currentSpace->partNum))
            ChangeType(currentSpace->partNum);
         break;
      case 'v': case 'V':
         Verify();
         break;
      case 'w': case 'W':
         SaveData();
         break;
      default:
         break;
   } // switch()
   DrawMenu();
   return exitNow;
} // GPTDataCurses::Dispatch()

// Draws the main menu
void GPTDataCurses::DrawMenu(void) {
   string title="cgdisk ";
   title += GPTFDISK_VERSION;
   string drive="Disk Drive: ";
   drive += device;
   ostringstream size;

   size << "Size: " << diskSize << ", " << BytesToIeee(diskSize, blockSize);

   clear();
   move(0, (COLS - title.length()) / 2);
   printw(title.c_str());
   move(2, (COLS - drive.length()) / 2);
   printw(drive.c_str());
   move(3, (COLS - size.str().length()) / 2);
   printw(size.str().c_str());
   DisplayParts(currentSpaceNum);
} // DrawMenu

int GPTDataCurses::MainMenu(void) {
   if (((LINES - RESERVED_TOP - RESERVED_BOTTOM) < 2) || (COLS < 80)) {
      Report("Display is too small; it must be at least 80 x 14 characters!");
   } else {
      if (GPTData::Verify() > 0)
         Report("Warning! Problems found on disk! Use the Verify function to learn more.\n"
                "Using gdisk or some other program may be necessary to repair the problems.");
      IdentifySpaces();
      currentSpaceNum = 0;
      DrawMenu();
      AcceptInput();
   } // if/else
   endwin();
   return 0;
} // GPTDataCurses::MainMenu

/***********************************************************
 *                                                         *
 * Non-class support functions (mostly related to ncurses) *
 *                                                         *
 ***********************************************************/

// Clears the specified line of all data....
void ClearLine(int lineNum) {
   move(lineNum, 0);
   clrtoeol();
} // ClearLine()

// Clear the last few lines of the display
void ClearBottom(void) {
   move(LINES - RESERVED_BOTTOM, 0);
   clrtobot();
} // ClearBottom()

void PromptToContinue(void) {
   ClearBottom();
   move(LINES - 2, (COLS - 29) / 2);
   printw("Press any key to continue....");
   cbreak();
   getch();
} // PromptToContinue()

// Display one line of text on the screen and prompt to press any key to continue.
void Report(string theText) {
   clear();
   move(0, 0);
   printw(theText.c_str());
   move(LINES - 2, (COLS - 29) / 2);
   printw("Press any key to continue....");
   cbreak();
   getch();
} // Report()

// Displays all the partition type codes and then prompts to continue....
// NOTE: This function temporarily exits curses mode as a matter of
// convenience.
void ShowTypes(void) {
   PartType tempType;
   char junk;

   def_prog_mode();
   endwin();
   tempType.ShowAllTypes(LINES - 3);
   cout << "\nPress the <Enter> key to continue: ";
   cin.get(junk);
   reset_prog_mode();
   refresh();
} // ShowTypes()
