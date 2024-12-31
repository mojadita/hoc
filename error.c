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
    fprintf(stderr,  "%s:", progname);
    vfprintf(stderr, fmt, args);
    fprintf(stderr,  " cerca de la linea %d\n", lineno);
} /* vwarning */
