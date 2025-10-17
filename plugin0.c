/* plugin0.c -- Sample test plugin to run in hoc.
 * Author: Edward Rivas <rivastkw@gmail.com>
 * Date: Tue Oct 14 06:01:58 -05 2025
 * Copyright: (c) 2025 Edward Rivas.  All rights reserved.
 * License: BSD
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "plugins.h"

/* ONE PARAMETER FUNCTIONS */

#define DOUBLE_F( /*                     { */\
        _rfld, /* result field */            \
        _name, /* builtin name */            \
        _func) /* funct to call */           \
void _name##_cb(int plugin_id)               \
{                                            \
    double par    = pop().val;               \
    Cell   result = { ._rfld = _func(par) }; \
    push(result);                            \
} /* _name##_cb                          }{ */

DOUBLE_F(val, abs,   fabs)
DOUBLE_F(val, acos,  acos)
DOUBLE_F(val, acosh, acosh)
DOUBLE_F(val, asin,  asin)
DOUBLE_F(val, asinh, asinh)
DOUBLE_F(val, atan,  atan)
DOUBLE_F(val, atanh, atanh)
DOUBLE_F(val, cos,   cos)
DOUBLE_F(val, cosh,  cosh)
DOUBLE_F(val, exp,   exp)
DOUBLE_F(val, inv,   1.0/)
DOUBLE_F(val, log,   log)
DOUBLE_F(val, log10, log10)
DOUBLE_F(val, ops,   - )
DOUBLE_F(val, sin,  sin)
DOUBLE_F(val, sinh, sinh)
DOUBLE_F(val, sqrt, sqrt)
DOUBLE_F(val, tan,  tan)
DOUBLE_F(val, tanh, tanh)

#undef DOUBLE_F /*                       } */

/* TWO PARAMETER FUNCTIONS */
#define DOUBLE_F2(_name, _expr) /*       { */\
void _name##_cb(int plugin_id)              \
{                                           \
    double    x = pop().val,                \
              y = pop().val;                \
    Cell result = { .val  = _name _expr };  \
                                            \
    push(result); /*                     }{ */\
} /* _name##_cb */

void mod_cb(int plugin_id)
{
    double y = pop().val,
           x = pop().val;
    int    i = x / y;
    Cell   result = { .val = x - i * y };

    push(result);
} /* mod_cb */


DOUBLE_F2(atan2, (y, x))
DOUBLE_F2(pow,   (x, y))

#undef DOUBLE_F2 /*                      } */

/* NO PARAMETER FUNCTIONS */

void random_cb(int plugin_id)
{
    Cell result = { .num = random() };
    push(result);
}

void time_cb(int plugin_id)
{
    Cell result = { .num = time(NULL) };
    push(result);
}

/* La rutina que dlopen() ejecuta automaticamente se llama
 * _init, pero es necesario enlazar el .so llamando al
 * linker ld(1) directamente, para que no cargue el modulo
 * crti0.o (que contiene un _init()).
 * Esto no afecta al modulo, que incluso sera mas peque;o,
 * de hecho. Esto se hace asi en el Makefile ahora. */
int _init()
{

#define REGISTER_BUILTIN(_ret_type, _name, ...) \
    register_builtin(#_name, _ret_type, _name##_cb, ##__VA_ARGS__, NULL, NULL)

    REGISTER_BUILTIN(Double,  abs,   "x", Double);
    REGISTER_BUILTIN(Double,  acos,  "x", Double);
    REGISTER_BUILTIN(Double,  acosh, "x", Double);
    REGISTER_BUILTIN(Double,  asin,  "x", Double);
    REGISTER_BUILTIN(Double,  asinh, "x", Double);
    REGISTER_BUILTIN(Double,  atan,  "x", Double);
    REGISTER_BUILTIN(Double,  atan2, "y", Double, "x", Double);
    REGISTER_BUILTIN(Double,  atanh, "x", Double);
    REGISTER_BUILTIN(Double,  cos,   "x", Double);
    REGISTER_BUILTIN(Double,  cosh,  "x", Double);
    REGISTER_BUILTIN(Double,  exp,   "x", Double);
    REGISTER_BUILTIN(Double,  inv,   "x", Double);
    REGISTER_BUILTIN(Double,  log,   "x", Double);
    REGISTER_BUILTIN(Double,  log10, "x", Double);
    REGISTER_BUILTIN(Double,  mod,   "x", Double, "y", Double);
    REGISTER_BUILTIN(Double,  ops,   "x", Double);
    REGISTER_BUILTIN(Double,  pow,   "x", Double, "y", Double);
    REGISTER_BUILTIN(Long,    random);
    REGISTER_BUILTIN(Double,  sin,   "x", Double);
    REGISTER_BUILTIN(Double,  sinh,  "x", Double);
    REGISTER_BUILTIN(Double,  sqrt,  "x", Double);
    REGISTER_BUILTIN(Double,  tan,   "x", Double);
    REGISTER_BUILTIN(Double,  tanh,  "x", Double);
    REGISTER_BUILTIN(Long,    time);

    return 0;
} /* _init() */
