MAKE_EXEC = $(MAKE) -f ../Makefile SRCS='' sub-all
MAKE_EXEC_CPLUSPLUS = $(MAKE) -f ../Makefile SRCS='' CC=c++ sub-all

all::   macro-1.test macro-1a.test macro-2.test \
	long-long.test \
	cplusplus-throw.cplusplustest \
	char-constant.runtest \
	pointer-reference.cplusplustest

.SUFFIXES: .i .test .cplusplustest .runtest

.i.test:
	@echo Checking Examples/guile/test-suite/$<
	@$(MAKE_EXEC) IFILE=$< SWIGOPT="-lguilemain.i" TARGET=$@-x || (echo "Examples/guile/test-suite/$<: FAILED"; exit 1)
	@echo "Examples/guile/test-suite/$<: PASSED"

.i.cplusplustest:
	@echo Checking Examples/guile/test-suite/$<
	@$(MAKE_EXEC_CPLUSPLUS) IFILE=$< SWIGOPT="-lguilemain.i -c++" TARGET=$@-x || (echo "Examples/guile/test-suite/$<: FAILED"; exit 1)
	@echo "Examples/guile/test-suite/$<: PASSED"

.i.runtest:
	@echo Checking Examples/guile/test-suite/$<
	@$(MAKE_EXEC) IFILE=$< SWIGOPT="-lguilemain.i" TARGET=$@-x || echo "Examples/guile/test-suite/$<: FAILED"
	@./$@-x -l $*.scm || (echo "Examples/guile/test-suite/$<: FAILED"; exit 1)
	@echo "Examples/guile/test-suite/$<: PASSED"

clean::
	rm -f matrix *_wrap* *test-x *~ .~* core

check: all
