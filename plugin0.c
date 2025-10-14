/* plugin0.c -- Sample test plugin to run in hoc.
 * Author: Edward Rivas <rivastkw@gmail.com>
 * Date: Tue Oct 14 06:01:58 -05 2025
 * Copyright: (c) 2025 Edward Rivas.  All rights reserved.
 * License: BSD
 */

#include <stdio.h>

/* La rutina que dlopen() ejecuta automaticamente se llama
 * _init, pero es necesario enlazar el .so llamando al
 * linker ld(1) directamente, para que no cargue el modulo
 * crti0.o (que contiene un _init()).
 * Esto no afecta al modulo, que incluso sera mas peque;o,
 * de hecho. Esto se hace asi en el Makefile ahora. */
int _init()
{
	fprintf(stderr, "Test plugin v1.0\n");
	return 0;
}
