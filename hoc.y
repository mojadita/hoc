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
#include "code.h"

static void yyerror(char *);

char *PRG_NAME = NULL;

/*  Necesario para hacer setjmp y longjmp */
jmp_buf begin;

#define CODE_INST(F) code_inst(F, "\033[36m"#F"\033[m")
#define CODE_STOP() code_inst(STOP, "\033[33mSTOP\033[m")

%}
/* continuamos el area de definicion y configuracion
 * de yacc */

/*  Declaracion tipos de datos de los objetos
    (TOKENS, SYMBOLOS no terminales)  */
%union {
    Inst    inst; /* machine instruction */
    Symbol *sym; /* symbol table pointer */
	double  val; /* double value */
	Cell   *cel; /* Cell reference */
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
%token       PRINT WHILE IF ELSE SYMBS
%type  <cel> stmt asgn expr stmtlist cond while if end

/* la directiva %type indica los tipos de datos de los diferentes
 * simbolos no terminales, definidos en la gramatica */

%right '='         /* right associative, minimum precedence */
%left  OR          /* || */
%left  AND         /* && */
%left  '>' GE '<' LE EQ NE /* > >= <= == != */
%left  '+' '-'     /* left associative, same precedence */
%left  '*' '/' '%' /* left associative, higher precedence */
%left  UNARY '!'   /* new, lo mas todavia */
%right '^'         /* operador exponenciacion */

/* fin del area de configuracion de yacc */
%%
/*  Area de definicion de reglas gramaticales */

list: /* nothing */
    | list       '\n'
	| list stmt  '\n'  { CODE_STOP(); return 1; }
    | list asgn  '\n'  { CODE_INST(assign);
						 code_sym(lookup("prev"));
                         CODE_INST(print);
						 CODE_STOP();
                         return 1; }
    | list error finish { yyerrok;
						  return 1; }
    ;

finish: '\n' | ';' ;

asgn: VAR   '=' expr   {
                         if ($1->type != VAR && $1->type != UNDEF) {
                            execerror("symbol '%s' is not a variable\n",
                                      $1->name);
                         }
						 $$ = $3;
						 CODE_INST(assign);
						 code_sym($1);
					   }
    ;

stmt: expr ';'                      { CODE_INST(drop); } 
    | PRINT expr ';'                { CODE_INST(print); $$ = $2; }
	| SYMBS ';'                     { $$ = CODE_INST(list_symbols); }
	| while cond stmt end           {
									  ($1)[1].cel =  $3; /* body of loop */
									  ($1)[2].cel =  $4; /* end, if cond fails */
									}
	| if cond stmt end              {
									  ($1)[1].cel = $3; /* then part */
									  ($1)[3].cel = $4; /* end, if cond fails */
									}
    | if cond stmt end ELSE stmt end {
									  ($1)[1].cel = $3; /* then part */
									  ($1)[2].cel = $6; /* else part, if cond fails */
									  ($1)[3].cel = $7; /* end */
									}
 	| '{' stmtlist '}'              { $$ = $2; }
	;

cond: '(' expr ')'                  { CODE_STOP(); $$ = $2; }
    ;

while
    : WHILE                         { $$ = CODE_INST(whilecode);
										   code_cel(NULL);
										   code_cel(NULL); }
	;

if  : IF                            { $$ = CODE_INST(ifcode);
                                           code_cel(NULL);
                                           code_cel(NULL);
                                           code_cel(NULL); }
	;

end : /* nothing */                 { CODE_STOP(); $$ = progp; }
    ;

stmtlist
    : /* nothing */                 { $$ = progp; }
	| stmtlist '\n'
	| stmtlist stmt
	;

expr: NUMBER                        { $$ = CODE_INST(constpush);
                                           code_val($1); }
    | VAR                           { $$ = CODE_INST(eval);
									       code_sym($1); }

    | CONST                         { CODE_INST(eval);
									  code_sym($1); 
									}
    | asgn                          /* asignacion equivalente a { $$ = $1; } */
    | BLTIN0 '(' ')'                { $$ = CODE_INST(bltin0);    code_sym($1); printf("\tBLTIN0 Rule in action\n"); }
    | BLTIN1 '(' expr ')'           { $$ = CODE_INST(bltin1);    code_sym($1); printf("\tBLTIN1 Rule in action\n"); }
    | BLTIN2 '(' expr ',' expr ')'  { $$ = CODE_INST(bltin2);    code_sym($1); printf("\tBLTIN2 rule in action\n"); }
	| expr '>' expr                 { CODE_INST(gt);  }
	| expr '<' expr                 { CODE_INST(lt);  }
	| expr EQ  expr  /* == */       { CODE_INST(eq);  }
	| expr NE  expr  /* != */       { CODE_INST(ne);  }
	| expr GE  expr  /* >= */       { CODE_INST(ge);  }
	| expr LE  expr  /* <= */       { CODE_INST(le);  }
	| expr AND expr  /* && */       { CODE_INST(and); }
	| expr OR  expr  /* || */       { CODE_INST(or);  }
    | expr '+' expr                 { CODE_INST(add); }
    | expr '-' expr                 { CODE_INST(sub); }
    | expr '%' expr                 { CODE_INST(mod); }
    | expr '*' expr                 { CODE_INST(mul); }
    | expr '/' expr                 { CODE_INST(divi); }
    | '(' expr ')'                  { $$ = $2; }
    | expr '^' expr                 { CODE_INST(pwr); }
    | '+' expr %prec UNARY          { $$ = $2; }
    | '-' expr %prec UNARY          { CODE_INST(neg); $$ = $2; } /* new */
	| '!' expr %prec UNARY          { CODE_INST(not); $$ = $2; } /* not */
    ;

/* Fin Area de definicion de reglas gramaticales */
%%

char *progname;     /* for error messages */
int   lineno = 1;   /* numero de linea */

int parse(void)
{
	printf("%s:%d:%s \033[1;36mBEGIN\033[m\n", __FILE__, __LINE__, __func__);
	int res = yyparse();
	printf("%s:%d:%s \033[1;36mEND\033[m\n", __FILE__, __LINE__, __func__);
	return res;
}

int
main(int argc, char *argv[]) /* hoc1 */
{
    progname = argv[0];
    PRG_NAME = argv[0];
    init();
    setjmp(begin);
    for (initcode(); parse(); initcode()) {
        execute(prog);
		printf("Stack size after execution: %d\n", stacksize());
	}
    return EXIT_SUCCESS;
} /* main */

static void yyerror(char *s)   /* called for yacc syntax error */
{
    //warning("%s", s);
    warning(" \033[1;32m%s", s);
}
