TOP        = ../..
SWIG       = $(TOP)/../swig -shadow
SRCS       = example.cxx
TARGET     = libexample
INTERFACE  = example.i
SWIGOPT    = 

all::	java

java::
	$(MAKE) -f $(TOP)/Makefile SRCS='$(SRCS)' SWIG='$(SWIG)' \
	SWIGOPT='$(SWIGOPT)' TARGET='$(TARGET)' INTERFACE='$(INTERFACE)' java_cpp

clean::
	mv main.java main.java.tmp
	rm -f *_wrap* *.o core *~ *.so *.class *.java 
	mv main.java.tmp main.java

check: all
