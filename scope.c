/* scope.c -- implementation of module scope.
 * Author: Luis Colorado <luiscoloradourcola@gmail.com>
 * Date: Fri Jul  4 07:25:10 -05 2025
 * Copyright: (c) 2025 Luis Colorado.  All rights reserved.
 * License: BSD
 */

#include <assert.h>
#include <stdlib.h>

#include "scope.h"
#include "dynarray.h"

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
    scope *scop = scopes + scopes_len++;
    scope *prnt = scopes_len > 1 ? s-1 : NULL; /* prnt == parent, NO PRINT!!! */
    scop->sentinel = current_symbol;
    if (prnt) {
        scop->base_offset = prnt->base_offset;
        scop->max_offset  = prnt->max_offset;
    } else {
        scop->base_offset = scop->max_offset = 0;
    }
    return scop;
} /* start_scope */

void end_scope()
{
    scope *sc = get_current_scope();
    assert(sc != NULL);
    for(    Symbol *sym = current_symbol;
            sym != NULL && sym != sc->sentinel;
            sym = current_symbol)
    {
        current_symbol = sym->next;
        assert(current_symbol->type == LVAR);
        free(sym);
    }
} /* end_scope */

static Symbol *lookup_internal(const char *sym_name, Symbol *sentl)
{
    Symbol *ret_val;

    for (   ret_val = current_symbol;
            ret_val != NULL && ret_val != sentl
            ret_val = ret_val->next)
    {
        if (ret_val->name == sym_name)
            return ret_val;
    }
    return NULL;
} /* lookup_internal */

Symbol *lookup_current_scope(const char *sym_name)
{
    scope *sc = get_current_scope();
    return lookup_internal(
            sym_name,
            sc  ? sc->sentinel
                : NULL);
} /* lookup_current_scope */

Symbol *lookup(const char *sym_name)
{
    return lookup_internal(
            sym_name,
            NULL);
} /* lookup */

Symbol *install(const char *sym_name, int sym_type)
{
    Symbol *ret_val = calloc(1, sizeof *ret_val);
    assert(ret_val != NULL);

    ret_val->name   = sym_name;
    ret_val->type   = sym_type;
    ret_val->next   = current_symbol;
    current_symbol  = ret_val;

    return ret_val;
} /* install */

int scope_calculate_offset(Symbol *type)
{
    scope *s = get_current_scope();
    s->base_offset -= type->size;
    if (s->base_offset < s->max_offset)
        s->max_offset = s->base_offset;
    return s->base_offset;
} /* scope_calculate_offset */
