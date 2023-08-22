/* mbr.h -- MBR data structure definitions, types, and functions */

/* This program is copyright (c) 2009-2013 by Roderick W. Smith. It is distributed
  under the terms of the GNU GPL version 2, as detailed in the COPYING file. */

#include <stdint.h>
#include <sys/types.h>
#include "gptpart.h"
//#include "partnotes.h"
#include "diskio.h"
#include "basicmbr.h"

#ifndef __MBRSTRUCTS
#define __MBRSTRUCTS

using namespace std;

/****************************************
 *                                      *
 * MBRData class and related structures *
 *                                      *
 ****************************************/

// Full data in tweaked MBR format
class MBRData : public BasicMBRData {
protected:
public:
   MBRData(void) {}
   MBRData(string deviceFilename) : BasicMBRData(deviceFilename) {}
   MBRData & operator=(const BasicMBRData & orig);

   // Functions to create, delete, or change partitions
   // Pass EmptyMBR 1 to clear the boot loader code, 0 to leave it intact
   void MakeProtectiveMBR(int clearBoot = 0);
   void OptimizeEESize(void);
   int DeleteByLocation(uint64_t start64, uint64_t length64);

   // Functions to extract data on specific partitions....
   GPTPart AsGPT(int i);
}; // struct MBRData

#endif
