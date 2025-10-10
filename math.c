/* math.c -- funciones matematicas y wrappers de funciones con
 * chequeo del dominio.
 * Author: Edward Rivas <rivastkw@gmail.com>
 *       y Luis Colorado <luiscoloradourcola@gmail.com>
 * Date: Tue Dec 31 17:36:17 -05 2024
 * Copyright: (c) 2025 Edward Rivas y Luis Colorado.  All rights reserved.
 * License: BSD.
 */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "error.h"

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


/*  Pow y Sqrt son wrappers o envoltorios para las funciones
    originales de la libreria math.h  */
double Pow(double b, double e)
{
    if (b == 0 && e == 0)
        execerror("indeterminacion tipo 0^0");
    return pow(b, e);
}

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

double Rand(void)
{
    return rand() / 2.147483648E9;
}

double rd(void)
{
    double v;
    if (scanf("%lg", &v) != 1) {
        execerror("couldn't parse a double\n");
    }
    return v;
}

double now(void)
{
    double n = time(NULL);
    return (double)n;
}

double integer(double x)
{
    return (int) x;
}

long fast_pwr_l(long x, int e)
{
    unsigned mask;

    if (x == 0.0 || e < 0)
		execerror("x(%li) ^^ y(%i) must "
			"have x != 0 and y >= 0 or "
			"use floating point",
			x, e);

    for (mask = 1; mask <= e; mask <<= 1)
        continue;
    mask >>= 1;

    double ret_val = 1.0;
    while (mask) {
        ret_val *= ret_val;
        if (mask & e) {
            ret_val *= x;
        }
        mask >>= 1;
    }
    return ret_val;
}
