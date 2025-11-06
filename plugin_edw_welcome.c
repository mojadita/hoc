/* plugin_edw_welcome.c -- Imprime mensaje de bienveida a Hoc
 * Author: Luis Colorado <luiscoloradourcola@gmail.com>
 * Date: Tue Oct 14 14:59:07 -05 2025
 * Copyright: (c) 2025 Edward Rivas - Luis Colorado.  All rights reserved.
 * License: BSD
 */

/*
   Objetivo:
   Mostrar un mensaje de bienvenida a la calculador
   y maquina virtual Hoc.

   construccion:

   "Construyendo Plugin_edw_welcome.so  (FORMA_DIRECTA)"
   $  gcc -fPIC -shared plugin_edw_welcome.c -o plugin_edw_welcome.so

   "Construyendo Plugin_edw_welcome.so  (FORMA_INDIRECTA)"  <-- OK.
   $  gcc -fPIC -c plugin_edw_welcome.c -o plugin_edw_welcome.pico
   $  ld -shared plugin_edw_welcome.pico -o plugin_edw_welcome.so
   "Instalando Plugin_edw_welcome.so"
   $  mv plugin_edw_welcome.so ./plugins
*/


#include  <stdio.h>

#include "config.h"
#include "colors.h"
#include "do_help.h"


/*  Modificacion hecha por Mi  */
//void __stack_chk_fail_local(){ }


void _init(void);


/* La rutina que dlopen() ejecuta automaticamente se llama
 * _init, pero es necesario enlazar el .so llamando al
 * linker ld(1) directamente, para que no cargue el modulo
 * crti0.o (que contiene un _init()).
 * Esto no afecta al modulo, que incluso sera mas peque;o,
 * de hecho. Esto se hace asi en el Makefile ahora. */
void _init(void)
{
    printf( "Hello Hoc'User !!. Arriba cachipurriana\n" );
    fprintf( stderr,
        "Welcome to '" BRIGHT "%s" ANSI_END " v%.1f' Calculator, "
        "Compiler and Executable language.\n",
        PROGRAM_NAME, UQ_VERSION );
}
