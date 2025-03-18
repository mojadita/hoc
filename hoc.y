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

/*  Necesario para hacer setjmp y longjmp */
jmp_buf begin;

#define CODE_INST(I) code_inst(I,    "\033[36m"#I"\033[m")
#define CODE_STOP()  code_inst(STOP, "\033[33mSTOP\033[m")

#ifndef UQ_HOC_DEBUG
//#warning UQ_HOC_DEBUG deberia ser configurado en config.mk
#define UQ_HOC_DEBUG 1
#endif

#if UQ_HOC_DEBUG  /*   {{ */
#define P(_fmt, ...)                      \
    printf("%s:%d:%s "_fmt,               \
            __FILE__, __LINE__, __func__, \
            ##__VA_ARGS__)
#else /* UQ_HOC_DEBUG  }{ */
#define P(_fmt, ...)
#endif /* UQ_HOC_DEBUG }} */

int indef_proc,  /* 1 si estamos en una definicion de procedimiento */
    indef_func;  /* 1 si estamos en una definicion de funcion */

%}
/* continuamos el area de definicion y configuracion
 * de yacc */

/*  Declaracion tipos de datos de los objetos
    (TOKENS, SYMBOLOS no terminales)  */
%union {
    Inst    inst; /* machine instruction */
    Symbol *sym;  /* symbol table pointer */
    double  val;  /* double value */
    Cell   *cel;  /* Cell reference */
    int     num;  /* valor entero, para $<n> */
    char   *str;  /* cadena de caracteres */
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
%token       OR AND GE LE EQ NE
%token       UNARY
%token <num> ARG
%token <int> FUNC PROC
%token       RETURN READ
%token <sym> FUNCTION PROCEDURE
%token <str> STRING
%type  <cel> stmt asig expr stmtlist cond while if end
%type  <num> arglist_opt arglist

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
    | list defn  '\n'
    | list stmt  '\n'      { CODE_STOP();
                             return 1; }
    | list asig  '\n'      { CODE_INST(assign);
                             code_sym(lookup("prev"));
                             CODE_INST(print);
                             CODE_STOP();
                             return 1; }
    | list error final     { yyerrok;
                             return 1; }
    ;

final: '\n' | ';' ;

stmt: asig        ';'      { CODE_INST(drop); }
    | RETURN      ';'      { defnonly(indef_proc, "return");
                             $$ = CODE_INST(procret); }
    | RETURN expr ';'      { defnonly(indef_func, "return <expr>");
                             $$ = $2;
                             CODE_INST(funcret); }
    | PRINT  asig ';'      { CODE_INST(print); $$ = $2; }
    | SYMBS       ';'      { $$ = CODE_INST(list_symbols); }
    | while cond stmt end  { $1[1].cel = $3;   /* body of loop */
                             $1[2].cel = $4;   /* end, if cond fails */ }
    | if    cond stmt end  { $1[1].cel = $3;   /* then part */
                             $1[3].cel = $4;   /* end, if cond fails */ }
    | if    cond stmt end ELSE stmt end {
                             $1[1].cel = $3;   /* then part */
                             $1[2].cel = $6;   /* else part, if cond fails */
                             $1[3].cel = $7;   /* end */ }
    | '{' stmtlist '}'     { $$ = $2; }
    | PROCEDURE '(' arglist_opt ')' ';' {
                             $$ = CODE_INST(call); /* instruction */
                                  code_sym($1);    /* symbol associated to proc*/
                                  code_num($3);    /* number of arguments */ }
    ;

cond: '(' asig ')'         { CODE_STOP(); $$ = $2; }
    ;

while : WHILE              { $$ = CODE_INST(whilecode);
                                  code_cel(NULL);
                                  code_cel(NULL); }
    ;

if  : IF                   { $$ = CODE_INST(ifcode);
                                  code_cel(NULL);
                                  code_cel(NULL);
                                  code_cel(NULL); }
    ;

end : /* nothing */        { CODE_STOP(); $$ = progp; }
    ;

stmtlist: /* empty */      { $$ = progp; }
    | stmtlist '\n'
    | stmtlist stmt
    ;

asig: VAR   '=' asig       { if ($1->type != VAR && $1->type != UNDEF) {
                                 execerror("symbol '%s' is not a variable\n",
                                           $1->name);
                             }
                             $$ = $3;
                             CODE_INST(assign);
                             code_sym($1); }
    | ARG   '=' asig       { defnonly(indef_proc || indef_func, "$%d assign", $1);
                             $$ = $3;
                             CODE_INST(argassign);
                             code_num($1);
                           }
    | expr
    ;

expr: NUMBER                        { $$ = CODE_INST(constpush);
                                           code_val($1); }
    | VAR                           { $$ = CODE_INST(eval);
                                           code_sym($1); }
    | ARG                           { defnonly(indef_proc || indef_func,
                                                "$%d assign", $1);
                                      $$ = CODE_INST(argeval);
                                           code_num($1);
                                    }

    | CONST                         { CODE_INST(eval);
                                      code_sym($1); }
    | BLTIN0 '(' ')'                { $$ = CODE_INST(bltin0);
                                           code_sym($1);
                                      printf("\tBLTIN0 Rule in action\n"); }
    | BLTIN1 '(' expr ')'           { $$ = CODE_INST(bltin1);
                                           code_sym($1);
                                      printf("\tBLTIN1 Rule in action\n"); }
    | BLTIN2 '(' expr ',' expr ')'  { $$ = CODE_INST(bltin2);
                                           code_sym($1);
                                      printf("\tBLTIN2 rule in action\n"); }
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
    | READ '(' VAR ')'              { $$ = CODE_INST(readopcode);
                                           code_sym($3); }
    | FUNCTION '(' arglist_opt ')' {
                              $$ = CODE_INST(call); /* instruction */
                              code_sym($1);         /* function symbol */
                              code_num($3);         /* number of arguments */ }
    ;

arglist_opt
    : arglist
    | /* empty */           { $$ = 0; }
    ;

arglist
    : arglist ',' asig      { $$ = $1 + 1; }
    | asig                  { $$ = 1; }
    ;

defn: proc_head '(' ')' stmt {
                              CODE_INST(procret);
                              end_define();
                              indef_proc    = 0;
                              P("FIN DEFINICION PROCEDIMIENTO\n");
                            }
    | func_head '(' ')' stmt {
                              CODE_INST(constpush);
                              code_val(0.0);
                              CODE_INST(funcret);
                              end_define();
                              indef_func    = 0;
                              P("FIN DEFINICION FUNCION\n");
                            }
proc_head
    : PROC VAR              {
                              P("DEFINIENDO EL PROCEDIMIENTO '%s'\n", $2->name);
                              define($2, PROCEDURE);
                              indef_proc    = 1; }
func_head
    : FUNC VAR              {
                              P("DEFINIENDO LA FUNCION '%s'\n", $2->name);
                              define($2, FUNCTION);
                              indef_func    = 1; }

/* Fin Area de definicion de reglas gramaticales */
%%

static void yyerror(char *s)   /* called for yacc syntax error */
{
    //warning("%s", s);
    warning(" \033[1;32m%s", s);
} /* yyerror */
