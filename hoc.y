/* hoc-sin-prec.y -- programa para implementar una calculadora.
 * Esta version no tiene precedencia de operadores.
 * Author: Edward Rivas <rivastkw@gmail.com>
 *       y Luis Colorado <luiscoloradourcola@gmail.com>
 * Date: Mon Dec 30 14:06:56 -05 2024
 * Copyright: (c) 2025 Edward Rivas y Luis Colorado.  All rights reserved.
 * License: BSD.
 */
%{
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#include "config.h"
#include "colors.h"
#include "hoc.h"
#include "lex.h"
#include "error.h"
#include "math.h"   /*  Modulo personalizado con nuevas funciones */
#include "instr.h"
#include "code.h"

void warning( const char *fmt, ...);
void vwarning(const char *fmt, va_list args);
void yyerror( char *);

/*  Necersario para hacer setjmp y longjmp */
jmp_buf begin;

#ifndef  UQ_HOC_DEBUG
#warning UQ_HOC_DEBUG deberia ser configurado en config.mk
#define  UQ_HOC_DEBUG 1
#endif

#ifndef  UQ_HOC_TRACE_PATCHING
#warning UQ_HOC_TRACE_PATCHING deberia ser configurado en config.mk
#define  UQ_HOC_TRACE_PATCHING 1
#endif

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

int indef_proc,  /* 1 si estamos en una definicion de procedimiento */
    indef_func;  /* 1 si estamos en una definicion de funcion */

%}
/* continuamos el area de definicion y configuracion
 * de yacc */

/*  Declaracion tipos de datos de los objetos
    (TOKENS, SYMBOLOS no terminales)  */
%union {
    const instr *inst; /* instruccion maquina */
    Symbol      *sym;  /* puntero a simbolo */
    double       val;  /* valor double */
    Cell        *cel;  /* referencia a Cell */
    int          num;  /* valor entero, para $<num> */
    char        *str;  /* cadena de caracteres */
}

%token ERROR
%token <val> NUMBER
%token <sym> VAR BLTIN0 BLTIN1 BLTIN2 UNDEF CONST
%token       PRINT WHILE IF ELSE SYMBS
%token       OR AND GE LE EQ NE EXP
%token       PLS_PLS MIN_MIN PLS_EQ MIN_EQ MUL_EQ DIV_EQ MOD_EQ PWR_EQ
%token <num> ARG
%token <num> FUNC PROC
%token       RETURN READ
%token <sym> FUNCTION PROCEDURE
%token <str> STRING
%token       LIST
%type  <cel> stmt cond stmtlist asig
%type  <cel> expr_or expr_and expr_rel expr term fact prim mark
%type  <cel> expr_seq item do else and or
%type  <num> arglist_opt arglist

%%
/*  Area de definicion de reglas gramaticales */

list: /* nothing */
    | list       '\n'
    | list defn  '\n'
    | list stmt  '\n'  { CODE_INST(STOP);  /* para que execute() pare al final */
                         return 1; }
    | list asig  '\n'  {
                         Symbol *prev = lookup("prev");
                         if (!prev->defn) {
                            register_global_var(prev);
                         }
                         CODE_INST(assign, prev);
                         CODE_INST(print);
                         CODE_INST(STOP);  /* para que execute() pare al final */
                         return 1; }
    | list error '\n' { /* yyerrok; */
                         return 1; }
    ;

final: '\n' | ';';      /* Regla para evaular si el caracter es '\n' ó ';'  */

stmt: asig ';'             { CODE_INST(drop); }
    | RETURN      ';'      { defnonly(indef_proc, "return;");
                             $$ = CODE_INST(procret); }
    | RETURN asig ';'      { defnonly(indef_func, "return <asig>;");
                             $$ = $2;
                             CODE_INST(funcret); }
    | PRINT expr_seq ';'   { $$ = $2; }
    | SYMBS       ';'      { $$ = CODE_INST(symbs); }
    | LIST        ';'      { $$ = CODE_INST(list); }
    | WHILE cond do stmt   { CODE_INST(Goto, $2);
                             Cell *saved_progp = progp;
                             progp = $3;
                             PT(">>> patching CODE @ [%04lx]\n", progp - prog);
                                 CODE_INST(if_f_goto, saved_progp);
                             PT("<<< end patching CODE @ [%04lx], "
                                "continuing @ [%04lx]\n",
                                 progp - prog, saved_progp - prog);
                             progp = saved_progp; }
    | IF    cond do stmt   { Cell *saved_progp = progp;
                             progp = $3;
                             PT(">>> patching CODE @ [%04lx]\n", progp - prog);
                                 CODE_INST(if_f_goto, saved_progp);
                             PT("<<< end patching CODE @ [%04lx], continuing @ [%04lx]\n",
                                 progp - prog, saved_progp - prog);
                             progp = saved_progp;
                           }
    | IF    cond do stmt else stmt {
                             Cell *saved_progp = progp;
                             progp = $3;
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
    | '{' stmtlist '}'     { $$ = $2; }
    | PROCEDURE mark '(' arglist_opt ')' ';' {
                             $$ = $2;
                             CODE_INST(call, $1, $4); /* instruction */
                           }
    ;

do  :  /* empty */         { $$ = progp;
                             PT(">>> inserting unpatched CODE @ [%04lx]\n",
                                     progp - prog);
                                 CODE_INST(if_f_goto, prog);
                             PT("<<< end inserting unpatched CODE @ [%04lx]\n",
                                     progp - prog); }
    ;

else:  ELSE                { $$ = progp;
                             PT(">>> inserting unpatched CODE @ [%04lx]\n",
                                     progp - prog);
                                 CODE_INST(if_f_goto, prog);
                             PT("<<< end inserting unpatched CODE @ [%04lx]\n",
                                     progp - prog); }
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
    ;

asig: VAR   '=' asig       { if ($1->type != VAR && $1->type != UNDEF) {
                                 execerror("Symbol '%s' is not a variable\n",
                                           $1->name);
                             }
                             if ($1->type == UNDEF && $1->defn == NULL) { /* indefinida y no registrada */
                                    register_global_var($1);  /* esto asigna defn */
                             }
                             $$ = $3;
                             CODE_INST(assign, $1); }
    | VAR  PLS_EQ asig    { $$ = CODE_INST(addvar, $1); }
    | VAR  MIN_EQ asig    { $$ = CODE_INST(subvar, $1); }
    | VAR  MUL_EQ asig    { $$ = CODE_INST(mulvar, $1); }
    | VAR  DIV_EQ asig    { $$ = CODE_INST(divvar, $1); }
    | VAR  MOD_EQ asig    { $$ = CODE_INST(modvar, $1); }
    | VAR  PWR_EQ asig    { $$ = CODE_INST(pwrvar, $1); }

    | ARG  PLS_EQ asig    { $$ = CODE_INST(addarg, $1); }
    | ARG  MIN_EQ asig    { $$ = CODE_INST(subarg, $1); }
    | ARG  MUL_EQ asig    { $$ = CODE_INST(mularg, $1); }
    | ARG  DIV_EQ asig    { $$ = CODE_INST(divarg, $1); }
    | ARG  MOD_EQ asig    { $$ = CODE_INST(modarg, $1); }
    | ARG  PWR_EQ asig    { $$ = CODE_INST(pwrarg, $1); }
    | ARG   '=' asig       {
                             defnonly(indef_proc || indef_func, "$%d assign", $1);
                             $$ = $3;
                             CODE_INST(argassign, $1);
                           }
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

and : AND                   { PT(">>> begin inserting unpatched CODE @ [%04lx]\n",
                                      progp - prog);
                              $$ = CODE_INST(and_then, prog);
                              PT("<<< end   inserting unpatched CODE\n"); }
    ;

expr_rel
        : '!' expr_rel      { $$ = $2;
                              CODE_INST(not); }
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
    | VAR                   { $$ = CODE_INST(eval, $1); }
    | PLS_PLS VAR           { $$ = CODE_INST(inceval, $2); }
    | VAR PLS_PLS           { $$ = CODE_INST(evalinc, $1); }
    | MIN_MIN VAR           { $$ = CODE_INST(deceval, $2); }
    | VAR MIN_MIN           { $$ = CODE_INST(evaldec, $1); }
    | PLS_PLS ARG           { $$ = CODE_INST(incarg, $2); }
    | ARG PLS_PLS           { $$ = CODE_INST(arginc, $1); }
    | MIN_MIN ARG           { $$ = CODE_INST(decarg, $2); }
    | ARG MIN_MIN           { $$ = CODE_INST(argdec, $1); }
    | ARG                   { defnonly(indef_proc || indef_func,
                                       "$%d assign", $1);
                              $$ = CODE_INST(argeval, $1);
                            }
    | CONST                 { $$ = CODE_INST(constpush, $1->val); }
    | BLTIN0 '(' ')'        { $$ = CODE_INST(bltin0, $1); }
    | BLTIN1 '(' asig ')'   { $$ = $3;
                              CODE_INST(bltin1, $1); }
    | BLTIN2 '(' asig ',' asig ')'
                            { $$ = $3;
                              CODE_INST(bltin2, $1); }
    | READ '(' VAR ')'      { $$ = CODE_INST(readopcode, $3); }
    | FUNCTION mark '(' arglist_opt ')' {
                              $$ = $2;
                              CODE_INST(call, $1, $4); /* instruction */
                            }
    ;

arglist_opt
    : arglist
    | /* empty */           { $$ = 0; }
    ;

arglist
    : arglist ',' asig      { $$ = $1 + 1; }
    | asig                  { $$ = 1; }
    ;

defn: proc_head '(' ')' stmt {
                              CODE_INST(procret);
                              end_define();
                              indef_proc = 0;
                              P("FIN DEFINICION PROCEDIMIENTO\n");
                             }
    | func_head '(' ')' stmt {
                              CODE_INST(constpush, 0.0);
                              CODE_INST(funcret);
                              end_define();
                              indef_func = 0;
                              P("FIN DEFINICION FUNCION\n");
                            }
proc_head
    : PROC VAR              {
                              P("DEFINIENDO EL PROCEDIMIENTO '%s' @ [%04lx]\n",
                                $2->name, progp - prog);
                              define($2, PROCEDURE);
                              indef_proc = 1; }
func_head
    : FUNC VAR              {
                              P("DEFINIENDO LA FUNCION '%s' @ [%04lx]\n",
                                $2->name, progp - prog);
                              define($2, FUNCTION);
                              indef_func = 1; }
%%

void yyerror(char *s)   /* called for yacc syntax error */
{
    /* LCU: Wed Mar 19 15:40:42 -05 2025
     * TODO: manejar un buffer circular de tokens que permita
     * imprimir mejor el contexto donde ocurren los errores
     * de la gramatica. */
    //warning("%s", s);

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
    longjmp(begin, 1);
} /* yyerror */
