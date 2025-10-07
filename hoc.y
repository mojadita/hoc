%{
/* hoc.y -- programa para implementar una calculadora.
 * Esta version no tiene precedencia de operadores.
 * Author: Edward Rivas <rivastkw@gmail.com>
 *       y Luis Colorado <luiscoloradourcola@gmail.com>
 * Date: Mon Dec 30 14:06:56 -05 2024
 * Copyright: (c) 2025 Edward Rivas y Luis Colorado.  All rights reserved.
 * License: BSD.
 */

#define YYDEBUG 1

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#include "config.h"
#include "colors.h"
#include "dynarray.h"
#include "hoc.h"
#include "lex.h"
#include "error.h"
#include "math.h"   /* Modulo personalizado con nuevas funciones */
#include "instr.h"
#include "init.h"   /* por los punteros a los tipos fundamentales */
#include "code.h"
#include "types.h"

#include "symbolP.h"
#include "cellP.h"
#include "scope.h"

void warning( const char *fmt, ...);
void vwarning( const char *fmt, va_list args );
void yyerror( char * msg );

static void patch_block(Cell*patch_point);
static void add_patch_return(Symbol *subr, Cell *patch_point);
static void patch_returns(const Symbol *subr, Cell *target);
static OpRel code_unpatched_op(token op);
static const Symbol *check_op_bin(const Expr *exp1, OpRel *op, const Expr *exp2);
static bool code_conv_val(const Symbol *t_src, const Symbol *t_dst);
static void patching_subr(const Symbol *subr, Cell *preamb, const char *what);

/*  Necersario para hacer setjmp y longjmp */
jmp_buf begin;

#ifndef   UQ_HOC_DEBUG /* { */
#warning  UQ_HOC_DEBUG deberia ser configurado en config.mk
#define   UQ_HOC_DEBUG                     1
#endif /* UQ_HOC_DEBUG    } */

#ifndef   UQ_HOC_TRACE_PATCHING /* { */
#warning  UQ_HOC_TRACE_PATCHING deberia ser configurado en config.mk
#define   UQ_HOC_TRACE_PATCHING            1
#endif /* UQ_HOC_TRACE_PATCHING    } */

#ifndef   UQ_MAX_SYMBOLS_PER_DECLARATION /* { */
#warning  UQ_MAX_SYMBOLS_PER_DECLARATION deberia ser configurado en config.mk
#define   UQ_MAX_SYMBOLS_PER_DECLARATION  20
#endif /* UQ_MAX_SYMBOLS_PER_DECLARATION    } */

#ifndef   UQ_RETURNS_TO_PATCH_INCRMNT /* { */
#warning  UQ_RETURNS_TO_PATCH_INCRMNT deberia ser configurado en config.mk
#define   UQ_RETURNS_TO_PATCH_INCRMNT  4
#endif /* UQ_RETURNS_TO_PATCH_INCRMNT    } */

#ifndef   UQ_ARGUMS_INCRMNT /* { */
#warning  UQ_ARGUMS_INCRMNT deberia ser configurado en config.mk
#define   UQ_ARGUMS_INCRMNT      (8)
#endif /* UQ_ARGUMS_INCRMNT    } */

#ifndef   UQ_SIZE_FP_RETADDR /* { */
#warning  UQ_SIZE_FP_RETADDR deberia ser configurado en config.mk
#define   UQ_SIZE_FP_RETADDR     (2)
#endif /* UQ_SIZE_FP_RETADDR    } */

#ifndef   UQ_SUB_CALL_INCRMNT /* { */
#warning  UQ_SUB_CALL_INCRMNT deberia ser configurado en config.mk
#define   UQ_SUB_CALL_INCRMNT    (8)
#endif /* UQ_SUB_CALL_INCRMNT    } */

#if       UQ_HOC_DEBUG /* {{ */
# define P(_fmt, ...)             \
    printf(F(_fmt), ##__VA_ARGS__)
#else /*  UQ_HOC_DEBUG    }{ */
# define P(_fmt, ...)
#endif /* UQ_HOC_DEBUG    }} */

#if UQ_HOC_TRACE_PATCHING
# define PT(_fmt, ...) P(_fmt, ##__VA_ARGS__)
#else
# define PT(_fmt, ...)
#endif

#define CODE_INST(I, ...)                \
        code_inst(INST_##I, ##__VA_ARGS__)

#define CODE_INST_TYP(_typ, _iname, ...) \
        code_inst(_typ->t2i->_iname->code_id, ##__VA_ARGS__)

Symbol *indef;  /* != NULL si estamos en una definicion de procedimiento/funcion */

/* en una llamada a funcion/procedimiento, almacena el simbolo a
 * llamar para tener acceso a la lista de argumentos del proc/func
 * y poder chequear al vuelo los tipos de estos y las expresiones
 * que se le pasan. */
static Symbol *top_sub_call_stack(void);
static void push_sub_call_stack(Symbol *sym);
static void pop_sub_call_stack(void);

size_t size_lvars = 0;

%}
/* continuamos el area de definicion y configuracion
 * de yacc */

/*  Declaracion tipos de datos de los objetos
    (TOKENS, SYMBOLOS no terminales)  */
%union {
    const instr  *inst; /* instruccion maquina */
    Symbol       *sym;  /* puntero a simbolo */
    double        val;  /* valor double */
    Cell         *cel;  /* referencia a Cell */
    Cell          lit;  /* literal */
    unsigned long num;  /* valor entero, para $<num> */
    const char   *str;  /* cadena de caracteres */
    var_decl_list vdl;  /* global var declaration list */
    var_init      vi;   /* global var name & initializer */
    Expr          expr; /* tipo con un puntero a Cell y una referencia a un tipo. */
    token         tok;   /* tipo asociado a un operador, con todo el token */
    OpRel         opr;  /* tipo del operador relacional. */
}

%token        ERROR
%token <lit>  DOUBLE FLOAT
%token <sym>  VAR LVAR BLTIN0 BLTIN1 BLTIN2 CONST
%token <sym>  FUNCTION PROCEDURE
%token        PRINT WHILE IF ELSE SYMBS SYMBS_ALL BRKPT
%token <tok>  OR AND GE LE EQ NE EXP
%token <tok>  BIT_AND_EQ BIT_OR_EQ BIT_XOR_EQ
%token <tok>  SHIFT_LEFT SHIFT_LEFT_EQ SHIFT_RIGHT SHIFT_RIGHT_EQ
%token <tok>  PLS_PLS MIN_MIN PLS_EQ MIN_EQ MUL_EQ DIV_EQ MOD_EQ PWR_EQ
%token <num>  FUNC PROC
%token <lit>  CHAR SHORT INTEGER LONG
%token        RETURN
%token <str>  STRING UNDEF
%token        LIST
%token <sym>  TYPE
%type  <cel>  stmt cond stmtlist bitop
%type  <expr> expr expr_or expr_and expr_bitor expr_bitand expr_bitxor expr_shift
%type  <expr> expr_rel expr_arit term fact prim expr_bltin
%type  <cel>  mark
%type  <cel>  expr_seq item do else and or preamb create_scope
%type  <num>  arglist_opt arglist formal_arglist_opt formal_arglist
%type  <sym>  proc_head func_head lvar_definable_ident function procedure
%type  <str>  lvar_valid_ident gvar_valid_ident
%type  <vdl>  gvar_decl_list gvar_decl lvar_decl_list lvar_decl
%type  <vi>   gvar_init lvar_init
%type  <tok>  '+' '-' '*' '/' '%' '<' '>' op_assign bitop_assign '&' '|' '^' '~'
%type  <opr>  op_rel op_sum op_mul op_exp

%%
/*  Area de definicion de reglas gramaticales */

list: /* empty */
    | list       '\n'

    | list defn  '\n'
    | list gvar_decl '\n' {
                         CODE_INST(STOP);
                         return 1;
                       }

    | list stmt  '\n'  { CODE_INST(STOP);  /* para que execute() pare al final */
                         return 1; }
    | list expr  '\n'  { bool expr_type_ne_prev_type = ($2.typ != Prev->typref);
                         if (expr_type_ne_prev_type) {
                            CODE_INST(dupl);
                         }
                         code_conv_val($2.typ, Prev->typref);
                         CODE_INST_TYP(Prev->typref, assign, Prev);
                         if (expr_type_ne_prev_type) {
                            CODE_INST(drop);
                         }
                         CODE_INST_TYP($2.typ, print);
                         CODE_INST(STOP);  /* para que execute() pare al final */
                         return 1; }
    | list error error_end {  yyerrok;
                         CODE_INST(STOP);
                         while (get_current_scope()) {
                            end_scope();
                         }
                         return 1; }
    ;

error_end
    : '\n'
    | ';'
    ;

stmt
    : expr        ';'      { $$ = $1.cel;
                             CODE_INST(drop); }
    | RETURN      ';'      { defnonly((indef != NULL) && (indef->type == PROCEDURE),
                                      "return;");
                             PT(">>> inserting UNPATCHED code @ [%04lx]\n", progp - prog);
                             $$ = CODE_INST(Goto, 0);
                             PT("<<< end inserting UNPATCHED code\n");
                             add_patch_return(indef, $$);
                           }
    | RETURN expr ';'      { defnonly((indef != NULL) && (indef->type == FUNCTION),
                                      "return <expr>;");
                             $$ = $2.cel;
                             /* asigno a la direccion de retorno de la funcion, en la
                              * cima de la lista de parametros */
                             code_conv_val($2.typ, indef->typref);
                             CODE_INST_TYP(indef->typref, argassign,
                                       indef->size_args + UQ_SIZE_FP_RETADDR,
                                       "{RET_VAL} ");
                             CODE_INST(drop);

                             PT(">>> inserting UNPATCHED code @ [%04lx]\n", progp - prog);
                             Cell *p = CODE_INST(Goto, prog);
                             add_patch_return(indef, p);
                             PT("<<< end inserting UNPATCHED code\n");
                           }
    | PRINT expr_seq ';'   { $$ = $2; }
    | SYMBS          ';'   { $$ = CODE_INST(symbs); }
    | SYMBS_ALL      ';'   { $$ = CODE_INST(symbs_all, get_current_symbol()); }
    | BRKPT          ';'   { $$ = CODE_INST(brkpt, get_current_symbol()); }
    | LIST           ';'   { $$ = CODE_INST(list); }
    | WHILE cond do stmt   { $$ = $2;
                             CODE_INST(Goto, $2);
                             Cell *saved_progp = progp;
                             progp = $3;
                             PT(">>> patching CODE @ [%04lx]\n", progp - prog);
                                 CODE_INST(if_f_goto, saved_progp);
                             PT("<<< end patching CODE @ [%04lx], "
                                "continuing @ [%04lx]\n",
                                 progp - prog, saved_progp - prog);
                             progp = saved_progp; }

    | IF cond do stmt      { $$ = $2;
                             Cell *saved_progp = progp;
                             progp = $3;
                             PT(">>> patching CODE @ [%04lx]\n", progp - prog);
                                 CODE_INST(if_f_goto, saved_progp);
                             PT("<<< end patching CODE @ [%04lx], continuing @ [%04lx]\n",
                                     progp - prog, saved_progp - prog);
                             progp = saved_progp;
                           }

    | IF cond do stmt else stmt {
                             $$ = $2;
                             PT(">>> PATCHING CODE @ [%04lx]\n", progp - prog);
                             Cell *saved_progp = progp;
                             progp = $3;
                             PT("*** $6 == %p\n", $6);
                             CODE_INST(if_f_goto, $6);
                             PT("<<< end PATCHING CODE @ [%04lx], "
                                   "continuing @ [%04lx]\n",
                                   progp - prog, saved_progp - prog);
                             progp = $5;
                             PT(">>> PATCHING CODE @ [%04lx]\n", progp - prog);
                                 CODE_INST(Goto, saved_progp);
                             PT("<<< end PATCHING CODE @ [%04lx], "
                                   "continuing @ [%04lx]\n",
                                   progp - prog, saved_progp - prog);
                             progp = saved_progp;
                           }

    | procedure mark '(' arglist_opt ')' ';' {
                             $$ = $2;
                             CODE_INST(call, $1);             /* instruction */
                             CODE_INST(spadd, $1->size_args); /* pop arguments */
                             pop_sub_call_stack();
                           }

    | '{' create_scope stmtlist '}'  {
                             $$ = $2;
                             scope *cs = get_current_scope();
                             if (cs->base_offset + cs->size > size_lvars) {
                                 size_lvars = cs->base_offset + cs->size;
                                 PT("*** UPDATING size_lvars TO %zd\n", size_lvars);
                             }
                             if (get_root_scope() == cs) {
                                 patch_block($2);
                             }
                             end_scope();
                           }
    ;

procedure
    : PROCEDURE            { push_sub_call_stack($1); }
    ;

create_scope
    :  /* empty */         { scope *sc = start_scope();
                             if (get_root_scope() == sc) {
                                 PT(">>> begin UNPATCHED code @ [%04lx]\n", progp - prog);
                                 $$ = CODE_INST(noop);
                                 PT("<<< end   UNPATCHED code\n");
                             } else {
                                 $$ = progp;
                             }
                           }
    ;

/* DECLARACION DE VARIABLES GLOBALES */
gvar_decl
    : gvar_decl_list ';'
    ;

gvar_decl_list
    : gvar_decl_list ',' gvar_init  { $$ = $1;
                                      if ($$.start == NULL && $3.start != NULL) {
                                          $$.start = $3.start;
                                      }
                                      Symbol *var = register_global_var(
                                            $3.name, $$.type_decl);
                                      if ($3.start) {
                                          code_conv_val($3.type_expr_init, $$.type_decl);
                                          CODE_INST_TYP($$.type_decl, assign, var);
                                          CODE_INST(drop);
                                      }
                                    }
    | TYPE gvar_init                { $$.type_decl = $1;
                                      $$.start     = $2.start;
                                      Symbol *var  = register_global_var(
                                            $2.name, $$.type_decl);
                                      if ($2.start) {
                                          code_conv_val($2.type_expr_init, $$.type_decl);
                                          CODE_INST_TYP($$.type_decl, assign, var);
                                          CODE_INST(drop);
                                      }
                                    }
    ;

gvar_init
    : gvar_valid_ident              { $$.name           = $1;
                                      $$.start          = NULL;
                                      $$.type_expr_init = NULL; }
    | gvar_valid_ident '=' expr     { $$.name           = $1;
                                      $$.start          = $3.cel;
                                      $$.type_expr_init = $3.typ; }
    ;

gvar_valid_ident
    : UNDEF
    ;

/* DECLARACION DE VARIABLES LOCALES */

lvar_decl :  lvar_decl_list ';'
    ;

lvar_decl_list
    : lvar_decl_list ',' lvar_init  { $$ = $1;
                                      if ($$.start == NULL && $3.start != NULL) {
                                          $$.start = $3.start;
                                      }
                                      Symbol *var = register_local_var(
                                            $3.name, $$.type_decl);
                                      if ($3.start) {
                                          code_conv_val(
                                                $3.type_expr_init,
                                                $$.type_decl);
                                          CODE_INST_TYP($$.type_decl, argassign,
                                                var->offset, var->name);
                                          CODE_INST(drop);
                                      }
                                    }

    | TYPE lvar_init                { $$.type_decl = $1;
                                      $$.start     = $2.start;
                                      Symbol *var  = register_local_var(
                                            $2.name, $$.type_decl);
                                      if ($2.start) {
                                          code_conv_val(
                                                $2.type_expr_init,
                                                $$.type_decl);
                                          CODE_INST_TYP($$.type_decl, argassign,
                                                var->offset, var->name);
                                          CODE_INST(drop);
                                      }
                                    }
    ;

lvar_init
    : lvar_valid_ident          { $$.name           = $1;
                                  $$.start          = NULL;
                                  $$.type_expr_init = NULL; }
    | lvar_valid_ident '=' expr { $$.name           = $1;
                                  $$.start          = $3.cel;
                                  $$.type_expr_init = $3.typ; }
    ;

lvar_valid_ident
    : UNDEF
    | lvar_definable_ident { $$ = $1->name; }
    ;

lvar_definable_ident
    : VAR
    | LVAR
    ;

do  :  /* empty */         {
                             PT(">>> inserting UNPATCHED CODE @ [%04lx]\n",
                                     progp - prog);
                             $$ = CODE_INST(if_f_goto, prog);
                             PT("<<< end inserting UNPATCHED CODE @ [%04lx]\n",
                                     progp - prog);
                           }
    ;

else:  ELSE                {
                             PT(">>> inserting UNPATCHED CODE @ [%04lx]\n",
                                     progp - prog);
                             $$ = CODE_INST(Goto, prog);
                             PT("<<< end inserting UNPATCHED CODE @ [%04lx]\n",
                                     progp - prog);
                           }
    ;

expr_seq
    : expr_seq ',' item
    | item
    ;

item: STRING               { $$ = CODE_INST(prstr, $1); }
    | expr                 { const Symbol *type = $1.typ;
                             $$ = CODE_INST_TYP(type, prexpr); }
    ;

cond: '(' expr ')'         { $$ = $2.cel;
                             code_conv_val($2.typ, Integer); }
    ;

mark: /* empty */          { $$ = progp; }
    ;

stmtlist
    : stmtlist '\n'
    | stmtlist stmt
    | stmtlist lvar_decl
    | /* empty */          { $$ = progp; }
    ;

expr
    : VAR    '=' expr      { $$ = $3;
                             code_conv_val($3.typ, $1->typref);
                             CODE_INST_TYP($1->typref, assign, $1);
                           }
    | LVAR   '=' expr      { $$ = $3;
                             code_conv_val($3.typ, $1->typref);
                             CODE_INST_TYP($1->typref, argassign,
                                    $1->offset, $1->name);
                           }
    | VAR   op_assign expr { $$ = $3;
                             /* LCU: Wed Oct  1 14:44:15 -05 2025
                              *
                              * The next code is encoded as a macro, as it
                              * is repeated below, for the next rule, but
                              * changing the parameters associated to the
                              * eval/assign vs the argeval/argassign
                              * instructions respectively.
                              *
                              * NOTE: do not move this macro definition from
                              * this place, as the macro substitution is made
                              * in the C code (after the substitution of the
                              * $<n> arguments are made in yacc, breaking the
                              * context of what gets substituted.
                              */
#define VAR_OPASSIGN_EXPR(_eval, _assign, ...) do {                                                  \
                                 const Symbol *var       = $1,                                       \
                                              *var_type  = var->typref,                              \
                                              *exp_type  = $3.typ,                                   \
                                              *res_type  = exp_type;                                 \
                                 const token   op        = $2;                                       \
                                 $$.cel = $3.cel;                                                    \
                                 $$.typ = var_type;                                                  \
                                                                                                     \
                                 code_conv_val(exp_type, var_type);   /* convert expr to var type */ \
                                 CODE_INST_TYP(var_type, _eval, ##__VA_ARGS__); /* variable evaluation */ \
                                 CODE_INST(swap);                     /* swap them */                \
                                 switch (op.id) {                                                    \
                                 case PLS_EQ:          CODE_INST_TYP(res_type, add,  var); break;    \
                                 case MIN_EQ:          CODE_INST_TYP(res_type, sub,  var); break;    \
                                 case MUL_EQ:          CODE_INST_TYP(res_type, mul,  var); break;    \
                                 case DIV_EQ:          CODE_INST_TYP(res_type, divi, var); break;    \
                                 case MOD_EQ:          CODE_INST_TYP(res_type, mod,  var); break;    \
                                 case PWR_EQ:          CODE_INST_TYP(res_type, pwr,  var); break;    \
                                 case SHIFT_LEFT_EQ:   CODE_INST(bit_shl,            var); break;    \
                                 case SHIFT_RIGHT_EQ:  CODE_INST(bit_shr,            var); break;    \
                                 case BIT_OR_EQ:       CODE_INST(bit_or,             var); break;    \
                                 case BIT_XOR_EQ:      CODE_INST(bit_xor,            var); break;    \
                                 case BIT_AND_EQ:      CODE_INST(bit_and,            var); break;    \
                                 } /* switch */                                                      \
                                 CODE_INST_TYP(var_type, _assign, ##__VA_ARGS__); /* assign to variable */ \
                             } while (0) /* VAR_OPASSIGN_EXPR */

                             VAR_OPASSIGN_EXPR(eval, assign, var);
                           }
    | LVAR op_assign expr  { /* LCU: Wed Oct  1 14:51:06 -05 2025
                              * NOTE: See above comment in previous rule*/
                             VAR_OPASSIGN_EXPR(argeval, argassign, var->offset, var->name);
                           }
    | VAR bitop_assign expr {
                             if ($1->typref->t2i->flags & TYPE_IS_FLOATING_POINT) {
                                 execerror("cannot %s operate to " BRIGHT GREEN "%s" ANSI_END
                                           ": must be integral type\n",
                                           $2.lex, $1->name);
                             }
                             VAR_OPASSIGN_EXPR(eval, assign, var);
                           }
    | LVAR bitop_assign expr {
                             if ($1->typref->t2i->flags & TYPE_IS_FLOATING_POINT) {
                                 execerror("cannot %s operate to " BRIGHT GREEN "%s" ANSI_END
                                           ": must be integral type\n",
                                           $2.lex, $1->name);
                             }
                             VAR_OPASSIGN_EXPR(argeval, argassign, var->offset, var->name);
                           }
    | expr_or
    ;

op_assign
    : PLS_EQ
    | MIN_EQ
    | MUL_EQ
    | DIV_EQ
    | MOD_EQ
    | PWR_EQ
    ;

bitop_assign
    : SHIFT_LEFT_EQ
    | SHIFT_RIGHT_EQ
    | BIT_OR_EQ
    | BIT_XOR_EQ
    | BIT_AND_EQ
    ;

expr_or
    : expr_and or expr_or  { $$.cel = $1.cel;
                             $$.typ = Integer;
                             code_conv_val($3.typ, Integer);

                             Cell *saved_progp = progp;
                             progp = $2;
                             PT(">>> begin patching CODE @ [%04lx]\n", progp - prog);
                                 code_conv_val($1.typ, Integer);
                                 CODE_INST(or_else, saved_progp);
                             PT("<<< end   patching CODE @ [%04lx], continuing @ [%04lx]\n",
                                   progp - prog, saved_progp - prog);
                             progp = saved_progp;

                           }
    | expr_and
    ;

or  : OR                   { PT(">>> begin inserting unpatched CODE @ [%04lx]\n", progp - prog);
                             $$ = CODE_INST(noop); /* para la instruccion de conversion */
                                  CODE_INST(noop); /* para la instruccion or_else */
                             PT("<<< end   inserting unpatched CODE\n"); }
    ;

expr_and
    : expr_bitor and expr_and { $$.cel = $1.cel;
                              $$.typ = Integer;
                              code_conv_val($3.typ, Integer);
                              Cell *saved_progp = progp;
                              progp = $2;
                              PT(">>> begin patching CODE @ [%04lx]\n", progp - prog);
                                  code_conv_val($1.typ, Integer);
                                  CODE_INST(and_then, saved_progp);
                              PT("<<< end   patching CODE @ [%04lx], "
                                      "continuing @ [%04lx]\n",
                                      progp - prog, saved_progp - prog);
                              progp = saved_progp; }
    | expr_bitor
    ;

and : AND                   {
                              PT(">>> begin inserting unpatched CODE @ [%04lx]\n",
                                      progp - prog);
                              $$ = CODE_INST(noop);
                                   CODE_INST(noop);
                              PT("<<< end   inserting unpatched CODE\n");
                            }
    ;

expr_bitor
    : expr_bitor bitop '|' expr_bitxor {

#define BITOP(_inst)          do {                                          \
                                  $$.cel = $1.cel;                          \
                                  $$.typ = Integer;                         \
                                  const Symbol *left_type  = $1.typ,        \
                                               *right_type = $4.typ;        \
                                  Cell *saved_progp = progp;                \
                                  progp = $2;                               \
                                  PT(">>> begin PATCHING CODE @ [%04lx]\n", \
                                          progp - prog);                    \
                                  code_conv_val($1.typ, Integer);           \
                                  PT("<<< end   PATCHING CODE\n");          \
                                  progp = saved_progp;                      \
                                  code_conv_val($4.typ, Integer);           \
                                  CODE_INST(_inst);                         \
                              } while (0)

                              BITOP(bit_or);
                            }
    | expr_bitxor
    ;

expr_bitxor
    : expr_bitxor bitop '^' expr_bitand {
                              BITOP(bit_xor);
                            }
    | expr_bitand
    ;

expr_bitand
    : expr_bitand bitop '&' expr_shift {
                              BITOP(bit_and);
                            }
    | expr_shift
    ;

bitop
    :  /* empty */          { PT(">>> begin inserting UNPATCHED CODE @ [%04lx]\n",
                                      progp - prog);
                              $$ = CODE_INST(noop);
                              PT("<<< end inserting UNPATCHED CODE\n");
                            }
    ;

expr_shift
    : expr_shift bitop SHIFT_LEFT expr_rel {
                              BITOP(bit_shl);
                            }
    | expr_shift bitop SHIFT_RIGHT expr_rel {
                              BITOP(bit_shr);
                            }
    | expr_rel
    ;

expr_rel
    : expr_arit op_rel expr_arit {
                              $$.typ = Integer;
                              $$.cel = $1.cel;
                              const Symbol *op_type = check_op_bin(&$1, &$2, &$3);
                              switch ($2.tok.id) {
                                  case '<':  CODE_INST_TYP(op_type, lt); break;
                                  case '>':  CODE_INST_TYP(op_type, gt); break;
                                  case  EQ:  CODE_INST_TYP(op_type, eq); break;
                                  case  NE:  CODE_INST_TYP(op_type, ne); break;
                                  case  GE:  CODE_INST_TYP(op_type, ge); break;
                                  case  LE:  CODE_INST_TYP(op_type, le); break;
                              } /* switch */
                            }
    | expr_arit
    ;

op_rel
    : '<'                   { $$ = code_unpatched_op($1); }
    | '>'                   { $$ = code_unpatched_op($1); }
    | EQ                    { $$ = code_unpatched_op($1); }
    | NE                    { $$ = code_unpatched_op($1); }
    | GE                    { $$ = code_unpatched_op($1); }
    | LE                    { $$ = code_unpatched_op($1); }
    ;

expr_arit
    : expr_arit op_sum term { /* LCU: Mon Sep 15 12:31:40 -05 2025
                               * NOTA 1:
                               * determinar el tipo de la expr_arit, a partir de los
                               * tipos de $1 y $3. En caso de que los tipos sean
                               * distintos, generar codigo para converti el tipo de
                               * $3 si $3 debe convertirse al tipo de $1 y parchear
                               * op_add si el que debe convertirse es $1 al tipo
                               * de $3 */
                              $$.cel = $1.cel;
                              $$.typ = check_op_bin(&$1, &$2, &$3);
                              switch($2.tok.id) {
                              case '+': CODE_INST_TYP($$.typ, add); break;
                              case '-': CODE_INST_TYP($$.typ, sub); break;
                              }
                            }
    | term
    ;

op_sum
    : '+'                   { $$ = code_unpatched_op($1); }
    | '-'                   { $$ = code_unpatched_op($1); }
    ;

term
    : term op_mul fact      { /* LCU: Mon Sep 15 12:36:54 -05 2025
                               * ver NOTA 1, arriba. */
                              $$.typ = check_op_bin(&$1, &$2, &$3);
                              switch($2.tok.id) {
                              case '*': CODE_INST_TYP($$.typ, mul);  break;
                              case '/': CODE_INST_TYP($$.typ, divi); break;
                              case '%': if (!($$.typ->t2i->flags & TYPE_IS_INTEGER))
                                            execerror("%% only operates on integer"
                                                  " values");
                                        CODE_INST_TYP($$.typ, mod);  break;
                              }
                            }
    | fact
    ;

op_mul
    : '*'                   { $$ = code_unpatched_op($1); }
    | '/'                   { $$ = code_unpatched_op($1); }
    | '%'                   { $$ = code_unpatched_op($1); }
    ;

fact: prim op_exp fact      {
                              /* LCU: Mon Sep 15 12:36:54 -05 2025
                               * ver NOTA 1, arriba. */
                              $$.typ = check_op_bin(&$1, &$2, &$3);
                              CODE_INST_TYP($$.typ, pwr);
                            }
    | prim
    ;

op_exp
    : EXP                   { $$ = code_unpatched_op($1); }
    ;

prim: UNDEF                 { execerror("Symbol " BRIGHT GREEN "%s"
                                        ANSI_END " undefined", $1); }
    | '(' TYPE ')' prim     { $$.cel = $4.cel;
                              $$.typ = $2; /* generamos el tipo del resultado y la
                                            * posicion de comienzo del codigo. */
                              code_conv_val($4.typ, $2); /* Insercion del codigo
                                                          * a ejecutar */
                            }
    | '(' expr ')'          { $$ = $2; }
    | FLOAT                 { $$.cel = CODE_INST_TYP(Float,  constpush, $1);
                              $$.typ = Float;
                            }
    | DOUBLE                { $$.cel = CODE_INST_TYP(Double,  constpush, $1);
                              $$.typ = Double;
                            }
    | CHAR                  { $$.cel = CODE_INST_TYP(Char,    constpush, $1);
                              $$.typ = Char;
                            }
    | SHORT                 { $$.cel = CODE_INST_TYP(Short, constpush, $1);
                              $$.typ = Short;
                            }
    | INTEGER               { $$.cel = CODE_INST_TYP(Integer, constpush, $1);
                              $$.typ = Integer;
                            }
    | LONG                  { $$.cel = CODE_INST_TYP(Long, constpush, $1);
                              $$.typ = Long;
                            }
    | VAR                   { $$.cel = CODE_INST_TYP($1->typref, eval,    $1);
                              $$.typ = $1->typref;
                            }
    | LVAR                  { $$.cel = CODE_INST_TYP($1->typref, argeval,
                                                     $1->offset, $1->name);
                              $$.typ = $1->typref; }
    | '!' prim              {
#define NEG_OP(_inst)         do {        /* { */                  \
                                  $$.cel = $2.cel;                 \
                                  $$.typ = Integer;                \
                                  code_conv_val($2.typ, Integer);  \
                                  CODE_INST(_inst);                \
                              } while (0)

                              NEG_OP(not);
                            }
    | '~' prim              { NEG_OP(bit_not); }
    | '+' prim              { $$ = $2; }
    | '-' prim              { $$ = $2; CODE_INST_TYP($2.typ, neg);  }

    | PLS_PLS VAR           {
#define INCDEC_PRE(_eval, _op, _assign, ...) do {                     \
                                  const Symbol                        \
                                         *var = $2,                   \
                                         *type = var->typref;         \
                                  $$.cel = progp;                     \
                                  $$.typ = type;                      \
                                                                      \
                                  CODE_INST_TYP(type, _eval,          \
                                                ##__VA_ARGS__);       \
                                  CODE_INST_TYP(type,                 \
                                                constpush,            \
                                                type->t2i->one);      \
                                  CODE_INST_TYP(type, _op);           \
                                  CODE_INST_TYP(type, _assign,        \
                                                ##__VA_ARGS__);       \
                              } while (0)

                              INCDEC_PRE(eval,    add, assign,    var);
                            }
    | MIN_MIN VAR           { INCDEC_PRE(eval,    sub, assign,    var); }

    | PLS_PLS LVAR          { INCDEC_PRE(argeval, add, argassign, var->offset, var->name); }
    | MIN_MIN LVAR          { INCDEC_PRE(argeval, sub, argassign, var->offset, var->name); }

    | VAR     PLS_PLS       {
#define INCDEC_POST(_eval, _op, _assign, ...) do {                    \
                                  const Symbol                        \
                                         *var  = $1,                  \
                                         *type = var->typref;         \
                                  $$.cel = progp;                     \
                                  $$.typ = type;                      \
                                                                      \
                                  CODE_INST_TYP(type, _eval,          \
                                                ##__VA_ARGS__);       \
                                  CODE_INST(dupl);                    \
                                  CODE_INST_TYP(type,                 \
                                                constpush,            \
                                                type->t2i->one);      \
                                  CODE_INST_TYP(type, _op);           \
                                  CODE_INST_TYP(type, _assign,        \
                                                ##__VA_ARGS__);       \
                                  CODE_INST(drop);                    \
                              } while (0)

                              INCDEC_POST(eval,    add, assign,    var);
                            }
    | VAR     MIN_MIN       { INCDEC_POST(eval,    sub, assign,    var); }

    | LVAR    PLS_PLS       { INCDEC_POST(argeval, add, argassign,
                                          var->offset, var->name);
                            }
    | LVAR    MIN_MIN       { INCDEC_POST(argeval, sub, argassign,
                                          var->offset, var->name);
                            }

    | CONST                 { $$.cel = CODE_INST_TYP($1->typref,
                                                     constpush,
                                                     $1->val);
                              $$.typ = $1->typref;
                            }

    | BLTIN0 '(' ')'        { $$.cel = CODE_INST(bltin0, $1);
                              $$.typ = $1->typref; }
    | BLTIN1 '(' expr_bltin ')'   { $$.cel = $3.cel;
                              $$.typ = $1->typref;
                              CODE_INST(bltin1, $1); }
    | BLTIN2 '(' expr_bltin ',' expr_bltin ')'
                            { $$.cel = $3.cel;
                              $$.typ = $1->typref;
                              CODE_INST(bltin2, $1); }
    | function mark '(' arglist_opt ')' {
                              $$.cel = $2;
                              $$.typ = $1->typref;
                              if ($4 != $1->argums_len) {
                                  execerror(" " BRIGHT GREEN "%s"
                                       ANSI_END " accepts "
                                       "%d arguments, passed %d",
                                       $1->name, $1->argums_len, $4);
                              }
                              CODE_INST(call,  $1);            /* instruction */
                              CODE_INST(spadd, $1->size_args);
                              pop_sub_call_stack(); }          /* eliminando argumentos */
    ;

expr_bltin: expr { code_conv_val($1.typ, Double); }

function
    : FUNCTION              { push_sub_call_stack($1);
                              CODE_INST(spadd, -$1->typref->t2i->size); /* PUSH espacio para el */
                            }                                           /* valor a retornar */
    ;

arglist_opt
    : arglist
    | /* empty */           { $$ = 0; }
    ;

arglist
    : arglist ',' expr      { $$ = $1 + 1;
                              Symbol *sub_call = top_sub_call_stack();
                              if ($$ > sub_call->argums_len) {
                                  execerror(" " BRIGHT GREEN "%s"
                                            ANSI_END " accepts "
                                            "only %d parameters, passed %d",
                                            sub_call->name,
                                            sub_call->argums_len,
                                            $$);
                              }
                              code_conv_val($3.typ, sub_call->argums[$1]->typref);
                            }
    | expr                  { $$ = 1;
                              Symbol *sub_call = top_sub_call_stack();
                              if (sub_call->argums_len == 0) {
                                  execerror(" " BRIGHT GREEN "%s"
                                            ANSI_END " accepts no"
                                            " parameters",
                                            sub_call->name);
                              }
                              code_conv_val($1.typ, sub_call->argums[0]->typref);
                            }
    ;

defn: proc_head '(' formal_arglist_opt ')' preamb block {
                              /* PARCHEO DE CODIGO */
                              patching_subr($1, $5, "PROCEDIMIENTO");
                            }

    | func_head '(' formal_arglist_opt ')' preamb block {
                              /* PARCHEO DE CODIGO */
                              patching_subr($1, $5, "FUNCION");
                            }
    ;

preamb: /* empty */         {
                              CODE_INST(push_fp);
                              CODE_INST(move_sp_to_fp);
                              PT(">>> begin UNPATCHED code @ [%04lx]\n", progp - prog);
                              /* LCU: Mon Sep  8 13:17:49 EEST 2025
                               * we need to return this pointer (not
                               * a pointer to the beginning of the code)
                               * because it is what we need to patch. */
                              $$ = CODE_INST(noop);
                              PT("<<< end   UNPATCHED code\n");
                            }
    ;

block
    : '{' stmtlist '}'
    ;

formal_arglist_opt
    : formal_arglist        {
                              PT("*** formal_arg_list_opt == %ld\n", $1);
                              /* cambiando los offsets para que se refieran a las
                               * posiciones de las expresiones calculadas antes de
                               * entrar a la funcion (sumando a sus offsets la cantidad
                               * indef->size_args */
                              for (int i = 0; i < indef->argums_len; ++i) {
                                    indef->argums[i]->offset += indef->size_args
                                                              + UQ_SIZE_FP_RETADDR;
                                    PT("+++ arg[%d] (%s), offset = %d\n",
                                        i,
                                        indef->argums[i]->name,
                                        indef->argums[i]->offset);
                              }
                              if (indef->type == FUNCTION) {
                                  PT("+++ RET_VAL offset = %d\n",
                                            indef->size_args
                                            + UQ_SIZE_FP_RETADDR);
                              }
                              get_current_scope()->size = 0;
                              $$ = indef->argums_len;
                            }
    | /* empty */           { $$ = 0; }
    ;

formal_arglist
    : formal_arglist ',' TYPE lvar_valid_ident {
                              Symbol *subr = indef;
                              DYNARRAY_GROW(
                                    subr->argums,
                                    Symbol *,
                                    1,
                                    UQ_ARGUMS_INCRMNT);
                              Symbol *sym = register_local_var($4, $3);
                              subr->argums[subr->argums_len++] = sym;
                              subr->size_args += $3->t2i->size;
                              $$ = $1 + 1;
                            }
    | TYPE lvar_valid_ident {
                              Symbol *subr = indef;
                              DYNARRAY_GROW(
                                    subr->argums,
                                    Symbol *,
                                    1,
                                    UQ_ARGUMS_INCRMNT);
                              Symbol *sym = register_local_var($2, $1);
                              subr->argums[subr->argums_len++] = sym;
                              subr->size_args += $1->t2i->size;
                              $$ = 1;
                            }
    ;

proc_head
    : PROC UNDEF            {
                              $$ = define($2, PROCEDURE, NULL, progp);
                              $$->main_scope = start_scope();
                              P("DEFINIENDO EL PROCEDIMIENTO '%s' @ [%04lx]\n",
                                        $2, progp - prog);
                              indef = $$;
                            }
    ;

func_head
    : FUNC TYPE UNDEF       {
                              $$ = define($3, FUNCTION, $2, progp);
                              $$->main_scope = start_scope();
                              P("DEFINIENDO LA FUNCION '%s' @ [%04lx]\n",
                                $3, progp - prog);
                              indef = $$;
                            }
    ;

%%

void patching_subr(
        const Symbol *subr,
        Cell         *preamb,
        const char *what)
{
    scope *cs = get_root_scope();

    if (cs->base_offset + cs->size > size_lvars) {
      size_lvars = cs->base_offset + cs->size;
      PT("*** UPDATING size_lvars TO %zd\n", size_lvars);
    }
    patch_returns(subr, progp); /* parcheamos todos los RETURN del
                                 * block */
    patch_block(preamb);        /* parcheamos el spadd, 0 de preamb */

    /* CODIGO A INSERTAR PARA TERMINAR (POSTAMBULO) */
    CODE_INST(pop_fp);
    CODE_INST(ret);
    end_scope();
    end_define(subr);
    indef = NULL;
    P("FIN DEFINICION %s\n", what);
} /* patching_subr */

/* en una llamada a funcion/procedimiento, almacena el simbolo a
 * llamar para tener acceso a la lista de argumentos del proc/func
 * y poder chequear al vuelo los tipos de estos y las expresiones
 * que se le pasan. */
static Symbol **sub_call_stack     = NULL;
static size_t   sub_call_stack_len = 0,
                sub_call_stack_cap = 0;

Symbol *top_sub_call_stack()
{
    return sub_call_stack[sub_call_stack_len - 1];
}

void push_sub_call_stack(Symbol *sym)
{
    DYNARRAY_GROW(sub_call_stack, Symbol *, 1, UQ_SUB_CALL_INCRMNT);
    sub_call_stack[sub_call_stack_len++] = sym;
}

void pop_sub_call_stack(void)
{
    --sub_call_stack_len;
}

bool
code_conv_val(
        const Symbol *t_src,
        const Symbol *t_dst)
{
    bool needs_to_change = t_src != t_dst;

    if (needs_to_change) {
        if          (t_src == Char) {
            if      (t_dst == Double)  CODE_INST(c2d);
            else if (t_dst == Float)   CODE_INST(c2f);
            else if (t_dst == Integer) CODE_INST(c2i);
            else if (t_dst == Long)    CODE_INST(c2l);
            else if (t_dst == Short)   CODE_INST(c2s);
            else execerror("Bad conversion from %s to %s",
                    t_src->name, t_dst->name);
        } else if   (t_src == Double) {
            if      (t_dst == Char)    CODE_INST(d2c);
            else if (t_dst == Float)   CODE_INST(d2f);
            else if (t_dst == Integer) CODE_INST(d2i);
            else if (t_dst == Long)    CODE_INST(d2l);
            else if (t_dst == Short)   CODE_INST(d2s);
            else execerror("Bad conversion from %s to %s",
                    t_src->name, t_dst->name);
        } else if   (t_src == Float) {
            if      (t_dst == Char)    CODE_INST(f2c);
            else if (t_dst == Double)  CODE_INST(f2d);
            else if (t_dst == Integer) CODE_INST(f2i);
            else if (t_dst == Long)    CODE_INST(f2l);
            else if (t_dst == Short)   CODE_INST(f2s);
            else execerror("Bad conversion from %s to %s",
                    t_src->name, t_dst->name);
        } else if   (t_src == Integer) {
            if      (t_dst == Char)    CODE_INST(i2c);
            else if (t_dst == Double)  CODE_INST(i2d);
            else if (t_dst == Float)   CODE_INST(i2f);
            else if (t_dst == Long)    CODE_INST(i2l);
            else if (t_dst == Short)   CODE_INST(i2s);
            else execerror("Bad conversion from %s to %s",
                    t_src->name, t_dst->name);
        } else if   (t_src == Long) {
            if      (t_dst == Char)    CODE_INST(l2c);
            else if (t_dst == Double)  CODE_INST(l2d);
            else if (t_dst == Float)   CODE_INST(l2f);
            else if (t_dst == Integer) CODE_INST(l2i);
            else if (t_dst == Short)   CODE_INST(l2s);
            else execerror("Bad conversion from %s to %s",
                    t_src->name, t_dst->name);
        } else if   (t_src == Short) {
            if      (t_dst == Char)    CODE_INST(s2c);
            else if (t_dst == Double)  CODE_INST(s2d);
            else if (t_dst == Float)   CODE_INST(s2f);
            else if (t_dst == Integer) CODE_INST(s2i);
            else if (t_dst == Long)    CODE_INST(s2l);
            else execerror("Bad conversion from %s to %s",
                    t_src->name, t_dst->name);
        } else execerror("Bad conversion from %s to %s",
                    t_src->name, t_dst->name);
    }
    return needs_to_change;
} /* code_conv_val */

OpRel code_unpatched_op(token tok)
{
    PT(">>> begin inserting UNPATCHED CODE @ [%04lx]\n",
          progp - prog);
    OpRel ret_val = {
        .tok   = tok,
        .start = CODE_INST(noop),
    };
    PT("<<< end   inserting UNPATCHED CODE\n");
    return ret_val;
} /* code_unpatched_op */

const Symbol *check_op_bin(const Expr *exp1, OpRel *op, const Expr *exp2)
{
    assert(exp1->typ->t2i->flags & (TYPE_IS_INTEGER | TYPE_IS_FLOATING_POINT));
    assert(exp2->typ->t2i->flags & (TYPE_IS_INTEGER | TYPE_IS_FLOATING_POINT));

    if (exp1->typ->t2i->weight == exp2->typ->t2i->weight)
        return exp1->typ;

    if (exp1->typ->t2i->weight > exp2->typ->t2i->weight) { /* greater */
        code_conv_val(exp2->typ, exp1->typ);
        return exp1->typ;
    }

    /* less */
    PT(">>> begin PATCHING CODE @ [%04lx]\n", op->start - prog);
    Cell *saved_progp = progp;
    progp             = op->start;
    code_conv_val(exp1->typ, exp2->typ);
    progp             = saved_progp;
    PT("<<< end   PATCHING CODE\n");
    return exp2->typ;

} /* check_op_bin */

void patch_block(Cell*patch_point)
{
    if (size_lvars != 0) {
        /* PARCHEO DE CODIGO */
        Cell *saved_progp = progp;
        progp             = patch_point;
        PT(">>> BEGIN PATCHING CODE @ [%04lx]\n", patch_point - prog);
        CODE_INST(spadd, -size_lvars);
        PT("<<< END   PATCHING CODE @ [%04lx]\n", patch_point - prog);
        progp = saved_progp;
        CODE_INST(spadd, size_lvars);
        size_lvars = 0;
    }
} /* patch_block */

void add_patch_return(Symbol *subr, Cell *patch_point)
{
    DYNARRAY_GROW(
            subr->returns_to_patch,
            Cell *,
            1,
            UQ_RETURNS_TO_PATCH_INCRMNT);
    subr->returns_to_patch[subr->returns_to_patch_len++]
            = patch_point;
} /* add_patch_return */

void patch_returns(const Symbol *subr, Cell *target)
{
    PT("<<< SAVING PROGRAMMING POINT @ [%04lx]\n", progp - prog);
    Cell *saved_progp = progp; /* salvamos el puntero de programacion */

    /* para cada punto a parchear */
    for (int i = 0; i < subr->returns_to_patch_len; ++i) {
        Cell *point_to_patch = subr->returns_to_patch[i];
        progp                = point_to_patch;
        PT(">>> BEGIN PATCHING CODE @ [%04lx]\n", point_to_patch - prog);
        CODE_INST(Goto, target);
        PT("<<< END   PATCHING CODE @ [%04lx]\n", point_to_patch - prog);
    }
    PT("<<< RESTORING PROGRAMMING POINT @ [%04lx]\n", saved_progp - prog);
    progp = saved_progp;
} /* patch_returns */

void yyerror(char *s)   /* called for yacc syntax error */
{

    int i = 0;
    const token *last = get_last_token(i++),
                *tok  = NULL;
    for (   ;  (i < UQ_LAST_TOKENS_SZ)
            && (tok = get_last_token(i))
            && (tok->lin == last->lin)
            ; i++)
        continue;

    printf(CYAN "%5d" WHITE ":" CYAN "%3d" WHITE ": "BRIGHT GREEN "%s\n" ANSI_END,
            last->lin, last->col, s);

    printf(CYAN "%5d" WHITE ":" CYAN "%3d" WHITE ": " GREEN,
            last->lin, last->col);

    int col = 1;
    for (i--; i > 0; i--) {
        tok = get_last_token(i);
        printf("%*s%s",
            tok->col - col, "", tok->lex);
        col = tok->col + tok->len;
    }
    printf(BRIGHT RED "%*s%s"ANSI_END"\n",
        last->col - col, "", last->lex);

    //execerror("SALIENDO DEL INTERPRETE");
    //longjmp(begin, 1);
} /* yyerror */
