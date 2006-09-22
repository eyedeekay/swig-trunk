/* -----------------------------------------------------------------------------
 * See the LICENSE file for information on copyright, usage and redistribution
 * of SWIG, and the README file for authors - http://www.swig.org/release.html.
 *
 * php4.swg
 *
 * PHP4 configuration file
 * ----------------------------------------------------------------------------- */

/* Typemaps for input parameters by value */
%include <utils.i>

/* Common C API type-checking code */
%insert(runtime) "swigrun.swg"

/* Swig error codes */
%insert(runtime) "swigerrors.swg"

/* php4 runtime functions */
%insert(runtime) "php4run.swg"

/* php4 initialization routine */
%include <php4init.swg>

/* Global variable support */
%include <globalvar.i>

/* Constant Support */
%include <const.i>

/* Specialization of the std typemap library */
%include <phptypemaps.swg>

// use %init %{ "/*code goes here*/ " %}
// or  %minit %{ "/* code goes here*/ " %} to
// insert code in the PHP_MINIT_FUNCTION
#define %minit %insert("init")

// use %rinit %{ "/* code goes here*/ " %} to
// insert code in the PHP_RINIT_FUNCTION
#define %rinit %insert("rinit")

// use %shutdown %{ " /*code goes here*/ " %} to
// insert code in the PHP_MSHUTDOWN_FUNCTION
#define %shutdown  %insert("shutdown")
#define %mshutdown  %insert("shutdown")

// use %rshutdown %{ " /*code goes here*/" %} to
// insert code in the PHP_RSHUTDOWN_FUNCTION
#define %rshutdown  %insert("rshutdown")

/* Typemaps for input parameters by value */

%include <utils.i>

%pass_by_val(bool,CONVERT_BOOL_IN);

%pass_by_val(size_t, CONVERT_INT_IN);

%pass_by_val(enum SWIGTYPE, CONVERT_INT_IN);

%pass_by_val(signed int, CONVERT_INT_IN);
%pass_by_val(int,CONVERT_INT_IN);
%pass_by_val(unsigned int,CONVERT_INT_IN);

%pass_by_val(signed short, CONVERT_INT_IN);
%pass_by_val(short,CONVERT_INT_IN);
%pass_by_val(unsigned short, CONVERT_INT_IN);

%pass_by_val(signed long, CONVERT_INT_IN);
%pass_by_val(long, CONVERT_INT_IN);
%pass_by_val(unsigned long, CONVERT_INT_IN);

%pass_by_val(signed char, CONVERT_INT_IN);
%pass_by_val(char, CONVERT_CHAR_IN);
%pass_by_val(unsigned char, CONVERT_INT_IN);

%pass_by_val(float, CONVERT_FLOAT_IN);

%pass_by_val(double, CONVERT_FLOAT_IN);

%pass_by_val(char *, CONVERT_STRING_IN);


%typemap(in) void *
{
	if(SWIG_ConvertPtr(*$input, (void **) &$1, 0, 0) < 0) {
	  /* Allow NULL from php for void* */
	  if ((*$input)->type==IS_NULL) $1=0;
	  else
            SWIG_PHP_Error(E_ERROR, "Type error in argument $argnum of $symname. Expected $&1_descriptor");
	}
}

/* Special case when void* is passed by reference so it can be made to point
   to opaque api structs */
%typemap(in) void ** ($*1_ltype ptr, int force),
             void *& ($*1_ltype ptr, int force)
{
  /* If they pass NULL by reference, make it into a void*
     This bit should go in arginit if arginit support init-ing scripting args */
  if(SWIG_ConvertPtr(*$input, (void **) &$1, $1_descriptor, 0) < 0) {
    /* So... we didn't get a ref or ptr, but we'll accept NULL by reference */
    if (!((*$input)->type==IS_NULL && PZVAL_IS_REF(*$input))) {
      /* wasn't a pre/ref/thing, OR anything like an int thing */
      SWIG_PHP_Error(E_ERROR, "Type error in argument $arg of $symname.");
    }
  }
  force=0;
  if (arg1==NULL) {
#ifdef __cplusplus
    ptr=new $*1_ltype;
#else
    ptr=($*1_ltype) calloc(1,sizeof($*1_ltype));
#endif
    $1=&ptr;
    /* have to passback arg$arg too */
    force=1;
  }
}
%typemap(argout) void **,
                 void *&
{
  if (force$argnum) {
    SWIG_SetPointerZval( *$input, (void*) ptr$argnum, $*1_descriptor, 1);
  }
}


%typemap(out) SWIGTYPE
#ifdef __cplusplus
{
  $&1_ltype resultobj = new $1_ltype(($1_ltype &) $1);
  SWIG_SetPointerZval(return_value, (void *)resultobj, $&1_descriptor, 1);
}
#else
{
  $&1_ltype resultobj = ($&1_ltype) emalloc(sizeof($1_type));
  memmove(resultobj, &$1, sizeof($1_type));
  SWIG_SetPointerZval(return_value, (void *)resultobj, $&1_descriptor, 1);
}
#endif

%typemap(out) void "";

%typemap(out) char [ANY]
{
  RETVAL_STRINGL($1,$1_dim0,1);
}

// This typecheck does hard checking for proper argument type.  If you want
// an argument to be converted from a different PHP type, you must convert
// it yourself before passing it (e.g. (string)4.7 or (int)"6").
%define %php_typecheck(_type,_prec,is)
%typemap(typecheck,precedence=_prec) _type
 " $1 = (Z_TYPE_PP($input) == is); "
%enddef

%php_typecheck(int,SWIG_TYPECHECK_INTEGER,IS_LONG)
%php_typecheck(unsigned int,SWIG_TYPECHECK_UINT32,IS_LONG)
%php_typecheck(short,SWIG_TYPECHECK_INT16,IS_LONG)
%php_typecheck(unsigned short,SWIG_TYPECHECK_UINT16,IS_LONG)
%php_typecheck(long,SWIG_TYPECHECK_INT64,IS_LONG)
%php_typecheck(unsigned long,SWIG_TYPECHECK_UINT64,IS_LONG)
%php_typecheck(signed char,SWIG_TYPECHECK_INT8,IS_LONG)
%php_typecheck(unsigned char,SWIG_TYPECHECK_UINT8,IS_LONG)
%php_typecheck(size_t,SWIG_TYPECHECK_INT16,IS_LONG)
%php_typecheck(enum SWIGTYPE,SWIG_TYPECHECK_INT8,IS_LONG)
%php_typecheck(bool,SWIG_TYPECHECK_BOOL,IS_BOOL)

%php_typecheck(char,SWIG_TYPECHECK_CHAR,IS_STRING)
%php_typecheck(char *,SWIG_TYPECHECK_STRING,IS_STRING)
%php_typecheck(char [],SWIG_TYPECHECK_STRING,IS_STRING)

%php_typecheck(float,SWIG_TYPECHECK_FLOAT,IS_DOUBLE)
%php_typecheck(double,SWIG_TYPECHECK_BOOL,IS_DOUBLE)

%typecheck(SWIG_TYPECHECK_POINTER) SWIGTYPE
 " /* typecheck SWIGTYPE */ "

%typecheck(SWIG_TYPECHECK_POINTER) SWIGTYPE *,
             SWIGTYPE [],
             SWIGTYPE &
{
  void *tmp;
  _v = (SWIG_ConvertPtr( *$input, (void**)&tmp, $1_descriptor, 0) >= 0);
}

%typecheck(SWIG_TYPECHECK_VOIDPTR) void *
 " /* typecheck void * */ "


/* Exception handling */

%typemap(throws) int,
                 long,
                 short,
                 unsigned int,
                 unsigned long,
                 unsigned short {
  char error_msg[256];
  sprintf(error_msg, "C++ $1_type exception thrown, value: %d", $1);
  SWIG_PHP_Error(E_ERROR, error_msg);
}

%typemap(throws) SWIGTYPE, SWIGTYPE &, SWIGTYPE *, SWIGTYPE [], SWIGTYPE [ANY] %{
  (void)$1;
  SWIG_PHP_Error(E_ERROR, "C++ $1_type exception thrown");
%}

%typemap(throws) char * %{
  SWIG_PHP_Error(E_ERROR, (char *)$1);
%}


/* php keywords */
%include <php4kw.swg>
