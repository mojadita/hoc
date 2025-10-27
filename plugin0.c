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

/* this macro expands to a generic builtin call of
 * one parameter (of any type, expressed by the _rfld
 * macro parameter) and one result (of the same type)
 * and calls the function internally, extracting the
 * parameter from the hoc stack, and pushing in the
 * stack the result value.
 * Example:
 * DOUBLE_F(dbl, sin, sin) -->
 * void sin_cb(int plugin_id)
 * {
 *     Cell par = pop();
 *     Cell result = { .dbl = sin(par.dbl) };  <-- result is calculated here
 *     push(result);
 * } / * sin_cb * /
 */
#define DOUBLE_F( /*                     { */      \
        _rfld, /* result field */                  \
        _name, /* builtin name */                  \
        _func) /* funct to call */                 \
void _name##_cb(int plugin_id)                     \
{                                                  \
    Cell   par    = pop();                         \
    Cell   result = { ._rfld = _func(par._rfld) }; \
    push(result);                                  \
} /* _name##_cb                          }{ */

DOUBLE_F(dbl, abs,   fabs)
DOUBLE_F(dbl, acos,  acos)
DOUBLE_F(dbl, acosh, acosh)
DOUBLE_F(dbl, asin,  asin)
DOUBLE_F(dbl, asinh, asinh)
DOUBLE_F(dbl, atan,  atan)
DOUBLE_F(dbl, atanh, atanh)
DOUBLE_F(dbl, cos,   cos)
DOUBLE_F(dbl, cosh,  cosh)
DOUBLE_F(dbl, exp,   exp)
DOUBLE_F(dbl, inv,   1.0/)
DOUBLE_F(dbl, log,   log)
DOUBLE_F(dbl, log10, log10)
DOUBLE_F(dbl, ops,   - )
DOUBLE_F(dbl, sin,  sin)
DOUBLE_F(dbl, sinh, sinh)
DOUBLE_F(dbl, sqrt, sqrt)
DOUBLE_F(dbl, tan,  tan)
DOUBLE_F(dbl, tanh, tanh)

#undef DOUBLE_F /*                       } */

/* TWO PARAMETER FUNCTIONS */
#define DOUBLE_F2(_name, _expr) /*       { */\
void _name##_cb(int plugin_id)              \
{                                           \
    double    x = pop().dbl,                \
              y = pop().dbl;                \
    Cell result = { .dbl  = _name _expr };  \
                                            \
    push(result); /*                     }{ */\
} /* _name##_cb */

DOUBLE_F2(atan2, (y, x))
DOUBLE_F2(pow,   (x, y))
DOUBLE_F2(fmod,  (y, x))

#undef DOUBLE_F2 /*                      } */

/* NO PARAMETER FUNCTIONS */

void random_cb(int plugin_id)
{
    Cell result = { .lng = random() };
    push(result);
}

void time_cb(int plugin_id)
{
    Cell result = { .lng = time(NULL) };
    push(result);
}

void exit_cb(int plugin_id)
{
    exit(pop().itg);
}

void srandom_cb(int plugin_id)
{
    srandom(pop().itg);
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
    REGISTER_BUILTIN(Double,  fmod,  "y", Double, "x", Double);
    REGISTER_BUILTIN(Double,  ops,   "x", Double);
    REGISTER_BUILTIN(Double,  pow,   "x", Double, "y", Double);
    REGISTER_BUILTIN(Long,    random);
    REGISTER_BUILTIN(Double,  sin,   "x", Double);
    REGISTER_BUILTIN(Double,  sinh,  "x", Double);
    REGISTER_BUILTIN(Double,  sqrt,  "x", Double);
    REGISTER_BUILTIN(NULL,    srandom, "x", Integer);
    REGISTER_BUILTIN(Double,  tan,   "x", Double);
    REGISTER_BUILTIN(Double,  tanh,  "x", Double);
    REGISTER_BUILTIN(Long,    time);
    REGISTER_BUILTIN(NULL,    exit, "x", Integer);

    return 0;
} /* _init() */
