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
#include "cell.h"
#include "code.h"
#include "scope.h"
#include "type2inst.h"
#include "types.h"
#include "init.h"

double integer(double x);
double Rand(void);

static struct constant { /* constants */
    char *name;
    double cval;
} consts[] = {
    { "DEG",     180.0/M_PI, },
    { "E",       M_E, },
    { "PHI",     1.61803398874989484820, },
    { "PI",      M_PI, },
    { "version", UQ_VERSION, },
    { NULL,      0.0, },
};

static struct builtin { /* built-ins-1 */
    char *name;
    double (*func)();
    int  type;
    const char *help;
} builtins[] = {
    { "abs",   fabs,      BLTIN1, "abs(x)", },
    { "acos",  acos,      BLTIN1, "acos(x)", },
    { "asin",  asin,      BLTIN1, "asin(x)", },
    { "atan",  atan,      BLTIN1, "atan(x)", },
    { "atan2", atan2,     BLTIN2, "atan2(y,x)", },
    { "cos",   cos,       BLTIN1, "cos(x)", },
    { "exp",   exp,       BLTIN1, "exp(x)", },
    { "intg",  integer,   BLTIN1, "intg(x)", },
    { "inv",   inverso,   BLTIN1, "inv(x)", },
    { "log",   log,       BLTIN1, "log(x)", },
    { "log10", log10,     BLTIN1, "log10(x)", },
    { "ops",   opuesto,   BLTIN1, "ops(x)", },
    { "pow",   Pow,       BLTIN2, "pow(x,y)", },
    { "rand",  Rand,      BLTIN0, "rand()", },
    { "read",  rd,        BLTIN0, "read()", },
    { "sin",   sin,       BLTIN1, "sin(x)", },
    { "sqrt",  Sqrt,      BLTIN1, "sqrt(x)", },
    { "tan",   tan,       BLTIN1, "tan(x)", },
    { "time",  now,       BLTIN0, "time()", },
    { NULL,    NULL, },
};


const Symbol
       *Char,
       *Double,
       *Float,
       *Integer,
       *Long,
       *Short,
       *String,
       *Prev;

Cell one_c = { .chr  = 1    },
     one_d = { .val  = 1.0  },
     one_f = { .flt  = 1.0F },
     one_i = { .inum = 1    },
     one_l = { .num  = 1L   },
     one_s = { .sht  = 1    };


static struct predefined_types { /* predefined types */
    char             *name;
    int               size,
                      align,
                      weight;
    const Symbol    **sym_ref;
    const Cell       *one;     /* pre and post increment */
    const type2inst  *t2i;     /* select instruction
                                * for type mapping */
    const char       *fmt;     /* format string */
} builtin_types [] = {
    { .name    = "char",
      .size    = 1,
      .align   = 1,
      .one     = &one_c,
      .weight  = 0,
      .sym_ref = &Char,
      .t2i     = &t2i_c,
      .fmt     = FMT_CHAR,
    }, {
      .name    = "int",
      .size    = 1,
      .align   = 1,
      .one     = &one_d,
      .weight  = 1,
      .sym_ref = &Integer,
      .t2i     = &t2i_i,
      .fmt     = FMT_INT,
    }, {
      .name    = "long",
      .size    = 1,
      .align   = 1,
      .one     = &one_f,
      .weight  = 2,
      .sym_ref = &Long,
      .t2i     = &t2i_l,
      .fmt     = FMT_LONG,
    }, {
      .name    = "float",
      .size    = 1,
      .align   = 1,
      .one     = &one_i,
      .weight  = 3,
      .sym_ref = &Float,
      .t2i     = &t2i_f,
      .fmt     = FMT_FLOAT,
    }, {
      .name    = "double",
      .size    = 1,
      .align   = 1,
      .one     = &one_l,
      .weight  = 4,
      .sym_ref = &Double,
      .t2i     = &t2i_d,
      .fmt     = FMT_DOUBLE,
    }, {
      .name    = "string",
      .size    = 1,
      .align   = 1,
      .one     = &one_s,
      .weight  = -1,
      .sym_ref = &String,
      .t2i     = NULL,
      .fmt     = "%s",
    }, {
      .name    = NULL,
    },
};

void init(void)  /* install constants and built-ins in table */
{

    /* vamos con los tipos */

    for ( struct predefined_types *p = builtin_types;
            p->name;
            p++)
    {
        Symbol *s   = install(p->name, TYPE, NULL);
        s->size     = p->size;
        s->weight   = p->weight;
        s->t2i      = p->t2i;
        s->one      = p->one;
        *p->sym_ref = s;
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
    Prev = register_global_var("prev", D);
}
