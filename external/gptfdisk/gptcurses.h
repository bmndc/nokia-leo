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
#include "gptpart.h"
#include "gpt.h"

#ifndef __GPT_CURSES
#define __GPT_CURSES

using namespace std;

struct MenuItem {
   int key; // Keyboard shortcut
   string name; // Item name; 8 characters
   string desc; // Description
};

static struct MenuItem menuMain[] = {
   { 'a', "Align ", "Set partition alignment policy" },
   { 'b', "Backup", "Back up the partition table" },
   { 'd', "Delete", "Delete the current partition" },
   { 'h', " Help ", "Print help screen" },
   { 'i', " Info ", "Display information about the partition" },
   { 'l', " Load ", "Load partition table backup from file" },
   { 'm', " naMe ", "Change the partition's name" },
   { 'n', " New  ", "Create new partition from free space" },
   { 'q', " Quit ", "Quit program without writing partition table" },
   { 't', " Type ", "Change the filesystem type code GUID" },
   { 'v', "Verify", "Verify the integrity of the disk's data structures" },
   { 'w', "Write ", "Write partition table to disk (this might destroy data)" },
   { 0, "", "" }
};

#define EMPTY_SPACE_OPTIONS "abhlnqvw"
#define PARTITION_OPTIONS "abdhilmqtvw"

// Constants for how to highlight a selected menu item
#define USE_CURSES 1
#define USE_ARROW 2

// A "Space" is a partition or an unallocated chunk of disk space, maintained
// in a doubly-linked-list data structure to facilitate creating displays of
// partitions and unallocated chunks of space on the disk in the main
// cgdisk partition list. This list MUST be correctly maintained and in order,
// and the numSpaces variable in the main GPTDataCurses class must specify
// how many Spaces are in the main linked list of Spaces.
struct Space {
   uint64_t firstLBA;
   uint64_t lastLBA;
   GPTPart *origPart;
   int partNum;
   Space *nextSpace;
   Space *prevSpace;
};

class GPTDataCurses : public GPTData {
protected:
   static int numInstances;
   GPTPart emptySpace;
   Space *firstSpace;
   Space *lastSpace;
   Space *currentSpace;
   int currentSpaceNum;
   string whichOptions;
   char currentKey;
   int numSpaces;
   int displayType;

   // Functions relating to Spaces data structures
   void EmptySpaces(void);
   int MakeSpacesFromParts(void);
   void AddEmptySpace(uint64_t firstLBA, uint64_t lastLBA);
   int AddEmptySpaces(void);
   void UnlinkSpace(Space *theSpace);
   void LinkToEnd(Space *theSpace);
   void SortSpaces(void);
   void IdentifySpaces(void);

   // Data display functions
   Space* ShowSpace(int spaceNum, int lineNum);
   int DisplayParts(int selected);
public:
   GPTDataCurses(void);
   ~GPTDataCurses(void);
   // Functions corresponding to main menu items
   void DeletePartition(int partNum);
   void ShowInfo(int partNum);
   void ChangeName(int partNum);
   void ChangeType(int partNum);
   void SetAlignment(void);
   void Verify(void);
   void MakeNewPart(void);
   void SaveData(void);
   void Backup(void);
   void LoadBackup(void);
   void ShowHelp(void);
   // User input and menuing functions
   void SetDisplayType(int dt) {displayType = dt;}
   void ChangeSpaceSelection(int delta);
   void MoveSelection(int delta);
   void DisplayOptions(char selectedKey);
   void AcceptInput();
   int Dispatch(char operation);
   void DrawMenu(void);
   int MainMenu(void);
}; // class GPTDataCurses

// Non-class support functions (mostly to do simple curses stuff)....

void ClearLine(int lineNum);
void ClearBottom(void);
void PromptToContinue(void);
void Report(string theText);
void ShowTypes(void);

#endif
