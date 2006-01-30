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
#include "swigkeys.h"
#include <errno.h>
#include <ctype.h>
#include <limits.h>


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
 * interface file instead. \n", PACKAGE_VERSION);
  /* String too long for ISO compliance */
  Printf(f,
" * ----------------------------------------------------------------------------- */\n\n");

}

/* -----------------------------------------------------------------------------
 * Swig_string_escape()
 *
 * Takes a string object and produces a string with escape codes added to it.
 * ----------------------------------------------------------------------------- */

String *Swig_string_escape(String *s) {
  String *ns;
  int c;
  ns = NewStringEmpty();
  
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
      if (c < 0) c += UCHAR_MAX +1;
      Printf(ns,"\\%o", c);
    } else {
      Putc(c,ns);
    }
  }
  return ns;
}


/* -----------------------------------------------------------------------------
 * Swig_string_upper()
 *
 * Takes a string object and returns a copy that is uppercase
 * ----------------------------------------------------------------------------- */

String *Swig_string_upper(String *s) {
  String *ns;
  int c;
  ns = NewStringEmpty();

  Seek(s,0,SEEK_SET);
  while ((c = Getc(s)) != EOF) {
    Putc(toupper(c),ns);
  }
  return ns;
}

/* -----------------------------------------------------------------------------
 * Swig_string_lower()
 *
 * Takes a string object and returns a copy that is lowercase
 * ----------------------------------------------------------------------------- */

String *Swig_string_lower(String *s) {
  String *ns;
  int c;
  ns = NewStringEmpty();

  Seek(s,0,SEEK_SET);
  while ((c = Getc(s)) != EOF) {
    Putc(tolower(c),ns);
  }
  return ns;
}


/* -----------------------------------------------------------------------------
 * Swig_string_title()
 *
 * Takes a string object and returns a copy that is lowercase with first letter
 * capitalized
 * ----------------------------------------------------------------------------- */

String *Swig_string_title(String *s) {
  String *ns;
  int first = 1;
  int c;
  ns = NewStringEmpty();

  Seek(s,0,SEEK_SET);
  while ((c = Getc(s)) != EOF) {
    Putc(first ? toupper(c) : tolower(c),ns);
    first = 0;
  }
  return ns;
}

/* -----------------------------------------------------------------------------
 * Swig_string_ccase()
 *
 * Takes a string object and returns a copy that is lowercase with thefirst letter
 * capitalized and the one following '_', which are removed.
 *
 *      camel_case -> CamelCase
 *      camelCase  -> CamelCase
 * ----------------------------------------------------------------------------- */

String *Swig_string_ccase(String *s) {
  String *ns;
  int first = 1;
  int c;
  ns = NewStringEmpty();

  Seek(s,0,SEEK_SET);
  while ((c = Getc(s)) != EOF) {
    if (c == '_') { 
      first = 1;
      continue;
    }
    Putc(first ? toupper(c) : c, ns);
    first = 0;
  }
  return ns;
}

/* -----------------------------------------------------------------------------
 * Swig_string_ucase()
 *
 * This is the reverse case of ccase, ie
 *
 *      CamelCase -> camel_case 
 * ----------------------------------------------------------------------------- */

String *Swig_string_ucase(String *s) {
  String *ns;
  int first = 0;
  int c;
  ns = NewStringEmpty();

  Seek(s,0,SEEK_SET);
  while ((c = Getc(s)) != EOF) {
    if (isalpha(c)) {
      if (isupper(c)) {
	if (first) Putc('_',ns);
	first = 0;
      } else {
	first = 1;
      }
    }
    else if (c == '_') {
      /* We don't want two underscores in a row */
      first = 0;
    }
    Putc(tolower(c),ns);
  }
  return ns;
}

/* -----------------------------------------------------------------------------
 * Swig_string_first_upper()
 *
 * Make the first character in the string uppercase, leave all the 
 * rest the same.  This is used by the Ruby module to provide backwards
 * compatibility with the old way of naming classes and constants.  For
 * more info see the Ruby documentation.
 *
 *      firstUpper -> FirstUpper 
 * ----------------------------------------------------------------------------- */

String *Swig_string_first_upper(String *s) {
  String *ns = NewStringEmpty();
  char *cs = Char(s);
  if (cs && cs[0] != 0) {
    Putc(toupper(cs[0]),ns);
    Append(ns, cs + 1);
  }
  return ns;
}

/* -----------------------------------------------------------------------------
 * Swig_string_first_lower()
 *
 * Make the first character in the string lowercase, leave all the 
 * rest the same.  This is used by the Ruby module to provide backwards
 * compatibility with the old way of naming classes and constants.  For
 * more info see the Ruby documentation.
 *
 *      firstLower -> FirstLower 
 * ----------------------------------------------------------------------------- */

String *Swig_string_first_lower(String *s) {
  String *ns = NewStringEmpty();
  char *cs = Char(s);
  if (cs && cs[0] != 0) {
    Putc(tolower(cs[0]),ns);
    Append(ns, cs + 1);
  }
  return ns;
}

/* -----------------------------------------------------------------------------
 * Swig_string_schemify()
 *
 * Replace underscores with dashes, to make identifiers look nice to Schemers.
 *
 *      under_scores -> under-scores
 * ----------------------------------------------------------------------------- */

String *Swig_string_schemify(String *s) {
  String *ns = NewString(s);
  Replaceall(ns, "_", "-");
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
  ns = NewStringEmpty();
  while ((c = Getc(s)) != EOF) {
    if (c == '`') {
      String *str = 0;
      tc = NewStringEmpty();
      while ((c = Getc(s)) != EOF) {
	if (c == '`') break;
	Putc(c,tc);
      }
      str = SwigType_str(tc,0);
      Append(ns,str);
      Delete(str);
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
 * Take a string and mangle it by stripping all non-valid C identifier
 * characters.
 *
 * This routine skips unnecessary blank spaces, therefore mangling
 * 'char *' and 'char*', 'std::pair<int, int >' and
 * 'std::pair<int,int>', produce the same result.
 *
 * However, note that 'long long' and 'long_long' produce different
 * mangled strings.
 *
 * The mangling method still is not 'perfect', for example std::pair and
 * std_pair return the same mangling. This is just a little better
 * than before, but it seems to be enough for most of the purposes.
 *
 * Having a perfect mangling will break some examples and code which
 * assume, for example, that A::get_value will be mangled as
 * A_get_value. 
 * ----------------------------------------------------------------------------- */

String *Swig_string_mangle(const String *s) {
#if 0 
  /* old mangling, not suitable for using in macros */
  String *t = Copy(s);
  char *c = Char(t);
  while (*c) {
    if (!isalnum(*c)) *c = '_';
    c++;
  }
  return t;
#else
  String *result = NewStringEmpty();
  int space = 0;
  int state = 0;
  char *pc, *cb;
  String *b = Copy(s);
  if (SwigType_istemplate(b)) {
    String *st = Swig_symbol_template_deftype(b, 0);
    String *sq = Swig_symbol_type_qualify(st,0);
    String *t = SwigType_namestr(sq);
    Delete(st);
    Delete(sq);
    Delete(b);
    b = t ;
  }
  pc = cb = StringChar(b);
  while (*pc) {
    char c = *pc;
    if (isalnum((int)c) || (c == '_')) {
      state = 1;
      if (space && (space == state)) {
	StringAppend(result,"_SS_");
      }
      space = 0;
      Printf(result,"%c",(int)c);
      
    } else {
      if (isspace((int)c)) {
	space = state;
	++pc;
	continue;
      } else {
	state = 3;
	space = 0;
      }
      switch(c) {
      case '.':
	if ((cb != pc) && (*(pc - 1) == 'p')) {
	  StringAppend(result,"_");
	  ++pc;
	  continue;
	} else {
	  c = 'f';
	}
	break;
      case ':':
	if (*(pc + 1) == ':') {
	  StringAppend(result,"_");
	  ++pc; ++pc;
	  continue;
	}
	break;
      case '*':
	c = 'm';
	break;
      case '&':
	c = 'A';
	break;
      case '<':
	c = 'l';
	break;
      case '>':
	c = 'g';
	break;
      case '=':
	c = 'e';
	break;
      case ',':
	c = 'c';
	break;
      case '(':
	c = 'p';
	break;
      case ')':
	c = 'P';
	break;
      case '[':
	c = 'b';
	break;
      case ']':
	c = 'B';
	break;
      case '^':
	c = 'x';
	break;
      case '|':
	c = 'o';
	break;
      case '~':
	c = 'n';
	break;
      case '!':
	c = 'N';
	break;
      case '%':
	c = 'M';
	break;
      case '?':
	c = 'q';
	break;
      case '+':
	c = 'a';
	break;
      case '-':
	c = 's';
	break;
      case '/':
	c = 'd';
	break;
      default:
	break;
      }
      if (isalpha((int)c)) {
	Printf(result,"_S%c_",(int)c);
      } else{
	Printf(result,"_S%02X_",(int)c);
      }
    }
    ++pc;
  }
  Delete(b);
  return result;
#endif
}

String *Swig_string_emangle(String *s) {
  return Swig_string_mangle(s);
}


/* -----------------------------------------------------------------------------
 * Swig_scopename_prefix()
 *
 * Take a qualified name like "A::B::C" and return the scope name.
 * In this case, "A::B".   Returns NULL if there is no base.
 * ----------------------------------------------------------------------------- */

void
Swig_scopename_split(String *s, String **rprefix, String **rlast) {
  char *tmp = Char(s);
  char *c = tmp;
  char *cc = c;
  char *co = 0;
  if (!strstr(c,"::")) {
    *rprefix = 0;
    *rlast = Copy(s);
  }
  
  if ((co = strstr(cc,"operator "))) {
    if (co == cc) {
      *rprefix = 0;
      *rlast = Copy(s);
      return;
    } else {
      *rprefix = NewStringWithSize(cc, co - cc - 2);
      *rlast = NewString(co);
      return;
    }
  }
  while (*c) {
    if ((*c == ':') && (*(c+1) == ':')) {
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

  if (cc != tmp) {
    *rprefix = NewStringWithSize(tmp, cc - tmp);
    *rlast = NewString(cc + 2);
    return;
  } else {
    *rprefix = 0;
    *rlast = Copy(s);
  }
}


String *
Swig_scopename_prefix(String *s) {
  char *tmp = Char(s);
  char *c = tmp;
  char *cc = c;
  char *co = 0;
  if (!strstr(c,"::")) return 0;
  if ((co = strstr(cc,"operator "))) {
    if (co == cc) {
      return 0;
    } else {
      String *prefix = NewStringWithSize(cc, co - cc - 2);
      return prefix;
    }
  }
  while (*c) {
    if ((*c == ':') && (*(c+1) == ':')) {
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

  if (cc != tmp) {
    return NewStringWithSize(tmp, cc - tmp);
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
  char *tmp = Char(s);
  char *c = tmp;
  char *cc = c;
  char *co = 0;
  if (!strstr(c,"::")) return NewString(s);

  if ((co = strstr(cc,"operator "))) {
    return NewString(co);
  }


  while (*c) {
    if ((*c == ':') && (*(c+1) == ':')) {
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
  char *tmp = Char(s);
  char   *c = tmp;
  char *co = 0;
  if (!strstr(c,"::")) return 0;
  if ((co = strstr(c,"operator "))) {
    if (co == c) {
      return 0;
    }
  } else {
    co = c + Len(s);
  }
  
  while (*c && (c != co)) {
    if ((*c == ':') && (*(c+1) == ':')) {
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
    return NewStringWithSize(tmp, c - tmp);
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
  char *tmp = Char(s);
  char *c = tmp;
  char *co = 0;
  if (!strstr(c,"::")) return 0;
  if ((co = strstr(c,"operator "))) {
    if (co == c) return 0;
  }
  while (*c) {
    if ((*c == ':') && (*(c+1) == ':')) {
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
  char *co = 0;
  if ((co = strstr(c,"operator "))) {
    if (co == c) return 0;
  }
  if (!strstr(c,"::")) return 0;
  while (*c) {
    if ((*c == ':') && (*(c+1) == ':')) {
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
 * Swig_string_command()
 *
 * Executes a external command via popen with the string as a command
 * line parameter. For example:
 *
 *  Printf(stderr,"%(command:sed 's/[a-z]/\U\\1/' <<<)s","hello") -> Hello
 * ----------------------------------------------------------------------------- */
#if defined(HAVE_POPEN)
#  if defined(_MSC_VER)
#    define popen _popen
#    define pclose _pclose
#  else
extern FILE *popen(const char *command, const char *type);
extern int pclose(FILE *stream);
#  endif
#else
#  if defined(_MSC_VER)
#    define HAVE_POPEN 1
#    define popen _popen
#    define pclose _pclose
#  endif
#endif

String *Swig_string_command(String *s) {
  String *res = NewStringEmpty();
#if defined(HAVE_POPEN)
  if (Len(s)) {
    char *command = Char(s);
    FILE *fp = popen(command,"r");
    if (fp) {
      char buffer[1025];
      while(fscanf(fp,"%1024s",buffer) != EOF) {
	Append(res,buffer);      
      }
      pclose(fp);
    }
    if (!fp || errno) {
      Swig_error("SWIG",Getline(s), "Command encoder fails attempting '%s'.\n", s);
      exit(1);
    }
  }
#endif
  return res;
}


/* -----------------------------------------------------------------------------
 * Swig_string_rxspencer()
 *
 * Executes a regexp substitution via the RxSpencer library. For example:
 *
 *   Printf(stderr,"gsl%(rxspencer:[GSL_.*_][@1])s","GSL_Hello_") -> gslHello
 * ----------------------------------------------------------------------------- */
#if defined(HAVE_RXSPENCER)
#include <sys/types.h>
#include <rxspencer/regex.h>
#define USE_RXSPENCER
#endif

const char *skip_delim(char pb, char pe, const char *ce) 
{
  int end = 0;
  int lb = 0;
  while (!end && *ce != '\0') {
    if (*ce == pb) {
      ++lb;
    }
    if (*ce == pe) {
      if (!lb) {
	end = 1;
	--ce;
      } else {
	--lb;
      }
    }
    ++ce;
  }
  return end ? ce : 0;
}


#if defined(USE_RXSPENCER)
String *Swig_string_rxspencer(String *s) {
  String *res = 0;
  if (Len(s)) {
    const char *cs = Char(s);
    const char *cb;
    const char *ce;
    if (*cs == '[') {
      int retval;
      regex_t compiled;
      cb = ++cs;
      ce = skip_delim('[',']', cb);
      if (ce) {
	char bregexp[512];
	strncpy(bregexp, cb, ce - cb);
	bregexp[ce - cb] = '\0';
	++ce;
	retval = regcomp(&compiled, bregexp, REG_EXTENDED);
	if(retval == 0) {
	  cs = ce;
	  if (*cs == '[') {
	    cb = ++cs;
	    ce = skip_delim('[',']', cb) ;
	    if (ce) {
	      const char *cvalue = ce + 1;
	      int nsub = (int) compiled.re_nsub+1;
	      regmatch_t *pmatch = (regmatch_t *)malloc(sizeof(regmatch_t)*(nsub));
	      retval = regexec(&compiled,cvalue,nsub,pmatch,0);
	      if (retval != REG_NOMATCH) {
		char *spos = 0;
		res = NewStringWithSize(cb, ce - cb);
		spos = Strstr(res,"@");
		while (spos) {
		  char cd = *(++spos);
		  if (isdigit(cd)) {
		    char arg[8];
		    size_t len;
		    int i = cd - '0';
		    sprintf(arg,"@%d", i);
		    if (i < nsub && (len = pmatch[i].rm_eo - pmatch[i].rm_so)) {
		      char value[256];
		      strncpy(value,cvalue + pmatch[i].rm_so, len);
		      value[len] = 0;
		      Replaceall(res,arg,value);
		    } else {
		      Replaceall(res,arg,"");
		    }
		    spos = Strstr(res,"@");
		  } else if (cd == '@') {
		    spos = strstr(spos + 1,"@");
		  }
		}
	      }
	      free(pmatch);
	    }
	  }
	}
	regfree(&compiled);
      }
    }
  }
  if (!res) res = NewStringEmpty();
  return res;
}
#else
String *Swig_string_rxspencer(String *s) {
  (void)s;  
  return NewStringEmpty();
}
#endif


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
  DohEncoding("ctitle", Swig_string_ccase);
  DohEncoding("utitle", Swig_string_ucase);
  DohEncoding("typecode",Swig_string_typecode);
  DohEncoding("mangle", Swig_string_emangle);
  DohEncoding("command", Swig_string_command);
  DohEncoding("rxspencer", Swig_string_rxspencer);
  DohEncoding("schemify", Swig_string_schemify);

  /* aliases for the case encoders */
  DohEncoding("uppercase", Swig_string_upper);
  DohEncoding("lowercase", Swig_string_lower);
  DohEncoding("camelcase", Swig_string_ccase);
  DohEncoding("undercase", Swig_string_ucase);
  DohEncoding("firstuppercase", Swig_string_first_upper);
  DohEncoding("firstlowercase", Swig_string_first_lower);

  /* Initialize the swig keys */
  Swig_keys_init();

  /* Initialize typemaps */
  Swig_typemap_init();

  /* Initialize symbol table */
  Swig_symbol_init();

  /* Initialize type system */
  SwigType_typesystem_init();

  /* Initialize template system */
  SwigType_template_init();
}
