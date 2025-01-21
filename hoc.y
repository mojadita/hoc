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

static void yyerror(char *);

/*  Necersario para hacer setjmp y longjmp */
jmp_buf begin;

%}
/* continuamos el area de definicion y configuracion
 * de yacc */

/*  Declaracion tipos de datos de los objetos
    (TOKENS, SYMBOLOS no terminales)  */
%union {
    double val;
    Symbol *sym;
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
%type  <val> expr asgn

%right '='         /* right associative, minimum precedence */
%left  '+' '-'     /* left associative, same precedence */
%left  '*' '/' '%' /* left associative, higher precedence */
%left  UNARY       /* new, lo mas todavia */
%right '^'         /* operador exponenciacion */

/* fin del area de configuracion de yacc */
%%
/*  Area de definicion de reglas gramaticales */

list: /* nothing */
    | list       final
    | list asgn  final
    | list expr  final     { /* si se escribe ; entonces
                              * hacer salto de linea */
                             printf( "\t"OUTPUT_FMT"\n", $2 );
                             /* lookup retorna un Symbol *, asi que
                              * el valor retornado por lookup puede
                              * ser usado para acceder directamente
                              * a la variable, como si lo hubieramos
                              * asignado a una variable intermedia.
                              *   Symbol *interm = lookup("prev");
                              *   interm->u.val  = $2;
                              */
                             lookup("prev")->u.val = $2;
                           }
    | list error final {  yyerrok;  }
    ;

asgn: VAR '=' expr         { if ($1->type == CONST) {
                                 execerror("intento de asignar la constante %s",
                                    $1->name);
                             }
                             $$ = $1->u.val = $3;
                             $1->type = VAR;
                           }
    | CONST '=' expr       { execerror("No se puede asignar la constante %s",
                                    $1->name); }
    ;

final: '\n' | ';' ;

expr: NUMBER            /* { $$ = $1; } */
    | VAR               { if ($1->type == UNDEF) {
                              execerror(
                                  "undefined variable '%s'",
                                  $1->name);
                          }
                          $$ = $1->u.val;
                        }
    | CONST                         { $$ = $1->u.val; }
    | asgn                          /* asignacion */
    | BLTIN0 '(' ')'                { $$ = $1->u.ptr0(); }
    | BLTIN1 '(' expr ')'           { $$ = $1->u.ptr1($3); }
    | BLTIN2 '(' expr ',' expr ')'  { $$ = $1->u.ptr2($3, $5); }
    | expr '+' expr                 { $$ = $1 + $3; }
    | expr '-' expr                 { $$ = $1 - $3; }
    | expr '%' expr                 { if ($3 == 0) {
                                          execerror(
                                              "error modulo 0");
                                      }
                                      $$ = fmod($1, $3); }
    | expr '*' expr                 { $$ = $1 * $3; }
    | expr '/' expr                 { if ($3 == 0) {
                                          execerror(
                                              "division por 0");
                                      }
                                      $$ = $1 / $3; }
    | '(' expr ')'                  { $$ = $2; }
    | expr '^' expr                 { $$ = Pow($1, $3); }
    | '+' expr %prec UNARY          { $$ =  $2; } /* new */
    | '-' expr %prec UNARY          { $$ = -$2; } /* new */
    ;

/* Fin Area de definicion de reglas gramaticales */
%%

char *progname;     /* for error messages */
int   lineno = 1;   /* numero de linea */

int
main(int argc, char *argv[]) /* hoc1 */
{
    progname = argv[0];
    init();
    setjmp(begin);
    yyparse();
    return EXIT_SUCCESS;
} /* main */

static void yyerror(char *s)   /* called for yacc syntax error */
{
    warning("%s", s);
}
