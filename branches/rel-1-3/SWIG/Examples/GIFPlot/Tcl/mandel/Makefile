TOP        = ../../..
SWIG       = $(TOP)/../swig
SWIGOPT    = -I../../Interface
SRCS       =
TARGET     = gifplot
INTERFACE  = mandel.i
LIBS       = -L../.. -lgifplot
INCLUDES   = -I../../Include

all::
	$(MAKE) -f $(TOP)/Makefile SRCS='$(SRCS)' SWIG='$(SWIG)' \
	INCLUDES='$(INCLUDES)' LIBS='$(LIBS)' SWIGOPT='$(SWIGOPT)' \
	TARGET='$(TARGET)' INTERFACE='$(INTERFACE)' tcl

static::
	$(MAKE) -f $(TOP)/Makefile SRCS='$(SRCS)' SWIG='$(SWIG)' \
	INCLUDES='$(INCLUDES)' LIBS='$(LIBS)' SWIGOPT='$(SWIGOPT)' \
	TARGET='mywish' INTERFACE='$(INTERFACE)' wish

clean::
	rm -f *_wrap* *.o *~ *.so mywish  .~* core *.gif

check: all
