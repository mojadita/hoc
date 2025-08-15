/* intern.c -- Internalizacion de cadenas de caracteres.
 * Author: Luis Colorado <luiscoloradourcola@gmail.com>
 *         Edward Rivas <rivastkw@gmail.com>
 * Date: Tue Aug  5 10:49:57 -05 2025
 * Copyright: (c) 2025 Luis Colorado.  All rights reserved.
 * License: BSD
 */

#include <string.h>

#include "config.h"
#include "intern.h"
#include "dynarray.h"

static const char **cadenas;
size_t              cadenas_len,
                    cadenas_cap;

#ifndef   UQ_INTERN_INCRMNT /* { */
#warning  UQ_INTERN_INCRMNT should be defined in 'config.mk'
#define   UQ_INTERN_INCRMNT (10)
#endif /* UQ_INTERN_INCRMNT    } */

const char *intern(
        const char *name)
{
    for (int i = 0; i < cadenas_len; i++) {
        if (strcmp(name, cadenas[i]) == 0)
            return cadenas[i];
    }
    DYNARRAY_GROW(cadenas, const char *, 1, UQ_INTERN_INCRMNT);
    return cadenas[cadenas_len++] = strdup(name);
} /* intern */
