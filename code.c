/* code.c -- instrucciones y rutinas de traza.
 * Author: Edward Rivas <rivastkw@gmail.com>
 *       y Luis Colorado <luiscoloradourcola@gmail.com>
 * Date: Sat Mar 22 14:20:43 -05 2025
 * Copyright: (c) 2025 Edward Rivas y Luis Colorado.  All rights reserved.
 * License: BSD
 */

#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "config.h"
#include "colors.h"

#include "code.h"
#include "hoc.h"

#include "scope.h"

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
        pc - prog,                  \
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
Cell *varbase  = prog + UQ_NPROG; /* pointer to last global allocated */
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


void initcode(void)  /* initalize for code generation */
{
    progp     = progbase;
    stackp    = stack + UQ_NSTACK;
    fp        = frame + UQ_NFRAME;
    fp--;
    fp->sym   = NULL;
    fp->retpc = NULL;
    fp->argn  = stackp;
    fp->nargs = 0;
    returning = 0;
} /* initcode */

Symbol *register_global_var(
        const char *name,
        Symbol     *typref)
{
    assert(get_current_scope() == NULL);
    if (progp >= varbase) {
        execerror("variables zone exhausted (progp >= varbase)\n");
    }
    if (lookup(name)) {
        execerror("Variable %s already defined\n", name);
    }
    Symbol *sym = install(name, VAR, typref);
    sym->defn = --varbase;
    PRG("Symbol '%s', type=%s, typref=%s, pos=[%04lx]\n",
        sym->name,
        lookup_type(sym->type),
        typref->name,
        sym->defn ? sym->defn - prog : -1);
    return sym;
} /* register_global_var */

Symbol *register_local_var(
        const char *name,
        Symbol     *typref)
{
    scope *scop = get_current_scope();
    assert(scop != NULL);
    if (lookup_current_scope(name)) {
        execerror("Variable %s already defined in current scope\n", name);
    }
    Symbol *sym = install(name, LVAR, typref);
    scop->size += typref->size;
    sym->offset = -(scop->base_offset + scop->size);

    PRG("Symbol '%s', type=%s, typref=%s, offset=%d\n",
        sym->name,
        lookup_type(sym->type),
        typref->name,
        sym->offset);
    return sym;
} /* register_global_var */


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
    if (stackp >= stack + UQ_NSTACK)
        execerror("stack empty");
    return stackp++[0];
}

Datum top(void)   /* returns the top of the stack */
{
    if (stackp == stack + UQ_NSTACK)
        execerror("stack empty");
    return *stackp;
}

Cell *code_inst(instr_code ins, ...) /* install one instruction of operand */
{

    if (ins >= instruction_set_len) {
        execerror("invalid instruction code [%d]",
            ins);
    }
    if (progp >= varbase) {
        execerror("program too big (max=%d)",
            UQ_NPROG);
    }

    const instr *i = instruction_set + ins;

    PRG("[%04lx]: %s", progp - prog, i->name);

    progp->inst = ins; /* instalamos la instruccion */

    if (i->prog != NULL) { /* si hay mas */
        va_list args;
        va_start(args, ins);

        i->prog(i, progp, args);

        va_end(args);
    }
    PRG("\n");

    Cell *ret_val = progp;
    progp        += i->n_cells;

    return ret_val;
}

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
        ins->exec(ins);
        P_TAIL("\n");
    }
    EXEC(" " BRIGHT YELLOW "END [%04lx]\n" ANSI_END, (pc - prog));
}

#define UPDATE_PC() do {   \
        pc += i->n_cells;  \
    } while (0)

void STOP(const instr *i)
{
    UPDATE_PC();
}

void STOP_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void drop(const instr *i) /* drops the top of stack */
{
    pop();
    UPDATE_PC();
}

void drop_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void constpush(const instr *i) /* push constant onto stack */
{
    Datum d = pc[1].val;
    P_TAIL(": -> %.8lg", d);
    push(d);
    UPDATE_PC();
}

/* push constant onto stack */
void constpush_prt(const instr *i, const Cell *pc)
{
    PR("%2.8g\n", pc[1].val);
}

void datum_prog(const instr *i, Cell *pc, va_list args)
{
    Datum d = progp[1].val = va_arg(args, Datum);

    PRG(" %.8lg", d);
}

void add(const instr *i) /* add top two elements on stack */
{
    Datum p2 = pop(),
          p1 = pop(),
          res = p1 + p2;
    P_TAIL(": %lg + %lg -> %lg",
            p1, p2, res);
    push(res);
    UPDATE_PC();
}

void add_prt(const instr *i, const Cell *pc)
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
    UPDATE_PC();
}

void sub_prt(const instr *i, const Cell *pc)
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
    UPDATE_PC();
}

void mul_prt(const instr *i, const Cell *pc)
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
    UPDATE_PC();
}

void divi_prt(const instr *i, const Cell *pc)
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
    UPDATE_PC();
}

void mod_prt(const instr *i, const Cell *pc)
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
    UPDATE_PC();
}

void neg_prt(const instr *i, const Cell *pc)
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
    UPDATE_PC();
}

void pwr_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void eval(const instr *i) /* evaluate variable on stack */
{
    Symbol *sym = pc[1].sym;
    if (sym->type == UNDEF) {
        execerror("undefined variable '%s'",
                sym->name);
    }
    P_TAIL(": %s -> %lg @ [%04lx]", sym->name, sym->defn->val, sym->defn - prog);
    push(sym->defn->val);
    UPDATE_PC();
}

void eval_prt(const instr *i, const Cell *pc)
{
    PR("'%s'\n", pc[1].sym->name);
}

void symb_prog(const instr *i, Cell *pc, va_list args)
{
    Symbol *s = progp[1].sym = va_arg(args, Symbol *);

    PRG(" '%s'", s->name);
}

void assign(const instr *i) /* assign top value to next value */
{
    Symbol *sym = pc[1].sym;
    Datum   d   = pop();

    P_TAIL(": %.8g -> '%s'", d, sym->name);

    sym->defn->val = d;
    if (sym->type == UNDEF) {
        sym->type = VAR;
    }

    push(d);
    UPDATE_PC();
}

void assign_prt(const instr *i, const Cell *pc)
{
    PR("'%s'\n", pc[1].sym->name);
}

void print(const instr *i) /* pop top value from stack, print it */
{
    Datum d = pop();
    printf("\t\t%32.8g\n", d);
    UPDATE_PC();
}

void print_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void bltin0(const instr *i) /* evaluate built-in on top of stack */
{
    Symbol *sym = pc[1].sym;
    Datum   res = sym->ptr0();
    P_TAIL(": %s() -> %.8lg",
        sym->name, res);
    push(res);
    UPDATE_PC();
}

void bltin0_prt(const instr *i, const Cell *pc)
{
    PR("'%s'\n", pc[1].sym->name);
}

void bltin1(const instr *i) /* evaluate built-in with one argument */
{
    Symbol *sym = pc[1].sym;
    Datum p   = pop(),
          res = sym->ptr1( p );
    P_TAIL(": %s(%.8lg) -> %.8lg",
        sym->name, p, res);
    push(res);
    UPDATE_PC();
}

void bltin1_prt(const instr *i, const Cell *pc)
{
    PR("'%s'\n", pc[1].sym->name);
}

void bltin2(const instr *i) /* evaluate built-in with two arguments */
{
    Symbol *sym = pc[1].sym;
    Datum p2 = pop(),
          p1 = pop(),
         res = sym->ptr2( p1, p2 );
    P_TAIL(": %s(%.8lg, %.8lg) -> %.8lg",
        sym->name, p1, p2, res);
    push(res);
    UPDATE_PC();
}

void bltin2_prt(const instr *i, const Cell *pc)
{
    PR("'%s'\n", pc[1].sym->name);
}

void ge(const instr *i) /* greater or equal */
{
    Datum p2  = pop(),
          p1  = pop(),
          res = p1 >= p2;
    P_TAIL(": %lg >= %lg -> %lg",
            p1, p2, res);
    push(res);
    UPDATE_PC();
}

void ge_prt(const instr *i, const Cell *pc)
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
    UPDATE_PC();
}

void le_prt(const instr *i, const Cell *pc)
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
    UPDATE_PC();
}

void gt_prt(const instr *i, const Cell *pc)
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
    UPDATE_PC();
}

void lt_prt(const instr *i, const Cell *pc)
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
    UPDATE_PC();
}

void eq_prt(const instr *i, const Cell *pc)
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
    UPDATE_PC();
}

void ne_prt(const instr *i, const Cell *pc)
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
    UPDATE_PC();
}

void not_prt(const instr *i, const Cell *pc)
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
    UPDATE_PC();
}

void and_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void and_then(const instr *i)  /* and_then */
{
    Datum d = top();
    if (d) {
        pop();
        UPDATE_PC();
    } else {
        pc = prog + pc[0].desp;
    }
    const char *op = d ? " drop;" : "";
    P_TAIL(": %lg &&%s -> [%04lx]",
        d, op, pc - prog);
}

void and_then_prt(const instr *i, const Cell *pc)
{
    PR("[%04x]\n", pc[0].desp);
}

void or(const instr *i)               /* or */
{
    Datum p2  = pop(),
          p1  = pop(),
          res = p1 || p2;
    P_TAIL(": %lg || %lg -> %lg",
            p1, p2, res);
    push(res);
    UPDATE_PC();
}

void or_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void or_else(const instr *i)  /* or_else */
{
    Datum d = top();
    if (!d) {
        pop();
        UPDATE_PC();
    } else {
        pc = prog + pc[0].desp;
    }
    const char *op = d ? " drop;" : "";
    P_TAIL(": %lg ||%s -> [%04lx]",
        d, op, pc - prog);
}

void or_else_prt(const instr *i, const Cell *pc)
{
    PR("[%04x]\n", pc[0].desp);
}

/* se llama al definir una funcion (o procedimiento) */
Symbol *define(
        const char *name,   /* nombre de la funcion/procedimiento */
        int         type,   /* tipo de symbolo (PROCEDURE/FUNCTION) */
        Symbol     *typref, /* simbolo del tipo del valor devuelto por la
                             * funcion, NULL para proc */
        Cell       *entry)  /* punto de entrada a la funcion */
{
    P_TAIL("define(\"%s\", %s, %s, [%04lx]);\n",
            name,
            lookup_type(type),
            typref  ? typref->name
                    : "VOID",
            entry - prog);
    Symbol *symb     = install(name, type, NULL);
    symb->typref     = typref;
    symb->defn       = entry;

    return symb;
}

/* se llama al terminar la definicion de una funcion
 * (o prodecimiento) */
void end_define(Symbol *subr)
{
    /* adjust progbase to point to the code starting point */
    progbase = progp;     /* next code starts here */
    P_TAIL("end_define(%s);\n", subr->name);
}

void symb_int_prog(const instr *i, Cell *pc, va_list args)
{
    Symbol *symb = pc[1].sym  = va_arg(args, Symbol *);
    int     narg = pc[0].args = va_arg(args, int);
                   pc[0].desp = symb->defn - prog;
    PRG(" '%s' <%d> [%04x]", symb->name, narg, pc[0].desp);
}

void call(const instr *i)   /* call a function */
{
    Symbol *sym  = pc[1].sym;

    if (fp == frame) {
        execerror("Llamada a '%s' demasiado profunda (%d niveles)",
            sym->name, UQ_NFRAME);
    }

    fp--;
    /* creamos el contexto de la funcion */
    fp->sym      = sym;
    fp->nargs    = pc[0].args;
    fp->retpc    = pc + i->n_cells;
    fp->argn     = stackp; /* pointer to last argument */

    static int max_niv = 0;
    int niv            = frame + UQ_NFRAME - fp;
    if (niv > max_niv)
            max_niv    = niv;

    P_TAIL(": -> execute @[%04lx], %s '%s', args=%d, ret_addr=[%04lx], niv=%d/%d\n",
        sym->defn - prog, sym->type == FUNCTION ? "func" : "proc",
        sym->name, fp->nargs, fp->retpc - prog, niv, max_niv);
    EXEC(  "              -> Parametros: ");
    const char *sep = "";
    for (int i = 0; i < sym->argums_len; i++) {
        P_TAIL("%s%.8g", sep, *getarg(sym->argums[i]->offset));
        sep = ", ";
    }
    P_TAIL("\n");

    execute(sym->defn);

    if (fp >= frame + UQ_NFRAME) {
        execerror("la pila de frames revento!!!!");
    }
    EXEC(": <- return from @[%04lx], %s '%s', "
         "args=%d, ret_addr=[%04lx], niv=%d/%d",
         sym->defn - prog,
         sym->type == FUNCTION
                ? "func"
                : "proc",
         sym->name,
         fp->nargs,
         fp->retpc - prog,
         niv, max_niv);

    pc        = fp->retpc;
    returning = 0;

    ++fp;
} /* call */

void call_prt(const instr *i, const Cell *pc)
{
    PR("'%s', args=%d -> [%04lx]\n",
        pc[1].sym->name,
        pc[0].args,
        pc[1].sym->defn - prog);
}

void ret(const instr *i) /* return from proc */
{
    returning = 1;
}

void ret_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

Datum *getarg(int arg)    /* return a pointer to argument */
{
    if (arg > fp->nargs) {
        execerror("Accessing arg $%d while only %d "
            "args have been passed to '%s'",
            arg, fp->nargs,
            fp->sym->name);
    }
    return fp->argn + arg;
}

void arg_prog(const instr *i, Cell *pc, va_list args)
{
    int n = pc[0].args = va_arg(args, int);
    PRG(" $%d", n);
}

void argeval(const instr *i) /* push argument onto stack */
{
    int arg = pc[0].args;
    Datum d = *getarg(arg);
    P_TAIL(": $%d -> %.8g", arg, d);
    push(d);
    UPDATE_PC();
}

void argeval_prt(const instr *i, const Cell *pc)
{
    PR("$%d\n", pc[0].args);
}

void argassign(const instr *i) /* store top of stack in argument */
{
    int arg = pc[0].args;
    Datum d = pop();
    P_TAIL(": %.8g -> $%d", d, arg);
    Datum *ref = getarg(arg);
    push(*ref = d);
    UPDATE_PC();
}

void argassign_prt(const instr *i, const Cell *pc)
{
    PR("$%d\n", pc[0].args);
}

void prstr(const instr *i) /* print string */
{
    const char *s = pc[1].str;
    P_TAIL(": \"%s\"\n", s);
    printf("%s", s);
    UPDATE_PC();
}

void prstr_prt(const instr *i, const Cell *pc)
{
    PR("\"%s\"\n", pc[1].str);
}

void str_prog(const instr *i, Cell *pc, va_list args)
{
    const char *str
        = progp[1].str
        = va_arg(args, const char *);

    PRG(" \"%s\"", str);
}

void prexpr(const instr *i)  /* print numeric value */
{
    P_TAIL("\n");
    printf("%.8g", pop());
    UPDATE_PC();
}

void prexpr_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void symbs(const instr *i)
{
    P_TAIL("\n");
    list_symbols();
    UPDATE_PC();
}

void symbs_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void symbs_all(const instr *i)
{
    Symbol *cs = pc[1].sym;
    P_TAIL("\n");
    list_all_symbols(cs);
    UPDATE_PC();
}

void symbs_all_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void brkpt(const instr *i)
{
    Symbol *cs = pc[1].sym;
    P_TAIL("\n");
    list_variables(cs);
    UPDATE_PC();
}

void brkpt_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void list(const instr *i)
{
    const Cell *ip = prog;

    P_TAIL("\n");
    while (ip < progp) {
        const instr *i = instruction_set + ip->inst;
        i->print(i, ip); /* LCU: Thu Apr 10 14:52:23 -05 2025
                          * Aqui es donde Edward desaparecio en el rio Orinoco. */
        ip += i->n_cells;
    }
    UPDATE_PC();
}

void list_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void if_f_goto(const instr *i) /* jump if false */
{
    P_TAIL(": -> [%04x]", pc[0].desp);
    Datum d = pop();
    pc = d ? pc + i->n_cells : prog + pc[0].desp;
}

void if_f_goto_prt(const instr *i, const Cell *pc)
{
    PR("[%04x]\n", pc[0].desp);
}

void addr_prog(const instr *i, Cell *pc, va_list args)
{
    progp[0].desp = va_arg(args, Cell *) - prog;

    PRG(" [%04x]", progp[0].desp);
}

void Goto(const instr *i) /* jump if false */
{
    P_TAIL(": -> [%04x]", pc[0].desp);
    pc = prog + pc[0].desp;
}

void Goto_prt(const instr *i, const Cell *pc)
{
    PR("[%04x]\n", pc[0].desp);
}

void noop(const instr *i)
{
    UPDATE_PC();
}

void noop_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void inceval(const instr *i) /* assign top value to next value */
{
    Symbol *sym = pc[1].sym;
    if (sym->type == UNDEF) {
        execerror("undefined variable '%s' cannot be incremented",
                sym->name);
    }
    push(++sym->defn->val);
    P_TAIL(": ++%s -> %.8lg", sym->name, sym->defn->val);
    UPDATE_PC();
}

void inceval_prt(const instr *i, const Cell *pc)
{
    PR("'%s'\n", pc[1].sym->name);
}

void evalinc(const instr *i) /* assign top value to next value */
{
    Symbol *sym = pc[1].sym;
    if (sym->type == UNDEF) {
        execerror("undefined variable '%s' cannot be incremented",
                sym->name);
    }
    P_TAIL(": %s++ -> %.8lg", sym->name, sym->defn->val);
    push(sym->defn->val++);
    UPDATE_PC();
}

void evalinc_prt(const instr *i, const Cell *pc)
{
    PR("'%s'\n", pc[1].sym->name);
}

void incarg(const instr *i) /* assign top value to next value */
{
    int arg = pc[0].args;

    Datum *ref = getarg(arg);

    push(++ref[0]);
    P_TAIL(": %.8g -> $%d -> %.8g", ref[0], arg, ref[0]);

    UPDATE_PC();
}

void incarg_prt(const instr *i, const Cell *pc)
{
    PR("$%d\n", pc[0].args);
}

void arginc(const instr *i) /* assign top value to next value */
{
    int arg = pc[0].args;

    Datum *ref = getarg(arg);

    Datum d = *ref; /* dato antes de incrementar */

    push(ref[0]++);
    P_TAIL(": %.8g -> $%d -> %lg", ref[0], arg, d);

    UPDATE_PC();
}

void arginc_prt(const instr *i, const Cell *pc)
{
    PR("$%d\n", pc[0].args);
}

void deceval(const instr *i) /* assign top value to next value */
{
    Symbol *sym = pc[1].sym;
    if (sym->type == UNDEF) {
        execerror("undefined variable '%s' cannot be decremented",
                sym->name);
    }
    push(--sym->defn->val);
    P_TAIL(": %s -> %.8lg", sym->name, sym->defn->val);
    UPDATE_PC();
}

void deceval_prt(const instr *i, const Cell *pc)
{
    PR("'%s'\n", pc[1].sym->name);
}

void evaldec(const instr *i) /* assign top value to next value */
{
    Symbol *sym = pc[1].sym;
    if (sym->type == UNDEF) {
        execerror("undefined variable '%s' cannot be decremented",
                sym->name);
    }
    P_TAIL(": %s -> %.8lg", sym->name, sym->defn->val);
    push(sym->defn->val--);
    UPDATE_PC();
}

void evaldec_prt(const instr *i, const Cell *pc)
{
    PR("'%s'\n", pc[1].sym->name);
}

void decarg(const instr *i) /* assign top value to next value */
{
    int arg = pc[0].args;

    Datum *ref = getarg(arg);

    push(--ref[0]);
    P_TAIL(": %.8g -> $%d -> %.8g", ref[0], arg, ref[0]);

    UPDATE_PC();
}

void decarg_prt(const instr *i, const Cell *pc)
{
    PR("$%d\n", pc[0].args);
}

void argdec(const instr *i) /* assign top value to next value */
{
    int arg = pc[0].args;

    Datum *ref = getarg(arg);

    Datum d = *ref; /* dato antes de incrementar */

    push(ref[0]--);
    P_TAIL(": %.8g -> $%d -> %lg", ref[0], arg, d);

    UPDATE_PC();
}

void argdec_prt(const instr *i, const Cell *pc)
{
    PR("$%d\n", pc[0].args);
}

void addvar(const instr *i) /* assign top value to next value */
{
    Symbol *sym = pc[1].sym;
    if (sym->type == UNDEF) {
        execerror("undefined variable '%s' cannot be decremented",
                sym->name);
    }

    sym->defn->val += pop();
    push(sym->defn->val);

    P_TAIL(": %.8lg -> %s", sym->defn->val, sym->name);
    UPDATE_PC();
}

void addvar_prt(const instr *i, const Cell *pc)
{
    PR("'%s'\n", pc[1].sym->name);
}

void addarg(const instr *i) /* assign top value to next value */
{
    int arg = pc[0].args;

    Datum *ref = getarg(arg);

    ref[0]    += pop();
    push(ref[0]);

    P_TAIL(": %.8g -> $%d -> %lg", ref[0], arg, top());

    UPDATE_PC();
}

void addarg_prt(const instr *i, const Cell *pc)
{
    PR("$%d\n", pc[0].args);
}

void subvar(const instr *i) /* assign top value to next value */
{
    Symbol *sym = pc[1].sym;
    if (sym->type == UNDEF) {
        execerror("undefined variable '%s' cannot be decremented",
                sym->name);
    }

    sym->defn->val -= pop();
    push(sym->defn->val);

    P_TAIL(": %.8lg -> %s", sym->defn->val, sym->name);
    UPDATE_PC();
}

void subvar_prt(const instr *i, const Cell *pc)
{
    PR("'%s'\n", pc[1].sym->name);
}

void subarg(const instr *i) /* assign top value to next value */
{
    int arg = pc[0].args;

    Datum *ref = getarg(arg);

    ref[0]    -= pop();
    push(ref[0]);

    P_TAIL(": %.8g -> $%d -> %lg", ref[0], arg, top());

    UPDATE_PC();
}

void subarg_prt(const instr *i, const Cell *pc)
{
    PR("$%d\n", pc[0].args);
}

void mulvar(const instr *i) /* assign top value to next value */
{
    Symbol *sym = pc[1].sym;
    if (sym->type == UNDEF) {
        execerror("undefined variable '%s' cannot be decremented",
                sym->name);
    }

    sym->defn->val *= pop();
    push(sym->defn->val);

    P_TAIL(": %.8lg -> %s", sym->defn->val, sym->name);
    UPDATE_PC();
}

void mulvar_prt(const instr *i, const Cell *pc)
{
    PR("'%s'\n", pc[1].sym->name);
}

void mularg(const instr *i) /* assign top value to next value */
{
    int arg    = pc[0].args;

    Datum *ref = getarg(arg);

    ref[0]    *= pop();
    push(ref[0]);

    P_TAIL(": %.8g -> $%d -> %lg", ref[0], arg, top());

    UPDATE_PC();
}

void mularg_prt(const instr *i, const Cell *pc)
{
    PR("$%d\n", pc[0].args);
}

void divvar(const instr *i) /* assign top value to next value */
{
    Symbol *sym = pc[1].sym;
    if (sym->type == UNDEF) {
        execerror("undefined variable '%s' cannot be decremented",
                sym->name);
    }

    sym->defn->val /= pop();
    push(sym->defn->val);

    P_TAIL(": %.8lg -> %s", sym->defn->val, sym->name);
    UPDATE_PC();
}

void divvar_prt(const instr *i, const Cell *pc)
{
    PR("'%s'\n", pc[1].sym->name);
}

void divarg(const instr *i) /* assign top value to next value */
{
    int arg = pc[0].args;

    Datum *ref = getarg(arg);

    ref[0]    /= pop();
    push(ref[0]);

    P_TAIL(": %.8g -> $%d -> %lg", ref[0], arg, top());

    UPDATE_PC();
}

void divarg_prt(const instr *i, const Cell *pc)
{
    PR("$%d\n", pc[0].args);
}

void pwrvar(const instr *i) /* assign top value to next value */
{
    Symbol *sym = pc[1].sym;
    if (sym->type == UNDEF) {
        execerror("undefined variable '%s' cannot be decremented",
                sym->name);
    }

    sym->defn->val  = pow(sym->defn->val, pop());
    push(sym->defn->val);

    P_TAIL(": %.8lg -> %s", sym->defn->val, sym->name);
    UPDATE_PC();
}

void pwrvar_prt(const instr *i, const Cell *pc)
{
    PR("'%s'\n", pc[1].sym->name);
}

void pwrarg(const instr *i) /* assign top value to next value */
{
    int arg    = pc[0].args;

    Datum *ref = getarg(arg);

    ref[0] = pow(ref[0], pop());
    push(ref[0]);

    P_TAIL(": %.8g -> $%d -> %lg", ref[0], arg, top());

    UPDATE_PC();
}

void pwrarg_prt(const instr *i, const Cell *pc)
{
    PR("$%d\n", pc[0].args);
}

void modvar(const instr *i) /* assign top value to next value */
{
    Symbol *sym = pc[1].sym;
    if (sym->type == UNDEF) {
        execerror("undefined variable '%s' cannot be decremented",
                sym->name);
    }

    sym->defn->val = fmod(sym->defn->val, pop());
    push(sym->defn->val);

    P_TAIL(": %.8lg -> %s", sym->defn->val, sym->name);
    UPDATE_PC();
}

void modvar_prt(const instr *i, const Cell *pc)
{
    PR("'%s'\n", pc[1].sym->name);
}

void modarg(const instr *i) /* assign top value to next value */
{
    int arg    = pc[0].args;

    Datum *ref = getarg(arg);

    ref[0] = fmod(ref[0], pop());
    push(ref[0]);

    P_TAIL(": %.8g -> $%d -> %lg", ref[0], arg, top());

    UPDATE_PC();
}

void modarg_prt(const instr *i, const Cell *pc)
{
    PR("$%d\n", pc[0].args);
}

void spadd(const instr *i) /* pop n elementos de la pila */
{
    int n = pc[0].args;
    P_TAIL(": %d", n);
    stackp += n;
    UPDATE_PC();
}

void spadd_prt(const instr *i, const Cell *pc)
{
    PR("%d\n", pc[0].args);
}
