/* symbol.h -- tipo symbol para la tabla de simbolos.
 * Author: Edward Rivas <rivastkw@gmail.com>
 * Date: Mon Aug  4 11:03:09 -05 2025
 * Copyright: (c) 2025 Edward Rivas.  All rights reserved.
 * License: BSD
 */
#ifndef SYMBOL_H_c71f231c_acea_11f0_b3dd_0023ae68f329
#define SYMBOL_H_c71f231c_acea_11f0_b3dd_0023ae68f329

typedef struct Symbol_s Symbol;

const char *lookup_type(int typ);
void list_symbols(void);
void list_all_symbols(Symbol *current_symbol);
void list_variables(Symbol *current_symbol);

Symbol *register_subr(
        const char   *name,    /* nombre de la funcion/procedimiento */
        int           type,    /* tipo de symbolo (PROCEDURE/FUNCTION) */
        const Symbol *typref,  /* simbolo del tipo del valor devuelto por la
                                * funcion, NULL para proc */
        Cell         *entry);  /* punto de entrada a la funcion */

void end_register_subr(        /* end subroutine definition */
        const Symbol *subr);   /* the symbol given by register_subr */

Symbol *register_global_var(   /* registers a global variable */
        const char   *name,    /* name of the function */
        const Symbol *typref);

Symbol *register_local_var(
        const char   *name,
        const Symbol *typref); /* registers a local variable */

#endif /* SYMBOL_H_c71f231c_acea_11f0_b3dd_0023ae68f329 */
