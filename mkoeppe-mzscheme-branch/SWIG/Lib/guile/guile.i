/* SWIG Configuration File for Guile. -*-c-*-
   This file is parsed by SWIG before reading any other interface
   file. */

/* Include headers */
%insert(runtime) "guiledec.swg"

#ifndef SWIG_NOINCLUDE
%insert(runtime) "guile.swg"
#endif

#define %scheme	    %insert("scheme")

#define %values_as_list   %pragma(guile) beforereturn ""
#define %values_as_vector %pragma(guile) beforereturn "GUILE_MAYBE_VECTOR"
#define %multiple_values  %pragma(guile) beforereturn "GUILE_MAYBE_VALUES"



/* Definitions */
#define SWIG_malloc(size) SCM_MUST_MALLOC(size)
#define SWIG_free(mem) scm_must_free(mem)

/* Read in standard typemaps. */
%include "typemaps.i"
