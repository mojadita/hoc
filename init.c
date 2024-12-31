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
    "PI",   M_PI,
    "E",    M_E,
    "DEG",  180.0/M_PI,
    "PHI",  1.61803398874989484820,
    "prev", 0.0,
    NULL,   0.0,
};

/* algunas definiciones que no estan en a libreria math */
/*  Confirmacion de las operaciones para obtener
 *  el arcoseno, arcocoseno
 *  cos(x)^2 + sin(x)^2 = 1
 *  cos(x)^2 = 1 - sin(x)^2
 *  cos(x) = sqrt(1 - sin(x)^2)
 *  tan(x) = sin(x) / cos(x)
 *  tan(x) = sin(x) / sqrt(1 - sin(x)^2)
 *  sx = sin(x)
 *  tan(x) = sx / sqrt(1 - sx^2)
 *  atan(tan(x)) = atan(sx / sqrt(1 - sx^2))
 *  x = atan(sx / sqrt(1 - sx^2))
 *  asin(sx) = asin(sin(x)) = x
 *  asin(sx) = atan(sx / sqrt(1 - sx^2))
 */
double asin(double sx)
{
    return atan(sx/sqrt(1-sx*sx));
} /* asin */

/*  sin(x)^2 = 1 - cos(x)^2
 *  sin(x) = sqrt(1 - cos(x)^2)
 *  tan(x) = sin(x) / cos(x) = sqrt(1 - cos(x)^2) / cos(x)
 *  cx = cos(x)  -->  x = acos(cx)
 *  tan(x) = sqrt(1 - cx^2) / cx
 *  atan(tan(x)) = atan(sqrt(1 - cx^2) / cx)
 *  x = atan(sqrt(1 - cx^2) / cx)
 *  acos(cx) = atan(sqrt(1 - cx^2) / cx)
 */
double acos(double cx)
{
    return atan(sqrt(1 - cx*cx) / cx);
} /* acos */

double Sqrt(double x)
{
    if (x < 0.0)
        execerror("Raiz de numero < 0");
    return sqrt(x);
}

double inverso( double x )
{
    return 1/x;
}

double opuesto( double x )
{
    return -x;
}

static struct builtin { /* built-ins-1 */
    char *name;
    double (*func)();
    int  type;
} builtins[] = {
    "rand",  Rand,     BLTIN0,
    "sin",   sin,      BLTIN1,
    "cos",   cos,      BLTIN1,
    "tan",   tan,      BLTIN1,
    "asin",  asin,     BLTIN1,
    "acos",  acos,     BLTIN1,
    "atan",  atan,     BLTIN1,
    "log",   log,      BLTIN1,
    "log10", log10,    BLTIN1,
    "exp",   exp,      BLTIN1,
    "sqrt",  Sqrt,     BLTIN1,
    "int",   integer,  BLTIN1,
    "abs",   fabs,     BLTIN1,
    "atan2", atan2,    BLTIN2,
    "pow",   pow,      BLTIN2,
    "inv",   inverso,  BLTIN1,
    "ops",   opuesto,  BLTIN1,
    NULL,    NULL,
};

void init(void)  /* install constants and built-ins in table */
{
    int i;
    Symbol *s;

    for (i = 0; consts[i].name != NULL; i++)
        install(consts[i].name, VAR, consts[i].cval);

    for (   struct builtin *p = builtins;
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
