/* symbolP.h -- Tablas de simbolos anidadas.
 * Author: Edward Rivas <rivastkw@gmail.com>
 * Date: Sat Jun  7 01:42:04 -05 2025
 * Copyright: (c) 2025 Edward Rivas.  All rights reserved.
 * License: BSD
 */
#ifndef SYMBOL_H_PRIVATE
#define SYMBOL_H_PRIVATE

#include "hash.h"

#include "symbol.h"

struct SymbolTable {
		struct hash_map *map;
		SymbolTable     *next;
}; /* SymbolTable */

#endif /* SYMBOL_H_PRIVATE */
