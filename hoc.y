
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
%left '+' '-'  /* left associative, same precedence */
%left '*' '/'  /* left associative, higher precedence */

%%

list: /* nothing */
	| list '\n'
	| list expr '\n' { printf("\t%.8g\n", $2); }
	;

expr: NUMBER
	| expr '+' expr { $$ = $1 + $3; }
	| expr '-' expr { $$ = $1 - $3; }
	| expr '*' expr { $$ = $1 * $3; }
	| expr '/' expr { $$ = $1 / $3; }
	| '(' expr ')'  { $$ = $1; }
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
		return NUMBER;  /* retornando tipo de token  */
	}
	if (c == '\n')
		lineno++;
	printf("yylex retornando [0x%02x]\n", c);
	return c;
}

void yyerror(char *s)   /* called for yacc syntax error */
{
	warning(s, NULL);
}

void warning(char *s, char *t)    /* print warning message */
{
	fprintf(stderr, "%s: %s", progname, s);
	if (t)
		fprintf(stderr, " %s", t);
	fprintf(stderr, " near line %d\n",  lineno);
}
