%module template_typedef

//
// Change this to #if 1 to test the 'test'
//
#if 0

%{
#include <complex>
typedef std::complex<double> cmplx;
%}
  
%inline %{
  typedef cmplx complex;
%}

#else

%inline %{
#include <complex>
  typedef std::complex<double> complex;
%}

#endif


%inline %{

  namespace vfncs {

    struct UnaryFunctionBase
    {
    };    
    
    template <class ArgType, class ResType>
    class UnaryFunction;
    
    template <class ArgType, class ResType>
    class ArithUnaryFunction;  
    
    template <class ArgType, class ResType>
    struct UnaryFunction : UnaryFunctionBase
    {
    };

    template <class ArgType, class ResType>
    struct ArithUnaryFunction : UnaryFunction<ArgType, ResType>
    {
    };      
    
    template <class ArgType, class ResType>         
    struct unary_func_traits 
    {
      typedef ArithUnaryFunction<ArgType, ResType > base;
    };
  
    template <class ArgType>
    inline
    typename unary_func_traits< ArgType, ArgType >::base
    make_Identity()
    {
      return typename unary_func_traits< ArgType, ArgType >::base();
    }

    template <class Arg1, class Arg2>
    struct arith_traits 
    {
    };

    template<>
    struct arith_traits< double, double >
    {
    
      typedef double argument_type;
      typedef double result_type;
      static const char* const arg_type = "double";
      static const char* const res_type = "double";
    };

    template<>
    struct arith_traits< complex, complex >
    {
    
      typedef complex argument_type;
      typedef complex result_type;
      static const char* const arg_type = "complex";
      static const char* const res_type = "complex";
    };

    template<>
    struct arith_traits< complex, double >
    {
      typedef double argument_type;
      typedef complex result_type;
      static const char* const arg_type = "double";
      static const char* const res_type = "complex";
    };

    template<>
    struct arith_traits< double, complex >
    {
      typedef double argument_type;
      typedef complex result_type;
      static const char* const arg_type = "double";
      static const char* const res_type = "complex";
    };

    template <class AF, class RF, class AG, class RG>
    inline
    ArithUnaryFunction<typename arith_traits< AF, AG >::argument_type,
		       typename arith_traits< RF, RG >::result_type >
    make_Multiplies(const ArithUnaryFunction<AF, RF>& f,
		    const ArithUnaryFunction<AG, RG >& g)
    {
      return 
	ArithUnaryFunction<typename arith_traits< AF, AG >::argument_type,
	                   typename arith_traits< RF, RG >::result_type>();
    }
  }
%}

namespace vfncs {  
  %template(UnaryFunction_double_double) UnaryFunction<double, double >;  
  %template(ArithUnaryFunction_double_double) ArithUnaryFunction<double, double >;  
  %template() unary_func_traits<double, double >;
  %template() arith_traits<double, double >;
  %template(make_Identity_double) make_Identity<double >;

  %template(UnaryFunction_complex_complex) UnaryFunction<complex, complex >;  
  %template(ArithUnaryFunction_complex_complex) ArithUnaryFunction<complex, complex >;  

  %template() unary_func_traits<complex, complex >;
  %template() arith_traits<complex, complex >;
  %template(make_Identity_complex) make_Identity<complex >;

  /* [beazley] Added this part */
  %template() unary_func_traits<double,complex>;
  %template(UnaryFunction_double_complex) UnaryFunction<double,complex>;
  %template(ArithUnaryFunction_double_complex) ArithUnaryFunction<double,complex>;

  /* */

  %template() arith_traits<complex, double >;
  %template() arith_traits<double, complex >;

  %template(make_Multiplies_double_double_complex_complex)
    make_Multiplies<double, double, complex, complex>;

  %template(make_Multiplies_double_double_double_double)
    make_Multiplies<double, double, double, double>;

  %template(make_Multiplies_complex_complex_complex_complex)
    make_Multiplies<complex, complex, complex, complex>;

  %template(make_Multiplies_complex_complex_double_double)
    make_Multiplies<complex, complex, double, double>;

}

