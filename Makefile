# Makefile -- build script for hoc.
# Author: Luis Colorado <luiscoloradourcola@gmail.com>
# Date: Tue Oct 14 13:29:59 EEST 2025
# Copyright: (c) 2025 Luis Colorado.  All rights reserved.
# License: BSD
#

##  nd = No Debugging    db = Debugging
DEBUG-TYPE       ?= db
OS               != uname -o

CFLAGS-nd        ?= -O3
LDFLAGS-nd       ?=
CFLAGS-db        ?= -O0 -g
LDFLAGS-db       ?= -g

CFLAGS            = $(CFLAGS-$(DEBUG-TYPE))
LDFLAGS           = $(LDFLAGS-$(DEBUG-TYPE))

LIBS             ?= -lm

RM               ?= rm -f
targets           = hoc hoc.1.gz ack
plugins           = plugin0.so plugin_edw_welcome.so
toclean          += $(targets) $(plugins)

.SUFFIXES: .out .so .o .pico .c .l .y
.PHONY: clean install uninstal deinstall

SUBDIRS           = plugins

OWN-GNU/Linux ?= root
GRP-GNU/Linux ?= bin

OWN-FreeBSD   ?= bin
GRP-FreeBSD   ?= wheel

FMOD          ?= 0644
XMOD          ?= 0755
DMOD          ?= 0755

INSTALL       ?= install
IFLAGS        ?= -o $(OWN-$(OS)) -g $(GRP-$(OS))

toinstall     ?= $(bindir)/hoc \
                 $(man1dir)/hoc.1.gz

hoc_deps           =
hoc_objs           = hoc.o symbol.o init.o error.o math.o code.o lex.o \
                     reserved_words.o main.o do_help.o instr.o scope.o \
                     intern.o type2inst.o types.o builtins.o binop_eval.o
hoc_ldfl           = -Wl,--export-dynamic
hoc_libs-GNU/Linux = -ldl
hoc_libs-FreeBSD   =
hoc_libs           = $(hoc_libs-$(OS))
toclean           += $(hoc_objs) lex.c

##  Crea un fichero donde se guarda la fecha hora de compilacion.
BUILD_DATE.txt: $(targets) $(SUBDIRS)
	@date > $@
	@echo -n "$(.CURDIR): Built on: "
	@cat $@
toclean += BUILD_DATE.txt

$(SUBDIRS)::
	$(MAKE) -C $@

include ./config-lib.mk

install: $(toinstall)
	-@for i in $(SUBDIRS); \
	do \
		echo $(MAKE) -C $$i install; \
		$(MAKE) -C $$i install; \
	done

uninstall:
	$(RM) $(toinstall)
	-@for i in $(SUBDIRS); \
	do \
		echo $(MAKE) -C $$i $@; \
		$(MAKE) -C $$i $@; \
	done

$(bindir)/hoc: $(@:T) $(@:H)
	-$(INSTALL) $(IFLAGS) -m $(XMOD) $(@:T) $@

$(man1dir)/hoc.1.gz: $(@:T)
	-$(INSTALL) $(IFLAGS) -m $(FMOD) $(@:T) $@

$(pkgactivepluginsdir) \
$(pkglibdir):
	-$(INSTALL) $(IFLAGS) -m $(DMOD) -d $@

clean:
	$(RM) $(toclean)

hoc hoc.out: $(hoc_objs)
	$(CC) $(LDFLAGS) $($@_ldfl) -o $@ $(hoc_objs) $(hoc_libs) $(LIBS)

plugin0.so: $(plugin0.so_deps) $(plugin0.so_objs)
	$(LD) $(LDFLAGS) $($@_ldfl) $($@_objs) -o $@

plugin_edw_welcome.so: $(plugin_edw_welcome.so_deps) $(plugin_edw_welcome.so_objs)
	$(LD) $(LDFLAGS) $($@_ldfl) $($@_objs) -o $@

type2inst.c: instrucciones.h binop_evals.h type2inst.sh
	./type2inst.sh >$@
toclean += type2inst.c

# REGLAS IMPLICITAS

hoc.c: hoc.y
	$(YACC) -d $?
	mv y.tab.c hoc.c
	mv y.tab.h hoc.tab.h
toclean += hoc.tab.h hoc.c

lex.o reserved_words.o scope.o: hoc.c

hoc.1: hoc.1.in config.mk
toclean += hoc.1

-include .depend
