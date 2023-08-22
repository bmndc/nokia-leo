// support.cc
// Non-class support functions for gdisk program.
// Primarily by Rod Smith, February 2009, but with a few functions
// copied from other sources (see attributions below).

/* This program is copyright (c) 2009-2013 by Roderick W. Smith. It is distributed
  under the terms of the GNU GPL version 2, as detailed in the COPYING file. */

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <string>
#include <iostream>
#include <sstream>
#include "support.h"

#include <sys/types.h>

// As of 1/2010, BLKPBSZGET is very new, so I'm explicitly defining it if
// it's not already defined. This should become unnecessary in the future.
// Note that this is a Linux-only ioctl....
#ifndef BLKPBSZGET
#define BLKPBSZGET _IO(0x12,123)
#endif

using namespace std;

// Reads a string from stdin, returning it as a C++-style string.
// Note that the returned string will NOT include the carriage return
// entered by the user.
string ReadString(void) {
   string inString;

   getline(cin, inString);
   if (!cin.good())
      exit(5);
   return inString;
} // ReadString()

// Get a numeric value from the user, between low and high (inclusive).
// Keeps looping until the user enters a value within that range.
// If user provides no input, def (default value) is returned.
// (If def is outside of the low-high range, an explicit response
// is required.)
int GetNumber(int low, int high, int def, const string & prompt) {
   int response, num;
   char line[255];

   if (low != high) { // bother only if low and high differ...
      do {
         cout << prompt;
         cin.getline(line, 255);
         if (!cin.good())
            exit(5);
         num = sscanf(line, "%d", &response);
         if (num == 1) { // user provided a response
            if ((response < low) || (response > high))
               cout << "Value out of range\n";
         } else { // user hit enter; return default
            response = def;
         } // if/else
      } while ((response < low) || (response > high));
   } else { // low == high, so return this value
      cout << "Using " << low << "\n";
      response = low;
   } // else
   return (response);
} // GetNumber()

// Gets a Y/N response (and converts lowercase to uppercase)
char GetYN(void) {
   char response;
   string line;
   bool again = 0 ;

   do {
      if ( again ) { cout << "Your option? " ; }
      again = 1 ;
      cout << "(Y/N): ";
      line = ReadString();
      response = toupper(line[0]);
   } while ((response != 'Y') && (response != 'N'));
   return response;
} // GetYN(void)

// Obtains a sector number, between low and high, from the
// user, accepting values prefixed by "+" to add sectors to low,
// or the same with "K", "M", "G", "T", or "P" as suffixes to add
// kilobytes, megabytes, gigabytes, terabytes, or petabytes,
// respectively. If a "-" prefix is used, use the high value minus
// the user-specified number of sectors (or KiB, MiB, etc.). Use the
// def value as the default if the user just hits Enter. The sSize is
// the sector size of the device.
uint64_t GetSectorNum(uint64_t low, uint64_t high, uint64_t def, uint64_t sSize,
                      const string & prompt) {
   uint64_t response;
   char line[255];

   do {
      cout << prompt;
      cin.getline(line, 255);
      if (!cin.good())
         exit(5);
      response = IeeeToInt(line, sSize, low, high, def);
   } while ((response < low) || (response > high));
   return response;
} // GetSectorNum()

// Convert an IEEE-1541-2002 value (K, M, G, T, P, or E) to its equivalent in
// number of sectors. If no units are appended, interprets as the number
// of sectors; otherwise, interprets as number of specified units and
// converts to sectors. For instance, with 512-byte sectors, "1K" converts
// to 2. If value includes a "+", adds low and subtracts 1; if SIValue
// inclues a "-", subtracts from high. If IeeeValue is empty, returns def.
// Returns final sector value. In case inValue is invalid, returns 0 (a
// sector value that's always in use on GPT and therefore invalid); and if
// inValue works out to something outside the range low-high, returns the
// computed value; the calling function is responsible for checking the
// validity of this value.
// NOTE: There's a difference in how GCC and VC++ treat oversized values
// (say, "999999999999999999999") read via the ">>" operator; GCC turns
// them into the maximum value for the type, whereas VC++ turns them into
// 0 values. The result is that IeeeToInt() returns UINT64_MAX when
// compiled with GCC (and so the value is rejected), whereas when VC++
// is used, the default value is returned.
uint64_t IeeeToInt(string inValue, uint64_t sSize, uint64_t low, uint64_t high, uint64_t def) {
   uint64_t response = def, bytesPerUnit = 1, mult = 1, divide = 1;
   size_t foundAt = 0;
   char suffix, plusFlag = ' ';
   string suffixes = "KMGTPE";
   int badInput = 0; // flag bad input; once this goes to 1, other values are irrelevant

   if (sSize == 0) {
      sSize = SECTOR_SIZE;
      cerr << "Bug: Sector size invalid in IeeeToInt()!\n";
   } // if

   // Remove leading spaces, if present
   while (inValue[0] == ' ')
      inValue.erase(0, 1);

   // If present, flag and remove leading plus or minus sign
   if ((inValue[0] == '+') || (inValue[0] == '-')) {
      plusFlag = inValue[0];
      inValue.erase(0, 1);
   } // if

   // Extract numeric response and, if present, suffix
   istringstream inString(inValue);
   if (((inString.peek() < '0') || (inString.peek() > '9')) && (inString.peek() != -1))
      badInput = 1;
   inString >> response >> suffix;
   suffix = toupper(suffix);

   // If no response, or if response == 0, use default (def)
   if ((inValue.length() == 0) || (response == 0)) {
      response = def;
      suffix = ' ';
      plusFlag = ' ';
   } // if

   // Find multiplication and division factors for the suffix
   foundAt = suffixes.find(suffix);
   if (foundAt != string::npos) {
      bytesPerUnit = UINT64_C(1) << (10 * (foundAt + 1));
      mult = bytesPerUnit / sSize;
      divide = sSize / bytesPerUnit;
   } // if

   // Adjust response based on multiplier and plus flag, if present
   if (mult > 1) {
      if (response > (UINT64_MAX / mult))
         badInput = 1;
      else
         response *= mult;
   } else if (divide > 1) {
         response /= divide;
   } // if/elseif

   if (plusFlag == '+') {
      // Recompute response based on low part of range (if default == high
      // value, which should be the case when prompting for the end of a
      // range) or the defaut value (if default != high, which should be
      // the case for the first sector of a partition).
      if (def == high) {
         if (response > 0)
            response--;
         if (response > (UINT64_MAX - low))
            badInput = 1;
         else
            response = response + low;
      } else {
         if (response > (UINT64_MAX - def))
            badInput = 1;
         else
            response = response + def;
      } // if/else
   } else if (plusFlag == '-') {
      if (response > high)
         badInput = 1;
      else
         response = high - response;
   } // if   

   if (badInput)
      response = UINT64_C(0);

   return response;
} // IeeeToInt()

// Takes a size and converts this to a size in IEEE-1541-2002 units (KiB, MiB,
// GiB, TiB, PiB, or EiB), returned in C++ string form. The size is either in
// units of the sector size or, if that parameter is omitted, in bytes.
// (sectorSize defaults to 1). Note that this function uses peculiar
// manual computation of decimal value rather than simply setting
// theValue.precision() because this isn't possible using the available
// EFI library.
string BytesToIeee(uint64_t size, uint32_t sectorSize) {
   uint64_t sizeInIeee;
   uint64_t previousIeee;
   float decimalIeee;
   uint index = 0;
   string units, prefixes = " KMGTPEZ";
   ostringstream theValue;

   sizeInIeee = previousIeee = size * (uint64_t) sectorSize;
   while ((sizeInIeee > 1024) && (index < (prefixes.length() - 1))) {
      index++;
      previousIeee = sizeInIeee;
      sizeInIeee /= 1024;
   } // while
   if (prefixes[index] == ' ') {
      theValue << sizeInIeee << " bytes";
   } else {
      units = "  iB";
      units[1] = prefixes[index];
      decimalIeee = ((float) previousIeee -
                     ((float) sizeInIeee * 1024.0) + 51.2) / 102.4;
      if (decimalIeee >= 10.0) {
         decimalIeee = 0.0;
         sizeInIeee++;
      }
      theValue << sizeInIeee << "." << (uint32_t) decimalIeee << units;
   } // if/else
   return theValue.str();
} // BytesToIeee()

// Converts two consecutive characters in the input string into a
// number, interpreting the string as a hexadecimal number, starting
// at the specified position.
unsigned char StrToHex(const string & input, unsigned int position) {
   unsigned char retval = 0x00;
   unsigned int temp;

   if (input.length() > position) {
      sscanf(input.substr(position, 2).c_str(), "%x", &temp);
      retval = (unsigned char) temp;
   } // if
   return retval;
} // StrToHex()

// Returns 1 if input can be interpreted as a hexadecimal number --
// all characters must be spaces, digits, or letters A-F (upper- or
// lower-case), with at least one valid hexadecimal digit; with the
// exception of the first two characters, which may be "0x"; otherwise
// returns 0.
int IsHex(string input) {
   int isHex = 1, foundHex = 0, i;

   if (input.substr(0, 2) == "0x")
      input.erase(0, 2);
   for (i = 0; i < (int) input.length(); i++) {
      if ((input[i] < '0') || (input[i] > '9')) {
         if ((input[i] < 'A') || (input[i] > 'F')) {
            if ((input[i] < 'a') || (input[i] > 'f')) {
               if ((input[i] != ' ') && (input[i] != '\n')) {
                  isHex = 0;
               }
            } else foundHex = 1;
         } else foundHex = 1;
      } else foundHex = 1;
   } // for
   if (!foundHex)
      isHex = 0;
   return isHex;
} // IsHex()

// Return 1 if the CPU architecture is little endian, 0 if it's big endian....
int IsLittleEndian(void) {
   int littleE = 1; // assume little-endian (Intel-style)
   union {
      uint32_t num;
      unsigned char uc[sizeof(uint32_t)];
   } endian;

   endian.num = 1;
   if (endian.uc[0] != (unsigned char) 1) {
      littleE = 0;
   } // if
   return (littleE);
} // IsLittleEndian()

// Reverse the byte order of theValue; numBytes is number of bytes
void ReverseBytes(void* theValue, int numBytes) {
   char* tempValue = NULL;
   int i;

   tempValue = new char [numBytes];
   if (tempValue != NULL) {
      memcpy(tempValue, theValue, numBytes);
      for (i = 0; i < numBytes; i++)
         ((char*) theValue)[i] = tempValue[numBytes - i - 1];
      delete[] tempValue;
   } else {
      cerr << "Could not allocate memory in ReverseBytes()! Terminating\n";
      exit(1);
   } // if/else
} // ReverseBytes()

// On Windows, display a warning and ask whether to continue. If the user elects
// not to continue, exit immediately.
void WinWarning(void) {
   #ifdef _WIN32
   cout << "\a************************************************************************\n"
        << "Most versions of Windows cannot boot from a GPT disk except on a UEFI-based\n"
        << "computer, and most varieties prior to Vista cannot read GPT disks. Therefore,\n"
        << "you should exit now unless you understand the implications of converting MBR\n"
        << "to GPT or creating a new GPT disk layout!\n"
        << "************************************************************************\n\n";
   cout << "Are you SURE you want to continue? ";
   if (GetYN() != 'Y')
      exit(0);
   #endif
} // WinWarning()
