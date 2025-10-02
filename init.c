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


static struct predefined_types { /* predefined types */
    char             *name;
    const Symbol    **sym_ref;
    const type2inst  *t2i;     /* select instruction
                                * for type mapping */
    const char       *fmt;     /* format string */
} builtin_types [] = {
    /* LCU: Tue Sep 30 11:35:26 -05 2025
     * Los tipos de esta tabla estan ordenados por pesos,
     * a fin de calcular que operando debe ser promocionado
     * a la hora de usarlo con un operador.  Si bien se
     * puede cambiar el orden, es mejor no hacerlo para
     * evitar errores al renumerar los tipos en caso de
     * tener que hacer una insercion. */
    { .name    = "string",
      .sym_ref = &String,
      .t2i     = &t2i_str,
    }, {
      .name    = "char",
      .sym_ref = &Char,
      .t2i     = &t2i_c,
    }, {
      .name    = "short",
      .sym_ref = &Short,
      .t2i     = &t2i_s,
    }, {
      .name    = "int",
      .sym_ref = &Integer,
      .t2i     = &t2i_i,
    }, {
      .name    = "long",
      .sym_ref = &Long,
      .t2i     = &t2i_l,
    }, {
      .name    = "float",
      .sym_ref = &Float,
      .t2i     = &t2i_f,
    }, {
      .name    = "double",
      .sym_ref = &Double,
      .t2i     = &t2i_d,
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
        s->t2i      = p->t2i;
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
