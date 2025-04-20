/* init.c -- funciones de inicializacion.
 * Author: Edward Rivas <rivastkw@gmail.com>
 *       y Luis Colorado <luiscoloradourcola@gmail.com>
 * Date: Sat Dec 28 14:06:39 -05 2024
 * Copyright: (c) 2025 Edward Rivas y Luis Colorado.  All rights reserved.
 * License: BSD.
 */

#include <stdlib.h>
#include <math.h>

#include "config.h"
#include "hoc.h"
#include "hoc.tab.h"
#include "math.h"
#include "code.h"

double integer(double x);
double Rand(void);

static struct { /* constants */
    char *name;
    double cval;
} consts[] = {
    "DEG",     180.0/M_PI,
    "E",       M_E,
    "PHI",     1.61803398874989484820,
    "PI",      M_PI,
    "version", UQ_VERSION,
    NULL,      0.0,
};

static struct builtin { /* built-ins-1 */
    char *name;
    double (*func)();
    int  type;
    const char *help;
} builtins[] = {
    "abs",   fabs,      BLTIN1, "abs(x)",
    "acos",  acos,      BLTIN1, "acos(x)",
    "asin",  asin,      BLTIN1, "asin(x)",
    "atan",  atan,      BLTIN1, "atan(x)",
    "atan2", atan2,     BLTIN2, "atan2(y,x)",
    "cos",   cos,       BLTIN1, "cos(x)",
    "exp",   exp,       BLTIN1, "exp(x)",
    "int",   integer,   BLTIN1, "int(x)",
    "inv",   inverso,   BLTIN1, "inv(x)",
    "log",   log,       BLTIN1, "log(x)",
    "log10", log10,     BLTIN1, "log10(x)",
    "ops",   opuesto,   BLTIN1, "ops(x)",
    "pow",   Pow,       BLTIN2, "pow(x,y)",
    "rand",  Rand,      BLTIN0, "rand()",
    "read",  rd,        BLTIN0, "read()", 
    "sin",   sin,       BLTIN1, "sin(x)",
    "sqrt",  Sqrt,      BLTIN1, "sqrt(x)",
    "tan",   tan,       BLTIN1, "tan(x)",
    "time",  now,       BLTIN0, "time()",
    NULL,    NULL,
};

void init(void)  /* install constants and built-ins in table */
{
    int i;
    Symbol *s;

    for (i = 0; consts[i].name != NULL; i++) {
        Symbol *s = install(consts[i].name, CONST, NULL);
        s->val = consts[i].cval;
    }

    for (   struct builtin *p = builtins;
            p->name;
            p++)
    {
        s = install(p->name, p->type, p->help);
        switch(p->type) {
            case BLTIN0: s->ptr0 = p->func; break;
            case BLTIN1: s->ptr1 = p->func; break;
            case BLTIN2: s->ptr2 = p->func; break;
        }
    }

    /* creamos el simbolo prev */
    Symbol *prev = install("prev", UNDEF, NULL);
}

double integer(double x)
{
    return (int) x;
}
