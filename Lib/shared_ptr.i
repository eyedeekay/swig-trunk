// The main implementation detail in using this smart pointer of a type is to customise the code generated
// to use a pointer to the smart pointer of the type, rather than the usual pointer to the underlying type.
// So for some type T, shared_ptr<T> * is used rather than T *.

// shared_ptr namespaces could be boost or std or std::tr1
// For example for std::tr1, use:
// #define SWIG_SHARED_PTR_NAMESPACE std
// #define SWIG_SHARED_PTR_SUBNAMESPACE tr1

#if !defined(SWIG_SHARED_PTR_NAMESPACE)
# define SWIG_SHARED_PTR_NAMESPACE boost
#endif

#if defined(SWIG_SHARED_PTR_SUBNAMESPACE)
# define SWIG_SHARED_PTR_QNAMESPACE SWIG_SHARED_PTR_NAMESPACE::SWIG_SHARED_PTR_SUBNAMESPACE
#else
# define SWIG_SHARED_PTR_QNAMESPACE SWIG_SHARED_PTR_NAMESPACE
#endif

namespace SWIG_SHARED_PTR_NAMESPACE {
#if defined(SWIG_SHARED_PTR_SUBNAMESPACE)
  namespace SWIG_SHARED_PTR_SUBNAMESPACE {
#endif
    template <class T> class shared_ptr {
    };
#if defined(SWIG_SHARED_PTR_SUBNAMESPACE)
  }
#endif
}

%fragment("SWIG_null_deleter", "header") {
struct SWIG_null_deleter {
  void operator() (void const *) const {
  }
};
%#define SWIG_NO_NULL_DELETER_0 , SWIG_null_deleter()
%#define SWIG_NO_NULL_DELETER_1
%#define SWIG_NO_NULL_DELETER_SWIG_POINTER_NEW
%#define SWIG_NO_NULL_DELETER_SWIG_POINTER_OWN
}


// Main user macro for defining shared_ptr typemaps for both const and non-const pointer types
// For plain classes, do not use for derived classes
%define SWIG_SHARED_PTR(PROXYCLASS, TYPE...)
SWIG_SHARED_PTR_TYPEMAPS(PROXYCLASS, , TYPE)
SWIG_SHARED_PTR_TYPEMAPS(PROXYCLASS, const, TYPE)
%enddef

// Main user macro for defining shared_ptr typemaps for both const and non-const pointer types
// For derived classes
%define SWIG_SHARED_PTR_DERIVED(PROXYCLASS, BASECLASSTYPE, TYPE...)
SWIG_SHARED_PTR_TYPEMAPS(PROXYCLASS, , TYPE)
SWIG_SHARED_PTR_TYPEMAPS(PROXYCLASS, const, TYPE)
%types(SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< TYPE > = SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< BASECLASSTYPE >) %{
  *newmemory = SWIG_CAST_NEW_MEMORY;
  return (void *) new SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< BASECLASSTYPE >(*(SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< TYPE > *)$from);
%}
%extend TYPE {
  static SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< BASECLASSTYPE > SWIGSharedPtrUpcast(SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< TYPE > swigSharedPtrUpcast) {
    return swigSharedPtrUpcast;
  }
}
%enddef

