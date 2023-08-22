/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef ANDROID_POPT_H
#define ANDROID_POPT_H

/*
 * popt has been deprecated for some time, and is replaced by GNOME's glib
 * option parser. Instead of pulling in either of those dependencies, this
 * stub implements just enough of popt to get things working.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#define POPT_ARG_NONE		 0U
#define POPT_ARG_STRING		 1U
#define POPT_ARG_INT		 2U

#define POPT_AUTOHELP

#pragma pack(push)
#pragma pack(0)

struct poptOption {
    const char *longName;
    char shortName;
    unsigned int argInfo;
    void *arg;
    int val;
    const char *descrip;
    const char *argDescrip;
};

struct _poptContext {
    int argc;
    const char **argv;
    const struct poptOption *options;
    struct option *long_options;
    const char *otherHelp;
};

typedef struct _poptContext *poptContext;

#pragma pack(pop)

poptContext poptGetContext(const char *name, int argc, const char **argv,
        const struct poptOption *options, unsigned int flags);
poptContext poptFreeContext(poptContext con);
void poptResetContext(poptContext con);

void poptSetOtherOptionHelp(poptContext con, const char *text);
void poptPrintUsage(poptContext con, FILE *fp, int flags);

int poptGetNextOpt(poptContext con);
const char *poptGetArg(poptContext con);

#endif
