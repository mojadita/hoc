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

#ifndef  UQ_CODE_DEBUG_P1
#warning UQ_CODE_DEBUG_P1 deberia ser incluido en config.mk
#define  UQ_CODE_DEBUG_P1 1
#endif
#ifndef  UQ_CODE_DEBUG_P2
#warning UQ_CODE_DEBUG_P2 deberia ser incluido en config.mk
#define  UQ_CODE_DEBUG_P2 1
#endif

#if      UQ_CODE_DEBUG_P1 /* {{ */
#define  P(_fmt, ...)                                 \
    printf("%s:%d:[%04x] %s"_fmt, __FILE__, __LINE__, \
        (int)(pc - prog - 1), __func__, ##__VA_ARGS__)
#define  P_TAIL(_fmt, ...)      \
    printf(_fmt, ##__VA_ARGS__)
#define  PR(_fmt, ...)       \
    printf("[%04lx] %s"_fmt, \
        pc - prog,           \
        i->name,             \
        ##__VA_ARGS__)
#else /* UQ_CODE_DEBUG_P1    }{ */
#define P(_fmt, ...)
#define P_TAIL(_fmt, ...)
#endif /* UQ_CODE_DEBUG_P1   }} */

#if UQ_CODE_DEBUG_P2
#define P2(_fmt, ...) printf(_fmt, ##__VA_ARGS__)
#else
#define P2(_fmt, ...)
#endif

#ifndef  UQ_NSTACK
#warning UQ_NSTACK debe definirse en config.mk
#define  UQ_NSTACK 100000
#endif

static Datum  stack[UQ_NSTACK];  /* the stack */
static Datum *stackp;         /* next free cell on stack */

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
    stackp    = stack;
    fp        = frame + UQ_NFRAME;
    returning = 0;
} /* initcode */

int stacksize(void) /* return the stack size */
{
    return stackp - stack;
} /* stacksize */

void push(Datum d)  /* push d onto stack */
{
    /*  Verificamos si el puntero apunta a una direccion mas alla
        del final de la pila  */
    if (stackp >= &stack[UQ_NSTACK])
        execerror("stack overflow\n");
    *stackp++ = d; /* equivalente a *stackp = d; stackp++; */
}

Datum pop(void)    /* pops Datum and rturn top element from stack */
{
    if (stackp == stack)
        execerror("stack empty\n");
    return *--stackp;
}

Cell *code_inst(const instr *ins) /* install one instruction of operand */
{
    Cell *old_progp = progp;

    if (progp >= &prog[UQ_NPROG])
        execerror("program too big");

    P2("0x%04x: %s\n", (int)(old_progp - prog), ins->name);

    (progp++)->inst = ins;
    return old_progp;
}

Cell *code_sym(Symbol *s) /* install one instruction of operand */
{
    Cell *old_progp = progp;

    if (progp >= &prog[UQ_NPROG])
        execerror("program too big");
    P2("      : Symbol '%s'\n", s->name);

    (progp++)->sym = s;
    return old_progp;
}

Cell *code_val(double val) /* install one instruction of operand */
{
    Cell *old_progp = progp;

    if (progp >= &prog[UQ_NPROG])
        execerror("program too big");
    P2("      : DATA %.10g\n", val);

    (progp++)->val = val;
    return old_progp;
}

Cell *code_cel(Cell *cel) /* install one reference to Cell */
{
    Cell *old_progp = progp;

    if (progp >= &prog[UQ_NPROG])
        execerror("program too big");
    P2("      : REF [0x%04x]\n",
            (int)(cel ? cel - prog : 0));

    (progp++)->cel = cel;
    return old_progp;
}

Cell *code_num(int val) /* install one integer on Cell */
{
    Cell *old_progp = progp;
    if (progp >= prog + UQ_NPROG)
        execerror("program too big");
    P2("      : INT [%d]\n", val);
    (progp++)->num = val;
    return old_progp;
}

Cell *code_str(const char *str) /* install one string on Cell */
{
    Cell *old_progp = progp;
    if (progp >= prog + UQ_NPROG)
        execerror("program too big");
    P2("      : STR \"%s\"\n", str);
    (progp++)->str = str;
    return old_progp;
}

void execute(Cell *p) /* run the machine */
{
    P(" " BRIGHT YELLOW "BEGIN [%04lx]" ANSI_END "\n", (p - prog));
    for (   pc = p;
            pc->inst != &STOP_instr && !returning;
        )
    {
        (pc++)->inst->exec(p->inst);
    }
    P(" " BRIGHT YELLOW "END [%04lx]" ANSI_END "\n", (pc - prog));
}

void drop(const instr *i) /* drops the top of stack */
{
    P("\n");
    pop();
}

void drop_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void constpush(const instr *i) /* push constant onto stack */
{
    P("\n");
    Datum d = (pc++)->val;
    P(": -> %.8lg\n", d);
    push(d);
}

/* push constant onto stack */
void constpush_prt(const instr *i, const Cell *pc)
{
    PR(" %.8g\n", pc[1].val);
}

void add(const instr *i) /* add top two elements on stack */
{
    P("\n");
    Datum p2 = pop(),
          p1 = pop(),
          res = p1 + p2;
    P(": %lg + %lg -> %lg\n",
            p1, p2, res);
    push(res);
}

void add_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void sub(const instr *i) /* subtract top two elements on stack */
{
    P("\n");
    Datum p2 = pop(),
          p1 = pop(),
          res = p1 - p2;
    P(": %lg - %lg -> %lg\n",
            p1, p2, res);
    push(res);
}

void sub_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void mul(const instr *i) /* multiply top two elements on stack */
{
    P("\n");
    Datum p2 = pop(),
          p1 = pop(),
          res = p1 * p2;
    P(": %lg * %lg -> %lg\n",
            p1, p2, res);
    push(res);
}

void mul_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void divi(const instr *i) /* divide top two elements on stack */
{
    P("\n");
    Datum p2  = pop(),
          p1  = pop(),
          res = p1 / p2;
    P(": %lg / %lg -> %lg\n",
            p1, p2, res);
    push(res);
}

void divi_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void mod(const instr *i) /* mod top two elements on stack */
{
    P("\n");
    Datum p2  = pop(),
          p1  = pop(),
          res = fmod(p1, p2);
    P(": %lg %% %lg -> %lg\n",
            p1, p2, res);
    push(res);
}

void mod_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void neg(const instr *i) /* change sign top element on stack */
{
    P("\n");
    Datum d   = pop(),
          res = -d;
    P(": d=%lg -> %lg\n",
            d, res);
    push(res);
}

void neg_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void pwr(const instr *i) /* pow top two elements on stack */
{
    P("\n");
    Datum p2  = pop(),
          p1  = pop(),
          res = pow(p1, p2);
    P(": p1=%lg, d2=%lg -> %lg\n",
            p1, p2, res);
    push(res);
}

void pwr_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void eval(const instr *i) /* evaluate variable on stack */
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

void eval_prt(const instr *i, const Cell *pc)
{
    PR(" '%s'\n", pc[1].sym->name);
}

void assign(const instr *i) /* assign top value to next value */
{
    P("\n");
    Symbol *sym = (pc++)->sym;
    Datum   d   = pop();
    P(": %.8lg -> %s\n", d, sym->name);
    sym->val    = d;
    sym->type   = VAR;
    push(d);
}

void assign_prt(const instr *i, const Cell *pc)
{
    PR(" '%s'\n", pc[1].sym->name);
}

void print(const instr *i) /* pop top value from stack, print it */
{
    P("\n");
    Datum d = pop();
    printf("\t\t%32.8g\n", d);
}

void print_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void bltin0(const instr *i) /* evaluate built-in on top of stack */
{
    P("\n");
    Symbol *sym = (pc++)->sym;
    Datum   res = sym->ptr0();
    P(": %s() -> %.8lg\n",
        sym->name, res);
    push(res);
}

void bltin0_prt(const instr *i, const Cell *pc)
{
    PR(" '%s'\n", pc[1].sym->name);
}

void bltin1(const instr *i) /* evaluate built-in with one argument */
{
    P("\n");
    Symbol *sym = (pc++)->sym;
    Datum p   = pop(),
          res = sym->ptr1( p );
    P(": %s(%.8lg) -> %.8lg\n",
        sym->name, p, res);
    push(res);
}

void bltin1_prt(const instr *i, const Cell *pc)
{
    PR(" '%s'\n", pc[1].sym->name);
}

void bltin2(const instr *i) /* evaluate built-in with two arguments */
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

void bltin2_prt(const instr *i, const Cell *pc)
{
    PR(" '%s'\n", pc[1].sym->name);
}

void whilecode(const instr *i) /* execute the while loop */
{
    P("\n");
    Cell *savepc = pc;  /* savepc[0] loop body address */
                        /* savepc[1] is the loop end address */
                        /* savepc + 2 is the cond starting point */
    P(" execute cond code [%04lx]\n", savepc + 2 - prog);
    execute(savepc + 2); /* execute cond code */
    Datum d = pop();     /* pop the boolean data */
    while (d != 0) {
        P(" execute body code [%04lx]\n", savepc[0].cel - prog);
        execute(savepc[0].cel); /* execute body */
        P(" execute cond code [%04lx]\n", savepc + 2 - prog);
        execute(savepc + 2);    /* execute cond again */
        d = pop();
    }
    P(" END while loop");
    if (!returning) {
        P_TAIL(" [%04lx]", savepc[1].cel - prog);
        pc = savepc[1].cel;
    }
    P_TAIL("\n");
}

void whilecode_prt(const instr *i, const Cell *pc)
{
    PR(" [%04lx], [%04lx]\n",
        pc[1].cel - prog,
        pc[2].cel - prog);
}

void ifcode(const instr *i) /* execute the if statement */
{
    P("\n");
    Cell *savepc = pc;  /* savepc[0] then statement address */
                        /* savepc[1] else statement address */
                        /* savepc[2] next statement address */
                        /* savepc + 3 first cond evaluation instruction */
    P(" execute cond code [%04lx]\n", savepc + 3 - prog);
    execute(savepc + 3); /* execute cond code */
    Datum d = pop();    /* pop the boolean data */
    if (d) {
        P(" execute THEN code [%04lx]\n", savepc[0].cel - prog);
        execute(savepc[0].cel);
    } else if (savepc[1].cel) {
        P(" execute ELSE code [%04lx]\n", savepc[1].cel - prog);
        execute(savepc[1].cel);
    }
    P(" END");
    if (!returning) {
        P_TAIL(" [%04lx]", savepc[2].cel - prog);
        pc = savepc[2].cel;
    }
    P_TAIL("\n");
}

void ifcode_prt(const instr *i, const Cell *pc)
{
    PR(" [%04lx], [%04lx], [%04lx]\n",
        pc[1].cel - prog,
        pc[2].cel - prog,
        pc[3].cel - prog);
}

void ge(const instr *i) /* greater or equal */
{
    P("\n");
    Datum p2  = pop(),
          p1  = pop(),
          res = p1 >= p2;
    P(": %lg >= %lg -> %lg\n",
            p1, p2, res);
    push(res);
}

void ge_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void le(const instr *i) /* less or equal */
{
    P("\n");
    Datum p2  = pop(),
          p1  = pop(),
          res = p1 <= p2;
    P(": %lg <= %lg -> %lg\n",
            p1, p2, res);
    push(res);
}

void le_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void gt(const instr *i) /* greater than */
{
    P("\n");
    Datum p2  = pop(),
          p1  = pop(),
          res = p1 > p2;
    P(": %lg > %lg -> %lg\n",
            p1, p2, res);
    push(res);
}

void gt_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void lt(const instr *i) /* less than */
{
    P("\n");
    Datum p2  = pop(),
          p1  = pop(),
          res = p1 < p2;
    P(": %lg < %lg -> %lg\n",
            p1, p2, res);
    push(res);
}

void lt_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void eq(const instr *i) /* equal */
{
    P("\n");
    Datum p2  = pop(),
          p1  = pop(),
          res = p1 == p2;
    P(": %lg == %lg -> %lg\n",
            p1, p2, res);
    push(res);
}

void eq_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void ne(const instr *i) /* not equal */
{
    P("\n");
    Datum p2  = pop(),
          p1  = pop(),
          res = p1 != p2;
    P(": %lg != %lg -> %lg\n",
            p1, p2, res);
    push(res);
}

void ne_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void not(const instr *i) /* not */
{
    P("\n");
    Datum p  = pop(),
          res = ! p;
    P(": ! %lg -> %lg\n",
            p, res);
    push(res);
}

void not_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void and(const instr *i)  /* and */
{
    P("\n");
    Datum p2  = pop(),
          p1  = pop(),
          res = p1 && p2;
    P(": %lg && %lg -> %lg\n",
            p1, p2, res);
    push(res);
}

void and_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void or(const instr *i)               /* or */
{
    P("\n");
    Datum p2  = pop(),
          p1  = pop(),
          res = p1 || p2;
    P(": %lg || %lg -> %lg\n",
            p1, p2, res);
    push(res);
}

void or_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void readopcode(const instr *i)  /* readopcode */
{
    P("\n");
    Symbol *sym = (pc++)->sym;
    if (scanf("%lg", &sym->val) != 1) {
        execerror("Lectura incorrecta del valor "
                  "de la variable %s\n",
                  sym->name);
    }
    P(": %.8lg -> %s\n", sym->val, sym->name);
    sym->type   = VAR;
    push(sym->val);
}

void readopcode_prt(const instr *i, const Cell *pc)
{
    PR(" '%s'\n", pc[1].sym->name);
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

void call(const instr *i)   /* call a function */
{
    P("\n");

    Symbol *sym  = pc[0].sym;

    if (fp == frame) {
        execerror("Llamada a '%s' demasiado profunda (%d niveles)\n",
            sym->name, UQ_NFRAME);
    }

    fp--;
    /* creamos el contexto de la funcion */
    fp->sym      = sym;
    fp->nargs    = pc[1].num;
    fp->retpc    = pc + 2;
    fp->argn     = stackp - 1; /* pointer to last argument */

    static int max_niv = 0;
    int niv = frame + UQ_NFRAME - fp;
    if (niv > max_niv) max_niv = niv;

    P(" execute @[0x%04x], %s '%s', args=%d, ret_addr=0x%04x, niv=%d/%d\n",
        (int)(sym->defn - prog), sym->type == FUNCTION ? "func" : "proc",
        sym->name, fp->nargs, (int)(fp->retpc - prog), niv, max_niv);
    P("   Parametros: ");
    const char *sep = "";
    for (int i = 1; i <= fp->nargs; i++) {
        P_TAIL("%s%.8g", sep, *getarg(i));
        sep = ", ";
    }
    P_TAIL("\n");
    execute(sym->defn);

    if (fp >= frame + UQ_NFRAME) {
        execerror("Smatching stack, la pila esta corrompida\n");
    }
    P(" return from @[0x%04x], %s '%s', args=%d, ret_addr=0x%04x, niv=%d/%d\n",
        (int)(sym->defn - prog), sym->type == FUNCTION ? "func" : "proc",
        sym->name, fp->nargs, (int)(fp->retpc - prog), niv, max_niv);
    pc        = fp->retpc;
    returning = 0;
    ++fp;
} /* call */

void call_prt(const instr *i, const Cell *pc)
{
    PR(" '%s' %ld\n",
        pc[1].sym->name,
		pc[2].num);
}

static void ret(void)
{
    for (int n = 0; n < fp->nargs; n++) {
        pop(); /* pop arguments */
    }

    returning = 1;
}

void procret(const instr *i) /* return from proc */
{
    P("\n");
    ret();
}

void procret_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void funcret(const instr *i) /* return from func */
{
    P("\n");
    Datum d = pop();  /* preserve function return value */
    ret();
    push(d);
}

void funcret_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

static Datum *getarg(int arg)    /* return a pointer to argument */
{
    if (arg > fp->nargs) {
        execerror("Accessing arg $%d while only %d "
            "args have been passed\n",
            arg, fp->nargs);
    }
    return fp->argn - fp->nargs + arg;
}

void argeval(const instr *i) /* push argument onto stack */
{
    P("\n");
    int arg = pc++[0].num;
    Datum d = *getarg(arg);
    P(" $%d -> %.8g\n", arg, d);
    push(d);
}

void argeval_prt(const instr *i, const Cell *pc)
{
    PR(" $%ld\n", pc[1].num);
}

void argassign(const instr *i) /* store top of stack in argument */
{
    P("\n");
    int arg = pc++[0].num;
    Datum d = pop();
    P(" %.8g -> $%d\n", d, arg);
    Datum *ref = getarg(arg);
    push(*ref = d);
}

void argassign_prt(const instr *i, const Cell *pc)
{
    PR(" $%ld\n", pc[1].num);
}

void prstr(const instr *i) /* print string */
{
    P("\n");
    printf("%s", pc++[0].str);
}

void prstr_prt(const instr *i, const Cell *pc)
{
    PR(" \"%s\"\n", pc[1].str);
}

void prexpr(const instr *i)  /* print numeric value */
{
    P("\n");
    printf("%.8g", pop());
}

void prexpr_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void symbs(const instr *i)
{
    P("\n");
    list_symbols();
}

void symbs_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void STOP_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}
