/* symbolP.h -- Symbol definition (opaque to the outide)
 * Author: Luis Colorado <luiscoloradourcola@gmail.com>,
 *         Edward Rivas <rivastkw@gmail.com>
 * Date: Mon Aug  4 11:03:09 -05 2025
 * Copyright: (c) 2025-2026 Luis Colorado.  All rights reserved.
 * License: BSD
 */
#ifndef SYMBOLP_H_f49099c0_acea_11f0_a250_0023ae68f329
#define SYMBOLP_H_f49099c0_acea_11f0_a250_0023ae68f329

#include <sys/types.h>

#include "symbol.h"

#include "cellP.h"
#include "types.h"
#include "instr.h"
#include "scope.h"

struct Symbol_s {                         /* Symbol table entry */
    const char    *name;                  /* symbol name */
    int            type;                  /* symbol type:
                                           * VAR, BLTIN[012], UNDEF */
    const char    *help;                  /* help text (optional, for builtins) */
    const Symbol  *typref;                /* ref to the function/variable/builtin
                                             returned value type. */
    union {
        Cell       cel;                   /* if type is CONST */
        struct {                          /* if type is FUNC, PROC,
                                           * VAR, BLTIN_PROC or BLTIN_FUNC */
            Cell       *defn;             /* function entry point */
            scope      *main_scope;       /* main scope for this function */

            /* Datos necesarios para la macro DYNARRAY() */
            Symbol    **argums;           /* reference to the array of Symbol ptrs */
            size_t      argums_len;       /* array length of the array of argument symbols */
            size_t      argums_cap;       /* capacity of the previous array */

            Cell      **returns_to_patch;     /* list of returns that must be patched in subroutine */
            size_t      returns_to_patch_len, /* number of entries in array */
                        returns_to_patch_cap; /* actual capacity of the array. */

            int         size_args;        /* stack size of subroutine parameters */
            int         size_lvars;       /* local variables size (including alignment) */
            int         bltin_index;      /* builtin index, for builtins. */
            int         ret_val_offset;   /* offset of the return value in the stack */
        };
        struct {                          /* if type is LVAR */
            int         offset;           /* offset of the local variable (LVAR),
                                           * all offsets are relative to the frmae pointer (fp). */
        };
        struct {                          /* if type is TYPE */
            const type2inst
                       *t2i;              /* e.g. sym->typref->t2i->constpush->code_id
                                           * will give data relative to each type.
                                           */
        };
    }  /* no name */ ;
       /* anonymous union, so all fields must have unique names.  This is not handled
        * in C++ so this code is not compatible with C++ */
    Symbol        *next;                  /* link to next Symbol. */
};

const char *lookup_type(int typ);
void        list_symbols(void);
void        list_all_symbols(Symbol *current_symbol);
void        list_variables(Symbol *current_symbol);

#endif /* SYMBOLP_H_f49099c0_acea_11f0_a250_0023ae68f329 */
