/* error.c -- rutinas de impresion de errores
 * Date: Tue Dec 31 16:19:19 -05 2024
 */

#include <stdarg.h>
#include <stdio.h>
#include <setjmp.h>

#include "hoc.h"
#include "error.h"

void execerror(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vwarning(fmt, args);
    va_end(args);
    longjmp(begin, 0);
} /* execerror */

void warning(const char *fmt, ...)    /* print warning message */
{
    va_list args;

    va_start(args, fmt);
    vwarning(fmt, args);
    va_end(args);
} /* warning */

/* print warning message */
void vwarning(const char *fmt, va_list args)
{
    /*
    fprintf(stderr,  "%s:", progname);
    vfprintf(stderr, fmt, args);
    fprintf(stderr,  " cerca de la linea %d\n", lineno);
    */
    printf("\033[1;37;33m%s:\033[0m", progname);
    vprintf(fmt, args);
    printf(" \033[1;33mcerca de la linea %d\033[0m\n", lineno);
} /* vwarning */
