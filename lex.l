%{
#include "hoc.h"
#include "y.tab.h"
#include "code.h"

#ifndef DEBUG
#define DEBUG 0
#endif

#if DEBUG
#define P(_fmt, ...)                      \
	printf("%s:%d:%s "_fmt,               \
			__FILE__, __LINE__, __func__, \
			##__VA_ARGS__)
#else
#define P(_fmt, ...)
#endif

extern int lineno;

static struct reserved_word { /* reserved statements */
	char *name;
	int   tokn;
} reserved_words[] = {
	{ .name = "if",    .tokn = IF    },
	{ .name = "else",  .tokn = ELSE  },
	{ .name = "while", .tokn = WHILE },
	{ .name = "print", .tokn = PRINT },
	{ .name = NULL }
};

%}

%option noyywrap

%%


[ \t]    ;  /* saltar blancos y tabuladores */

([0-9]+\.?|[0-9]*\.[0-9]+)([eE][-+]?[0-9]+)? {
            sscanf(yytext, "%lf", &yylval.val);
			P("number %g\n", yylval.val);
            return NUMBER;
        }   /* Un numero punto flotante  */

"?"     {
            list_symbols();
        }

[a-zA-Z_][a-zA-Z0-9_]*  {
           	struct reserved_word *rw;
			for (rw = reserved_words; rw->name; rw++) {
				if (strcmp(rw->name, yytext) == 0) {
					P("reserved word %s, token %d\n",
						rw->name, rw->tokn);
					return rw->tokn;
				}
			}
            Symbol *s;
            if ((s = lookup(yytext)) == NULL) {
                /* no esta, lo instalamos */
                s = install(yytext, UNDEF, 0.0, NULL);
            }
            yylval.sym = s;
			P("symbol <%s>\n", s->name);
            return s->type == UNDEF ? VAR : s->type; }

">="    { P("operador %s\n", yytext); return GE; }
"<="    { P("operador %s\n", yytext); return LE; }
"=="    { P("operador %s\n", yytext); return EQ; }
"!="    { P("operador %s\n", yytext); return NE; }
"&&"    { P("operador %s\n", yytext); return AND; }
"||"    { P("operador %s\n", yytext); return OR; }

\n      {   lineno++;
			P("salto de linea\n");
            return '\n';
        }

#.*     ;

.       { P("symbol/operator '%c'\n", yytext[0]); return yytext[0]; }
<<EOF>> { P("EOF\n"); return 0; }

%%

#if 0
/*  Si esta funcion no se escribe, debemos enlazar con -ll
 *  o poner la opcion %option noyywrap arriba. */
int yywrap()
{
    return 1;
}
#endif
