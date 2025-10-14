
/* plugin_edw_welcome.c -- Imprime mensaje de bienveida a Hoc
 * Author: Luis Colorado <luiscoloradourcola@gmail.com>
 * Date: Tue Oct 14 14:59:07 -05 2025
 * Copyright: (c) 2025 Edward Rivas - Luis Colorado.  All rights reserved.
 * License: BSD
 */

#include  <stdio.h>

#include "config.h"
#include "colors.h"
#include "do_help.h"

void _init(void);


void _init(void)
{
    printf( "Hello HOC\n" );
    fprintf( stderr,
        "Welcome to '" BRIGHT "%s" ANSI_END " v%.1f' Calculator, "
        "Compiler and Executable language.\n",
        PROGRAM_NAME, UQ_VERSION );
}
