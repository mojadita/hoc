/* hoc.y -- analizador sintactico de hoc.
 * Date: Mon Dec 30 11:59:54 -05 2024
 */
%{
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#include "hoc.h"

void warning(char *, char *);
int  yylex(void);
void yyerror(char *);

jmp_buf begin;

%}

%union {
    double val;
    Symbol *sym;
}

%token <val> NUMBER
%token <sym> VAR BLTIN0 BLTIN1 BLTIN2 UNDEF
%type  <val> expr asgn

%right '='         /* right associative, minimum precedence */
%left  '+' '-'     /* left associative, same precedence */
%left  '*' '/' '%' /* left associative, higher precedence */
%left  UNARY       /* new, lo mas todavia */
%right '^'         /* operador exponenciacion */

%%

list: /* nothing */
    | list       final
	| list asgn  final
    | list expr  final     { /* si se escribe ; entonces
						      * hacer salto de linea */
                             printf( "\t%.8g\n", $2 );
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

asgn: VAR '=' expr         { $$ = $1->u.val = $3;
							 $1->type = VAR;
						   }
	;

final: '\n' | ';' ;

expr: NUMBER
    | VAR                  { if ($1->type == UNDEF) {
								execerror(
									"undefined variable '%s'\n",
									$1->name);
							 }
							 $$ = $1->u.val;
						   }
    | asgn                          /* asignacion */
	| BLTIN0 '(' ')'                { $$ = $1->u.ptr0(); }
	| BLTIN1 '(' expr ')'           { $$ = $1->u.ptr1($3); }
	| BLTIN2 '(' expr ',' expr ')'  { $$ = $1->u.ptr2($3, $5); }
    | expr '+' expr                 { $$ = $1 + $3; }
    | expr '-' expr                 { $$ = $1 - $3; }
    | expr '%' expr                 { $$ = fmod($1, $3); }
    | expr '*' expr                 { $$ = $1 * $3; }
    | expr '/' expr                 { $$ = $1 / $3; }
    | '(' expr ')'                  { $$ = $2; }
	| expr '^' expr                 { $$ = pow($1, $3); }
    | '+' expr %prec UNARY          { $$ =  $2; } /* new */
    | '-' expr %prec UNARY          { $$ = -$2; } /* new */
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
} /* main */

int yylex(void)   /* hoc1 */
{
    int c;

    while ((c=getchar()) == ' ' || c == '\t')
        continue;
	/*  c != ' ' && c != '\t' */

    /*  si se presiona  [Escape] [Enter]  salimos del programa  */
    /*  [Escape]:   \e   27    x01b   */
    if ( c == '\e' )
    {
        printf( "  Saliendo .... Chao!!...\n" );
        return 0;
    }
    if (c == EOF)   /*  si se preiona  [Control] d  Salimos del programa  */
        return 0;  /* retornando tipo de token  */
    if (c == '.' || isdigit(c)) { /* number */
        ungetc(c, stdin);  /* retornando tipo de token  */
        if (scanf("%lf", &yylval.val) != 1) {
#if 1
            /*  Esta es la condicion que sempre se cumple  */
            /*  Leer un solo caracter  */
            getchar();  /*  esto es similar al codigo de abajo  */ 
#else 
            /*  Lectura hasta el final de la linea  */ 
            while ((c = getchar()) != EOF && c != '\n')
                continue;
			/* c == EOF || c == '\n' */

			/*  Se hace para que luego el parser lea el siguiente TOKENs
			 * (si c diera la casualidad de ser un salto de linea '\n' */
            if (c == '\n')
                ungetc(c, stdin);  /* retrocede el ulitmo caracter leido */
#endif
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
		if ((s = lookup(sbuf)) == NULL)
			s = install(sbuf, UNDEF, 0.0);
        yylval.sym = s;
        return s->type == UNDEF ? VAR : s->type;
    }
    /*  Salto de linea normal  */
    if (c == '\n') lineno++;

    return c;
} /* yylex */

void yyerror(char *s)   /* called for yacc syntax error */
{
    warning(s, NULL);
}

void execerror(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	longjmp(begin, 0);
} /* execerror */

void warning(char *s, char *t)    /* print warning message */
{
    fprintf(stderr, "%s: %s", progname, s);
    if (t)
        fprintf(stderr, " %s", t);
    fprintf(stderr,     " near line %d\n",  lineno);
}
