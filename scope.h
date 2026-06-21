/* scope.h -- Scope object.
 * Author: Luis Colorado <luiscoloradourcola@gmail.com>
 *       & Edward Rivas <rivastkw@gmail.com>
 * Date: Fri Jul  4 07:07:08 -05 2025
 * Copyright: (c) 2025-2026 Luis Colorado.  All rights reserved.
 * License: BSD
 *
 * Scope is needed to handle the value of the
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
 *
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
 *
 * A global function to lookup a symbol will use the last
 * registered symbol so it always be found if the current
 * symbol exists in the scope chain.
 *
 * A global function to lookup a symbol in the current scope
 * only will be available.
 *
 * A global function to push a new scope will return the
 * current scope (from which we can navigate the scope list
 * upto the root scope, or to go directly to the root scope
 * ---this scope is the main scope in the function/procedure
 * being defined---)
 *
 * A global function to pop a scope will be used when the
 * scope ends.  This will free all symbols related to the
 * current scope upto the parent scope.
 */
#ifndef SCOPE_H_ac8e7764_acea_11f0_b273_0023ae68f329
#define SCOPE_H_ac8e7764_acea_11f0_b273_0023ae68f329

/* Symbol table is managed as a linked list of Symbol,
 * chained through a pointer in the Symbol struct (.next)
 * The Symbol objects can only be added to the list,
 * and no provision has been taken to erase them once
 * used.  This allows to make them accessible while the
 * program is running, as the instruction symbs_all allows
 * to get a pointer to the context so all the available
 * symbols in that context can be accessed and printed.
 * The new symbols are inserted to the beginning of the
 * context list, so the most recent ones are the first
 * found in the chain, making local context better accessed
 * than parent ones. */

typedef struct scope_s scope;

#include "code.h"
#include "symbol.h"

struct scope_s {
    Symbol *sentinel;    /* This symbol marks the start
						  * of the next context. */
    int     base_offset; /* offset base for this
						  * scope.  Local variables add
						  * to stack offsets starting
						  * at this offset. */
    int     size;        /* scope size.  This is
						  * computed as the file is
						  * being parsed, and so, it
						  * registers the amount of
						  * space used by local variables
						  * at end of parsing the routine. */
}; /* struct scope_s */

/**
 * @brief gets the first symbol found in the scope (the
 *        one defined most recently.
 * @return the reference to the most recent Symbol or
 *         NULL if no symbol with that name is found.
 *         This only happens in compilation time.
 */
Symbol *get_current_symbol();

/**
 * @brief gets the current scope.
 *
 * @return a reference to the current scope.
 */
scope  *get_current_scope(void);

/**
 * @brief gets the root scope.
 *
 * The root scope is a predefined scope in which
 * builtins and special variables (like prev) are
 * defined.
 *
 * @return the refernec to the most depthly defined
 *         subroutine scope, or NULL if we are not
 *         in a function/procedure definition scope.
 */
scope  *get_root_scope(void);

/**
 * @brief creates a new scope for local symbols.
 * @return a reference to the scope just created.
 */
scope  *start_scope(void);

/**
 * @brief Gets the offset of a variable of type
 *        'type' in the current scope.
 * @param type is the Symbol associated to the
 *             local variable/parameter.
 */
int scope_calculate_offset(Symbol *type);

/**
 * @brief Destroys the current scope.
 * @return the symbol defined last in the current scope.
 *         This scope is not deleted, but unlinked from
 *         the symbol table visibility.  It is still usable
 *         (e.g. in the symbs_all instruction to get the
 *         scope of variables to be consulted)
 */
Symbol *end_scope(void);

/**
 * @brief Locates a Symbol in the symbol table.
 * @param sym_name is the string that represents the
 *        Symbol's name.  It must be a previously internalized
 *        string, as no allocation is provided from the symbol
 *        table for symbol names. 
 * @return the Symbol found or NULL if it's not in the table.
 */
Symbol *lookup(
        const char *sym_name);

/**
 * @brief searches a Symbol in the symbol table.
 *
 * The search reduces to the current scope, so the function
 * stops searching when the next scope is found.  The reason
 * is to locate an already defined symbol in the current scope
 * before redefining it with another definition.
 * @param sym_name is the name of the symbol.
 * @return the Symbol found or NULL if there's no such reference
 *         in the current scope.
 */
Symbol *lookup_current_scope(
        const char *sym_name);

/**
 * @brief Installs a new symbol in the Symbol table.
 *
 * @param name the name of the new Symbol.  Must be
 *             previously internalized.  See intern().
 * @param typ is the Symbol type.
 * @param val value to assigne to the new symbol.
 * @param ptr pointer to the function that calculates
 *            the value of the expression when this
 *            symbol is selected.
 * @return a reference to the new Symbol created. */
Symbol *install(
        const char   *sym_name,
        int           sym_type,
        const Symbol *lvar_type);

#endif /* SCOPE_H_ac8e7764_acea_11f0_b273_0023ae68f329 */
