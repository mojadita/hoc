/* lists.c -- estructuras para manejo de listas.
 * Author: Luis Colorado <luiscoloradourcola@gmail.com>
 * Date: Mon Jun 30 09:22:39 -05 2025
 * Copyright: (c) 2025 Luis Colorado.  All rights reserved.
 * License: BSD
 */

#include <assert.h>
#include <stdlib.h>

#include "dynarray.h"

#include "lists.h"

list_of_params *
new_list_of_params()
{
    list_of_params *ret_val = malloc(sizeof *ret_val);
    assert(ret_val != NULL);

    const static list_of_params ini = {
        .data         = NULL,
        .data_len     = 0,
        .data_cap     = 0,
        .accum_offset = 0,
    };

    *ret_val = ini;

    return ret_val;
} /* new_list_of_params */

param *
add_to_list_of_params(
        list_of_params *list,
        Symbol         *type,
        const char     *param_name,
        off_t           offset,
        Cell           *init)
{
    DYNARRAY_GROW(
            list->data,
            param *,
            1,
            UQ_ADD_TO_LIST_OF_PARAMS_INCR);

    const static param val = {
        .type      = type,
        .parm_name = param_name,
        .offset    = offset,
    };

	param *ret_val = list->data + list->data_len++;
	*ret_val       = val;

    return ret_val;
} /* add_to_list_of_params */
