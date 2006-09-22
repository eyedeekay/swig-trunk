
/* ------------------------------------------------------------
 *  Fragment section
 * ------------------------------------------------------------ */
%include <typemaps/fragments.swg>

%include <typemaps/swigmacros.swg>

%include <phpstrings.swg>

%include <phpprimtypes.swg>

/* ------------------------------------------------------------
 *  Unified typemap section
 * ------------------------------------------------------------ */
/* directors are not supported in PHP */
#ifdef SWIG_DIRECTOR_TYPEMAPS
#undef SWIG_DIRECTOR_TYPEMAPS
#endif

/* Php types */
#define SWIG_Object zval**

%define %set_output(obj)
  REPLACE_ZVAL_VALUE(&$result,%static_cast(obj,zval*),0);
%enddef

/* append output */
%define %append_output(obj)
   SWIG_PHP_AppendOutput(&$result,%static_cast(obj,zval*));
%enddef

/* set constant
 * PHP cannot use the unified typemap library support for constants
 * because PHP has a different method for each type of registration.
 * Note:  PHP currently uses const.i which defines
 * a bunch of consttab typemaps for constants.
 */
%define %set_constant(name,obj)
%enddef

/* raise */
#define SWIG_Raise(obj,type,desc) SWIG_PHP_Error(SWIG_TypeError, "Error converting " ## type );

/* include the unified typemap library */
%include <typemaps/swigtypemaps.swg>

