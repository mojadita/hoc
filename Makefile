##  nd = No Debugging    db = Debugging
DEBUG-TYPE    ?= db

CFLAGS-nd     ?= -O3
LDFLAGS-nd    ?=
CFLAGS-db     ?= -O0 -g
LDFLAGS-db    ?= -g

CFLAGS         = $(CFLAGS-$(DEBUG-TYPE))
LDFLAGS        = $(LDFLAGS-$(DEBUG-TYPE))

RM            ?= rm -f
targets        = hoc hoc.1.gz ack
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

toinstall     ?= $(bindir)/hoc $(man1dir)/hoc.1.gz

common_objs    = symbol.o init.o error.o math.o code.o $(WHICH_LEX) \
                reserved_words.o main.o do_help.o instr.o scope.o \
				intern.o type2inst.o
toclean       += $(common_objs) lex.c

hoc_objs       = hoc.o $(common_objs)
hoc_libs       = -lm 
toclean       += hoc.o

##  Crea un fichero donde se guarda la fecha hora de compilacion.
BUILD_DATE.txt: $(targets)
	date > $@
toclean += BUILD_DATE.txt

include ./config-lib.mk

install: $(toinstall)

uninstall:
	$(RM) $(toinstall)

$(bindir)/hoc: hoc
	-$(INSTALL) $(IFLAGS) -m $(XMOD) $? $@

$(man1dir)/hoc.1.gz: hoc.1.gz
	-$(INSTALL) $(IFLAGS) -m $(FMOD) $? $@

clean:
	$(RM) $(toclean)

hoc hoc.out: $(hoc_objs)
	$(CC) $(LDFLAGS) -o $@ $(hoc_objs) $(hoc_libs)

type2inst.c: type2inst.sh
	./type2inst.sh >$@

##
##  Crear un .c a partir de un .y
.y.c:
	$(YACC) -d $<
	mv -f y.tab.c $@
	mv -f y.tab.h $*.tab.h

hoc.tab.h: hoc.c
hoc.1: hoc.1.in config.mk
toclean += HOC.tab.h hoc.1 hoc.c

# ack.c code.c do_help.c error.c
# hoc.c init.c instr.c lex.c main.c math.c
# reserved_words.c symbol.c yylex.c

ack.o: ack.c 
code.o: code.c config.h colors.h hoc.h instr.h \
  instrucciones.h hoc.tab.h code.h
do_help.o: do_help.c config.h do_help.h
error.o: error.c config.h colors.h hoc.h instr.h \
  instrucciones.h error.h
hoc.o: hoc.c config.h colors.h hoc.h instr.h \
  instrucciones.h error.h math.h code.h 
init.o: init.c config.h hoc.h instr.h \
  instrucciones.h hoc.tab.h math.h code.h
instr.o: instr.c code.h hoc.h instr.h \
  instrucciones.h
lex.o: lex.c config.h hoc.h instr.h \
  instrucciones.h hoc.tab.h code.h \
  reserved_words.h 
main.o: main.c config.h colors.h do_help.h hoc.h \
  instr.h instrucciones.h code.h
math.o: math.c error.h
reserved_words.o: reserved_words.c hoc.h instr.h \
  instrucciones.h hoc.tab.h reserved_words.h
symbol.o: symbol.c hoc.h instr.h instrucciones.h \
  config.h colors.h hoc.tab.h
yylex.o: yylex.c hoc.h instr.h \
  instrucciones.h hoc.tab.h
