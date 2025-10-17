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
#include "cellP.h"
#include "code.h"
#include "scope.h"
#include "type2inst.h"
#include "types.h"
#include "init.h"
#include "symbolP.h"

double integer(double x);
double Rand(void);

static struct constant { /* constants */
    char *name;
    Cell  cval;
} consts[] = {
    { "DEG",     { .val = 180.0/M_PI }, },
    { "E",       { .val = M_E }, },
    { "PHI",     { .val = 1.61803398874989484820 }, },
    { "PI",      { .val = M_PI }, },
    { "version", { .val = UQ_VERSION }, },
    { NULL,      0.0, },
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
    { .name = "string", .sym_ref = &String, .t2i = &t2i_str, },
    { .name = "char",   .sym_ref = &Char,   .t2i = &t2i_c, },
    { .name = "short",  .sym_ref = &Short,  .t2i = &t2i_s, },
    { .name = "int",    .sym_ref = &Integer,.t2i = &t2i_i, },
    { .name = "long",   .sym_ref = &Long,   .t2i = &t2i_l, },
    { .name = "float",  .sym_ref = &Float,  .t2i = &t2i_f, },
    { .name = "double", .sym_ref = &Double, .t2i = &t2i_d, },
    { .name = NULL, },
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

    for (   struct constant *p = consts;
            p->name != NULL;
            p++)
    {
        Symbol *s = install(p->name, CONST, NULL);
        s->typref = D;
        s->cel    = p->cval;
    }

    /* creamos el simbolo prev */
    Prev = register_global_var("prev", D);
}
