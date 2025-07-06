/* intern.c -- Internalizacion de cadenas de caracteres.
 * Author: Luis Colorado <luiscoloradourcola@gmail.com>
 * Date: Sun Jun 29 09:24:56 -05 2025
 * Copyright: (c) 2025 Luis Colorado.  All rights reserved.
 * License: BSD
 */

#include <string.h>

#include "config.h"
#include "dynarray.h"

#include "intern.h"

#ifndef   UQ_INTERN_INCREMENT /* { */
#warning  UQ_INTERN_INCREMENT should be defined in config.mk
#define   UQ_INTERN_INCREMENT (32)
#endif /* UQ_INTERN_INCREMENT    } */

static char   **data;
static size_t   data_len = 0,
                data_cap = 0;

const char *intern(const char *s)
{
    int i;
    for (i = 0; i < data_len && strcmp(s, data[i]) != 0; i++)
        continue;
    /* i >= data_len || strcmp(s, data[i]) == 0 */

    if (i < data_len) return data[i];

    DYNARRAY_GROW(data, char *, 1, UQ_INTERN_INCREMENT);

    return (data[data_len++] = strdup(s));
} /* intern */
