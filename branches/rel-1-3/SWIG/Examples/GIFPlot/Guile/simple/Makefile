TOP        = ../../..
SWIG       = $(TOP)/../swig
SWIGOPT    =
SRCS       =
TARGET     = simple
INTERFACE  = simple.i
LIBS       = -L../.. -lgifplot
INCLUDES   = -I../../Include

all:: static

dynamic::
	$(MAKE) -f $(TOP)/Makefile TOP='$(TOP)' \
	SRCS='$(SRCS)' SWIG='$(SWIG)' \
	INCLUDES='$(INCLUDES)' LIBS='$(LIBS)' SWIGOPT='$(SWIGOPT)' \
	TARGET='$(TARGET)' INTERFACE='$(INTERFACE)' guile

static::
	$(MAKE) -f $(TOP)/Makefile TOP='$(TOP)' \
	SRCS='$(SRCS)' SWIG='$(SWIG)' \
	INCLUDES='$(INCLUDES)' LIBS='$(LIBS)' SWIGOPT='$(SWIGOPT)' \
	TARGET='$(TARGET)' INTERFACE='$(INTERFACE)' guile_static

clean::
	rm -f *_wrap* *.o *~ *.so gifguile .~* core *.gif

check: all
