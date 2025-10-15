/* plugin0.c -- Sample test plugin to run in hoc.
 * Author: Edward Rivas <rivastkw@gmail.com>
 * Date: Tue Oct 14 06:01:58 -05 2025
 * Copyright: (c) 2025 Edward Rivas.  All rights reserved.
 * License: BSD
 */

#include <stdio.h>
#include <math.h>

#include "plugins.h"

void sinh_cb(int plugin_id)
{
    double par    = pop().val;
    Cell   result = { .val = sinh(par) };
    push(result);
}

void cosh_cb(int plugin_id)
{
    double par    = pop().val;
    Cell   result = { .val = cosh(par) };
    push(result);
}

void tanh_cb(int plugin_id)
{
    double par    = pop().val;
    Cell   result = { .val = tanh(par) };
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
    register_builtin("sinh", Double, sinh_cb, "x", Double, NULL);
    register_builtin("cosh", Double, cosh_cb, "x", Double, NULL);
    register_builtin("tanh", Double, tanh_cb, "x", Double, NULL);

    return 0;
}
