/* error.c -- rutinas de impresion de errores
 * Author: Edward Rivas <rivastkw@gmail.com>
 *       y Luis Colorado <luiscoloradourcola@gmail.com>
 * Date: Tue Dec 31 16:19:19 -05 2024
 * Copyright: (c) 2025 Edward Rivas y Luis Colorado.  All rights reserved.
 * License: BSD.
 */

#include <stdarg.h>
#include <stdio.h>
#include <setjmp.h>

#include "config.h"
#include "colors.h"
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
    printf(BRIGHT YELLOW "\n%s: " ANSI_END, progname);
    vprintf(fmt, args);
    printf(" " BRIGHT YELLOW "cerca de la linea %d" ANSI_END "\n", lineno);
} /* vwarning */

void defnonly(int cual, const char *name, ...)
{
    char buffer[1000];
    va_list args;
    va_start(args, name);
    vsnprintf(buffer, sizeof buffer, name, args);
    va_end(args);

    if (!cual) {
        execerror("'%s': debe usarse dentro de una definicion proc/func.\n",
                  buffer);
    }
} /* defnonly */
