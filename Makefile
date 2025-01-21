RM            ?= rm -f
targets        = hoc hoc-sin-prec hoc.1.gz
toclean       += $(targets)
OS            != uname -o

prefix        ?= /usr/local
exec_prefix   ?= $(prefix)
bindir        ?= $(exec_prefix)/bin
datarootdir   ?= $(prefix)/share
mandir        ?= $(datarootdir)/man
man1dir       ?= $(mandir)/man1

OWN-GNU/Linux ?= root
GRP-GNU/Linux ?= bin

OWN-FreeBSD   ?= bin
GRP-FreeBSD   ?= wheel

FMOD          ?= 0644
XMOD          ?= 0755

INSTALL       ?= install
IFLAGS        ?= -o $(OWN-$(OS)) -g $(GRP-$(OS))

toinstall ?= $(bindir)/hoc $(bindir)/hoc-sin-prec $(man1dir)/hoc.1.gz

common_objs = symbol.o init.o error.o math.o lex.o
toclean += $(common_objs) lex.c

hoc_objs = hoc.o $(common_objs)
hoc_libs = -lm 
toclean += hoc.o hoc.c y.tab.h

hoc-sin-prec_objs = hoc-sin-prec.o $(common_objs)
hoc-sin-prec_libs = -lm 
toclean          += hoc-sin-prec.o hoc-sin-prec.c

##  Crea un fichero donde se gurda la fecha hora de compilacion.
BUILD_DATE.txt: $(targets)
	date > $@

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

y.tab.h hoc.c: hoc.y
	$(YACC) -d hoc.y
	mv -f y.tab.c hoc.c

hoc.o: hoc.c hoc.h
symbol.o: symbol.c hoc.h
init.o: init.c hoc.h y.tab.h
yylex.o: hoc.h y.tab.h
