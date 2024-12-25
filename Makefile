RM      ?= rm -f
targets  = hoc hoc-sin-prec
toclean += $(targets)

hoc_objs = hoc.o
hoc_libs = -lm
toclean += $(hoc_objs) hoc.c y.tab.h

hoc-sin-prec_objs = hoc-sin-prec.o
hoc-sin-prec_libs = -lm
toclean          += $(hoc-sin-prec_objs) hoc-sin-prec.c

##  Crea un fichero donde se gurda la fecha hora de compilacion.
BUILD_DATE.txt: $(targets)
	date > $@

clean:
	$(RM) $(toclean)

hoc: $(hoc_objs)
	$(CC) $(LDFLAGS) -o $@ $(hoc_objs) $(hoc_libs)

hoc-sin-prec: $(hoc-sin-prec_objs)
	$(CC) $(LDFLAGS) -o $@ $(hoc-sin-prec_objs) $(hoc-sin-prec_libs)
