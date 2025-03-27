/* do_help.c -- help routine.
 * Author: Luis Colorado <luis.colorado@spindrive.fi>
 * Date: Fri Nov  8 12:05:30 EET 2024
 * Copyright: (c) 2024 SpinDrive Oy, FI.  All rights reserved.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "config.h"
#include "do_help.h"

#if 0 /* {{ */
#include "log.h"
#include "execute_program.h"


void
do_help(int exit_code)
{
    char *argv[] = { "man", PROGRAM_NAME, NULL };

    int res = EXECUTE_PROGRAM(MAN_CMD, "man", PROGRAM_NAME);

    if (res == 0) {
        /* we had the man command and it worked. */
        exit(exit_code);
    }

    /* something wrong with the manual page.
     * Try the help file. */
    FILE *fmt_manpage = fopen(HELP_FILE_PATH, "rt");
    if (fmt_manpage) {
        /* copy formatted man file to stdout */
        int c;
        while ((c = fgetc(fmt_manpage)) != EOF) {
            putchar(c);
        }
        fclose(fmt_manpage);
        exit(exit_code);
    }

    /* no help file? */
    WRN("fopen: "HELP_FILE_PATH": %s\n",
            strerror(errno));

    /* nothing works... print a brief, default message */
    printf( "Usage: "PROGRAM_NAME" [-h]\n");

    exit(exit_code);
} /* do_help */

#endif /* }} */

void do_version(int cod)
{

#define PD(V)      printf("%24s %g\n", #V, V)
#define PS(V)      printf("%24s %s\n", #V, V)
#define P(V)       printf("%24s %2d\n", #V, V)

    printf("%s, %s: (C) %s %s.  All rights reserved\n\n",
            PROGRAM_NAME, VERSION, COPYRIGHT_YEARS,
            AUTHOR_CORP);

    PS(PROGRAM_NAME);
    PS(BUILD_DATE);
    PS(DOC_DATE);
    PS(PACKAGE);
    PS(AUTHOR_NAME);
    PS(AUTHOR_EMAIL);
    PS(COPYRIGHT_YEARS);
    PS(PROGRAM_NAME_UC);
    PS(AUTHOR_CORP);
    PS(AUTHOR_SITE);
    PS(VERSION);
    PS(VERSION_DATE);
    PS(OPERATING_SYSTEM);
    PS(prefix);
    PS(exec_prefix);
    PS(bindir);
    PS(sbindir);
    PS(datarootdir);
    PS(pkgdatadir);
    PS(mandir);
    PS(man8dir);
    PS(docdir);
    PS(vardir);
    PS(logdir);

    P(UQ_HOC_DEBUG);
    P(UQ_HOC_TRACE_PATCHING);
    P(UQ_LEX_DEBUG);
    P(UQ_USE_COLORS);
    P(UQ_USE_LOCUS);
    P(UQ_USE_DEB);
    P(UQ_USE_INF);
    P(UQ_USE_WRN);
    P(UQ_USE_ERR);
    P(UQ_USE_CRT);
    P(UQ_LAST_TOKENS_SIZE);
    //P(UQ_DEFAULT_LOGLEVEL);
    PD(UQ_VERSION);

    exit(cod);
#undef PS
#undef P
} /* do_version */
