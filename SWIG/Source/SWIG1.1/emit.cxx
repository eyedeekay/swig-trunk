/* -----------------------------------------------------------------------------
 * emit.cxx
 *
 *     Useful functions for emitting various pieces of code.
 *
 * Author(s) : David Beazley (beazley@cs.uchicago.edu)
 *
 * Copyright (C) 1998-2000.  The University of Chicago
 * Copyright (C) 1995-1998.  The University of Utah and The Regents of the
 *                           University of California.
 *
 * See the file LICENSE for information on usage and redistribution.
 * ----------------------------------------------------------------------------- */

#include "internal.h"
extern "C" {
#include "swig.h"
}

static char cvsroot[] = "$Header$";

/* -----------------------------------------------------------------------------
 * int emit_args(char *d, DataType *rt, ParmList *l, Wrapper *f)
 *
 * Creates a list of variable declarations for both the return value
 * and function parameters.
 *
 * The return value is always called result and arguments arg0, arg1, arg2, etc...
 * Returns the number of parameters associated with a function.
 * ----------------------------------------------------------------------------- */

int emit_args(DataType *rt, ParmList *l, Wrapper *f) {

  Parm *p;
  int   i;
  char *tm;
  DataType *pt;
  char  *pvalue;
  char  *pname;
  char  *lname;

  /* Emit function arguments */
  Swig_cargs(f, l);

  i = 0;
  p = ParmList_first(l);
  while (p != 0) {
    lname  = Parm_Getlname(p);
    pt     = Parm_Gettype(p);
    pname  = Parm_Getname(p);
    pvalue = Parm_Getvalue(p);

    tm = typemap_lookup((char*)"arginit", typemap_lang, pt,pname,(char*)"",lname,f);
    if (tm) {
      Printv(f->code,tm,"\n",0);
    }
    // Check for ignore or default typemaps
    tm = typemap_lookup((char*)"default",typemap_lang,pt,pname,(char*)"",lname,f);
    if (tm) {
      Printv(f->code,tm,"\n",0);
    }
    tm = typemap_lookup((char*)"ignore",typemap_lang,pt,pname,(char*)"",lname,f);
    if (tm) {
      Printv(f->code,tm,"\n",0);
      Parm_Setignore(p,1);
    }
    i++;
    p = ParmList_next(l);
  }
  return(i);
}

/* -----------------------------------------------------------------------------
 * int emit_func_call(char *decl, DataType *t, ParmList *l, Wrapper*f)
 *
 * Emits code for a function call (new version).
 *
 * Exception handling support :
 *
 *     -  This function checks to see if any sort of exception mechanism
 *        has been defined.  If so, we emit the function call in an exception
 *        handling block.
 * ----------------------------------------------------------------------------- */

static DOH *fcall = 0;

void emit_set_action(DOHString_or_char *decl) {
  if (fcall) Delete (fcall);
  fcall = NewString(decl);
}

void emit_func_call(char *decl, DataType *t, ParmList *l, Wrapper *f) {
  char *tm;

  if ((tm = typemap_lookup((char*)"except",typemap_lang,t,decl,(char*)"result",(char*)""))) {
    Printv(f->code,tm,0);
    Replace(f->code,"$name",decl,DOH_REPLACE_ANY);
  } else if ((tm = fragment_lookup((char*)"except",typemap_lang, t->id))) {
    Printv(f->code,tm,0);
    Replace(f->code,"$name",decl,DOH_REPLACE_ANY);
  } else {
    Printv(f->code,"$function",0);
  }
  
  if (!fcall) fcall = NewString(Swig_cfunction_call(decl,l));

  if (CPlusPlus) {
    Swig_cppresult(f, t, "result", Char(fcall));
  } else {
    Swig_cresult(f, t, "result", Char(fcall));
  }
  Delete(fcall);
  fcall = 0;
}

/* -----------------------------------------------------------------------------
 * void emit_set_get(char *name, char *iname, DataType *type)
 *
 * Emits a pair of functions to set/get the value of a variable.  This is
 * only used in the event the target language can't provide variable linking
 * on its own.
 *
 * double foo;
 *
 * Gets translated into the following :
 *
 * double foo_set(double x) {
 *      return foo = x;
 * }
 *
 * double foo_get() {
 *      return foo;
 * }
 * ----------------------------------------------------------------------------- */

/* How to assign a C allocated string */

static char *c_str = (char *)" {\n\
if ($target) free($target);\n\
$target = ($rtype) malloc(strlen($source)+1);\n\
strcpy((char *)$target,$source);\n\
return $ltype $target;\n\
}\n";

/* How to assign a C allocated string */

static char *cpp_str = (char *)" {\n\
if ($target) delete [] $target;\n\
$target = ($rtype) (new char[strlen($source)+1]);\n\
strcpy((char *)$target,$source);\n\
return ($ltype) $target;\n\
}\n";

void emit_set_get(char *name, char *iname, DataType *t) {

  Wrapper *w;
  char new_iname[256];
  char    *code = 0;
    
  /* First write a function to set the variable of the variable */
  if (!(Status & STAT_READONLY)) {

    if ((t->type == T_CHAR) && (t->is_pointer == 1)) {
      if (CPlusPlus)
	code = cpp_str;
      else
	code = c_str;
      
    }
    w = Swig_cvarset_wrapper(name, t, code);
    Wrapper_print(w,f_header);
    strcpy(new_iname, Swig_name_set(iname));
    lang->create_function(Wrapper_Getname(w), new_iname, Wrapper_Gettype(w), Wrapper_Getparms(w));
    DelWrapper(w);
  }

  w = Swig_cvarget_wrapper(name,t,0);
  Wrapper_print(w,f_header);
  strcpy(new_iname, Swig_name_get(iname));
  lang->create_function(Wrapper_Getname(w), new_iname, Wrapper_Gettype(w), Wrapper_Getparms(w));
  DelWrapper(w);
}






