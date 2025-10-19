/* code.h -- prototipos y definiciones para el modulo code.c
 * Author: Edward Rivas <rivastkw@gmail.com>
 *       y Luis Colorado <luiscoloradourcola@gmail.com>
 * Date: Thu Apr 17 12:58:58 EEST 2025
 * Copyright: (c) 2025 Luis Colorado y Edward Rivas.  All rights reserved.
 * License: BSD
 */
#ifndef CODE_H_56139530_ac78_11f0_b0d7_0023ae68f329
#define CODE_H_56139530_ac78_11f0_b0d7_0023ae68f329

#include "instr.h"

#include "cell.h"
#include "symbol.h"
#include "hoc.h"

extern Cell *progp;                     /* next free cell for code generation */
extern Cell *progbase;                  /* pointer to first program instruction */
extern Cell *varbase;                   /* pointer to last assigned variable */

void    initcode(void);                 /* initalize for code generation */
void    initexec(void);                 /* initalize for code execution */

void    push(                           /* push d onto stack */
        Cell          d);
Cell    pop(void);                      /* pops Cell and rturn top element from stack */

Cell   *code_inst(                      /* encodes one instruction of operand */
        instr_code    f,
        ...);

void    execute(
        Cell         *p);               /* run the machine */

Symbol *register_subr(                  /* put func/proc in symbol table */
        const char   *name,
        int           type,
        const Symbol *typref,
        Cell         *entry_point);

void    end_register_subr(              /* housekeeping after function definition */
        const Symbol *subr);

int     stacksize(void);                /* return the stack size */

Cell   *getarg(int arg);                /* return a pointer to argument */

/* instructions */
/* LCU: Esta macro define dos prototipos por cada instruccion:
 * * el prototipo de la instruccion propiamente dicha (el que
 *   se ejecuta cuando se invoca la instruccion.
 * * el prototipo de impresion de la instruccion (el que se
 *   ejecuta para imprimir la instruccion)
 * Se invoca la macro una vez por cada instruccion, generandose
 * ambos prototipos (estos deben implementarse normalmente en la
 * unidad de compiladion code.c) */
#define INST(_nom,_n, ...) \
        void _nom(         \
            const instr *);\
        void _nom##_prt(   \
            const instr *, \
            const Cell *); \
        __VA_ARGS__

#define SUFF(_typ, _nom, _suf)  \
        _typ _nom##_##_suf(     \
                const instr *,  \
                Cell        *,  \
                va_list args);

#include "instrucciones.h"

#undef  INST
#undef  SUFF

#endif /* CODE_H_56139530_ac78_11f0_b0d7_0023ae68f329 */
