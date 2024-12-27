/* symbol.c -- tabla de symbolos.
 * Date: Fri Dec 27 15:16:22 -05 2024
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "hoc.h"

static Symbol *lista_simbolos = NULL;

Symbol *install(
		const char *name,
		sym_type    typ,
		double      val,
		double    (*ptr)(double))
{
	Symbol *ret_val = malloc(sizeof *ret_val);
	assert(ret_val != NULL);
	ret_val->name   = name;
	ret_val->type   = typ;

	switch (typ) {
		case VAR:   ret_val->u.val = val; break;
		case BLTIN: ret_val->u.ptr = ptr; break;
		case UNDEF: ret_val->u.val = 0.0; break;
	}
	ret_val->next = lista_simbolos;
	lista_simbolos = ret_val;

	return ret_val;
} /* install */

Symbol *lookup(
		const char *name)
{
	for (Symbol *p = lista_simbolos; p != NULL; p = p->next) {
		if (strcmp(name, p->name) == 0)
			return p;
	}
	/* p == NULL */
	return NULL;
} /* lookup */
