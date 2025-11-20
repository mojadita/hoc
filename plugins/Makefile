# Makefile -- build script for hoc plugins.
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
targets           = plugin0.so plugin_edw_welcome.so
toclean          += $(targets)

.SUFFIXES: .out .so .o .pico .c
.PHONY: clean install uninstal deinstall

OWN-GNU/Linux ?= root
GRP-GNU/Linux ?= bin

OWN-FreeBSD   ?= bin
GRP-FreeBSD   ?= wheel

FMOD          ?= 0644
XMOD          ?= 0755
DMOD          ?= 0755

INSTALL       ?= install
IFLAGS        ?= -o $(OWN-$(OS)) -g $(GRP-$(OS))

include ../config.mk

toinstall     ?= $(pkglibdir)/plugin0.so \
                 $(pkglibdir)/plugin_edw_welcome.so \
                 $(pkgactivepluginsdir)

plugin0.so_objs    = plugin0.pico
plugin0.so_ldfl    = -shared -soname=plugin0.so
toclean           += $(plugin0.so_objs) plugin0.so

plugin_edw_welcome.so_objs = plugin_edw_welcome.pico
plugin_edw_welcome.so_ldfl = -shared -soname=plugin_edw_welcome.so
toclean                   += $(plugin_edw_welcome.so_objs) \
                             plugin_edw_welcome.so

##  Crea un fichero donde se guarda la fecha hora de compilacion.
BUILD_DATE.txt: $(targets) $(plugins)
	@date > $@
	@echo -n "$(.CURDIR): Built on: "
	@cat $@
toclean += BUILD_DATE.txt

install: $(toinstall)

uninstall:
	$(RM) $(toinstall)

$(pkglibdir)/plugin0.so \
$(pkglibdir)/plugin_edw_welcome.so : $(@:T)
	-$(INSTALL) $(IFLAGS) -m $(XMOD) $(@:T) $@

$(pkgactivepluginsdir) \
$(pkglibdir):
	-$(INSTALL) $(IFLAGS) -m $(DMOD) -d $@

clean:
	$(RM) $(toclean)

plugin0.so: $(plugin0.so_deps) $(plugin0.so_objs)
	$(LD) $(LDFLAGS) $($@_ldfl) $($@_objs) -o $@

plugin_edw_welcome.so: $(plugin_edw_welcome.so_deps) $(plugin_edw_welcome.so_objs)
	$(LD) $(LDFLAGS) $($@_ldfl) $($@_objs) -o $@


# REGLAS IMPLICITAS

.c.pico:
	$(CC) $(CFLAGS) $($@_cflgs) -fPIC -c $< -o $@

plugin0.pico: plugin0.c plugins.h builtins.h \
  instr.h instrucciones.h cell.h symbol.h \
  types.h config.h cellP.h code.h hoc.h lex.h

plugin_edw_welcome.pico: plugin_edw_welcome.c \
  config.h colors.h do_help.h
