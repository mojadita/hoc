/* math.c -- funciones matematicas y wrappers de funciones con
 * chequeo del dominio.
 * Author: Edward Rivas <rivastkw@gmail.com>
 *       y Luis Colorado <luiscoloradourcola@gmail.com>
 * Date: Tue Dec 31 17:36:17 -05 2024
 * Copyright: (c) 2025 Edward Rivas y Luis Colorado.  All rights reserved.
 * License: BSD.
 */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "error.h"

long fast_pwr_l(long x, int e)
{
    unsigned mask;

    if (x == 0.0 || e < 0)
        execerror("x(%li) ^^ y(%i) must "
            "have x != 0 and y >= 0 or "
            "use floating point",
            x, e);

    for (mask = 1; mask <= e; mask <<= 1)
        continue;
    mask >>= 1;

    double ret_val = 1.0;
    while (mask) {
        ret_val *= ret_val;
        if (mask & e) {
            ret_val *= x;
        }
        mask >>= 1;
    }
    return ret_val;
} /* fast_pwr_l */
