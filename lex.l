%{

#include "hoc.h"
#include "hoc.tab.h"
#include "code.h"
#include "reserved_words.h"

#ifndef UQ_LEX_DEBUG
//#warning UQ_LEX_DEBUG deberia ser configurado en config.mk
#define UQ_LEX_DEBUG 0
#endif

#if UQ_LEX_DEBUG  /*   {{ */
#define P(_fmt, ...)                      \
	printf("%s:%d:%s "_fmt,               \
			__FILE__, __LINE__, __func__, \
			##__VA_ARGS__)
#else /* UQ_LEX_DEBUG  }{ */
#define P(_fmt, ...)
#endif /* UQ_LEX_DEBUG }} */

extern int lineno;

%}

%option noyywrap
%x COMMENT

%%


[ \t]    ;  /* saltar blancos y tabuladores */

([0-9]+\.?|[0-9]*\.[0-9]+)([eE][-+]?[0-9]+)? {
            sscanf(yytext, "%lf", &yylval.val);
			P("number %g (%d)\n", yylval.val, NUMBER);
            return NUMBER;
        }   /* Un numero punto flotante  */

"/*"    {   BEGIN COMMENT;
			printf("%s", yytext);
		}
<COMMENT>. { ECHO; }
<COMMENT>"*/" {
	        printf("%s\n", yytext);	
			BEGIN INITIAL;
		}

[a-zA-Z_][a-zA-Z0-9_]*  {
           	const reserved_word *rw = rw_lookup(yytext);
			if (rw) {
				P("reserved word %s -> %s(%d)\n",
					rw->name, rw->tokn_str, rw->tokn);
				return rw->tokn;
			}
            Symbol *s;
            if ((s = lookup(yytext)) == NULL) {
                /* no esta, lo instalamos */
                s = install(yytext, UNDEF, 0.0, NULL);
            }
            yylval.sym = s;
			P("Symbol <%s> (%d)\n", s->name, s->type);
            return s->type == UNDEF ? VAR : s->type; }

">="    { P("operador %s (%d)\n", yytext, GE); return GE; }
"<="    { P("operador %s (%d)\n", yytext, LE); return LE; }
"=="    { P("operador %s (%d)\n", yytext, EQ); return EQ; }
"!="    { P("operador %s (%d)\n", yytext, NE); return NE; }
"&&"    { P("operador %s (%d)\n", yytext, AND); return AND; }
"||"    { P("operador %s (%d)\n", yytext, OR); return OR; }

\n      {   lineno++;
			P("salto de linea (%d)\n", '\n');
            return '\n';
        }

#.*     ;

.       { P("symbol/operator '%c' (%d)\n",
				yytext[0], yytext[0]);
		  return yytext[0]; }

<<EOF>> { P("EOF (0)\n"); return 0; }

%%

#if 0
/*  Si esta funcion no se escribe, debemos enlazar con -ll
 *  o poner la opcion %option noyywrap arriba. */
int yywrap()
{
    return 1;
}
#endif
