# config.mk -- configurable parameters for PACKAGE (see below)
# Author: Luis Colorado <luis.colorado@spindrive.fi>
# Date: Thu Dec 12 09:25:11 EET 2024
# Copyright: (c) 2024 SpinDrive Oy, FI.  All rights reserved.

PROGRAM_NAME             ?= hoc
BUILD_DATE               ?= $(build_date)
DOC_DATE                 ?= $(doc_date)
PACKAGE                  ?= $(PROGRAM_NAME)
AUTHOR_NAME              ?= Luis Colorado
AUTHOR_EMAIL             ?= luiscoloradourcola@gmail.com
COPYRIGHT_YEARS          ?= 2024-2025
PROGRAM_NAME_UC          ?= HOC
AUTHOR_CORP              ?= LUIS COLORADO SISTEMAS S.L.U.
AUTHOR_SITE              ?= https://github.com/mojadita/$(PACKAGE)
UQ_VERSION               ?= 12.1
VERSION                  ?= $(UQ_VERSION)
VERSION_DATE             ?= Mon Oct 27 13:53:33 -05 2025
OPERATING_SYSTEM         ?= UNIX

prefix                   ?= /usr/local
exec_prefix              ?= $(prefix)
bindir                   ?= $(prefix)/bin
sbindir                  ?= $(exec_prefix)/sbin
datarootdir              ?= $(prefix)/share
pkgdatadir               ?= $(datarootdir)/$(PACKAGE)
pkglibdir                ?= $(pkgdatadir)/plugins
pkgactivepluginsdir      ?= $(pkglibdir)/active
mandir                   ?= $(datarootdir)/man
man8dir                  ?= $(mandir)/man8
man1dir                  ?= $(mandir)/man1
docdir                   ?= $(datarootdir)/doc/$(PACKAGE)
vardir                   ?= $(exec_prefix)/var
logdir                   ?= $(vardir)/log

UQ_HOC_DEBUG             ?=  0
UQ_HOC_TRACE_PATCHING    ?=  0
UQ_TRACE_SYMBS           ?=  0
UQ_LEX_DEBUG             ?=  0
UQ_LEX_COMMENTS          ?=  0
UQ_CODE_DEBUG_EXEC       ?=  0
UQ_CODE_DEBUG_PROG       ?=  0
UQ_DEBUG_STACK           ?=  0

UQ_USE_COLORS            ?=  1
UQ_USE_LOCUS             ?=  0
UQ_USE_DEB               ?=  1
UQ_USE_INF               ?=  1
UQ_USE_WRN               ?=  1
UQ_USE_ERR               ?=  1
UQ_USE_CRT               ?=  1
UQ_NPROG                 ?= 0x800000
UQ_TAB_SIZE              ?= 4

UQ_LAST_TOKENS_SZ               ?=  64
UQ_DEFAULT_LOGLEVEL             ?=   0
UQ_MAX_SYMBOLS_PER_DECLARATION  ?=  32
UQ_SCOPES_INCRMNT               ?=  10
UQ_RETURNS_TO_PATCH_INCRMNT     ?=   4
UQ_INTERN_INCRMNT               ?=  10
UQ_ARGUMS_INCRMNT               ?=   8

UQ_COL1_SYMBS                   ?=  30
UQ_COL2_SYMBS                   ?= -18
UQ_COL3_SYMBS                   ?= -10
UQ_COL4_SYMBS                   ?= -10
UQ_COL5_SYMBS                   ?= -10

UQ_BRKPT_WIDTH1                 ?= -17
UQ_BRKPT_WIDTH2                 ?= -17
UQ_SIZE_FP_RETADDR              ?=   2
UQ_SUB_CALL_INCRMNT             ?=   8
UQ_BUILTINS_INCRMNT             ?=  64
FMT_CHAR                        ?= 0x%02hhx
FMT_DOUBLE                      ?= %#.15lg
FMT_FLOAT                       ?= %#.7g
FMT_INT                         ?= %i
FMT_LONG                        ?= %li
FMT_SHORT                       ?= 0x%04hx
