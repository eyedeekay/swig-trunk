%module sizet
%{
#include <vector>
%}

%include "std_common.i"
%include "std_vector.i"

%inline
{
  size_t test1(size_t s)
  {
    return s;
  }

  std::size_t test2(std::size_t s)
  {
    return s;
  }

  const std::size_t& test3(const std::size_t& s)
  {
    return s;
  }

}

#ifdef SWIGPYTHON
%template(vectors) std::vector<unsigned long>;
  
%inline 
{
  std::vector<std::size_t> testv1(std::vector<std::size_t> s)
  {
    return s;
  }

  const std::vector<std::size_t>& testv2(const std::vector<std::size_t>& s)
  {
    return s;
  }

}
#endif
