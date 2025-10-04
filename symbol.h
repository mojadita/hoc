/* symbol.h -- tipo symbol para la tabla de simbolos.
 * Author: Edward Rivas <rivastkw@gmail.com>
 * Date: Mon Aug  4 11:03:09 -05 2025
 * Copyright: (c) 2025 Edward Rivas.  All rights reserved.
 * License: BSD
 */
#ifndef SYMBOL_H
#define SYMBOL_H

typedef struct Symbol_s Symbol;

const char *lookup_type(int typ);
void list_symbols(void);
void list_all_symbols(Symbol *current_symbol);
void list_variables(Symbol *current_symbol);

#endif /* SYMBOL_H */
