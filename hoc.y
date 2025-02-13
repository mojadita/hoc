/* area de definiciones y configuracion de yacc
 * se extiende hasta encontrar %% aislado en una linea */
/* hoc.y -- analizador sintactico de hoc.
 * Date: Mon Dec 30 11:59:54 -05 2024
 */
%{
/* area reservada para insertar codigo C */
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#include "hoc.h"
#include "error.h"
#include "math.h"   /*  Modulo personalizado con nuevas funciones */
#include "code.h"

static void yyerror(char *);

char *PRG_NAME = NULL;

/*  Necesario para hacer setjmp y longjmp */
jmp_buf begin;

%}
/* continuamos el area de definicion y configuracion
 * de yacc */

/*  Declaracion tipos de datos de los objetos
    (TOKENS, SYMBOLOS no terminales)  */
%union {
    Inst   *inst; /* machine instruction */
    Symbol *sym; /* symbol table pointer */
	double  val; /* double value */
}

/* Los valores que retorna la fncion  yylex son declarados con
 * la directiva %token.
 *
 * LCU: Tue Jan 21 10:55:22 EET 2025
 * definimos los mismos tokens aqui que en hoc-sin-prec.y, para
 * asegurar que se asignaran los mismos valores, y no habra
 * problemas al construir ambos ejecutables.
 */
%token ERROR
%token <val> NUMBER
%token <sym> VAR BLTIN0 BLTIN1 BLTIN2 UNDEF CONST

/* la directiva %type indica los tipos de datos de los diferentes
 * simbolos no terminales, definidos en la gramatica */

%right '='         /* right associative, minimum precedence */
%left  '+' '-'     /* left associative, same precedence */
%left  '*' '/' '%' /* left associative, higher precedence */
%left  UNARY       /* new, lo mas todavia */
%right '^'         /* operador exponenciacion */

/* fin del area de configuracion de yacc */
%%
/*  Area de definicion de reglas gramaticales */

list: /* nothing */
    | list       '\n'
	| list asgn  ';'  { code_inst(drop); }
	| list expr  ';'  { code_inst(print); }
    | list asgn  '\n' { code_inst(drop); code_inst(STOP); return 1; }
    | list expr  '\n' {
						 code_inst(assign);
						 code_sym(lookup("prev"));
                         code_inst(print); code_inst(STOP);
                         return 1; }
    | list error final { yyerrok; }
    ;

asgn: VAR   '=' expr   {
                         if ($1->type != VAR && $1->type != UNDEF) {
                            execerror("symbol '%s' is not a variable\n",
                                      $1->name);
                         }
						 code_inst(assign);
						 code_sym($1);
					   }
    | CONST '=' expr   { execerror("No se puede asignar la constante %s",
                                   $1->name); }
    ;

final: '\n' | ';' ;

expr: NUMBER                        { code_inst(constpush);
                                      code_val($1); }
    | VAR                           { code_inst(eval);
									  code_sym($1); }
    | CONST                         { code_inst(eval);
									  code_sym($1); 
                                      if ( strcmp($1->name, "version")==0 )
										   printf( "\t\033[1;33;40m%s "
                                                   "\033[0;36;40mcalculator version "
                                                   "\033[1;33;40m%.1g\033[0m\n",
                                                   PRG_NAME,
                                                   $1->val );
									}
    | asgn                          /* asignacion */
    | BLTIN0 '(' ')'                { code_inst(bltin0);    code_sym($1); printf("\tBLTIN0 Rule in action\n"); }
    | BLTIN1 '(' expr ')'           { code_inst(bltin1);    code_sym($1); printf("\tBLTIN1 Rule in action\n"); }
    | BLTIN2 '(' expr ',' expr ')'  { code_inst(bltin2);    code_sym($1); printf("\tBLTIN2 rule in action\n"); }
    | expr '+' expr                 { code_inst(add); }
    | expr '-' expr                 { code_inst(sub); }
    | expr '%' expr                 { code_inst(mod); }
    | expr '*' expr                 { code_inst(mul); }
    | expr '/' expr                 { code_inst(divi); }
    | '(' expr ')'
    | expr '^' expr                 { code_inst(pwr); }
    | '+' expr %prec UNARY
    | '-' expr %prec UNARY          { code_inst(neg); } /* new */
    ;

/* Fin Area de definicion de reglas gramaticales */
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
} /* main */

static void yyerror(char *s)   /* called for yacc syntax error */
{
    //warning("%s", s);
    warning(" \033[1;32m%s", s);
}
