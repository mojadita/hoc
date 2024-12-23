
%{
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define YYSTYPE double /* data type of yacc stack */

void warning(char *, char *);
int  yylex(void);
void yyerror(char *);

%}

%token NUMBER

%%

list: /* nothing */  { puts("list: /* nothing */");  }
	| list '\n'      { puts("list: list '\n'");      }
	| list expr '\n' { puts("list: list expr '\n'"); }
	;

expr: term          { puts("expr: term");          }
	| expr '+' term { puts("expr: expr '+' term"); }
	| expr '-' term { puts("expr: expr '-' term"); }
	;

term: fact          { puts("term: fact");          }
	| term '*' fact { puts("term: term '*' fact"); }
	| term '/' fact { puts("term: term '/' fact"); }
	;

fact: NUMBER        { puts("fact: NUMBER");       }
	| '(' expr ')'  { puts("fact: '(' expr ')'"); }
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
	
	if (c == EOF)
		return 0;  /* retornando tipo de token  */
	if (c == '.' || isdigit(c)) { /* number */
		ungetc(c, stdin);  /* retornando tipo de token  */
		scanf("%lf", &yylval);
		printf("Hemos leido un numero: %.8lg\n", yylval);
		return NUMBER;  /* retornando tipo de token  */
	}
	if (c == '\n')
		lineno++;
	printf("yylex: retornamos '%c'\n", isprint(c) ? c : '@');
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
