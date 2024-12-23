RM   ?= rm -f
targets = hoc hoc-sin-prec
toclean += $(targets)

hoc_objs = hoc.o
hoc_libs = -lm
toclean += $(hoc_objs)

hoc-sin-prec_objs = hoc-sin-prec.o
hoc-sin-prec_libs = -lm
toclean += $(hoc-sin-prec_objs)

all: $(targets)
clean:
	$(RM) $(toclean)

hoc: $(hoc_objs)
	$(CC) $(LDFLAGS) -o $@ $(hoc_objs) $(hoc_libs)

hoc-sin-prec: $(hoc-sin-prec_objs)
	$(CC) $(LDFLAGS) -o $@ $(hoc-sin-prec_objs) $(hoc-sin-prec_libs)
