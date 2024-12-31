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

void warning(const char *fmt, ...);
void vwarning(const char *fmt, va_list args);
int  yylex(void);
void yyerror(char *);

/*  Necersario para hacer setjmp y longjmp */
jmp_buf begin;

%}

%union {
    double val;
    Symbol *sym;
}

%token <val> NUMBER
%token <sym> VAR BLTIN0 BLTIN1 BLTIN2 CONST
%type  <val> expr term fact asig prim asg_exp

%%

list: /* nothing */
    | list final
    | list asig final
    | list expr final {  /*  Si se escribe ; ó \n entonces hacer salto de linea  */
                         printf("   %.8g\n", $2);
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

final:  '\n' | ';';    /* Regla para evaular si el caracter es '\n' ó ';'  */

asig: VAR '=' asg_exp     { if ($1->type == CONST) {
                                 execerror("intento de asignar la constante %s",
                                    $1->name);
                             }
                             $$ = $1->u.val = $3;
                             $1->type = VAR;
                           }
	| CONST '=' asg_exp    { execerror("No se puede asignar la constante %s",
									$1->name); }
    ;

expr: term
    | '-' term      { $$ = -$2;      }
    | '+' term      { $$ =  $2;      }
    | asg_exp '+' term { $$ =  $1 + $3; }
    | asg_exp '-' term { $$ =  $1 - $3; }
    ;

term: fact
    | term '*' fact { $$ = $1 * $3; }
    | term '/' fact { if ($3 == 0) {
                          execerror("error /0");
                      }
                      $$ = $1 / $3; }
    | term '%' fact { if ($3 == 0) {
                          execerror("error modulo 0");
                      }
                      $$ = fmod($1, $3); }
    ;

fact: prim '^' fact { $$ = Pow($1, $3); }
    | prim
    ;

asg_exp: asig
      | expr
      ;

prim: NUMBER
    | '(' asg_exp ')'  { $$ = $2; }
    | VAR           { if ($1->type == UNDEF) {
                          execerror(
                              "undefined variable '%s'",
                              $1->name);
                      }
                      $$ = $1->u.val;
                    }
	| CONST                              { $$ = $1->u.val; }
    | BLTIN0 '(' ')'                     { $$ = $1->u.ptr0(); }
    | BLTIN1 '(' asg_exp ')'             { $$ = $1->u.ptr1($3); }
    | BLTIN2 '(' asg_exp ',' asg_exp ')' { $$ = $1->u.ptr2($3, $5); }
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
    yyparse();
    return EXIT_SUCCESS;
}

int yylex(void)   /* hoc3 */
{
    int c;

    while ((c=getchar()) == ' ' || c == '\t')
        continue;

    if ( c == 27 )  /*  si se presiona  [Escape] [Enter]  vamos a salir  */
    {
        printf( "  ***  Saliendo .... chao!!...\n" );
        return 0;
    }
    if (c == EOF)   /*  si se presiona  [Control] d  vamos a salir  */
        return 0;  /* retornando tipo de token  */
    if (c == '.' || isdigit(c)) { /* number */
        ungetc(c, stdin);  /* retornando tipo de token  */
        if (scanf("%lf", &yylval.val) < 1) {
            getchar();
            return YYERRCODE;
        }
        return NUMBER;  /* retornando tipo de token  */
    }
    if (isalpha(c)) {
        Symbol *s;
        char sbuf[100], *p = sbuf;

        do {
            *p++ = c;
        } while (((c = getchar()) != EOF) && isalnum(c));
        /* c == EOF || !isalnum(c) */
        if (c != EOF) ungetc(c, stdin);
        *p = '\0';
        if ((s = lookup(sbuf)) == NULL) {
            s = install(sbuf, UNDEF, 0.0);
        }
        /* el valor que se usa en el interprete */
        yylval.sym = s;

        return s->type == UNDEF
                ? VAR
                : s->type;
    }
    /* actualizar numero de linea */
    if (c == '\n') lineno++;

    return c;
}

void yyerror(char *s)   /* called for yacc syntax error */
{
    warning("%s", s);
}
