MAKE_EXEC = $(MAKE) -f ../Makefile SRCS='' sub-all
MAKE_EXEC_CPLUSPLUS = $(MAKE) -f ../Makefile SRCS='' CC=c++ sub-all

all::   macro-1.test macro-1a.test macro-2.test \
	cplusplus-throw.cplusplustest \
	char-constant.runtest \
	pointer-reference.cplusplustest

.SUFFIXES: .i .test .cplusplustest .runtest

.i.test:
	@echo Checking Examples/guile/test-suite/$<
	@$(MAKE_EXEC) IFILE=$< TARGET=$@

.i.cplusplustest:
	@echo Checking Examples/guile/test-suite/$<
	@$(MAKE_EXEC_CPLUSPLUS) IFILE=$< SWIGOPT=-c++ TARGET=$@

.i.runtest:
	@echo Checking Examples/guile/test-suite/$<
	@$(MAKE_EXEC) IFILE=$< TARGET=$@
	@./$@ -c $*.scm

clean::
	rm -f matrix *_wrap* *~ .~* core

check: all
