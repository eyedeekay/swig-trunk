/* SWIG Configuration File for Guile. -*-c-*-
   This file is parsed by SWIG before reading any other interface
   file. */

/* Include headers */
%insert(runtime) "guiledec.swg"

#ifndef SWIG_NOINCLUDE
%insert(runtime) "guile.swg"
#endif

#define %scheme(text) %pragma(guile) scheme = text;

/* Read in standard typemaps. */
%include "typemaps.i"

