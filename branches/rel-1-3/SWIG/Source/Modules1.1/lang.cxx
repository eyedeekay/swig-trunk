/* ----------------------------------------------------------------------------- 
 * lang.cxx
 *
 *     Language base class functions.  Default C++ handling is also implemented here.
 * 
 * Author(s) : David Beazley (beazley@cs.uchicago.edu)
 *
 * Copyright (C) 1998-2000.  The University of Chicago
 * Copyright (C) 1995-1998.  The University of Utah and The Regents of the
 *                           University of California.
 *
 * See the file LICENSE for information on usage and redistribution.	
 * ----------------------------------------------------------------------------- */

static char cvsroot[] = "$Header$";

#include "swigmod.h"
#include <ctype.h>

/* Some status variables used during parsing */

static int      InClass = 0;          /* Parsing C++ or not */
static String  *ClassName = 0;        /* This is the real name of the current class */
static String  *ClassPrefix = 0;      /* Class prefix */
static String  *ClassType = 0;        /* Fully qualified type name to use */
int             Abstract = 0;
int             ImportMode = 0;
int             IsVirtual = 0;
static String  *AttributeFunctionGet = 0;
static String  *AttributeFunctionSet = 0;
static int      cplus_mode = 0;
static Node    *CurrentClass = 0;
int             line_number = 0;
char           *input_file = 0;
int             SmartPointer = 0;

extern    int           GenerateDefault;
extern    int           ForceExtern;
extern    int           NoExtern;

/* import modes */

#define  IMPORT_MODE     1
#define  IMPORT_MODULE   2

/* C++ access modes */

#define CPLUS_PUBLIC     0
#define CPLUS_PROTECTED  1
#define CPLUS_PRIVATE    2

/* ----------------------------------------------------------------------
 * Dispatcher::emit_one()
 *
 * Dispatch a single node
 * ---------------------------------------------------------------------- */

int Dispatcher::emit_one(Node *n) {
  String *wrn;
  int     ret = SWIG_OK;

  char *tag = Char(nodeType(n));
  if (!tag) {
    Printf(stderr,"SWIG: Fatal internal error. Malformed parse tree node!\n");
    return SWIG_ERROR;
  }
    
  /* Do not proceed if marked with an error */
    
  if (Getattr(n,"error")) return SWIG_OK;

  /* Look for warnings */
  wrn = Getattr(n,"feature:warnfilter");
  if (wrn) {
    Swig_warnfilter(wrn,1);
  }

  /* ============================================================
   * C/C++ parsing
   * ============================================================ */
    
  if (strcmp(tag,"extern") == 0) {
    ret = externDeclaration(n);
  } else if (strcmp(tag,"cdecl") == 0) {
    ret = cDeclaration(n);
  } else if (strcmp(tag,"enum") == 0) {
    ret = enumDeclaration(n);
  } else if (strcmp(tag,"enumitem") == 0) {
    ret = enumvalueDeclaration(n);
  } else if (strcmp(tag,"class") == 0) {
    ret = classDeclaration(n);
  } else if (strcmp(tag,"classforward") == 0) {
    ret = classforwardDeclaration(n);
  } else if (strcmp(tag,"constructor") == 0) {
    ret = constructorDeclaration(n);
  } else if (strcmp(tag,"destructor") == 0) {
    ret = destructorDeclaration(n);
  } else if (strcmp(tag,"access") == 0) {
    ret = accessDeclaration(n);
  } else if (strcmp(tag,"using") == 0) {
    ret = usingDeclaration(n);
  } else if (strcmp(tag,"namespace") == 0) {
    ret = namespaceDeclaration(n);
  } else if (strcmp(tag,"template") == 0) {
    ret = templateDeclaration(n);
  }
    
  /* ===============================================================
   *  SWIG directives
   * =============================================================== */

  else if (strcmp(tag,"top") == 0) {
    ret = top(n);
  } else if (strcmp(tag,"extend") == 0) {
    ret = extendDirective(n);
  } else if (strcmp(tag,"apply") == 0) {
    ret = applyDirective(n);
  } else if (strcmp(tag,"clear") == 0) {
    ret = clearDirective(n);
  } else if (strcmp(tag,"constant") == 0) {
    ret = constantDirective(n);
  } else if (strcmp(tag,"fragment") == 0) {
    ret = fragmentDirective(n);
  } else if (strcmp(tag,"import") == 0) {
    ret = importDirective(n);
  } else if (strcmp(tag,"include") == 0) {
    ret = includeDirective(n);
  } else if (strcmp(tag,"insert") == 0) {
    ret = insertDirective(n);
  } else if (strcmp(tag,"module") == 0) { 
    ret = moduleDirective(n);
  } else if (strcmp(tag,"native") == 0) {
    ret = nativeDirective(n);
  } else if (strcmp(tag,"pragma") == 0) {
    ret = pragmaDirective(n);
  } else if (strcmp(tag,"typemap") == 0) {
    ret = typemapDirective(n);
  } else if (strcmp(tag,"typemapcopy") == 0) {
    ret = typemapcopyDirective(n);
  } else if (strcmp(tag,"typemapitem") == 0) {
    ret = typemapitemDirective(n);
  } else if (strcmp(tag,"types") == 0) {
    ret = typesDirective(n);
  } else {
    Printf(stderr,"%s:%d. Unrecognized parse tree node type '%s'\n", input_file, line_number, tag);
    ret = SWIG_ERROR;
  }
  if (wrn) {
    Swig_warnfilter(wrn,0);
  }
  return ret;
}

/* ----------------------------------------------------------------------
 * Dispatcher::emit_children()
 *
 * Emit all children.
 * ---------------------------------------------------------------------- */

int Dispatcher::emit_children(Node *n) {
  Node *c;
  for (c = firstChild(n); c; c = nextSibling(c)) {
    emit_one(c);
  }
  return SWIG_OK;
}

/* Stubs for dispatcher class.  We don't do anything by default---up to derived class
   to fill in traversal code */

int Dispatcher::defaultHandler(Node *) { return SWIG_OK; }
int Dispatcher::extendDirective(Node *n) { return defaultHandler(n); }
int Dispatcher::applyDirective(Node *n) { return defaultHandler(n); }
int Dispatcher::clearDirective(Node *n) { return defaultHandler(n); }
int Dispatcher::constantDirective(Node *n) { return defaultHandler(n); }
int Dispatcher::fragmentDirective(Node *n) { return defaultHandler(n); }
int Dispatcher::importDirective(Node *n) { return defaultHandler(n); }
int Dispatcher::includeDirective(Node *n) { return defaultHandler(n); }
int Dispatcher::insertDirective(Node *n) { return defaultHandler(n); }
int Dispatcher::moduleDirective(Node *n) { return defaultHandler(n); }
int Dispatcher::nativeDirective(Node *n) { return defaultHandler(n); }
int Dispatcher::pragmaDirective(Node *n) { return defaultHandler(n); }
int Dispatcher::typemapDirective(Node *n) { return defaultHandler(n); }
int Dispatcher::typemapitemDirective(Node *n) { return defaultHandler(n); }
int Dispatcher::typemapcopyDirective(Node *n) { return defaultHandler(n); }
int Dispatcher::typesDirective(Node *n) { return defaultHandler(n); }
int Dispatcher::cDeclaration(Node *n) { return defaultHandler(n); }
int Dispatcher::externDeclaration(Node *n) { return defaultHandler(n); }
int Dispatcher::enumDeclaration(Node *n) { return defaultHandler(n); }
int Dispatcher::enumvalueDeclaration(Node *n) { return defaultHandler(n); }
int Dispatcher::classDeclaration(Node *n) { return defaultHandler(n); }
int Dispatcher::templateDeclaration(Node *n) { return defaultHandler(n); }
int Dispatcher::classforwardDeclaration(Node *n) { return defaultHandler(n); }
int Dispatcher::constructorDeclaration(Node *n) { return defaultHandler(n); }
int Dispatcher::destructorDeclaration(Node *n) { return defaultHandler(n); }
int Dispatcher::accessDeclaration(Node *n) { return defaultHandler(n); }
int Dispatcher::usingDeclaration(Node *n) { return defaultHandler(n); }
int Dispatcher::namespaceDeclaration(Node *n) { return defaultHandler(n); }

/* Allocators */
Language::Language() {
  symbols = NewHash();
  classtypes = NewHash();
  overloading = 0;
}

Language::~Language() {
  Delete(symbols);
  Delete(classtypes);
}

/* ----------------------------------------------------------------------
   emit_one()
   ---------------------------------------------------------------------- */

int Language::emit_one(Node *n) {
  int ret;
  
  if (!n) return SWIG_OK;

  if (Getattr(n,"feature:ignore")) return SWIG_OK;

  line_number = Getline(n);
  input_file = Char(Getfile(n));

  /*
    symtab = Getattr(n,"symtab");
    if (symtab) {
    symtab = Swig_symbol_setscope(symtab);
    }
  */
  ret = Dispatcher::emit_one(n);
  /*
    if (symtab) {
    Swig_symbol_setscope(symtab);
    }
  */
  return ret;
}


static Parm *nonvoid_parms(Parm *p) {
  if (p) {
    SwigType *t = Getattr(p,"type");
    if (SwigType_type(t) == T_VOID) return 0;
  }
  return p;
}

/* This is a hack */
SwigType *cplus_value_type(SwigType *t) {
  Node *n;
  if (!CPlusPlus) return 0;
  if (SwigType_isclass(t)) {
    SwigType *td = SwigType_typedef_resolve_all(t);
    if ((n = Swig_symbol_clookup(td,0))) {
      if ((Strcmp(nodeType(n),"class") == 0) && (!Getattr(n,"allocate:default_constructor") || (Getattr(n,"allocate:noassign")))) {
	String *s = NewStringf("SwigValueWrapper< %s >",t);
	Delete(td);
	return s;
      }
    }
    if (SwigType_issimple(td) && SwigType_istemplate(td)) {
      String *s = NewStringf("SwigValueWrapper< %s >",t);
      Delete(td);
      return s;
    }
    Delete(td);
  }
  return 0;
}

/* Patch C++ pass-by-value */
void Language::patch_parms(Parm *p) {
  while (p) {
    SwigType *t = Getattr(p,"type");
    SwigType *s = cplus_value_type(t);
    if (s) {
      Setattr(p,"alttype",s);
      Delete(s);
    }
    p = nextSibling(p);
  }
}

static Node *first_nontemplate(Node *n) {
  while (n) {
    if (Strcmp(nodeType(n),"template") != 0) return n;
    n = Getattr(n,"sym:nextSibling");
  }
  return n;
}

/* --------------------------------------------------------------------------
 * swig_pragma()
 *
 * Handle swig pragma directives.  
 * -------------------------------------------------------------------------- */

void swig_pragma(char *lang, char *name, char *value) {
  if (strcmp(lang,"swig") == 0) {
    if ((strcmp(name,"make_default") == 0) || ((strcmp(name,"makedefault") == 0))) {
      GenerateDefault = 1;
    } else if ((strcmp(name,"no_default") == 0) || ((strcmp(name,"nodefault") == 0))) {
      GenerateDefault = 0;
    } else if (strcmp(name,"attributefunction") == 0) {
      String *nvalue = NewString(value);
      char *s = strchr(Char(nvalue),':');
      if (!s) {
	Swig_error(input_file, line_number, "Bad value for attributefunction. Expected \"fmtget:fmtset\".\n");
      } else {
	*s = 0;
	AttributeFunctionGet = NewString(Char(nvalue));
	AttributeFunctionSet = NewString(s+1);
      }
      Delete(nvalue);
    } else if (strcmp(name,"noattributefunction") == 0) {
      AttributeFunctionGet = 0;
      AttributeFunctionSet = 0;
    }
  }
}

/* ----------------------------------------------------------------------
 * Language::top()   - Top of parsing tree 
 * ---------------------------------------------------------------------- */

int Language::top(Node *n) {
  return emit_children(n);
}

/* ----------------------------------------------------------------------
 * Language::extendDirective()
 * ---------------------------------------------------------------------- */

int Language::extendDirective(Node *n) {
  int oldam = Extend;
  int oldmode = cplus_mode;
  Extend = CWRAP_EXTEND;
  cplus_mode = CPLUS_PUBLIC;

  emit_children(n);

  Extend = oldam;
  cplus_mode = oldmode;
  return SWIG_OK;
}

/* ----------------------------------------------------------------------
 * Language::applyDirective()
 * ---------------------------------------------------------------------- */

int Language::applyDirective(Node *n) {

  Parm     *pattern = Getattr(n,"pattern");
  Node     *c = firstChild(n);
  while (c) {
    Parm   *apattern = Getattr(c,"pattern");
    if (ParmList_len(pattern) != ParmList_len(apattern)) {
      Swig_error(input_file, line_number, "Can't apply (%s) to (%s).  Number of arguments don't match.\n",
		 ParmList_str(pattern), ParmList_str(apattern));
    } else {
      if (!Swig_typemap_apply(pattern,apattern)) {
	Swig_warning(WARN_TYPEMAP_APPLY_UNDEF,input_file,line_number,"Can't apply (%s). No typemaps are defined.\n", ParmList_str(pattern));
      }
    }
    c = nextSibling(c);
  }
  return SWIG_OK;
}

/* ----------------------------------------------------------------------
 * Language::clearDirective()
 * ---------------------------------------------------------------------- */

int Language::clearDirective(Node *n) {
  Node *p;
  for (p = firstChild(n); p; p = nextSibling(p)) {
    ParmList *pattern = Getattr(p,"pattern");
    Swig_typemap_clear_apply(pattern);
  }
  return SWIG_OK;
}

/* ----------------------------------------------------------------------
 * Language::constantDirective()
 * ---------------------------------------------------------------------- */

int Language::constantDirective(Node *n) {

  if (CurrentClass && (cplus_mode != CPLUS_PUBLIC)) return SWIG_NOWRAP;

  if (!ImportMode) {
    Swig_require(&n,"name", "?value",NULL);
    String *name = Getattr(n,"name");
    String *value = Getattr(n,"value");
    if (!value) {
      value = Copy(name);
    } else {
      value = NewStringf("%(escape)s", value);
    }
    Setattr(n,"value", value);
    this->constantWrapper(n);
    Swig_restore(&n);
    return SWIG_OK;
  }
  return SWIG_NOWRAP;
}

/* ----------------------------------------------------------------------
 * Language::fragmentDirective()
 * ---------------------------------------------------------------------- */

int Language::fragmentDirective(Node *n) {
  String *name = Getattr(n,"name");
  String *code = Getattr(n,"code");
  String *section = Getattr(n,"section");
  Swig_fragment_register(name,section,code);
  return SWIG_OK;
}

/* ----------------------------------------------------------------------
 * Language::importDirective()
 * ---------------------------------------------------------------------- */

int Language::importDirective(Node *n) {
  int oldim = ImportMode;
  ImportMode = IMPORT_MODE;
  emit_children(n);
  ImportMode = oldim;
  return SWIG_OK;
}

/* ----------------------------------------------------------------------
 * Language::includeDirective()
 * ---------------------------------------------------------------------- */

int Language::includeDirective(Node *n) {
  emit_children(n);
  return SWIG_OK;
}

/* ----------------------------------------------------------------------
 * Language::insertDirective()
 * ---------------------------------------------------------------------- */

int Language::insertDirective(Node *n) {
  /* %insert directive */
  if ((!ImportMode) || Getattr(n,"generated")) {
    String *code     = Getattr(n,"code");
    String *section  = Getattr(n,"section");
    File *f = 0;
    if (!section) {     /* %{ ... %} */
      f = Swig_filebyname("header");
    } else {
      f = Swig_filebyname(section);
    }
    if (f) {
      Printf(f,"%s\n",code);
    } else {
      Swig_error(input_file,line_number,"Unknown target '%s' for %%insert directive.\n", section);
    }
    return SWIG_OK;
  } else {
    return SWIG_NOWRAP;
  }
}

/* ----------------------------------------------------------------------
 * Language::moduleDirective()
 * ---------------------------------------------------------------------- */

int Language::moduleDirective(Node *n) {
  /* %module directive */
  return SWIG_OK;
}

/* ----------------------------------------------------------------------
 * Language::nativeDirective()
 * ---------------------------------------------------------------------- */

int Language::nativeDirective(Node *n) {
  if (!ImportMode) {
    return nativeWrapper(n);
  } else {
    return SWIG_NOWRAP;
  }
}

/* ----------------------------------------------------------------------
 * Language::pragmaDirective()
 * ---------------------------------------------------------------------- */

int Language::pragmaDirective(Node *n) {
  /* %pragma directive */
  if (!ImportMode) {
    String *lan = Getattr(n,"lang");
    String *name = Getattr(n,"name");
    String *value = Getattr(n,"value");
    swig_pragma(Char(lan),Char(name),Char(value));
    /*	pragma(Char(lan),Char(name),Char(value)); */
    return SWIG_OK;
  }
  return SWIG_OK;
}

/* ----------------------------------------------------------------------
 * Language::typemapDirective()
 * ---------------------------------------------------------------------- */

extern "C" void Swig_cparse_replace_descriptor(String *s);

int Language::typemapDirective(Node *n) {
  /* %typemap directive */
  String *method = Getattr(n,"method");
  String *code   = Getattr(n,"code");
  Parm   *kwargs = Getattr(n,"kwargs");
  Node   *items  = firstChild(n);
  static  int  namewarn = 0;
    

  if (code && (Strstr(code,"$source") || (Strstr(code,"$target")))) {
    Swig_warning(WARN_TYPEMAP_SOURCETARGET,Getfile(n),Getline(n),"Deprecated typemap feature ($source/$target).\n");
    if (!namewarn) {
      Swig_warning(WARN_TYPEMAP_SOURCETARGET, Getfile(n), Getline(n),
		   "The use of $source and $target in a typemap declaration is deprecated.\n\
For typemaps related to argument input (in,ignore,default,arginit,check), replace\n\
$source by $input and $target by $1.   For typemaps related to return values (out,\n\
argout,ret,except), replace $source by $1 and $target by $result.  See the file\n\
Doc/Manual/Typemaps.html for complete details.\n");
      namewarn = 1;
    }
  }

  /* Replace $descriptor() macros */

  if (code) {
    Setfile(code,Getfile(n));
    Setline(code,Getline(n));
    Swig_cparse_replace_descriptor(code);
  }

  while (items) {
    Parm     *pattern   = Getattr(items,"pattern");
    Parm     *parms     = Getattr(items,"parms");
    if (code) {
      Swig_typemap_register(method,pattern,code,parms,kwargs);
    } else {
      Swig_typemap_clear(method,pattern);
    }
    items = nextSibling(items);
  }
  return SWIG_OK;

}

/* ----------------------------------------------------------------------
 * Language::typemapcopyDirective()
 * ---------------------------------------------------------------------- */

int Language::typemapcopyDirective(Node *n) {
  String *method  = Getattr(n,"method");
  Parm   *pattern = Getattr(n,"pattern");
  Node *items    = firstChild(n);
  int   nsrc = 0;
  nsrc = ParmList_len(pattern);
  while (items) {
    ParmList *npattern = Getattr(items,"pattern");
    if (nsrc != ParmList_len(npattern)) {
      Swig_error(input_file,line_number,"Can't copy typemap. Number of types differ.\n");
    } else {
      if (Swig_typemap_copy(method,pattern,npattern) < 0) {
	Swig_error(input_file, line_number, "Can't copy typemap.\n");
      }
    }
    items = nextSibling(items);
  }
  return SWIG_OK;
}

/* ----------------------------------------------------------------------
 * Language::typesDirective()
 * ---------------------------------------------------------------------- */

int Language::typesDirective(Node *n) {
  Parm  *parms = Getattr(n,"parms");
  while (parms) {
    SwigType *t = Getattr(parms,"type");
    String   *v = Getattr(parms,"value");
    if (!v) {
      SwigType_remember(t);
    } else {
      if (SwigType_issimple(t)) {
	SwigType_inherit(t,v,0);
      }
    }
    parms = nextSibling(parms);
  }
  return SWIG_OK;
}

/* ----------------------------------------------------------------------
 * Language::cDeclaration()
 * ---------------------------------------------------------------------- */

int Language::cDeclaration(Node *n) {

  String *name    = Getattr(n,"name");
  String *symname = Getattr(n,"sym:name");
  SwigType *type  = Getattr(n,"type");
  SwigType *decl  = Getattr(n,"decl");
  String *storage = Getattr(n,"storage");
  Node   *over;
  File   *f_header = 0;
  SwigType *ty, *fullty;

  if (CurrentClass && (cplus_mode != CPLUS_PUBLIC)) return SWIG_NOWRAP;

  if (Cmp(storage,"typedef") == 0) {
    Swig_save(&n,"type",NULL);
    SwigType *t = Copy(type);
    if (t) {
      SwigType_push(t,decl);
      Setattr(n,"type",t);
      typedefHandler(n);
    }
    Swig_restore(&n);
    return SWIG_OK;
  } else if (Cmp(storage,"friend") == 0) {
    Swig_warning(WARN_LANG_FRIEND_IGNORE, Getfile(n), Getline(n),
		 "friend function '%s' ignored.\n", name);
    return SWIG_NOWRAP;
  } 

  /* If in import mode, we proceed no further */
  if (ImportMode) return SWIG_NOWRAP;

  /* Overloaded symbol check */
  over = Swig_symbol_isoverloaded(n);
  if (!overloading) {
    if (over) over = first_nontemplate(over);
    if (over && (over != n)) {
      SwigType *tc = Copy(decl);
      SwigType *td = SwigType_pop_function(tc);
      String   *oname;
      String   *cname;
      if (CurrentClass) {
	oname = NewStringf("%s::%s",ClassName,name);
	cname = NewStringf("%s::%s",ClassName,Getattr(over,"name"));
      } else {
	oname = NewString(name);
	cname = NewString(Getattr(over,"name"));
      }
      
      SwigType *tc2 = Copy(Getattr(over,"decl"));
      SwigType *td2 = SwigType_pop_function(tc2);
      
      Swig_warning(WARN_LANG_OVERLOAD_DECL, input_file, line_number, "Overloaded declaration ignored.  %s\n", SwigType_str(td,SwigType_namestr(oname)));
      Swig_warning(WARN_LANG_OVERLOAD_DECL, Getfile(over), Getline(over),"Previous declaration is %s\n", SwigType_str(td2,SwigType_namestr(cname)));
      
      Delete(tc2);
      Delete(td2);
      Delete(tc);
      Delete(td);
      Delete(oname);
      Delete(cname);
      return SWIG_NOWRAP;
    }
  }

  if (symname && !validIdentifier(symname)) {
    Swig_warning(WARN_LANG_IDENTIFIER,input_file, line_number, "Can't wrap '%s' unless renamed to a valid identifier.\n",
		 symname);
    return SWIG_NOWRAP;
  }

  ty = Copy(type);
  SwigType_push(ty,decl);
  fullty = SwigType_typedef_resolve_all(ty);
  if (SwigType_isfunction(fullty)) {
    if (!SwigType_isfunction(ty)) {
      Delete(ty);
      ty = fullty;
      fullty = 0;
      ParmList *parms = SwigType_function_parms(ty);
      Setattr(n,"parms",parms);
    }
    /* Transform the node into a 'function' node and emit */
    if (!CurrentClass) {
      f_header = Swig_filebyname("header");

      if (!NoExtern) {
	if (f_header) {
	  if ((Cmp(storage,"extern") == 0) || (ForceExtern && !storage)) {
	    Printf(f_header,"extern %s;\n", SwigType_str(ty,name));
	  } else if (Cmp(storage,"externc") == 0) {
	    Printf(f_header,"extern \"C\" %s;\n", SwigType_str(ty,name));
	  }
	}
      }
    }
    /* This needs to check qualifiers */
    if (SwigType_isqualifier(ty)) {
      Setattr(n,"qualifier", SwigType_pop(ty));
    }
    Delete(SwigType_pop_function(ty));
    DohIncref(type);
    Setattr(n,"type",ty);
    functionHandler(n);
    Setattr(n,"type",type);
    Delete(ty);
    Delete(type);
    return SWIG_OK;
  } else {
    /* Some kind of variable declaration */
    Delattr(n,"decl");
    if (Getattr(n,"nested")) Setattr(n,"feature:immutable","1");
    if (!CurrentClass) {
      if ((Cmp(storage,"extern") == 0) || ForceExtern) {
	f_header = Swig_filebyname("header");
	if (!NoExtern) {
	  if (f_header) {
	    Printf(f_header,"extern %s;\n", SwigType_str(ty,name));
	  }
	}
      }
    }
    if (SwigType_isconst(ty)) {
      Setattr(n,"feature:immutable","1");
    }
    /* If an array and elements are const, then read-only */
    if (SwigType_isarray(ty)) {
      SwigType *tya = SwigType_array_type(ty);
      if (SwigType_isconst(tya)) {
	Setattr(n,"feature:immutable","1");
      }
    }
    DohIncref(type);
    Setattr(n,"type",ty);
    variableHandler(n);
    Setattr(n,"type",type);
    Setattr(n,"decl",decl);
    Delete(ty);
    Delete(type);
    Delete(fullty);
    return SWIG_OK;
  }
}

/* ----------------------------------------------------------------------
 * Language::functionHandler()
 * ---------------------------------------------------------------------- */

int
Language::functionHandler(Node *n) {
  Parm *p;
  p = Getattr(n,"parms");
  if (CPlusPlus) patch_parms(p);
  if (!CurrentClass) {
    globalfunctionHandler(n);
  } else {
    String *storage   = Getattr(n,"storage");
    if (Cmp(storage,"static") == 0) {
      staticmemberfunctionHandler(n);
    } else {
      memberfunctionHandler(n);
    }
  }
  return SWIG_OK;
}

/* ----------------------------------------------------------------------
 * Language::globalfunctionHandler()
 * ---------------------------------------------------------------------- */

int
Language::globalfunctionHandler(Node *n) {

  Swig_require(&n,"name","sym:name","type","?parms",NULL);

  String   *name    = Getattr(n,"name");
  String   *symname = Getattr(n,"sym:name");
  SwigType *type    = Getattr(n,"type");
  String   *storage = Getattr(n,"storage");  
  ParmList *parms   = Getattr(n,"parms");

  if (0 && (Cmp(storage,"static") == 0)) {
    Swig_restore(&n);
    return SWIG_NOWRAP;   /* Can't wrap static functions */
  } else {
    /* Check for callback mode */
    String *cb = Getattr(n,"feature:callback");
    if (cb) {
      String   *cbname = NewStringf(cb,symname);
      callbackfunctionHandler(n);
      if (Cmp(cbname, symname) == 0) {
	Delete(cbname);
	Swig_restore(&n);
	return SWIG_NOWRAP;
      }
      Delete(cbname);
    }
    Setattr(n,"parms",nonvoid_parms(parms));
    Setattr(n,"wrap:action", Swig_cresult(type,"result", Swig_cfunction_call(name,parms)));
    functionWrapper(n);
  }
  Swig_restore(&n);
  return SWIG_OK;
}

/* ----------------------------------------------------------------------
 * Language::callbackfunctionHandler()
 * ---------------------------------------------------------------------- */

int 
Language::callbackfunctionHandler(Node *n) {
  Swig_require(&n,"name","*sym:name","*type","?value",NULL);
  String *symname = Getattr(n,"sym:name");
  String *type    = Getattr(n,"type");
  String *name    = Getattr(n,"name");
  String *parms   = Getattr(n,"parms");
  String *cb      = Getattr(n,"feature:callback");
  String  *cbname = NewStringf(cb,symname);
  SwigType *cbty = Copy(type);
  SwigType_add_function(cbty,parms); 
  SwigType_add_pointer(cbty);

  Setattr(n,"sym:name", cbname);
  Setattr(n,"type", cbty);
  Setattr(n,"value", name);

  constantWrapper(n);
  Delete(cbname);
  Delete(cbty);

  Swig_restore(&n);
  return SWIG_OK;
}

/* ----------------------------------------------------------------------
 * Language::memberfunctionHandler()
 * ---------------------------------------------------------------------- */

int
Language::memberfunctionHandler(Node *n) {

  Swig_require(&n,"*name","*sym:name","*type","?parms","?value",NULL);

  String *storage   = Getattr(n,"storage");
  String   *name    = Getattr(n,"name");
  String   *symname = Getattr(n,"sym:name");
  SwigType *type    = Getattr(n,"type");
  String   *value   = Getattr(n,"value");
  ParmList *parms   = Getattr(n,"parms");
  String   *cb;

  if (Cmp(storage,"virtual") == 0) {
    if (Cmp(value,"0") == 0) {
      IsVirtual = PURE_VIRTUAL;
    } else {
      IsVirtual = PLAIN_VIRTUAL;
    }
  } else {
    IsVirtual = 0;
  }
  cb = Getattr(n,"feature:callback");
  if (cb) {
    Node   *cb = NewHash();
    String *cbname = NewStringf(cb,symname);
    String *cbvalue;
    SwigType *cbty = Copy(type);
    SwigType_add_function(cbty,parms); 
    SwigType_add_memberpointer(cbty,ClassName);
    cbvalue = NewStringf("&%s::%s",ClassName,name);
    Setattr(cb,"sym:name", cbname);
    Setattr(cb,"type", cbty);
    Setattr(cb,"value", cbvalue);
    Setattr(cb,"name", name);

    memberconstantHandler(n);

    Delete(cb);
    Delete(cbvalue);
    Delete(cbty);
    Delete(cbname);
    if (Cmp(cbname,symname) == 0) {
      Swig_restore(&n);
      return SWIG_NOWRAP;
    }
  }

  String *fname = Swig_name_member(ClassPrefix, symname);
  /* Transformation */
  Swig_MethodToFunction(n,ClassType, Getattr(n,"template") ? 0 : Extend | SmartPointer);
  Setattr(n,"sym:name",fname);
  functionWrapper(n);

  /*  DelWrapper(w);*/
  Delete(fname);
  Swig_restore(&n);
  return SWIG_OK;
}

/* ----------------------------------------------------------------------
 * Language::staticmemberfunctionHandler()
 * ---------------------------------------------------------------------- */

int
Language::staticmemberfunctionHandler(Node *n) {

  Swig_require(&n,"*name","*sym:name","*type",NULL);
  Swig_save(&n,"storage",NULL);
  String   *name    = Getattr(n,"name");
  String   *symname = Getattr(n,"sym:name");
  SwigType *type    = Getattr(n,"type");
  ParmList *parms   = Getattr(n,"parms");
  String   *code    = Getattr(n,"code");
  String   *cname, *mrename;

  if (!Extend) {
    cname = NewStringf("%s::%s",ClassName,name);
  } else {
    cname = Copy(Swig_name_member(ClassPrefix,name));    
  }
  mrename = Swig_name_member(ClassPrefix, symname);

  Setattr(n,"name",cname);
  Setattr(n,"sym:name",mrename);

  if ((Extend) && (code)) {
    /* Hmmm. An added static member.  We have to create a little wrapper for this */
    String *tmp = NewStringf("%s(%s)", cname, ParmList_str(parms));
    String *wrap = SwigType_str(type,tmp);
    Printv(wrap,code,"\n",NULL);
    Setattr(n,"wrap:code",wrap);
    Delete(tmp);
    Delete(wrap);
  }
  Delattr(n,"storage");
  globalfunctionHandler(n);

  Delete(cname);
  Delete(mrename);
  Swig_restore(&n);
  return SWIG_OK;
}

/* ----------------------------------------------------------------------
 * Language::variableHandler()
 * ---------------------------------------------------------------------- */

int
Language::variableHandler(Node *n) {
  if (!CurrentClass) {
    globalvariableHandler(n);
  } else {
    String *storage = Getattr(n,"storage");
    if ((Cmp(storage,"static") == 0)) {
      if (!SmartPointer) {
	staticmembervariableHandler(n);
      } 
    } else {
      membervariableHandler(n);
    }
  }
  return SWIG_OK;
}

/* ----------------------------------------------------------------------
 * Language::globalvariableHandler()
 * ---------------------------------------------------------------------- */

int
Language::globalvariableHandler(Node *n) {
  String *storage = Getattr(n,"storage");
  if (0 && (Cmp(storage,"static") == 0)) return SWIG_NOWRAP;
  variableWrapper(n);
  return SWIG_OK;
}

/* ----------------------------------------------------------------------
 * Language::membervariableHandler()
 * ---------------------------------------------------------------------- */

int
Language::membervariableHandler(Node *n) {

  Swig_require(&n,"*name","*sym:name","*type",NULL);
  Swig_save(&n,"parms",NULL);

  String   *name    = Getattr(n,"name");
  String   *symname = Getattr(n,"sym:name");
  SwigType *type  = Getattr(n,"type");

  if (!AttributeFunctionGet) {
  
    String *mrename_get, *mrename_set;
    
    mrename_get = Swig_name_get(Swig_name_member(ClassPrefix, symname));
    mrename_set = Swig_name_set(Swig_name_member(ClassPrefix, symname));

    /* Create a function to set the value of the variable */
    
    if (!Getattr(n,"feature:immutable")) {
      int       make_wrapper = 1;
      String *tm = 0;
      String *target = 0;
      if (!Extend) {
	target = NewStringf("%s->%s", Swig_cparm_name(0,0),name);
	tm = Swig_typemap_lookup_new("memberin",n,target,0);
      }
      Swig_MembersetToFunction(n,ClassType,Extend | SmartPointer);
      if (!Extend) {
	/* Check for a member in typemap here */

	/* String *tm = Swig_typemap_lookup((char *) "memberin",type,name,target,Swig_cparm_name(0,1),target,0);*/
	if (!tm) {
	  if (SwigType_isarray(type)) {
	    /*	  Printf(stderr,"%s:%d. Warning. Array member %s will be read-only.\n", input_file, line_number, name);*/
	    make_wrapper = 0;
	  }
	}  else {
	  Replace(tm,"$source", Swig_cparm_name(0,1), DOH_REPLACE_ANY);
	  Replace(tm,"$target", target, DOH_REPLACE_ANY);
	  Replace(tm,"$input",Swig_cparm_name(0,1),DOH_REPLACE_ANY);
	  Replace(tm,"$self",Swig_cparm_name(0,0),DOH_REPLACE_ANY);
	  Setattr(n,"wrap:action", tm);
	  Delete(tm);
	}
	Delete(target);
      }
      if (make_wrapper) {
	Setattr(n,"sym:name", mrename_set);
	functionWrapper(n);
      } else {
	Setattr(n,"feature:immutable","1");
      }
      /* Restore parameters */
      Setattr(n,"type",type);
      Setattr(n,"name",name);
      Setattr(n,"sym:name",symname);
    }
    /* Emit get function */
    {
      Swig_MembergetToFunction(n,ClassType,Extend | SmartPointer);
      Setattr(n,"sym:name",  mrename_get);
      functionWrapper(n);
    }
    Delete(mrename_get);
    Delete(mrename_set);

  } else {

    /* This code is used to support the attributefunction directive 
       where member variables are converted automagically to 
       accessor functions */

#if 0    
    Parm *p;
    String *gname;
    SwigType *vty;
    p = NewParm(type,0);
    gname = NewStringf(AttributeFunctionGet,symname);
    if (!Extend) {
      ActionFunc = Copy(Swig_cmemberget_call(name,type));
      cpp_member_func(Char(gname),Char(gname),type,0);
      Delete(ActionFunc);
    } else {
      String *cname = Copy(Swig_name_get(name));
      cpp_member_func(Char(cname),Char(gname),type,0);
      Delete(cname);
    }
    Delete(gname);
    if (!Getattr(n,"feature:immutable")) {
      gname = NewStringf(AttributeFunctionSet,symname);
      vty = NewString("void");
      if (!Extend) {
	ActionFunc = Copy(Swig_cmemberset_call(name,type));
	cpp_member_func(Char(gname),Char(gname),vty,p);
	Delete(ActionFunc);
      } else {
	String *cname = Copy(Swig_name_set(name));
	cpp_member_func(Char(cname),Char(gname),vty,p);
	Delete(cname);
      }
      Delete(gname);
    }
    ActionFunc = 0;
#endif
  }
  Swig_restore(&n);
  return SWIG_OK;
}

/* ----------------------------------------------------------------------
 * Language::staticmembervariableHandler()
 * ---------------------------------------------------------------------- */

int 
Language::staticmembervariableHandler(Node *n)
{
  Swig_require(&n,"*name","*sym:name","*type", "?value", NULL);
  String *value = Getattr(n,"value");
  if (!value) {
    String *name    = Getattr(n,"name");
    String *symname = Getattr(n,"sym:name");
    String *cname, *mrename;
    
    /* Create the variable name */
    mrename = Swig_name_member(ClassPrefix, symname);
    cname = NewStringf("%s::%s", ClassName,name);
    
    Setattr(n,"sym:name",mrename);
    Setattr(n,"name", cname);
    
    /* Wrap as an ordinary global variable */
    variableWrapper(n);
    
    Delete(mrename);
    Delete(cname);
  } else {
    String *name    = Getattr(n,"name");
    String *cname   = NewStringf("%s::%s", ClassName,name);
    String* value   = SwigType_namestr(cname);
    Setattr(n, "value", value);
    
    SwigType *t1    = SwigType_typedef_resolve_all(Getattr(n,"type"));
    SwigType *t2    = SwigType_strip_qualifiers(t1);
    Setattr(n, "type", t2);
    Delete(t1);
    Delete(t2);
    
    memberconstantHandler(n);
    Delete(cname);
  }  
  
  Swig_restore(&n);
  return SWIG_OK;
}


/* ----------------------------------------------------------------------
 * Language::externDeclaration()
 * ---------------------------------------------------------------------- */

int Language::externDeclaration(Node *n) {
  return emit_children(n);
}

/* ----------------------------------------------------------------------
 * Language::enumDeclaration()
 * ---------------------------------------------------------------------- */

int Language::enumDeclaration(Node *n) {
  if (!ImportMode) {
    emit_children(n);
  }
  return SWIG_OK;
}

/* ----------------------------------------------------------------------
 * Language::enumvalueDeclaration()
 * ---------------------------------------------------------------------- */

int Language::enumvalueDeclaration(Node *n) {
  if (CurrentClass && (cplus_mode != CPLUS_PUBLIC)) return SWIG_NOWRAP;

  Swig_require(&n,"*name", "?value",NULL);
  String *value = Getattr(n,"value");
  String *name  = Getattr(n,"name");
  String *tmpValue;
  
  if (value)
    tmpValue = NewString(value);
  else
    tmpValue = NewString(name);
  Setattr(n, "value", tmpValue);

  if (!CurrentClass) {
    Setattr(n,"name",tmpValue); /* for wrapping of enums in a namespace when emit_action is used */
    constantWrapper(n);
  } else {
    memberconstantHandler(n);
  }
  
  Delete(tmpValue);
  Swig_restore(&n);
  return SWIG_OK;
}

/* ----------------------------------------------------------------------------- 
 * Language::memberconstantHandler()
 * ----------------------------------------------------------------------------- */

int Language::memberconstantHandler(Node *n) {

  Swig_require(&n,"*name","*sym:name","*value",NULL);

  String *name    = Getattr(n,"name");
  String *symname = Getattr(n,"sym:name");
  String *value   = Getattr(n,"value");

  String *mrename;
  String *new_value;

  mrename = Swig_name_member(ClassPrefix, symname);
  /*  Fixed by namespace-enum patch
      if ((!value) || (Cmp(value,name) == 0)) {
      new_value = NewStringf("%s::%s",ClassName,name);
      } else {
      new_value = NewString(value);
      }
  */
  new_value = Copy(value);
  Setattr(n,"sym:name", mrename);
  Setattr(n,"value", new_value);
  Setattr(n,"name", NewStringf("%s::%s", ClassName,name));
  constantWrapper(n);
  Delete(mrename);
  Delete(new_value);
  Swig_restore(&n);
  return SWIG_OK;
}

/* ----------------------------------------------------------------------
 * Language::typedefHandler() 
 * ---------------------------------------------------------------------- */

int Language::typedefHandler(Node *) {
  return SWIG_OK;
}

/* ----------------------------------------------------------------------
 * Language::classDeclaration()
 * ---------------------------------------------------------------------- */

int Language::classDeclaration(Node *n) {
  String *kind = Getattr(n,"kind");
  String *name = Getattr(n,"name");
  String *tdname = Getattr(n,"tdname");
  String *symname = Getattr(n,"sym:name");

  char *classname = tdname ? Char(tdname) : Char(name);
  char *iname = Char(symname);
  int   strip = (tdname || CPlusPlus) ? 1 : 0;

  /* Check symbol name for template.   If not renamed. Issue a warning */
  /*    Printf(stdout,"sym:name = %s\n", symname); */

  if (!validIdentifier(symname)) {
    Swig_warning(WARN_LANG_IDENTIFIER, input_file, line_number, "Can't wrap class %s unless renamed to a valid identifier.\n",
		 SwigType_namestr(symname));
    return SWIG_NOWRAP;
  }

  Swig_save(&n,"name",NULL);
  Setattr(n,"name",classname);

  if (Cmp(kind,"class") == 0) {
    cplus_mode = CPLUS_PRIVATE;
  } else {
    cplus_mode = CPLUS_PUBLIC;
  }

  ClassName = NewString(classname);
  ClassPrefix = NewString(iname);
  if (strip) {
    ClassType = NewString(classname);
  } else {
    ClassType = NewStringf("%s %s", kind, classname);
  }
  Setattr(n,"classtype", SwigType_namestr(ClassType));

  InClass = 1;
  CurrentClass = n;

  if (Getattr(n,"abstract")) {
    Abstract = 1;
  } else {
    Abstract = 0;
  }

  /* Call classHandler() here */
  if (!ImportMode) 
    classHandler(n);
  else
    Language::classHandler(n);

  InClass = 0;
  CurrentClass = 0;
  Delete(ClassType);     ClassType = 0;
  Delete(ClassPrefix);   ClassPrefix = 0;
  Delete(ClassName);     ClassName = 0;
  Swig_restore(&n);
  return SWIG_OK;
}

/* ----------------------------------------------------------------------
 * Language::classHandler()
 * ---------------------------------------------------------------------- */

int Language::classHandler(Node *n) {

  /* Emit all of the class members */
  emit_children(n);

  /* Look for smart pointer handling */
  if (Getattr(n,"allocate:smartpointer")) {
    List *methods = Getattr(n,"allocate:smartpointer");
    cplus_mode = CPLUS_PUBLIC;
    SmartPointer = CWRAP_SMART_POINTER;
    Node *c;
    for (c = Firstitem(methods); c; c= Nextitem(methods)) {
      /*      Swig_print_node(c); */
      emit_one(c);
    }
    SmartPointer = 0;
  }

  cplus_mode = CPLUS_PUBLIC;
  if (!ImportMode && (GenerateDefault && !Getattr(n,"feature:nodefault"))) {
    if (!Getattr(n,"has_constructor") && !Getattr(n,"allocate:has_constructor") && (Getattr(n,"allocate:default_constructor"))) {
      /* Note: will need to change this to support different kinds of classes */
      if (!Abstract) {
	Setattr(CurrentClass,"feature:new","1");
	constructorHandler(CurrentClass);
	Delattr(CurrentClass,"feature:new");
      }
    }
    if (!Getattr(n,"has_destructor") && (!Getattr(n,"allocate:has_destructor")) && (Getattr(n,"allocate:default_destructor"))) {
      destructorHandler(CurrentClass);
    }
  }
  return SWIG_OK;
}

/* ----------------------------------------------------------------------
 * Language::classforwardDeclaration()
 * ---------------------------------------------------------------------- */

int Language::classforwardDeclaration(Node *n) {
  return SWIG_OK;
}

/* ----------------------------------------------------------------------
 * Language::constructorDeclaration()
 * ---------------------------------------------------------------------- */

int Language::constructorDeclaration(Node *n) {
  String *name = Getattr(n,"name");
  String *symname = Getattr(n,"sym:name");

  if (!CurrentClass) return SWIG_NOWRAP;
  if (cplus_mode != CPLUS_PUBLIC) return SWIG_NOWRAP;
  if (ImportMode) return SWIG_NOWRAP;

  /* Name adjustment for %name */
  Swig_save(&n,"sym:name",NULL);

  {
    String *base = Swig_scopename_last(name);
    if ((Strcmp(base,symname) == 0) && (Strcmp(symname, ClassPrefix) != 0)) {
      Setattr(n,"sym:name", ClassPrefix);
    }
    Delete(base);
  }
  /* Only create a constructor if the class is not abstract */

  if (!Abstract) {
    Node *over;
    over = Swig_symbol_isoverloaded(n);
    if (over) over = first_nontemplate(over);
    if ((over) && (!overloading)) {
      /* If the symbol is overloaded.  We check to see if it is a copy constructor.  If so, 
	 we invoke copyconstructorHandler() as a special case. */
      if (Getattr(n,"copy_constructor") && (!Getattr(CurrentClass,"has_copy_constructor"))) {
	copyconstructorHandler(n);
	Setattr(CurrentClass,"has_copy_constructor","1");
      } else {
	if (Getattr(over,"copy_constructor")) over = Getattr(over,"sym:nextSibling");
	if (over != n) {
	  String *oname = NewStringf("%s::%s", ClassName, Swig_scopename_last(SwigType_namestr(name)));
	  String *cname = NewStringf("%s::%s", ClassName, Swig_scopename_last(SwigType_namestr(Getattr(over,"name"))));
	  SwigType *decl = Getattr(n,"decl");
	  Swig_warning(WARN_LANG_OVERLOAD_CONSTRUCT, input_file, line_number,
		       "Overloaded constructor ignored.  %s\n", SwigType_str(decl,SwigType_namestr(oname)));
	  Swig_warning(WARN_LANG_OVERLOAD_CONSTRUCT, Getfile(over), Getline(over),
		       "Previous declaration is %s\n", SwigType_str(Getattr(over,"decl"),SwigType_namestr(cname)));
	  Delete(oname);
	  Delete(cname);
	} else {
	  constructorHandler(n);
	}
      }
    } else {
      if (name && (Cmp(name,ClassName)) && !(Getattr(n,"template"))) {
	Printf(stdout,"name = '%s', ClassName='%s'\n", name, ClassName);
	Swig_warning(WARN_LANG_RETURN_TYPE, input_file,line_number,"Function %s must have a return type.\n", 
		     name);
	Swig_restore(&n);
	return SWIG_NOWRAP;
      }
      constructorHandler(n);
    }
  }
  Setattr(CurrentClass,"has_constructor","1");
  Swig_restore(&n);
  return SWIG_OK;
}

/* ----------------------------------------------------------------------
 * Language::constructorHandler()
 * ---------------------------------------------------------------------- */

int 
Language::constructorHandler(Node *n) {
  Swig_require(&n,"?name","*sym:name","?type","?parms",NULL);
  String *symname = Getattr(n,"sym:name");
  String *mrename;
  Parm   *parms = Getattr(n,"parms");

  mrename = Swig_name_construct(symname);
  if (CPlusPlus) patch_parms(parms);
  Swig_ConstructorToFunction(n,ClassType,CPlusPlus,Getattr(n,"template") ? 0 :Extend);
  Setattr(n,"sym:name", mrename);
  functionWrapper(n);
  Delete(mrename);
  Swig_restore(&n);
  return SWIG_OK;
}

/* ----------------------------------------------------------------------
 * Language::copyconstructorHandler()
 * ---------------------------------------------------------------------- */

int
Language::copyconstructorHandler(Node *n) {
  Swig_require(&n,"?name","*sym:name","?type","?parms", NULL);
  String *symname = Getattr(n,"sym:name");
  String *mrename;
  Parm   *parms = Getattr(n,"parms");
  if (CPlusPlus) patch_parms(parms);
  mrename = Swig_name_copyconstructor(symname);
  Swig_ConstructorToFunction(n,ClassType, CPlusPlus, Getattr(n,"template") ? 0 : Extend);
  Setattr(n,"sym:name", mrename);
  functionWrapper(n);
  Delete(mrename);
  Swig_restore(&n);
  return SWIG_OK;
}

/* ----------------------------------------------------------------------
 * Language::destructorDeclaration()
 * ---------------------------------------------------------------------- */

int Language::destructorDeclaration(Node *n) {

  if (!CurrentClass) return SWIG_NOWRAP;
  if (cplus_mode != CPLUS_PUBLIC) return SWIG_NOWRAP;
  if (ImportMode) return SWIG_NOWRAP;

  Swig_save(&n,"name", "sym:name",NULL);

  char *c = GetChar(n,"name");
  if (c && (*c == '~')) Setattr(n,"name",c+1);

  c = GetChar(n,"sym:name");
  if (c && (*c == '~')) Setattr(n,"sym:name",c+1);

  /* Name adjustment for %name */

  String *name = Getattr(n,"name");
  String *symname = Getattr(n,"sym:name");

  if ((Strcmp(name,symname) == 0) || (Strcmp(symname,ClassPrefix) != 0)) {
    Setattr(n,"sym:name", ClassPrefix);
  }

  destructorHandler(n);

  Setattr(CurrentClass,"has_destructor","1");
  Swig_restore(&n);
  return SWIG_OK;
}

/* ----------------------------------------------------------------------
 * Language::destructorHandler()
 * ---------------------------------------------------------------------- */

int Language::destructorHandler(Node *n) {
  Swig_require(&n,"?name","*sym:name",NULL);
  Swig_save(&n,"type","parms",NULL);

  String *symname = Getattr(n,"sym:name");
  String *mrename;
  char *csymname = Char(symname);
  if (csymname && (*csymname == '~')) csymname +=1;

  mrename = Swig_name_destroy(csymname);
 
  Swig_DestructorToFunction(n,ClassType,CPlusPlus,Extend);
  Setattr(n,"sym:name", mrename);
  functionWrapper(n);
  Delete(mrename);
  Swig_restore(&n);
  return SWIG_OK;
}

/* ----------------------------------------------------------------------
 * Language::accessDeclaration()
 * ---------------------------------------------------------------------- */

int Language::accessDeclaration(Node *n) {
  String *kind = Getattr(n,"kind");
  if (Cmp(kind,"public") == 0) {
    cplus_mode = CPLUS_PUBLIC;
  } else if (Cmp(kind,"private") == 0) {
    cplus_mode = CPLUS_PRIVATE;
  } else if (Cmp(kind,"protected") == 0) {
    cplus_mode = CPLUS_PROTECTED;
  }
  return SWIG_OK;
}

/* -----------------------------------------------------------------------------
 * Language::namespaceDeclaration()
 * ----------------------------------------------------------------------------- */

int Language::namespaceDeclaration(Node *n) {
  if (Getattr(n,"alias")) return SWIG_OK;
  emit_children(n);
  return SWIG_OK;
}

int Language::validIdentifier(String *s) {
  char *c = Char(s);
  while (*c) {
    if (!(isalnum(*c) || (*c == '_'))) return 0;
    c++;
  }
  return 1;
}

/* Stubs. Language modules need to implement these */

/* ----------------------------------------------------------------------
 * Language::constantWrapper()
 * ---------------------------------------------------------------------- */

int Language::constantWrapper(Node *n) {
  String   *name  = Getattr(n,"sym:name");
  SwigType *type  = Getattr(n,"type");
  String   *value = Getattr(n,"value");

  Printf(stdout,"constantWrapper   : %s = %s\n", SwigType_str(type,name), value);
  return SWIG_OK;
}

/* ----------------------------------------------------------------------
 * Language::variableWrapper()
 * ---------------------------------------------------------------------- */
int Language::variableWrapper(Node *n) {
  Swig_require(&n,"*name","*sym:name","*type","?parms",NULL);
  String *symname    = Getattr(n,"sym:name");
  SwigType *type  = Getattr(n,"type");
  String *name   = Getattr(n,"name");

  /* If no way to set variables.  We simply create functions */
  if (!Getattr(n,"feature:immutable")) {
    int make_wrapper = 1;
    String *tm = Swig_typemap_lookup_new("globalin", n, name, 0);

    Swig_VarsetToFunction(n);
    Setattr(n,"sym:name", Swig_name_set(symname));

    /*    String *tm = Swig_typemap_lookup((char *) "globalin",type,name,name,Swig_cparm_name(0,0),name,0);*/

    if (!tm) {
      if (SwigType_isarray(type)) {
	/*	  Printf(stderr,"%s:%d. Warning. Array member %s will be read-only.\n", input_file, line_number, name);*/
	make_wrapper = 0;
      }
    }  else {
      Replace(tm,"$source", Swig_cparm_name(0,0), DOH_REPLACE_ANY);
      Replace(tm,"$target", name, DOH_REPLACE_ANY);
      Replace(tm,"$input",Swig_cparm_name(0,0),DOH_REPLACE_ANY);
      Setattr(n,"wrap:action", tm);
      Delete(tm);
    }
    if (make_wrapper) {
      functionWrapper(n);
    }
    Setattr(n,"sym:name",symname);
    Setattr(n,"type",type);
    Setattr(n,"name",name);
  }
  Swig_VargetToFunction(n);
  Setattr(n,"sym:name", Swig_name_get(symname));
  functionWrapper(n);
  Swig_restore(&n);
  return SWIG_OK;
}

/* ----------------------------------------------------------------------
 * Language::functionWrapper()
 * ---------------------------------------------------------------------- */

int Language::functionWrapper(Node *n) {
  String   *name   = Getattr(n,"sym:name");
  SwigType *type   = Getattr(n,"type");
  ParmList *parms  = Getattr(n,"parms");

  Printf(stdout,"functionWrapper   : %s\n", SwigType_str(type, NewStringf("%s(%s)", name, ParmList_str(parms))));
  Printf(stdout,"           action : %s\n", Getattr(n,"wrap:action")); 
  return SWIG_OK;
}

/* -----------------------------------------------------------------------------
 * Language::nativeWrapper()
 * ----------------------------------------------------------------------------- */

int Language::nativeWrapper(Node *n) {
  return SWIG_OK;
}

void Language::main(int argc, char *argv[]) {
}

/* -----------------------------------------------------------------------------
 * Language::addSymbol()
 *
 * Adds a symbol entry.  Returns 1 if the symbol is added successfully.
 * Prints an error message and returns 0 if a conflict occurs.
 * ----------------------------------------------------------------------------- */

int
Language::addSymbol(String *s, Node *n) {
  Node *c = Getattr(symbols,s);
  if (c && (c != n)) {
    Swig_error(input_file, line_number, "Error. '%s' is multiply defined in the generated module.\n", s);
    Swig_error(Getfile(c),Getline(c), "Previous declaration of '%s'\n", s);
    return 0;
  }
  Setattr(symbols,s,n);
  return 1;
}

/* -----------------------------------------------------------------------------
 * Language::symbolLookup()
 * ----------------------------------------------------------------------------- */

Node *
Language::symbolLookup(String *s) {
  return Getattr(symbols,s);
}

/* -----------------------------------------------------------------------------
 * Language::classLookup()
 *
 * Tries to locate a class from a type definition
 * ----------------------------------------------------------------------------- */

Node *
Language::classLookup(SwigType *s) {
  Node *n = 0;
  SwigType *lt, *ty1,*ty2;
  String *base;
  String *prefix;
  Symtab  *stab = 0;

  /* Look in hash of cached values */
  n = Getattr(classtypes,s);
  if (n) return n;

  lt = SwigType_ltype(s);
  ty1 = SwigType_typedef_resolve_all(lt);
  ty2 = SwigType_strip_qualifiers(ty1);
  Delete(lt);
  Delete(ty1);

  base = SwigType_base(ty2);

  Replaceall(base,"class ","");
  Replaceall(base,"struct ","");
  Replaceall(base,"union ","");

  prefix = SwigType_prefix(ty2);

  while (!n) {
    Hash *nstab;
    n = Swig_symbol_clookup(base,stab);
    if (!n) break;
    if (Strcmp(nodeType(n),"class") == 0) break;
    n = parentNode(n);
    if (!n) break;
    nstab = Getattr(n,"sym:symtab");
    n = 0;
    if ((!nstab) || (nstab == stab)) {
      break;
    }
    stab = nstab;
  }
  /* Do a symbol table search on the base type */
  /*    n = Swig_symbol_clookup(base,0); */
  if (n) {
    /* Found a match.  Look at the prefix.  We only allow
       a few cases: pointers, references, and simple */
    if ((Len(prefix) == 0) ||               /* Simple type */
	(Strcmp(prefix,"p.") == 0) ||       /* pointer     */ 
	(Strcmp(prefix,"r.") == 0)) {       /* reference   */
      Setattr(classtypes,Copy(s),n);
    } else {
      n = 0;
    }
  }
  Delete(ty2);
  Delete(base);
  Delete(prefix);

  if (n && (Getattr(n,"feature:ignore"))) return 0;
  return n; 
}

/* -----------------------------------------------------------------------------
 * Language::allow_overloading()
 * ----------------------------------------------------------------------------- */

void Language::allow_overloading() {
  overloading = 1;
}

/* -----------------------------------------------------------------------------
 * Language::is_wrapping_class()
 * ----------------------------------------------------------------------------- */

int Language::is_wrapping_class() {
    return InClass;
}

/* -----------------------------------------------------------------------------
 * Language::getCurrentClass()
 * ----------------------------------------------------------------------------- */

Node * Language::getCurrentClass() const {
    return CurrentClass;
}

/* -----------------------------------------------------------------------------
 * Language::getClassName()
 * ----------------------------------------------------------------------------- */

String * Language::getClassName() const {
    return ClassName;
}

/* -----------------------------------------------------------------------------
 * Language::getClassPrefix()
 * ----------------------------------------------------------------------------- */

String * Language::getClassPrefix() const {
    return ClassPrefix;
}
