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

char cvsroot_misc_c[] = "$Header$";

#include "swig.h"
#include "swigver.h"
#include <ctype.h>

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
Swig_banner(File *f) {
  Printf(f,
"/* ----------------------------------------------------------------------------\n\
 * This file was automatically generated by SWIG (http://www.swig.org).\n\
 * Version %s\n\
 * \n\
 * This file is not intended to be easily readable and contains a number of \n\
 * coding conventions designed to improve portability and efficiency. Do not make\n\
 * changes to this file unless you know what you are doing--modify the SWIG \n\
 * interface file instead. \n\
 * ----------------------------------------------------------------------------- */\n\n", SWIG_VERSION);

}

/* -----------------------------------------------------------------------------
 * Swig_string_escape()
 *
 * Takes a string object and produces a string with escape codes added to it.
 * ----------------------------------------------------------------------------- */

String *Swig_string_escape(String *s) {
  String *ns;
  int c;
  ns = NewString("");
  
  while ((c = Getc(s)) != EOF) {
    if (c == '\n') {
      Printf(ns,"\\n");
    } else if (c == '\r') {
      Printf(ns,"\\r");
    } else if (c == '\t') {
      Printf(ns,"\\t");
    } else if (c == '\\') {
      Printf(ns,"\\\\");
    } else if (c == '\'') {
      Printf(ns,"\\'");
    } else if (c == '\"') {
      Printf(ns,"\\\"");
    } else if (c == ' ') {
      Putc(c,ns);
    } else if (!isgraph(c)) {
      Printf(ns,"\\0%o", c);
    } else {
      Putc(c,ns);
    }
  }
  return ns;
}


/* -----------------------------------------------------------------------------
 * Swig_string_upper()
 *
 * Takes a string object and convets it to all caps.
 * ----------------------------------------------------------------------------- */

String *Swig_string_upper(String *s) {
  String *ns;
  int c;
  ns = NewString("");
  
  while ((c = Getc(s)) != EOF) {
    Putc(toupper(c),ns);
  }
  return ns;
}

/* -----------------------------------------------------------------------------
 * Swig_string_lower()
 *
 * Takes a string object and convets it to all lower.
 * ----------------------------------------------------------------------------- */

String *Swig_string_lower(String *s) {
  String *ns;
  int c;
  ns = NewString("");
  
  while ((c = Getc(s)) != EOF) {
    Putc(tolower(c),ns);
  }
  return ns;
}


/* -----------------------------------------------------------------------------
 * Swig_string_title()
 *
 * Takes a string object and convets it to all lower.
 * ----------------------------------------------------------------------------- */

String *Swig_string_title(String *s) {
  String *ns;
  int first = 1;
  int c;
  ns = NewString("");
  
  while ((c = Getc(s)) != EOF) {
    Putc(first ? toupper(c) : tolower(c),ns);
    first = 0;
  }
  return ns;
}

/* -----------------------------------------------------------------------------
 * Swig_string_typecode()
 *
 * Takes a string with possible type-escapes in it and replaces them with
 * real C datatypes.
 * ----------------------------------------------------------------------------- */

String *Swig_string_typecode(String *s) {
  String *ns;
  int c;
  String *tc;
  ns = NewString("");
  while ((c = Getc(s)) != EOF) {
    if (c == '`') {
      tc = NewString("");
      while ((c = Getc(s)) != EOF) {
	if (c == '`') break;
	Putc(c,tc);
      }
      Printf(ns,"%s",SwigType_str(tc,0));
    } else {
      Putc(c,ns);
      if (c == '\'') {
	while ((c = Getc(s)) != EOF) {
	  Putc(c,ns);
	  if (c == '\'') break;
	  if (c == '\\') {
	    c = Getc(s);
	    Putc(c,ns);
	  }
	}
      } else if (c == '\"') {
	while ((c = Getc(s)) != EOF) {
	  Putc(c,ns);
	  if (c == '\"') break;
	  if (c == '\\') {
	    c = Getc(s);
	    Putc(c,ns);
	  }
	}
      }
    }
  }
  return ns;
}
      
/* -----------------------------------------------------------------------------
 * Swig_string_mangle()
 * 
 * Take a string and mangle it by stripping all non-valid C identifier characters
 * ----------------------------------------------------------------------------- */

String *Swig_string_mangle(String *s) {
  String *t = Copy(s);
  char *c = Char(t);
  while (*c) {
    if (!isalnum(*c)) *c = '_';
    c++;
  }
  return t;
}

/* -----------------------------------------------------------------------------
 * Swig_scopename_prefix()
 *
 * Take a qualified name like "A::B::C" and return the scope name.
 * In this case, "A::B".   Returns NULL if there is no base.
 * ----------------------------------------------------------------------------- */

String *
Swig_scopename_prefix(String *s) {
  char tmp[1024];
  char   *c, *cc;
  if (!Strstr(s,"::")) return 0;
  strcpy(tmp,Char(s));
  c = tmp;
  cc = c;
  while (*c) {
    if (strncmp(c,"::",2) == 0) {
      cc = c;
      c += 2;
    } else {
      if (*c == '<') {
	int level = 1;
	c++;
	while (*c && level) {
	  if (*c == '<') level++;
	  if (*c == '>') level--;
	  c++;
	}
      } else {
	c++;
      }
    }
  }

  *cc = 0;
  if (cc != tmp) {
    return NewString(tmp);
  } else {
    return 0;
  }
}

/* -----------------------------------------------------------------------------
 * Swig_scopename_last()
 *
 * Take a qualified name like "A::B::C" and returns the last.  In this
 * case, "C". 
 * ----------------------------------------------------------------------------- */

String *
Swig_scopename_last(String *s) {
  char tmp[1024];
  char   *c, *cc;
  if (!Strstr(s,"::")) return NewString(s);
  strcpy(tmp,Char(s));
  c = tmp;
  cc = c;

  while (*c) {
    if (strncmp(c,"::",2) == 0) {
      cc = c;
      c += 2;
    } else {
      if (*c == '<') {
	int level = 1;
	c++;
	while (*c && level) {
	  if (*c == '<') level++;
	  if (*c == '>') level--;
	  c++;
	}
      } else {
	c++;
      }
    }
  }
  return NewString(cc+2);
}

/* -----------------------------------------------------------------------------
 * Swig_scopename_first()
 *
 * Take a qualified name like "A::B::C" and returns the first scope name.
 * In this case, "A".   Returns NULL if there is no base.
 * ----------------------------------------------------------------------------- */

String *
Swig_scopename_first(String *s) {
  char tmp[1024];
  char   *c;
  if (!Strstr(s,"::")) return 0;
  strcpy(tmp,Char(s));
  c = tmp;
  while (*c) {
    if (strncmp(c,"::",2) == 0) {
      break;
    } else {
      if (*c == '<') {
	int level = 1;
	c++;
	while (*c && level) {
	  if (*c == '<') level++;
	  if (*c == '>') level--;
	  c++;
	}
      } else {
	c++;
      }
    }
  }
  if (*c && (c != tmp)) {
    *c = 0;
    return NewString(tmp);
  } else {
    return 0;
  }
}


/* -----------------------------------------------------------------------------
 * Swig_scopename_suffix()
 *
 * Take a qualified name like "A::B::C" and returns the suffix.
 * In this case, "B::C".   Returns NULL if there is no suffix.
 * ----------------------------------------------------------------------------- */

String *
Swig_scopename_suffix(String *s) {
  char tmp[1024];
  char   *c;
  if (!Strstr(s,"::")) return 0;
  strcpy(tmp,Char(s));
  c = tmp;
  while (*c) {
    if (strncmp(c,"::",2) == 0) {
      break;
    } else {
      if (*c == '<') {
	int level = 1;
	c++;
	while (*c && level) {
	  if (*c == '<') level++;
	  if (*c == '>') level--;
	  c++;
	}
      } else {
	c++;
      }
    }
  }
  if (*c && (c != tmp)) {
    return NewString(c+2);
  } else {
    return 0;
  }
}
/* -----------------------------------------------------------------------------
 * Swig_scopename_check()
 *
 * Checks to see if a name is qualified with a scope name
 * ----------------------------------------------------------------------------- */

int Swig_scopename_check(String *s) {
  char *c = Char(s);
  if (!Strstr(s,"::")) return 0;
  while (*c) {
    if (strncmp(c,"::",2) == 0) {
      return 1;
    } else {
      if (*c == '<') {
	int level = 1;
	c++;
	while (*c && level) {
	  if (*c == '<') level++;
	  if (*c == '>') level--;
	  c++;
	}
      } else {
	c++;
      }
    }
  }
  return 0;
}


/* -----------------------------------------------------------------------------
 * Swig_init()
 *
 * Initialize the SWIG core
 * ----------------------------------------------------------------------------- */

void
Swig_init() {
  /* Set some useful string encoding methods */
  DohEncoding("escape", Swig_string_escape);
  DohEncoding("upper", Swig_string_upper);
  DohEncoding("lower", Swig_string_lower);
  DohEncoding("title", Swig_string_title);
  DohEncoding("typecode",Swig_string_typecode);

  /* Initialize typemaps */
  Swig_typemap_init();

  /* Initialize symbol table */
  Swig_symbol_init();

  /* Initialize type system */
  SwigType_typesystem_init();

}
