%{
/* hoc.y -- programa para implementar una calculadora.
 * Esta version no tiene precedencia de operadores.
 * Author: Edward Rivas <rivastkw@gmail.com>
 *       y Luis Colorado <luiscoloradourcola@gmail.com>
 * Date: Mon Dec 30 14:06:56 -05 2024
 * Copyright: (c) 2025 Edward Rivas y Luis Colorado.  All rights reserved.
 * License: BSD.
 */

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
#include "builtins.h"

void warning( const char *fmt, ...);
void vwarning( const char *fmt, va_list args );
void yyerror( char * msg );

static void patch_block(Cell*patch_point);
static void add_patch_return(Symbol *subr, Cell *patch_point);
static void patch_returns(const Symbol *subr, Cell *target);
static OpRel code_unpatchedop(token op);
static const Symbol *check_op_bin(const Expr *exp1, OpRel *op, const Expr *exp2);
static bool code_conv_val(const Symbol *t_src, const Symbol *t_dst);
static void patching_subr(const Symbol *subr, Cell *preamb, const char *what);
static ConstArglist const_arglist_add(
        ConstArglist  list,
        const Symbol *bltin,
        ConstExpr     const_expr);
static Cell
const_conv_val(
        const Symbol *t_src,
        const Symbol *t_dst,
        Cell          orig);

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

#ifndef   UQ_CONST_EXPR_INCRMNT /* { */
#warning  UQ_CONST_EXPR_INCRMNT deberia ser configurado en config.mk
#define   UQ_CONST_EXPR_INCRMNT    (4)
#endif /* UQ_CONST_EXPR_INCRMNT    } */

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

#define BEGIN_UNPATCHED_CODE()          \
        do {                            \
            PT(CYAN ">>>" ANSI_END      \
                " begin inserting "     \
                MAGENTA                 \
                "UNPATCHED" ANSI_END    \
                " code @ [%04lx]\n",    \
                progp - prog);          \
            do {} while(0)

#define END_UNPATCHED_CODE()            \
            PT(CYAN "<<<" ANSI_END      \
                " end   inserting "     \
                MAGENTA                 \
                "UNPATCHED" ANSI_END    \
                " code\n");             \
        } while (0)

#define BEGIN_PATCHING_CODE(_addr)      \
        do {                            \
            PT(CYAN ">>>" ANSI_END      \
                " begin inserting "     \
                GREEN                   \
                " PATCHED " ANSI_END    \
                " code @ [%04lx]\n",    \
                _addr - prog);          \
            Cell *saved_progp = progp;  \
            progp = _addr;              \
            do {} while(0)

#define CHANGE_PATCHING_TO(_addr)       \
        do {                            \
            PT(CYAN "###" ANSI_END      \
                "   change to new "     \
                YELLOW                  \
                "PATCHING" ANSI_END     \
                " point "               \
                "@ [%04lx]\n",          \
                _addr - prog);          \
            progp = _addr;              \
        } while (0)

#define END_PATCHING_CODE()             \
            PT(CYAN "<<<" ANSI_END      \
                " end   inserting "     \
                GREEN                   \
                " PATCHED " ANSI_END    \
                " code\n");             \
            progp = saved_progp;        \
        } while (0)

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
    Cell         *cel;  /* referencia a Cell */
    Cell          lit;  /* literal */
    unsigned long num;  /* valor entero, para $<num> */
    const char   *str;  /* cadena de caracteres */
    var_decl_list vdl;  /* global var declaration list */
    var_init      vi;   /* global var name & initializer */
    Expr          expr; /* tipo con un puntero a Cell y una referencia a un tipo. */
    ConstExpr     const_expr; /* tipo de una expresion constante */
    ConstArglist  const_arglist; /* constant expression argument lists for builtins */
    token         tok;  /* tipo asociado a un operador, con todo el token */
    OpRel         opr;  /* tipo del operador relacional. */
}

%token        ERROR
%token <lit>  DOUBLE FLOAT
%token <sym>  VAR LVAR BLTIN_FUNC BLTIN_PROC CONSTANT
%token <sym>  FUNCTION PROCEDURE
%token        PRINT WHILE IF ELSE SYMBS SYMBS_ALL BRKPT CONST
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
%type  <cel>  stmt cond stmtlist
%type  <expr> expr expr_or expr_and expr_bitor expr_bitand expr_bitxor expr_shift
%type  <expr> expr_rel expr_arit term fact prim expr_and_left expr_or_left
%type  <cel>  mark
%type  <cel>  expr_seq item do else and or preamb create_scope
%type  <num>  arglist_opt arglist formal_arglist_opt formal_arglist
%type  <sym>  proc_head func_head lvar_definable_ident function procedure builtin_proc builtin_func const_definable_ident
%type  <str>  lvar_valid_ident gvar_valid_ident const_valid_ident
%type  <vdl>  gvar_decl_list gvar_decl lvar_decl_list lvar_decl
%type  <vi>   gvar_init lvar_init
%type  <tok>  '+' '-' '*' '/' '%' '<' '>' op_assign bitop_assign
%type  <tok>  '&' '|' '^' '~' const_op_mul const_op_sum const_op_rel
%type  <opr>  op_rel op_sum op_mul op_exp binop_bitor binop_bitxor binop_bitand binop_shift
%type  <const_expr> const_prim const_fact const_term const_expr_arit const_expr_rel
%type  <const_expr> const_expr_shift const_expr_bitand const_expr_bitxor const_expr_bitor
%type  <const_expr> const_expr_and const_expr
%type  <const_arglist> const_arglist const_arglist_opt

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
    :             ';'      { $$ = progp; } /* null statement */
    | expr        ';'      { $$ = $1.cel;
                             CODE_INST(drop); }
    | RETURN      ';'      { defnonly((indef != NULL) && (indef->type == PROCEDURE),
                                      "return;");
                             BEGIN_UNPATCHED_CODE();
                                 $$ = CODE_INST(Goto, 0);
                             END_UNPATCHED_CODE();
                             add_patch_return(indef, $$);
                           }
    | RETURN expr ';'      { defnonly((indef != NULL) && (indef->type == FUNCTION),
                                      "return <expr>;");
                             $$ = $2.cel;
                             /* asigno a la direccion de retorno de la funcion, en la
                              * cima de la lista de parametros */
                             code_conv_val($2.typ, indef->typref);
                             CODE_INST_TYP(
                                       indef->typref,
                                       argassign,
                                       indef->ret_val_offset,
                                       "{RET_VAL} ");
                             CODE_INST(drop);

                             BEGIN_UNPATCHED_CODE();
                                 Cell *p = CODE_INST(Goto, prog);
                                 add_patch_return(indef, p);
                             END_UNPATCHED_CODE();
                           }
    | PRINT expr_seq ';'   { $$ = $2; }
    | SYMBS          ';'   { $$ = CODE_INST(symbs); }
    | SYMBS_ALL      ';'   { $$ = CODE_INST(symbs_all, get_current_symbol()); }
    | BRKPT          ';'   { $$ = CODE_INST(brkpt, get_current_symbol()); }
    | LIST           ';'   { $$ = CODE_INST(list); }
    | const_decl     ';'   { $$ = progp; }
    | WHILE cond do stmt   { $$ = $2;
                             CODE_INST(Goto, $2);
                             BEGIN_PATCHING_CODE($3);
                                 CODE_INST(if_f_goto, saved_progp);
                             END_PATCHING_CODE(); }

    | IF cond do stmt      { $$ = $2;
                             BEGIN_PATCHING_CODE($3);
                                 CODE_INST(if_f_goto, saved_progp);
                             END_PATCHING_CODE();
                           }

    | IF cond do stmt else stmt {
                             $$ = $2;
                             BEGIN_PATCHING_CODE($3);
                                 CODE_INST(if_f_goto, $6);
                                 CHANGE_PATCHING_TO($5);
                                 CODE_INST(Goto, saved_progp);
                             END_PATCHING_CODE();
                           }

    | builtin_proc mark '(' arglist_opt ')' ';' {
                             $$ = $2;
                             if ($4 != $1->argums_len) {
                                 execerror(" " BRIGHT GREEN "%s"
                                           ANSI_END " accepts "
                                           "%d arguments, passed %d",
                                           $1->name, $1->argums_len, $4);
                             }
                             CODE_INST(bltin, $1->bltin_index);
                             if ($1->type == BLTIN_FUNC) {
                                 CODE_INST(drop);
                             }
                             pop_sub_call_stack(); }

    | procedure mark '(' arglist_opt ')' ';' {
                             $$ = $2;
                             if ($4 != $1->argums_len) {
                                 execerror(" " BRIGHT GREEN "%s"
                                           ANSI_END " accepts "
                                           "%d arguments, passed %d",
                                           $1->name, $1->argums_len, $4);
                             }
                             CODE_INST(call, $1);             /* instruction */
                             if ($1->size_args) {
                                 CODE_INST(spadd, $1->size_args); /* pop arguments */
                             }
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

builtin_proc
    : BLTIN_PROC           { push_sub_call_stack($1); }
    ;

procedure
    : PROCEDURE            { push_sub_call_stack($1); }
    ;

create_scope
    :  /* empty */         { scope *sc = start_scope();
                             if (get_root_scope() == sc) {
                                 BEGIN_UNPATCHED_CODE();
                                     $$ = CODE_INST(noop);
                                 END_UNPATCHED_CODE();
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
    : gvar_decl_list ',' gvar_init  {

/* LCU: Fri Oct 10 13:35:32 EEST 2025
 * LBL: ea69df38_a5c4_11f0_a46c_0023ae68f329
 * local/global variable registration and
 * coding of initialization code on
 * initialization expressions. This serves
 * here, and below, in the places marqued
 * REF: ... */
#define DO_VAR_REGISTRATION(        /* { */  \
        _type_decl,                          \
        _f_to_call,                          \
        _name,                               \
        _type_expr_init,                     \
        _start,                              \
        ...)                                 \
    do {                                     \
        Symbol *var = _f_to_call(            \
                _name, _type_decl);          \
        if (_start) {                        \
            code_conv_val(                   \
                    _type_expr_init,         \
                    _type_decl);             \
            CODE_INST_TYP(_type_decl,        \
                    ##__VA_ARGS__);          \
            CODE_INST(drop);                 \
        }                                    \
    } while (0) /* DO_VAR_REGISTRATION } */

                                      $$ = $1;
                                      if ($$.start == NULL && $3.start != NULL) {
                                          $$.start = $3.start;
                                      }

                                      /* see macro definition in
                                       * REF: ea69df38_a5c4_11f0_a46c_0023ae68f329
                                       * above */
                                      DO_VAR_REGISTRATION(
                                            $$.type_decl,
                                            register_global_var,
                                            $3.name,
                                            $3.type_expr_init,
                                            $3.start,
                                            assign,
                                            var);
                                    }
    | TYPE gvar_init                { $$.type_decl = $1;
                                      $$.start     = $2.start;

                                      /* see macro definition in
                                       * REF: ea69df38_a5c4_11f0_a46c_0023ae68f329
                                       * above */
                                      DO_VAR_REGISTRATION(
                                            $$.type_decl,
                                            register_global_var,
                                            $2.name,
                                            $2.type_expr_init,
                                            $2.start,
                                            assign,
                                            var);
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
    : lvar_decl_list ',' lvar_init  {
                                      $$ = $1;
                                      if ($$.start == NULL && $3.start != NULL) {
                                          $$.start = $3.start;
                                      }

                                      /* see macro definition in
                                       * REF: ea69df38_a5c4_11f0_a46c_0023ae68f329
                                       * above */
                                      DO_VAR_REGISTRATION(
                                            $$.type_decl,
                                            register_local_var,
                                            $3.name,
                                            $3.type_expr_init,
                                            $3.start,
                                            argassign,
                                            var->offset,
                                            var->name);
                                    }

    | TYPE lvar_init                { $$.type_decl = $1;
                                      $$.start     = $2.start;

                                      /* see macro definition in
                                       * REF: ea69df38_a5c4_11f0_a46c_0023ae68f329
                                       * above */
                                      DO_VAR_REGISTRATION(
                                            $$.type_decl,
                                            register_local_var,
                                            $2.name,
                                            $2.type_expr_init,
                                            $2.start,
                                            argassign,
                                            var->offset,
                                            var->name);
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

/* DECLARACION DE CONSTANTES LOCALES/GLOBALES { */

const_decl : const_decl_list
    ;

const_decl_list
    : const_decl_list ',' const_init
    | CONST const_init
    ;

const_init
    : const_valid_ident '=' const_expr {
                                      register_const($1, $3.typ, $3.cel);
                                    }
    ;

const_valid_ident
    : UNDEF
    | const_definable_ident { $$ = $1->name; }
    ;

const_definable_ident
    : VAR
    | LVAR
    | CONSTANT
    ;

/* LCU: Fri Oct 31 13:29:27 -05 2025
 * TODO: voy por aqui  } */

do  :  /* empty */         {
                             BEGIN_UNPATCHED_CODE();
                                 $$ = CODE_INST(if_f_goto, prog);
                             END_UNPATCHED_CODE();
                           }
    ;

else:  ELSE                {
                             BEGIN_UNPATCHED_CODE();
                                 $$ = CODE_INST(Goto, prog);
                             END_UNPATCHED_CODE();
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

/* LCU: Fri Oct 10 13:58:51 EEST 2025
 * LBL: 2d1da078_a5c8_11f0_9c7c_0023ae68f329
 * macro to encode converting non-Integer types
 * to an Integer boolean result */
#define TOBOOL(_type) /*      { */           \
    do {                                     \
        if (_type != Integer) {              \
            CODE_INST_TYP(_type, constpush,  \
                          _type->t2i->zero); \
            CODE_INST_TYP(_type, ne);        \
        }                                    \
    } while (0) /* TOBOOL    } */

                             /* see macro definition in
                              * REF: 2d1da078_a5c8_11f0_9c7c_0023ae68f329
                              */
                             TOBOOL($2.typ);
                           }
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
    : VAR    '=' expr      {

#define VAR_ASSIGN_EXPR(_exp_type, _var_type, ...) /* { */ \
        do {                                           \
            code_conv_val(_exp_type, _var_type);       \
            CODE_INST_TYP(_var_type, ##__VA_ARGS__);   \
        } while (0) /* VAR_ASSIGN_EXPR                } */

                             $$.cel = $3.cel;
                             $$.typ = $1->typref;

                             VAR_ASSIGN_EXPR(
                                     $3.typ,
                                     $1->typref,
                                     assign,
                                     $1);
                           }
    | LVAR   '=' expr      {
                             $$.cel = $3.cel;
                             $$.typ = $1->typref;

                             VAR_ASSIGN_EXPR(
                                     $3.typ,
                                     $1->typref,
                                     argassign,
                                     $1->offset,
                                     $1->name);
                           }

    | VAR   op_assign expr {

/* LCU: Wed Oct  1 14:44:15 -05 2025
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
 * LBL: 3fe7aaea_a5c9_11f0_b8f1_0023ae68f329
 */
#define VAR_OPASSIGN_EXPR(_eval, _assign, ...) /* { */                          \
        do {                                                                    \
            const Symbol *var       = $1,                                       \
                         *var_type  = var->typref,                              \
                         *exp_type  = $3.typ,                                   \
                         *res_type  = var_type;                                 \
            const token   op        = $2;                                       \
                                                                                \
            code_conv_val(exp_type, var_type); /* convert expr to var type */   \
            CODE_INST_TYP(var_type, _eval, ##__VA_ARGS__); /* variable evaluation */ \
            CODE_INST(swap);                     /* swap them */                \
            switch (op.id) {                                                    \
            case PLS_EQ:          CODE_INST_TYP(res_type, add,     var); break; \
            case MIN_EQ:          CODE_INST_TYP(res_type, sub,     var); break; \
            case MUL_EQ:          CODE_INST_TYP(res_type, mul,     var); break; \
            case DIV_EQ:          CODE_INST_TYP(res_type, divi,    var); break; \
            case MOD_EQ:          CODE_INST_TYP(res_type, mod,     var); break; \
            case PWR_EQ:          CODE_INST_TYP(res_type, pwr,     var); break; \
            case SHIFT_LEFT_EQ:   CODE_INST_TYP(res_type, bit_shl, var); break; \
            case SHIFT_RIGHT_EQ:  CODE_INST_TYP(res_type, bit_shr, var); break; \
            case BIT_OR_EQ:       CODE_INST_TYP(res_type, bit_or,  var); break; \
            case BIT_XOR_EQ:      CODE_INST_TYP(res_type, bit_xor, var); break; \
            case BIT_AND_EQ:      CODE_INST_TYP(res_type, bit_and, var); break; \
            } /* switch */                                                      \
            CODE_INST_TYP(var_type, _assign, ##__VA_ARGS__); /* assign to variable */ \
        } while (0) /* VAR_OPASSIGN_EXPR          } */

                             $$.cel = $3.cel;
                             $$.typ = $1->typref;

                             VAR_OPASSIGN_EXPR(
                                     eval,
                                     assign,
                                     var);
                           }

    | LVAR op_assign expr  {
                             $$.cel = $3.cel;
                             $$.typ = $1->typref;

                             /* LCU: Wed Oct  1 14:51:06 -05 2025
                              * see REF 3fe7aaea_a5c9_11f0_b8f1_0023ae68f329
                              * above, for a definition */
                             VAR_OPASSIGN_EXPR(
                                     argeval,
                                     argassign,
                                     var->offset,
                                     var->name);
                           }
    | VAR bitop_assign expr {

/* LCU: Fri Oct 10 14:13:36 EEST 2025
 * LBL: 37dac12e_a5ca_11f0_9c26_0023ae68f329
 * macro to forbid an operation when floating point data is
 * given to an operator */
#define VAR_NOT_FLOATING_POINT(_var)       /* { */          \
        do {                                                \
            if (   _var->typref->t2i->flags                 \
                   & TYPE_IS_FLOATING_POINT)                \
            {                                               \
                execerror("cannot %s operate to "           \
                       BRIGHT GREEN "%s" ANSI_END           \
                       ": must be integral type (is %s)\n", \
                       $2.lex, $1->name, $1->typref->name); \
            }                                               \
        }while (0) /* VAR_NOT_FLOATING_POINT  } */

                             /* LCU: Fri Oct 10 14:15:44 EEST 2025
                              * See 37dac12e_a5ca_11f0_9c26_0023ae68f329
                              * for a definition of this macro */
                             VAR_NOT_FLOATING_POINT($1);
                             VAR_OPASSIGN_EXPR(
                                     eval,
                                     assign,
                                     var);
                           }
    | LVAR bitop_assign expr {
                             /* LCU: Fri Oct 10 14:15:44 EEST 2025
                              * See 37dac12e_a5ca_11f0_9c26_0023ae68f329
                              * for a definition of this macro */
                             VAR_NOT_FLOATING_POINT($1);
                             VAR_OPASSIGN_EXPR(
                                     argeval,
                                     argassign,
                                     var->offset,
                                     var->name);
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
    : expr_or_left or expr_or  {

/* LCU: Fri Oct 10 14:19:02 EEST 2025
 * Macro to insert to-patch code for a boolean operator.
 * The macro inserts THREE noop instructions to allow
 * space for a 'constpush' zero and 'eq' evaluation (if needed),
 * and then, an 'and_then' or 'or_else' instruction.  It
 * is used in boolean left operators to binary '&&' and
 * '||' operators.
 * LBL: f121b1fa_a5cb_11f0_9912_0023ae68f329
 */
#define BOOLEAN_OP(_inst)       /* { */               \
        do {                                          \
            $$.cel = $1.cel;                          \
            $$.typ = Integer;                         \
            TOBOOL($3.typ);                           \
                                                      \
            BEGIN_PATCHING_CODE($2);                  \
                CODE_INST(_inst, saved_progp);        \
            END_PATCHING_CODE();                      \
        } while (0)  /* BOOLEAN_OP } */

                             /* See definition of this macro in
                              * REF: f121b1fa_a5cb_11f0_9912_0023ae68f329
                              * above.
                              */
                             BOOLEAN_OP(or_else);
                           }
    | expr_and
    ;

const_expr
    : const_expr_and OR const_expr {
                              /* LCU: Sun Nov  2 11:41:18 -05 2025
                               * TODO: voy por aqui. */
                              $$ = const_eval_op_bin($1, $2, $3);
                            }
    | const_expr_and
    ;

expr_or_left
    : expr_and             { TOBOOL($1.typ); }
    ;

or  : OR                   {

/* LCU: Fri Oct 10 14:35:42 EEST 2025
 * Macro to insert to-patch code for a boolean operator.
 * The macro inserts THREE noop instructions to allow
 * space for a 'constpush' zero and 'eq' evaluation (if needed),
 * and then, an 'and_then' or 'or_else' instruction.  It
 * is used in boolean left operators to binary '&&' and
 * '||' operators.
 * LBL: 5c908960_a5cd_11f0_a7a8_0023ae68f329
 */
#define INSERT_PATCHABLE_NOOP() /* { */                       \
        do {                                                  \
            BEGIN_UNPATCHED_CODE();                           \
            $$ = CODE_INST(noop); /* para luego 'and_then' */ \
            END_UNPATCHED_CODE();                             \
        } while (0)  /* INSERT_PATCHABLE_NOOP } */

                             /* See definition of this macro in
                              * REF 5c908960_a5cd_11f0_a7a8_0023ae68f329
                              * above */
                             INSERT_PATCHABLE_NOOP();
                           }
    ;

expr_and
    : expr_and_left and expr_and {
                             /* See definition of this macro in
                              * REF: f121b1fa_a5cb_11f0_9912_0023ae68f329
                              * above.
                              */
                             BOOLEAN_OP(and_then);
                           }
    | expr_bitor
    ;

expr_and_left
    : expr_bitor           { TOBOOL($1.typ); }
    ;

and : AND                  {
                             /* See definition of this macro in
                              * REF 5c908960_a5cd_11f0_a7a8_0023ae68f329
                              * above */
                             INSERT_PATCHABLE_NOOP();
                           }
    ;

const_expr_and
    : const_expr_bitor AND const_expr_and {
                              /* LCU: Sun Nov  2 11:41:18 -05 2025
                               * TODO: voy por aqui. */
                              $$ = const_eval_op_bin($1, $2, $3);
                            }
    | const_expr_bitor
    ;

expr_bitor
    : expr_bitor binop_bitor expr_bitxor {

#define BITOP()               /* { */              \
        do {                                       \
            const Symbol *left_type  = $1.typ,     \
                         *right_type = $3.typ;     \
                                                   \
            if (     left_type->t2i->flags         \
                          & TYPE_IS_FLOATING_POINT \
                  || right_type->t2i->flags        \
                          & TYPE_IS_FLOATING_POINT)\
            {                                      \
                execerror("%s %s %s not "          \
                          "allowed, must be "      \
                          "integral",              \
                          left_type->name,         \
                          $2.tok.lex,              \
                          right_type->name);       \
            }                                      \
                                                   \
            $$.cel = $1.cel;                       \
            $$.typ = check_op_bin(&$1, &$2, &$3);  \
        } while (0)  /* BITOP    }  */

                              BITOP();
                              CODE_INST_TYP($$.typ, bit_or);
                            }
    | expr_bitxor
    ;

binop_bitor
    : '|'                   { $$ = code_unpatched_op($1); }
    ;

const_expr_bitor
    : const_expr_bitor '|' const_expr_bitxor {
                              /* LCU: Sun Nov  2 11:41:18 -05 2025
                               * TODO: voy por aqui. */
                              $$ = const_eval_op_bin($1, $2, $3);
                            }
    | const_expr_bitxor
    ;


expr_bitxor
    : expr_bitxor binop_bitxor expr_bitand {
                              BITOP();
                              CODE_INST_TYP($$.typ, bit_xor);
                            }
    | expr_bitand
    ;

const_expr_bitxor
    : const_expr_bitxor '^' const_expr_bitand {
                              /* LCU: Sun Nov  2 11:41:18 -05 2025
                               * TODO: voy por aqui. */
                              $$ = const_eval_op_bin($1, $2, $3);
                            }
    | const_expr_bitand
    ;

binop_bitxor
    : '^'                   { $$ = code_unpatched_op($1); }
    ;

expr_bitand
    : expr_bitand binop_bitand expr_shift {
                              BITOP();
                              CODE_INST_TYP($$.typ, bit_and);
                            }
    | expr_shift
    ;

const_expr_bitand
    : const_expr_bitand '&' const_expr_shift {
                              /* LCU: Sun Nov  2 11:41:18 -05 2025
                               * TODO: voy por aqui. */
                              $$ = const_eval_op_bin($1, $2, $3);
                            }
    | const_expr_shift
    ;

binop_bitand
    : '&'                   { $$ = code_unpatched_op($1); }
    ;


expr_shift
    : expr_shift binop_shift expr_rel {
                              BITOP();
                              switch ($2.tok.id) {
                              case SHIFT_LEFT:   CODE_INST_TYP($$.typ, bit_shl); break;
                              case SHIFT_RIGHT:  CODE_INST_TYP($$.typ, bit_shr); break;
                              } /* switch */
                            }
    | expr_rel
    ;

const_expr_shift
    : const_expr_shift SHIFT_LEFT  const_expr_rel {
                              /* LCU: Sun Nov  2 11:41:18 -05 2025
                               * TODO: voy por aqui. */
                              $$ = const_eval_op_bin($1, $2, $3);
                            }
    | const_expr_shift SHIFT_RIGHT const_expr_rel {
                              /* LCU: Sun Nov  2 11:41:18 -05 2025
                               * TODO: voy por aqui. */
                              $$ = const_eval_op_bin($1, $2, $3);
                            }
    | const_expr_rel
    ;

binop_shift
    : SHIFT_LEFT            { $$ = code_unpatched_op($1); }
    | SHIFT_RIGHT           { $$ = code_unpatched_op($1); }
    ;

expr_rel
    : expr_arit op_rel expr_arit {
                              $$.typ = Integer;
                              $$.cel = $1.cel;
                              const Symbol *type = check_op_bin(&$1, &$2, &$3);
                              switch ($2.tok.id) {
                                  case '<':  CODE_INST_TYP(type, lt); break;
                                  case '>':  CODE_INST_TYP(type, gt); break;
                                  case  EQ:  CODE_INST_TYP(type, eq); break;
                                  case  NE:  CODE_INST_TYP(type, ne); break;
                                  case  GE:  CODE_INST_TYP(type, ge); break;
                                  case  LE:  CODE_INST_TYP(type, le); break;
                              } /* switch */
                            }
    | expr_arit
    ;

const_expr_rel
    : const_expr_rel const_op_rel const_expr_arit {
                              /* LCU: Sun Nov  2 11:41:18 -05 2025
                               * TODO: voy por aqui. */
                              $$ = const_eval_op_bin($1, $2, $3);
                            }
    | const_expr_arit
    ;

op_rel
    : '<'                   { $$ = code_unpatched_op($1); }
    | '>'                   { $$ = code_unpatched_op($1); }
    | EQ                    { $$ = code_unpatched_op($1); }
    | NE                    { $$ = code_unpatched_op($1); }
    | GE                    { $$ = code_unpatched_op($1); }
    | LE                    { $$ = code_unpatched_op($1); }
    ;

const_op_rel
    : '<'
    | '>'
    | EQ
    | NE
    | GE
    | LE
    ;

expr_arit
    : expr_arit op_sum term { /* LCU: Mon Sep 15 12:31:40 -05 2025
                               * LBL: 9966c546_a5cf_11f0_b8f9_0023ae68f329
                               * determinar el tipo de la expr_arit, a partir de los
                               * tipos de $1 y $3. En caso de que los tipos sean
                               * distintos, generar codigo para convertir el tipo de
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

const_expr_arit
    : const_expr_arit const_op_sum const_term {
                              /* LCU: Sun Nov  2 11:41:18 -05 2025
                               * TODO: voy por aqui. */
                              $$ = const_eval_op_bin($1, $2, $3);
                            }
    | const_term
    ;

op_sum
    : '+'                   { $$ = code_unpatched_op($1); }
    | '-'                   { $$ = code_unpatched_op($1); }
    ;

const_op_sum
    : '+'
    | '-'
    ;

term
    : term op_mul fact      {
                              /* LCU: Mon Sep 15 12:36:54 -05 2025
                               * ver 9966c546_a5cf_11f0_b8f9_0023ae68f329,
                               * arriba. */
                              $$.typ = check_op_bin(&$1, &$2, &$3);
                              switch($2.tok.id) {
                              case '*': CODE_INST_TYP($$.typ, mul);  break;
                              case '/': CODE_INST_TYP($$.typ, divi); break;
                              case '%': BITOP();
                                        CODE_INST_TYP($$.typ, mod);  break;
                              }
                            }
    | fact
    ;

const_term
    : const_term const_op_mul const_fact {
                              $$ = const_eval_op_bin($1, $2, $3);
                            }
    | const_fact
    ;

op_mul
    : '*'                   { $$ = code_unpatched_op($1); }
    | '/'                   { $$ = code_unpatched_op($1); }
    | '%'                   { $$ = code_unpatched_op($1); }
    ;

const_op_mul
    : '*'
    | '/'
    | '%'
    ;

fact: prim op_exp fact      {
                              /* LCU: Mon Sep 15 12:36:54 -05 2025
                               * ver 9966c546_a5cf_11f0_b8f9_0023ae68f329,
                               * arriba. */
                              $$.typ = check_op_bin(&$1, &$2, &$3);
                              CODE_INST_TYP($$.typ, pwr);
                            }
    | prim
    ;

const_fact
    : const_prim EXP const_fact {
                              $$ = const_eval_op_bin($1, $2, $3);
#if 0 /* { */
                              if (       ($1.typ->t2i->flags & TYPE_IS_INTEGER)
                                      && ($3.typ->t2i->flags & TYPE_IS_INTEGER))
                              {
                                  /* convert base to long */
                                  $1.cel = const_conv_val($1.typ, Long, $1.cel);
                                  /* convert exponent to int */
                                  $3.cel = const_conv_val($3.typ, Integer, $3.cel);
                                  $$.typ = Long;
                                  $$.cel.lng = fast_pwr_l($1.cel.lng, $3.cel.itg);
                              } else if (   ($1.typ->t2i->flags & TYPE_IS_FLOATING_POINT)
                                         && ($3.typ->t2i->flags & TYPE_IS_FLOATING_POINT))
                              {
                                  /* convert base to double */
                                  $1.cel = const_conv_val($1.typ, Double, $1.cel);
                                  /* convert exponent to double */
                                  $1.cel = const_conv_val($3.typ, Double, $3.cel);
                                  $$.typ = Double;
                                  $$.cel.dbl = pow($1.cel.dbl, $3.cel.dbl);
                              } else {
                                  execerror("arguments (%s & %s) must be integers or "
                                          "floating point, but not mixed or other type",
                                          $1.typ->name, $3.typ->name);
                              }
#endif /* } */
                            }
    | const_prim
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
                              $$.typ = Float; }
    | DOUBLE                { $$.cel = CODE_INST_TYP(Double,  constpush, $1);
                              $$.typ = Double; }
    | CHAR                  { $$.cel = CODE_INST_TYP(Char,    constpush, $1);
                              $$.typ = Char; }
    | SHORT                 { $$.cel = CODE_INST_TYP(Short, constpush, $1);
                              $$.typ = Short; }
    | INTEGER               { $$.cel = CODE_INST_TYP(Integer, constpush, $1);
                              $$.typ = Integer; }
    | LONG                  { $$.cel = CODE_INST_TYP(Long, constpush, $1);
                              $$.typ = Long; }
    | VAR                   { $$.cel = CODE_INST_TYP($1->typref, eval,    $1);
                              $$.typ = $1->typref; }
    | LVAR                  { $$.cel = CODE_INST_TYP($1->typref, argeval,
                                                     $1->offset, $1->name);
                              $$.typ = $1->typref; }
    | '!' prim              { $$.cel = $2.cel;
                              $$.typ = Integer;
                              TOBOOL($2.typ);
                              CODE_INST(not);
                            }
    | '~' prim              { if ($2.typ->t2i->flags
                                      & TYPE_IS_FLOATING_POINT)
                              {
                                  execerror("%s type cannot be used with "
                                            "operator %s, use a cast",
                                            $2.typ->name, $1.lex);
                              }
                              $$.cel = $2.cel;
                              $$.typ = $2.typ;
                              CODE_INST_TYP($2.typ, bit_not);
                            }
    | '+' prim              { $$ = $2; }
    | '-' prim              { $$ = $2;
                              CODE_INST_TYP($2.typ, neg);  }

    | PLS_PLS VAR           {

/* LCU: Fri Oct 10 15:04:03 EEST 2025
 * LBL: 6a8b4aa6_a5d1_11f0_a355_0023ae68f329
 * Next macro codes the pre increment/decrement operators on
 * global/local variables. */
#define INCDEC_PRE(_eval, _op, _assign, ...) /*  { */ \
        do {                                    \
            const Symbol                        \
                   *var  = $2,                  \
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
        } while (0) /* INCDEC_PRE                } */

                              /* See definition of this macro in
                               * REF: 6a8b4aa6_a5d1_11f0_a355_0023ae68f329
                               * avobe.  */
                              INCDEC_PRE(eval,    add, assign,    var);
                            }
    | MIN_MIN VAR           {
                              /* See definition of this macro in
                               * REF: 6a8b4aa6_a5d1_11f0_a355_0023ae68f329
                               * avobe.  */
                              INCDEC_PRE(eval,    sub, assign,    var); }
    | PLS_PLS LVAR          {
                              /* See definition of this macro in
                               * REF: 6a8b4aa6_a5d1_11f0_a355_0023ae68f329
                               * avobe.  */
                              INCDEC_PRE(argeval, add, argassign, var->offset, var->name); }
    | MIN_MIN LVAR          {
                              /* See definition of this macro in
                               * REF: 6a8b4aa6_a5d1_11f0_a355_0023ae68f329
                               * avobe.  */
                              INCDEC_PRE(argeval, sub, argassign, var->offset, var->name); }

    | VAR     PLS_PLS       {

/* LCU: Fri Oct 10 15:12:47 EEST 2025
 * LBL: 814c1152_a5d2_11f0_bf28_0023ae68f329
 * Next macro codes the post increment/decrement operators on
 * global/local variables. */
#define INCDEC_POST(_eval, _op, _assign, ...) /* { */ \
        do {                               \
            const Symbol                   \
                   *var  = $1,             \
                   *type = var->typref;    \
            $$.cel = progp;                \
            $$.typ = type;                 \
                                           \
            CODE_INST_TYP(type, _eval,     \
                          ##__VA_ARGS__);  \
            CODE_INST(dupl);               \
            CODE_INST_TYP(type,            \
                          constpush,       \
                          type->t2i->one); \
            CODE_INST_TYP(type, _op);      \
            CODE_INST_TYP(type, _assign,   \
                          ##__VA_ARGS__);  \
            CODE_INST(drop);               \
        } while (0) /* INCDEC_POST               } */

                              /* See definition of this macro in
                               * REF: 814c1152_a5d2_11f0_bf28_0023ae68f329
                               * avobe.  */
                              INCDEC_POST(eval,    add, assign,    var);
                            }

    | VAR     MIN_MIN       { /* See definition of this macro in
                               * REF: 814c1152_a5d2_11f0_bf28_0023ae68f329
                               * avobe.  */
                              INCDEC_POST(eval,    sub, assign,    var); }

    | LVAR    PLS_PLS       { /* See definition of this macro in
                               * REF: 814c1152_a5d2_11f0_bf28_0023ae68f329
                               * avobe.  */
                              INCDEC_POST(argeval, add, argassign,
                                          var->offset, var->name); }

    | LVAR    MIN_MIN       { /* See definition of this macro in
                               * REF: 814c1152_a5d2_11f0_bf28_0023ae68f329
                               * avobe.  */
                              INCDEC_POST(argeval, sub, argassign,
                                          var->offset, var->name); }

    | CONSTANT              { $$.cel = CODE_INST_TYP($1->typref,
                                                     constpush,
                                                     $1->cel);
                              $$.typ = $1->typref;
                            }

    | builtin_func mark '(' arglist_opt ')' {
                              $$.cel = $2;
                              $$.typ = $1->typref;
                              if ($4 != $1->argums_len) {
                                  execerror(" " BRIGHT GREEN "%s"
                                            ANSI_END " accepts "
                                            "%d arguments, passed %d",
                                            $1->name, $1->argums_len, $4);
                              }
                              CODE_INST(bltin, $1->bltin_index);
                              pop_sub_call_stack();
                            }

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
                              CODE_INST(spadd, $1->size_args); /* eliminando argumentos */
                              pop_sub_call_stack();
                            }
    ;

const_prim
    : '(' TYPE ')' const_prim
                            { $$.cel = const_conv_val($4.typ, $2, $4.cel);
                              $$.typ = $2; }
    | '(' const_expr ')'    { $$     = $2; }
    | FLOAT                 { $$.cel = $1;
                              $$.typ = Float; }
    | DOUBLE                { $$.cel = $1;
                              $$.typ = Double; }
    | CHAR                  { $$.cel = $1;
                              $$.typ = Char; }
    | SHORT                 { $$.cel = $1;
                              $$.typ = Short; }
    | INTEGER               { $$.cel = $1;
                              $$.typ = Integer; }
    | LONG                  { $$.cel = $1;
                              $$.typ = Long; }
    | '!' const_prim        { $$.typ = Integer;
                              if        ($$.typ == Char) {
                                  $$.cel.itg = ! $2.cel.chr;
                              } else if ($$.typ == Short) {
                                  $$.cel.itg = ! $2.cel.sht;
                              } else if ($$.typ == Integer) {
                                  $$.cel.itg = ! $2.cel.itg;
                              } else if ($$.typ == Long) {
                                  $$.cel.itg = ! $2.cel.lng;
                              } else if ($$.typ == Float) {
                                  $$.cel.itg = ! $2.cel.flt;
                              } else if ($$.typ == Double) {
                                  $$.cel.itg = ! $2.cel.dbl;
                              } else {
                                  execerror(GREEN "%s" ANSI_END " invalid",
                                          $2.typ->name);
                              }
                            }
    | '~' const_prim        { $$ = $2;
                              if        ($$.typ == Char) {
                                  $$.cel.chr = ~ $2.cel.chr;
                              } else if ($$.typ == Short) {
                                  $$.cel.sht = ~ $2.cel.sht;
                              } else if ($$.typ == Integer) {
                                  $$.cel.itg = ~ $2.cel.itg;
                              } else if ($$.typ == Long) {
                                  $$.cel.lng = ~ $2.cel.lng;
                              } else {
                                  execerror(GREEN "%s" ANSI_END " invalid",
                                          $2.typ->name);
                              }
                            }
    | '+' const_prim        { $$     = $2; }
    | '-' const_prim        { $$     = $2;
                              if        ($$.typ == Char) {
                                  $$.cel.chr = - $2.cel.chr;
                              } else if ($$.typ == Short) {
                                  $$.cel.sht = - $2.cel.sht;
                              } else if ($$.typ == Integer) {
                                  $$.cel.itg = - $2.cel.itg;
                              } else if ($$.typ == Long) {
                                  $$.cel.lng = - $2.cel.lng;
                              } else if ($$.typ == Float) {
                                  $$.cel.flt = - $2.cel.flt;
                              } else if ($$.typ == Double) {
                                  $$.cel.dbl = - $2.cel.dbl;
                              } else {
                                  execerror(GREEN "%s" ANSI_END " invalid",
                                          $2.typ->name);
                              }
                            }
    | CONSTANT              { $$.typ = $1->typref;
                              $$.cel = $1->cel;
                            }
    | builtin_func '(' const_arglist ')' {
                              $$ = eval_const_builtin_func($1->bltin_index, &$3);
                              pop_sub_call_stack();
                            }
    | builtin_func '('  ')' {
                              const ConstArglist empty = {
                                  .expr_list     = NULL,
                                  .expr_list_len = 0,
                                  .expr_list_cap = 0,
                              };
                              $$ = eval_const_builtin_func($1->bltin_index, &empty);
                              pop_sub_call_stack();
                            }
    ;

const_arglist
    : const_arglist ',' const_expr {
                              $$ = const_arglist_add($1, top_sub_call_stack(), $3);
                            }
    | const_expr            { $$.expr_list     = NULL;
                              $$.expr_list_len = $$.expr_list_cap
                                               = 0;
                              $$ = const_arglist_add($$, top_sub_call_stack(), $1);
                            }
    ;

builtin_func
    : BLTIN_FUNC            { push_sub_call_stack($1); }
    ;

function
    : FUNCTION              { push_sub_call_stack($1);

                              /* PUSH espacio para el valor a retornar */
                              CODE_INST(spadd, -$1->typref->t2i->size);
                            }
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
                              BEGIN_UNPATCHED_CODE();
                                  /* LCU: Mon Sep  8 13:17:49 EEST 2025
                                   * we need to return this pointer (not
                                   * a pointer to the beginning of the code)
                                   * because it is what we need to patch. */
                                  $$ = CODE_INST(noop);
                              END_UNPATCHED_CODE();
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
                                  indef->ret_val_offset = indef->size_args
                                                        + UQ_SIZE_FP_RETADDR;
                                  PT("+++ RET_VAL offset = %d\n",
                                            indef->ret_val_offset);
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
                              $$ = register_subr($2, PROCEDURE, NULL, progp);
                              $$->main_scope = start_scope();
                              P("DEFINIENDO EL PROCEDIMIENTO '%s' @ [%04lx]\n",
                                        $2, progp - prog);
                              indef = $$;
                            }
    ;

func_head
    : FUNC TYPE UNDEF       {
                              $$ = register_subr($3, FUNCTION, $2, progp);
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

    int scope_size = cs->base_offset + cs->size;
    if (scope_size > size_lvars) {
      size_lvars = scope_size;
      PT("*** UPDATING size_lvars TO %zd\n", size_lvars);
    }
    patch_returns(subr, progp); /* parcheamos todos los RETURN del
                                 * block */
    patch_block(preamb);        /* parcheamos el spadd, 0 de preamb */

    /* CODIGO A INSERTAR PARA TERMINAR (POSTAMBULO) */
    CODE_INST(pop_fp);
    CODE_INST(ret);
    end_scope();
    end_register_subr(subr);
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

static Cell
const_conv_val(
        const Symbol *t_src,
        const Symbol *t_dst,
        Cell orig)
{
    if (t_src != t_dst) {
        if          (t_src == Char) {
            if      (t_dst == Double)  orig.dbl = orig.chr;
            else if (t_dst == Float)   orig.flt = orig.chr;
            else if (t_dst == Integer) orig.itg = orig.chr;
            else if (t_dst == Long)    orig.lng = orig.chr;
            else if (t_dst == Short)   orig.sht = orig.chr;
            else execerror("Bad conversion from %s to %s",
                    t_src->name, t_dst->name);
        } else if   (t_src == Double) {
            if      (t_dst == Char)    orig.chr = orig.dbl;
            else if (t_dst == Float)   orig.flt = orig.dbl;
            else if (t_dst == Integer) orig.itg = orig.dbl;
            else if (t_dst == Long)    orig.lng = orig.dbl;
            else if (t_dst == Short)   orig.sht = orig.dbl;
            else execerror("Bad conversion from %s to %s",
                    t_src->name, t_dst->name);
        } else if   (t_src == Float) {
            if      (t_dst == Char)    orig.chr = orig.flt;
            else if (t_dst == Double)  orig.dbl = orig.flt;
            else if (t_dst == Integer) orig.itg = orig.flt;
            else if (t_dst == Long)    orig.lng = orig.flt;
            else if (t_dst == Short)   orig.sht = orig.flt;
            else execerror("Bad conversion from %s to %s",
                    t_src->name, t_dst->name);
        } else if   (t_src == Integer) {
            if      (t_dst == Char)    orig.chr = orig.itg;
            else if (t_dst == Double)  orig.dbl = orig.itg;
            else if (t_dst == Float)   orig.flt = orig.itg;
            else if (t_dst == Long)    orig.lng = orig.itg;
            else if (t_dst == Short)   orig.sht = orig.itg;
            else execerror("Bad conversion from %s to %s",
                    t_src->name, t_dst->name);
        } else if   (t_src == Long) {
            if      (t_dst == Char)    orig.chr = orig.lng;
            else if (t_dst == Double)  orig.dbl = orig.lng;
            else if (t_dst == Float)   orig.flt = orig.lng;
            else if (t_dst == Integer) orig.itg = orig.lng;
            else if (t_dst == Short)   orig.sht = orig.lng;
            else execerror("Bad conversion from %s to %s",
                    t_src->name, t_dst->name);
        } else if   (t_src == Short) {
            if      (t_dst == Char)    orig.chr = orig.sht;
            else if (t_dst == Double)  orig.dbl = orig.sht;
            else if (t_dst == Float)   orig.flt = orig.sht;
            else if (t_dst == Integer) orig.itg = orig.sht;
            else if (t_dst == Long)    orig.lng = orig.sht;
            else execerror("Bad conversion from %s to %s",
                    t_src->name, t_dst->name);
        } else execerror("Bad conversion from %s to %s",
                    t_src->name, t_dst->name);
    } /* if */
    return orig;
} /* const_conv_val */

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
    OpRel ret_val = { .tok = tok, .start = progp };

    BEGIN_UNPATCHED_CODE();
        CODE_INST(noop);
    END_UNPATCHED_CODE();

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
    BEGIN_PATCHING_CODE(op->start);
        code_conv_val(exp1->typ, exp2->typ);
    END_PATCHING_CODE();

    return exp2->typ;

} /* check_op_bin */

ConstExpr
const_eval_op_bin(ConstExpr exp1, token op, ConstExpr exp2)
{
    assert(exp1.typ->t2i->flags & (TYPE_IS_INTEGER | TYPE_IS_FLOATING_POINT));
    assert(exp2.typ->t2i->flags & (TYPE_IS_INTEGER | TYPE_IS_FLOATING_POINT));

    const Symbol *typ_res = exp1.typ;
    if (exp1.typ->t2i->weight > exp2.typ->t2i->weight) {
        exp2.cel = const_conv_val(exp2.typ, exp1.typ, exp2.cel);
    } else
    if (exp1.typ->t2i->weight < exp2.typ->t2i->weight) {
        typ_res = exp2.typ;
        exp1.cel = const_conv_val(exp1.typ, exp2.typ, exp1.cel);
    }

    const type2inst *t2i = typ_res->t2i; /* tipo de los operandos */
    /* LCU: Thu Nov  6 15:55:39 -05 2025
     * TODO: voy por aqui.  Resolver el problema del tipo del resultado
     * segun el operador */

    switch (op.id) {
    case OR:          return t2i->     or_binop(Integer, exp1, exp2);
    case AND:         return t2i->    and_binop(Integer, exp1, exp2);
    case '|':         return t2i->  bitor_binop(typ_res, exp1, exp2);
    case '^':         return t2i-> bitxor_binop(typ_res, exp1, exp2);
    case '&':         return t2i-> bitand_binop(typ_res, exp1, exp2);
    case SHIFT_LEFT:  return t2i->    shl_binop(typ_res, exp1, exp2);
    case SHIFT_RIGHT: return t2i->    shr_binop(typ_res, exp1, exp2);
    case '<':         return t2i->     lt_binop(Integer, exp1, exp2);
    case '>':         return t2i->     gt_binop(Integer, exp1, exp2);
    case EQ:          return t2i->     eq_binop(Integer, exp1, exp2);
    case GE:          return t2i->     ge_binop(Integer, exp1, exp2);
    case LE:          return t2i->     le_binop(Integer, exp1, exp2);
    case NE:          return t2i->     ne_binop(Integer, exp1, exp2);
    case '+':         return t2i->   plus_binop(typ_res, exp1, exp2);
    case '-':         return t2i->  minus_binop(typ_res, exp1, exp2);
    case '*':         return t2i->   mult_binop(typ_res, exp1, exp2);
    case '/':         return t2i->   divi_binop(typ_res, exp1, exp2);
    case '%':         return t2i->    mod_binop(typ_res, exp1, exp2);
    case EXP:         return t2i->    exp_binop(typ_res, exp1, exp2);
    } /* switch */

    execerror("No operator selected: %s(%d)", op.lex, op.id);
    /* NOTREACHED */
    ConstExpr ret_val = { .typ = NULL };
    return ret_val;

} /* const_eval_op_bin */

static ConstArglist
const_arglist_add(
		ConstArglist  list,
		const Symbol *bltin,
		ConstExpr     const_expr)
{
    int i = list.expr_list_len;  /* argument position */
    if (i >= bltin->argums_len) {
        execerror("passing more arguments than bltin '%s' "
                "requires (%d)\n",
                bltin->name, bltin->argums_len);
    }
    const Symbol *expr_type     = const_expr.typ,
                 *bltin_arg     = bltin->argums[i],
                 *arg_type      = bltin_arg->typref;
    const char   *bltin_argname = bltin_arg->name,
                 *bltin_name    = bltin->name,
                 *arg_typename  = arg_type->name,
                 *expr_typename = expr_type->name;

    if (expr_type != arg_type) {
        printf("bltin '%s': arg #%d(%s)'s "
               "type(%s) != expr's type (%s) converting to (%s)\n",
               bltin_name, i, bltin_argname,
               arg_typename, expr_typename,
               arg_typename);
        const_expr.cel = const_conv_val(
                expr_type,
                arg_type,
                const_expr.cel);
        const_expr.typ = arg_type;
    }

    DYNARRAY_GROW(
            list.expr_list,
            ConstExpr, 1,
            UQ_CONST_EXPR_INCRMNT);
    list.expr_list[list.expr_list_len++] = const_expr;

    return list;
} /* const_arglist_add */

void patch_block(Cell*patch_point)
{
    if (size_lvars != 0) {
        BEGIN_PATCHING_CODE(patch_point);
            CODE_INST(spadd, -size_lvars);
        END_PATCHING_CODE();
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
    BEGIN_PATCHING_CODE(progp);
        /* para cada punto a parchear */
        for (int i = 0; i < subr->returns_to_patch_len; ++i) {
            Cell *point_to_patch = subr->returns_to_patch[i];
            CHANGE_PATCHING_TO(point_to_patch);
            CODE_INST(Goto, target);
        }
    END_PATCHING_CODE();
} /* patch_returns */

void yyerror(char *s)   /* called for yacc syntax error */
{
    int          i    = 0;
    const token *last = get_last_token(i++),
                *tok  = NULL;

    for (   ;  (i < UQ_LAST_TOKENS_SZ)
            && (tok = get_last_token(i))
            && (tok->lin == last->lin)
            ; i++)
        continue;

    printf(CYAN "%5d" WHITE ":" CYAN "%3d" WHITE ": "
           BRIGHT GREEN "%s\n" ANSI_END,
           last->lin, last->col, s);

    printf(CYAN "%5d" WHITE ":" CYAN "%3d" WHITE ": " GREEN,
            last->lin, last->col);

    int col = 1;
    for (--i; i > 0; --i) {
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
