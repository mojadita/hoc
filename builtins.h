/* builtins.h -- tipos y constantes de las funciones builtin.
 * Author: Edward Rivas <rivastkw@gmail.com>
 * Date: Mon Oct 13 10:53:50 -05 2025
 * Copyright: (c) 2025 Edward Rivas.  All rights reserved.
 * License: BSD
 */
#ifndef BUILTINS_H_f92b2754_a84c_11f0_a2b7_0023ae68f329
#define BUILTINS_H_f92b2754_a84c_11f0_a2b7_0023ae68f329

#include "instr.h"
#include "symbol.h"

typedef struct builtin_s builtin;

typedef void (*bltin_cb)(const instr *i);

int
register_builtin(
        const char   *name,
        const Symbol *type,
        bltin_cb      function_ref,
        ...); /* ...parameter_name, parameter_type, ... */

#endif /* BUILTINS_H_f92b2754_a84c_11f0_a2b7_0023ae68f329 */
