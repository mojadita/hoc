/* dynarray.h -- dynamic growing array (malloc based) via macros.
 * Author: Luis Colorado <luis.colorado@spindrive.fi>
 * Date: Mon Dec 16 13:35:35 EET 2024
 * Copyright: (c) 2024 SpinDrive Oy, FI.  All rights reserved.
 */
#ifndef DYNARRAY_H
#define DYNARRAY_H

#include <assert.h>  /* for assert macro */
#include <stdlib.h>  /* for realloc */

/* the following macro will realloc a pointer to an array _arry
 * of _type elements (dynamically allocated) by calculating the
 * needed number of elements (assuming a grow granularity of
 * _inc) the resize consists in a multiple of _inc elements
 * to the actual number in _arry##_cap */

#define DYNARRAY_GROW(_arry, _type, _need, _inc) \
        do {                                     \
            size_t inc = (_inc),                 \
                   nxt = _arry##_len             \
                    + (_need) + inc - 1;         \
            /* nxt to be mult of inc */          \
            nxt    -= nxt % inc;                 \
            if (nxt > _arry##_cap) {             \
                _arry##_cap = nxt;               \
                _arry = realloc(_arry, nxt       \
                        * (sizeof _arry[0]));    \
                assert(_arry != NULL);           \
            }                                    \
        } while (0)

#endif /* DYNARRAY_H */
