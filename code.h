/* code.h -- prototipos y definiciones para el modulo code.c
 * Author: Edward Rivas <rivastkw@gmail.com>
 *       y Luis Colorado <luiscoloradourcola@gmail.com>
 * Date: Thu Apr 17 12:58:58 EEST 2025
 * Copyright: (c) 2025 Luis Colorado y Edward Rivas.  All rights reserved.
 * License: BSD
 */
#ifndef CODE_H
#define CODE_H

#include "hoc.h"
#include "instr.h"

extern Cell *progp;                   /* next free cell for code generation */
extern Cell *progbase;                /* pointer to first program instruction */
extern Cell *varbase;

void  initcode(void);                 /* initalize for code generation */
void  push(Datum           d);        /* push d onto stack */
Datum pop(void);                      /* pops Datum and rturn top element from stack */
Cell *code_inst(instr_code f, ...);   /* encodes one instruction of operand */
Cell *code_sym(Symbol     *s);        /* encodes one symbol in a Cell */
Cell *code_val(double      val);      /* encodes a double value in a Cell */
Cell *code_cel(Cell       *cel);      /* install one reference to Cell */
Cell *code_num(int         val);      /* install one integer on Cell */
Cell *code_str(const char *str);      /* install one string on Cell */
void  execute(Cell        *p);        /* run the machine */
Cell *define(Symbol       *symb,
             int           type);     /* put func/proc in symbol table */
void  end_define(Symbol   *subr);     /* housekeeping after function definition */
int   stacksize(void);                /* return the stack size */
Cell *register_global_var(Symbol *symb); /* registers a global variable */

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

#endif /* CODE_H */
