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

#ifndef   UQ_DEBUG_STACK /* { */
#warning  UQ_DEBUG_STACK deberia ser incluido en config.mk
#define   UQ_DEBUG_STACK    0
#endif /* UQ_DEBUG_STACK    } */

#if      UQ_CODE_DEBUG_EXEC /* {{ */
#define  EXEC(_fmt, ...)            \
    printf(F(_fmt), ##__VA_ARGS__)
#define  P_TAIL(_fmt, ...)          \
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
#define PRG(_fmt, ...) printf(F(_fmt), ##__VA_ARGS__)
#else
#define PRG(_fmt, ...)
#endif

#ifndef  UQ_NPROG
#warning UQ_NPROG debe definirse en config.mk
#define  UQ_NPROG 10000 /* 65536 celdas para instrucciones/datos/pila */
#endif

Cell  prog[UQ_NPROG];  /* the machine */
Cell *progp    = prog; /* next free cell for code generation */
Cell *pc       = prog; /* program counter during execution */
Cell *fp       = NULL;
Cell *sp       = NULL;
Cell *progbase = prog; /* start of current subprogram */
Cell *varbase  = prog + UQ_NPROG; /* pointer to last global allocated */

void initcode(void)  /* initalize for code generation */
{
    progp = progbase;
    fp    =
    sp    = varbase;
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
        execerror("Variable " GREEN "%s" ANSI_END
            " already defined in current scope\n", name);
    }
    Symbol *sym = install(name, LVAR, typref);
    scop->size += typref->size;
    sym->offset = -(scop->base_offset + scop->size);

    PRG("Symbol '%s', type=%s, typref=%s, offset=%+d\n",
        sym->name,
        lookup_type(sym->type),
        typref->name,
        sym->offset);
    return sym;
} /* register_local_var */


int stacksize(void) /* return the stack size */
{
    return varbase - sp;
} /* stacksize */

void push(Cell d)  /* push d onto stack */
{
    /*  Verificamos si el puntero apunta a una direccion mas alla
        del final de la pila  */
    if (sp <= progp)
        execerror("stack overflow: "GREEN"progp=[%04lx], sp=[%04lx]",
                progp - prog, sp - prog);
    *--sp = d;
}

Cell pop(void)    /* pops Datum and return top element from stack */
{
    if (sp >= varbase)
        execerror("stack empty: sp=[%04lx], varbase[%04lx]",
                sp, varbase);
    return *sp++;
}

Cell top(void)   /* returns the top of the stack */
{
    if (sp == varbase)
        execerror("stack empty: sp=[%04lx], varbase[%04lx]",
                sp, varbase);
    return *sp;
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
    EXEC(BRIGHT YELLOW "BEGIN [%04lx], fp=[%04lx], "
            "sp=[%04lx], varbase=[%04lx], stacksize=%d" ANSI_END "\n",
            (p - prog), fp - prog, sp - prog, varbase - prog, stacksize());
    for (   pc = p;
            pc->inst != INST_STOP;
        )
    {
        const instr *ins = instruction_set + pc->inst;
        EXEC("[%04lx]: %s", pc - prog, ins->name);
        ins->exec(ins);
#if       UQ_DEBUG_STACK /* { */
        P_TAIL(", fp=[%04lx], sp=[%04lx], ss=%d",
                fp - prog, sp - prog, stacksize());
#endif /* UQ_DEBUG_STACK    } */
        P_TAIL("\n");
    }
    EXEC(BRIGHT YELLOW "END [%04lx], fp=[%04lx], "
            "sp=[%04lx], stacksize=%d" ANSI_END "\n",
            (pc - prog), fp - prog, sp - prog, stacksize());
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
    Cell d = pc[1];

    push(d);
    P_TAIL(": -> %.8lg", d.val);

    UPDATE_PC();
}

/* push constant onto stack */
void constpush_prt(const instr *i, const Cell *pc)
{
    PR("%2.8g\n", pc[1].val);
}

void datum_prog(const instr *i, Cell *pc, va_list args)
{
    progp[1].val = va_arg(args, double);

    PRG(" %.8lg", progp[1].val);
}

void add(const instr *i) /* add top two elements on stack */
{
    Cell p2  = pop(),
         p1  = pop(),
         res = { .val = p1.val + p2.val };

    P_TAIL(": %lg + %lg -> %lg",
            p1.val, p2.val, res.val);
    push(res);

    UPDATE_PC();
}

void add_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void sub(const instr *i) /* subtract top two elements on stack */
{
    Cell p2  = pop(),
         p1  = pop(),
         res = { .val = p1.val - p2.val };

    P_TAIL(": %lg - %lg -> %lg",
            p1.val, p2.val, res.val);
    push(res);

    UPDATE_PC();
}

void sub_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void mul(const instr *i) /* multiply top two elements on stack */
{
    Cell p2  = pop(),
         p1  = pop(),
         res = { .val = p1.val * p2.val };

    P_TAIL(": %lg * %lg -> %lg",
            p1.val, p2.val, res.val);
    push(res);

    UPDATE_PC();
}

void mul_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void divi(const instr *i) /* divide top two elements on stack */
{
    Cell p2  = pop(),
         p1  = pop(),
         res = { .val = p1.val / p2.val };
    P_TAIL(": %lg / %lg -> %lg",
            p1.val, p2.val, res.val);
    push(res);

    UPDATE_PC();
}

void divi_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void mod(const instr *i) /* mod top two elements on stack */
{
    Cell p2  = pop(),
         p1  = pop(),
         res = { .val = fmod(p1.val, p2.val) };

    P_TAIL(": %lg %% %lg -> %lg",
            p1.val, p2.val, res.val);
    push(res);

    UPDATE_PC();
}

void mod_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void neg(const instr *i) /* change sign top element on stack */
{
    Cell d   = pop(),
         res = { .val = -d.val };

    P_TAIL(": d=%lg -> %lg",
            d.val, res.val);
    push(res);

    UPDATE_PC();
}

void neg_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void pwr(const instr *i) /* pow top two elements on stack */
{
    Cell e  = pop(),
         b  = pop(),
         res = { .val = pow(b.val, e.val) };

    P_TAIL(": b=%lg, e=%lg -> %lg",
            b.val, e.val, res.val);
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

    P_TAIL(": "GREEN"%s"ANSI_END"[%04x] -> %lg",
        sym->name, pc[0].desp, sym->defn->val);
    push(*sym->defn);

    UPDATE_PC();
}

void eval_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].desp);
}

void symb_prog(const instr *i, Cell *pc, va_list args)
{
    Symbol *sym = va_arg(args, Symbol *);

    pc[0].desp  = sym->defn - prog;
    pc[1].sym   = sym;

    PRG(" "GREEN"%s"ANSI_END"[%04x]", sym->name, pc[0].desp);
}

void assign(const instr *i) /* assign top value to next value */
{
    Symbol *sym = pc[1].sym;
    Cell    d   = top();

    P_TAIL(": %.8g -> "GREEN"%s"ANSI_END"[%04x]", d.val, sym->name, pc[0].desp);
    *sym->defn = d;

    UPDATE_PC();
}

void assign_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].desp);
}

void print(const instr *i) /* pop top value from stack, print it */
{
    Cell d = pop();

    printf("\t\t%32.8g\n", d.val);

    UPDATE_PC();
}

void print_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void bltin0(const instr *i) /* evaluate built-in on top of stack */
{
    Symbol *sym = pc[1].sym;
    Cell    res = { .val = sym->ptr0() };

    P_TAIL(": %s() -> %.8lg", sym->name, res.val);
    push(res);

    UPDATE_PC();
}

void bltin0_prt(const instr *i, const Cell *pc)
{
    PR("%s\n", pc[1].sym->help);
}

void bltin1(const instr *i) /* evaluate built-in with one argument */
{
    Symbol *sym = pc[1].sym;
    Cell  p   = pop(),
          res = { .val = sym->ptr1( p.val ) };

    P_TAIL(": %s(%.8lg) -> %.8lg",
        sym->name, p.val, res.val);
    push(res);

    UPDATE_PC();
}

void bltin1_prt(const instr *i, const Cell *pc)
{
    PR("%s\n", pc[1].sym->help);
}

void bltin2(const instr *i) /* evaluate built-in with two arguments */
{
    Symbol *sym = pc[1].sym;
    Cell    p2  = pop(),
            p1  = pop(),
            res = { .val = sym->ptr2( p1.val, p2.val ) };

    P_TAIL(": %s(%.8lg, %.8lg) -> %.8lg",
        sym->name, p1.val, p2.val, res.val);
    push(res);

    UPDATE_PC();
}

void bltin2_prt(const instr *i, const Cell *pc)
{
    PR("%s\n", pc[1].sym->help);
}

void ge(const instr *i) /* greater or equal */
{
    Cell p2  = pop(),
         p1  = pop(),
         res = { .val = p1.val >= p2.val };

    P_TAIL(": %lg >= %lg -> %lg", p1.val, p2.val, res.val);
    push(res);

    UPDATE_PC();
}

void ge_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void le(const instr *i) /* less or equal */
{
    Cell  p2  = pop(),
          p1  = pop(),
          res = { .val = p1.val <= p2.val };

    P_TAIL(": %lg <= %lg -> %lg", p1.val, p2.val, res.val);
    push(res);

    UPDATE_PC();
}

void le_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void gt(const instr *i) /* greater than */
{
    Cell p2  = pop(),
         p1  = pop(),
         res = { .val = p1.val > p2.val };

    P_TAIL(": %lg > %lg -> %lg", p1.val, p2.val, res.val);
    push(res);

    UPDATE_PC();
}

void gt_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void lt(const instr *i) /* less than */
{
    Cell  p2  = pop(),
          p1  = pop(),
          res = { .val = p1.val < p2.val };

    P_TAIL(": %lg < %lg -> %lg", p1.val, p2.val, res.val);
    push(res);

    UPDATE_PC();
}

void lt_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void eq(const instr *i) /* equal */
{
    Cell p2  = pop(),
         p1  = pop(),
         res = { .val = p1.val == p2.val };

    P_TAIL(": %lg == %lg -> %lg", p1.val, p2.val, res.val);
    push(res);

    UPDATE_PC();
}

void eq_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void ne(const instr *i) /* not equal */
{
    Cell p2  = pop(),
         p1  = pop(),
         res = { .val = p1.val != p2.val };

    P_TAIL(": %lg != %lg -> %lg", p1.val, p2.val, res.val);
    push(res);

    UPDATE_PC();
}

void ne_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void not(const instr *i) /* not */
{
    Cell p  = pop(),
         res = { .val = ! p.val };

    P_TAIL(": ! %lg -> %lg", p.val, res.val);
    push(res);

    UPDATE_PC();
}

void not_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void and(const instr *i)  /* and */
{
    Cell p2  = pop(),
         p1  = pop(),
         res = { .val = p1.val && p2.val };

    P_TAIL(": %lg && %lg -> %lg", p1.val, p2.val, res.val);
    push(res);

    UPDATE_PC();
}

void and_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void and_then(const instr *i)  /* and_then */
{
    Cell        d  = top();
    const char *op = d.val
                   ? " drop;"
                   : "";

    if (d.val) {
        pop();
        UPDATE_PC();
    } else {
        pc = prog + pc[0].desp;
    }

    P_TAIL(": %lg &&%s -> [%04lx]", d.val, op, pc - prog);
}

void and_then_prt(const instr *i, const Cell *pc)
{
    PR("[%04x]\n", pc[0].desp);
}

void or(const instr *i)               /* or */
{
    Cell p2  = pop(),
         p1  = pop(),
         res = { .val = p1.val || p2.val };

    P_TAIL(": %lg || %lg -> %lg", p1.val, p2.val, res.val);
    push(res);

    UPDATE_PC();
}

void or_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void or_else(const instr *i)  /* or_else */
{
    Cell d = top();
    const char *op = d.val ? " drop;" : "";

    if (!d.val) {
        pop();
        UPDATE_PC();
    } else {
        pc = prog + pc[0].desp;
    }
    P_TAIL(": %lg ||%s -> [%04lx]",
        d.val, op, pc - prog);
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
    Symbol *symb = install(name, type, typref);
    symb->defn   = entry;

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

void call(const instr *i)   /* call a function */
{
    Symbol *sym = pc[1].sym;

    P_TAIL(": "GREEN"%s"ANSI_END"[%04x] -> ret_addr=[%04lx]",
        sym->name, pc[0].desp, pc + i->n_cells - prog);

    Cell ret_addr = { .cel = pc + i->n_cells };

    push(ret_addr);

    pc = prog + pc[0].desp;
} /* call */

void call_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"[%04x], args=%ld -> \n",
        pc[1].sym->name,
        pc[0].desp,
        pc[1].sym->argums_len);
}

void ret(const instr *i) /* return from proc */
{
    Cell dest = pop();

    P_TAIL(": -> [%04lx]", dest.cel - prog);

    pc = dest.cel;
}

void ret_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

Cell *getarg(int offset)    /* return a pointer to argument */
{
    return fp + offset;
}

void arg_prog(const instr *i, Cell *pc, va_list args)
{
    pc[0].args = va_arg(args, int);
    PRG(" %+d", pc[0].args);
}

void arg_str_prog(const instr *i, Cell *pc, va_list args)
{
    pc[0].args = va_arg(args, int);
    pc[1].str  = va_arg(args, char *);
    PRG(" "GREEN"%s"ANSI_END"<%+d>", pc[1].str, pc[0].args);
}

void argeval(const instr *i) /* push argument onto stack */
{
    int arg = pc[0].args;
    Cell  d = *getarg(arg);

    P_TAIL(": "GREEN"%s"ANSI_END"<%+d> -> %.8g", pc[1].str, arg, d.val);
    push(d);

    UPDATE_PC();
}

void argeval_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].args);
}

void argassign(const instr *i) /* store top of stack in argument */
{
    int   arg = pc[0].args;
    Cell  d   = top();
    Cell *ref = getarg(arg);

    P_TAIL(": %.8g -> "GREEN"%s"ANSI_END"<%+d>", d.val, pc[1].str, arg);
    ref->val = d.val;

    UPDATE_PC();
}

void argassign_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].args);
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
    progp[1].str = va_arg(args, const char *);

    PRG(" \"%s\"", progp->str);
}

void prexpr(const instr *i)  /* print numeric value */
{
    P_TAIL("\n");
    printf("%.8g", pop().val);
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
    P_TAIL("\n");
    list_all_symbols(pc[1].sym);
    UPDATE_PC();
}

void symbs_all_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void brkpt(const instr *i)
{
    P_TAIL("\n");
    list_variables(pc[1].sym);
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
    Cell d = pop();
    pc = d.val ? pc + i->n_cells : prog + pc[0].desp;
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

void inceval(const instr *i) /* 1. incremento, 2. evaluo la variable */
{
    Symbol *sym = pc[1].sym;

    ++sym->defn->val;
    push(*sym->defn);
    P_TAIL(": ++"GREEN"%s"ANSI_END"[%04x] -> %.8lg",
            sym->name, pc[0].desp, sym->defn->val);

    UPDATE_PC();
}

void inceval_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].desp);
}

void evalinc(const instr *i) /* 1. copio valor,
                              * 2. evaluo variable,
                              * 3. devuelvo valor copiado. */
{
    Symbol *sym = pc[1].sym;

    push(*sym->defn);
    ++sym->defn->val;
    P_TAIL(": "GREEN"%s"ANSI_END"++[%04x] -> %.8lg",
            sym->name, pc[0].desp, sym->defn->val);

    UPDATE_PC();
}

void evalinc_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].desp);
}

void incarg(const instr *i) /* assign top value to next value */
{
    int arg = pc[0].args;

    Cell *ref = getarg(arg);
    ++ref->val;
    push(*ref);
    P_TAIL(": %.8g -> "GREEN"%s"ANSI_END"<%+d> -> %.8g",
           ref->val, pc[1].str, arg, ref->val);

    UPDATE_PC();
}

void incarg_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].args);
}

void arginc(const instr *i) /* evaluo e incremento */
{
    int arg = pc[0].args;

    Cell *ref = getarg(arg);

    push(*ref);
    ++ref->val;

    P_TAIL(": %.8g -> "GREEN"%s"ANSI_END"<%+d> -> %lg", ref->val, pc[1].str, arg, top().val);

    UPDATE_PC();
}

void arginc_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].args);
}

void deceval(const instr *i) /* assign top value to next value */
{
    Symbol *sym = pc[1].sym;

    --sym->defn->val;
    push(*sym->defn);
    P_TAIL(": "GREEN"%s"ANSI_END"[%04x] -> %.8lg", sym->name, pc[0].desp, top().val);

    UPDATE_PC();
}

void deceval_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].desp);
}

void evaldec(const instr *i) /* assign top value to next value */
{
    Symbol *sym = pc[1].sym;

    push(*sym->defn);
    P_TAIL(": "GREEN"%s"ANSI_END"[%04x] -> %.8lg", sym->name, pc[0].desp, top().val);
    --sym->defn->val;

    UPDATE_PC();
}

void evaldec_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].desp);
}

void decarg(const instr *i) /* assign top value to next value */
{
    int arg = pc[0].args;

    Cell *ref = getarg(arg);

    --ref->val;
    push(*ref);

    P_TAIL(": %.8g -> "GREEN"%s"ANSI_END"<%+d> -> %.8g", ref->val, pc[1].str, arg, top().val);

    UPDATE_PC();
}

void decarg_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].args);
}

void argdec(const instr *i) /* assign top value to next value */
{
    int arg = pc[0].args;
    Cell *ref = getarg(arg);

    push(*ref);
    --ref->val;
    P_TAIL(": %.8g -> "GREEN"%s"ANSI_END"<%+d> -> %lg", ref->val, pc[1].str, arg, top().val);

    UPDATE_PC();
}

void argdec_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].args);
}

void addvar(const instr *i) /* assign top value to next value */
{
    Symbol *sym = pc[1].sym;

    sym->defn->val += pop().val;
    push(*sym->defn);

    P_TAIL(": %.8lg -> "GREEN"%s"ANSI_END"[%04x]", sym->defn->val, sym->name, pc[0].desp);
    UPDATE_PC();
}

void addvar_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].desp);
}

void addarg(const instr *i) /* assign top value to next value */
{
    int arg = pc[0].args;

    Cell *ref = getarg(arg);

    ref->val += pop().val;
    push(*ref);

    P_TAIL(": %.8g -> "GREEN"%s"ANSI_END"<%+d> -> %lg", ref->val, pc[1].str, arg, top().val);

    UPDATE_PC();
}

void addarg_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].args);
}

void subvar(const instr *i) /* assign top value to next value */
{
    Symbol *sym = pc[1].sym;

    sym->defn->val -= pop().val;
    push(*sym->defn);
    P_TAIL(": %.8lg -> "GREEN"%s"ANSI_END"[%04x]", sym->defn->val, sym->name, pc[0].desp);

    UPDATE_PC();
}

void subvar_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].desp);
}

void subarg(const instr *i) /* assign top value to next value */
{
    int   arg = pc[0].args;
    Cell *ref = getarg(arg);

    ref->val -= pop().val;
    push(*ref);

    P_TAIL(": %.8g -> "GREEN"%s"ANSI_END"<%+d> -> %lg", ref->val, pc[1].str, arg, top().val);

    UPDATE_PC();
}

void subarg_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].args);
}

void mulvar(const instr *i) /* assign top value to next value */
{
    Symbol *sym = pc[1].sym;

    sym->defn->val *= pop().val;
    push(*sym->defn);

    P_TAIL(": %.8lg -> "GREEN"%s"ANSI_END"[%04x]", sym->defn->val, sym->name, pc[0].desp);
    UPDATE_PC();
}

void mulvar_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].desp);
}

void mularg(const instr *i) /* assign top value to next value */
{
    int   arg = pc[0].args;
    Cell *ref = getarg(arg);

    ref->val *= pop().val;
    push(*ref);

    P_TAIL(": %.8g -> "GREEN"%s"ANSI_END"<%+d> -> %lg", ref->val, pc[1].str, arg, top().val);

    UPDATE_PC();
}

void mularg_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].args);
}

void divvar(const instr *i) /* assign top value to next value */
{
    Symbol *sym = pc[1].sym;

    sym->defn->val /= pop().val;
    push(*sym->defn);
    P_TAIL(": %.8lg -> "GREEN"%s"ANSI_END"[%04x]", sym->defn->val, sym->name, pc[0].desp);

    UPDATE_PC();
}

void divvar_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].desp);
}

void divarg(const instr *i) /* assign top value to next value */
{
    int arg = pc[0].args;

    Cell *ref = getarg(arg);

    ref->val /= pop().val;
    push(*ref);
    P_TAIL(": %.8g -> "GREEN"%s"ANSI_END"<%+d> -> %lg", ref->val, pc[1].str, arg, top().val);

    UPDATE_PC();
}

void divarg_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].args);
}

void pwrvar(const instr *i) /* assign top value to next value */
{
    Symbol *sym = pc[1].sym;

    sym->defn->val  = pow(sym->defn->val, pop().val);
    push(*sym->defn);
    P_TAIL(": %.8lg -> "GREEN"%s"ANSI_END"[%04x]", sym->defn->val, sym->name, pc[0].desp);

    UPDATE_PC();
}

void pwrvar_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].desp);
}

void pwrarg(const instr *i) /* assign top value to next value */
{
    int arg    = pc[0].args;

    Cell *ref = getarg(arg);

    ref->val = pow(ref->val, pop().val);
    push(*ref);
    P_TAIL(": %.8g -> "GREEN"%s"ANSI_END"<%+d> -> %lg", ref->val, pc[1].str, arg, top().val);

    UPDATE_PC();
}

void pwrarg_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].args);
}

void modvar(const instr *i) /* assign top value to next value */
{
    Symbol *sym = pc[1].sym;

    sym->defn->val = fmod(sym->defn->val, pop().val);
    push(*sym->defn);
    P_TAIL(": %.8lg -> "GREEN"%s"ANSI_END"[%04x]", sym->defn->val, sym->name, pc[0].desp);

    UPDATE_PC();
}

void modvar_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].desp);
}

void modarg(const instr *i) /* assign top value to next value */
{
    int arg    = pc[0].args;

    Cell *ref = getarg(arg);

    ref->val = fmod(ref->val, pop().val);
    push(*ref);
    P_TAIL(": %.8g -> "GREEN"%s"ANSI_END"<%+d> -> %lg", ref->val, pc[1].str, arg, top().val);

    UPDATE_PC();
}

void modarg_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].args);
}

void spadd(const instr *i) /* pop n elementos de la pila */
{
    int n = pc[0].args;
    P_TAIL(": %+d", n);
    sp += n;
    UPDATE_PC();
}

void spadd_prt(const instr *i, const Cell *pc)
{
    PR("%+d\n", pc[0].args);
}

void push_fp(const instr *i)
{
    Cell dato = { .cel = fp };

    P_TAIL(": fp=[%04lx] -> sp = %04lx", fp - prog, sp - prog);
    push(dato);

    UPDATE_PC();
}

void push_fp_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void pop_fp(const instr *i)
{
    Cell dato = pop();

    fp = dato.cel;
    P_TAIL(": sp=%04lx -> FP=[%04lx]", sp - prog, fp - prog);

    UPDATE_PC();
}

void pop_fp_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void move_sp_to_fp(const instr *i)
{
    fp = sp;

    P_TAIL(": %04lx -> FP", fp - prog);

    UPDATE_PC();
}

void move_sp_to_fp_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}
