MAKE_EXEC = $(MAKE) -f ../Makefile SRCS='' TARGET=test sub-all
MAKE_EXEC_CPLUSPLUS = $(MAKE) -f ../Makefile SRCS='' TARGET=test CC=c++ sub-all

all::  macro-1.test macro-1a.test macro-2.test cplusplus-throw.cplusplustest

.SUFFIXES: .i .test .cplusplustest

.i.test:
	echo Checking Examples/guile/test-suite/$<
	$(MAKE_EXEC) IFILE=$<

.i.cplusplustest:
	echo Checking Examples/guile/test-suite/$<
	$(MAKE_EXEC_CPLUSPLUS) IFILE=$< SWIGOPT=-c++

clean::
	rm -f matrix *_wrap* *~ .~* core

check: all
