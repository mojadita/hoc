# Makefile -- Build script for tests.
# Author: Edward Rivas <rivastkw@gmail.com>
# Date: Mon Jun	 2 11:23:11 -05 2025
# Copyright: (c) 2025 Edward Rivas.	 All rights reserved.
# License: BSD
#

##	nd = No Debugging	 db = Debugging
DEBUG-TYPE	   = db

CFLAGS-nd	   = -O3
LDFLAGS-nd	   =
CFLAGS-db	   = -O0 -g
LDFLAGS-db	   = -g

CFLAGS		   = $(CFLAGS-$(DEBUG-TYPE)) -I/usr/local/include
LDFLAGS		   = $(LDFLAGS-$(DEBUG-TYPE)) -L/usr/local/lib

RM			  ?= rm -f
targets		   = test-hash test01
toclean		  += $(targets)
OS			  != uname -o

common_objs	   =  hash.o
toclean		  += $(common_objs)

test01_objs = test01.o
test-hash_objs = test-hash.o
test-hash_libs = -lgtest -lgtest_main
toclean		  += $(test-hash_objs)

all: $(targets)

include ./config-lib.mk

clean:
	$(RM) $(toclean)

test: $(targets)
	for t in $(targets); \
	do $$t $(TSTFLAGS); \
	done

test01: $(test01_objs) $(common_objs)
	$(CXX) $(LDFLAGS) -o $@ $($@_objs) $(common_objs) $($@_libs)

test-hash: $(test-hash_objs) $(common_objs)
	$(CXX) $(LDFLAGS) -o $@ $($@_objs) $(common_objs) $($@_libs)
