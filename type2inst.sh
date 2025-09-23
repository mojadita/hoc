# type2inst.sh -- busca instrucciones de tipo _x en el fichero
#                 instrucciones.h e imprime a la salida los inicializadores
#                 de las tablas de conversion de tipo -> instruccion.
# Author: Edward Rivas <rivastkw@gmail.com>
# Date: Tue Sep 23 13:26:19 -05 2025
# Copyright: (c) 2025 Edward Rivas.  All rights reserved.
# License: BSD
#

grep "INST([a-zA-Z_][_A-Za-z0-9]*$1," instrucciones.h \
| sed -e "s/INST(\([a-zA-Z_][_A-Za-z0-9]*\)$1,.*/    .\1        = instruction_set + INST_\1$1,/"
