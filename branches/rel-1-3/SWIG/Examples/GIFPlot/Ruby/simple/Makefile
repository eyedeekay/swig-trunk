TOP        = ../../..
SWIG       = $(TOP)/../swig
SWIGOPT    =
SRCS       =
TARGET     = simple
INTERFACE  = simple.i
LIBS       = -L../.. -lgifplot
INCLUDES   = -I../../Include

all::
	$(MAKE) -f $(TOP)/Makefile SRCS='$(SRCS)' SWIG='$(SWIG)' \
	INCLUDES='$(INCLUDES)' LIBS='$(LIBS)' SWIGOPT='$(SWIGOPT)' \
	TARGET='$(TARGET)' INTERFACE='$(INTERFACE)' ruby

static::
	$(MAKE) -f $(TOP)/Makefile SRCS='$(SRCS)' SWIG='$(SWIG)' \
	INCLUDES='$(INCLUDES)' LIBS='$(LIBS)' SWIGOPT='$(SWIGOPT)' \
	TARGET='myruby' INTERFACE='$(INTERFACE)' ruby_static

clean::
	rm -f *_wrap* *.o *~ *.so myruby .~* core *.gif

check: all
