// sgdisk.cc
// Command-line-based version of gdisk. This program is named after sfdisk,
// and it can serve a similar role (easily scripted, etc.), but it's used
// strictly via command-line arguments, and it doesn't bear much resemblance
// to sfdisk in actual use.
//
// by Rod Smith, project began February 2009; sgdisk begun January 2010.

/* This program is copyright (c) 2009-2011 by Roderick W. Smith. It is distributed
  under the terms of the GNU GPL version 2, as detailed in the COPYING file. */

#include <iostream>
#include <fstream>
#include <string.h>
#include <string>
#include <iostream>
#include <sstream>
#include <errno.h>
#include "gptcl.h"
#include <fcntl.h>

using namespace std;

#define MAX_OPTIONS 50

/*
 * Dump partition details in a machine readable format:
 *
 * DISK [mbr|gpt] [guid]
 * PART [n] [type] [guid]
 */
static int android_dump(char* device) {
    BasicMBRData mbrData;
    GPTData gptData;
    GPTPart partData;
    int numParts = 0;
    stringstream res;

    /* Silence noisy underlying library */
    int stdout = dup(STDOUT_FILENO);
    int silence = open("/dev/null", 0);
    dup2(silence, STDOUT_FILENO);
    dup2(silence, STDERR_FILENO);

    if (!mbrData.ReadMBRData((string) device)) {
        cerr << "Failed to read MBR" << endl;
        return 8;
    }

    switch (mbrData.GetValidity()) {
    case mbr:
        res << "DISK mbr" << endl;
        for (int i = 0; i < MAX_MBR_PARTS; i++) {
            if (mbrData.GetLength(i) > 0) {
                res << "PART " << (i + 1) << " " << hex
                        << (int) mbrData.GetType(i) << dec << endl;
            }
        }
        break;
    case gpt:
        gptData.JustLooking();
        if (!gptData.LoadPartitions((string) device)) {
            cerr << "Failed to read GPT" << endl;
            return 9;
        }

        res << "DISK gpt " << gptData.GetDiskGUID() << endl;
        numParts = gptData.GetNumParts();
        for (int i = 0; i < numParts; i++) {
            partData = gptData[i];
            if (partData.GetFirstLBA() > 0) {
                res << "PART " << (i + 1) << " " << partData.GetType() << " "
                        << partData.GetUniqueGUID() << " "
                        << partData.GetDescription() << endl;
            }
        }
        break;
    default:
        cerr << "Unknown partition table" << endl;
        return 10;
    }

    /* Write our actual output */
    string resString = res.str();
    write(stdout, resString.c_str(), resString.length());
    return 0;
}

int main(int argc, char *argv[]) {
    for (int i = 0; i < argc; i++) {
        if (!strcmp("--android-dump", argv[i])) {
            return android_dump(argv[i + 1]);
        }
    }

    GPTDataCL theGPT;
    return theGPT.DoOptions(argc, argv);
}
