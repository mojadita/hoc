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

#include "hoc.h"
#include "error.h"
#include "math.h"   /*  Modulo personalizado con nuevas funciones */
#include "code.h"

void warning(const char *fmt, ...);
void vwarning(const char *fmt, va_list args);
void yyerror(char *);

/*  Necersario para hacer setjmp y longjmp */
jmp_buf begin;

%}

%union {
    Inst *val;
    Symbol *sym;
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
/* %type  <val> expr term fact asig prim asg_exp */

%%

list: /* nothing */
    | list final
    | list asg_exp '\n'  { code2((Inst)pop, STOP); return 1; }
    | list expr    '\n'  { /*Si se escribe ; ó \n entonces hacer salto de linea*/
                           /* lookup retorna un Symbol *, asi que
                            * el valor retornado por lookup puede
                            * ser usado para acceder directamente
                            * a la variable, como si lo hubieramos
                            * asignado a una variable intermedia.
                            *   Symbol *interm = lookup("prev");
                            *   interm->u.val  = $2;
                            */
                           code3(varpush, (Inst)lookup("prev"), assign);
                           code2(print, STOP); return 1;
                         }
    | list asg_exp ';'  { code((Inst)pop); }
    | list expr    ';'  { code(print);     }
    | list error final  { yyerrok;         }
    ;

final:  '\n' | ';';     /* Regla para evaular si el caracter es '\n' ó ';'  */

asig: VAR   '=' asg_exp { code3(varpush, (Inst)$1, assign); }
    | CONST '=' asg_exp { execerror("No se puede asignar la constante %s",
                                    $1->name); }
    ;

asg_exp
    : asig
    | expr
    ;

expr: term
    | '-' term      { code(neg); }
    | '+' term
    | expr '+' term { code(add); }
    | expr '-' term { code(sub); }
    ;

term: fact
    | term '*' fact { code(mul);  }
    | term '/' fact { code(divi); }
    | term '%' fact { code(mod);  }
    ;

fact: prim '^' fact { code(pwr);  }
    | prim
    ;

prim: NUMBER                             { code2(constpush, (Inst)$1);     }
    | '(' asg_exp ')'
    | VAR                                { code3(varpush, (Inst)$1, eval); }
    | CONST                              { code3(varpush, (Inst)$1, eval); }
    | BLTIN0 '(' ')'                     { code2(bltin0, (Inst)$1); }
    | BLTIN1 '(' asg_exp ')'             { code2(bltin1, (Inst)$1); }
    | BLTIN2 '(' asg_exp ',' asg_exp ')' { code2(bltin2, (Inst)$1); }
    | VAR    '(' asg_exp ',' asg_exp ',' lista_expr ')' {
              execerror("functions (%s) with large list "
                        "parameters are not yet implemented",
                        $1->name);
            }
    ;

lista_expr
    : asg_exp
    | lista_expr ',' asg_exp
    ;

%%

char *progname;     /* for error messages */
int   lineno = 1;   /* numero de linea */

int
main(int argc, char *argv[]) /* hoc1 */
{
    progname = argv[0];
    init();
    setjmp(begin);
    for (initcode(); yyparse(); initcode())
        execute(prog);
    return EXIT_SUCCESS;
}

void yyerror(char *s)   /* called for yacc syntax error */
{
    warning("%s", s);
}
