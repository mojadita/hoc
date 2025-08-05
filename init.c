/* init.c -- funciones de inicializacion.
 * Author: Edward Rivas <rivastkw@gmail.com>
 *       y Luis Colorado <luiscoloradourcola@gmail.com>
 * Date: Sat Dec 28 14:06:39 -05 2024
 * Copyright: (c) 2025 Edward Rivas y Luis Colorado.  All rights reserved.
 * License: BSD.
 */

#include <assert.h>
#include <stdlib.h>
#include <math.h>

#include "config.h"
#include "hoc.h"
#include "hoc.tab.h"
#include "math.h"
#include "code.h"
#include "scope.h"

double integer(double x);
double Rand(void);

static struct constant { /* constants */
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
    "intg",  integer,   BLTIN1, "intg(x)",
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

static struct predefined_types { /* predefined types */
    char *name;
    int   size;
} builtin_types [] = {
    "char",   1,
    "int",    1,
    "long",   1,
    "float",  1,
    "double", 1,
    "string", 1,
    NULL,     0,
};

void init(void)  /* install constants and built-ins in table */
{

    /* vamos con los tipos */
    for ( struct predefined_types *p = builtin_types;
            p->name;
            p++)
    {
        Symbol *s = install(p->name, TYPE, NULL);
        s->size = p->size;
    }

    Symbol *D = lookup("double");
    assert(D->type == TYPE);

    for (   struct builtin *p = builtins;
            p->name;
            p++)
    {
        Symbol *s = install(p->name, p->type, NULL);
        s->help = p->help;
        s->typref = D;
        switch(p->type) {
            case BLTIN0: s->ptr0 = p->func; break;
            case BLTIN1: s->ptr1 = p->func; break;
            case BLTIN2: s->ptr2 = p->func; break;
        }
    }

    for (   struct constant *p = consts;
            p->name != NULL;
            p++)
    {
        Symbol *s = install(p->name, CONST, NULL);
        s->typref = D;
        s->val = p->cval;
    }

    /* creamos el simbolo prev */
    register_global_var("prev", D);
}
