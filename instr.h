/* instr.h -- definitions for types and constants of the
 * instr.c module.
 * Author: Luis Colorado <luiscoloradourcola@gmail.com>
 *       y Edward Rivas <rivastkw@gmail.com>
 * Date: Sat Mar 22 12:49:24 -05 2025
 * Copyright: (c) 2025-2026 Luis Colorado.  All rights reserved.
 * License: BSD
 */
#ifndef INSTR_H_c9973130_ace9_11f0_aae7_0023ae68f329
#define INSTR_H_c9973130_ace9_11f0_aae7_0023ae68f329

#include <stdarg.h>
#include <sys/types.h>

typedef enum   instr_code_e instr_code;
typedef struct instr        instr;

/* LCU: Mon Mar 24 12:27:57 -05 2025
 * This enumeration is introduced from the macro INST(_name)
 * and the instructions file "instrucciones.h".
 * Each instruction is defined as a constant of name
 * INST_<_name> in this enum, and it allows to create a
 * unique value for it and to assign all the related data of
 * the instruction in the correct position, in the order that
 * the values are defined in the named file. */
enum instr_code_e {
#define INST(_nom,_n, ...) INST_##_nom,
#define SUFF(_typ, _p1,_p2)

#include "instrucciones.h"

#undef  INST
#undef  SUFF
}; /* enum instr_code_e */

#include "cell.h"

struct instr {
    instr_code    code_id;
    int           n_cells; /* number of cells used by the instruccion. */
    const char   *name;
    void        (*exec)(const instr *);
    void        (*print)(const instr *, const Cell *);
    void        (*prog)(const instr *, Cell *progp, va_list args);
};

extern const instr  instruction_set[];
extern const size_t instruction_set_len;

#endif /* INSTR_H_c9973130_ace9_11f0_aae7_0023ae68f329 */
