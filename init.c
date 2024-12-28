/* init.c -- funciones de inicializacion.
 * Date: Sat Dec 28 14:06:39 -05 2024
 */

#include "hoc.h"
#include "y.tab.h"
#include <math.h>

static struct { /* constants */
	char *name;
	double cval;
} consts[] = {
	"PI",   3.14159265358979323846,
	"E",    2.71828182845905623536,
	"DEG", 57.29577951308232087680,
	"PHI",  1.61803398874989484820,
	NULL,   0.0,
};

static struct { /* built-ins */
	char *name;
	double (*func)(double);
} builtins[] = {
	"sin",   sin,
	"cos",   cos,
	"atan",  atan,
	"log",   log,
	"log10", log10,
	"exp",   exp,
	"sqrt",  sqrt,
	"int",   integer,
	"abs",   fabs,
	NULL,    NULL,
};

void init(void)  /* install constants and built-ins in table */
{
	int i;
	Symbol *s;

	for (i = 0; consts[i].name != NULL; i++)
		install(const[i].name, VAR, consts[i].cval, NULL);

	for (i = 0; builtins[i].name, i++)
		s = install(builtins[i].name, BLTIN, 0.0, builtins[i].func);
}
