/* builtinsP.h -- acceso privado a los detalles del tipo builtin
 * y subr_cb.
 * Author: Edward Rivas <rivastkw@gmail.com>
 * Date: Mon Oct 13 12:30:09 -05 2025
 * Copyright: (c) 2025 Edward Rivas.  All rights reserved.
 * License: BSD
 */
#ifndef BUILTINSP_H_650fa348_a85a_11f0_9d05_0023ae68f329
#define BUILTINSP_H_650fa348_a85a_11f0_9d05_0023ae68f329

#include "instr.h"

#include "builtins.h"

struct builtin_s {
    Symbol   *sym;
    bltin_cb  subr;
}; /* struct builtin_s */

#endif /* BUILTINSP_H_650fa348_a85a_11f0_9d05_0023ae68f329 */
