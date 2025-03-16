/*
    Este modulo es comun para:
       hoc.y
        hoc-sin-prec.y
*/

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hoc.h"
#include "code.h"

char *progname;     /* for error messages */
int   lineno = 1;   /* numero de linea */

int parse(void)
{
    printf("%s:%d:%s \033[1;36mBEGIN\033[m\n", __FILE__, __LINE__, __func__);
    int res = yyparse();
    printf("%s:%d:%s \033[1;36mEND\033[m\n", __FILE__, __LINE__, __func__);
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
    while ((opt = getopt(argc, argv, "h")) != EOF) {
        switch (opt) {
        case 'h': do_help();
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
        printf("Stack size after execution: %d\n", stacksize());
    }
} /* process */
