CFLAGS         = -O2
RM            ?= rm -f
targets        = hoc hoc-sin-prec hoc.1.gz
toclean       += $(targets)
OS            != uname -o

WHICH_LEX	  ?= lex.o

OWN-GNU/Linux ?= root
GRP-GNU/Linux ?= bin

OWN-FreeBSD   ?= bin
GRP-FreeBSD   ?= wheel

FMOD          ?= 0644
XMOD          ?= 0755

INSTALL       ?= install
IFLAGS        ?= -o $(OWN-$(OS)) -g $(GRP-$(OS))

toinstall     ?= $(bindir)/hoc $(bindir)/hoc-sin-prec $(man1dir)/hoc.1.gz

common_objs    = symbol.o init.o error.o math.o code.o $(WHICH_LEX) reserved_words.o main.o do_help.o instr.o
toclean       += $(common_objs) lex.c

hoc_objs       = hoc.o $(common_objs)
hoc_libs       = -lm 
toclean       += hoc.o hoc.c y.tab.h

hoc-sin-prec_objs = hoc-sin-prec.o $(common_objs)
hoc-sin-prec_libs = -lm 
toclean          += hoc-sin-prec.o hoc-sin-prec.c

##  Crea un fichero donde se gurda la fecha hora de compilacion.
BUILD_DATE.txt: $(targets)
	date > $@
toclean += BUILD_DATE.txt

include ./config-lib.mk

install: $(toinstall)

uninstall:
	$(RM) $(toinstall)

.SUFFIXES: .1.gz .1

.1.1.gz:
	gzip < $< > $@

$(bindir)/hoc: hoc
	-$(INSTALL) $(IFLAGS) -m $(XMOD) $? $@

$(bindir)/hoc-sin-prec: hoc-sin-prec
	-$(INSTALL) $(IFLAGS) -m $(XMOD) $? $@

$(man1dir)/hoc.1.gz: hoc.1.gz
	-$(INSTALL) $(IFLAGS) -m $(FMOD) $? $@

clean:
	$(RM) $(toclean)

hoc: $(hoc_objs)
	$(CC) $(LDFLAGS) -o $@ $(hoc_objs) $(hoc_libs)

hoc-sin-prec: $(hoc-sin-prec_objs)
	$(CC) $(LDFLAGS) -o $@ $(hoc-sin-prec_objs) $(hoc-sin-prec_libs)

##
##  Crear un .c a partir de un .y
.y.c:
	$(YACC) -d $<
	mv -f y.tab.c $@
	mv -f y.tab.h $*.tab.h

hoc.tab.h: hoc.c
hoc.1: hoc.1.in

# ack.c code.c do_help.c error.c hoc-sin-prec.c hoc.c init.c lex.c
# main.c math.c pepe.c reserved_words.c symbol.c yylex.c
ack.o: ack.c 
code.o: code.c config.h hoc.h hoc.tab.h colors.h
do_help.o: do_help.c config.h do_help.h
error.o: error.c hoc.h error.h config.h colors.h
hoc-sin-prec.o: hoc-sin-prec.c config.h hoc.h error.h math.h code.h colors.h
hoc.o: hoc.c config.h hoc.h error.h math.h code.h colors.h
init.o: init.c config.h hoc.h hoc.tab.h math.h code.h
lex.o: lex.c config.h hoc.h hoc.tab.h code.h reserved_words.h 
main.o: main.c config.h do_help.h hoc.h code.h colors.h
math.o: math.c error.h
reserved_words.o: reserved_words.c hoc.h hoc.tab.h reserved_words.h
symbol.o: symbol.c hoc.h hoc.tab.h config.h colors.h
yylex.o: yylex.c hoc.h hoc.tab.h
