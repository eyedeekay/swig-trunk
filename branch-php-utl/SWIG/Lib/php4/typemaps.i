/* -----------------------------------------------------------------------------
 * See the LICENSE file for information on copyright, usage and redistribution
 * of SWIG, and the README file for authors - http://www.swig.org/release.html.
 *
 * typemaps.i.
 *
 * SWIG Typemap library for PHP4.
 *
 * This library provides standard typemaps for modifying SWIG's behavior.
 * With enough entries in this file, I hope that very few people actually
 * ever need to write a typemap.
 *
 * Define macros to define the following typemaps:
 *
 * TYPE *INPUT.   Argument is passed in as native variable by value.
 * TYPE *OUTPUT.  Argument is returned as an array from the function call.
 * TYPE *INOUT.   Argument is passed in by value, and out as part of returned list
 * TYPE *REFERENCE.  Argument is passed in as native variable with value
 *                   semantics.  Variable value is changed with result.
 *                   Use like this:
 *                   int foo(int *REFERENCE);
 *
 *                   $a = 0;
 *                   $rc = foo($a);
 *
 *                   Even though $a looks like it's passed by value,
 *                   its value can be changed by foo().
 * ----------------------------------------------------------------------------- */

%include <typemaps/typemaps.swg>

%define double_typemap(TYPE)
%typemap(in) TYPE *REFERENCE (TYPE dvalue)
{
  convert_to_double_ex($input);
  dvalue = (TYPE) (*$input)->value.dval;
  $1 = &dvalue;
}
%typemap(argout) TYPE *REFERENCE
{
  $1->value.dval = (double)(lvalue$argnum);
  $1->type = IS_DOUBLE;
}
%enddef

%define int_typemap(TYPE)
%typemap(in) TYPE *REFERENCE (TYPE lvalue)
{
  convert_to_long_ex($input);
  lvalue = (TYPE) (*$input)->value.lval;
  $1 = &lvalue;
}
%typemap(argout) TYPE	*REFERENCE
{

  (*$arg)->value.lval = (long)(lvalue$argnum);
  (*$arg)->type = IS_LONG;
}
%enddef

double_typemap(float);
double_typemap(double);

int_typemap(int);
int_typemap(short);
int_typemap(long);
int_typemap(unsigned int);
int_typemap(unsigned short);
int_typemap(unsigned long);
int_typemap(unsigned char);

