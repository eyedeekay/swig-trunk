TOP        = ../../..
SWIG       = $(TOP)/../swig
SWIGOPT    = -I../../Include -noproxy
SRCS       =
TARGET     = php_gifplot
INTERFACE  = gifplot.i
LIBS       = -L../.. -lgifplot -lm
INCLUDES   = -I../../Include

all::
	$(MAKE) -f $(TOP)/Makefile SRCS='$(SRCS)' SWIG='$(SWIG)' \
	INCLUDES='$(INCLUDES)' LIBS='$(LIBS)' SWIGOPT='$(SWIGOPT)' \
	TARGET='$(TARGET)' INTERFACE='$(INTERFACE)' php4

clean::
	rm -f *_wrap* *.o *~ *.so .~* core *.gif *.h *.php

check: all
