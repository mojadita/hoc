/* init.c -- funciones de inicializacion.
 * Date: Sat Dec 28 14:06:39 -05 2024
 */

#include <stdlib.h>
#include <math.h>

#include "hoc.h"
#include "y.tab.h"

double integer(double x);
double Rand(void);

static struct { /* constants */
	char *name;
	double cval;
} consts[] = {
	"PI",   3.14159265358979323846,
	"E",    2.71828182845905623536,
	"DEG", 57.29577951308232087680,
	"PHI",  1.61803398874989484820,
	"prev", 0.0,
	NULL,   0.0,
};

static struct builtin { /* built-ins-1 */
	char *name;
	double (*func)();
	int  type;
} builtins[] = {
	"rand",  Rand,     BLTIN0,
	"sin",   sin,      BLTIN1,
	"cos",   cos,      BLTIN1,
	"atan",  atan,     BLTIN1,
	"log",   log,      BLTIN1,
	"log10", log10,    BLTIN1,
	"exp",   exp,      BLTIN1,
	"sqrt",  sqrt,     BLTIN1,
	"int",   integer,  BLTIN1,
	"abs",   fabs,     BLTIN1,
	"atan2", atan2,    BLTIN2,
	"pow",   pow,      BLTIN2,
	NULL,    NULL,
};

void init(void)  /* install constants and built-ins in table */
{
	int i;
	Symbol *s;

	for (i = 0; consts[i].name != NULL; i++)
		install(consts[i].name, VAR, consts[i].cval);

	for (	struct builtin *p = builtins;
			p->name;
			p++)
	{
		s = install(p->name, p->type, 0.0);
		switch(p->type) {
			case BLTIN0: s->u.ptr0 = p->func; break;
			case BLTIN1: s->u.ptr1 = p->func; break;
			case BLTIN2: s->u.ptr2 = p->func; break;
		}
	}
}

double integer(double x)
{
	return (int) x;
}

double Rand(void)
{
	return (double) rand();
}
