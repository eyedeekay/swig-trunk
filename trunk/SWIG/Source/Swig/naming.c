/* ----------------------------------------------------------------------------- 
 * naming.c
 *
 *     Functions for generating various kinds of names during code generation
 * 
 * Author(s) : David Beazley (beazley@cs.uchicago.edu)
 *
 * Copyright (C) 1999-2000.  The University of Chicago
 * See the file LICENSE for information on usage and redistribution.	
 * ----------------------------------------------------------------------------- */

static char cvsroot[] = "$Header$";

#include "swig.h"
#include <ctype.h>

/* Hash table containing naming data */

static DOH *naming_hash = 0;

/* -----------------------------------------------------------------------------
 * Swig_name_register()
 *
 * Register a new naming format.
 * ----------------------------------------------------------------------------- */

void Swig_name_register(DOHString_or_char *method, DOHString_or_char *format) {
  if (!naming_hash) naming_hash = NewHash();
  Setattr(naming_hash,method,format);
}

/* -----------------------------------------------------------------------------
 * Swig_name_mangle()
 *
 * Converts all of the non-identifier characters of a string to underscores.
 * ----------------------------------------------------------------------------- */

char *Swig_name_mangle(DOHString_or_char *s) {
  static DOHString *r = 0;
  char  *c;
  if (!r) r = NewString("");
  Clear(r);
  Append(r,s);
  c = Char(r);
  while (*c) {
    if (!isalnum(*c)) *c = '_';
    c++;
  }
  return Char(r);
}

/* -----------------------------------------------------------------------------
 * Swig_name_wrapper()
 *
 * Returns the name of a wrapper function.
 * ----------------------------------------------------------------------------- */

char *Swig_name_wrapper(DOHString_or_char *fname) {
  static DOHString *r = 0;
  DOHString *f;

  if (!r) r = NewString("");
  if (!naming_hash) naming_hash = NewHash();
  Clear(r);
  f = Getattr(naming_hash,"wrapper");
  if (!f) {
    Append(r,"_wrap_%f");
  } else {
    Append(r,f);
  }
  Replace(r,"%f",fname, DOH_REPLACE_ANY);
  return Char(r);
}


/* -----------------------------------------------------------------------------
 * Swig_name_member()
 *
 * Returns the name of a class method.
 * ----------------------------------------------------------------------------- */

char *Swig_name_member(DOHString_or_char *classname, DOHString_or_char *mname) {
  static DOHString *r = 0;
  DOHString *f;
  char   *cname, *c;

  if (!r) r = NewString("");
  if (!naming_hash) naming_hash = NewHash();
  Clear(r);
  f = Getattr(naming_hash,"member");
  if (!f) {
    Append(r,"%c_%m");
  } else {
    Append(r,f);
  }
  cname = Char(classname);
  c = strchr(cname, ' ');
  if (c) cname = c+1;
  Replace(r,"%c",cname, DOH_REPLACE_ANY);
  Replace(r,"%m",mname, DOH_REPLACE_ANY);
  return Char(r);
}

/* -----------------------------------------------------------------------------
 * Swig_name_get()
 *
 * Returns the name of the accessor function used to get a variable.
 * ----------------------------------------------------------------------------- */

char *Swig_name_get(DOHString_or_char *vname) {
  static DOHString *r = 0;
  DOHString *f;

  if (!r) r = NewString("");
  if (!naming_hash) naming_hash = NewHash();
  Clear(r);
  f = Getattr(naming_hash,"get");
  if (!f) {
    Append(r,"%v_get");
  } else {
    Append(r,f);
  }
  Replace(r,"%v",vname, DOH_REPLACE_ANY);
  return Char(r);
}

/* ----------------------------------------------------------------------------- 
 * Swig_name_set()
 *
 * Returns the name of the accessor function used to set a variable.
 * ----------------------------------------------------------------------------- */

char *Swig_name_set(DOHString_or_char *vname) {
  static DOHString *r = 0;
  DOHString *f;

  if (!r) r = NewString("");
  if (!naming_hash) naming_hash = NewHash();
  Clear(r);
  f = Getattr(naming_hash,"set");
  if (!f) {
    Append(r,"%v_set");
  } else {
    Append(r,f);
  }
  Replace(r,"%v",vname, DOH_REPLACE_ANY);
  return Char(r);
}

/* -----------------------------------------------------------------------------
 * Swig_name_construct()
 *
 * Returns the name of the accessor function used to create an object.
 * ----------------------------------------------------------------------------- */

char *Swig_name_construct(DOHString_or_char *classname) {
  static DOHString *r = 0;
  DOHString *f;
  char *cname, *c;
  if (!r) r = NewString("");
  if (!naming_hash) naming_hash = NewHash();
  Clear(r);
  f = Getattr(naming_hash,"construct");
  if (!f) {
    Append(r,"new_%c");
  } else {
    Append(r,f);
  }

  cname = Char(classname);
  c = strchr(cname, ' ');
  if (c) cname = c+1;

  Replace(r,"%c",cname, DOH_REPLACE_ANY);
  return Char(r);
}
  

/* -----------------------------------------------------------------------------
 * Swig_name_destroy()
 *
 * Returns the name of the accessor function used to destroy an object.
 * ----------------------------------------------------------------------------- */

char *Swig_name_destroy(DOHString_or_char *classname) {
  static DOHString *r = 0;
  DOHString *f;
  char *cname, *c;
  if (!r) r = NewString("");
  if (!naming_hash) naming_hash = NewHash();
  Clear(r);
  f = Getattr(naming_hash,"destroy");
  if (!f) {
    Append(r,"delete_%c");
  } else {
    Append(r,f);
  }

  cname = Char(classname);
  c = strchr(cname, ' ');
  if (c) cname = c+1;

  Replace(r,"%c",cname, DOH_REPLACE_ANY);
  return Char(r);
}



