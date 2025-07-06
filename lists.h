/* lists.h -- estructuras para manejo de listas.
 * Author: Luis Colorado <luiscoloradourcola@gmail.com>
 * Date: Sun Jun 29 08:15:52 -05 2025
 * Copyright: (c) 2025 Luis Colorado.  All rights reserved.
 * License: BSD
 */
#ifndef _LISTS_H
#define _LISTS_H

#include <sys/types.h>

#include "code.h"
#include "symbol.h"

typedef struct var_init_s   var_init;
typedef struct decl_list_s  decl_list;

typedef struct formal_param_s     formal_param;
typedef struct formal_paramlist_s formal_paramlist;

struct var_init_s {
    int         offset;
    const char *var_name;
    Cell       *initializer;
}; /* var_init_s */

struct decl_list_s {
    Symbol     *typref;
    Cell       *start;
    var_init   *data;
    size_t      data_len,
                data_cap;
    int         accum_offset;
}; /* decl_list */

struct formal_param_s {
    Symbol     *param; /* param */
    Symbol     *type;       /* param type */
    off_t       offset;     /* offset respect frame pointer fp */
}; /* formal_param_s */

struct formal_paramlist_s {
    formal_param **data;
    size_t         data_len,
                   data_cap;
    int            accum_offset;
}; /* formal_paramlist */

formal_paramlist *
new_formal_paramlist();

decl_list *
new_decl_list(Symbol *type);

void add_to_decl_list(
        decl_list   *lst,
        const var_init *to_add);

formal_param *
add_to_formal_paramlist(
        formal_paramlist *list,
        Symbol         *type,
        const char     *param_name,
        off_t           offset,
        Cell           *init);

#endif /* _LISTS_H */
