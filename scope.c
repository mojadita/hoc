/* scope.c -- implementation of module scope.
 * Author: Luis Colorado <luiscoloradourcola@gmail.com>
 * Date: Fri Jul  4 07:25:10 -05 2025
 * Copyright: (c) 2025 Luis Colorado.  All rights reserved.
 * License: BSD
 */

#include <assert.h>
#include <stdlib.h>

#include "scope.h"

struct scope_s {
	scope  *parent;
	Symbol *sentinel;
	int     base_offset;
	int     max_offset;
};

static scope  *current_scope  = NULL;
static scope  *root_scope     = NULL;
static Symbol *current_symbol = NULL;

scope *get_current_scope()
{
	return current_scope;
} /* get_current_scope */

scope *get_root_scope()
{
	return root_scope;
} /* get_root_scope */

scope *push_scope()
{
	scope *new_scope = malloc(sizeof *new_scope);
	assert(new_scope != NULL);

	new_scope->parent = current_scope;
	new_scope->sentinel     = current_symbol;
	new_scope->base_offset  = current_scope
						    ? current_scope-> base_offset
						    : 0 ;
	new_scope->max_offset   = new_scope->base_offset;
	/* link it */
	current_scope = new_scope;
	if (!root_scope) root_scope = new_scope;

	return new_scope;
} /* push_scope */

void pop_scope()
{
	assert(current_scope != NULL);
	scope *to_delete = current_scope;
	scope *parent = to_delete->parent;
	current_scope = parent;

    /* LCU: Fri Jul  4 14:41:39 -05 2025
	 * TODO: seguir aqui.
	 */
} /* pop_scope */
