/* This program is copyright (c) 2009-2013 by Roderick W. Smith. It is distributed
  under the terms of the GNU GPL version 2, as detailed in the COPYING file. */

#include <stdint.h>
#include <stdlib.h>
#include <string>

#ifndef __GPTSUPPORT
#define __GPTSUPPORT

#define GPTFDISK_VERSION "0.8.10.2"

#if defined (__FreeBSD__) || defined (__FreeBSD_kernel__) || defined (__APPLE__)
// Darwin (Mac OS) & FreeBSD: disk IOCTLs are different, and there is no lseek64
#include <sys/disk.h>
#define lseek64 lseek
#endif

#if defined (__FreeBSD__) || defined (__FreeBSD_kernel__)
#define DEFAULT_GPT_TYPE 0xA503
#endif

#ifdef __APPLE__
#define DEFAULT_GPT_TYPE 0xAF00
#endif

#ifdef _WIN32
#define DEFAULT_GPT_TYPE 0x0700
#endif

#ifdef __sun__
#define DEFAULT_GPT_TYPE 0xbf01
#endif

// Microsoft Visual C++ only
#if defined (_MSC_VER)
#define sscanf sscanf_s
#define strcpy strcpy_s
#define sprintf sprintf_s
#endif

// Linux only....
#ifdef __linux__
#include <linux/fs.h>
#define DEFAULT_GPT_TYPE 0x8300
#endif

#ifndef DEFAULT_GPT_TYPE
#define DEFAULT_GPT_TYPE 0x8300
#endif

// Set this as a default
#define SECTOR_SIZE UINT32_C(512)

// Signatures for Apple (APM) disks, multiplied by 0x100000000
#define APM_SIGNATURE1 UINT64_C(0x00004D5000000000)
#define APM_SIGNATURE2 UINT64_C(0x0000535400000000)

/**************************
 * Some GPT constants.... *
 **************************/

#define GPT_SIGNATURE UINT64_C(0x5452415020494645)

// Number and size of GPT entries...
#define NUM_GPT_ENTRIES 128
#define GPT_SIZE 128
#define HEADER_SIZE UINT32_C(92)
#define GPT_RESERVED 420
#define NAME_SIZE 36 // GPT allows 36 UTF-16LE code units for a name in a 128 byte partition entry

using namespace std;

string ReadString(void);
int GetNumber(int low, int high, int def, const string & prompt);
char GetYN(void);
uint64_t GetSectorNum(uint64_t low, uint64_t high, uint64_t def, uint64_t sSize, const std::string& prompt);
uint64_t IeeeToInt(string IeeeValue, uint64_t sSize, uint64_t low, uint64_t high, uint64_t def = 0);
string BytesToIeee(uint64_t size, uint32_t sectorSize);
unsigned char StrToHex(const string & input, unsigned int position);
int IsHex(string input); // Returns 1 if input can be hexadecimal number....
int IsLittleEndian(void); // Returns 1 if CPU is little-endian, 0 if it's big-endian
void ReverseBytes(void* theValue, int numBytes); // Reverses byte-order of theValue
void WinWarning(void);

#endif
