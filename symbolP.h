/* symbolP.h -- tipo symbol para la tabla de simbolos.
 * Author: Edward Rivas <rivastkw@gmail.com>
 * Date: Mon Aug  4 11:03:09 -05 2025
 * Copyright: (c) 2025 Edward Rivas.  All rights reserved.
 * License: BSD
 */
#ifndef SYMBOLP_H
#define SYMBOLP_H

#include <sys/types.h>

#include "symbol.h"

#include "cellP.h"
#include "types.h"
#include "instr.h"
#include "scope.h"

struct Symbol_s {                         /* Symbol table entry */
    const char    *name;                  /* nombre del simbolo */
    int            type;                  /* tipo del simbolo:
                                           * VAR, BLTIN[012], UNDEF */
    const char    *help;                  /* help text (optional) */
    const Symbol  *typref;                /* ref al tipo de la
                                           * variable/func/builtin... */
    union {
        Cell       cel;                   /* si el tipo es CONST */
        double   (*ptr0)(void);           /* si el tipo es BLTIN0 */
        double   (*ptr1)(double);         /* si el tipo es BLTIN1 */
        double   (*ptr2)(double, double); /* si el tipo es BLTIN2 */
        struct {                          /* si el tipo es FUNC, PROC o
                                           * VAR o BLTIN_PROC o BLTIN_FUNC */
            Cell       *defn;             /* donde empieza el codigo de la funcion */
            scope      *main_scope;       /* scope principal */

            /* Datos necesarios para la macro DYNARRAY() */
            Symbol    **argums;           /* puntero a array de punteros a Symbol * */
            size_t      argums_len;       /* longitud del array de Symbol * argums */
            size_t      argums_cap;       /* capacidad del array anterior */

            Cell      **returns_to_patch; /* lista de returns que hay que parchear */
            size_t      returns_to_patch_len, /* num elementos en la lista */
                        returns_to_patch_cap; /* capacidad de la lista */

            int         size_args;        /* tama;o de los argumentos */
            int         size_lvars;       /* tama;o de las variables locales */
            int         bltin_index;      /* indice del builtin, para los builtins */
            int         ret_val_offset;   /* offset del valor a retornar */
        };
        struct {                          /* si el tipo es LVAR */
            int         offset;           /* variables locales y argumentos (LVAR),
                                           * offset respecto al frame pointer (fp). */
        };
        struct {                          /* si el tipo es TYPE */
            const type2inst
                       *t2i;              /* ej. sym->typref->t2i->constpush->code_id
                                           * nos dara para cada tipo, la instruccion
                                           * constpush que opera con datos de ese tipo
                                           */
        };
    }  /* no hay nombre de campo */ ;
       /* union anonima, el nombre del campo no existe, de forma que los
        * nombres de los campos de la union pueden usarse directamente desde
        * la estructura Symbol.  Esto ***solo*** es valido en C, y no en
        * C++ */
    Symbol        *next;                  /* enlace al siguiente
                                           * simbolo de la tabla.*/
};

const char *lookup_type(int typ);
void list_symbols(void);
void list_all_symbols(Symbol *current_symbol);
void list_variables(Symbol *current_symbol);

#endif /* SYMBOLP_H */
