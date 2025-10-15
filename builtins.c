/* builtins.c -- Gestion de funciones builtin.
 * Author: Edward Rivas <rivastkw@gmail.com>
 * Date: Mon Oct 13 10:49:48 -05 2025
 * Copyright: (c) 2025 Edward Rivas.  All rights reserved.
 * License: BSD
 */

#include <stdarg.h>
#include <stdbool.h>

#include "config.h"
#include "dynarray.h"
#include "hoc.h"
#include "scope.h"
#include "symbolP.h"

#include "builtinsP.h"

#ifndef   UQ_BUILTINS_INCRMNT /* { */
#warning  UQ_BUILTINS_INCRMNT should be included in config.mk
#define   UQ_BUILTINS_INCRMNT  (10)
#endif /* UQ_BUILTINS_INCRMNT    } */

static builtin *builtins;
static size_t   builtins_len,
                builtins_cap;

int
register_builtin(
        const char   *name,
        const Symbol *type,
        bltin_cb      function_ref,
        ...)
{
    va_list args;

    DYNARRAY_GROW(
            builtins,
            builtin *,
            1,
            UQ_BUILTINS_INCRMNT);

    int      ret_val   = builtins_len;
    builtin *bltin     = builtins + builtins_len++;
    bool is_func       = (type != NULL);

    bltin->sym = install(
            name,
            is_func
                ? BLTIN_FUNC
                : BLTIN_PROC,
            type);

    bltin->subr             = function_ref;
    bltin->sym->bltin_index = ret_val;

    va_start(args, function_ref);

    const Symbol *par_type;
    const char   *par_name;

    start_scope();
    while ((par_name = va_arg(args, const char *)) != NULL) {
        par_type     = va_arg(args, const Symbol *);

        DYNARRAY_GROW(bltin->sym->argums,
                      Symbol *,
                      1,
                      UQ_ARGUMS_INCRMNT);

        Symbol *lpar = register_local_var(par_name, par_type);
        assert(lpar != NULL);

        bltin->sym->argums[bltin->sym->argums_len++] = lpar;
        bltin->sym->size_args += par_type->t2i->size;
    }
    end_scope();

    /* SECOND PASS TO ADJUST PARAMETER OFFSETS */
    for (int i = 0; i < bltin->sym->argums_len; ++i) {
        bltin->sym->argums[i]->offset += bltin->sym->size_args;
    }

    if (is_func) {
        bltin->sym->ret_val_offset = bltin->sym->size_args;
    }

    return ret_val;

} /* register_builtin */
