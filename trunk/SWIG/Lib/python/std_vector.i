//
// std::vector
// Python implementation

%include std_container.i

// Vector

%define %std_vector_methods(vector)
  %std_sequence_methods(vector)
  
  void reserve(size_type n);
  size_type capacity() const;
%enddef


%define %std_vector_methods_val(vector)
  %std_sequence_methods_val(vector)
  
  void reserve(size_type n);
  size_type capacity() const;
%enddef


// ------------------------------------------------------------------------
// std::vector
// 
// The aim of all that follows would be to integrate std::vector with 
// Python as much as possible, namely, to allow the user to pass and 
// be returned Python tuples or lists.
// const declarations are used to guess the intent of the function being
// exported; therefore, the following rationale is applied:
// 
//   -- f(std::vector<T>), f(const std::vector<T>&):
//      the parameter being read-only, either a Python sequence or a
//      previously wrapped std::vector<T> can be passed.
//   -- f(std::vector<T>&), f(std::vector<T>*):
//      the parameter may be modified; therefore, only a wrapped std::vector
//      can be passed.
//   -- std::vector<T> f(), const std::vector<T>& f():
//      the vector is returned by copy; therefore, a Python sequence of T:s 
//      is returned which is most easily used in other Python functions
//   -- std::vector<T>& f(), std::vector<T>* f():
//      the vector is returned by reference; therefore, a wrapped std::vector
//      is returned
//   -- const std::vector<T>* f(), f(const std::vector<T>*):
//      for consistency, they expect and return a plain vector pointer.
// ------------------------------------------------------------------------

%{
#include <vector>
%}

    
%fragment("StdVectorTraits","header",fragment="StdSequenceTraits")
%{
  namespace swigpy {
    template <class T>
    struct traits_asptr<std::vector<T> >  {
      typedef std::vector<T> vector_type;
      typedef T value_type;
      static int asptr(PyObject *obj, vector_type **vec) {
	return traits_asptr_stdseq<vector_type>::asptr(obj, vec);
      }
    };

    template <class T>
    struct traits_from<std::vector<T> > {
      typedef std::vector<T> vector_type;
      static PyObject *from(const vector_type& vec) {
	return traits_from_stdseq<vector_type>::from(vec);
      }
    };
  }
%}

// exported classes

namespace std {

  template<class T > class vector {
  public:
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef T value_type;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef T& reference;
    typedef const T& const_reference;

    %traits_swigtype(T);

    %fragment(SWIG_Traits_frag(std::vector<T >), "header",
	      fragment=SWIG_Traits_frag(T),
	      fragment="StdVectorTraits") {
      namespace swigpy {
	template <>  struct traits<std::vector<T > > {
	  typedef pointer_category category;
	  static const char* type_name() {
	    return "std::vector<" #T " >";
	  }
	};
      }
    }

    %typemap_traits_ptr(SWIG_CCode(VECTOR), std::vector<T >);
  
    %std_vector_methods(vector);
    %pysequence_methods(std::vector<T >);
  };


  // ***
  // This especialization should dissapears or
  // get simplified when a 'const SWIGTYPE*&' can be
  // be defined
  // ***
  template<class T > class vector<T*> {
  public:
    typedef size_t size_type;    
    typedef ptrdiff_t difference_type;
    typedef T* value_type;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef value_type reference;
    typedef value_type const_reference;

    %traits_swigtype(T);

    %fragment(SWIG_Traits_frag(std::vector<T* >), "header",
	      fragment="StdVectorTraits") {
      namespace swigpy {
	template <>  struct traits<std::vector<T* > > {
	  typedef value_category category;
	  static const char* type_name() {
	    return "std::vector<" #T " * >";
	  }
	};
      }
    }

    %typemap_traits_ptr(SWIG_CCode(VECTOR), std::vector<T* >);

    %std_vector_methods_val(vector);
    %pysequence_methods_val(std::vector<T* >);
  };

  // Add the order operations <,>,<=,=> as needed
  
  %define %std_order_vector(T)
    %std_comp_methods(vector<T>);
  %enddef
  
  %apply_otypes(%std_order_vector);

  // bool specialization
  %extend vector<bool> {
    void flip() 
    {
      self->flip();
    }
  }

}


%define %std_vector_ptypen(...) 
  namespace std {
    %template() vector<__VA_ARGS__ >;
  }
%enddef

%apply_cpptypes(%std_vector_ptypen);

