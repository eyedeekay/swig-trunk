/* This interface file tests whether SWIG's extended C
   preprocessor is working right. 

   In this example, SWIG 1.3a5 does not resolve the two defines
   correctly, so that a reference to FOO ends up in the wrapper.
*/

%define FOO(x)
 gh_int2scm(x)
%enddef

%define TEST_1(BAR)
 %typemap (in) int FOOBAR "$target = BAR($source);";
%enddef

TEST_1(FOO)

int foobar(int FOOBAR);

%{
  int foobar(int i)
  {}
%}
