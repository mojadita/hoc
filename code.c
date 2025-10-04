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
        Cell c =  { ._fld = va_arg(args, _typ) };  \
                                                   \
        progp[1] = c;                              \
                                                   \
        PRG(" " _fmt, c._fld);                     \
    }

DATUM_PROG(double,   , val,  FMT_DOUBLE)  /* la original */
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

CONSTPUSH(,   val,  FMT_DOUBLE)
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
    } /* add##_suff */                              \
                                                    \
    void _nam##_suff##_prt(                         \
            const instr    *i,                      \
            const Cell     *pc)                     \
    {                                               \
        PR("\n");                                   \
    } /* _nam##_suff##_prt         }{ */

OP(add,   , val,  +, FMT_DOUBLE)
OP(add, _c, chr,  +, FMT_CHAR) /* add top two elements on stack */
OP(add, _d, val,  +, FMT_DOUBLE)
OP(add, _f, flt,  +, FMT_FLOAT)
OP(add, _i, inum, +, FMT_INT)
OP(add, _l, num,  +, FMT_LONG)
OP(add, _s, sht,  +, FMT_SHORT)

OP(sub,   , val,  -, FMT_DOUBLE) /* subtract two elements on stack */
OP(sub, _c, chr,  -, FMT_CHAR)
OP(sub, _d, val,  -, FMT_DOUBLE)
OP(sub, _f, flt,  -, FMT_FLOAT)
OP(sub, _i, inum, -, FMT_INT)
OP(sub, _l, num,  -, FMT_LONG)
OP(sub, _s, chr,  -, FMT_SHORT)

OP(mul,   , val,  *, FMT_DOUBLE) /* multiply two elements on stack */
OP(mul, _c, chr,  *, FMT_CHAR)
OP(mul, _d, val,  *, FMT_DOUBLE)
OP(mul, _f, flt,  *, FMT_FLOAT)
OP(mul, _i, inum, *, FMT_INT)
OP(mul, _l, num,  *, FMT_LONG)
OP(mul, _s, chr,  *, FMT_SHORT)

OP(divi,   , val,  /, FMT_DOUBLE) /* multiply two elements on stack */
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

RELOP(ge,,    val,  >=,  FMT_DOUBLE)
RELOP(ge, _c, chr,  >=,  FMT_CHAR)
RELOP(ge, _d, val,  >=,  FMT_DOUBLE)
RELOP(ge, _f, flt,  >=,  FMT_FLOAT)
RELOP(ge, _i, inum, >=,  FMT_INT)
RELOP(ge, _l, num,  >=,  FMT_LONG)
RELOP(ge, _s, sht,  >=,  FMT_SHORT)

RELOP(le,,    val,  <=,  FMT_DOUBLE)
RELOP(le, _c, chr,  <=,  FMT_CHAR)
RELOP(le, _d, val,  <=,  FMT_DOUBLE)
RELOP(le, _f, flt,  <=,  FMT_FLOAT)
RELOP(le, _i, inum, <=,  FMT_INT)
RELOP(le, _l, num,  <=,  FMT_LONG)
RELOP(le, _s, sht,  <=,  FMT_SHORT)

RELOP(gt,,    val,  >,  FMT_DOUBLE)
RELOP(gt, _c, chr,  >,  FMT_CHAR)
RELOP(gt, _d, val,  >,  FMT_DOUBLE)
RELOP(gt, _f, flt,  >,  FMT_FLOAT)
RELOP(gt, _i, inum, >,  FMT_INT)
RELOP(gt, _l, num,  >,  FMT_LONG)
RELOP(gt, _s, sht,  >,  FMT_SHORT)

RELOP(lt,,    val,  <,  FMT_DOUBLE)
RELOP(lt, _c, chr,  <,  FMT_CHAR)
RELOP(lt, _d, val,  <,  FMT_DOUBLE)
RELOP(lt, _f, flt,  <,  FMT_FLOAT)
RELOP(lt, _i, inum, <,  FMT_INT)
RELOP(lt, _l, num,  <,  FMT_LONG)
RELOP(lt, _s, sht,  <,  FMT_SHORT)

RELOP(eq,,    val,  ==,  FMT_DOUBLE)
RELOP(eq, _c, chr,  ==,  FMT_CHAR)
RELOP(eq, _d, val,  ==,  FMT_DOUBLE)
RELOP(eq, _f, flt,  ==,  FMT_FLOAT)
RELOP(eq, _i, inum, ==,  FMT_INT)
RELOP(eq, _l, num,  ==,  FMT_LONG)
RELOP(eq, _s, sht,  ==,  FMT_SHORT)

RELOP(ne,,    val,  !=,  FMT_DOUBLE)
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

MOD(,   val, FMT_DOUBLE)
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

UNARY_LOP(neg,,    val,  val,  -, FMT_DOUBLE)
UNARY_LOP(neg, _c, chr,  chr,  -, FMT_CHAR) /* change sign top element on stack */
UNARY_LOP(neg, _d, val,  val,  -, FMT_DOUBLE)
UNARY_LOP(neg, _f, flt,  flt,  -, FMT_FLOAT)
UNARY_LOP(neg, _i, inum, inum, -, FMT_INT)
UNARY_LOP(neg, _l, num,  num,  -, FMT_LONG)
UNARY_LOP(neg, _s, sht,  sht,  -, FMT_SHORT)

UNARY_LOP(not,,    val,  inum, !, FMT_DOUBLE)
UNARY_LOP(not, _c, chr,  inum, !, FMT_CHAR) /* boolean not top element on stack */
UNARY_LOP(not, _d, val,  inum, !, FMT_DOUBLE)
UNARY_LOP(not, _f, flt,  inum, !, FMT_FLOAT)
UNARY_LOP(not, _i, inum, inum, !, FMT_INT)
UNARY_LOP(not, _l, num,  inum, !, FMT_LONG)
UNARY_LOP(not, _s, sht,  inum, !, FMT_SHORT)

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

PWR(,   val,  pow,        FMT_DOUBLE)
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

EVAL(,   val,  FMT_DOUBLE)
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

ARGEVAL(, val, FMT_DOUBLE)
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

ASSIGN(,   val,  FMT_DOUBLE)
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

ARGASSIGN(,   val,  FMT_DOUBLE)
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

PRINT_INST(,   val,  FMT_DOUBLE)
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

void bltin0_i(const instr *i) /* evaluate built-in on top of stack */
{
    Symbol *sym = pc[1].sym;
    Cell    res = { .inum = sym->ptr0() };

    P_TAIL(": %s() -> %li", sym->name, res.num);
    push(res);

    UPDATE_PC();
}

void bltin0_i_prt(const instr *i, const Cell *pc)
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

void prexpr_c(const instr *i)  /* print numeric value */
{
    P_TAIL("\n");
    printf("%c", pop().chr);

    UPDATE_PC();
}

void prexpr_c_prt(const instr *i, const Cell *pc)
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

void prexpr_f(const instr *i)  /* print numeric value */
{
    P_TAIL("\n");
    printf("0x%02hhx", pop().chr);

    UPDATE_PC();
}

void prexpr_f_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void prexpr_i(const instr *i)  /* print numeric value */
{
    P_TAIL("\n");
    printf("%i", pop().inum);

    UPDATE_PC();
}

void prexpr_i_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void prexpr_l(const instr *i)  /* print numeric value */
{
    P_TAIL("\n");
    printf("%li", pop().num);

    UPDATE_PC();
}

void prexpr_l_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

void prexpr_s(const instr *i)  /* print numeric value */
{
    P_TAIL("\n");
    printf("0x%04x", pop().sht);

    UPDATE_PC();
}

void prexpr_s_prt(const instr *i, const Cell *pc)
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

/* LCU: Mon Sep 29 15:31:44 -05 2025
 * TODO: desde aqui... { */
void inceval(const instr *i) /* 1. incremento, 2. evaluo la variable */
{
    int     addr = pc[0].param;
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
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].param);
}

void inceval_c(const instr *i) /* 1. incremento, 2. evaluo la variable */
{
    int     addr = pc[0].param;
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
    int     addr = pc[0].param;
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
    int     addr = pc[0].param;
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
    int     addr = pc[0].param;
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
    int     addr = pc[0].param;
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
    int     addr = pc[0].param;
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
    int     addr = pc[0].param;
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
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].param);
}

void evalinc_c(const instr *i) /* 1. copio valor,
                                * 2. evaluo variable,
                                * 3. devuelvo valor copiado. */
{
    int     addr = pc[0].param;
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
    int     addr = pc[0].param;
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
    int     addr = pc[0].param;
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
    int     addr = pc[0].param;
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
    int     addr = pc[0].param;
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
    int     addr = pc[0].param;
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
    int     addr = pc[0].param;
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
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].param);
}

void deceval_c(const instr *i) /* assign top value to next value */
{
    int     addr = pc[0].param;
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
            pc[1].sym->name, pc[0].param);
}

void deceval_d(const instr *i) /* assign top value to next value */
{
    int     addr = pc[0].param;
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
            pc[1].sym->name, pc[0].param);
}

void deceval_f(const instr *i) /* assign top value to next value */
{
    int     addr = pc[0].param;
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
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].param);
}

void deceval_i(const instr *i) /* assign top value to next value */
{
    int     addr = pc[0].param;
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
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].param);
}

void deceval_l(const instr *i) /* assign top value to next value */
{
    int     addr = pc[0].param;
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
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].param);
}

void deceval_s(const instr *i) /* assign top value to next value */
{
    int     addr = pc[0].param;
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
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].param);
}

void evaldec(const instr *i) /* assign top value to next value */
{
    int     addr = pc[0].param;
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
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].param);
}

void evaldec_c(const instr *i) /* assign top value to next value */
{
    int     addr = pc[0].param;
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
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].param);
}

void evaldec_d(const instr *i) /* assign top value to next value */
{
    int     addr = pc[0].param;
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
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].param);
}

void evaldec_f(const instr *i) /* assign top value to next value */
{
    int     addr = pc[0].param;
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
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].param);
}

void evaldec_i(const instr *i) /* assign top value to next value */
{
    int     addr = pc[0].param;
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
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].param);
}

void evaldec_l(const instr *i) /* assign top value to next value */
{
    int     addr = pc[0].param;
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
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].param);
}

void evaldec_s(const instr *i) /* assign top value to next value */
{
    int     addr = pc[0].param;
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
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].param);
}

void addvar(const instr *i) /* assign top value to next value */
{
    int     addr = pc[0].param;
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
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].param);
}

void addvar_c(const instr *i) /* assign top value to next value */
{
    int     addr = pc[0].param;
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
    int     addr = pc[0].param;
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
    int     addr = pc[0].param;
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
    int     addr = pc[0].param;
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
    int     addr = pc[0].param;
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
    int     addr = pc[0].param;
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
    int     addr = pc[0].param;
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
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].param);
}

void subvar_c(const instr *i) /* assign top value to next value */
{
    int     addr = pc[0].param;
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
    int     addr = pc[0].param;
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
    int     addr = pc[0].param;
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
    int     addr = pc[0].param;
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
    int     addr = pc[0].param;
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
    int     addr = pc[0].param;
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
    int     addr = pc[0].param;
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
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].param);
}

void mulvar_c(const instr *i) /* assign top value to next value */
{
    int     addr = pc[0].param;
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
    int     addr = pc[0].param;
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
    int     addr = pc[0].param;
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
    int     addr = pc[0].param;
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
    int     addr = pc[0].param;
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
    int     addr = pc[0].param;
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
    int     addr = pc[0].param;
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
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].param);
}

void divvar_c(const instr *i) /* assign top value to next value */
{
    int     addr = pc[0].param;
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
    int     addr = pc[0].param;
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
    int     addr = pc[0].param;
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
    int     addr = pc[0].param;
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
    int     addr = pc[0].param;
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
    int     addr = pc[0].param;
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
    int     addr = pc[0].param;
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
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].param);
}

void modvar_c(const instr *i) /* assign top value to next value */
{
    int     addr = pc[0].param;
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
    int     addr = pc[0].param;
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
    int     addr = pc[0].param;
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
    int     addr = pc[0].param;
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
    int     addr = pc[0].param;
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
    int     addr = pc[0].param;
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
    int     addr = pc[0].param;
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
    PR(GREEN"%s"ANSI_END"[%04x]\n", pc[1].sym->name, pc[0].param);
}

void pwrvar_c(const instr *i) /* assign top value to next value */
{
    int     addr = pc[0].param;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .num = var->chr = fast_pwr_l(var->chr, pop().num) };

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
    int     addr = pc[0].param;
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
    int     addr = pc[0].param;
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
    int     addr = pc[0].param;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .num = var->inum = fast_pwr_l(var->inum, pop().num) };

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
    int     addr = pc[0].param;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .num = var->num = fast_pwr_l(var->num, pop().num) };

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
    int     addr = pc[0].param;
    Symbol *sym  = pc[1].sym;
    Cell   *var  = prog + addr;
    Cell    tgt  = { .num = var->sht = fast_pwr_l(var->sht, pop().num) };

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
    int         lvar_offset = pc[0].param;
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
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].param);
}

void arginc_c(const instr *i) /* evaluo e incremento */
{
    int         lvar_offset = pc[0].param;
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
    int         lvar_offset = pc[0].param;
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
    int         lvar_offset = pc[0].param;
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
    int         lvar_offset = pc[0].param;
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
    int         lvar_offset = pc[0].param;
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
    int         lvar_offset = pc[0].param;
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
    int         lvar_offset = pc[0].param;
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
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].param);
}

void incarg_c(const instr *i) /* assign top value to next value */
{
    int         lvar_offset = pc[0].param;
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
    int         lvar_offset = pc[0].param;
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
    int         lvar_offset = pc[0].param;
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
    int         lvar_offset = pc[0].param;
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
    int         lvar_offset = pc[0].param;
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
    int         lvar_offset = pc[0].param;
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
    int         lvar_offset = pc[0].param;
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
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].param);
}

void decarg_c(const instr *i) /* assign top value to next value */
{
    int         lvar_offset = pc[0].param;
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
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].param);
}

void decarg_d(const instr *i) /* assign top value to next value */
{
    int         lvar_offset = pc[0].param;
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
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].param);
}

void decarg_f(const instr *i) /* assign top value to next value */
{
    int         lvar_offset = pc[0].param;
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
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].param);
}

void decarg_i(const instr *i) /* assign top value to next value */
{
    int         lvar_offset = pc[0].param;
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
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].param);
}

void decarg_l(const instr *i) /* assign top value to next value */
{
    int         lvar_offset = pc[0].param;
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
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].param);
}

void decarg_s(const instr *i) /* assign top value to next value */
{
    int         lvar_offset = pc[0].param;
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
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].param);
}

void argdec(const instr *i) /* assign top value to next value */
{
    int         lvar_offset = pc[0].param;
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
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].param);
}

void argdec_c(const instr *i) /* assign top value to next value */
{
    int         lvar_offset = pc[0].param;
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
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].param);
}

void argdec_d(const instr *i) /* assign top value to next value */
{
    int         lvar_offset = pc[0].param;
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
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].param);
}

void argdec_f(const instr *i) /* assign top value to next value */
{
    int         lvar_offset = pc[0].param;
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
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].param);
}

void argdec_i(const instr *i) /* assign top value to next value */
{
    int         lvar_offset = pc[0].param;
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
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].param);
}

void argdec_l(const instr *i) /* assign top value to next value */
{
    int         lvar_offset = pc[0].param;
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
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].param);
}

void argdec_s(const instr *i) /* assign top value to next value */
{
    int         lvar_offset = pc[0].param;
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
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].param);
}

void addarg(const instr *i) /* assign top value to next value */
{
    int         lvar_offset = pc[0].param;
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
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].param);
}

void addarg_c(const instr *i) /* assign top value to next value */
{
    int         lvar_offset = pc[0].param;
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
    int         lvar_offset = pc[0].param;
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
    int         lvar_offset = pc[0].param;
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
    int         lvar_offset = pc[0].param;
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
    int         lvar_offset = pc[0].param;
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
    int         lvar_offset = pc[0].param;
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
    int         lvar_offset = pc[0].param;
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
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].param);
}

void subarg_c(const instr *i) /* assign top value to next value */
{
    int         lvar_offset = pc[0].param;
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
    int         lvar_offset = pc[0].param;
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
    int         lvar_offset = pc[0].param;
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
    int         lvar_offset = pc[0].param;
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
    int         lvar_offset = pc[0].param;
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
    int         lvar_offset = pc[0].param;
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
    int         lvar_offset = pc[0].param;
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
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].param);
}

void mularg_c(const instr *i) /* assign top value to next value */
{
    int         lvar_offset = pc[0].param;
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
    int         lvar_offset = pc[0].param;
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
    int         lvar_offset = pc[0].param;
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
    int         lvar_offset = pc[0].param;
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
    int         lvar_offset = pc[0].param;
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
    int         lvar_offset = pc[0].param;
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
    int         lvar_offset = pc[0].param;
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
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].param);
}

void divarg_c(const instr *i) /* assign top value to next value */
{
    int         lvar_offset = pc[0].param;
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
    int         lvar_offset = pc[0].param;
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
    int         lvar_offset = pc[0].param;
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
    int         lvar_offset = pc[0].param;
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
    int         lvar_offset = pc[0].param;
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
    int         lvar_offset = pc[0].param;
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
    int         lvar_offset = pc[0].param;
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
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].param);
}

void modarg_c(const instr *i) /* assign top value to next value */
{
    int         lvar_offset = pc[0].param;
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
    int         lvar_offset = pc[0].param;
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
    int         lvar_offset = pc[0].param;
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
    int         lvar_offset = pc[0].param;
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
    int         lvar_offset = pc[0].param;
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
    int         lvar_offset = pc[0].param;
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
    int         lvar_offset = pc[0].param;
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
    PR(GREEN"%s"ANSI_END"<%+d>\n", pc[1].str, pc[0].param);
}

void pwrarg_c(const instr *i) /* assign top value to next value */
{
    int         lvar_offset = pc[0].param;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .num = lvar->chr = fast_pwr_l(lvar->chr, pop().num) };

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
    int         lvar_offset = pc[0].param;
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
    int         lvar_offset = pc[0].param;
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
    int         lvar_offset = pc[0].param;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .num = lvar->inum = fast_pwr_l(lvar->inum, pop().num) };

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
    int         lvar_offset = pc[0].param;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .num = lvar->num = fast_pwr_l(lvar->num, pop().num) };

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
    int         lvar_offset = pc[0].param;
    const char *lvar_name   = pc[1].str;
    Cell       *lvar        = getarg(lvar_offset);
    Cell        tgt         = { .num = lvar->sht = fast_pwr_l(lvar->sht, pop().num) };

    push(tgt);

    P_TAIL(": 0x%04hx -> "GREEN"%s"ANSI_END"<%+d> -> %li",
            lvar->sht, lvar_name, lvar_offset, tgt.num);

    UPDATE_PC();
}

void pwrarg_s_prt(const instr *i, const Cell *pc)
{
    PR("\n");
}

/* LCU: Mon Sep 29 15:37:22 -05 2025
 * Hasta aqui... ufff... todo para eliminar!!! } */

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

#define VAL2CELL(_suff, _fld, _type)                \
Cell val2cell##_suff(va_list args)                  \
{                                                   \
    Cell ret_val = { ._fld = va_arg(args, _type) }; \
    return ret_val;                                 \
} /* val2cell##_suff */

VAL2CELL(_c, chr, int)
VAL2CELL(_d, val, int)
VAL2CELL(_f, flt, double)
VAL2CELL(_i, inum, int)
VAL2CELL(_l, num, long)
VAL2CELL(_s, sht, int)

Cell val2cell(Symbol *typ, ...)
{
    va_list args;
    va_start(args, typ);
    Cell ret_val = typ->t2i->val2cell(args);
    va_end(args);
    return ret_val;
} /* val2cell */

#undef VAL2CELL
