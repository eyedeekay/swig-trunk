MAKE_EXEC = $(MAKE) -f ../Makefile SRCS='' TARGET=test sub-all

all::  macro-1.test macro-2.test cplusplus-throw.i

.SUFFIXES: .i .test

.i.test:
	$(MAKE_EXEC) IFILE=$<

clean::
	rm -f matrix *_wrap* *~ .~* core

check: all
