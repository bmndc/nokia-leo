//
// C++ Interface: diskio (Unix components [Linux, FreeBSD, Mac OS X])
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

#include <sys/ioctl.h>
#include <string.h>
#include <string>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef __linux__
#include "linux/hdreg.h"
#endif

#include <iostream>

#include "diskio.h"

using namespace std;

// Returns the official "real" name for a shortened version of same.
// Trivial here; more important in Windows
void DiskIO::MakeRealName(void) {
   realFilename = userFilename;
} // DiskIO::MakeRealName()

// Open the currently on-record file for reading. Returns 1 if the file is
// already open or is opened by this call, 0 if opening the file doesn't
// work.
int DiskIO::OpenForRead(void) {
   int shouldOpen = 1;
   struct stat64 st;

   if (isOpen) { // file is already open
      if (openForWrite) {
         Close();
      } else {
         shouldOpen = 0;
      } // if/else
   } // if

   if (shouldOpen) {
      fd = open(realFilename.c_str(), O_RDONLY);
      if (fd == -1) {
         cerr << "Problem opening " << realFilename << " for reading! Error is " << errno << ".\n";
         if (errno == EACCES) // User is probably not running as root
            cerr << "You must run this program as root or use sudo!\n";
         if (errno == ENOENT)
            cerr << "The specified file does not exist!\n";
         realFilename = "";
         userFilename = "";
         isOpen = 0;
         openForWrite = 0;
      } else {
         isOpen = 0;
         openForWrite = 0;
         if (fstat64(fd, &st) == 0) {
            if (S_ISDIR(st.st_mode))
               cerr << "The specified path is a directory!\n";
#if !defined(__FreeBSD__) && !defined(__APPLE__)
            else if (S_ISCHR(st.st_mode))
               cerr << "The specified path is a character device!\n";
#endif
            else if (S_ISFIFO(st.st_mode))
               cerr << "The specified path is a FIFO!\n";
            else if (S_ISSOCK(st.st_mode))
               cerr << "The specified path is a socket!\n";
            else
               isOpen = 1;
         } // if (fstat64()...)
      } // if/else
   } // if

   return isOpen;
} // DiskIO::OpenForRead(void)

// An extended file-open function. This includes some system-specific checks.
// Returns 1 if the file is open, 0 otherwise....
int DiskIO::OpenForWrite(void) {
   if ((isOpen) && (openForWrite))
      return 1;

   // Close the disk, in case it's already open for reading only....
   Close();

   // try to open the device; may fail....
   fd = open(realFilename.c_str(), O_WRONLY | O_CREAT, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
#ifdef __APPLE__
   // MacOS X requires a shared lock under some circumstances....
   if (fd < 0) {
      cerr << "Warning: Devices opened with shared lock will not have their\npartition table automatically reloaded!\n";
      fd = open(realFilename.c_str(), O_WRONLY | O_SHLOCK);
   } // if
#endif
   if (fd >= 0) {
      isOpen = 1;
      openForWrite = 1;
   } else {
      isOpen = 0;
      openForWrite = 0;
   } // if/else
   return isOpen;
} // DiskIO::OpenForWrite(void)

// Close the disk device. Note that this does NOT erase the stored filenames,
// so the file can be re-opened without specifying the filename.
void DiskIO::Close(void) {
   if (isOpen)
      if (close(fd) < 0)
         cerr << "Warning! Problem closing file!\n";
   isOpen = 0;
   openForWrite = 0;
} // DiskIO::Close()

// Returns block size of device pointed to by fd file descriptor. If the ioctl
// returns an error condition, print a warning but return a value of SECTOR_SIZE
// (512). If the disk can't be opened at all, return a value of 0.
int DiskIO::GetBlockSize(void) {
   int err = -1, blockSize = 0;
#ifdef __sun__
   struct dk_minfo minfo;
#endif

   // If disk isn't open, try to open it....
   if (!isOpen) {
      OpenForRead();
   } // if

   if (isOpen) {
#ifdef __APPLE__
      err = ioctl(fd, DKIOCGETBLOCKSIZE, &blockSize);
#endif
#ifdef __sun__
      err = ioctl(fd, DKIOCGMEDIAINFO, &minfo);
      if (err == 0)
          blockSize = minfo.dki_lbsize;
#endif
#if defined (__FreeBSD__) || defined (__FreeBSD_kernel__)
      err = ioctl(fd, DIOCGSECTORSIZE, &blockSize);
#endif
#ifdef __linux__
      err = ioctl(fd, BLKSSZGET, &blockSize);
#endif

      if (err == -1) {
         blockSize = SECTOR_SIZE;
         // ENOTTY = inappropriate ioctl; probably being called on a disk image
         // file, so don't display the warning message....
         // 32-bit code returns EINVAL, I don't know why. I know I'm treading on
         // thin ice here, but it should be OK in all but very weird cases....
         if ((errno != ENOTTY) && (errno != EINVAL)) {
            cerr << "\aError " << errno << " when determining sector size! Setting sector size to "
                 << SECTOR_SIZE << "\n";
            cout << "Disk device is " << realFilename << "\n";
         } // if
      } // if (err == -1)
   } // if (isOpen)

   return (blockSize);
} // DiskIO::GetBlockSize()

// Returns the number of heads, according to the kernel, or 255 if the
// correct value can't be determined.
uint32_t DiskIO::GetNumHeads(void) {
   uint32_t numHeads = 255;

#ifdef HDIO_GETGEO
   struct hd_geometry geometry;

   // If disk isn't open, try to open it....
   if (!isOpen)
      OpenForRead();

   if (!ioctl(fd, HDIO_GETGEO, &geometry))
      numHeads = (uint32_t) geometry.heads;
#endif
   return numHeads;
} // DiskIO::GetNumHeads();

// Returns the number of sectors per track, according to the kernel, or 63
// if the correct value can't be determined.
uint32_t DiskIO::GetNumSecsPerTrack(void) {
   uint32_t numSecs = 63;

   #ifdef HDIO_GETGEO
   struct hd_geometry geometry;

   // If disk isn't open, try to open it....
   if (!isOpen)
      OpenForRead();

   if (!ioctl(fd, HDIO_GETGEO, &geometry))
      numSecs = (uint32_t) geometry.sectors;
   #endif
   return numSecs;
} // DiskIO::GetNumSecsPerTrack()

// Resync disk caches so the OS uses the new partition table. This code varies
// a lot from one OS to another.
// Returns 1 on success, 0 if the kernel continues to use the old partition table.
// (Note that for most OSes, the default of 0 is returned because I've not yet
// looked into how to test for success in the underlying system calls...)
int DiskIO::DiskSync(void) {
   int i, retval = 0, platformFound = 0;

   // If disk isn't open, try to open it....
   if (!isOpen) {
      OpenForRead();
   } // if

   if (isOpen) {
      sync();
#if defined(__APPLE__) || defined(__sun__)
      cout << "Warning: The kernel may continue to use old or deleted partitions.\n"
           << "You should reboot or remove the drive.\n";
               /* don't know if this helps
               * it definitely will get things on disk though:
               * http://topiks.org/mac-os-x/0321278542/ch12lev1sec8.html */
#ifdef __sun__
      i = ioctl(fd, DKIOCFLUSHWRITECACHE);
#else
      i = ioctl(fd, DKIOCSYNCHRONIZECACHE);
#endif
      platformFound++;
#endif
#if defined (__FreeBSD__) || defined (__FreeBSD_kernel__)
      sleep(2);
      i = ioctl(fd, DIOCGFLUSH);
      cout << "Warning: The kernel may continue to use old or deleted partitions.\n"
           << "You should reboot or remove the drive.\n";
      platformFound++;
#endif
#ifdef __linux__
      sleep(1); // Theoretically unnecessary, but ioctl() fails sometimes if omitted....
      fsync(fd);
      i = ioctl(fd, BLKRRPART);
      if (i) {
         cout << "Warning: The kernel is still using the old partition table.\n"
              << "The new table will be used at the next reboot.\n";
      } else {
         retval = 1;
      } // if/else
      platformFound++;
#endif
      if (platformFound == 0)
         cerr << "Warning: Platform not recognized!\n";
      if (platformFound > 1)
         cerr << "\nWarning: We seem to be running on multiple platforms!\n";
   } // if (isOpen)
   return retval;
} // DiskIO::DiskSync()

// Seek to the specified sector. Returns 1 on success, 0 on failure.
// Note that seeking beyond the end of the file is NOT detected as a failure!
int DiskIO::Seek(uint64_t sector) {
   int retval = 1;
   off64_t seekTo, sought;

   // If disk isn't open, try to open it....
   if (!isOpen) {
      retval = OpenForRead();
   } // if

   if (isOpen) {
      seekTo = sector * (uint64_t) GetBlockSize();
      sought = lseek64(fd, seekTo, SEEK_SET);
      if (sought != seekTo) {
         retval = 0;
      } // if
   } // if
   return retval;
} // DiskIO::Seek()

// A variant on the standard read() function. Done to work around
// limitations in FreeBSD concerning the matching of the sector
// size with the number of bytes read.
// Returns the number of bytes read into buffer.
int DiskIO::Read(void* buffer, int numBytes) {
   int blockSize, numBlocks, retval = 0;
   char* tempSpace;

   // If disk isn't open, try to open it....
   if (!isOpen) {
      OpenForRead();
   } // if

   if (isOpen) {
      // Compute required space and allocate memory
      blockSize = GetBlockSize();
      if (numBytes <= blockSize) {
         numBlocks = 1;
         tempSpace = new char [blockSize];
      } else {
         numBlocks = numBytes / blockSize;
         if ((numBytes % blockSize) != 0)
            numBlocks++;
         tempSpace = new char [numBlocks * blockSize];
      } // if/else
      if (tempSpace == NULL) {
         cerr << "Unable to allocate memory in DiskIO::Read()! Terminating!\n";
         exit(1);
      } // if

      // Read the data into temporary space, then copy it to buffer
      retval = read(fd, tempSpace, numBlocks * blockSize);
      memcpy(buffer, tempSpace, numBytes);

      // Adjust the return value, if necessary....
      if (((numBlocks * blockSize) != numBytes) && (retval > 0))
         retval = numBytes;

      delete[] tempSpace;
   } // if (isOpen)
   return retval;
} // DiskIO::Read()

// A variant on the standard write() function. Done to work around
// limitations in FreeBSD concerning the matching of the sector
// size with the number of bytes read.
// Returns the number of bytes written.
int DiskIO::Write(void* buffer, int numBytes) {
   int blockSize = 512, i, numBlocks, retval = 0;
   char* tempSpace;

   // If disk isn't open, try to open it....
   if ((!isOpen) || (!openForWrite)) {
      OpenForWrite();
   } // if

   if (isOpen) {
      // Compute required space and allocate memory
      blockSize = GetBlockSize();
      if (numBytes <= blockSize) {
         numBlocks = 1;
         tempSpace = new char [blockSize];
      } else {
         numBlocks = numBytes / blockSize;
         if ((numBytes % blockSize) != 0) numBlocks++;
         tempSpace = new char [numBlocks * blockSize];
      } // if/else
      if (tempSpace == NULL) {
         cerr << "Unable to allocate memory in DiskIO::Write()! Terminating!\n";
         exit(1);
      } // if
      
      // Copy the data to my own buffer, then write it
      memcpy(tempSpace, buffer, numBytes);
      for (i = numBytes; i < numBlocks * blockSize; i++) {
         tempSpace[i] = 0;
      } // for
      retval = write(fd, tempSpace, numBlocks * blockSize);

      // Adjust the return value, if necessary....
      if (((numBlocks * blockSize) != numBytes) && (retval > 0))
         retval = numBytes;

      delete[] tempSpace;
   } // if (isOpen)
   return retval;
} // DiskIO:Write()

/**************************************************************************************
 *                                                                                    *
 * Below functions are lifted from various sources, as documented in comments before  *
 * each one.                                                                          *
 *                                                                                    *
 **************************************************************************************/

// The disksize function is taken from the Linux fdisk code and modified
// greatly since then to enable FreeBSD and MacOS support, as well as to
// return correct values for disk image files.
uint64_t DiskIO::DiskSize(int *err) {
   uint64_t sectors = 0; // size in sectors
   off_t bytes = 0; // size in bytes
   struct stat64 st;
   int platformFound = 0;
#ifdef __sun__
   struct dk_minfo minfo;
#endif

   // If disk isn't open, try to open it....
   if (!isOpen) {
      OpenForRead();
   } // if

   if (isOpen) {
      // Note to self: I recall testing a simplified version of
      // this code, similar to what's in the __APPLE__ block,
      // on Linux, but I had some problems. IIRC, it ran OK on 32-bit
      // systems but not on 64-bit. Keep this in mind in case of
      // 32/64-bit issues on MacOS....
#ifdef __APPLE__
      *err = ioctl(fd, DKIOCGETBLOCKCOUNT, &sectors);
      platformFound++;
#endif
#ifdef __sun__
      *err = ioctl(fd, DKIOCGMEDIAINFO, &minfo);
      if (*err == 0)
          sectors = minfo.dki_capacity;
      platformFound++;
#endif
#if defined (__FreeBSD__) || defined (__FreeBSD_kernel__)
      *err = ioctl(fd, DIOCGMEDIASIZE, &bytes);
      long long b = GetBlockSize();
      sectors = bytes / b;
      platformFound++;
#endif
#ifdef __linux__
      long sz;
      long long b;
      *err = ioctl(fd, BLKGETSIZE, &sz);
      if (*err) {
         sectors = sz = 0;
      } // if
      if ((!*err) || (errno == EFBIG)) {
         *err = ioctl(fd, BLKGETSIZE64, &b);
         if (*err || b == 0 || b == sz)
            sectors = sz;
         else
            sectors = (b >> 9);
      } // if
      // Unintuitively, the above returns values in 512-byte blocks, no
      // matter what the underlying device's block size. Correct for this....
      sectors /= (GetBlockSize() / 512);
      platformFound++;
#endif
      if (platformFound != 1)
         cerr << "Warning! We seem to be running on no known platform!\n";

      // The above methods have failed, so let's assume it's a regular
      // file (a QEMU image, dd backup, or what have you) and see what
      // fstat() gives us....
      if ((sectors == 0) || (*err == -1)) {
         if (fstat64(fd, &st) == 0) {
            bytes = st.st_size;
            if ((bytes % UINT64_C(512)) != 0)
               cerr << "Warning: File size is not a multiple of 512 bytes!"
                    << " Misbehavior is likely!\n\a";
            sectors = bytes / UINT64_C(512);
         } // if
      } // if
   } // if (isOpen)
   return sectors;
} // DiskIO::DiskSize()
