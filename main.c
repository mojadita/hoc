/* main.c -- Codigo principal de hoc.
 * Author: Edward Rivas <rivastkw@gmail.com>
 *       y Luis Colorado <luiscoloradourcola@gmail.com>
 * Date: Thu Apr 17 13:08:32 EEST 2025
 * Copyright: (c) 2025 Luis Colorado y Edward Rivas.  All rights reserved.
 * License: BSD
 */

#include <fcntl.h>
#include <dlfcn.h>
#include <dirent.h>
#include <getopt.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "colors.h"
#include "do_help.h"

#include "hoc.h"
#include "code.h"
#include "init.h"

#ifndef   UQ_CODE_DEBUG_EXEC /* { */
#warning  UQ_CODE_DEBUG_EXEC deberia ser configurado en config.mk
#define   UQ_CODE_DEBUG_EXEC  0
#endif /* UQ_CODE_DEBUG_EXEC    } */

#if       UQ_CODE_DEBUG_EXEC /* {{ */
# define EXEC(_fmt, ...)     \
    printf(F(_fmt), ##__VA_ARGS__)
# define P_TAIL(_fmt, ...)   \
    printf(F(_fmt), ##__VA_ARGS__)
#else  /* UQ_CODE_DEBUG_EXEC    }{ */
# define EXEC(_fmt, ...)
# define P_TAIL(_fmt, ...)
#endif /* UQ_CODE_DEBUG_EXEC    }} */

char *progname;     /* for error messages */

int parse(void)
{
    EXEC(BRIGHT CYAN "BEGIN" ANSI_END "\n");
    int res = yyparse();
    EXEC(BRIGHT CYAN "END" ANSI_END "\n");
    return res;
} /* parse */

void do_help(int exit_code)
{
    printf(
        "Uso: %s [ opts ] [ file ... ]\n"
        "Where opts are:\n"
        "  -h  this help screen\n",
        progname);
    exit(exit_code);
} /* do_help */

static void process(FILE *in);
void init_plugins(void);

int main(int argc, char *argv[]) /* hoc1 */
{
    progname = argv[0];
    setbuf(stdout, NULL);
    int opt;
    while ((opt = getopt(argc, argv, "hv")) != EOF) {
        switch (opt) {
        case 'h': do_help(EXIT_SUCCESS);
        case 'v': do_version(EXIT_SUCCESS);
        }
    } /* while */

    argc -= optind;
    argv += optind;

    init();

    init_plugins();

    if (argc) {
        for (int i = 0; i < argc; i++) {
            if (strcmp(argv[i], "-") == 0) {
                yysetFILE(stdin);
                process(stdin);
            } else {
                FILE *f = yysetfilename(argv[i]);
                if (!f) {
                    fprintf(stderr, "%s: %s\n",
                        argv[i],
                        strerror(errno));
                    exit(EXIT_FAILURE);
                }
                process(f);
                fclose(f);
            }
        }
    } else {
        process(stdin);
    }
    return EXIT_SUCCESS;
} /* main */

static void process(FILE *in)
{
    setjmp(begin);
    for (initcode(); parse(); initcode()) {
        /* EDW: Mon Sep  8 11:35:06 -05 2025
         *
         * La funcion initexec() debe ponerse aqui, porque de ser llamada
         * antes ejecutar antes de ejecutar execute().
         * initexec() inicializa sp y fp para ejecutar el codigo.
         * initcode() inicializa progp para preparar la memoria
         * para generar codigo.
         */
        initexec();
        execute(progbase);
        EXEC("Stack size after execution: %d\n", stacksize());
    }
} /* process */

void init_plugins(void)
{
    DIR *d = opendir(pkglibdir);
    if (d == NULL) {
        fprintf(stderr,
                "directorio %s: %s\n",
                pkglibdir,
                strerror(errno));
        return;
    }

    struct dirent *file;
    int dir_fd = dirfd(d);

    while ((file = readdir(d)) != NULL) {

            if (       !strcmp(file->d_name, "..")
                    || !strcmp(file->d_name, "."))
                continue;

            char plugin_name[1024];
            snprintf(plugin_name, sizeof plugin_name,
                     pkglibdir "/%s", file->d_name);

            void *plugin_so = dlopen(plugin_name, RTLD_LAZY);
            if (plugin_so == NULL) {
                fprintf(stderr, "dlopen %s: %s\n",
                    plugin_name,
                    dlerror());
                continue;
            }
    }
} /* init_plugins */
