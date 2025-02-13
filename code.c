
#include <math.h>
#include <stdio.h>

#include "hoc.h"
#include "y.tab.h"

#define NSTACK 256
static Datum  stack[NSTACK];  /* the stack */
static Datum *stackp;         /* next free cell on stack */

#define NPROG 2000  /* 2000 celdas para instrucciones */
Cell  prog[NPROG];  /* the machine */
Cell *progp;        /* next free cell for code generation */
Cell *pc;           /* program counter during execution */

#define P(_fmt, ...) \
	printf("%s:%d: %s"_fmt, __FILE__, __LINE__, __func__, ##__VA_ARGS__)

void initcode(void)  /* initalize for code generation */
{
    stackp = stack;
    progp  = prog;
}

void push(Datum d)  /* push d onto stack */
{
    /*  Verificamos si el puntero apunta a una direccion mas alla
        del final de la pila  */
    if (stackp >= &stack[NSTACK])
        execerror("stack overflow\n");
    *stackp++ = d; /* equivalente a *stackp = d; stackp++; */
}

Datum pop(void)    /* pops Datum and rturn top element from stack */
{
    if (stackp == stack)
        execerror("stack empty\n");
    return *--stackp;
}

Cell *code_inst(Inst f) /* install one instruction of operand */
{
	Cell *oprogp = progp;

	if (progp >= &prog[NPROG])
		execerror("program too big");

	(progp++)->inst = f;
	return oprogp;
}

Cell *code_sym(Symbol *s) /* install one instruction of operand */
{
	Cell *oprogp = progp;

	if (progp >= &prog[NPROG])
		execerror("program too big");

	(progp++)->sym = s;
	return oprogp;
}

Cell *code_val(double val) /* install one instruction of operand */
{
	Cell *oprogp = progp;

	if (progp >= &prog[NPROG])
		execerror("program too big");

	(progp++)->val = val;
	return oprogp;
}

void execute(Cell *p) /* run the machine */
{
	P("\n");
	for (pc = p; pc->inst != STOP;) {
		(pc++)->inst();
	}
}

void drop(void) /* drops the top of stack */
{
	P("\n");
	pop();
}

void constpush(void) /* push constant onto stack */
{
	P("\n");
	Datum d = (pc++)->val;
	P(": -> %.8lg\n", d);
	push(d);
}

void add(void) /* add top two elements on stack */
{
	P("\n");
	Datum p2 = pop(),
		  p1 = pop(),
		  res = p1 + p2;
	P(": %lg + %lg -> %lg\n",
			p1, p2, res);
	push(res);
}

void sub(void) /* subtract top two elements on stack */
{
	P("\n");
	Datum p2 = pop(),
		  p1 = pop(),
		  res = p1 - p2;
	P(": %lg - %lg -> %lg\n",
			p1, p2, res);
	push(res);
}

void mul(void) /* multiply top two elements on stack */
{
	P("\n");
	Datum p2 = pop(),
		  p1 = pop(),
		  res = p1 * p2;
	P(": %lg * %lg -> %lg\n",
			p1, p2, res);
	push(res);
}

void divi(void) /* divide top two elements on stack */
{
	P("\n");
	Datum p2  = pop(),
		  p1  = pop(),
		  res = p1 / p2;
	P(": %lg / %lg -> %lg\n",
			p1, p2, res);
	push(res);
}

void mod(void) /* mod top two elements on stack */
{
	P("\n");
	Datum p2  = pop(),
		  p1  = pop(),
		  res = fmod(p1, p2);
	P(": %lg %% %lg -> %lg\n",
			p1, p2, res);
	push(res);
}

void neg(void) /* change sign top element on stack */
{
	P("\n");
	Datum d   = pop(),
	      res = -d;
	P(": d=%lg -> %lg\n",
			d, res);
	push(res);
}

void pwr(void) /* pow top two elements on stack */
{
	P("\n");
	Datum p2  = pop(),
		  p1  = pop(),
		  res = pow(p1, p2);
	P(": p1=%lg, d2=%lg -> %lg\n",
			p1, p2, res);
	push(res);
}

void eval(void) /* evaluate variable on stack */
{
	P("\n");
	Symbol *sym = (pc++)->sym;
	if (sym->type == UNDEF)
		execerror("undefined variable '%s'",
				sym->name);
	P(": %s -> %lg\n",
		sym->name, sym->val);
	push(sym->val);
}

void assign(void) /* assign top value to next value */
{
	P("\n");
	Symbol *sym = (pc++)->sym;
	Datum   d   = pop();
	P(": %.8lg -> %s\n",
			d, sym->name);
	sym->val = d;
	sym->type  = VAR;
	push(d);
}
		
void print(void) /* pop top value from stack, print it */
{
	P("\n");
	Datum d = pop();
	printf("\t\t%32.8g\n", d);
}

void bltin0(void) /* evaluate built-in on top of stack */
{
	P("\n");
	Symbol *sym = (pc++)->sym;
	Datum   res = sym->ptr0();
	P(": %s() -> %.8lg\n",
		sym->name, res);
	push(res);
}

void bltin1(void) /* evaluate built-in with one argument */
{
	P("\n");
	Symbol *sym = (pc++)->sym;
	Datum p   = pop(),
	      res = sym->ptr1( p );
	P(": %s(%.8lg) -> %.8lg\n",
		sym->name, p, res);
	push(res);
}

void bltin2(void) /* evaluate built-in with two arguments */
{
	P("\n");
	Symbol *sym = (pc++)->sym;
	Datum p2 = pop(),
	      p1 = pop(),
	     res = sym->ptr2( p1, p2 );
	P(": %s(%.8lg, %.8lg) -> %.8lg\n",
		sym->name, p1, p2, res);
	push(res);
}
