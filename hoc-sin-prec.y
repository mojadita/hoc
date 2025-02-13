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

void warning( const char *fmt, ...);
void vwarning(const char *fmt, va_list args);
void yyerror( char *);

char *PRG_NAME = NULL;

/*  Necersario para hacer setjmp y longjmp */
jmp_buf begin;

%}

%union {
    Symbol *sym;
	double  val;
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
/* %type  <inst> expr term fact asig prim asg_exp */

%%

list: /* nothing */
    | list final
    | list asg_exp '\n'  { code_inst(drop); code_inst(STOP); return 1; }
    | list expr    '\n'  { /*Si se escribe ; ó \n entonces hacer salto de linea*/
                           /* lookup retorna un Symbol *, asi que
                            * el valor retornado por lookup puede
                            * ser usado para acceder directamente
                            * a la variable, como si lo hubieramos
                            * asignado a una variable intermedia.
                            *   Symbol *interm = lookup("prev");
                            *   interm->val  = $2;
                            */
						   code_inst(assign);
						   code_sym(lookup("prev"));
                           code_inst(print);
						   code_inst(STOP);
						   return 1;
                         }
    | list asg_exp ';'   { code_inst(drop);  }
    | list expr    ';'   { code_inst(print); }
    | list error final   { yyerrok;          }
    ;

final:  '\n' | ';';      /* Regla para evaular si el caracter es '\n' ó ';'  */

asig: VAR   '=' asg_exp  {
						   code_inst(assign);
						   code_sym($1);
						 }
    | CONST '=' asg_exp  { execerror("No se puede asignar la constante %s",
                                    $1->name);
						 }
    ;

asg_exp
    : asig
    | expr
    ;

expr: term
    | '-' term           { code_inst(neg);  }
    | '+' term
    | expr '+' term      { code_inst(add);  }
    | expr '-' term      { code_inst(sub);  }
    ;

term: fact
    | term '*' fact      { code_inst(mul);  }
    | term '/' fact      { code_inst(divi); }
    | term '%' fact      { code_inst(mod);  }
    ;

fact: prim '^' fact      { code_inst(pwr);  }
    | prim
    ;

prim: NUMBER                             { code_inst(constpush);
                                           code_val($1);
										 }
    | '(' asg_exp ')'
    | VAR                                {
										   code_inst(eval);
										   code_sym($1);
										 }
    | CONST                              {
										   code_inst(eval);
										   code_sym($1);
                                           if ( strcmp($1->name, "version")==0 )
                                                printf( "\t\033[1;33;40m%s "
                                                        "\033[0;36;40mcalculator version "
                                                        "\033[1;33;40m%.1g\033[0m\n",
                                                        PRG_NAME,
                                                        $1->val );
										 }
    | BLTIN0 '(' ')'                     { code_inst(bltin0);
										   code_sym($1);
										 }
    | BLTIN1 '(' asg_exp ')'             { code_inst(bltin1);
										   code_sym($1);
										 }
    | BLTIN2 '(' asg_exp ',' asg_exp ')' { code_inst(bltin2);
										   code_sym($1);
										 }
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
    PRG_NAME = argv[0];
    init();
    setjmp(begin);
    for (initcode(); yyparse(); initcode())
        execute(prog);
    return EXIT_SUCCESS;
}

void yyerror(char *s)   /* called for yacc syntax error */
{
    //warning("%s", s);
    warning(" \033[1;32m%s", s);
}
