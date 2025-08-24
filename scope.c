/* scope.c -- implementation of module scope.
 * Author: Luis Colorado <luiscoloradourcola@gmail.com>
 * Date: Fri Jul  4 07:25:10 -05 2025
 * Copyright: (c) 2025 Luis Colorado.  All rights reserved.
 * License: BSD
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "scope.h"
#include "dynarray.h"
#include "hoc.tab.h"
#include "intern.h"

#ifndef   UQ_SCOPES_INCRMNT /* { */
#warning  UQ_SCOPES_INCRMNT should be set in 'config.mk'
#define   UQ_SCOPES_INCRMNT (8)
#endif /* UQ_SCOPES_INCRMNT    } */

static scope  *scopes         = NULL;
static size_t  scopes_len     = 0,
               scopes_cap     = 0;

static Symbol *current_symbol = NULL;

Symbol *get_current_symbol()
{
    return current_symbol;
} /* get_current_symbol */

scope *get_current_scope()
{
    if (scopes_len == 0) return NULL;
    return scopes + scopes_len - 1;
} /* get_current_scope */

scope *get_root_scope()
{
    if (scopes_len == 0) return NULL;
    return scopes;
} /* get_root_scope */

scope *start_scope()
{
    DYNARRAY_GROW(scopes, scope, 1, UQ_SCOPES_INCRMNT);
    scope *scop   = scopes + scopes_len++;
    scope *parent = scopes_len > 1
		? scop - 1
		: NULL;
    scop->sentinel = current_symbol;
    if (parent) {
        scop->base_offset = parent->base_offset + parent->size;
        scop->size        = 0;
    } else {
        scop->base_offset = scop->size = 0;
    }
    return scop;
} /* start_scope */

Symbol *end_scope()
{
    Symbol *ret_val = current_symbol;
    scope  *scop    = get_current_scope();
    assert(scop != NULL);
    for(    Symbol *sym = current_symbol;
            sym != NULL && sym != scop->sentinel;
            sym = sym->next)
    {
        assert(sym->type == LVAR
            || sym->type == MAIN_FUNCTION);
    }
    current_symbol = scop->sentinel;
	scopes_len--;

    return ret_val;
} /* end_scope */

static Symbol *
lookup_internal(
        const char *sym_name,
        Symbol     *sentl)
{
    Symbol *ret_val;

    sym_name = intern(sym_name);

    for (   ret_val = current_symbol;
            ret_val != NULL && ret_val != sentl;
            ret_val = ret_val->next)
    {
        if (ret_val->name == sym_name)
            return ret_val;
    }
    return NULL;
} /* lookup_internal */

Symbol *lookup_current_scope(const char *sym_name)
{
    scope *scop = get_current_scope();
    return lookup_internal(
            sym_name,
            scop ? scop->sentinel
                 : NULL);
} /* lookup_current_scope */

Symbol *lookup(const char *sym_name)
{
    return lookup_internal(
            sym_name,
            NULL);
} /* lookup */

Symbol *install(
        const char *sym_name,
        int         sym_type,
        Symbol     *lvar_type)
{
    sym_name = intern(sym_name);
    Symbol *ret_val = calloc(1, sizeof *ret_val);
    assert(ret_val != NULL);

    ret_val->name   = sym_name;
    ret_val->type   = sym_type;

    /* insertamos el simbolo en el scope */
    ret_val->next   = current_symbol;
    current_symbol  = ret_val;

    return ret_val;
} /* install */
