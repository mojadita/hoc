/* code.c -- instrucciones y rutinas de traza.
 * Author: Edward Rivas <rivastkw@gmail.com>
 * Date: Sat Mar 22 14:20:43 -05 2025
 * Copyright: (c) 2025 Edward Rivas.  All rights reserved.
 * License: BSD
 */

#include <math.h>
#include <stdio.h>

#include "config.h"
#include "colors.h"

#include "hoc.h"
#include "hoc.tab.h"

#include "code.h"

#ifndef  UQ_CODE_DEBUG_EXEC
#warning UQ_CODE_DEBUG_EXEC deberia ser incluido en config.mk
#define  UQ_CODE_DEBUG_EXEC 1
#endif
#ifndef  UQ_CODE_DEBUG_PROG
#warning UQ_CODE_DEBUG_PROG deberia ser incluido en config.mk
#define  UQ_CODE_DEBUG_PROG 1
#endif

#if      UQ_CODE_DEBUG_EXEC /* {{ */
#define  EXEC(_fmt, ...)                     \
    printf("%s:%d:"_fmt, __FILE__, __LINE__, \
        ##__VA_ARGS__)
#define  P_TAIL(_fmt, ...)      \
    printf(_fmt, ##__VA_ARGS__)
#else /*  UQ_CODE_DEBUG_EXEC   }{ */
#define EXEC(_fmt, ...)
#define P_TAIL(_fmt, ...)
#endif /* UQ_CODE_DEBUG_EXEC   }} */

#define  PR(_fmt, ...)              \
    printf(YELLOW"%04lx" WHITE ": " \
        CYAN"%-10s"ANSI_END _fmt,   \
        *pc - prog,                 \
        i->name,                    \
        ##__VA_ARGS__)

#if UQ_CODE_DEBUG_PROG
#define PRG(_fmt, ...) printf(_fmt, ##__VA_ARGS__)
#else
#define PRG(_fmt, ...)
#endif

#ifndef  UQ_NSTACK
#warning UQ_NSTACK debe definirse en config.mk
#define  UQ_NSTACK 100000
#endif

static Datum  stack[UQ_NSTACK];  /* the stack */
static Datum *stackp = stack + UQ_NSTACK; /* next free cell on stack */

#ifndef  UQ_NPROG
#warning UQ_NPROG debe definirse en config.mk
#define  UQ_NPROG 2000 /* 2000 celdas para instrucciones */
#endif

Cell  prog[UQ_NPROG];  /* the machine */
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

#ifndef  UQ_NFRAME
#warning UQ_NFRAME debe definirse en config.mk
#define  UQ_NFRAME 10000
#endif

Frame  frame[UQ_NFRAME];
Frame *fp     = frame + UQ_NFRAME;      /* frame pointer */

static Datum *getarg(int arg);    /* return a pointer to argument */

void initcode(void)  /* initalize for code generation */
{
    progp     = progbase;
    stackp    = stack + UQ_NSTACK;
    fp        = frame + UQ_NFRAME;
    returning = 0;
} /* initcode */

int stacksize(void) /* return the stack size */
{
    return stack + UQ_NSTACK - stackp;
} /* stacksize */

void push(Datum d)  /* push d onto stack */
{
    /*  Verificamos si el puntero apunta a una direccion mas alla
        del final de la pila  */
    if (stackp <= stack)
        execerror("stack overflow");
    *--stackp = d;
}

Datum pop(void)    /* pops Datum and rturn top element from stack */
{
    if (stackp == stack + UQ_NSTACK)
        execerror("stack empty");
    return *stackp++;
}

Datum top(void)   /* returns the top of the stack */
{
    if (stackp == stack + UQ_NSTACK)
        execerror("stack empty");
    return *stackp;
}

Cell *code_inst(instr_code ins, ...) /* install one instruction of operand */
{
    Cell *old_progp = progp;

    if (ins >= instruction_set_len) {
        execerror("invalid instruction code [%d]",
            ins);
    }
    if (progp >= prog + UQ_NPROG) {
        execerror("program too big (max=%d)",
            UQ_NPROG);
    }

    const instr *i = instruction_set + ins;

    PRG("[%04lx]: %s", old_progp - prog, i->name);

    progp->inst = ins; /* instalamos la instruccion */

    if (i->prog != NULL) { /* si hay mas */
        va_list args;
        va_start(args, ins);

        progp += i->prog(i, old_progp, args);
        va_end(args);
    }
    PRG("\n");

    progp++;

    return old_progp;
}

#if 0 /* {{ */
Cell *code_sym(Symbol *s) /* install one instruction of operand */
{
    Cell *old_progp = progp;

    if (progp >= &prog[UQ_NPROG])
        execerror("program too big");
    PRG("      : Symbol '%s'\n", s->name);

    (progp++)->sym = s;
    return old_progp;
}

Cell *code_val(double val) /* install one instruction of operand */
{
    Cell *old_progp = progp;

    if (progp >= &prog[UQ_NPROG])
        execerror("program too big");
    PRG("      : DATA %.10g\n", val);

    (progp++)->val = val;
    return old_progp;
}

Cell *code_cel(Cell *cel) /* install one reference to Cell */
{
    Cell *old_progp = progp;

    if (progp >= &prog[UQ_NPROG])
        execerror("program too big");
    PRG("      : REF [%04x]\n",
            (int)(cel ? cel - prog : 0));

    (progp++)->cel = cel;
    return old_progp;
}

Cell *code_num(int val) /* install one integer on Cell */
{
    Cell *old_progp = progp;
    if (progp >= prog + UQ_NPROG)
        execerror("program too big");
    PRG("      : INT [%d]\n", val);
    (progp++)->num = val;
    return old_progp;
}

Cell *code_str(const char *str) /* install one string on Cell */
{
    Cell *old_progp = progp;
    if (progp >= prog + UQ_NPROG)
        execerror("program too big");
    PRG("      : STR \"%s\"\n", str);
    (progp++)->str = str;
    return old_progp;
}
#endif /* }} */

void execute(Cell *p) /* run the machine */
{
    EXEC(BRIGHT YELLOW " BEGIN [%04lx]" ANSI_END "\n", (p - prog));
    for (   pc = p;
            pc->inst != INST_STOP
                && !returning;
        )
    {
        const instr *ins = instruction_set + pc->inst;
        EXEC("[%04lx]: %s", pc - prog, ins->name);
        pc++;
        ins->exec(ins);
        P_TAIL("\n");
    }
    EXEC(" " BRIGHT YELLOW "END [%04lx]\n" ANSI_END, (pc - prog));
}

void drop(const instr *i) /* drops the top of stack */
{
    pop();
}

void drop_prt(const instr *i, const Cell **pc)
{
    PR("\n");
}

void constpush(const instr *i) /* push constant onto stack */
{
    Datum d = (pc++)->val;
    P_TAIL(": -> %.8lg", d);
    push(d);
}

/* push constant onto stack */
void constpush_prt(const instr *i, const Cell **pc)
{
    PR("%2.8g\n", (*pc)[1].val);
    (*pc)++;
}

int datum_prog(const instr *i, Cell *pc, va_list args)
{
    Datum d = progp[1].val = va_arg(args, Datum);

    PRG(" %.8lg", d);
    return 1;
}

void add(const instr *i) /* add top two elements on stack */
{
    Datum p2 = pop(),
          p1 = pop(),
          res = p1 + p2;
    P_TAIL(": %lg + %lg -> %lg",
            p1, p2, res);
    push(res);
}

void add_prt(const instr *i, const Cell **pc)
{
    PR("\n");
}

void sub(const instr *i) /* subtract top two elements on stack */
{
    Datum p2 = pop(),
          p1 = pop(),
          res = p1 - p2;
    P_TAIL(": %lg - %lg -> %lg",
            p1, p2, res);
    push(res);
}

void sub_prt(const instr *i, const Cell **pc)
{
    PR("\n");
}

void mul(const instr *i) /* multiply top two elements on stack */
{
    Datum p2 = pop(),
          p1 = pop(),
          res = p1 * p2;
    P_TAIL(": %lg * %lg -> %lg",
            p1, p2, res);
    push(res);
}

void mul_prt(const instr *i, const Cell **pc)
{
    PR("\n");
}

void divi(const instr *i) /* divide top two elements on stack */
{
    Datum p2  = pop(),
          p1  = pop(),
          res = p1 / p2;
    P_TAIL(": %lg / %lg -> %lg",
            p1, p2, res);
    push(res);
}

void divi_prt(const instr *i, const Cell **pc)
{
    PR("\n");
}

void mod(const instr *i) /* mod top two elements on stack */
{
    Datum p2  = pop(),
          p1  = pop(),
          res = fmod(p1, p2);
    P_TAIL(": %lg %% %lg -> %lg",
            p1, p2, res);
    push(res);
}

void mod_prt(const instr *i, const Cell **pc)
{
    PR("\n");
}

void neg(const instr *i) /* change sign top element on stack */
{
    Datum d   = pop(),
          res = -d;
    P_TAIL(": d=%lg -> %lg",
            d, res);
    push(res);
}

void neg_prt(const instr *i, const Cell **pc)
{
    PR("\n");
}

void pwr(const instr *i) /* pow top two elements on stack */
{
    Datum e  = pop(),
          b  = pop(),
          res = pow(b, e);
    P_TAIL(": b=%lg, e=%lg -> %lg", b, e, res);
    push(res);
}

void pwr_prt(const instr *i, const Cell **pc)
{
    PR("\n");
}

void eval(const instr *i) /* evaluate variable on stack */
{
    Symbol *sym = (pc++)->sym;
    if (sym->type == UNDEF)
        execerror("undefined variable '%s'",
                sym->name);
    P_TAIL(": %s -> %lg",
        sym->name, sym->val);
    push(sym->val);
}

void eval_prt(const instr *i, const Cell **pc)
{
    PR("'%s'\n", (*pc)[1].sym->name);
	(*pc)++;
}

int symb_prog(const instr *i, Cell *pc, va_list args)
{
    Symbol *s = progp[1].sym = va_arg(args, Symbol *);

    PRG(" '%s'", s->name);
    return 1;
}

void assign(const instr *i) /* assign top value to next value */
{
    Symbol *sym = (pc++)->sym;
    Datum   d   = pop();
    P_TAIL(": %.8lg -> %s", d, sym->name);
    sym->val    = d;
    sym->type   = VAR;
    push(d);
}

void assign_prt(const instr *i, const Cell **pc)
{
    PR("'%s'\n", (*pc)[1].sym->name);
	(*pc)++;
}

void print(const instr *i) /* pop top value from stack, print it */
{
    Datum d = pop();
    printf("\t\t%32.8g\n", d);
}

void print_prt(const instr *i, const Cell **pc)
{
    PR("\n");
}

void bltin0(const instr *i) /* evaluate built-in on top of stack */
{
    Symbol *sym = (pc++)->sym;
    Datum   res = sym->ptr0();
    P_TAIL(": %s() -> %.8lg",
        sym->name, res);
    push(res);
}

void bltin0_prt(const instr *i, const Cell **pc)
{
    PR("'%s'\n", (*pc)[1].sym->name);
    (*pc)++;
}

void bltin1(const instr *i) /* evaluate built-in with one argument */
{
    Symbol *sym = (pc++)->sym;
    Datum p   = pop(),
          res = sym->ptr1( p );
    P_TAIL(": %s(%.8lg) -> %.8lg",
        sym->name, p, res);
    push(res);
}

void bltin1_prt(const instr *i, const Cell **pc)
{
    PR("'%s'\n", (*pc)[1].sym->name);
    (*pc)++;
}

void bltin2(const instr *i) /* evaluate built-in with two arguments */
{
    Symbol *sym = (pc++)->sym;
    Datum p2 = pop(),
          p1 = pop(),
         res = sym->ptr2( p1, p2 );
    P_TAIL(": %s(%.8lg, %.8lg) -> %.8lg",
        sym->name, p1, p2, res);
    push(res);
}

void bltin2_prt(const instr *i, const Cell **pc)
{
    PR("'%s'\n", (*pc)[1].sym->name);
    (*pc)++;
}
void ge(const instr *i) /* greater or equal */
{
    Datum p2  = pop(),
          p1  = pop(),
          res = p1 >= p2;
    P_TAIL(": %lg >= %lg -> %lg",
            p1, p2, res);
    push(res);
}

void ge_prt(const instr *i, const Cell **pc)
{
    PR("\n");
}

void le(const instr *i) /* less or equal */
{
    Datum p2  = pop(),
          p1  = pop(),
          res = p1 <= p2;
    P_TAIL(": %lg <= %lg -> %lg",
            p1, p2, res);
    push(res);
}

void le_prt(const instr *i, const Cell **pc)
{
    PR("\n");
}

void gt(const instr *i) /* greater than */
{
    Datum p2  = pop(),
          p1  = pop(),
          res = p1 > p2;
    P_TAIL(": %lg > %lg -> %lg",
            p1, p2, res);
    push(res);
}

void gt_prt(const instr *i, const Cell **pc)
{
    PR("\n");
}

void lt(const instr *i) /* less than */
{
    Datum p2  = pop(),
          p1  = pop(),
          res = p1 < p2;
    P_TAIL(": %lg < %lg -> %lg",
            p1, p2, res);
    push(res);
}

void lt_prt(const instr *i, const Cell **pc)
{
    PR("\n");
}

void eq(const instr *i) /* equal */
{
    Datum p2  = pop(),
          p1  = pop(),
          res = p1 == p2;
    P_TAIL(": %lg == %lg -> %lg",
            p1, p2, res);
    push(res);
}

void eq_prt(const instr *i, const Cell **pc)
{
    PR("\n");
}

void ne(const instr *i) /* not equal */
{
    Datum p2  = pop(),
          p1  = pop(),
          res = p1 != p2;
    P_TAIL(": %lg != %lg -> %lg",
            p1, p2, res);
    push(res);
}

void ne_prt(const instr *i, const Cell **pc)
{
    PR("\n");
}

void not(const instr *i) /* not */
{
    Datum p  = pop(),
          res = ! p;
    P_TAIL(": ! %lg -> %lg",
            p, res);
    push(res);
}

void not_prt(const instr *i, const Cell **pc)
{
    PR("\n");
}

void and(const instr *i)  /* and */
{
    Datum p2  = pop(),
          p1  = pop(),
          res = p1 && p2;
    P_TAIL(": %lg && %lg -> %lg",
            p1, p2, res);
    push(res);
}

void and_prt(const instr *i, const Cell **pc)
{
    PR("\n");
}

void and_then(const instr *i)  /* and_then */
{
    Datum d = top();
    if (d) {
        pc++;
        pop();
    } else {
        pc = pc[0].cel;
    }
    const char *op = d ? " drop;" : "";
    P_TAIL(": %lg &&%s -> [%04lx]",
        d, op, pc - prog);
}

void and_then_prt(const instr *i, const Cell **pc)
{
    PR("[%04lx]\n", (*pc)[1].cel - prog);
    (*pc)++;
}

void or(const instr *i)               /* or */
{
    Datum p2  = pop(),
          p1  = pop(),
          res = p1 || p2;
    P_TAIL(": %lg || %lg -> %lg",
            p1, p2, res);
    push(res);
}

void or_prt(const instr *i, const Cell **pc)
{
    PR("\n");
}

void or_else(const instr *i)  /* or_else */
{
    Datum d = top();
    if (!d) {
        pc++;
        pop();
    } else {
        pc = pc[0].cel;
    }
    const char *op = d ? " drop;" : "";
    P_TAIL(": %lg ||%s -> [%04lx]",
        d, op, pc - prog);
}

void or_else_prt(const instr *i, const Cell **pc)
{
    PR("[%04lx]\n", (*pc)[1].cel - prog);
    (*pc)++;
}

void readopcode(const instr *i)  /* readopcode */
{
    Symbol *sym = (pc++)->sym;
    if (scanf("%lg", &sym->val) != 1) {
        execerror("Lectura incorrecta del valor "
                  "de la variable %s",
                  sym->name);
    }
    P_TAIL(": %.8lg -> %s",
        sym->val, sym->name);
    sym->type   = VAR;
    push(sym->val);
}

void readopcode_prt(const instr *i, const Cell **pc)
{
    PR("'%s'\n", (*pc)[1].sym->name);
	(*pc)++;
}

void end_define(void)
{
    /* adjust progbase to point to the code starting point */
    progbase = progp;     /* next code starts here */
}

Cell *define(Symbol *symb, int type)
{
    if (symb->type != UNDEF) {
        execerror("symbol redefinition not allowed (%s)",
                symb->name);
    }
    symb->type = type;
    symb->defn = progp;

    return progp;
}

void call(const instr *i)   /* call a function */
{
    Symbol *sym  = pc[0].sym;

    if (fp == frame) {
        execerror("Llamada a '%s' demasiado profunda (%d niveles)",
            sym->name, UQ_NFRAME);
    }

    fp--;
    /* creamos el contexto de la funcion */
    fp->sym      = sym;
    fp->nargs    = pc[1].num;
    fp->retpc    = pc + 2;
    fp->argn     = stackp; /* pointer to last argument */

    static int max_niv = 0;
    int niv = frame + UQ_NFRAME - fp;
    if (niv > max_niv) max_niv = niv;

    P_TAIL(": -> execute @[%04lx], %s '%s', args=%d, ret_addr=[%04lx], niv=%d/%d\n",
        sym->defn - prog, sym->type == FUNCTION ? "func" : "proc",
        sym->name, fp->nargs, fp->retpc - prog, niv, max_niv);
    EXEC(  "              -> Parametros: ");
    const char *sep = "";
    for (int i = 1; i <= fp->nargs; i++) {
        P_TAIL("%s%.8g", sep, *getarg(i));
        sep = ", ";
    }
    P_TAIL("\n");
    execute(sym->defn);

    if (fp >= frame + UQ_NFRAME) {
        execerror("Smatching stack, la pila esta corrompida");
    }
    EXEC(": <- return from @[%04lx], %s '%s', args=%d, ret_addr=[%04lx], niv=%d/%d",
        sym->defn - prog, sym->type == FUNCTION ? "func" : "proc",
        sym->name, fp->nargs, fp->retpc - prog, niv, max_niv);
    pc        = fp->retpc;
    returning = 0;
    ++fp;
} /* call */

void call_prt(const instr *i, const Cell **pc)
{
    PR("'%s', args=%ld -> [%04lx]\n",
        (*pc)[1].sym->name,
		(*pc)[2].num,
        (*pc)[1].sym->defn - prog);
	(*pc) += 2;
}

int symb_int_prog(const instr *i, Cell *pc, va_list args)
{
    Symbol *symb = pc[1].sym = va_arg(args, Symbol *);
    int     narg = pc[2].num = va_arg(args, int);
    PRG(" '%s' <%d>", symb->name, narg);
    return 2;
}

static void ret(void)
{
#if 0
    for (int n = 0; n < fp->nargs; n++) {
        pop(); /* pop arguments */
    }
#else
    stackp += fp->nargs;
#endif

    returning = 1;
}

void procret(const instr *i) /* return from proc */
{
    ret();
}

void procret_prt(const instr *i, const Cell **pc)
{
    PR("\n");
}

void funcret(const instr *i) /* return from func */
{
    Datum d = pop();
    P_TAIL(": -> %lg", d);
    ret();
    push(d);
}

void funcret_prt(const instr *i, const Cell **pc)
{
    PR("\n");
}

static Datum *getarg(int arg)    /* return a pointer to argument */
{
    if (arg > fp->nargs) {
        execerror("Accessing arg $%d while only %d "
            "args have been passed",
            arg, fp->nargs);
    }
    return fp->argn + fp->nargs - arg;
}

void argeval(const instr *i) /* push argument onto stack */
{
    int arg = pc++[0].num;
    Datum d = *getarg(arg);
    P_TAIL(": $%d -> %.8g", arg, d);
    push(d);
}

void argeval_prt(const instr *i, const Cell **pc)
{
    PR("$%ld\n", (*pc)[1].num);
	(*pc)++;
}

int arg_prog(const instr *i, Cell *pc, va_list args)
{
    int n = pc[1].num = va_arg(args, int);
    PRG(" $%d", n);
    return 1;
}

void argassign(const instr *i) /* store top of stack in argument */
{
    int arg = pc++->num;
    Datum d = pop();
    P_TAIL(": %.8g -> $%d", d, arg);
    Datum *ref = getarg(arg);
    push(*ref = d);
}

void argassign_prt(const instr *i, const Cell **pc)
{
    PR("$%ld\n", (*pc)[1].num);
	(*pc)++;
}

void prstr(const instr *i) /* print string */
{
    const char *s = pc++->str;
    P_TAIL(": \"%s\"\n", s);
    printf("%s", s);
}

void prstr_prt(const instr *i, const Cell **pc)
{
    PR("\"%s\"\n", (*pc)[1].str);
	(*pc)++;
}

int str_prog(const instr *i, Cell *pc, va_list args)
{
    const char *str
        = progp[1].str
        = va_arg(args, const char *);

    PRG(" \"%s\"", str);
    return 1;
}

void prexpr(const instr *i)  /* print numeric value */
{
    P_TAIL("\n");
    printf("%.8g", pop());
}

void prexpr_prt(const instr *i, const Cell **pc)
{
    PR("\n");
}

void symbs(const instr *i)
{
    P_TAIL("\n");
    list_symbols();
}

void symbs_prt(const instr *i, const Cell **pc)
{
    PR("\n");
}

void STOP(const instr *i)
{
}

void STOP_prt(const instr *i, const Cell **pc)
{
    PR("\n");
}

void list(const instr *i)
{
	const Cell *pc = prog;

    P_TAIL("\n");
	while (pc < progp) {
		const instr *i = instruction_set + pc->inst;
		i->print(i, &pc);
        pc++;
	}
}

void list_prt(const instr *i, const Cell **pc)
{
    PR("\n");
}

void if_f_goto(const instr *i) /* jump if false */
{
    Datum d = pop();
    pc = d ? pc + 1 : pc[0].cel;
    P_TAIL(": -> [%04lx]", pc - prog);
}

void if_f_goto_prt(const instr *i, const Cell **pc)
{
    PR("[%04lx]\n", (*pc)[1].cel - prog);
	(*pc)++;
}

int addr_prog(const instr *i, Cell *pc, va_list args)
{
    Cell *dest = progp[1].cel = va_arg(args, Cell *);

    PRG(" [%04lx]", dest - prog);
    return 1;
}

void Goto(const instr *i) /* jump if false */
{
    pc = pc[0].cel;
    P_TAIL(": -> [%04lx]", pc - prog);
}

void Goto_prt(const instr *i, const Cell **pc)
{
    PR("[%04lx]\n", (*pc)[1].cel - prog);
	(*pc)++;
}


void noop(const instr *i)
{
}

void noop_prt(const instr *i, const Cell **pc)
{
    PR("\n");
}
