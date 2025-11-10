/* builtins.c -- Gestion de funciones builtin.
 * Author: Edward Rivas <rivastkw@gmail.com>
 * Date: Mon Oct 13 10:49:48 -05 2025
 * Copyright: (c) 2025 Edward Rivas.  All rights reserved.
 * License: BSD
 */

#include <stdarg.h>
#include <stdbool.h>

#include "config.h"
#include "colors.h"
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
        const char     *name,
        const Symbol   *type,
        bltin_cb        function_ref,
        bltin_const_cb  const_function_ref,
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
    bltin->sym->bltin_index = ret_val;
    bltin->subr             = function_ref;
    bltin->subr_eval        = const_function_ref;

    va_start(args, const_function_ref);

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

ConstExpr
eval_const_builtin_func(
        int                  id,
        const ConstArglist  *args)
{
    const builtin *bltin = get_builtin_info(id);
    const Symbol  *sym   = bltin->sym;
    const Symbol  *typ   = sym->typref;
    printf(F("%s %s("), typ->name, sym->name);
    char *sep = "";
    char workbench[256];
    ConstExpr *p = args->expr_list; /* parameter */
    for (int i = 0; i < args->expr_list_len; ++i, ++p) {
        const Symbol *p_typ = p->typ;
        printf("%s(%s) %s",
                sep,
                p_typ->name,
                p_typ->t2i->printval(
                        p->cel,
                        workbench,
                        sizeof workbench));
        sep = ", ";
    } /* for */
    printf(") -> ");

    /* LCU: Sun Nov  9 13:48:28 -05 2025
     * TODO: llamar a function builtin (evaluada, no programada) */

    if (bltin->subr_eval == NULL) {
        execerror("builtin " GREEN "%s" ANSI_END " cannot be used in "
                  "a constant expression",
                  sym->name);
    }
    ConstExpr ret_val = bltin->subr_eval(sym->bltin_index, args);
    puts(ret_val.typ->t2i->printval(
                   ret_val.cel,
                   workbench,
                   sizeof workbench));
    return ret_val;
} /* eval_const_builtin_fun */

const builtin *get_builtin_info(int id)
{
    assert(id >= 0 && id < builtins_len);
    return &builtins[id];
} /* get_builtin_info */
