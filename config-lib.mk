# config-lib.mk -- include file for Makefile to prepare
# the environment to handle configuration parameters.
# Author: Luis Colorado <luis.colorado@spindrive.fi>
# Date: Thu Dec 12 09:12:05 EET 2024


build_date   != LANG=C date 
doc_date     != LANG=C date +'%b %Y'

SED_FLAGS    != ./mksedparms.sh < config.mk

include config.mk

%: %.in
	sed -E $(SED_FLAGS) < $< > $@
%.1:%.1.in
	sed -E $(SED_FLAGS) < $< > $@

config.h.in: ./config.h.in.sh ./config.mk
	./config.h.in.sh < ./config.mk > $@
toclean += config.h.in

config.h: config.h.in
	sed -E $(SED_FLAGS) < $< >$@
toclean += config.h

