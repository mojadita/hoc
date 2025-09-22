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
#include "math.h"

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
} /* initcode */

void initexec(void) /* initialize for execution */
{
    fp    =
    sp    = varbase;
} /* initexec */


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

void constpush_prt(const instr *i, const Cell *pc)
{
    PR("%2.8g\n", pc[1].val);
}

void constpush_d(const instr *i) /* push constant onto stack */
{
    UPDATE_PC();
}

void constpush_d_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void constpush_l(const instr *i)
{
    UPDATE_PC();
}

void constpush_l_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void datum_prog(const instr *i, Cell *pc, va_list args)
{
    progp[1].val = va_arg(args, double);

    PRG(" %.8lg", progp[1].val);
}

void datum_d_prog(const instr *i, Cell *pc, va_list args)
{
    progp[1].val = va_arg(args, double);

    PRG(" %.15lg", progp[1].val);
}

void datum_i_prog(const instr *i, Cell *pc, va_list args)
{
    progp[1].num = va_arg(args, int);

    PRG(" %li", progp[1].num);
}

void datum_l_prog(const instr *i, Cell *pc, va_list args)
{
    progp[1].num = va_arg(args, long);

    PRG(" %li", progp[1].num);
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

void add_d(const instr *i) /* add top two elements on stack */
{
    Cell p2  = pop(),
         p1  = pop(),
         res = { .val = p1.val + p2.val };

    P_TAIL(": %lg + %lg -> %lg",
            p1.val, p2.val, res.val);
    push(res);

    UPDATE_PC();
}

void add_d_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void add_i(const instr *i)
{
    Cell p2  = pop(),
         p1  = pop(),
         res = { .num = p1.num + p2.num };

    P_TAIL(": %li + %li -> %li",
            p1.num, p2.num, res.num);
    push(res);

    UPDATE_PC();
}

void add_i_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void add_l(const instr *i)
{
    Cell p2  = pop(),
         p1  = pop(),
         res = { .num = p1.num + p2.num };

    P_TAIL(": %li + %li -> %li",
            p1.num, p2.num, res.num);
    push(res);

    UPDATE_PC();
}

void add_l_prt(const instr *i, const Cell *pc)
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

void sub_d(const instr *i) /* subtract top two elements on stack */
{
    Cell p2  = pop(),
         p1  = pop(),
         res = { .val = p1.val - p2.val };

    P_TAIL(": %lg - %lg -> %lg",
            p1.val, p2.val, res.val);
    push(res);

    UPDATE_PC();
}

void sub_d_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void sub_i(const instr *i)
{
    Cell p2  = pop(),
         p1  = pop(),
         res = { .num = p1.num - p2.num };

    P_TAIL(": %li - %li -> %li",
            p1.num, p2.num, res.num);
    push(res);

    UPDATE_PC();
}

void sub_i_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void sub_l(const instr *i)
{
    Cell p2  = pop(),
         p1  = pop(),
         res = { .num = p1.num - p2.num };

    P_TAIL(": %li - %li -> %li",
            p1.num, p2.num, res.num);
    push(res);

    UPDATE_PC();
}

void sub_l_prt(const instr *i, const Cell *pc)
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

void mul_d(const instr *i) /* multiply top two elements on stack */
{
    Cell p2  = pop(),
         p1  = pop(),
         res = { .val = p1.val * p2.val };

    P_TAIL(": %lg * %lg -> %lg",
            p1.val, p2.val, res.val);
    push(res);

    UPDATE_PC();
}

void mul_d_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void mul_i(const instr *i)
{
    Cell p2  = pop(),
         p1  = pop(),
         res = { .num = p1.num * p2.num };

    P_TAIL(": %li * %li -> %li",
            p1.num, p2.num, res.num);
    push(res);

    UPDATE_PC();
}

void mul_i_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void mul_l(const instr *i)
{
    Cell p2  = pop(),
         p1  = pop(),
         res = { .num = p1.num * p2.num };

    P_TAIL(": %li * %li -> %li",
            p1.num, p2.num, res.num);
    push(res);

    UPDATE_PC();
}

void mul_l_prt(const instr *i, const Cell *pc)
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

void divi_d(const instr *i) /* divide top two elements on stack */
{
    Cell p2  = pop(),
         p1  = pop(),
         res = { .val = p1.val / p2.val };
    P_TAIL(": %lg / %lg -> %lg",
            p1.val, p2.val, res.val);
    push(res);

    UPDATE_PC();
}

void divi_d_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void divi_i(const instr *i)
{
    Cell p2  = pop(),
         p1  = pop(),
         res = { .num = p1.num / p2.num };
    P_TAIL(": %li / %li -> %li",
            p1.num, p2.num, res.num);
    push(res);

    UPDATE_PC();
}

void divi_i_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void divi_l(const instr *i)
{
    Cell p2  = pop(),
         p1  = pop(),
         res = { .num = p1.num / p2.num };
    P_TAIL(": %li / %li -> %li",
            p1.num, p2.num, res.num);
    push(res);

    UPDATE_PC();
}

void divi_l_prt(const instr *i, const Cell *pc)
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

void mod_d(const instr *i) /* mod top two elements on stack */
{
    Cell p2  = pop(),
         p1  = pop(),
         res = { .val = fmod(p1.val, p2.val) };

    P_TAIL(": %lg %% %lg -> %lg",
            p1.val, p2.val, res.val);
    push(res);

    UPDATE_PC();
}

void mod_d_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void mod_i(const instr *i)
{
    Cell p2  = pop(),
         p1  = pop(),
         res = { .num = p1.num % p2.num };

    P_TAIL(": %li %% %li -> %li",
            p1.num, p2.num, res.num);
    push(res);

    UPDATE_PC();
}

void mod_i_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void mod_l(const instr *i)
{
    Cell p2  = pop(),
         p1  = pop(),
         res = { .num = p1.num % p2.num };

    P_TAIL(": %li %% %li -> %li",
            p1.num, p2.num, res.num);
    push(res);

    UPDATE_PC();
}

void mod_l_prt(const instr *i, const Cell *pc)
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

void neg_d(const instr *i) /* change sign top element on stack */
{
    Cell d   = pop(),
         res = { .val = -d.val };

    P_TAIL(": d=%lg -> %lg",
            d.val, res.val);
    push(res);

    UPDATE_PC();
}

void neg_d_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void neg_i(const instr *i)
{
    Cell d   = pop(),
         res = { .num = -d.num };

    P_TAIL(": d=%li -> %li",
            d.num, res.num);
    push(res);

    UPDATE_PC();
}

void neg_i_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void neg_l(const instr *i)
{
    Cell d   = pop(),
         res = { .num = -d.num };

    P_TAIL(": d=%li -> %li",
            d.num, res.num);
    push(res);

    UPDATE_PC();
}

void neg_l_prt(const instr *i, const Cell *pc)
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

void pwr_d(const instr *i) /* pow top two elements on stack */
{
    Cell exp  = pop(),
         base = pop();

    if (base.val == 0 && exp.val == 0 || exp.val < 0)
        execerror("0.0 raised to 0.0 (or negative) not allowed");

    Cell tgt  = { .val = pow(base.val, exp.val) };

    P_TAIL(": base=%lg, exp=%lg -> %lg",
            base.val, exp.val, tgt.val);

    push(tgt);

    UPDATE_PC();
}

void pwr_d_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void pwr_l(const instr *i) /* pow top two elements on stack */
{
    int base     = pop().num;
    unsigned exp = pop().num;
    unsigned mask;

    if (base == 0 && exp == 0 || exp < 0)
        execerror("0 raised to 0 not allowed");

    for (mask = 1; mask <= exp; mask <<= 1)
        continue;
    mask >>= 1;

    int ret_val = 1;
    while (mask) {
        ret_val *= ret_val;
        if (mask & exp) {
            ret_val *= base;
        }
        mask >>= 1;
    }

    Cell result = { .num = ret_val };
    push(result);

    UPDATE_PC();
}

void pwr_l_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void eval(const instr *i)
{
    int     var_addr = pc[0].desp;
    Symbol *sym      = pc[1].sym;
    Cell   *var      = prog + var_addr;
    Cell    tgt      = { .val = var->val };

    push(tgt);

    P_TAIL(": "GREEN"%s"ANSI_END"[%04x](%.15lg) -> %.15lg",
        sym->name, var_addr, var->val, tgt.val);

    UPDATE_PC();
}

void eval_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"[%04x]\n",
        pc[1].sym->name, pc[0].desp);
}

void eval_c(const instr *i) /* evaluate global variable on stack */
{
    int     var_addr = pc[0].desp;
    Symbol *sym      = pc[1].sym;
    Cell   *var      = prog + var_addr;
    Cell    tgt      = { .num = var->chr };

    push(tgt);

    P_TAIL(": "GREEN"%s"ANSI_END"[%04x](0x%02hhx) -> %li",
        sym->name, var_addr, var->chr, tgt.num);

    UPDATE_PC();
}

void eval_c_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"[%04x]\n",
        pc[1].sym->name, pc[0].desp);
}

void eval_d(const instr *i)
{
    int     var_addr = pc[0].desp;
    Symbol *sym      = pc[1].sym;
    Cell   *var      = prog + var_addr;
    Cell    tgt      = { .val = var->val };

    push(tgt);

    P_TAIL(": "GREEN"%s"ANSI_END"[%04x](%.15lg) -> %.15lg",
        sym->name, var_addr, var->val, tgt.val);

    UPDATE_PC();
}

void eval_d_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"[%04x]\n",
        pc[1].sym->name, pc[0].desp);
}

void eval_f(const instr *i)
{
    int     var_addr = pc[0].desp;
    Symbol *sym      = pc[1].sym;
    Cell   *var      = prog + var_addr;
    Cell    tgt      = { .val = var->flt };

    push(tgt);

    P_TAIL(": "GREEN"%s"ANSI_END"[%04x](%.8g) -> %.15lg",
        sym->name, var_addr, var->flt, tgt.val);

    UPDATE_PC();
}

void eval_f_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"[%04x]\n",
        pc[1].sym->name, pc[0].desp);
}

void eval_i(const instr *i)
{
    int     var_addr = pc[0].desp;
    Symbol *sym      = pc[1].sym;
    Cell   *var      = prog + var_addr;
    Cell    tgt      = { .num = var->inum };

    push(tgt);

    P_TAIL(": "GREEN"%s"ANSI_END"[%04x](%i) -> %li",
        sym->name, var_addr, var->inum, tgt.num);

    UPDATE_PC();
}

void eval_i_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"[%04x]\n",
        pc[1].sym->name, pc[0].desp);
}

void eval_l(const instr *i)
{
    int     var_addr = pc[0].desp;
    Symbol *sym      = pc[1].sym;
    Cell   *var      = prog + var_addr;
    Cell    tgt      = { .num = var->num };

    push(tgt);

    P_TAIL(": "GREEN"%s"ANSI_END"[%04x](%li) -> %li",
        sym->name, var_addr, var->num, tgt.num);

    UPDATE_PC();
}

void eval_l_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"[%04x]\n",
        pc[1].sym->name, pc[0].desp);
}

void eval_s(const instr *i)
{
    int     var_addr = pc[0].desp;
    Symbol *sym      = pc[1].sym;
    Cell   *var      = prog + var_addr;
    Cell    tgt      = { .num = var->sht };

    push(tgt);

    P_TAIL(": "GREEN"%s"ANSI_END"[%04x](0x%04hx) -> %li",
        sym->name, var_addr, var->sht, tgt.num);

    UPDATE_PC();
}

void eval_s_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"[%04x]\n",
        pc[1].sym->name, pc[0].desp);
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
    int     var_addr = pc[0].desp;
    Symbol *sym      = pc[1].sym;
    Cell   *var      = prog + var_addr;
    Cell    src      = top();

    *var = src;

    P_TAIL(": %.8g -> "GREEN"%s"ANSI_END"[%04x]",
           src.val, sym->name, var_addr);

    UPDATE_PC();
}

void assign_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].desp);
}

void assign_c(const instr *i) /* assign top value to next value */
{
    int     var_addr = pc[0].desp;
    Symbol *sym      = pc[1].sym;
    Cell   *var      = prog + var_addr;
    Cell    src      = top();

    var->chr = src.chr;

    P_TAIL(": 0x%02x (char) -> "GREEN"%s"ANSI_END"[%04x]",
            src.chr, sym->name, var_addr);

    UPDATE_PC();
}

void assign_c_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void assign_d(const instr *i)
{
    int     var_addr = pc[0].desp;
    Symbol *sym      = pc[1].sym;
    Cell   *var      = prog + var_addr;
    Cell    src      = top();

    var->val = src.val;

    P_TAIL(": %.15lg (double) -> "GREEN"%s"ANSI_END"[%04x]",
            src.val, sym->name, var_addr);

    UPDATE_PC();
}

void assign_d_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void assign_f(const instr *i)
{
    int     var_addr = pc[0].desp;
    Symbol *sym      = pc[1].sym;
    Cell   *var      = prog + var_addr;
    Cell    src      = top();

    var->flt = src.val;

    P_TAIL(": %.15lg (float) -> "GREEN"%s"ANSI_END"[%04x]",
            src.val, sym->name, var_addr);

    UPDATE_PC();
}

void assign_f_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void assign_i(const instr *i)
{
    int     var_addr = pc[0].desp;
    Symbol *sym      = pc[1].sym;
    Cell   *var      = prog + var_addr;
    Cell    src      = top();

    var->inum = src.num;

    P_TAIL(": %li (int) -> "GREEN"%s"ANSI_END"[%04x]",
            src.num, sym->name, var_addr);

    UPDATE_PC();
}

void assign_i_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void assign_l(const instr *i)
{
    int     var_addr = pc[0].desp;
    Symbol *sym      = pc[1].sym;
    Cell   *var      = prog + var_addr;
    Cell    src      = top();

    var->num = src.num;
    P_TAIL(": %li (long) -> "GREEN"%s"ANSI_END"[%04x]",
            src.num, sym->name, var_addr);

    UPDATE_PC();
}

void assign_l_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void assign_s(const instr *i)
{
    int     var_addr = pc[0].desp;
    Symbol *sym      = pc[1].sym;
    Cell   *var      = prog + var_addr;
    Cell    src      = top();

    var->str = var->str;
    P_TAIL(": \"%s\" (string) -> "GREEN"%s"ANSI_END"[%04x]",
            var->str, sym->name, var_addr);

    UPDATE_PC();
}

void assign_s_prt(const instr *i, const Cell *pc)
{
    PR("\n");
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

void print_d(const instr *i) /* pop top value from stack, print it */
{
    Cell d = pop();

    printf("\t\t%32.8g\n", d.val);

    UPDATE_PC();
}

void print_d_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void print_i(const instr *i) /* pop top value from stack, print it */
{
    Cell d = pop();

    printf("\t\t%32li\n", d.num);

    UPDATE_PC();
}

void print_i_prt(const instr *i, const Cell *pc)
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

void bltin0_d(const instr *i) /* evaluate built-in on top of stack */
{
    Symbol *sym = pc[1].sym;
    Cell    res = { .val = sym->ptr0() };

    P_TAIL(": %s() -> %.8lg", sym->name, res.val);

    push(res);

    UPDATE_PC();
}

void bltin0_d_prt(const instr *i, const Cell *pc)
{
    PR("%s\n", pc[1].sym->help);
}

void bltin0_l(const instr *i) /* evaluate built-in on top of stack */
{
    Symbol *sym = pc[1].sym;
    Cell    res = { .num = sym->ptr0() };

    P_TAIL(": %s() -> %li", sym->name, res.num);
    push(res);

    UPDATE_PC();
}

void bltin0_l_prt(const instr *i, const Cell *pc)
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

void bltin1_d(const instr *i) /* evaluate built-in with one argument */
{
    Symbol *sym = pc[1].sym;
    Cell  p   = pop(),
          res = { .val = sym->ptr1( p.val ) };

    P_TAIL(": %s(%.8lg) -> %.8lg",
        sym->name, p.val, res.val);
    push(res);

    UPDATE_PC();
}

void bltin1_d_prt(const instr *i, const Cell *pc)
{
    PR("%s\n", pc[1].sym->help);
}

void bltin1_l(const instr *i) /* evaluate built-in with one argument */
{
    Symbol *sym = pc[1].sym;
    Cell  p   = pop(),
          res = { .num = sym->ptr1( p.val ) };

    P_TAIL(": %s(%.8lg) -> %li",
        sym->name, p.val, res.num);
    push(res);

    UPDATE_PC();
}

void bltin1_l_prt(const instr *i, const Cell *pc)
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

void bltin2_d(const instr *i) /* evaluate built-in with two arguments */
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

void bltin2_d_prt(const instr *i, const Cell *pc)
{
    PR("%s\n", pc[1].sym->help);
}

void bltin2_l(const instr *i) /* evaluate built-in with two arguments */
{
    Symbol *sym = pc[1].sym;
    Cell    p2  = pop(),
            p1  = pop(),
            res = { .num = sym->ptr2( p1.val, p2.val ) };

    P_TAIL(": %s(%.8lg, %.8lg) -> %li",
        sym->name, p1.val, p2.val, res.num);
    push(res);

    UPDATE_PC();
}

void bltin2_l_prt(const instr *i, const Cell *pc)
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

void ge_d(const instr *i) /* greater or equal */
{
    Cell p2  = pop(),
         p1  = pop(),
         res = { .num = p1.val >= p2.val };

    P_TAIL(": %lg >= %lg -> %li", p1.val, p2.val, res.num);
    push(res);

    UPDATE_PC();
}

void ge_d_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void ge_l(const instr *i)
{
    Cell p2  = pop(),
         p1  = pop(),
         res = { .num = p1.num >= p2.num };

    P_TAIL(": %li >= %li -> %li", p1.num, p2.num, res.num);

    push(res);

    UPDATE_PC();
}

void ge_l_prt(const instr *i, const Cell *pc)
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

void le_d(const instr *i) /* less or equal */
{
    Cell  p2  = pop(),
          p1  = pop(),
          res = { .num = p1.val <= p2.val };

    P_TAIL(": %lg <= %lg -> %li", p1.val, p2.val, res.num);
    push(res);

    UPDATE_PC();
}

void le_d_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void le_l(const instr *i)
{
    Cell  p2  = pop(),
          p1  = pop(),
          res = { .num = p1.num <= p2.num };

    P_TAIL(": %li <= %li -> %li", p1.num, p2.num, res.num);
    push(res);

    UPDATE_PC();
}

void le_l_prt(const instr *i, const Cell *pc)
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

void gt_d(const instr *i) /* greater than */
{
    Cell p2  = pop(),
         p1  = pop(),
         res = { .num = p1.val > p2.val };

    P_TAIL(": %lg > %lg -> %li", p1.val, p2.val, res.num);
    push(res);

    UPDATE_PC();
}

void gt_d_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void gt_l(const instr *i)
{
    Cell p2  = pop(),
         p1  = pop(),
         res = { .num = p1.num > p2.num };

    P_TAIL(": %li > %li -> %li", p1.num, p2.num, res.num);
    push(res);

    UPDATE_PC();
}

void gt_l_prt(const instr *i, const Cell *pc)
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

void lt_d(const instr *i) /* less than */
{
    Cell  p2  = pop(),
          p1  = pop(),
          res = { .num = p1.val < p2.val };

    P_TAIL(": %lg < %lg -> %li", p1.val, p2.val, res.num);
    push(res);

    UPDATE_PC();
}

void lt_d_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void lt_l(const instr *i)
{
    Cell  p2  = pop(),
          p1  = pop(),
          res = { .num = p1.num < p2.num };

    P_TAIL(": %li < %li -> %li", p1.num, p2.num, res.num);
    push(res);

    UPDATE_PC();
}

void lt_l_prt(const instr *i, const Cell *pc)
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

void eq_d(const instr *i) /* equal */
{
    Cell p2  = pop(),
         p1  = pop(),
         res = { .num = p1.val == p2.val };

    P_TAIL(": %lg == %lg -> %li", p1.val, p2.val, res.num);
    push(res);

    UPDATE_PC();
}

void eq_d_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void eq_l(const instr *i)
{
    Cell p2  = pop(),
         p1  = pop(),
         res = { .num = p1.num == p2.num };

    P_TAIL(": %li == %li -> %li", p1.num, p2.num, res.num);
    push(res);

    UPDATE_PC();
}

void eq_l_prt(const instr *i, const Cell *pc)
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

void ne_d(const instr *i) /* not equal */
{
    Cell p2  = pop(),
         p1  = pop(),
         res = { .num = p1.val != p2.val };

    P_TAIL(": %lg != %lg -> %li", p1.val, p2.val, res.num);
    push(res);

    UPDATE_PC();
}

void ne_d_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void ne_l(const instr *i)
{
    Cell p2  = pop(),
         p1  = pop(),
         res = { .num = p1.num != p2.num };

    P_TAIL(": %li != %li -> %li", p1.num, p2.num, res.num);
    push(res);

    UPDATE_PC();
}

void ne_l_prt(const instr *i, const Cell *pc)
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

void and_then_i(const instr *i)  /* and_then */
{
    Cell        d  = top();
    const char *op = d.num
                   ? " drop;"
                   : "";

    if (d.num) {
        pop();
        UPDATE_PC();
    } else {
        pc = prog + pc[0].desp;
    }

    P_TAIL(": %li &&%s -> [%04lx]", d.num, op, pc - prog);
}

void and_then_i_prt(const instr *i, const Cell *pc)
{
    PR("[%04x]\n", pc[0].desp);
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

void or_else_i(const instr *i)  /* or_else */
{
    Cell d = top();
    const char *op = d.num ? " drop;" : "";

    if (!d.num) {
        pop();
        UPDATE_PC();
    } else {
        pc = prog + pc[0].desp;
    }
    P_TAIL(": %li ||%s -> [%04lx]",
        d.num, op, pc - prog);
}

void or_else_i_prt(const instr *i, const Cell *pc)
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
    PR(GREEN"%s"ANSI_END"[%04x], args=%ld\n",
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
    int         arg = pc[0].args;
    const char *nam = pc[1].str;
    Cell        d   = *getarg(arg);

    P_TAIL(": "GREEN"%s"ANSI_END"<%+d> -> %.8g", nam, arg, d.val);

    push(d);

    UPDATE_PC();
}

void argeval_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].args);
}

void argeval_c(const instr *i) /* push argument onto stack */
{
    int         arg = pc[0].args;
    const char *nam = pc[1].str;
    Cell        d   = *getarg(arg);

    P_TAIL(": "GREEN"%s"ANSI_END"<%+d> -> 0x%02x", nam, arg, d.chr);

    d.num = d.chr;
    push(d);

    UPDATE_PC();
}

void argeval_c_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].args);
}

void argeval_d(const instr *i) /* push argument onto stack */
{
    int         arg = pc[0].args;
    const char *nam = pc[1].str;
    Cell        d   = *getarg(arg);

    P_TAIL(": "GREEN"%s"ANSI_END"<%+d> -> %.15lg", nam, arg, d.val);
    push(d);

    UPDATE_PC();
}

void argeval_d_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].args);
}

void argeval_f(const instr *i) /* push argument onto stack */
{
    int         arg = pc[0].args;
    const char *nam = pc[1].str;
    Cell        d   = *getarg(arg);

    P_TAIL(": "GREEN"%s"ANSI_END"<%+d> -> %.7g", nam, arg, d.flt);

    d.val = d.flt;
    push(d);

    UPDATE_PC();
}

void argeval_f_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].args);
}

void argeval_i(const instr *i) /* push argument onto stack */
{
    int         arg = pc[0].args;
    const char *nam = pc[1].str;
    Cell        d   = *getarg(arg);

    P_TAIL(": "GREEN"%s"ANSI_END"<%+d> -> %i", nam, arg, d.inum);

    d.num = d.inum;
    push(d);

    UPDATE_PC();
}

void argeval_i_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].args);
}

void argeval_l(const instr *i) /* push argument onto stack */
{
    int         arg = pc[0].args;
    const char *nam = pc[1].str;
    Cell        d   = *getarg(arg);

    P_TAIL(": "GREEN"%s"ANSI_END"<%+d> -> %li", nam, arg, d.num);
    push(d);

    UPDATE_PC();
}

void argeval_l_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].args);
}

void argassign(const instr *i) /* store top of stack in argument */
{
    int         arg = pc[0].args;
    const char *nam = pc[1].str;
    Cell        d   = top();
    Cell       *ref = getarg(arg);

    ref->val = d.val;
    P_TAIL(": %.8g -> "GREEN"%s"ANSI_END"<%+d>", d.val, nam, arg);

    UPDATE_PC();
}

void argassign_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].args);
}

void argassign_c(const instr *i) /* store top of stack in argument */
{
    int         arg = pc[0].args;
    const char *nam = pc[1].str;
    Cell        d   = top();
    Cell       *ref = getarg(arg);

    ref->chr = d.num;
    P_TAIL(": 0x%02hhx -> "GREEN"%s"ANSI_END"<%+d>",
           ref->chr, nam, arg);

    UPDATE_PC();
}

void argassign_c_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void argassign_d(const instr *i)
{
    int         arg = pc[0].args;
    const char *nam = pc[1].str;
    Cell        d   = top();
    Cell       *ref = getarg(arg);

    ref->val = d.val;
    P_TAIL(": %.15lg -> "GREEN"%s"ANSI_END"<%+d>",
            ref->val, nam, arg);

    UPDATE_PC();
}

void argassign_d_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void argassign_f(const instr *i)
{
    int         arg = pc[0].args;
    const char *nam = pc[1].str;
    Cell        d   = top();
    Cell       *ref = getarg(arg);

    ref->flt = d.val;
    P_TAIL(": %.15g -> "GREEN"%s"ANSI_END"<%+d>",
            ref->flt, nam, arg);

    UPDATE_PC();
}

void argassign_f_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void argassign_i(const instr *i)
{
    int         arg = pc[0].args;
    const char *nam = pc[1].str;
    Cell        d   = top();
    Cell       *ref = getarg(arg);

    ref->inum = d.num;
    P_TAIL(": %i -> "GREEN"%s"ANSI_END"<%+d>",
            ref->inum, nam, arg);

    UPDATE_PC();
}

void argassign_i_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void argassign_l(const instr *i)
{
    int         arg = pc[0].args;
    const char *nam = pc[1].str;
    Cell        d   = top();
    Cell       *ref = getarg(arg);

    ref->num = d.num;
    P_TAIL(": %li -> "GREEN"%s"ANSI_END"<%+d>",
            ref->num, nam, arg);

    UPDATE_PC();
}

void argassign_l_prt(const instr *i, const Cell *pc)
{
    PR("\n");
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

    PRG(" \"%s\"", progp[1].str);
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

void prexpr_i(const instr *i)  /* print numeric value */
{
    P_TAIL("\n");
    printf("%li", pop().num);

    UPDATE_PC();
}

void prexpr_i_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void prexpr_d(const instr *i)  /* print numeric value */
{
    P_TAIL("\n");
    printf("%.15lg", pop().val);

    UPDATE_PC();
}

void prexpr_d_prt(const instr *i, const Cell *pc)
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
    while (ip->inst != INST_STOP) {
        const instr *i = instruction_set + ip->inst;
        if (ip == progbase) {
            printf("START:\n");
        }
        i->print(i, ip); /* LCU: Thu Apr 10 14:52:23 -05 2025
                          * Aqui es donde Edward desaparecio en el rio Orinoco. */
        ip += i->n_cells;
    }
    const instr *stop = instruction_set + INST_STOP;
    stop->print(stop, ip); /* STOP :) */
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
    pc = d.val
        ? pc + i->n_cells
        : prog + pc[0].desp;
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
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;

    ++var->val;
    push(*var);
    P_TAIL(": ++"GREEN"%s"ANSI_END"[%04x] -> %.8lg",
            sym->name, addr, var->val);

    UPDATE_PC();
}

void inceval_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].desp);
}

void inceval_c(const instr *i) /* 1. incremento, 2. evaluo la variable */
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .num = ++var->chr };

    push(tgt);

    P_TAIL(": ++"GREEN"%s"ANSI_END"[%04x] -> %li",
            sym->name, addr, tgt.num);

    UPDATE_PC();
}

void inceval_c_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void inceval_d(const instr *i)
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .val = ++var->val };

    push(tgt);

    P_TAIL(": ++"GREEN"%s"ANSI_END"[%04x] -> %.15lg",
            sym->name, addr, tgt.val);

    UPDATE_PC();
}

void inceval_d_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void inceval_f(const instr *i)
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .val = ++var->flt };

    push(tgt);
    P_TAIL(": ++"GREEN"%s"ANSI_END"[%04x] -> %.15g",
            sym->name, addr, tgt.val);

    UPDATE_PC();
}

void inceval_f_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void inceval_i(const instr *i)
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .num = ++var->inum };

    push(tgt);
    P_TAIL(": ++"GREEN"%s"ANSI_END"[%04x] -> %li",
            sym->name, addr, tgt.num);

    UPDATE_PC();
}

void inceval_i_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void inceval_l(const instr *i)
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .num = ++var->num };

    push(tgt);
    P_TAIL(": ++"GREEN"%s"ANSI_END"[%04x] -> %li",
            sym->name, addr, tgt.num);

    UPDATE_PC();
}

void inceval_l_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void inceval_s(const instr *i)
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .num = ++var->sht };

    push(tgt);
    P_TAIL(": ++"GREEN"%s"ANSI_END"[%04x] -> %li",
            sym->name, addr, tgt.num);

    UPDATE_PC();
}

void inceval_s_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void evalinc(const instr *i) /* 1. copio valor,
                              * 2. evaluo variable,
                              * 3. devuelvo valor copiado. */
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;

    push(*var);
    ++var->val;
    P_TAIL(": "GREEN"%s"ANSI_END"++[%04x] -> %.8lg",
            sym->name, addr, var->val);

    UPDATE_PC();
}

void evalinc_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].desp);
}

void evalinc_c(const instr *i) /* 1. copio valor,
                                * 2. evaluo variable,
                                * 3. devuelvo valor copiado. */
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .num = var->chr++ };

    push(tgt);

    P_TAIL(": "GREEN"%s"ANSI_END"++[%04x] -> %li",
            sym->name, addr, tgt.num);

    UPDATE_PC();
}

void evalinc_c_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void evalinc_d(const instr *i)
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .val = var->val++ };

    push(tgt);

    P_TAIL(": "GREEN"%s"ANSI_END"++[%04x] -> %.15lg",
            sym->name, addr, tgt.val);

    UPDATE_PC();
}

void evalinc_d_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void evalinc_f(const instr *i)
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .val = var->flt++ };

    push(tgt);

    P_TAIL(": "GREEN"%s"ANSI_END"++[%04x] -> %.15lg",
            sym->name, addr, tgt.val);

    UPDATE_PC();
}

void evalinc_f_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void evalinc_i(const instr *i)
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .num = var->inum++ };

    push(tgt);

    P_TAIL(": "GREEN"%s"ANSI_END"++[%04x] -> %li",
            sym->name, addr, tgt.num);

    UPDATE_PC();
}

void evalinc_i_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void evalinc_l(const instr *i)
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .num = var->num++ };

    push(tgt);

    P_TAIL(": "GREEN"%s"ANSI_END"++[%04x] -> %li",
            sym->name, addr, tgt.num);

    UPDATE_PC();
}

void evalinc_l_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void evalinc_s(const instr *i)
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .num = ++var->sht };

    push(tgt);

    P_TAIL(": "GREEN"%s"ANSI_END"++[%04x] -> %li",
            sym->name, addr, tgt.num);

    UPDATE_PC();
}

void evalinc_s_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void deceval(const instr *i) /* assign top value to next value */
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .val = --var->val };

    push(tgt);

    P_TAIL(": "GREEN"%s"ANSI_END"[%04x] -> %.8lg",
            sym->name, addr, tgt.val);

    UPDATE_PC();
}

void deceval_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].desp);
}

void deceval_c(const instr *i) /* assign top value to next value */
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .num = --var->chr };

    push(tgt);

    P_TAIL(": "GREEN"%s"ANSI_END"[%04x] -> %li",
            sym->name, addr, tgt.num);

    UPDATE_PC();
}

void deceval_c_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"[%04x]\n",
            pc[1].sym->name, pc[0].desp);
}

void deceval_d(const instr *i) /* assign top value to next value */
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .val = --var->val };

    push(tgt);

    P_TAIL(": "GREEN"%s"ANSI_END"[%04x] -> %.15lg",
            sym->name, addr, tgt.val);

    UPDATE_PC();
}

void deceval_d_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"[%04x]\n",
            pc[1].sym->name, pc[0].desp);
}

void deceval_f(const instr *i) /* assign top value to next value */
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .val = --var->flt };

    push(tgt);

    P_TAIL(": "GREEN"%s"ANSI_END"[%04x] -> %.15lg",
            sym->name, addr, tgt.val);

    UPDATE_PC();
}

void deceval_f_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].desp);
}

void deceval_i(const instr *i) /* assign top value to next value */
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .num = --var->inum };

    push(tgt);

    P_TAIL(": "GREEN"%s"ANSI_END"[%04x] -> %li",
            sym->name, addr, tgt.num);

    UPDATE_PC();
}

void deceval_i_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].desp);
}

void deceval_l(const instr *i) /* assign top value to next value */
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .num = --var->num };

    push(tgt);

    P_TAIL(": "GREEN"%s"ANSI_END"[%04x] -> %li",
            sym->name, addr, tgt.num);

    UPDATE_PC();
}

void deceval_l_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].desp);
}

void deceval_s(const instr *i) /* assign top value to next value */
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .num = --var->sht };

    push(tgt);

    P_TAIL(": "GREEN"%s"ANSI_END"[%04x] -> %li",
            sym->name, addr, tgt.num);

    UPDATE_PC();
}

void deceval_s_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].desp);
}

void evaldec(const instr *i) /* assign top value to next value */
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .val = var->val-- };

    push(tgt);

    P_TAIL(": "GREEN"%s"ANSI_END"[%04x] -> %.8lg",
            sym->name, addr, tgt.val);

    UPDATE_PC();
}

void evaldec_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].desp);
}

void evaldec_c(const instr *i) /* assign top value to next value */
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .num = var->chr-- };

    push(tgt);

    P_TAIL(": "GREEN"%s"ANSI_END"[%04x] -> %li",
            sym->name, addr, tgt.num);

    UPDATE_PC();
}

void evaldec_c_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].desp);
}

void evaldec_d(const instr *i) /* assign top value to next value */
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .val = var->val-- };

    push(tgt);

    P_TAIL(": "GREEN"%s"ANSI_END"[%04x] -> %.15lg",
            sym->name, addr, tgt.val);

    UPDATE_PC();
}

void evaldec_d_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].desp);
}

void evaldec_f(const instr *i) /* assign top value to next value */
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .val = var->flt-- };

    push(tgt);
    P_TAIL(": "GREEN"%s"ANSI_END"[%04x] -> %.15lg",
            sym->name, addr, tgt.val);

    UPDATE_PC();
}

void evaldec_f_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].desp);
}

void evaldec_i(const instr *i) /* assign top value to next value */
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .num = var->inum-- };

    push(tgt);
    P_TAIL(": "GREEN"%s"ANSI_END"[%04x] -> %li",
            sym->name, addr, tgt.num);

    UPDATE_PC();
}

void evaldec_i_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].desp);
}

void evaldec_l(const instr *i) /* assign top value to next value */
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .num = var->num-- };

    push(tgt);

    P_TAIL(": "GREEN"%s"ANSI_END"[%04x] -> %li",
            sym->name, addr, tgt.num);

    UPDATE_PC();
}

void evaldec_l_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].desp);
}

void evaldec_s(const instr *i) /* assign top value to next value */
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .num = var->sht-- };

    push(tgt);
    P_TAIL(": "GREEN"%s"ANSI_END"[%04x] -> %li",
            sym->name, addr, tgt.num);

    UPDATE_PC();
}

void evaldec_s_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].desp);
}

void addvar(const instr *i) /* assign top value to next value */
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .val = var->val += pop().val };

    push(tgt);

    P_TAIL(": %.8lg -> "GREEN"%s"ANSI_END"[%04x] -> %.15lg",
            var->val, sym->name, addr, tgt.val);

    UPDATE_PC();
}

void addvar_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].desp);
}

void addvar_c(const instr *i) /* assign top value to next value */
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .num = var->chr += pop().num };

    push(tgt);

    P_TAIL(": 0x%02hhx -> "GREEN"%s"ANSI_END"[%04x] -> %li",
            var->chr, sym->name, addr, tgt.num);

    UPDATE_PC();
}

void addvar_c_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void addvar_d(const instr *i)
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .val = var->val += pop().val };

    push(tgt);

    P_TAIL(": %.15lg -> "GREEN"%s"ANSI_END"[%04x] -> %.15lg",
            var->val, sym->name, addr, tgt.val);

    UPDATE_PC();
}

void addvar_d_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void addvar_f(const instr *i)
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .val = var->flt += pop().val };

    push(tgt);

    P_TAIL(": %.8g -> "GREEN"%s"ANSI_END"[%04x] -> %.15lg",
            var->flt, sym->name, addr, tgt.val);

    UPDATE_PC();
}

void addvar_f_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void addvar_i(const instr *i)
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .num = var->inum += pop().num };

    push(tgt);

    P_TAIL(": %i -> "GREEN"%s"ANSI_END"[%04x] -> %li",
            var->inum, sym->name, addr, tgt.num);

    UPDATE_PC();
}

void addvar_i_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void addvar_l(const instr *i)
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .num = var->num += pop().num };

    push(tgt);

    P_TAIL(": %li -> "GREEN"%s"ANSI_END"[%04x] -> %li",
            var->num, sym->name, addr, tgt.num);

    UPDATE_PC();
}

void addvar_l_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void addvar_s(const instr *i)
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .num = var->sht += pop().num };

    push(tgt);

    P_TAIL(": 0x%04hx -> "GREEN"%s"ANSI_END"[%04x] -> %li",
            var->sht, sym->name, addr, tgt.num);

    UPDATE_PC();
}

void addvar_s_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void subvar(const instr *i) /* assign top value to next value */
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .val = var->val -= pop().val };

    push(tgt);
    P_TAIL(": %.8lg -> "GREEN"%s"ANSI_END"[%04x] -> %.15lg",
            var->val, sym->name, addr, tgt.val);

    UPDATE_PC();
}

void subvar_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].desp);
}

void subvar_c(const instr *i) /* assign top value to next value */
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .num = var->chr -= pop().num };

    push(tgt);

    P_TAIL(": 0x%02hhx -> "GREEN"%s"ANSI_END"[%04x] -> %li",
            var->chr, sym->name, addr, tgt.num);

    UPDATE_PC();
}

void subvar_c_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void subvar_d(const instr *i)
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .val = var->val -= pop().val };

    push(tgt);

    P_TAIL(": %.15lg -> "GREEN"%s"ANSI_END"[%04x] -> %.15lg",
            var->val, sym->name, addr, tgt.val);

    UPDATE_PC();
}

void subvar_d_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void subvar_f(const instr *i)
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .val = var->flt -= pop().val };

    push(tgt);

    P_TAIL(": %.7g -> "GREEN"%s"ANSI_END"[%04x] -> %.15lg",
            var->flt, sym->name, addr, tgt.val);

    UPDATE_PC();
}

void subvar_f_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void subvar_i(const instr *i)
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .num = var->inum -= pop().num };

    push(tgt);

    P_TAIL(": %i -> "GREEN"%s"ANSI_END"[%04x] -> %li",
            var->inum, sym->name, addr, tgt.num);

    UPDATE_PC();
}

void subvar_i_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void subvar_l(const instr *i)
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .num = var->num -= pop().num };

    push(tgt);
    P_TAIL(": %li -> "GREEN"%s"ANSI_END"[%04x] -> %li",
            var->num, sym->name, addr, tgt.num);

    UPDATE_PC();
}

void subvar_l_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void subvar_s(const instr *i)
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .num = var->sht -= pop().num };

    push(tgt);
    P_TAIL(": 0x%04hx -> "GREEN"%s"ANSI_END"[%04x] -> %li",
            var->sht, sym->name, addr, tgt.num);

    UPDATE_PC();
}

void subvar_s_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void mulvar(const instr *i) /* assign top value to next value */
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .val = var->val *= pop().val };

    push(tgt);

    P_TAIL(": %.8lg -> "GREEN"%s"ANSI_END"[%04x] -> %.15lg",
            var->val, sym->name, addr, tgt.val);

    UPDATE_PC();
}

void mulvar_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].desp);
}

void mulvar_c(const instr *i) /* assign top value to next value */
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .num = var->chr *= pop().num };

    push(tgt);

    P_TAIL(": 0x%02hhx -> "GREEN"%s"ANSI_END"[%04x] -> %li",
            var->chr, sym->name, addr, tgt.num);

    UPDATE_PC();
}

void mulvar_c_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void mulvar_d(const instr *i)
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .val = var->val *= pop().val };

    push(tgt);

    P_TAIL(": %.15lg -> "GREEN"%s"ANSI_END"[%04x] %.15lg",
            var->val, sym->name, addr, tgt.val);

    UPDATE_PC();
}

void mulvar_d_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void mulvar_f(const instr *i)
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .val = var->flt *= pop().val };

    push(tgt);

    P_TAIL(": %.7g -> "GREEN"%s"ANSI_END"[%04x] -> %.15lg",
            var->flt, sym->name, addr, tgt.val);

    UPDATE_PC();
}

void mulvar_f_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void mulvar_i(const instr *i)
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .num = var->inum *= pop().num };

    push(tgt);

    P_TAIL(": %i -> "GREEN"%s"ANSI_END"[%04x] -> %li",
            var->inum, sym->name, addr, tgt.num);

    UPDATE_PC();
}

void mulvar_i_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void mulvar_l(const instr *i)
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .num = var->num *= pop().num };

    push(tgt);

    P_TAIL(": %li -> "GREEN"%s"ANSI_END"[%04x] -> %li",
            var->num, sym->name, addr, tgt.num);

    UPDATE_PC();
}

void mulvar_l_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void mulvar_s(const instr *i)
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .num = var->sht *= pop().num };

    push(tgt);

    P_TAIL(": 0x%04hx -> "GREEN"%s"ANSI_END"[%04x] -> %li",
            var->sht, sym->name, addr, tgt.num);

    UPDATE_PC();
}

void mulvar_s_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void divvar(const instr *i) /* assign top value to next value */
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .val = var->val /= pop().val };

    push(tgt);

    P_TAIL(": %.8lg -> "GREEN"%s"ANSI_END"[%04x] -> %.15lg",
            var->val, sym->name, addr, tgt.val);

    UPDATE_PC();
}

void divvar_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].desp);
}

void divvar_c(const instr *i) /* assign top value to next value */
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .num = var->chr /= pop().num };

    push(tgt);

    P_TAIL(": 0x%02hhx -> "GREEN"%s"ANSI_END"[%04x] -> %li",
            var->chr, sym->name, addr, tgt.num);

    UPDATE_PC();
}

void divvar_c_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void divvar_d(const instr *i)
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .val = var->val /= pop().val };

    push(tgt);

    P_TAIL(": %.15lg -> "GREEN"%s"ANSI_END"[%04x] -> %.15lg",
            var->val, sym->name, addr, tgt.val);

    UPDATE_PC();
}

void divvar_d_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void divvar_f(const instr *i)
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .val = var->flt /= pop().val };

    push(tgt);

    P_TAIL(": %.7g -> "GREEN"%s"ANSI_END"[%04x] -> %.15lg",
            var->flt, sym->name, addr, tgt.val);

    UPDATE_PC();
}

void divvar_f_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void divvar_i(const instr *i)
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .num = var->inum /= pop().num };

    push(tgt);

    P_TAIL(": %i -> "GREEN"%s"ANSI_END"[%04x] -> %li",
            var->inum, sym->name, addr, tgt.num);

    UPDATE_PC();
}

void divvar_i_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void divvar_l(const instr *i)
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .num = var->num /= pop().num };

    push(tgt);

    P_TAIL(": %li -> "GREEN"%s"ANSI_END"[%04x] -> %li",
            var->num, sym->name, addr, tgt.num);

    UPDATE_PC();
}

void divvar_l_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void divvar_s(const instr *i)
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .num = var->sht /= pop().num };

    push(tgt);

    P_TAIL(": 0x%04hx -> "GREEN"%s"ANSI_END"[%04x] -> %li",
            var->sht, sym->name, addr, tgt.num);

    UPDATE_PC();
}

void divvar_s_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void modvar(const instr *i) /* assign top value to next value */
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .val = var->val = fmod(var->val, pop().val) };

    push(tgt);

    P_TAIL(": %.8lg -> "GREEN"%s"ANSI_END"[%04x] -> %.15lg",
            var->val, sym->name, addr, tgt.val);

    UPDATE_PC();
}

void modvar_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].desp);
}

void modvar_c(const instr *i) /* assign top value to next value */
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .num = var->chr %= pop().num };

    push(tgt);

    P_TAIL(": 0x%02hhx -> "GREEN"%s"ANSI_END"[%04x] -> %li",
            var->chr, sym->name, addr, tgt.num);

    UPDATE_PC();
}

void modvar_c_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void modvar_d(const instr *i)
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .val = var->val = fmod(var->val, pop().val) };

    push(tgt);

    P_TAIL(": %.15lg -> "GREEN"%s"ANSI_END"[%04x] -> %.15lg",
            var->val, sym->name, addr, tgt.val);

    UPDATE_PC();
}

void modvar_d_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void modvar_f(const instr *i)
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .val = var->flt = fmod(var->flt, pop().val) };

    push(tgt);
    P_TAIL(": %.7g -> "GREEN"%s"ANSI_END"[%04x] -> %.15lg",
            var->flt, sym->name, addr, tgt.val);

    UPDATE_PC();
}

void modvar_f_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void modvar_i(const instr *i)
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .num = var->inum %= pop().num };

    push(tgt);

    P_TAIL(": %i -> "GREEN"%s"ANSI_END"[%04x] -> %li",
            var->inum, sym->name, addr, tgt.num);

    UPDATE_PC();
}

void modvar_i_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void modvar_l(const instr *i)
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .num = var->num %= pop().num };

    push(tgt);

    P_TAIL(": %li -> "GREEN"%s"ANSI_END"[%04x] -> %li",
            var->num, sym->name, addr, tgt.num);

    UPDATE_PC();
}

void modvar_l_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void modvar_s(const instr *i)
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .num = var->sht %= pop().num };

    push(tgt);

    P_TAIL(": 0x%04hx -> "GREEN"%s"ANSI_END"[%04x] -> %li",
            var->sht, sym->name, addr, tgt.num);

    UPDATE_PC();
}

void modvar_s_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void pwrvar(const instr *i) /* assign top value to next value */
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .val = var->val = pow(var->val, pop().val) };

    push(tgt);

    P_TAIL(": %.8lg -> "GREEN"%s"ANSI_END"[%04x] -> %.15lg",
            var->val, sym->name, addr, tgt.val);

    UPDATE_PC();
}

void pwrvar_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].desp);
}

void pwrvar_c(const instr *i) /* assign top value to next value */
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .num = var->chr = fast_pwr_i(var->chr, pop().num) };

    push(tgt);

    P_TAIL(": %.8lg -> "GREEN"%s"ANSI_END"[%04x] -> %li",
            var->val, sym->name, addr, tgt.num);

    UPDATE_PC();
}

void pwrvar_c_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void pwrvar_d(const instr *i)
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .val = var->val = pow(var->val, pop().val) };

    push(tgt);

    P_TAIL(": %.15lg -> "GREEN"%s"ANSI_END"[%04x] -> %.15lg",
            var->val, sym->name, addr, tgt.val);

    UPDATE_PC();
}

void pwrvar_d_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void pwrvar_f(const instr *i)
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .val = var->flt = pow(var->flt, pop().val) };

    push(tgt);

    P_TAIL(": %.8g -> "GREEN"%s"ANSI_END"[%04x] -> %.15lg",
            var->flt, sym->name, addr, tgt.val);

    UPDATE_PC();
}

void pwrvar_f_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void pwrvar_i(const instr *i)
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .num = var->inum = fast_pwr_i(var->inum, pop().num) };

    push(tgt);

    P_TAIL(": %.8lg -> "GREEN"%s"ANSI_END"[%04x] -> %li",
            var->val, sym->name, addr, tgt.num);

    UPDATE_PC();
}

void pwrvar_i_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void pwrvar_l(const instr *i)
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .num = var->num = fast_pwr_i(var->num, pop().num) };

    push(tgt);
    P_TAIL(": %li -> "GREEN"%s"ANSI_END"[%04x] -> %li",
            var->num, sym->name, addr, tgt.num);

    UPDATE_PC();
}

void pwrvar_l_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void pwrvar_s(const instr *i)
{
    int     addr = pc[0].desp;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .num = var->sht = fast_pwr_i(var->sht, pop().num) };

    push(tgt);

    P_TAIL(": 0x%04hx -> "GREEN"%s"ANSI_END"[%04x] -> %li",
            var->sht, sym->name, addr, tgt.num);

    UPDATE_PC();
}

void pwrvar_s_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void arginc(const instr *i) /* evaluo e incremento */
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .val = lvar->val++ };

    push(tgt);

    P_TAIL(": %.8g -> "GREEN"%s"ANSI_END"<%+d> -> %lg",
            lvar->val, lvar_name, lvar_offset, tgt.val);

    UPDATE_PC();
}

void arginc_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].args);
}

void arginc_c(const instr *i) /* evaluo e incremento */
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .num = lvar->chr++ };

    push(tgt);

    P_TAIL(": 0x%02hhx -> "GREEN"%s"ANSI_END"<%+d> -> %li",
            lvar->chr, lvar_name, lvar_offset, tgt.num);

    UPDATE_PC();
}

void arginc_c_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void arginc_d(const instr *i)
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .val = lvar->val++ };

    push(tgt);

    P_TAIL(": %.8g -> "GREEN"%s"ANSI_END"<%+d> -> %.15lg",
            lvar->val, lvar_name, lvar_offset, tgt.val);

    UPDATE_PC();
}

void arginc_d_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void arginc_f(const instr *i)
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .val = lvar->flt++ };

    push(tgt);

    P_TAIL(": %.7g -> "GREEN"%s"ANSI_END"<%+d> -> %.15lg",
            lvar->flt, lvar_name, lvar_offset, tgt.val);

    UPDATE_PC();
}

void arginc_f_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void arginc_i(const instr *i)
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .num = lvar->inum++ };

    push(tgt);

    P_TAIL(": %i -> "GREEN"%s"ANSI_END"<%+d> -> %li",
            lvar->inum, lvar_name, lvar_offset, tgt.num);

    UPDATE_PC();
}

void arginc_i_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void arginc_l(const instr *i)
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .num = lvar->num++ };

    push(tgt);

    P_TAIL(": %li -> "GREEN"%s"ANSI_END"<%+d> -> %li",
            lvar->num, lvar_name, lvar_offset, tgt.num);

    UPDATE_PC();
}

void arginc_l_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void arginc_s(const instr *i)
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .num = lvar->sht++ };

    push(tgt);

    P_TAIL(": 0x%04hx -> "GREEN"%s"ANSI_END"<%+d> -> %li",
            lvar->sht, lvar_name, lvar_offset, tgt.num);

    UPDATE_PC();
}

void arginc_s_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void incarg(const instr *i) /* assign top value to next value */
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .val = ++lvar->val };

    push(tgt);

    P_TAIL(": %.8lg -> "GREEN"%s"ANSI_END"<%+d> -> %.8lg",
            lvar->val, lvar_name, lvar_offset, tgt.val);

    UPDATE_PC();
}

void incarg_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].args);
}

void incarg_c(const instr *i) /* assign top value to next value */
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .num = ++lvar->chr };

    push(tgt);

    P_TAIL(": 0x%02hhx -> "GREEN"%s"ANSI_END"<%+d> -> %li",
            lvar->chr, lvar_name, lvar_offset, tgt.num);

    UPDATE_PC();
}

void incarg_c_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void incarg_d(const instr *i)
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .val = ++lvar->val };

    push(tgt);

    P_TAIL(": %.15lg -> "GREEN"%s"ANSI_END"<%+d> -> %.15lg",
            lvar->val, lvar_name, lvar_offset, tgt.val);

    UPDATE_PC();
}

void incarg_d_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void incarg_f(const instr *i)
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .val = ++lvar->flt };

    push(tgt);

    P_TAIL(": %.8g -> "GREEN"%s"ANSI_END"<%+d> -> %.15lg",
            lvar->flt, lvar_name, lvar_offset, tgt.val);

    UPDATE_PC();
}

void incarg_f_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void incarg_i(const instr *i)
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .num = ++lvar->inum };

    push(tgt);

    P_TAIL(": %i -> "GREEN"%s"ANSI_END"<%+d> -> %li",
            lvar->inum, lvar_name, lvar_offset, tgt.num);

    UPDATE_PC();
}

void incarg_i_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void incarg_l(const instr *i)
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .num = ++lvar->num };

    push(tgt);

    P_TAIL(": %li -> "GREEN"%s"ANSI_END"<%+d> -> %li",
            lvar->num, lvar_name, lvar_offset, tgt.num);

    UPDATE_PC();
}

void incarg_l_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void incarg_s(const instr *i)
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .num = ++lvar->sht };

    push(tgt);

    P_TAIL(": 0x%04hx -> "GREEN"%s"ANSI_END"<%+d> -> %li",
            lvar->sht, lvar_name, lvar_offset, tgt.num);

    UPDATE_PC();
}

void incarg_s_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void decarg(const instr *i) /* assign top value to next value */
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .val = --lvar->val };

    push(tgt);

    P_TAIL(": %.8lg -> "GREEN"%s"ANSI_END"<%+d> -> %.8lg",
            lvar->val, lvar_name, lvar_offset, tgt.val);

    UPDATE_PC();
}

void decarg_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].args);
}

void decarg_c(const instr *i) /* assign top value to next value */
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .num = --lvar->chr };

    push(tgt);

    P_TAIL(": 0x%02hhx -> "GREEN"%s"ANSI_END"<%+d> -> %li",
            lvar->chr, lvar_name, lvar_offset, tgt.num);

    UPDATE_PC();
}

void decarg_c_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].args);
}

void decarg_d(const instr *i) /* assign top value to next value */
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .val = --lvar->val };

    push(tgt);

    P_TAIL(": %.15lg -> "GREEN"%s"ANSI_END"<%+d> -> %.15lg",
            lvar->val, lvar_name, lvar_offset, tgt.val);

    UPDATE_PC();
}

void decarg_d_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].args);
}

void decarg_f(const instr *i) /* assign top value to next value */
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .val = --lvar->flt };

    push(tgt);

    P_TAIL(": %.7g -> "GREEN"%s"ANSI_END"<%+d> -> %.15lg",
            lvar->flt, lvar_name, lvar_offset, tgt.val);

    UPDATE_PC();
}

void decarg_f_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].args);
}

void decarg_i(const instr *i) /* assign top value to next value */
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .num = --lvar->inum };

    push(tgt);

    P_TAIL(": %i -> "GREEN"%s"ANSI_END"<%+d> -> %li",
            lvar->inum, lvar_name, lvar_offset, tgt.num);

    UPDATE_PC();
}

void decarg_i_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].args);
}

void decarg_l(const instr *i) /* assign top value to next value */
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .num = --lvar->num };

    push(tgt);

    P_TAIL(": %li -> "GREEN"%s"ANSI_END"<%+d> -> %li",
            lvar->num, lvar_name, lvar_offset, tgt.num);

    UPDATE_PC();
}

void decarg_l_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].args);
}

void decarg_s(const instr *i) /* assign top value to next value */
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .num = --lvar->sht };

    push(tgt);

    P_TAIL(": 0x%04hx -> "GREEN"%s"ANSI_END"<%+d> -> %li",
            lvar->sht, lvar_name, lvar_offset, tgt.num);

    UPDATE_PC();
}

void decarg_s_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].args);
}

void argdec(const instr *i) /* assign top value to next value */
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .val = lvar->val-- };

    push(tgt);

    P_TAIL(": %.8lg -> "GREEN"%s"ANSI_END"<%+d> -> %.8lg",
            lvar->val, lvar_name, lvar_offset, tgt.val);

    UPDATE_PC();
}

void argdec_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].args);
}

void argdec_c(const instr *i) /* assign top value to next value */
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .num = lvar->chr-- };

    push(tgt);

    P_TAIL(": 0x%02hhx -> "GREEN"%s"ANSI_END"<%+d> -> %li",
            lvar->chr, lvar_name, lvar_offset, tgt.num);

    UPDATE_PC();
}

void argdec_c_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].args);
}

void argdec_d(const instr *i) /* assign top value to next value */
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .val = lvar->val-- };

    push(tgt);

    P_TAIL(": %.8lg -> "GREEN"%s"ANSI_END"<%+d> -> %.8lg",
            lvar->val, lvar_name, lvar_offset, tgt.val);

    UPDATE_PC();
}

void argdec_d_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].args);
}

void argdec_f(const instr *i) /* assign top value to next value */
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .val = lvar->flt-- };

    push(tgt);

    P_TAIL(": %.7g -> "GREEN"%s"ANSI_END"<%+d> -> %.15lg",
            lvar->flt, lvar_name, lvar_offset, tgt.val);

    UPDATE_PC();
}

void argdec_f_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].args);
}

void argdec_i(const instr *i) /* assign top value to next value */
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .num = lvar->inum-- };

    push(tgt);

    P_TAIL(": %i -> "GREEN"%s"ANSI_END"<%+d> -> %li",
            lvar->inum, lvar_name, lvar_offset, tgt.num);

    UPDATE_PC();
}

void argdec_i_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].args);
}

void argdec_l(const instr *i) /* assign top value to next value */
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .num = lvar->num-- };

    push(tgt);

    P_TAIL(": %li -> "GREEN"%s"ANSI_END"<%+d> -> %li",
            lvar->num, lvar_name, lvar_offset, tgt.num);

    UPDATE_PC();
}

void argdec_l_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].args);
}

void argdec_s(const instr *i) /* assign top value to next value */
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .num = lvar->sht-- };

    push(tgt);

    P_TAIL(": 0x%04hx -> "GREEN"%s"ANSI_END"<%+d> -> %li",
            lvar->sht, lvar_name, lvar_offset, tgt.num);

    UPDATE_PC();
}

void argdec_s_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].args);
}

void addarg(const instr *i) /* assign top value to next value */
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .val = lvar->val += pop().val };

    push(tgt);

    P_TAIL(": %.8lg -> "GREEN"%s"ANSI_END"<%+d> -> %.8lg",
            lvar->val, lvar_name, lvar_offset, tgt.val);

    UPDATE_PC();
}

void addarg_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].args);
}

void addarg_c(const instr *i) /* assign top value to next value */
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .num = lvar->chr += pop().num };

    push(tgt);

    P_TAIL(": 0x%02hhx -> "GREEN"%s"ANSI_END"<%+d> -> %li",
            lvar->chr, lvar_name, lvar_offset, tgt.num);

    UPDATE_PC();
}

void addarg_c_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void addarg_d(const instr *i)
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .val = lvar->val += pop().val };

    push(tgt);

    P_TAIL(": %.8lg -> "GREEN"%s"ANSI_END"<%+d> -> %.8lg",
            lvar->val, lvar_name, lvar_offset, tgt.val);

    UPDATE_PC();
}

void addarg_d_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void addarg_f(const instr *i)
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .val = lvar->flt += pop().val };

    push(tgt);

    P_TAIL(": %.7g -> "GREEN"%s"ANSI_END"<%+d> -> %.15lg",
            lvar->flt, lvar_name, lvar_offset, tgt.val);

    UPDATE_PC();
}

void addarg_f_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void addarg_i(const instr *i)
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .num = lvar->inum += pop().num };

    push(tgt);

    P_TAIL(": %i -> "GREEN"%s"ANSI_END"<%+d> -> %li",
            lvar->inum, lvar_name, lvar_offset, tgt.num);

    UPDATE_PC();
}

void addarg_i_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void addarg_l(const instr *i)
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .num = lvar->num += pop().num };

    push(tgt);

    P_TAIL(": %li -> "GREEN"%s"ANSI_END"<%+d> -> %li",
            lvar->num, lvar_name, lvar_offset, top().num);

    UPDATE_PC();
}

void addarg_l_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void addarg_s(const instr *i)
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .num = lvar->sht += pop().num };

    push(tgt);

    P_TAIL(": 0x%04hx -> "GREEN"%s"ANSI_END"<%+d> -> %li",
            lvar->sht, lvar_name, lvar_offset, tgt.num);

    UPDATE_PC();
}

void addarg_s_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void subarg(const instr *i) /* assign top value to next value */
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .val = lvar->val -= pop().val };

    push(tgt);

    P_TAIL(": %.8lg -> "GREEN"%s"ANSI_END"<%+d> -> %.15lg",
            lvar->val, lvar_name, lvar_offset, tgt.val);

    UPDATE_PC();
}

void subarg_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].args);
}

void subarg_c(const instr *i) /* assign top value to next value */
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .num = lvar->chr -= pop().num };

    push(tgt);

    P_TAIL(": 0x%02hhx -> "GREEN"%s"ANSI_END"<%+d> -> %li",
            lvar->chr, lvar_name, lvar_offset, tgt.num);

    UPDATE_PC();
}

void subarg_c_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void subarg_d(const instr *i)
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .val = lvar->val -= pop().val };

    push(tgt);

    P_TAIL(": %.8lg -> "GREEN"%s"ANSI_END"<%+d> -> %.8lg",
            lvar->val, lvar_name, lvar_offset, tgt.val);

    UPDATE_PC();
}

void subarg_d_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void subarg_f(const instr *i)
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .val = lvar->val -= pop().val };

    lvar->flt -= pop().val;

    Cell        result      = { .val = lvar->flt };

    push(result);

    P_TAIL(": %.7g -> "GREEN"%s"ANSI_END"<%+d> -> %.15lg",
            lvar->flt, lvar_name, lvar_offset, top().val);

    UPDATE_PC();
}

void subarg_f_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void subarg_i(const instr *i)
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .num = lvar->inum -= pop().num };

    push(tgt);

    P_TAIL(": %i -> "GREEN"%s"ANSI_END"<%+d> -> %li",
            lvar->inum, lvar_name, lvar_offset, tgt.num);

    UPDATE_PC();
}

void subarg_i_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void subarg_l(const instr *i)
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .num = lvar->num -= pop().num };

    push(tgt);

    P_TAIL(": %li -> "GREEN"%s"ANSI_END"<%+d> -> %li",
            lvar->num, lvar_name, lvar_offset, tgt.num);

    UPDATE_PC();
}

void subarg_l_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void subarg_s(const instr *i)
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .num = lvar->sht -= pop().num };

    push(tgt);

    P_TAIL(": 0x%04hx -> "GREEN"%s"ANSI_END"<%+d> -> %li",
            lvar->sht, lvar_name, lvar_offset, tgt.num);

    UPDATE_PC();
}

void subarg_s_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void mularg(const instr *i) /* assign top value to next value */
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .val = lvar->val *= pop().val };

    push(tgt);

    P_TAIL(": %.8lg -> "GREEN"%s"ANSI_END"<%+d> -> %.8lg",
            lvar->val, lvar_name, lvar_offset, tgt.val);

    UPDATE_PC();
}

void mularg_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].args);
}

void mularg_c(const instr *i) /* assign top value to next value */
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .num = lvar->chr *= pop().num };

    push(tgt);

    P_TAIL(": 0x%02hhx -> "GREEN"%s"ANSI_END"<%+d> -> %li",
            lvar->chr, lvar_name, lvar_offset, tgt.num);

    UPDATE_PC();
}

void mularg_c_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void mularg_d(const instr *i)
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .val = lvar->val *= pop().val };

    push(tgt);

    P_TAIL(": %.8lg -> "GREEN"%s"ANSI_END"<%+d> -> %.8lg",
            lvar->val, lvar_name, lvar_offset, tgt.val);

    UPDATE_PC();
}

void mularg_d_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void mularg_f(const instr *i)
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .val = lvar->flt *= pop().val };

    push(tgt);

    P_TAIL(": %.7g -> "GREEN"%s"ANSI_END"<%+d> -> %.15lg",
            lvar->flt, lvar_name, lvar_offset, tgt.val);

    UPDATE_PC();
}

void mularg_f_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void mularg_i(const instr *i)
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .num = lvar->inum *= pop().num };

    push(tgt);

    P_TAIL(": %i -> "GREEN"%s"ANSI_END"<%+d> -> %li",
            lvar->inum, lvar_name, lvar_offset, tgt.num);

    UPDATE_PC();
}

void mularg_i_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void mularg_l(const instr *i)
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .num = lvar->num *= pop().num };

    push(tgt);

    P_TAIL(": %li -> "GREEN"%s"ANSI_END"<%+d> -> %li",
            lvar->num, lvar_name, lvar_offset, tgt.num);

    UPDATE_PC();
}

void mularg_l_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void mularg_s(const instr *i)
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .num = lvar->sht *= pop().num };

    push(tgt);

    P_TAIL(": 0x%04hx -> "GREEN"%s"ANSI_END"<%+d> -> %li",
            lvar->sht, lvar_name, lvar_offset, tgt.num);

    UPDATE_PC();
}

void mularg_s_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void divarg(const instr *i) /* assign top value to next value */
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .val = lvar->val /= pop().val };

    push(tgt);

    P_TAIL(": %.8lg -> "GREEN"%s"ANSI_END"<%+d> -> %.8lg",
            lvar->val, lvar_name, lvar_offset, tgt.val);

    UPDATE_PC();
}

void divarg_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].args);
}

void divarg_c(const instr *i) /* assign top value to next value */
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .num = lvar->chr /= pop().num };

    push(tgt);

    P_TAIL(": 0x%02hhx -> "GREEN"%s"ANSI_END"<%+d> -> %li",
            lvar->chr, lvar_name, lvar_offset, tgt.num);

    UPDATE_PC();
}

void divarg_c_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void divarg_d(const instr *i)
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .val = lvar->val /= pop().val };

    push(tgt);

    P_TAIL(": %.8lg -> "GREEN"%s"ANSI_END"<%+d> -> %.8lg",
            lvar->val, lvar_name, lvar_offset, tgt.val);

    UPDATE_PC();
}

void divarg_d_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void divarg_f(const instr *i)
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .val = lvar->flt /= pop().val };

    push(tgt);

    P_TAIL(": %.7g -> "GREEN"%s"ANSI_END"<%+d> -> %.15lg",
            lvar->flt, lvar_name, lvar_offset, tgt.val);

    UPDATE_PC();
}

void divarg_f_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void divarg_i(const instr *i)
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .num = lvar->inum /= pop().num };

    push(tgt);

    P_TAIL(": %i -> "GREEN"%s"ANSI_END"<%+d> -> %li",
            lvar->inum, lvar_name, lvar_offset, tgt.num);

    UPDATE_PC();
}

void divarg_i_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void divarg_l(const instr *i)
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .num = lvar->num /= pop().num };

    push(tgt);

    P_TAIL(": %li -> "GREEN"%s"ANSI_END"<%+d> -> %li",
            lvar->num, lvar_name, lvar_offset, tgt.num);

    UPDATE_PC();
}

void divarg_l_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void divarg_s(const instr *i)
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .num = lvar->sht /= pop().num };

    push(tgt);

    P_TAIL(": 0x%04hx -> "GREEN"%s"ANSI_END"<%+d> -> %li",
            lvar->sht, lvar_name, lvar_offset, tgt.num);

    UPDATE_PC();
}

void divarg_s_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void modarg(const instr *i) /* assign top value to next value */
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .val = lvar->val = fmod(lvar->val, pop().val) };

    push(tgt);

    P_TAIL(": %.8lg -> "GREEN"%s"ANSI_END"<%+d> -> %.15lg",
            lvar->val, lvar_name, lvar_offset, tgt.val);

    UPDATE_PC();
}

void modarg_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].args);
}

void modarg_c(const instr *i) /* assign top value to next value */
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .num = lvar->chr %= pop().num };

    push(tgt);

    P_TAIL(": 0x%02hhx -> "GREEN"%s"ANSI_END"<%+d> -> %li",
            lvar->chr, lvar_name, lvar_offset, tgt.num);

    UPDATE_PC();
}

void modarg_c_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void modarg_d(const instr *i)
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .val = lvar->val = fmod(lvar->val, pop().val) };

    push(tgt);

    P_TAIL(": %.15lg -> "GREEN"%s"ANSI_END"<%+d> -> %.15lg",
            lvar->val, lvar_name, lvar_offset, tgt.val);

    UPDATE_PC();
}

void modarg_d_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void modarg_f(const instr *i)
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .val = lvar->flt = fmod(lvar->flt, pop().val) };

    push(tgt);

    P_TAIL(": %.8lg -> "GREEN"%s"ANSI_END"<%+d> -> %.8lg",
            lvar->val, lvar_name, lvar_offset, tgt.val);

    UPDATE_PC();
}

void modarg_f_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void modarg_i(const instr *i)
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .num = lvar->inum %= pop().num };

    push(tgt);

    P_TAIL(": %i -> "GREEN"%s"ANSI_END"<%+d> -> %li",
            lvar->inum, lvar_name, lvar_offset, tgt.num);

    UPDATE_PC();
}

void modarg_i_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void modarg_l(const instr *i)
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .num = lvar->num %= pop().num };

    push(tgt);

    P_TAIL(": %li -> "GREEN"%s"ANSI_END"<%+d> -> %li",
            lvar->num, lvar_name, lvar_offset, tgt.num);

    UPDATE_PC();
}

void modarg_l_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void modarg_s(const instr *i)
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .num = lvar->sht %= pop().num };

    push(tgt);

    P_TAIL(": 0x%04hx -> "GREEN"%s"ANSI_END"<%+d> -> %li",
            lvar->sht, lvar_name, lvar_offset, tgt.num);

    UPDATE_PC();
}

void modarg_s_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void pwrarg(const instr *i) /* assign top value to next value */
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .val = lvar->val = pow(lvar->val, pop().val) };

    push(tgt);

    P_TAIL(": %.8lg -> "GREEN"%s"ANSI_END"<%+d> -> %.8lg",
            lvar->val, lvar_name, lvar_offset, tgt.val);

    UPDATE_PC();
}

void pwrarg_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].args);
}

void pwrarg_c(const instr *i) /* assign top value to next value */
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .num = lvar->chr = fast_pwr_i(lvar->chr, pop().num) };

    push(tgt);

    P_TAIL(": 0x%02hhx -> "GREEN"%s"ANSI_END"<%+d> -> %li",
            lvar->chr, lvar_name, lvar_offset, tgt.num);

    UPDATE_PC();
}

void pwrarg_c_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void pwrarg_d(const instr *i)
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .val = lvar->val = pow(lvar->val, pop().val) };

    push(tgt);

    P_TAIL(": %.15lg -> "GREEN"%s"ANSI_END"<%+d> -> %.15lg",
            lvar->val, lvar_name, lvar_offset, tgt.val);

    UPDATE_PC();
}

void pwrarg_d_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void pwrarg_f(const instr *i)
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .val = lvar->flt = pow(lvar->flt, pop().val) };

    push(tgt);

    P_TAIL(": %.8lg -> "GREEN"%s"ANSI_END"<%+d> -> %.8lg",
            lvar->val, lvar_name, lvar_offset, tgt.val);

    UPDATE_PC();
}

void pwrarg_f_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void pwrarg_i(const instr *i)
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .num = lvar->inum = fast_pwr_i(lvar->inum, pop().num) };

    push(tgt);

    P_TAIL(": %i -> "GREEN"%s"ANSI_END"<%+d> -> %li",
            lvar->inum, lvar_name, lvar_offset, tgt.num);

    UPDATE_PC();
}

void pwrarg_i_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void pwrarg_l(const instr *i)
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .num = lvar->num = fast_pwr_i(lvar->num, pop().num) };

    push(tgt);

    P_TAIL(": %li -> "GREEN"%s"ANSI_END"<%+d> -> %li",
            lvar->num, lvar_name, lvar_offset, tgt.num);

    UPDATE_PC();
}

void pwrarg_l_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void pwrarg_s(const instr *i)
{
    int         lvar_offset = pc[0].args;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .num = lvar->sht = fast_pwr_i(lvar->sht, pop().num) };

    push(tgt);

    P_TAIL(": 0x%04hx -> "GREEN"%s"ANSI_END"<%+d> -> %li",
            lvar->sht, lvar_name, lvar_offset, tgt.num);

    UPDATE_PC();
}

void pwrarg_s_prt(const instr *i, const Cell *pc)
{
    PR("\n");
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

void d2f(const instr *i) /* cast double to float */
{
    UPDATE_PC();
}

void d2f_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void d2i(const instr *i) /* cast double to int */
{
    UPDATE_PC();
}

void d2i_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void d2l(const instr *i) /* cast double to long */
{
    UPDATE_PC();
}

void d2l_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void f2d(const instr *i) /* cast float to double */
{
    UPDATE_PC();
}

void f2d_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void f2i(const instr *i) /* cast float to int */
{
    UPDATE_PC();
}

void f2i_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void f2l(const instr *i) /* cast float to long */
{
    UPDATE_PC();
}

void f2l_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}


void i2d(const instr *i) /* cast int to double */
{
    UPDATE_PC();
}

void i2d_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void i2f(const instr *i) /* cast int to float */
{
    UPDATE_PC();
}

void i2f_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void i2l(const instr *i) /* cast int to long */
{
    UPDATE_PC();
}

void i2l_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void l2d(const instr *i) /* cast long to double */
{
    UPDATE_PC();
}

void l2d_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void l2f(const instr *i) /* cast long to float */
{
    UPDATE_PC();
}

void l2f_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void l2i(const instr *i) /* cast long to int */
{
    UPDATE_PC();
}

void l2i_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}
