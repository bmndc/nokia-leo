/* basicmbr.h -- MBR data structure definitions, types, and functions */

/* This program is copyright (c) 2009-2013 by Roderick W. Smith. It is distributed
  under the terms of the GNU GPL version 2, as detailed in the COPYING file. */

#include <stdint.h>
#include <sys/types.h>
#include "diskio.h"
#include "mbrpart.h"

#ifndef __BASICMBRSTRUCTS
#define __BASICMBRSTRUCTS

#define MBR_SIGNATURE UINT16_C(0xAA55)

// Maximum number of MBR partitions
#define MAX_MBR_PARTS 128

using namespace std;

/****************************************
 *                                      *
 * MBRData class and related structures *
 *                                      *
 ****************************************/

// A 512-byte data structure into which the MBR can be loaded in one
// go. Also used when loading logical partitions.
#pragma pack(1)
struct TempMBR {
   uint8_t code[440];
   uint32_t diskSignature;
   uint16_t nulls;
   struct MBRRecord partitions[4];
   uint16_t MBRSignature;
}; // struct TempMBR

// Possible states of the MBR
enum MBRValidity {invalid, gpt, hybrid, mbr};

// Full data in tweaked MBR format
class BasicMBRData {
protected:
   uint8_t code[440];
   uint32_t diskSignature;
   uint16_t nulls;
   // MAX_MBR_PARTS defaults to 128. This array holds both the primary and
   // the logical partitions, to simplify data retrieval for GPT conversions.
   MBRPart partitions[MAX_MBR_PARTS];
   uint16_t MBRSignature;

   // Above are basic MBR data; now add more stuff....
   uint32_t blockSize; // block size (usually 512)
   uint64_t diskSize; // size in blocks
   uint32_t numHeads; // number of heads, in CHS scheme
   uint32_t numSecspTrack; // number of sectors per track, in CHS scheme
   DiskIO* myDisk;
   int canDeleteMyDisk;
   string device;
   MBRValidity state;
   MBRPart* GetPartition(int i); // Return primary or logical partition
public:
   BasicMBRData(void);
   BasicMBRData(string deviceFilename);
   ~BasicMBRData(void);
   BasicMBRData & operator=(const BasicMBRData & orig);

   // File I/O functions...
   int ReadMBRData(const string & deviceFilename);
   int ReadMBRData(DiskIO * theDisk, int checkBlockSize = 1);
   int ReadLogicalParts(uint64_t extendedStart, int partNum);
   int WriteMBRData(void);
   int WriteMBRData(DiskIO *theDisk);
   int WriteMBRData(const string & deviceFilename);
   int WriteMBRData(struct TempMBR & mbr, DiskIO *theDisk, uint64_t sector);
   void DiskSync(void) {myDisk->DiskSync();}
   void SetDisk(DiskIO *theDisk);

   // Display data for user...
   void DisplayMBRData(void);
   void ShowState(void);

   // GPT checks and fixes...
   int CheckForGPT(void);
   int BlankGPTData(void);

   // Functions that set or get disk metadata (size, CHS geometry, etc.)
   void SetDiskSize(uint64_t ds) {diskSize = ds;}
   void SetBlockSize(uint32_t bs) {blockSize = bs;}
   MBRValidity GetValidity(void) {return state;}
   void SetHybrid(void) {state = hybrid;} // Set hybrid flag
   void ReadCHSGeom(void);
   int GetPartRange(uint32_t* low, uint32_t* high);
   int LBAtoCHS(uint64_t lba, uint8_t * chs); // Convert LBA to CHS
   int FindOverlaps(void);
   int NumPrimaries(void);
   int NumLogicals(void);
   int CountParts(void);
   void UpdateCanBeLogical(void);
   uint64_t FirstLogicalLBA(void);
   uint64_t LastLogicalLBA(void);
   int AreLogicalsContiguous(void);
   int DoTheyFit(void);
   int SpaceBeforeAllLogicals(void);
   int IsLegal(void);
   int IsEEActive(void);
   int FindNextInUse(int start);

   // Functions to create, delete, or change partitions
   // Pass EmptyMBR 1 to clear the boot loader code, 0 to leave it intact
   void EmptyMBR(int clearBootloader = 1);
   void EmptyBootloader(void);
   void AddPart(int num, const MBRPart& newPart);
   void MakePart(int num, uint64_t startLBA, uint64_t lengthLBA, int type = 0x07,
                 int bootable = 0);
   int SetPartType(int num, int type);
   int SetPartBootable(int num, int bootable = 1);
   int MakeBiggestPart(int i, int type); // Make partition filling most space
   void DeletePartition(int i);
   int SetInclusionwChecks(int num, int inclStatus);
   void RecomputeCHS(int partNum);
   void SortMBR(int start = 0);
   int DeleteOversizedParts();
   int DeleteExtendedParts();
   void OmitOverlaps(void);
   void MaximizeLogicals();
   void MaximizePrimaries();
   void TrimPrimaries();
   void MakeLogicalsContiguous(void);
   void MakeItLegal(void);
   int RemoveLogicalsFromFirstFour(void);
   int MovePrimariesToFirstFour(void);
   int CreateExtended(void);

   // Functions to find information on free space....
   uint64_t FindFirstAvailable(uint64_t start = 1);
   uint64_t FindLastInFree(uint64_t start);
   uint64_t FindFirstInFree(uint64_t start);
   int SectorUsedAs(uint64_t sector, int topPartNum = MAX_MBR_PARTS);

   // Functions to extract data on specific partitions....
   uint8_t GetStatus(int i);
   uint8_t GetType(int i);
   uint64_t GetFirstSector(int i);
   uint64_t GetLength(int i);

   // User interaction functions....
   int DoMenu(const string& prompt = "\nMBR command (? for help): ");
   void ShowCommands(void);
}; // class BasicMBRData

#endif
