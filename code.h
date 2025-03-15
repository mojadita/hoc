#ifndef CODE_H
#define CODE_H

#include "hoc.h"

extern Cell *progp;          /* next free cell for code generation */

void initcode(void);         /* initalize for code generation */
void push(Datum        d);   /* push d onto stack */
Datum pop(void);             /* pops Datum and rturn top element from stack */
Cell *code_inst(
        Inst           f,
        const char    *name);/* encodes one instruction of operand */
Cell *code_sym(Symbol *s);   /* encodes one symbol in a Cell */
Cell *code_val(double  val); /* encodes a double value in a Cell */
Cell *code_cel(Cell   *cel); /* install one reference to Cell */
void execute(Cell     *p);   /* run the machine */
/* instructions */
void drop(void);             /* deletes the top stack value */
void constpush(void);        /* push constant onto stack */
void add(void);              /* add top two elements on stack */
void sub(void);              /* subtract top two elements on stack */
void mul(void);              /* multiply top two elements on stack */
void divi(void);             /* divide top two elements on stack */
void mod(void);              /* mod top two elements on stack */
void neg(void);              /* change sign top element on stack */
void pwr(void);              /* pow top two elements on stack */
void eval(void);             /* evaluate variable on stack */
void assign(void);           /* assign top value to next value */
void print(void);            /* pop top value from stack, print it */
void bltin0(void);           /* evaluate built-in on top of stack */
void bltin1(void);           /* evaluate built-in with one argument */
void bltin2(void);           /* evaluate built-in with two arguments */
void whilecode(void);        /* execute while loop */
void ifcode(void);           /* execute if statement */
void ge(void);               /* greater or equal */
void le(void);               /* less or equal */
void gt(void);               /* greater than */
void lt(void);               /* less than */
void eq(void);               /* equal */
void ne(void);               /* not equal */
void not(void);              /* not */
void and(void);              /* and */
void or(void);               /* or */
void readopcode(void);       /* read to var */
void procret(void);          /* return from proc */
void funcret(void);          /* return from func */
void define(Symbol *sp);     /* put func/proc in symbol table */
void call(void);             /* call a function */
void procret(void);          /* return from proc */
void funcret(void);          /* return from func */
Datum *getarg(void);         /* return a pointer to argument */
void argeval(void);          /* push argument onto stack */
void argassign(void);        /* store top of stack in argument */
void prstr(void);            /* print string */
void prexpr(void);           /* print numeric value */

int stacksize(void);         /* return the stack size */

#endif /* CODE_H */
