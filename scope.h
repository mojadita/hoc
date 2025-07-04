/* scope.h -- Scope object is needed to handle the value of the
 * base offset used for local variables in this scope.  Also,
 * navigating through scopes should be possible to push/pop
 * scopes as we are interpreting the code.
 * Scope is linked to it's parent scope, being the NULL scope
 * the final parent (this is, no scope at all) a new scope is
 * created on defining a new function.  A scope gives access
 * to its parent scope (in case there exists) and to the root
 * scope of this scope (the farthest parent before NULL)
 * The scope has a pointer to the last variable of the parent
 * scope, that is used to detect when we switch scopes up to
 * the root.
 * Global variables are created in no scope (the null scope)
 * and a new scope (with no parent) is created on entry to the
 * function/procedure definition, and a the parent and root
 * scopes are set for this scope.  On non-root scopes, the
 * parent scope is set to the parent scope (if it exists) and
 * the root scope is set to the present scope in case we are
 * the root.  A pointer to the last variable Symbol is stored
 * and a pointer to the last variable symbol is stored as a
 * sentinel to check where in the symbol list the current
 * scope ends.
 * A global function to lookup a symbol will use the last
 * registered symbol so it always be found if the current
 * symbol exists in the scope chain.
 * A global function to lookup a symbol in the current scope
 * only will be available.
 * A global function to push a new scope will return the
 * current scope (from which we can navigate the scope list
 * upto the root scope, or to go directly to the root scope
 * ---this scope is the main scope in the function/procedure
 * being defined---)
 * A global function to pop a scope will be used when the
 * scope ends.  This will free all symbols related to the
 * current scope upto the parent scope.
 *
 * Author: Luis Colorado <luiscoloradourcola@gmail.com>
 * Date: Fri Jul  4 07:07:08 -05 2025
 * Copyright: (c) 2025 Edward Rivas & Luis Colorado.  All
 *       rights reserved.
 * License: BSD
 */
#ifndef SCOPE_H
#define SCOPE_H

typedef struct scope_s scope;

#include "code.h"
#include "symbol.h"

scope  *get_current_scope(void);
scope  *get_root_scope(void);

scope  *push_scope(void);
void    pop_scope(void);

Symbol *lookup(
		const char *sym_name);

Symbol *lookup_current_scope(
		const char *sym_name);

Symbol *install(
		const char *sym_name,
		int         sym_type);

#endif /* SCOPE_H */
