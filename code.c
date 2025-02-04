
#include <math.h>
#include <stdio.h>

#include "hoc.h"
#include "y.tab.h"

#define NSTACK 256
static Datum stack[NSTACK];   /* the stack */
static Datum *stackp;         /* next free cell on stack */

#define NPROG 2000   /* 2000 celdas para instrucciones */
Inst  prog[NPROG];   /* the machine */
Inst  *progp;        /* next free cell for code generation */
Inst  *pc;           /* program counter during execution */

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

Inst *code(Inst f) /* install one instruction of operand */
{
	Inst *oprogp = progp;
	if (progp >= &prog[NPROG])
		execerror("program too big");
	*progp++ = f;
	return oprogp;
}

void execute(Inst *p) /* run the machine */
{
	P("\n");
	for (pc = p; *pc != STOP;) {
		(*pc++)();
	}
}

void constpush(void) /* push constant onto stack */
{
	P("\n");
	Datum d;
	d.val = ((Symbol *)*pc++)->u.val;
	P(": %.8lg\n", d.val);
	push(d);
}

void varpush(void)   /* push variable onto stack */
{
	P("\n");
	Datum d;
	d.sym = (Symbol *)(*pc++);
	P(": %s\n", d.sym->name);
	push(d);
}

void add(void) /* add top two elements on stack */
{
	P("\n");
	Datum d2 = pop(), d1 = pop();
	P(": d1=%lg, d2=%lg\n", d1.val, d2.val);
	d1.val += d2.val;
	P(": sum -> %lg\n", d1.val);
	push(d1);
}

void sub(void) /* subtract top two elements on stack */
{
	P("\n");
	Datum d2 = pop(), d1 = pop();
	P(": d1=%lg, d2=%lg\n", d1.val, d2.val);
	d1.val -= d2.val;
	P(": -> %lg\n", d1.val);
	push(d1);
}

void mul(void) /* multiply top two elements on stack */
{
	P("\n");
	Datum d2 = pop(), d1 = pop();
	P(": d1=%lg, d2=%lg\n", d1.val, d2.val);
	d1.val *= d2.val;
	P(": -> %lg\n", d1.val);
	push(d1);
}

void divi(void) /* divide top two elements on stack */
{
	P("\n");
	Datum d2 = pop(), d1 = pop();
	P(": d1=%lg, d2=%lg\n", d1.val, d2.val);
	d1.val /= d2.val;
	push(d1);
}

void mod(void) /* mod top two elements on stack */
{
	P("\n");
	Datum d2 = pop(), d1 = pop();
	P(": d1=%lg, d2=%lg\n", d1.val, d2.val);
	d1.val = fmod(d1.val, d2.val);
	P(": -> %lg\n", d1.val);
	push(d1);
}

void neg(void) /* change sign top element on stack */
{
	P("\n");
	Datum d = pop();
	P(": d=%lg\n", d.val);
	d.val = -d.val;
	P(": -> %lg\n", d.val);
	push(d);
}

void pwr(void) /* pow top two elements on stack */
{
	P("\n");
	Datum d2 = pop(), d1 = pop();
	P(": d1=%lg, d2=%lg\n", d1.val, d2.val);
	d1.val = pow(d1.val, d2.val);
	P(": -> %lg\n", d1.val);
	push(d1);
}

void eval(void) /* evaluate variable on stack */
{
	P("\n");
	Datum d;
	d = pop();
	P(": d1=%s\n", d.sym->name);
	if (d.sym->type == UNDEF)
		execerror("undefined variable '%s'", d.sym->name);
	d.val = d.sym->u.val;
	P(": -> %lg\n", d.val);
	push(d);
}

void assign(void) /* assign top value to next value */
{
	P("\n");
	Datum d1 = pop(),
		  d2 = pop();
	P(": d1 = '%s', d2 = %.8lg\n", d1.sym->name, d2.val);
	d1.sym->u.val = d2.val;
	d1.sym->type  = VAR;
	push(d2);
}
		
void print(void) /* pop top value from stack, print it */
{
	P("\n");
	Datum d = pop();
	printf("\t\t%32.8g\n", d.val);
}

void bltin0(void) /* evaluate built-in on top of stack */
{
	P("\n");
	Symbol *sym = (Symbol *)(*pc++);
	Datum d;
	/* d.val = (*((Symbol *)(*pc++))->u.bltn0)(); */
	d.val = sym->u.ptr0();
	P(": %s() -> %.8lg\n", sym->name, d.val);
	push(d);
}

void bltin1(void) /* evaluate built-in with one argument */
{
	P("\n");
	//Datum d = pop();
	//d.val   = (*(double(*)(double))(*pc++))(d.val);

	Symbol *sym = (Symbol *)(*pc++);
	Datum d = pop();
	P(": d = %.8lg\n", d.val );
	/* d.val = (*((Symbol *)(*pc++))->u.bltn1)( d.val ); */
	d.val = sym->u.ptr1( d.val );
	P(": %s(d) -> %.8lg\n", sym->name, d.val);
	push(d);
}

void bltin2(void) /* evaluate built-in with two arguments */
{
	//Datum d2 = pop(), d1 = pop();
	//d1.val = (*(double(*)(double, double))(*pc++))(d1.val, d2.val);
	//push(d);


	P("\n");
	Symbol *sym = (Symbol *)(*pc++);
	Datum d2 = pop();
	Datum d1 = pop();
	P( " d1 = %lg, d2 = %lg\n", d1.val, d2.val );
	/* d.val = (*((Symbol *)(*pc++))->u.bltn1)( d.val ); */
	d1.val = sym->u.ptr2( d1.val, d2.val );
	P(": %s(d1, d2) -> %.8lg\n", sym->name, d1.val);
	push(d1);
}
