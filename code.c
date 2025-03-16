
#include <math.h>
#include <stdio.h>

#include "hoc.h"
#include "hoc.tab.h"

#ifndef DEBUG_P1
#define DEBUG_P1 1
#endif
#ifndef DEBUG_P2
#define DEBUG_P2 1
#endif

#if DEBUG_P1
#define P(_fmt, ...) \
    printf("%s:%d: %s"_fmt, __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#else
#define P(_fmt, ...)
#endif

#if DEBUG_P2
#define P2(_fmt, ...) printf(_fmt, ##__VA_ARGS__)
#else
#define P2(_fmt, ...)
#endif

#define NSTACK 256
static Datum  stack[NSTACK];  /* the stack */
static Datum *stackp;         /* next free cell on stack */

#define NPROG 10000 /* 10000 celdas para instrucciones */

Cell  prog[NPROG];  /* the machine */
Cell *progp;        /* next free cell for code generation */
Cell *pc;           /* program counter during execution */
Cell *progbase = prog; /* start of current subprogram */
int   returning;    /* 1 if return stmt seen */

typedef struct Frame { /* proc/func call stack frame */
	Symbol *sym;       /* symbol table entry */
	Cell   *retpc;     /* where to resume after return */
	Datum  *argn;      /* n-th argument on stack */
	int     nargs;     /* number of arguments */
} Frame;

#define NFRAME 100
Frame frame[NFRAME];
Frame *fp     = frame + NFRAME;      /* frame pointer */

void initcode(void)  /* initalize for code generation */
{
    progp     = progbase;
    stackp    = stack;
	fp        = frame + NFRAME;
	returning = 0;
}

int stacksize(void) /* return the stack size */
{
    return stackp - stack;
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

void drop(void) /* drops the top of stack */
{
    P("\n");
    pop();
}

Cell *code_inst(Inst f, const char *name) /* install one instruction of operand */
{
    Cell *old_progp = progp;

    if (progp >= &prog[NPROG])
        execerror("program too big");

    P2("0x%04x: %s\n", (int)(old_progp - prog), name);

    (progp++)->inst = f;
    return old_progp;
}

Cell *code_sym(Symbol *s) /* install one instruction of operand */
{
    Cell *old_progp = progp;

    if (progp >= &prog[NPROG])
        execerror("program too big");
    P2("      : Symbol '%s'\n", s->name);

    (progp++)->sym = s;
    return old_progp;
}

Cell *code_val(double val) /* install one instruction of operand */
{
    Cell *old_progp = progp;

    if (progp >= &prog[NPROG])
        execerror("program too big");
    P2("      : DATA %.10g\n", val);

    (progp++)->val = val;
    return old_progp;
}

Cell *code_cel(Cell *cel) /* install one reference to Cell */
{
    Cell *old_progp = progp;

    if (progp >= &prog[NPROG])
        execerror("program too big");
    P2("      : REF [0x%04x]\n",
            (int)(cel ? cel - prog : 0));

    (progp++)->cel = cel;
    return old_progp;
}

Cell *code_num(int val) /* install one integer on Cell */
{
	Cell *old_progp = progp;
	if (progp >= prog + NPROG)
		execerror("program too big");
	P2("       : INT [%d]\n", val);
	(progp++)->num = val;
	return old_progp;
}

void execute(Cell *p) /* run the machine */
{
    P(" \033[1;33mBEGIN\033[m\n");
    for (pc = p; pc->inst != STOP && !returning;) {
        (pc++)->inst();
    }
    P(" \033[1;33mEND\033[m\n");
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
    P(": %.8lg -> %s\n", d, sym->name);
    sym->val    = d;
    sym->type   = VAR;
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

void whilecode(void) /* execute the while loop */
{
    P("\n");
    Cell *savepc = pc;  /* savepc[0] loop body address */
                        /* savepc[1] is the loop end address */
                        /* savepc + 2 is the cond starting point */
    P(" execute cond code\n");
    execute(savepc + 2); /* execute cond code */
    Datum d = pop();     /* pop the boolean data */
    while (d) {
        P(" execute body code\n");
        execute(savepc[0].cel); /* execute body */
        P(" execute cond code\n");
        execute(savepc + 2);    /* execute cond again */
        d = pop();
    }
    P(" END while loop\n");
	if (!returning) {
		pc = savepc[1].cel;
	}
}

void ifcode(void) /* execute the if statement */
{
    P("\n");
    Cell *savepc = pc;  /* savepc[0] then statement address */
                        /* savepc[1] else statement address */
                        /* savepc[2] next statement address */
                        /* savepc + 3 first cond evaluation instruction */
    P(" execute cond code\n");
    execute(savepc + 3); /* execute cond code */
    Datum d = pop();    /* pop the boolean data */
    if (d) {
        P(" execute THEN code\n");
        execute(savepc[0].cel);
    } else if (savepc[1].cel) {
        P(" execute ELSE code\n");
        execute(savepc[1].cel);
    }
    P(" END\n");
	if (!returning) {
		pc = savepc[2].cel;
	}
}

void ge(void) /* greater or equal */
{
    P("\n");
    Datum p2  = pop(),
          p1  = pop(),
          res = p1 >= p2;
    P(": %lg >= %lg -> %lg\n",
            p1, p2, res);
    push(res);
}

void le(void) /* less or equal */
{
    P("\n");
    Datum p2  = pop(),
          p1  = pop(),
          res = p1 <= p2;
    P(": %lg <= %lg -> %lg\n",
            p1, p2, res);
    push(res);
}

void gt(void) /* greater than */
{
    P("\n");
    Datum p2  = pop(),
          p1  = pop(),
          res = p1 > p2;
    P(": %lg > %lg -> %lg\n",
            p1, p2, res);
    push(res);
}

void lt(void) /* less than */
{
    P("\n");
    Datum p2  = pop(),
          p1  = pop(),
          res = p1 < p2;
    P(": %lg < %lg -> %lg\n",
            p1, p2, res);
    push(res);
}

void eq(void) /* equal */
{
    P("\n");
    Datum p2  = pop(),
          p1  = pop(),
          res = p1 == p2;
    P(": %lg == %lg -> %lg\n",
            p1, p2, res);
    push(res);
}

void ne(void) /* not equal */
{
    P("\n");
    Datum p2  = pop(),
          p1  = pop(),
          res = p1 != p2;
    P(": %lg != %lg -> %lg\n",
            p1, p2, res);
    push(res);
}

void not(void) /* not */
{
    P("\n");
    Datum p  = pop(),
          res = ! p;
    P(": ! %lg -> %lg\n",
            p, res);
    push(res);
}

void and(void)              /* and */
{
    P("\n");
    Datum p2  = pop(),
          p1  = pop(),
          res = p1 && p2;
    P(": %lg && %lg -> %lg\n",
            p1, p2, res);
    push(res);
}

void or(void)               /* or */
{
    P("\n");
    Datum p2  = pop(),
          p1  = pop(),
          res = p1 || p2;
    P(": %lg || %lg -> %lg\n",
            p1, p2, res);
    push(res);
}

void readopcode(void)               /* readopcode */
{
    P("\n");
    Symbol *sym = (pc++)->sym;
    if (scanf("%lg", &sym->val) != 1) {
        execerror("Lectura incorrecta del valor de la variable %s\n",
                  sym->name);
    }
    P(": %.8lg -> %s\n", sym->val, sym->name);
    sym->type   = VAR;
	push(sym->val);
}

void end_define(void)
{
	/* adjust progbase to point to the code starting point */
	progbase = progp;     /* next code starts here */
}

Cell *define(Symbol *symb, int type)
{
	if (symb->type != UNDEF) {
		execerror("symbol redefinition not allowed (%s)\n",
				symb->name);
	}
	symb->type = type;
	symb->defn = progp;

	return progp;
}

void call(void)   /* call a function */
{
    P("\n");

	Symbol *sym  = pc[0].sym;

	if (fp == frame) {
		execerror("Llamada a '%s' demasiado profunda\n",
			sym->name);
	}
	 
	fp--;
	fp->sym      = sym;
	fp->nargs    = pc[1].num;
	fp->retpc    = pc + 2;
	fp->argn     = stackp - 1; /* pointer to last argument */
	P("execute @[0x%04x], %s '%s', args=%d, ret_addr=0x%04x\n",
		(int)(sym->defn - prog), sym->type == FUNCTION ? "func" : "proc",
		sym->name, fp->nargs, (int)(fp->retpc - prog));
	execute(sym->defn);

	if (fp >= frame + NPROG) {
		execerror("Smatching stack, la pila esta corrompida\n");
	}
	P("return from @[0x%04x], %s '%s', args=%d, ret_addr=0x%04x\n",
		(int)(sym->defn - prog), sym->type == FUNCTION ? "func" : "proc",
		sym->name, fp->nargs, (int)(fp->retpc - prog));
	pc        = fp->retpc;
	returning = 0;
	++fp;
}

static void ret(void)
{
	for (int i = 0; i < fp->nargs; i++) {
		drop(); /* pop arguments */
	}

	returning = 1;
}

void procret(void) /* return from proc */
{
    P("\n");
	ret();
}

void funcret(void) /* return from func */
{
    P("\n");
	Datum d = pop();  /* preserve function return value */
	ret();
	push(d);
}

Datum *getarg(void)    /* return a pointer to argument */
{
	int arg = pc[0].num;
	if (arg > fp->nargs) {
		execerror("Accessing arg $%d while only %d "
			"args have been passed\n",
			arg, fp->nargs);
	}
	return fp->argn - fp->nargs + arg;
}

void argeval(void) /* push argument onto stack */
{
	push(*getarg());
}

void argassign(void) /* store top of stack in argument */
{
	push(*getarg() = pop()); /* pop the top, assign to arg,
							  * then push it again */
}

void prstr(void) /* print string */
{
	printf("%s", pc++[0].str);
}

void prexpr(void)  /* print numeric value */
{
	printf("%.8g ", pop());
}
