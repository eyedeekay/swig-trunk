/* ----------------------------------------------------------------------------- 
 * misc.c
 *
 *     Miscellaneous functions that don't really fit anywhere else.
 * 
 * Author(s) : David Beazley (beazley@cs.uchicago.edu)
 *
 * Copyright (C) 1999-2000.  The University of Chicago
 * See the file LICENSE for information on usage and redistribution.	
 * ----------------------------------------------------------------------------- */

static char cvsroot[] = "$Header$";

#include "swig.h"
#include "swigver.h"

/* -----------------------------------------------------------------------------
 * Swig_copy_string()
 *
 * Duplicate a NULL-terminate string given as a char *.
 * ----------------------------------------------------------------------------- */

char *
Swig_copy_string(const char *s) {
  char *c = 0;
  if (s) {
    c = (char *) malloc(strlen(s)+1);
    strcpy(c,s);
  }
  return c;
}

/* -----------------------------------------------------------------------------
 * Swig_banner()
 *
 * Emits the SWIG identifying banner.
 * ----------------------------------------------------------------------------- */

void
Swig_banner(DOHFile *f) {
  Printf(f,
"/* ----------------------------------------------------------------------------\n\
 * This file was automatically generated by SWIG (http://www.swig.org).\n\
 * Version %s %s\n\
 * \n\
 * Portions Copyright (c) 1995-2000\n\
 * The University of Utah, The Regents of the University of California, and\n\
 * The University of Chicago.  Permission is hereby granted to use, modify, \n\
 * and distribute this file in any manner provided this notice remains intact.\n\
 * \n\
 * This file is not intended to be easily readable and contains a number of \n\
 * coding conventions designed to improve portability and efficiency. Do not make\n\
 * changes to this file unless you know what you are doing--modify the SWIG \n\
 * interface file instead. \n\
 * ----------------------------------------------------------------------------- */\n\n", SWIG_VERSION, SWIG_SPIN);
}

  
  
     
