/*
    Este modulo es comun para:
       hoc.y
        hoc-sin-prec.y
*/

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "colors.h"
#include "do_help.h"

#include "hoc.h"
#include "code.h"

#ifndef UQ_MAIN_DEBUG
#warning UQ_MAIN_DEBUG deberia ser configurado en config.mk
#define UQ_MAIN_DEBUG  0
#endif

#if UQ_MAIN_DEBUG
#define P(_fmt, ...) \
    printf("%s:%d:%s: "_fmt, __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#else
#define P(_fmt, ...)
#endif

char *progname;     /* for error messages */
int   lineno = 1;   /* numero de linea */

int parse(void)
{
    P(BRIGHT CYAN "BEGIN" ANSI_END "\n");
    int res = yyparse();
    P(BRIGHT CYAN "END" ANSI_END "\n");
    return res;
} /* parse */

void do_help(void)
{
    printf(
        "Uso: %s [ opts ] [ file ... ]\n"
        "Where opts are:\n"
        "  -h  this help screen\n",
        progname);
    exit(EXIT_SUCCESS);
} /* do_help */

static void process(FILE *in);

int main(int argc, char *argv[]) /* hoc1 */
{
    progname = argv[0];
    int opt;
    while ((opt = getopt(argc, argv, "hv")) != EOF) {
        switch (opt) {
        case 'h': do_help();
        case 'v': do_version(EXIT_SUCCESS);
        }
    } /* while */

    argc -= optind;
    argv += optind;

    init();
    if (argc) {
        for (int i = 0; i < argc; i++) {
            if (strcmp(argv[i], "-") == 0) {
                yysetFILE(stdin);
                process(stdin);
            } else {
                FILE *f = yysetfilename(argv[i]);
                if (f) {
                    process(f);
                    fclose(f);
                }
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
        execute(progbase);
        P("Stack size after execution: %d\n", stacksize());
    }
} /* process */
