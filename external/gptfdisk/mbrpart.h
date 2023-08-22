/*
    MBRPart class, part of GPT fdisk program family.
    Copyright (C) 2011  Roderick W. Smith

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

#ifndef MBRPART_H
#define MBRPART_H

#include <stdint.h>

#define MAX_HEADS 255        /* numbered 0 - 254 */
#define MAX_SECSPERTRACK 63  /* numbered 1 - 63 */
#define MAX_CYLINDERS 1024   /* numbered 0 - 1023 */

#define NONE 0    /* don't include partition when writing */
#define PRIMARY 1 /* write partition as primary */
#define LOGICAL 2 /* write partition as logical */
#define EBR 4     /* sector is used as an EBR or MBR */
#define INVALID 8 /* sector number is too large for disk */

using namespace std;

// Data for a single MBR partition record
// Note that firstSector and lastSector are in CHS addressing, which
// splits the bits up in a weird way.
// On read or write of MBR entries, firstLBA is an absolute disk sector.
// On read of logical entries, it's relative to the EBR record for that
// partition. When writing EBR records, it's relative to the extended
// partition's start.
#pragma pack(1)
struct MBRRecord {
   uint8_t status;
   uint8_t firstSector[3];
   uint8_t partitionType;
   uint8_t lastSector[3];
   uint32_t firstLBA; // see above
   uint32_t lengthLBA;
}; // struct MBRRecord

class MBRPart {
protected:
   uint8_t status;
   uint8_t firstSector[3];
   uint8_t partitionType;
   uint8_t lastSector[3];
   uint32_t firstLBA; // see above
   uint32_t lengthLBA;
   int includeAs; // PRIMARY, LOGICAL, or NONE
   int canBeLogical;
   int canBePrimary;
   static uint32_t numHeads;
   static uint32_t numSecspTrack;
   static uint64_t diskSize;
   static uint32_t blockSize;
   static int numInstances;

public:
    MBRPart();
    MBRPart(const MBRPart& other);
    virtual ~MBRPart();
    virtual MBRPart& operator=(const MBRPart& orig);
    virtual MBRPart& operator=(const struct MBRRecord& orig);
    bool operator<(const MBRPart &other) const;

    // Set information on partitions or disks...
    void SetGeometry(uint32_t heads, uint32_t sectors, uint64_t ds, uint32_t bs);
    void Empty(void);
    void SetStartLBA(uint64_t s);
    void SetLengthLBA(uint64_t l);
    void SetLocation(uint64_t start, uint64_t length);
    int SetType(uint8_t typeCode, int isExtended = 0);
    void SetStatus(uint8_t s) {status = s;}
    void SetInclusion(int status = PRIMARY) {includeAs = status;}
    void SetCanBeLogical(int c) {canBeLogical = c;}
    void SetCanBePrimary(int c) {canBePrimary = c;}
    void StoreInStruct(struct MBRRecord *theStruct);

    // Get information on partitions or disk....
    uint8_t GetType(void) {return partitionType;}
    uint8_t GetStatus(void) {return status;}
    uint64_t GetStartLBA(void) {return firstLBA;}
    uint64_t GetLengthLBA(void) {return lengthLBA;}
    uint64_t GetLastLBA(void) const;
    uint8_t GetInclusion(void) {return includeAs;}
    int CanBeLogical(void) {return canBeLogical;}
    int CanBePrimary(void) {return canBePrimary;}
    int DoTheyOverlap (const MBRPart& other);

    // Adjust information on partitions or disks...
    int RecomputeCHS(void);
    int LBAtoCHS(uint32_t lba, uint8_t * chs);
    void ReverseByteOrder(void);

    // User I/O...
    void ShowData(int isGpt);
}; // MBRPart

#endif // MBRPART_H
