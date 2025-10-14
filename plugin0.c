/* plugin0.c -- Sample test plugin to run in hoc.
 * Author: Edward Rivas <rivastkw@gmail.com>
 * Date: Tue Oct 14 06:01:58 -05 2025
 * Copyright: (c) 2025 Edward Rivas.  All rights reserved.
 * License: BSD
 */

#include <stdio.h>
#include <math.h>

#include "builtins.h"
#include "symbol.h"
#include "instr.h"
#include "types.h"
#include "code.h"
#include "cellP.h"

void sinh_cb(const instr *i)
{
    double par    = pop().val;
    Cell   result = { .val = sinh(par) };
    push(result);
}

void cosh_cb(const instr *i)
{
    double par    = pop().val;
    Cell   result = { .val = cosh(par) };
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
    fprintf(stderr, "Test plugin v1.0\n");
    int res = register_builtin(
            "sinh", Double, sinh_cb,
            "x",    Double,
            NULL);

    if (res < 0) {
        fprintf(stderr, "Error al registrar la funcion double sinh(double x).\n");
    }
    fprintf(stderr, "double sinh(double x) registered ok.\n");

    res = register_builtin(
            "cosh", Double, cosh_cb,
            "x",    Double,
            NULL);

    if (res < 0) {
        fprintf(stderr, "Error al registrar la funcion double cosh(double x).\n");
    }
    fprintf(stderr, "double cosh(double x) registered ok.\n");

    return 0;
}
