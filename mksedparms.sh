#!/bin/sh
# mksedparms.sh -- generate from config.mk the set of sed parameters
# to use in Makefile to change configuration options into their
# configured values.  The script is a filter, reads its standard input
# and writes to standard output, so maintainance is simpler.
# Author: Luis Colorado <luiscoloradourcola@gmail.com>
#       & Edward Rivas <rivastkw@gmail.com>
# Date: Mon Jun 22 03:05:30 EEST 2026
# Copyright: (c) 2025-2026 Luis Colorado.  All rights reserved.
# License: BSD.

sed -Ee '/^[	 ]*(#.*)?$/d' \
     -e 's"^[	 ]*([A-Za-z_][A-Za-z0-9_]*)[	 ]*\??=[	 ]*(.*)$"-e '\''s\"@\1@\"\2\"g'\''"'

echo
