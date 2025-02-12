#ifndef CODE_H
#define CODE_H

#include "hoc.h"

void initcode(void);  /* initalize for code generation */
void push(Datum d);  /* push d onto stack */
Datum pop(void);    /* pops Datum and rturn top element from stack */
void drop(void);    /* deletes the top stack value */
Cell *code_inst(Inst f); /* encodes one instruction of operand */
Cell *code_sym(Symbol *s); /* encodes one symbol in a Cell */
Cell *code_val(double val); /* encodes a double value in a Cell */
void execute(Cell *p); /* run the machine */
void constpush(void); /* push constant onto stack */
void varpush(void);   /* push variable onto stack */
void add(void); /* add top two elements on stack */
void sub(void); /* subtract top two elements on stack */
void mul(void); /* multiply top two elements on stack */
void divi(void); /* divide top two elements on stack */
void mod(void); /* mod top two elements on stack */
void neg(void); /* change sign top element on stack */
void pwr(void); /* pow top two elements on stack */
void eval(void); /* evaluate variable on stack */
void assign(void); /* assign top value to next value */
void print(void); /* pop top value from stack, print it */
void bltin0(void); /* evaluate built-in on top of stack */
void bltin1(void); /* evaluate built-in with one argument */
void bltin2(void); /* evaluate built-in with two arguments */

#endif /* CODE_H */
