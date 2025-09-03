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
#include "math.h"   /*  Modulo personalizado con nuevas funciones */
#include "instr.h"
#include "code.h"

#include "symbol.h"
#include "cell.h"
#include "scope.h"

void warning( const char *fmt, ...);
void vwarning( const char *fmt, va_list args );
void yyerror( char * msg );

static void patch_block(Cell*patch_point);
static void patch_returns(Symbol *subr, Cell *target);

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
#define   UQ_ARGUMS_INCRMNT  8
#endif /* UQ_ARGUMS_INCRMNT    } */

#if       UQ_HOC_DEBUG /* {{ */
# define P(_fmt, ...)                     \
    printf("%s:%d:%s "_fmt,               \
            __FILE__, __LINE__, __func__, \
            ##__VA_ARGS__)
#else /*  UQ_HOC_DEBUG    }{ */
# define P(_fmt, ...)
#endif /* UQ_HOC_DEBUG    }} */

#if UQ_HOC_TRACE_PATCHING
# define PT(_fmt, ...) P(_fmt, ##__VA_ARGS__)
#else
# define PT(_fmt, ...)
#endif

#define CODE_INST(I, ...) code_inst(INST_##I, ##__VA_ARGS__)

Symbol *indef;  /* != NULL si estamos en una definicion de procedimiento/funcion */

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
    int           num;  /* valor entero, para $<num> */
    const char   *str;  /* cadena de caracteres */
    var_decl_list vdl; /* global var declaration list */
    var_init      vi;  /* global var name & initializer */
}

%token        ERROR
%token <val>  NUMBER
%token <sym>  VAR LVAR BLTIN0 BLTIN1 BLTIN2 CONST
%token <sym>  FUNCTION PROCEDURE
%token        PRINT WHILE IF ELSE SYMBS
%token        OR AND GE LE EQ NE EXP
%token        PLS_PLS MIN_MIN PLS_EQ MIN_EQ MUL_EQ DIV_EQ MOD_EQ PWR_EQ
%token <num>  FUNC PROC INTEGER
%token        RETURN
%token <str>  STRING UNDEF
%token        LIST
%token <sym>  TYPE
%type  <cel>  stmt cond stmtlist asig
%type  <cel>  expr_or expr_and expr_rel expr term fact prim mark
%type  <cel>  expr_seq item do else and or preamb create_scope
%type  <num>  arglist_opt arglist formal_arglist_opt formal_arglist
%type  <sym>  proc_head func_head lvar_definable_ident function
%type  <str>  lvar_valid_ident gvar_valid_ident
%type  <vdl>  gvar_decl_list lvar_decl_list
%type  <vi>   gvar_init lvar_init

%%
/*  Area de definicion de reglas gramaticales */

list: /* nothing */
    | list       '\n'

    | list defn  '\n'
    | list gvar_decl_list ';' '\n' {
                         CODE_INST(STOP);
                         return 1;
                       }

    | list stmt  '\n'  {
                         CODE_INST(STOP);  /* para que execute() pare al final */
                         return 1; }
    | list asig  '\n'  { Symbol *prev = lookup("prev");
                         CODE_INST(assign, prev);
                         CODE_INST(print);
                         CODE_INST(STOP);  /* para que execute() pare al final */
                         return 1; }
    | list error '\n' {  yyerrok;
                         CODE_INST(STOP);
                         while (get_current_scope()) {
                            end_scope();
                         }
                         return 1; }
    ;

stmt
    : asig        ';'      { CODE_INST(drop); }
    | RETURN      ';'      { defnonly((indef != NULL) && (indef->type == PROCEDURE), "return;");
                             PT(">>> inserting unpatched code @ [%04lx]\n", progp - prog);
                             $$ = CODE_INST(Goto, 0);
                             PT("<<< end inserting unpatched code\n");
                             add_patch_return(indef, $$);
                           }
    | RETURN asig ';'      { defnonly((indef != NULL) && (indef->type == FUNCTION), "return <asig>;");
                             $$ = $2;
                             /* asigno a la direccion de retorno de la funcion, en la
                              * cima de la lista de parametros */
                             CODE_INST(argassign, indef->size_args);
                             CODE_INST(drop);

                             PT(">>> inserting unpatched code @ [%04lx]\n", progp - prog);
                             Cell *p = CODE_INST(Goto, prog);
                             PT("<<< end inserting unpatched code\n");
                             add_patch_return(indef, p);
                           }
    | PRINT expr_seq ';'   { $$ = $2; }
    | SYMBS          ';'   { $$ = CODE_INST(symbs); }
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
                             Cell *saved_progp = progp;
                             progp = $3;
                             /* LCU: Mon Sep  1 15:26:34 -05 2025
                              * este codigo esta fallando, no se por que esta
                              * generando un parcheo incorrecto */
                             PT("*** $6 == %p\n", $6);
                             PT(">>> patching CODE @ [%04lx]\n", progp - prog);
                                 CODE_INST(if_f_goto, $6);
                             PT("<<< end patching CODE @ [%04lx], "
                                   "continuing @ [%04lx]\n",
                                   progp - prog, saved_progp - prog);
                             progp = $5;
                             PT(">>> patching CODE @ [%04lx]\n", progp - prog);
                                 CODE_INST(Goto, saved_progp);
                             PT("<<< end patching CODE @ [%04lx], "
                                   "continuing @ [%04lx]\n",
                                   progp - prog, saved_progp - prog);
                             progp = saved_progp;
                           }

    | PROCEDURE mark '(' arglist_opt ')' ';' {
                             $$ = $2;
                             CODE_INST(call, $1, $1->size_args); /* instruction */
                             CODE_INST(spadd, $1->size_args);    /* pop arguments */
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
gvar_decl_list
    : gvar_decl_list ',' gvar_init  { $$ = $1;
                                      if ($$.start == NULL && $3.start != NULL) {
                                        $$.start = $3.start;
                                      }
                                      Symbol *s = register_global_var($3.name, $$.typref);
                                      if ($3.start) {
                                          CODE_INST(assign, s);
                                          CODE_INST(drop);
                                      }
                                    }
    | TYPE gvar_init                { $$.typref = $1;
                                      $$.start  = $2.start ? $2.start : NULL;
                                      Symbol *s = register_global_var($2.name, $$.typref);
                                      if ($2.start) {
                                          CODE_INST(assign, s);
                                          CODE_INST(drop);
                                      }
                                    }
    ;

gvar_init
    : gvar_valid_ident              { $$.name = $1; $$.start = NULL; }
    | gvar_valid_ident '=' asig     { $$.name = $1; $$.start = $3; }
    ;

gvar_valid_ident
    : UNDEF
    ;

/* DECLARACION DE VARIABLES LOCALES */
lvar_decl_list
    : lvar_decl_list ',' lvar_init  { $$ = $1;
                                      if ($$.start == NULL && $3.start != NULL) {
                                          $$.start = $3.start;
                                      }
                                      Symbol *sym = register_local_var(
                                            $3.name, $$.typref);
                                      if ($3.start) {
                                          CODE_INST(argassign, sym->offset);
                                          CODE_INST(drop);
                                      }
                                    }

    | TYPE lvar_init                { $$.typref = $1;
                                      $$.start  = $2.start;
                                      Symbol *sym = register_local_var(
                                            $2.name, $$.typref);
                                      if ($2.start) {
                                          CODE_INST(argassign, sym->offset);
                                          CODE_INST(drop);
                                      }
                                    }
    ;

lvar_init
    : lvar_valid_ident          { $$.name = $1; $$.start = NULL; }
    | lvar_valid_ident '=' asig { $$.name = $1; $$.start = $3; }
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
    | asig                 { CODE_INST(prexpr); }
    ;

cond: '(' asig ')'         { $$ = $2; }
    ;

mark: /* nada */           { $$ = progp; }
    ;

stmtlist: /* nada */       { $$ = progp; }
    | stmtlist '\n'
    | stmtlist stmt
    | stmtlist lvar_decl_list ';'
    ;

asig:
      UNDEF  '=' asig      { execerror("Variable %s no esta declarada", $1); }
    | VAR    '=' asig      { $$ = $3; CODE_INST(assign, $1); }

    | LVAR   '=' asig      { $$ = $3; CODE_INST(argassign, $1->offset); }

    | VAR  PLS_EQ asig     { $$ = $3; CODE_INST(addvar, $1); }
    | VAR  MIN_EQ asig     { $$ = $3; CODE_INST(subvar, $1); }
    | VAR  MUL_EQ asig     { $$ = $3; CODE_INST(mulvar, $1); }
    | VAR  DIV_EQ asig     { $$ = $3; CODE_INST(divvar, $1); }
    | VAR  MOD_EQ asig     { $$ = $3; CODE_INST(modvar, $1); }
    | VAR  PWR_EQ asig     { $$ = $3; CODE_INST(pwrvar, $1); }

    | LVAR PLS_EQ asig     { $$ = $3; CODE_INST(addarg, $1->offset); }
    | LVAR MIN_EQ asig     { $$ = $3; CODE_INST(subarg, $1->offset); }
    | LVAR MUL_EQ asig     { $$ = $3; CODE_INST(mularg, $1->offset); }
    | LVAR DIV_EQ asig     { $$ = $3; CODE_INST(divarg, $1->offset); }
    | LVAR MOD_EQ asig     { $$ = $3; CODE_INST(modarg, $1->offset); }
    | LVAR PWR_EQ asig     { $$ = $3; CODE_INST(pwrarg, $1->offset); }

    | expr_or
    ;

expr_or
    : expr_and or expr_or  { Cell *saved_progp = progp;
                             progp = $2;
                             PT(">>> begin patching CODE @ [%04lx]\n",
                                   progp - prog);
                                 CODE_INST(or_else, saved_progp);
                             PT("<<< end   patching CODE @ [%04lx], "
                                   "continuing @ [%04lx]\n",
                                   progp - prog, saved_progp - prog);
                             progp = saved_progp; }
    | expr_and
    ;

or  : OR                   { PT(">>> begin inserting unpatched CODE @ [%04lx]\n",
                                 progp - prog);
                             $$ = CODE_INST(or_else, prog);
                             PT("<<< end   inserting unpatched CODE\n"); }
    ;

expr_and
    : expr_rel and expr_and { Cell *saved_progp = progp;
                              progp = $2;
                              PT(">>> begin patching CODE @ [%04lx]\n",
                                      progp - prog);
                              CODE_INST(and_then, saved_progp);
                              PT("<<< end   patching CODE @ [%04lx], "
                                      "continuing @ [%04lx]\n",
                                      progp - prog, saved_progp - prog);
                              progp = saved_progp; }
    | expr_rel
    ;

and : AND                   {
                              PT(">>> begin inserting unpatched CODE @ [%04lx]\n",
                                      progp - prog);
                              $$ = CODE_INST(and_then, prog);
                              PT("<<< end   inserting unpatched CODE\n");
                            }
    ;

expr_rel
    : '!' expr_rel      { $$ = $2; CODE_INST(not); }
    | expr '<' expr     { CODE_INST(lt); }
    | expr '>' expr     { CODE_INST(gt); }
    | expr EQ  expr     { CODE_INST(eq); }
    | expr NE  expr     { CODE_INST(ne); }
    | expr GE  expr     { CODE_INST(ge); }
    | expr LE  expr     { CODE_INST(le); }
    | expr
    ;

expr: term
    | '-' term              { $$ = $2; CODE_INST(neg);  }
    | '+' term              { $$ = $2; }
    | expr '+' term         { CODE_INST(add);  }
    | expr '-' term         { CODE_INST(sub);  }
    ;

term: fact
    | term '*' fact         { CODE_INST(mul);  }
    | term '/' fact         { CODE_INST(divi); }
    | term '%' fact         { CODE_INST(mod);  }
    ;

fact: prim EXP fact         { CODE_INST(pwr);  }
    | prim
    ;

prim: '(' asig ')'          { $$ = $2; }
    | NUMBER                { $$ = CODE_INST(constpush, $1); }
    | INTEGER               { $$ = CODE_INST(constpush, (double) $1); }

    | PLS_PLS VAR           { $$ = CODE_INST(inceval, $2); }
    | MIN_MIN VAR           { $$ = CODE_INST(deceval, $2); }
    | VAR     PLS_PLS       { $$ = CODE_INST(evalinc, $1); }
    | VAR     MIN_MIN       { $$ = CODE_INST(evaldec, $1); }
    | VAR                   { $$ = CODE_INST(eval,    $1); }

    | PLS_PLS LVAR          { $$ = CODE_INST(incarg,  $2->offset); }
    | MIN_MIN LVAR          { $$ = CODE_INST(decarg,  $2->offset); }
    | LVAR    PLS_PLS       { $$ = CODE_INST(arginc,  $1->offset); }
    | LVAR    MIN_MIN       { $$ = CODE_INST(argdec,  $1->offset); }
    | LVAR                  { $$ = CODE_INST(argeval, $1->offset); }

    | CONST                 { $$ = CODE_INST(constpush, $1->val); }
    | BLTIN0 '(' ')'        { $$ = CODE_INST(bltin0, $1); }
    | BLTIN1 '(' asig ')'   { $$ = $3;
                              CODE_INST(bltin1, $1); }
    | BLTIN2 '(' asig ',' asig ')'
                            { $$ = $3;
                              CODE_INST(bltin2, $1); }
    | function mark '(' arglist_opt ')' {
                              /* LCU: Tue Sep  2 00:41:28 -05 2025
                               * comprobar que los argumentos concuerdan
                               * con la lita de argumentos de la funcion */
                              $$ = $2;
                              CODE_INST(call, $1, $1->size_args); /* instruction */
                              CODE_INST(spadd, $1->size_args);    /* eliminando argumentos */
                            }
    ;

function
    : FUNCTION              { CODE_INST(spadd, -$1->typref->size); } /* PUSH espacio para el
                                                                      * valor a retornar */
    ;

arglist_opt
    : arglist
    | /* empty */           { $$ = 0; }
    ;

arglist
    : arglist ',' asig      { $$ = $1 + 1; }
    | asig                  { $$ = 1; }
    ;

defn: proc_head '(' formal_arglist_opt ')' preamb block {

                              /* PARCHEO DE CODIGO */
                              scope *cs = get_root_scope();
                              if (cs->base_offset + cs->size > size_lvars) {
                                  size_lvars = cs->base_offset + cs->size;
                                  PT("*** UPDATING size_lvars TO %zd\n", size_lvars);
                              }
                              patch_returns($1, progp); /* parcheamos todos los RETURN del
                                                         * block */
                              patch_block($5);          /* parcheamos el spadd, 0 de preamb */

                              /* CODIGO A INSERTAR PARA TERMINAR (POSTAMBULO) */
                              CODE_INST(ret);
                              end_scope();
                              end_define($1);
                              indef = NULL;
                              P("FIN DEFINICION PROCEDIMIENTO\n");
                            }

    | func_head '(' formal_arglist_opt ')' preamb block {

                              /* PARCHEO DE CODIGO */
                              scope *cs = get_root_scope();
                              if (cs->base_offset + cs->size > size_lvars) {
                                  size_lvars = cs->base_offset + cs->size;
                                  PT("*** UPDATING size_lvars TO %zd\n", size_lvars);
                              }
                              patch_returns($1, progp);
                              patch_block($5);

                              /* CODIGO A INSERTAR PARA TERMINAR (POSTAMBULO) */
                              CODE_INST(ret);
                              end_scope();
                              end_define($1);
                              indef = NULL;
                              P("FIN DEFINICION FUNCION\n");
                            }
    ;

preamb: /* empty */         {
                              PT(">>> begin UNPATCHED code @ [%04lx]\n", progp - prog);
                              $$ = CODE_INST(noop);
                              PT("<<< end   UNPATCHED code\n");
                            }
    ;

block
    : '{' stmtlist '}'
    ;

formal_arglist_opt
    : formal_arglist        {
                              PT("*** formal_arg_list_opt == %d\n", $1);
                              /* cambiando los offsets para que se refieran a las
                               * posiciones de las expresiones calculadas antes de
                               * entrar a la funcion (sumando a sus offsets la cantidad
                               * indef->size_args */
                              for (int i = 0; i < indef->argums_len; ++i) {
                                    indef->argums[i]->offset += indef->size_args;
                                    PT("+++ arg[%d] (%s), offset = %d\n",
                                        i,
                                        indef->argums[i]->name,
                                        indef->argums[i]->offset);
                              }
                              if (indef->type == FUNCTION) {
                                  PT("+++ RET_VAL offset = %d\n",
                                            indef->size_args);
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
                              subr->size_args += $3->size;
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
                              subr->size_args += $1->size;
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

void patch_returns(Symbol *subr, Cell *target)
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
