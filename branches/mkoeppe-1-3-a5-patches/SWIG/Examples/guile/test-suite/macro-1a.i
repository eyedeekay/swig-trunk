/* This interface file tests whether SWIG's extended C
   preprocessor is working right. 

   This is equivalent to macro-1 but using a different type of macro
   and {} instead of "". It works.
*/

#define FOO(x) gh_scm2int(x)

#define TEST_1(BAR) \
 %typemap (in) int FOOBAR {$target = BAR($source);}

TEST_1(FOO)

int foobar(int FOOBAR);

%{
  int foobar(int i)
  {}
%}
