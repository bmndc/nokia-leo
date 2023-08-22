//
// C++ Interface: diskio (platform-independent components)
//
// Description: Class to handle low-level disk I/O for GPT fdisk
//
//
// Author: Rod Smith <rodsmith@rodsbooks.com>, (C) 2009
//
// Copyright: See COPYING file that comes with this distribution
//
//
// This program is copyright (c) 2009 by Roderick W. Smith. It is distributed
// under the terms of the GNU GPL version 2, as detailed in the COPYING file.

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#ifdef _WIN32
#include <windows.h>
#include <winioctl.h>
#define fstat64 fstat
#define stat64 stat
#define S_IRGRP 0
#define S_IROTH 0
#else
#include <sys/ioctl.h>
#endif
#include <string>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <iostream>

#include "support.h"
#include "diskio.h"
//#include "gpt.h"

using namespace std;

DiskIO::DiskIO(void) {
   userFilename = "";
   realFilename = "";
   isOpen = 0;
   openForWrite = 0;
} // constructor

DiskIO::~DiskIO(void) {
   Close();
} // destructor

// Open a disk device for reading. Returns 1 on success, 0 on failure.
int DiskIO::OpenForRead(const string & filename) {
   int shouldOpen = 1;

   if (isOpen) { // file is already open
      if (((realFilename != filename) && (userFilename != filename)) || (openForWrite)) {
         Close();
      } else {
         shouldOpen = 0;
      } // if/else
   } // if

   if (shouldOpen) {
      userFilename = filename;
      MakeRealName();
      OpenForRead();
   } // if

   return isOpen;
} // DiskIO::OpenForRead(string filename)

// Open a disk for reading and writing by filename.
// Returns 1 on success, 0 on failure.
int DiskIO::OpenForWrite(const string & filename) {
   int retval = 0;

   if ((isOpen) && (openForWrite) && ((filename == realFilename) || (filename == userFilename))) {
      retval = 1;
   } else {
      userFilename = filename;
      MakeRealName();
      retval = OpenForWrite();
      if (retval == 0) {
         realFilename = userFilename = "";
      } // if
   } // if/else
   return retval;
} // DiskIO::OpenForWrite(string filename)
