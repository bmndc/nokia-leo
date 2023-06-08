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

#include <popt.h>

// #define LOCAL_DEBUG

/*
 * popt has been deprecated for some time, and is replaced by GNOME's glib
 * option parser. Instead of pulling in either of those dependencies, this
 * stub implements just enough of popt to get things working.
 */

poptContext poptGetContext(const char *name, int argc, const char **argv,
        const struct poptOption *options, unsigned int flags) {
    // Convert into getopt format, sanity checking our limited
    // capabilities along the way
    int count = 0;
    for (; options[count].longName; count++) {
    }

    struct option *long_options = (struct option *)
            calloc(count, sizeof(struct option));
    for (int i = 0; options[i].longName; i++) {
        long_options[i].name = options[i].longName;
        long_options[i].flag = 0;

        if (!options[i].val) {
            fprintf(stderr, __FILE__ ": val required\n");
            abort();
        }
        long_options[i].val = options[i].val;

        switch (options[i].argInfo) {
        case POPT_ARG_NONE:
            long_options[i].has_arg = no_argument;
            break;
        case POPT_ARG_STRING:
        case POPT_ARG_INT:
            if (!options[i].arg) {
                fprintf(stderr, __FILE__ ": arg required\n");
                abort();
            }
            long_options[i].has_arg = required_argument;
            break;
        default:
            fprintf(stderr, __FILE__ ": unsupported argInfo\n");
            abort();
        }
    }

    poptContext con = (poptContext) calloc(1, sizeof(struct _poptContext));
    con->argc = argc;
    con->argv = argv;
    con->options = options;
    con->long_options = long_options;
    return con;
}

poptContext poptFreeContext(poptContext con) {
    free(con->long_options);
    free(con);
    return 0;
}

void poptResetContext(poptContext con) {
    optind = 1;
}

void poptSetOtherOptionHelp(poptContext con, const char *text) {
    con->otherHelp = text;
}

void poptPrintUsage(poptContext con, FILE *fp, int flags) {
    fprintf(fp, "USAGE: %s %s\n", con->argv[0], con->otherHelp);
    int i = 0;
    for (; con->options[i].longName; i++) {
        fprintf(fp, "\t--%s\t%s\n", con->options[i].longName,
                con->options[i].descrip);
    }
    fprintf(fp, "\n");
}

int poptGetNextOpt(poptContext con) {
    int i = -1;
    int res = getopt_long(con->argc, (char *const *) con->argv, "",
            con->long_options, &i);
#ifdef LOCAL_DEBUG
    fprintf(stderr, "getopt_long()=%c\n", res);
#endif
    if (res == 0 || res == '?') {
        return -1;
    }

    // Copy over found argument value
    switch (con->options[i].argInfo) {
    case POPT_ARG_STRING:
        *((char**) con->options[i].arg) = strdup(optarg);
        break;
    case POPT_ARG_INT:
        *((int*) con->options[i].arg) = atoi(optarg);
        break;
    }

    return res;
}

const char *poptGetArg(poptContext con) {
    const char *res = con->argv[optind++];
#ifdef LOCAL_DEBUG
    fprintf(stderr, "poptGetArg()=%s\n", res);
#endif
    return res;
}
