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

typedef struct var_init_s       var_init;
typedef struct list_of_vars_s   list_of_vars;

typedef struct formal_param_s   formal_param;
typedef struct list_of_params_s list_of_params;

struct var_init_s {
	int         offset;
	const char *var_name;
	Cell       *initializer;
}; /* var_init_s */

struct list_of_vars_s {
	Symbol     *typref;
	Cell       *start;
	var_init   *data;
	size_t      data_len,
                data_cap;
    int         accum_offset;
}; /* list_of_vars */

struct formal_param_s {
    const char *param_name; /* param name */
    Symbol     *type;       /* param type */
    off_t       offset;     /* offset respect frame pointer fp */
}; /* formal_param_s */

struct list_of_params_s {
    formal_param **data;
    size_t         data_len,
                   data_cap;
    int            accum_offset;
}; /* list_of_params */

list_of_params *
new_list_of_params();

list_of_vars *
new_list_of_vars(Symbol *type);

void add_to_list_of_vars(
		list_of_vars   *lst,
		const var_init *to_add);

formal_param *
add_to_list_of_params(
        list_of_params *list,
        Symbol         *type,
        const char     *param_name,
        off_t           offset,
        Cell           *init);

#endif /* _LISTS_H */
