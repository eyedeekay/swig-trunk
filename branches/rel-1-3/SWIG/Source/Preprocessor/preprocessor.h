/* -----------------------------------------------------------------------------
 * preprocessor.h
 *
 *     SWIG preprocessor module.
 *
 * Author(s) : David Beazley (beazley@cs.uchicago.edu)
 *
 * Copyright (C) 1999-2000.  The University of Chicago
 * See the file LICENSE for information on usage and redistribution.
 *
 * $Header$
 * ----------------------------------------------------------------------------- */

#ifndef _PREPROCESSOR_H
#define _PREPROCESSOR_H

#include "swig.h"

#ifdef __cplusplus
extern "C" {
#endif
extern int     Preprocessor_expr(String *s, int *error);
extern char   *Preprocessor_expr_error(void);
extern Hash   *Preprocessor_define(String_or_char *str, int swigmacro);
extern void    Preprocessor_undef(String_or_char *name);
extern void    Preprocessor_init();
extern String *Preprocessor_parse(File *s);
extern void    Preprocessor_include_all(int);
extern void    Preprocessor_import_all(int);
extern int     Preprocessor_errors(void);

#ifdef __cplusplus
}
#endif

#endif




