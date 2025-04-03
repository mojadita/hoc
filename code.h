#ifndef CODE_H
#define CODE_H

#include "hoc.h"
#include "instr.h"

extern Cell *progp;                   /* next free cell for code generation */
extern Cell *progbase;                /* pointer to first program instruction */

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
void  end_define(void);               /* housekeeping after function definition */
int   stacksize(void);                /* return the stack size */

/* instructions */
/* LCU: Esta macro define dos prototipos por cada instruccion:
 * * el prototipo de la instruccion propiamente dicha (el que
 *   se ejecuta cuando se invoca la instruccion.
 * * el prototipo de impresion de la instruccion (el que se
 *   ejecuta para imprimir la instruccion)
 * Se invoca la macro una vez por cada instruccion, generandose
 * ambos prototipos (estos deben implementarse normalmente en la
 * unidad de compiladion code.c) */
#define INST(_nom)         \
        void _nom(         \
            const instr *);\
        void _nom##_prt(   \
            const instr *, \
            const Cell **);

#include "instrucciones.h"
#undef  INST

#endif /* CODE_H */
