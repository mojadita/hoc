/* init.c -- funciones de inicializacion.
 * Date: Sat Dec 28 14:06:39 -05 2024
 */

#include <stdlib.h>
#include <math.h>

#include "hoc.h"
#include "hoc.tab.h"
#include "math.h"
#include "code.h"

double integer(double x);
double Rand(void);

static struct { /* constants */
    char *name;
    double cval;
} consts[] = {
    "PI",      M_PI,
    "E",       M_E,
    "DEG",     180.0/M_PI,
    "PHI",     1.61803398874989484820,
    "prev",    0.0,
    "version", 5.0,
    NULL,      0.0,
};

static struct builtin { /* built-ins-1 */
    char *name;
    double (*func)();
    int  type;
	const char *help;
} builtins[] = {
    "rand",  Rand,      BLTIN0, "rand()",
    "sin",   sin,       BLTIN1, "sin(x)",
    "cos",   cos,       BLTIN1, "cos(x)",
    "tan",   tan,       BLTIN1, "tan(x)",
    "asin",  asin,      BLTIN1, "asin(x)",
    "acos",  acos,      BLTIN1, "acos(x)",
    "atan",  atan,      BLTIN1, "atan(x)",
    "log",   log,       BLTIN1, "log(x)",
    "log10", log10,     BLTIN1, "log10(x)",
    "exp",   exp,       BLTIN1, "exp(x)",
    "sqrt",  Sqrt,      BLTIN1, "sqrt(x)",
    "int",   integer,   BLTIN1, "int(x)",
    "abs",   fabs,      BLTIN1, "abs(x)",
    "atan2", atan2,     BLTIN2, "atan2(y,x)",
    "pow",   Pow,       BLTIN2, "pow(x,y)",
    "inv",   inverso,   BLTIN1, "inv(x)",
    "ops",   opuesto,   BLTIN1, "ops(x)",
    NULL,    NULL,
};

void init(void)  /* install constants and built-ins in table */
{
    int i;
    Symbol *s;

    for (i = 0; consts[i].name != NULL; i++) {
        install(consts[i].name, CONST, consts[i].cval, NULL);
    }

    for (   struct builtin *p = builtins;
            p->name;
            p++)
    {
        s = install(p->name, p->type, 0.0, p->help);
        switch(p->type) {
            case BLTIN0: s->ptr0 = p->func; break;
            case BLTIN1: s->ptr1 = p->func; break;
            case BLTIN2: s->ptr2 = p->func; break;
        }
    }
}

double integer(double x)
{
    return (int) x;
}
