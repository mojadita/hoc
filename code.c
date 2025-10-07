/* code.c -- instrucciones y rutinas de traza.
 * Author: Edward Rivas <rivastkw@gmail.com>
 *       y Luis Colorado <luiscoloradourcola@gmail.com>
 * Date: Sat Mar 22 14:20:43 -05 2025
 * Copyright: (c) 2025 Edward Rivas y Luis Colorado.  All rights reserved.
 * License: BSD
 */

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>

#include "config.h"
#include "colors.h"

#include "cellP.h"
#include "symbolP.h"
#include "code.h"
#include "hoc.h"
#include "math.h"
#include "types.h"

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
        "<" CYAN "%02x" WHITE "> "  \
        CYAN"%-14s"ANSI_END _fmt,   \
        pc - prog,                  \
        i->code_id,                 \
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
        const char   *name,
        const Symbol *typref)
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
        const char   *name,
        const Symbol *typref)
{
    scope *scop = get_current_scope();
    assert(scop != NULL);
    if (lookup_current_scope(name)) {
        execerror("Variable " GREEN "%s" ANSI_END
            " already defined in current scope\n", name);
    }
    Symbol *sym = install(name, LVAR, typref);
    scop->size += typref->t2i->size;
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

    PRG("[%04lx]: <%02x> %s",
            progp - prog, i->code_id, i->name);

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
        EXEC("[%04lx]: <%02x> %s", pc - prog, ins->code_id, ins->name);
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

void dupl(const instr *i) /* duplicate cell */
{
    push(top());
    UPDATE_PC();
}

void dupl_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void swap(const instr *i) /* swap cell */
{
    Cell aux1 = pop(),
         aux2 = pop();

    push(aux1);
    push(aux2);

    UPDATE_PC();
}

void swap_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

/* ver instancias de esta macro mas abajo para saber el tipo
 * de funcion que generan, ejemplo:
 * void datum_c_prog(
 *      const instr    *i
 *      Cell           *pc,
 *      va_list         args)
 * {
 *  Cell c = { .chr = va_arg(args, char) };
 *
 *  progp[1] = c;
 *
 *  PRG(" { .chr = " FMT_CHAR " }", c.chr);
 * }
 *
 */
#define DATUM_PROG(_typ, _suff, _fld, _fmt)        \
    void datum##_suff##_prog(                      \
            const instr    *i,                     \
            Cell           *pc,                    \
            va_list         args)                  \
    {                                              \
                                                   \
        progp[1] = va_arg(args, Cell);             \
                                                   \
        PRG(" " _fmt, c._fld);                     \
    }

DATUM_PROG(int,    _c, chr,  FMT_CHAR)
DATUM_PROG(double, _d, val,  FMT_DOUBLE)
DATUM_PROG(double, _f, flt,  FMT_FLOAT)
DATUM_PROG(int,    _i, inum, FMT_INT)
DATUM_PROG(long,   _l, num,  FMT_LONG)
DATUM_PROG(int,    _s, sht,  FMT_SHORT)

#undef DATUM_PROG

#define CONSTPUSH(_suff, _fld, _fmt)  /* { */ \
        void constpush##_suff(const instr *i) \
        {                                     \
            Cell d = pc[1];                   \
                                              \
            push(d);                          \
                                              \
            P_TAIL(": -> " _fmt, d._fld);     \
                                              \
            UPDATE_PC();                      \
        } /* constpush##_suff */              \
                                              \
        void constpush##_suff##_prt(          \
                const instr *i,               \
                const Cell *pc)               \
        {                                     \
            PR(" " _fmt "\n", pc[1]._fld);    \
        } /* constpush##_suff##_prt      }{ */

CONSTPUSH(_c, chr,  FMT_CHAR)   /* push constant onto stack */
CONSTPUSH(_d, val,  FMT_DOUBLE)
CONSTPUSH(_f, flt,  FMT_FLOAT)
CONSTPUSH(_i, inum, FMT_INT)
CONSTPUSH(_l, num,  FMT_LONG)
CONSTPUSH(_s, sht,  FMT_SHORT)

#undef CONSTPUSH /*                      } */

#define OP(_nam, _suff, _fld, _op, _fmt) /* { */    \
    void _nam##_suff(const instr *i)                \
    {                                               \
        Cell p2  = pop(),                           \
             p1  = pop(),                           \
             res = { ._fld = p1._fld _op p2._fld }; \
                                                    \
        P_TAIL(": " _fmt " %s " _fmt " -> " _fmt,   \
                p1._fld, #_op, p2._fld, res._fld);  \
        push(res);                                  \
                                                    \
        UPDATE_PC();                                \
    } /* _nam##_suff */                             \
                                                    \
    void _nam##_suff##_prt(                         \
            const instr    *i,                      \
            const Cell     *pc)                     \
    {                                               \
        PR("\n");                                   \
    } /* _nam##_suff##_prt         }{ */

OP(add, _c, chr,  +, FMT_CHAR) /* add top two elements on stack */
OP(add, _d, val,  +, FMT_DOUBLE)
OP(add, _f, flt,  +, FMT_FLOAT)
OP(add, _i, inum, +, FMT_INT)
OP(add, _l, num,  +, FMT_LONG)
OP(add, _s, sht,  +, FMT_SHORT)

OP(sub, _c, chr,  -, FMT_CHAR)
OP(sub, _d, val,  -, FMT_DOUBLE)
OP(sub, _f, flt,  -, FMT_FLOAT)
OP(sub, _i, inum, -, FMT_INT)
OP(sub, _l, num,  -, FMT_LONG)
OP(sub, _s, chr,  -, FMT_SHORT)

OP(mul, _c, chr,  *, FMT_CHAR)
OP(mul, _d, val,  *, FMT_DOUBLE)
OP(mul, _f, flt,  *, FMT_FLOAT)
OP(mul, _i, inum, *, FMT_INT)
OP(mul, _l, num,  *, FMT_LONG)
OP(mul, _s, chr,  *, FMT_SHORT)

OP(divi, _c, chr,  /, FMT_CHAR)
OP(divi, _d, val,  /, FMT_DOUBLE)
OP(divi, _f, flt,  /, FMT_FLOAT)
OP(divi, _i, inum, /, FMT_INT)
OP(divi, _l, num,  /, FMT_LONG)
OP(divi, _s, chr,  /, FMT_SHORT)

OP(mod, _c, chr,  %, FMT_CHAR) /* multiply two elements on stack (only integers) */
OP(mod, _l, num,  %, FMT_LONG)
OP(mod, _i, inum, %, FMT_INT)
OP(mod, _s, chr,  %, FMT_SHORT)

#undef OP  /* } */

#define RELOP(_nam, _suff, _fld, _op, _fmt) /* { */ \
    void _nam##_suff(const instr *i)                \
    {                                               \
        Cell p2  = pop(),                           \
             p1  = pop(),                           \
             res = { .inum = p1._fld _op p2._fld }; \
                                                    \
        P_TAIL(": " _fmt " %s " _fmt " -> " _fmt,   \
                p1._fld, #_op, p2._fld, res._fld);  \
        push(res);                                  \
                                                    \
        UPDATE_PC();                                \
    } /* add##_suff */                              \
                                                    \
    void _nam##_suff##_prt(                         \
            const instr    *i,                      \
            const Cell     *pc)                     \
    {                                               \
        PR("\n");                                   \
    } /* _nam##_suff##_prt         }{ */

RELOP(ge, _c, chr,  >=,  FMT_CHAR)
RELOP(ge, _d, val,  >=,  FMT_DOUBLE)
RELOP(ge, _f, flt,  >=,  FMT_FLOAT)
RELOP(ge, _i, inum, >=,  FMT_INT)
RELOP(ge, _l, num,  >=,  FMT_LONG)
RELOP(ge, _s, sht,  >=,  FMT_SHORT)

RELOP(le, _c, chr,  <=,  FMT_CHAR)
RELOP(le, _d, val,  <=,  FMT_DOUBLE)
RELOP(le, _f, flt,  <=,  FMT_FLOAT)
RELOP(le, _i, inum, <=,  FMT_INT)
RELOP(le, _l, num,  <=,  FMT_LONG)
RELOP(le, _s, sht,  <=,  FMT_SHORT)

RELOP(gt, _c, chr,  >,  FMT_CHAR)
RELOP(gt, _d, val,  >,  FMT_DOUBLE)
RELOP(gt, _f, flt,  >,  FMT_FLOAT)
RELOP(gt, _i, inum, >,  FMT_INT)
RELOP(gt, _l, num,  >,  FMT_LONG)
RELOP(gt, _s, sht,  >,  FMT_SHORT)

RELOP(lt, _c, chr,  <,  FMT_CHAR)
RELOP(lt, _d, val,  <,  FMT_DOUBLE)
RELOP(lt, _f, flt,  <,  FMT_FLOAT)
RELOP(lt, _i, inum, <,  FMT_INT)
RELOP(lt, _l, num,  <,  FMT_LONG)
RELOP(lt, _s, sht,  <,  FMT_SHORT)

RELOP(eq, _c, chr,  ==,  FMT_CHAR)
RELOP(eq, _d, val,  ==,  FMT_DOUBLE)
RELOP(eq, _f, flt,  ==,  FMT_FLOAT)
RELOP(eq, _i, inum, ==,  FMT_INT)
RELOP(eq, _l, num,  ==,  FMT_LONG)
RELOP(eq, _s, sht,  ==,  FMT_SHORT)

RELOP(ne, _c, chr,  !=,  FMT_CHAR)
RELOP(ne, _d, val,  !=,  FMT_DOUBLE)
RELOP(ne, _f, flt,  !=,  FMT_FLOAT)
RELOP(ne, _i, inum, !=,  FMT_INT)
RELOP(ne, _l, num,  !=,  FMT_LONG)
RELOP(ne, _s, sht,  !=,  FMT_SHORT)

#undef OP  /* } */

#define MOD(_suff, _fld, _fmt) /* { */                 \
    void mod##_suff(const instr *i)                    \
    {                                                  \
        Cell p2  = pop(),                              \
             p1  = pop(),                              \
             res = { ._fld = fmod(p1._fld, p2._fld) }; \
                                                       \
        P_TAIL(": " _fmt " %% " _fmt " -> " _fmt,      \
                p1._fld, p2._fld, res._fld);           \
                                                       \
        push(res);                                     \
                                                       \
        UPDATE_PC();                                   \
    } /* mod##_suff */                                 \
                                                       \
    void mod##_suff##_prt(                             \
            const instr *i,                            \
            const Cell  *pc)                           \
    {                                                  \
        PR("\n");                                      \
    } /* mod##_suff##_prt         }{ */

MOD(_d, val, FMT_DOUBLE) /* mod top two elements on stack */
MOD(_f, val, FMT_FLOAT)

#undef MOD /*                     } */

#define UNARY_LOP(_name, _suff, _fld, _res, _op, _fmt) /* { */    \
    void _name##_suff(const instr *i)       \
    {                                     \
        Cell d   = pop(),                 \
             res = { ._res = _op d._fld };\
                                          \
        P_TAIL(": " #_op " " _fmt " -> " _fmt,   \
                d._fld, res._fld);        \
        push(res);                        \
                                          \
        UPDATE_PC();                      \
    } /* _name##_suff */                    \
                                          \
    void _name##_suff##_prt(const instr *i, \
            const Cell *pc)               \
    {                                     \
        PR("\n");                         \
    } /* _name##_suff##_prt         }{ */

UNARY_LOP(neg, _c, chr,  chr,  -, FMT_CHAR) /* change sign top element on stack */
UNARY_LOP(neg, _d, val,  val,  -, FMT_DOUBLE)
UNARY_LOP(neg, _f, flt,  flt,  -, FMT_FLOAT)
UNARY_LOP(neg, _i, inum, inum, -, FMT_INT)
UNARY_LOP(neg, _l, num,  num,  -, FMT_LONG)
UNARY_LOP(neg, _s, sht,  sht,  -, FMT_SHORT)

UNARY_LOP(not,,    inum, inum, !, FMT_INT)

#undef NEG /*                     } */

#define PWR(_suff, _fld, _fn, _fmt)  /* { */        \
    void pwr##_suff(const instr *i)                 \
    {                                               \
        Cell e  = pop(),                            \
             b  = pop(),                            \
             res = { ._fld = _fn(b._fld, e._fld) }; \
                                                    \
        P_TAIL(": b=" _fmt ", e=" _fmt " -> " _fmt, \
                b._fld, e._fld, res._fld);          \
        push(res);                                  \
                                                    \
        UPDATE_PC();                                \
    } /* pwr##_suff */                              \
                                                    \
    void pwr##_suff##_prt(                          \
            const instr *i,                         \
            const Cell  *pc)                        \
    {                                               \
        PR("\n");                                   \
    } /* pwr##_suff##_prt               }{ */

PWR(_d, val,  pow,        FMT_DOUBLE)
PWR(_f, flt,  pow,        FMT_FLOAT)
PWR(_c, chr,  fast_pwr_l, FMT_CHAR)   /* pow top two elements on stack */
PWR(_i, inum, fast_pwr_l, FMT_INT)
PWR(_l, num,  fast_pwr_l, FMT_LONG)
PWR(_s, sht,  fast_pwr_l, FMT_SHORT)

#undef PWR /*                           } */

void symb_prog(const instr *i, Cell *pc, va_list args)
{
    Symbol *sym = va_arg(args, Symbol *);

    pc[0].param  = sym->defn - prog;
    pc[1].sym   = sym;

    PRG(" "GREEN"%s"ANSI_END"[%04x]",
        sym->name, pc[0].param);
}

#define EVAL(_suff, _fld, _fmt) /* { */                \
    void eval##_suff(const instr *i)                   \
    {                                                  \
        int     var_addr = pc[0].param;                \
        Symbol *sym      = pc[1].sym;                  \
        Cell   *var      = prog + var_addr;            \
        Cell    tgt      = { ._fld = var->_fld };      \
                                                       \
        push(tgt);                                     \
                                                       \
        P_TAIL(": "GREEN"%s"ANSI_END"[%04x] -> " _fmt, \
            sym->name, var_addr, tgt._fld);            \
                                                       \
        UPDATE_PC();                                   \
    } /* eval##_suff */                                \
                                                       \
    void eval##_suff##_prt(                            \
            const instr *i,                            \
            const Cell  *pc)                           \
    {                                                  \
        PR(GREEN"%s"ANSI_END"[%04x]\n",                \
            pc[1].sym->name, pc[0].param);             \
    } /* eval##_suff##_prt               }{ */

EVAL(_c, chr,  FMT_CHAR)   /* evaluates a global variable */
EVAL(_d, val,  FMT_DOUBLE)
EVAL(_f, val,  FMT_FLOAT)
EVAL(_i, inum, FMT_INT)
EVAL(_l, num,  FMT_LONG)
EVAL(_s, sht,  FMT_SHORT)

#undef EVAL /*                           } */

#define ARGEVAL(_suff, _fld, _fmt) /* { */ \
    void argeval##_suff(const instr *i)  \
    {                                    \
        int         arg = pc[0].param;   \
        const char *nam = pc[1].str;     \
        Cell        d   = *getarg(arg);  \
                                         \
        P_TAIL(": " GREEN "%s" ANSI_END  \
            "<%+d> -> " _fmt,            \
            nam, arg, d._fld);           \
                                         \
        push(d);                         \
                                         \
        UPDATE_PC();                     \
    } /* argeval##_suff */               \
                                         \
    void argeval##_suff##_prt(           \
            const instr *i,              \
            const Cell  *pc)             \
    {                                    \
        PR(GREEN"%s"ANSI_END"<%+d>\n",   \
            pc[1].str, pc[0].param);     \
    } /* argeval##_suff##_prt         }{ */

ARGEVAL(_c, chr, FMT_CHAR)   /* push local var onto stack */
ARGEVAL(_d, val, FMT_DOUBLE)
ARGEVAL(_f, flt, FMT_FLOAT)
ARGEVAL(_i, inum, FMT_INT)
ARGEVAL(_l, num, FMT_LONG)
ARGEVAL(_s, sht, FMT_SHORT)

#undef EVAL /*                        } */

#define ASSIGN(_suff, _fld, _fmt) /* { */        \
    void assign##_suff(const instr *i)           \
    {                                            \
        int     gvar_addr = pc[0].param;         \
        Symbol *sym       = pc[1].sym;           \
        Cell   *var       = prog + gvar_addr;    \
        Cell    src       = top();               \
                                                 \
        *var = src;                              \
                                                 \
        P_TAIL(": " _fmt " -> "                  \
                GREEN "%s" ANSI_END "[%04x]",    \
                src._fld, sym->name, gvar_addr); \
                                                 \
        UPDATE_PC();                             \
    } /* assign##_suff */                        \
                                                 \
    void assign##_suff##_prt(                    \
            const instr *i,                      \
            const Cell  *pc)                     \
    {                                            \
        PR(GREEN "%s" ANSI_END "[%04x]\n",       \
            pc[1].sym->name, pc[0].param);       \
    } /* assign##_suff##_prt         }{ */

ASSIGN(_c, chr,  FMT_CHAR)      /* assign top value to next value */
ASSIGN(_d, val,  FMT_DOUBLE)
ASSIGN(_f, flt,  FMT_FLOAT)
ASSIGN(_i, inum, FMT_INT)
ASSIGN(_l, num,  FMT_LONG)
ASSIGN(_s, sht,  FMT_SHORT)

#undef ASSIGN /*                     } */


void arg_str_prog(const instr *i, Cell *pc, va_list args)
{
    pc[0].param = va_arg(args, int);
    pc[1].str  = va_arg(args, char *);
    PRG(" "GREEN"%s"ANSI_END"<%+d>", pc[1].str, pc[0].param);
}

#define ARGASSIGN(_suff, _fld, _fmt) /* { */ \
    void argassign##_suff(const instr *i)    \
    {                                        \
        int  lvar_off = pc[0].param;         \
        const char                           \
            *name     = pc[1].str;           \
        Cell                                 \
            *var      = getarg(lvar_off),    \
             src      = top();               \
                                             \
        *var = src;                          \
                                             \
        P_TAIL(": " _fmt " -> "              \
                GREEN "%s" ANSI_END "<%+d>", \
                src._fld, name, lvar_off);   \
                                             \
        UPDATE_PC();                         \
    } /* argassign##_suff */                 \
                                             \
    void argassign##_suff##_prt(             \
            const instr *i,                  \
            const Cell  *pc)                 \
    {                                        \
        PR(GREEN "%s" ANSI_END "<%+d>\n",    \
            pc[1].str, pc[0].param);         \
    } /* argassign##_suff##_prt         }{ */

ARGASSIGN(_c, chr,  FMT_CHAR)   /* store top of stack in local var */
ARGASSIGN(_d, val,  FMT_DOUBLE)
ARGASSIGN(_f, flt,  FMT_FLOAT)
ARGASSIGN(_i, inum, FMT_INT)
ARGASSIGN(_l, num,  FMT_LONG)
ARGASSIGN(_s, sht,  FMT_SHORT)

#undef ARGASSIGN /*                     } */

#define PRINT_INST(_suff, _fld, _fmt) /* { */     \
    void print##_suff(const instr *i)             \
    {                                             \
        Cell d = pop();                           \
                                                  \
        printf("\t\t\t\t\t\t" _fmt "\n", d._fld); \
                                                  \
        UPDATE_PC();                              \
    } /* print##_suff */                          \
                                                  \
    void print##_suff##_prt(                      \
            const instr *i,                       \
            const Cell  *pc)                      \
    {                                             \
        PR("\n");                                 \
    } /* print##_suff##_prt         }{ */

PRINT_INST(_c, chr,  FMT_CHAR)   /* pop top value from stack, print it */
PRINT_INST(_d, val,  FMT_DOUBLE)
PRINT_INST(_f, flt,  FMT_FLOAT)
PRINT_INST(_i, inum, FMT_INT)
PRINT_INST(_l, num,  FMT_LONG)
PRINT_INST(_s, sht,  FMT_SHORT)

#undef PRINT /*                     } */

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

#define AND_THEN_OR_ELSE(_name, _suff, _fld, _op, _ops, _fmt) /* { */\
    void _name##_suff(const instr *i)            \
    {                                            \
        Cell        d  = top();                  \
        const char *op = d._fld ? " drop;" : ""; \
                                                 \
        if (_op d._fld) {                        \
            pop();                               \
            UPDATE_PC();                         \
        } else {                                 \
            pc = prog + pc[0].param;             \
        }                                        \
                                                 \
        P_TAIL(": " _fmt " " #_ops               \
            " %s -> [%04lx]",                    \
            d._fld, op, pc - prog);              \
    } /* _name##_suff */                         \
                                                 \
    void _name##_suff##_prt(                     \
            const instr *i,                      \
            const Cell  *pc)                     \
    {                                            \
        PR("[%04x]\n", pc[0].param);             \
    } /* _name##_suff##_prt                                      }{*/

AND_THEN_OR_ELSE(and_then,, inum, , "&&", FMT_INT)
AND_THEN_OR_ELSE(or_else,,  val, !, "||", FMT_DOUBLE)

#undef AND_THEN_OR_ELSE /*                                       } */

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
void end_define(const Symbol *subr)
{
    /* adjust progbase to point to the code starting point */
    progbase = progp;     /* next code starts here */
    P_TAIL("end_define(%s);\n", subr->name);
}

void call(const instr *i)   /* call a function */
{
    Symbol *sym = pc[1].sym;

    P_TAIL(": "GREEN"%s"ANSI_END"[%04x] -> ret_addr=[%04lx]",
        sym->name, pc[0].param, pc + i->n_cells - prog);

    Cell ret_addr = { .cel = pc + i->n_cells };

    push(ret_addr);

    pc = prog + pc[0].param;
} /* call */

void call_prt(const instr *i, const Cell *pc)
{
    PR(GREEN"%s"ANSI_END"[%04x], args=%ld\n",
        pc[1].sym->name,
        pc[0].param,
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
    pc[0].param = va_arg(args, int);
    PRG(" <%+d>", pc[0].param);
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

#define PREXPR(_suffix, _fld, _fmt)  \
void prexpr##_suffix(const instr *i) \
{                                    \
    P_TAIL("\n");                    \
    printf(_fmt, pop()._fld);        \
                                     \
    UPDATE_PC();                     \
} /* prexpr##_suffix */              \
                                     \
void prexpr##_suffix##_prt(          \
        const instr *i,              \
        const Cell  *pc)             \
{                                    \
    PR("\n");                        \
} /* prexpr##_suffix##_prt */

PREXPR(_c, chr,  FMT_CHAR)   /* print numeric value */
PREXPR(_d, val,  FMT_DOUBLE)
PREXPR(_f, flt,  FMT_FLOAT)
PREXPR(_i, inum, FMT_INT)
PREXPR(_l, num,  FMT_LONG)
PREXPR(_s, sht,  FMT_SHORT)

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

    pc = pop().inum
        ? pc + i->n_cells
        : prog + pc[0].param;

    P_TAIL(": -> [%04x]", pc[0].param);
}

void if_f_goto_prt(const instr *i, const Cell *pc)
{
    PR("[%04x]\n", pc[0].param);
}

void addr_prog(const instr *i, Cell *pc, va_list args)
{
    progp[0].param = va_arg(args, Cell *) - prog;

    PRG(" [%04x]", progp[0].param);
}

void Goto(const instr *i) /* jump if false */
{
    P_TAIL(": -> [%04x]", pc[0].param);
    pc = prog + pc[0].param;
}

void Goto_prt(const instr *i, const Cell *pc)
{
    PR("[%04x]\n", pc[0].param);
}

void noop(const instr *i)
{
    UPDATE_PC();
}

void noop_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void spadd(const instr *i) /* pop n elementos de la pila */
{
    int n = pc[0].param;
    P_TAIL(": %+d", n);
    sp += n;

    UPDATE_PC();
}

void spadd_prt(const instr *i, const Cell *pc)
{
    PR("%+d\n", pc[0].param);
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

#define CHG_TYPE(_name, _from, _fmt_f, _to, _fmt_t) \
    void _name(const instr *i)                \
    {                                         \
        Cell data = pop();                    \
                                              \
        P_TAIL(": " _fmt_f, data._from);      \
                                              \
        /* now convert from field (and type)  \
         * '_from' to field (and type) '_to'. \
         */                                   \
        data._to  = data._from;               \
                                              \
        P_TAIL(" -> " _fmt_t, data._to);      \
                                              \
        push(data);                           \
                                              \
        UPDATE_PC();                          \
    } /* _name */                             \
    void _name##_prt(                         \
            const instr *i,                   \
            const Cell  *pc)                  \
    {                                         \
        PR("\n");                             \
    } /* _name##_prt */

CHG_TYPE(c2d, chr,  FMT_CHAR,   val,  FMT_DOUBLE) /* cast char to double */
CHG_TYPE(c2f, chr,  FMT_CHAR,   flt,  FMT_FLOAT)  /* cast char to float */
CHG_TYPE(c2i, chr,  FMT_CHAR,   inum, FMT_INT)    /* cast char to int */
CHG_TYPE(c2l, chr,  FMT_CHAR,   num,  FMT_LONG)   /* cast char to long */
CHG_TYPE(c2s, chr,  FMT_CHAR,   sht,  FMT_SHORT)  /* cast char to short */
CHG_TYPE(d2c, val,  FMT_DOUBLE, chr,  FMT_CHAR)   /* cast double to char */
CHG_TYPE(d2f, val,  FMT_DOUBLE, flt,  FMT_FLOAT)  /* cast double to float */
CHG_TYPE(d2i, val,  FMT_DOUBLE, inum, FMT_INT)    /* cast double to int */
CHG_TYPE(d2l, val,  FMT_DOUBLE, num,  FMT_LONG)   /* cast double to long */
CHG_TYPE(d2s, val,  FMT_DOUBLE, sht,  FMT_SHORT)  /* cast double to short */
CHG_TYPE(f2c, flt,  FMT_FLOAT,  chr,  FMT_CHAR)   /* cast float to char */
CHG_TYPE(f2d, flt,  FMT_FLOAT,  val,  FMT_DOUBLE) /* cast float to double */
CHG_TYPE(f2i, flt,  FMT_FLOAT,  inum, FMT_INT)    /* cast float to int */
CHG_TYPE(f2l, flt,  FMT_FLOAT,  num,  FMT_LONG)   /* cast float to long */
CHG_TYPE(f2s, flt,  FMT_FLOAT,  sht,  FMT_SHORT)  /* cast float to short */
CHG_TYPE(i2c, inum, FMT_INT,    chr,  FMT_CHAR)   /* cast int to char */
CHG_TYPE(i2d, inum, FMT_INT,    val,  FMT_DOUBLE) /* cast int to double */
CHG_TYPE(i2f, inum, FMT_INT,    flt,  FMT_FLOAT)  /* cast int to float */
CHG_TYPE(i2l, inum, FMT_INT,    num,  FMT_LONG)   /* cast int to long */
CHG_TYPE(i2s, inum, FMT_INT,    sht,  FMT_SHORT)  /* cast int to short */
CHG_TYPE(l2c, num,  FMT_LONG,   chr,  FMT_CHAR)   /* cast long to char */
CHG_TYPE(l2d, num,  FMT_LONG,   val,  FMT_DOUBLE) /* cast long to double */
CHG_TYPE(l2f, num,  FMT_LONG,   flt,  FMT_FLOAT)  /* cast long to float */
CHG_TYPE(l2i, num,  FMT_LONG,   inum, FMT_INT)    /* cast long to int */
CHG_TYPE(l2s, num,  FMT_LONG,   sht,  FMT_SHORT)  /* cast long to short */
CHG_TYPE(s2c, sht,  FMT_SHORT,  chr,  FMT_CHAR)   /* cast short to char */
CHG_TYPE(s2d, sht,  FMT_SHORT,  val,  FMT_DOUBLE) /* cast short to double */
CHG_TYPE(s2f, sht,  FMT_SHORT,  flt,  FMT_FLOAT)  /* cast short to float */
CHG_TYPE(s2i, sht,  FMT_SHORT,  inum, FMT_INT)    /* cast short to int */
CHG_TYPE(s2l, sht,  FMT_SHORT,  num,  FMT_LONG)   /* cast short to long */

#undef CHG_TYPE


#define BIT_OPER( _suff, _symb_opr )                              \
        void bit_##_suff( const instr *i )                        \
        {                                                         \
            UPDATE_PC();                                          \
        }                                                         \
                                                                  \
        void bit_##_suff##_prt( const instr *i, const Cell *pc )  \
        {                                                         \
            PR("\n");                                             \
        }

BIT_OPER( or, "|" )   /* operador OR de bits */
BIT_OPER( xor, "^" )  /* operador XOR de bits */
BIT_OPER( and, "&" )  /* operador AND de bits */
BIT_OPER( shl, << )   /* operador << de bits */
BIT_OPER( shr, >> )   /* operador >> de bits */

#undef BIT_OPER
