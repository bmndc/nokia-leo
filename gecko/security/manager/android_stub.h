/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* This file allows NSS to build by stubbing out
 * features that aren't provided by Android/Bionic */

#ifndef ANDROID_STUB_H
#define ANDROID_STUB_H

/* sysinfo is defined but not implemented.
 * we may be able to implement it ourselves. */
#define _SYS_SYSINFO_H_

#include <sys/cdefs.h>
#include <sys/resource.h>
#include <linux/kernel.h>
#include <unistd.h>
#include <dlfcn.h>

/* Use this stub version of getdtablesize
 * instead of the one in the header */
__attribute__((unused))
static int getdtablesize_stub(void)
{
    struct rlimit r;
    if (getrlimit(RLIMIT_NOFILE, &r) < 0) {
        return sysconf(_SC_OPEN_MAX);
    }
    return r.rlim_cur;
}
#define getdtablesize() getdtablesize_stub()

#ifndef RTLD_NOLOAD
#define RTLD_NOLOAD 0
#endif

#define sysinfo(foo) -1

#endif /* ANDROID_STUB_H */
