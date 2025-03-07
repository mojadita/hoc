/*
    Este modulo es comun para:
	   hoc.y
	    hoc-sin-prec.y
*/

#include <stdio.h>
#include <stdlib.h>

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

int main(int argc, char *argv[]) /* hoc1 */
{
    progname = argv[0];
    init();
    setjmp(begin);
    for (initcode(); parse(); initcode()) {
        execute(prog);
        printf("Stack size after execution: %d\n", stacksize());
    }
    return EXIT_SUCCESS;
} /* main */
