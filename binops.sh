# binops.sh -- extraccion de las funciones binop.
# Author: Luis Colorado <luiscoloradourcola@gmail.com>
# Date: Fri Nov  7 17:46:58 EET 2025
# Copyright: (c) 2025 Luis Colorado.  All rights reserved.
# License: BSD
#

sp='[ 	]*'
id='[a-zA-Z_][a-zA-Z0-9_]*'
comma="${sp},${sp}"
suff='_[cdfilh]'

binop_extract() {
	grep "^\s*BINOP_EVAL(${sp}${id}${comma}${1}${comma}" <binop_evals.h \
	| sed -E "s/^${sp}BINOP_EVAL\(${sp}(${id})${comma}(${suff})${comma}.*$/    .\1_binop\2 = \1_binop\2,/"
}
binop_extract _c
binop_extract _f
