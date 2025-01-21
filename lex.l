%{
#include "hoc.h"
#include "y.tab.h"
extern int lineno;
%}

%option noyywrap

%%


[ \t]    ;  /* saltar blancos y tabuladores */

([0-9]+\.?|[0-9]*\.[0-9]+)([eE][-+]?[0-9]+)? {
			sscanf(yytext, "%lf", &yylval.val);
			return NUMBER;
		}   /* Un numero punto flotante  */

[a-zA-Z_][a-zA-Z0-9_]*  {
			Symbol *s;
			if ((s = lookup(yytext)) == NULL) { 
				/* no esta, lo instalamos */
				s = install(yytext, UNDEF, 0.0);
			}
			yylval.sym = s;
			return s->type == UNDEF ? VAR : s->type;
		}

\n      {   lineno++;
			return '\n';
		}

.		return yytext[0];

%%

#if 0
/*  Si esta funcion no se escribe, debemos enlazar con -ll
 *  o poner la opcion %option noyywrap arriba. */
int yywrap()
{
	return 1;
}
#endif
