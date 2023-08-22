//
// C++ Implementation: GUIDData
//
// Description: GUIDData class header
// Implements the GUIDData data structure and support methods
//
//
// Author: Rod Smith <rodsmith@rodsbooks.com>, (C) 2010-2011
//
// Copyright: See COPYING file that comes with this distribution
//
//

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <string>
#include <iostream>
#include "guid.h"
#include "support.h"

using namespace std;

bool GUIDData::firstInstance = 1;

GUIDData::GUIDData(void) {
   if (firstInstance) {
      srand((unsigned int) time(0));
      firstInstance = 0;
   } // if
   Zero();
} // constructor

GUIDData::GUIDData(const GUIDData & orig) {
   memcpy(uuidData, orig.uuidData, sizeof(uuidData));
} // copy constructor

GUIDData::GUIDData(const string & orig) {
   operator=(orig);
} // copy (from string) constructor

GUIDData::GUIDData(const char * orig) {
   operator=(orig);
} // copy (from char*) constructor

GUIDData::~GUIDData(void) {
} // destructor

GUIDData & GUIDData::operator=(const GUIDData & orig) {
   memcpy(uuidData, orig.uuidData, sizeof(uuidData));
   return *this;
} // GUIDData::operator=(const GUIDData & orig)

// Assign the GUID from a string input value. A GUID is normally formatted
// with four dashes as element separators, for a total length of 36
// characters. If the input string is this long or longer, this function
// assumes standard separator positioning; if the input string is less
// than 36 characters long, this function assumes the input GUID has
// been compressed by removal of separators. In either event, there's
// little in the way of sanity checking, so garbage in = garbage out!
// One special case: If the first character is 'r' or 'R', a random
// GUID is assigned.
GUIDData & GUIDData::operator=(const string & orig) {
   string copy, fragment;
   size_t len;
   // Break points for segments, either with or without characters separating the segments....
   size_t longSegs[6] = {0, 9, 14, 19, 24, 36};
   size_t shortSegs[6] = {0, 8, 12, 16, 20, 32};
   size_t *segStart = longSegs; // Assume there are separators between segments

   // If first character is an 'R' or 'r', set a random GUID; otherwise,
   // try to parse it as a real GUID
   if ((orig[0] == 'R') || (orig[0] == 'r')) {
      Randomize();
   } else {
      Zero();

      // Delete stray spaces....
      copy = DeleteSpaces(orig);

      // If length is too short, assume there are no separators between segments
      len = copy.length();
      if (len < 36) {
         segStart = shortSegs;
      };

      // Extract data fragments at fixed locations and convert to
      // integral types....
      if (len >= segStart[1]) {
         uuidData[3] = StrToHex(copy, 0);
         uuidData[2] = StrToHex(copy, 2);
         uuidData[1] = StrToHex(copy, 4);
         uuidData[0] = StrToHex(copy, 6);
      } // if
      if (len >= segStart[2]) {
         uuidData[5] = StrToHex(copy, (unsigned int) segStart[1]);
         uuidData[4] = StrToHex(copy, (unsigned int) segStart[1] + 2);
      } // if
      if (len >= segStart[3]) {
         uuidData[7] = StrToHex(copy, (unsigned int) segStart[2]);
         uuidData[6] = StrToHex(copy, (unsigned int) segStart[2] + 2);
      } // if
      if (len >= segStart[4]) {
         uuidData[8] = StrToHex(copy, (unsigned int) segStart[3]);
         uuidData[9] = StrToHex(copy, (unsigned int) segStart[3] + 2);
      } // if
      if (len >= segStart[5]) {
         uuidData[10] = StrToHex(copy, (unsigned int) segStart[4]);
         uuidData[11] = StrToHex(copy, (unsigned int) segStart[4] + 2);
         uuidData[12] = StrToHex(copy, (unsigned int) segStart[4] + 4);
         uuidData[13] = StrToHex(copy, (unsigned int) segStart[4] + 6);
         uuidData[14] = StrToHex(copy, (unsigned int) segStart[4] + 8);
         uuidData[15] = StrToHex(copy, (unsigned int) segStart[4] + 10);
      } // if
   } // if/else randomize/set value

   return *this;
} // GUIDData::operator=(const string & orig)

// Assignment from C-style string; rely on C++ casting....
GUIDData & GUIDData::operator=(const char * orig) {
   return operator=((string) orig);
} // GUIDData::operator=(const char * orig)

// Erase the contents of the GUID
void GUIDData::Zero(void) {
   memset(uuidData, 0, sizeof(uuidData));
} // GUIDData::Zero()

// Set a completely random GUID value....
// The uuid_generate() function returns a value that needs to have its
// first three fields byte-reversed to conform to Intel's GUID layout.
// The Windows UuidCreate() function doesn't need this adjustment. If
// neither function is defined, or if UuidCreate() fails, set a completely
// random GUID -- not completely kosher, but it works on most platforms
// (immediately after creating the UUID on Windows 7 being an important
// exception).
void GUIDData::Randomize(void) {
   int i, uuidGenerated = 0;

#ifdef _UUID_UUID_H
   uuid_generate(uuidData);
   ReverseBytes(&uuidData[0], 4);
   ReverseBytes(&uuidData[4], 2);
   ReverseBytes(&uuidData[6], 2);
   uuidGenerated = 1;
#endif
#if defined (_RPC_H) || defined (__RPC_H__)
   UUID MsUuid;
   if (UuidCreate(&MsUuid) == RPC_S_OK) {
      memcpy(uuidData, &MsUuid, 16);
      uuidGenerated = 1;
   } // if
#endif

   if (!uuidGenerated) {
      cerr << "Warning! Unable to generate a proper UUID! Creating an improper one as a last\n"
           << "resort! Windows 7 may crash if you save this partition table!\a\n";
      for (i = 0; i < 16; i++)
         uuidData[i] = (unsigned char) (256.0 * (rand() / (RAND_MAX + 1.0)));
   } // if
} // GUIDData::Randomize

// Equality operator; returns 1 if the GUIDs are equal, 0 if they're unequal
int GUIDData::operator==(const GUIDData & orig) const {
   return !memcmp(uuidData, orig.uuidData, sizeof(uuidData));
} // GUIDData::operator==

// Inequality operator; returns 1 if the GUIDs are unequal, 0 if they're equal
int GUIDData::operator!=(const GUIDData & orig) const {
   return !operator==(orig);
} // GUIDData::operator!=

// Return the GUID as a string, suitable for display to the user.
string GUIDData::AsString(void) const {
   char theString[40];

   sprintf(theString,
           "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
           uuidData[3], uuidData[2], uuidData[1], uuidData[0], uuidData[5],
           uuidData[4], uuidData[7], uuidData[6], uuidData[8], uuidData[9],
           uuidData[10], uuidData[11], uuidData[12], uuidData[13], uuidData[14],
           uuidData[15]);
   return theString;
} // GUIDData::AsString(void)

// Delete spaces or braces (which often enclose GUIDs) from the orig string,
// returning modified string.
string GUIDData::DeleteSpaces(string s) {
   size_t position;

   if (s.length() > 0) {
      for (position = s.length(); position > 0; position--) {
         if ((s[position - 1] == ' ') || (s[position - 1] == '{') || (s[position - 1] == '}')) {
            s.erase(position - 1, 1);
         } // if
      } // for
   } // if
   return s;
} // GUIDData::DeleteSpaces()

/*******************************
 *                             *
 * Non-class support functions *
 *                             *
 *******************************/

// Display a GUID as a string....
ostream & operator<<(ostream & os, const GUIDData & data) {
//   string asString;

   os << data.AsString();
   return os;
} // GUIDData::operator<<()
