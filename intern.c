/* intern.c -- Interning string literals.
 * Author: Luis Colorado <luiscoloradourcola@gmail.com>
 *         Edward Rivas <rivastkw@gmail.com>
 * Date: Tue Aug  5 10:49:57 -05 2025
 * Copyright: (c) 2025-2026 Luis Colorado.  All rights reserved.
 * License: BSD
 */

#include <string.h>

#include "config.h"
#include "intern.h"
#include "dynarray.h"

static const char **strings;
size_t              strings_len,
                    strings_cap;

#ifndef   UQ_INTERN_INCRMNT /* { */
#warning  UQ_INTERN_INCRMNT should be defined in 'config.mk'
#define   UQ_INTERN_INCRMNT (10)
#endif /* UQ_INTERN_INCRMNT    } */

const char *intern(
        const char *name)
{
    for (int i = 0; i < strings_len; i++) {
        if (strcmp(name, strings[i]) == 0)
            return strings[i];
    }
    DYNARRAY_GROW(strings, const char *, 1, UQ_INTERN_INCRMNT);
    return strings[strings_len++] = strdup(name);
} /* intern */
