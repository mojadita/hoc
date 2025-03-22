/* hoc-sin-prec.y -- programa para implementar una calculadora.
 * Date: Mon Dec 30 14:06:56 -05 2024
 * Esta version no tiene precedencia de operadores.
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
#include "error.h"
#include "math.h"   /*  Modulo personalizado con nuevas funciones */
#include "code.h"

void warning( const char *fmt, ...);
void vwarning(const char *fmt, va_list args);
void yyerror( char *);

/*  Necersario para hacer setjmp y longjmp */
jmp_buf begin;

#ifndef UQ_HOC_DEBUG
#warning UQ_HOC_DEBUG deberia ser configurado en config.mk
#define UQ_HOC_DEBUG 1
#endif

#ifndef UQ_HOC_USE_PATCHING
#warning UQ_HOC_USE_PATCHING deberia ser configurado en config.mk
#define UQ_HOC_USE_PATCHING 1
#endif

#if UQ_HOC_DEBUG  /*   {{ */
#define P(_fmt, ...)                      \
    printf("%s:%d:%s "_fmt,               \
            __FILE__, __LINE__, __func__, \
            ##__VA_ARGS__)
#else /* UQ_HOC_DEBUG  }{ */
#define P(_fmt, ...)
#endif /* UQ_HOC_DEBUG }} */

#if UQ_HOC_USE_PATCHING
#define PT(_fmt, ...) P(_fmt, ##__VA_ARGS__)
#else
#define PT(_fmt, ...)
#endif

#define CODE_INST(F) code_inst(F,    CYAN #F ANSI_END)
#define CODE_STOP()  code_inst(STOP, YELLOW "STOP" ANSI_END)

int indef_proc,  /* 1 si estamos en una definicion de procedimiento */
    indef_func;  /* 1 si estamos en una definicion de funcion */

%}
/* continuamos el area de definicion y configuracion
 * de yacc */

/*  Declaracion tipos de datos de los objetos
    (TOKENS, SYMBOLOS no terminales)  */
%union {
    Inst    inst; /* instruccion maquina */
    Symbol *sym;  /* puntero a simbolo */
    double  val;  /* valor double */
    Cell   *cel;  /* referencia a Cell */
    int     num;  /* valor entero, para $<num> */
    char   *str;  /* cadena de caracteres */
}

/*
 * LCU: Tue Jan 21 10:58:10 EET 2025
 * definimos los mismos tokens aqui que en hoc-sin-prec.y, para
 * asegurar que se asignaran los mismos valores, y no habra
 * problemas al construir ambos ejecutables.
 */
%token ERROR
%token <val> NUMBER
%token <sym> VAR BLTIN0 BLTIN1 BLTIN2 UNDEF CONST
%token       PRINT WHILE IF ELSE SYMBS
%token       OR AND GE LE EQ NE
%token       UNARY
%token <num> ARG
%token <num> FUNC PROC
%token       RETURN READ
%token <sym> FUNCTION PROCEDURE
%token <str> STRING
%type  <cel> stmt cond stmtlist while if end asig
%type  <cel> expr_or expr_and expr_rel expr term fact prim mark
%type  <cel> expr_seq item
%type  <num> arglist_opt arglist

%%
/*  Area de definicion de reglas gramaticales */

list: /* nothing */
    | list       '\n'
    | list defn  '\n'
    | list stmt  '\n'  { CODE_STOP();  /* para que execute() pare al final */
                         return 1; }
    | list asig  '\n'  { CODE_INST(assign);
                         code_sym(lookup("prev"));
                         CODE_INST(print);
                         CODE_STOP();  /* para que execute() pare al final */
                         return 1; }
    | list error final { yyerrok;
                         return 1; }
    ;

final: '\n' | ';';      /* Regla para evaular si el caracter es '\n' ó ';'  */

stmt: asig ';'             { CODE_INST(drop); }
    | RETURN      ';'      { defnonly(indef_proc, "return;");
                             $$ = CODE_INST(procret); }
    | RETURN expr ';'      { defnonly(indef_func, "return <expr>;");
                             $$ = $2;
                             CODE_INST(funcret); }
    | PRINT expr_seq ';'   { $$ = $2; }
    | SYMBS ';'            { $$ = CODE_INST(list_symbols); }
    | while cond stmt end  {
                             Cell *saved_progp = progp;
                             progp = $1;
                             P(">>> patching CODE @ [%04lx]\n", progp - prog);
                             CODE_INST(whilecode);
                             code_cel($3);
                             code_cel($4);
                             P("<<< end patching CODE @ [%04lx], continuing @ [%04lx]\n",
                                 progp - prog, saved_progp - prog);
                             progp = saved_progp;
                           }
    | if    cond stmt end  {
                             Cell *saved_progp = progp;
                             progp = $1;
                             PT(">>> patching CODE @ [%04lx]\n", progp - prog);
                                 CODE_INST(ifcode);
                                 code_cel($3);
                                 code_cel(NULL);   /* no hay parte else, ver `ifcode` */
                                 code_cel($4);
                             PT("<<< end patching CODE @ [%04lx], continuing @ [%04lx]\n",
                                 progp - prog, saved_progp - prog);
                             progp = saved_progp;
                           }
    | if    cond stmt end ELSE stmt end {
                             Cell *saved_progp = progp;
                             progp = $1;
                             PT(">>> patching CODE @ [%04lx]\n", progp - prog);
                                 CODE_INST(ifcode);
                                 code_cel($3);
                                 code_cel($6);
                                 code_cel($7);
                             PT("<<< end patching CODE @ [%04lx], "
                                   "continuing @ [%04lx]\n",
                                   progp - prog, saved_progp - prog);
                             progp = saved_progp;
                           }
    | '{' stmtlist '}'     { $$ = $2; }
    | PROCEDURE mark '(' arglist_opt ')' ';' {
                             $$ = $2;
                             CODE_INST(call); /* instruction */
                             code_sym($1);    /* symbol associated to proc */
                             code_num($4);    /* number of arguments */ }
    ;

expr_seq
    : expr_seq ',' item
    | item
    ;

item: STRING               { $$ = CODE_INST(prstr);
                             code_str($1); }
    | asig                 { CODE_INST(prexpr); }
    ;

cond: '(' asig ')'         { CODE_STOP(); $$ = $2; }
    ;

while: WHILE               {
                             PT(">>> unpatched code @ [%04lx]\n", progp - prog);
                             $$ = CODE_INST(whilecode);
                                  code_cel(NULL);
                                  code_cel(NULL);
                             PT("<<< end of unpatched code\n");
                           }
    ;

if  : IF                   {
                             PT(">>> unpatched code @ [%04lx]\n", progp - prog);
                             $$ = CODE_INST(ifcode);
                                  code_cel(NULL);
                                  code_cel(NULL);
                                  code_cel(NULL);
                             PT("<<< end of unpatched code\n");
                           }
    ;

mark: /* nada */        { $$ = progp; }
    ;

end: /* nada */            { CODE_STOP(); $$ = progp; }
    ;

stmtlist: /* nada */       { $$ = progp; }
    | stmtlist '\n'
    | stmtlist stmt
    ;

asig: VAR   '=' asig       { if ($1->type != VAR && $1->type != UNDEF) {
                                 execerror("Symbol '%s' is not a variable\n",
                                           $1->name);
                             }
                             $$ = $3;
                             CODE_INST(assign);
                             code_sym($1); }
    | ARG   '=' asig       {
                             defnonly(indef_proc || indef_func, "$%d assign", $1);
                             $$ = $3;
                             CODE_INST(argassign);
                             code_num($1);
                           }
    | expr_or
    ;

expr_or
    : expr_or OR expr_and  { CODE_INST(or); }
    | expr_and
    ;

expr_and
    : expr_and AND expr_rel { CODE_INST(and); }
    | expr_rel
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

fact: prim '^' fact         { CODE_INST(pwr);  }
    | prim
    ;

prim: '(' asig ')'          { $$ = $2; }
    | NUMBER                { $$ = CODE_INST(constpush);
                                   code_val($1); }
    | VAR                   { $$ = CODE_INST(eval);
                                   code_sym($1); }
    | ARG                   { defnonly(indef_proc || indef_func,
                                       "$%d assign", $1);
                              $$ = CODE_INST(argeval);
                              code_num($1);
                            }
    | CONST                 { $$ = CODE_INST(eval);
                                   code_sym($1); }
    | BLTIN0 '(' ')'        { $$ = CODE_INST(bltin0);
                                   code_sym($1); }
    | BLTIN1 '(' asig ')'   { $$ = CODE_INST(bltin1);
                                   code_sym($1); }
    | BLTIN2 '(' asig ',' asig ')'
                            { $$ = CODE_INST(bltin2);
                                   code_sym($1); }
    | READ '(' VAR ')'      { $$ = CODE_INST(readopcode);
                                   code_sym($3); }
    | FUNCTION mark '(' arglist_opt ')' {
                              $$ = $2;
                              CODE_INST(call); /* instruction */
                              code_sym($1);    /* function symbol */
                              code_num($4);    /* number of arguments */ }
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
                              CODE_INST(constpush);
                              code_val(0.0);
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
    warning(" "BRIGHT GREEN "%s" ANSI_END, s);
} /* yyerror */
