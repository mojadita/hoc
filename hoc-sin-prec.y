
%{
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

void warning(char *, char *);
int  yylex(void);
void yyerror(char *);

double mem[26];   /*  Array de variables desde 'a' hasta 'z'  */

%}

%union {
	double val;
	int    index;
}

%token <val>   NUMBER
%token <index> VAR
%type  <val>   expr term fact asig

%%

list: /* nothing */
	| list '\n'      
	| list asig '\n'  {  printf("\t%.8g\n", $2); 
                         /*  Asignar el dato impreso a la variable que esta 
						     en la posicion 'p' del array de variables  */
                         mem['p' - 'a'] = $2;
                      }
	| list asig ';'   {  /*  Si se escribe ; entonces hacer salto de linea  */
                         printf("   %.8g\n", $2); 
                         /*  Asignar el dato impreso a la variable que esta 
						     en la posicion 'p' del array de variables  */
                         mem['p' - 'a'] = $2;
                      }
	| list error '\n' {  yyerrok;  }
	;

asig: VAR '=' asig { $$ = mem[$1] = $3; }
	| expr ;

expr: term          
    | '-' term      { $$ = -$2;      }
    | '+' term      { $$ =  $2;      }
	| expr '+' term { $$ =  $1 + $3; }
	| expr '-' term { $$ =  $1 - $3; }
	;

term: fact          
	| term '*' fact { $$ = $1 * $3; }
	| term '/' fact { $$ = $1 / $3; }
	| term '%' fact { $$ = fmod($1, $3); }
	;

fact: NUMBER        
	| '(' asig ')'  { $$ = $2; }
	| VAR           { $$ = mem[$1]; }
	;

%%

char *progname;     /* for error messages */
int   lineno = 1;   /* numero de linea */

int
main(int argc, char *argv[]) /* hoc1 */
{
	progname = argv[0];
	yyparse();
	return EXIT_SUCCESS;
}
	
int yylex(void)   /* hoc1 */
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
	if (c == '\n')
		lineno++;
	if (islower(c)) {
		yylval.index = c - 'a';
		return VAR;
	}
	return c;
}

void yyerror(char *s)   /* called for yacc syntax error */
{
	warning(s, NULL);
}

void warning(char *s, char *t)    /* print warning message */
{
	fprintf(stdout, "%s: %s", progname, s);
	if (t)
		fprintf(stdout, " %s", t);
	fprintf(stdout, " near line %d\n",  lineno);
}
