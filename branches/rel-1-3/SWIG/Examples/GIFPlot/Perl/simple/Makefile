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
	TARGET='$(TARGET)' INTERFACE='$(INTERFACE)' perl5

static::
	$(MAKE) -f $(TOP)/Makefile SRCS='$(SRCS)' SWIG='$(SWIG)' \
	INCLUDES='$(INCLUDES)' LIBS='$(LIBS)' SWIGOPT='$(SWIGOPT)' \
	TARGET='myperl' INTERFACE='$(INTERFACE)' perl5_static

clean::
	rm -f *_wrap* *.o *~ *.so myperl *.pm .~* core *.gif

check: all
