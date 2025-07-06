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
new_formal_param_list()
{
    list_of_params *ret_val = calloc(1, sizeof *ret_val);
    assert(ret_val != NULL);

#if 0 /* mientras mantengamos calloc arriba esto es redundante { */
    const static list_of_params ini = {
        .data         = NULL,
        .data_len     = 0,
        .data_cap     = 0,
        .accum_offset = 0,
    };

    *ret_val = ini;
#endif   /* } */

    return ret_val;
} /* new_list_of_params */

param *
add_to_formal_param_list(
        list_of_params *list,
        formal_param   *elem)
{
    DYNARRAY_GROW(
            list->data,
            param *,
            1,
            UQ_ADD_TO_LIST_OF_PARAMS_INCR);

    param *ret_val =  list->data + list->data_len++;
    *ret_val       = *elem;

    list->accum_offset = elem->offset;

    return ret_val;
} /* add_to_formal_param_list */
