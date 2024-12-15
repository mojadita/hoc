
%{
#include <stdio.h>
#include <ctype.h>

#define YYSTYPE double /* data type of yacc stack */
%}

%token NUMBER

%%

list: expr '\n'  { printf("\t%.8g\n", $1); }
	| list expr '\n' { printf("\t%.8g\n", $2); }
	| /* nothing */ ;

expr: NUMBER
	| expr '+' expr { $$ = $1 + $3; }
	| expr '-' expr { $$ = $1 - $3; }
	| expr '*' expr { $$ = $1 * $3; }
	| expr '/' expr { $$ = $1 / $3; }
	| '(' expr ')'  { $$ = $1; };

%%

char *progname;   /* for error messages */
int   lineno = 1;   /* numero de linea */

main(argc, argv)    /* hoc1 */
	char *argv[];
{
	progname = argv[0];
	yyparse();
}
	
int yylex()   /* hoc1 */
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

yyerror(s)   /* called for yacc syntax error */
	char *s;
{
	warning(s, NULL);
}

warning(s, t)    /* print warning message */
	char *s, *t;
{
	fprintf(stderr, "%s: %s", progname, s);
	if (t)
		fprintf(stderr, " %s", t);
	fprintf(stderr, " near line %d\n",  lineno);
}
