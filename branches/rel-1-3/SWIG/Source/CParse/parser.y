%{
/* -----------------------------------------------------------------------------
 * parser.yxx
 *
 *     YACC parser for SWIG1.1.   This grammar is a broken subset of C/C++.
 *     This file is in the process of being deprecated.
 *
 * Author(s) : David Beazley (beazley@cs.uchicago.edu)
 *
 * Copyright (C) 1998-2001.  The University of Chicago
 * Copyright (C) 1995-1998.  The University of Utah and The Regents of the
 *                           University of California.
 *
 * See the file LICENSE for information on usage and redistribution.
 * ----------------------------------------------------------------------------- */

#define yylex yylex

static char cvsroot[] = "$Header$";

#include "cparse.h"
#include "preprocessor.h"
#include <ctype.h>

/* We do this for portability */
#undef alloca
#define alloca malloc

/* -----------------------------------------------------------------------------
 *                               Externals
 * ----------------------------------------------------------------------------- */

extern int   yylex();
extern void  yyerror (const char *s);

/* scanner.cxx */

extern int  line_number;
extern int  start_line;
extern void skip_balanced(int startchar, int endchar);
extern void skip_decl(void);
extern void scanner_check_typedef(void);
extern void scanner_ignore_typedef(void);
extern void scanner_last_id(int);
extern void start_inline(char *, int);
extern String *scanner_ccode;
extern void cparse_error(String *, int, char *, ...);

/* NEW Variables */

extern void generate_all(Node *);

static Node    *top = 0;      /* Top of the generated parse tree */
static int      unnamed = 0;  /* Unnamed datatype counter */
static Hash    *addmethods = 0;     /* Hash table of added methods */
static Hash    *classes = 0;        /* Hash table of classes */
static Symtab  *prev_symtab = 0;
static Node    *current_class = 0;
       String  *ModuleName = 0;
static String  *Classprefix = 0;  
static int      inclass = 0;
static int      templatemode = 0;
static int      templatenum = 0;
static Hash    *templatetypes = 0;
static String  *templatename = 0;        /* Name of the template used during expansion */
static String  *templateiname = 0;       /* Instantiation name of the template being parsed */
static String  *templateargs = 0;
static Hash    *templatemaps = 0;        /* Mapping of templates to %template directive */

int      ShowTemplates = 0;    /* Debugging mode */

/* -----------------------------------------------------------------------------
 *                            Assist Functions
 * ----------------------------------------------------------------------------- */

static Node *new_node(const String_or_char *tag) {
  Node *n = NewHash();
  set_nodeType(n,tag);
  Setfile(n,input_file);
  Setline(n,line_number);
  return n;
}

/* -----------------------------------------------------------------------------
 *                              Variables
 * ----------------------------------------------------------------------------- */

      char  *typemap_lang = 0;    /* Current language setting */
       int            Error = 0;

static int cplus_mode  = 0;
static char  *class_rename = 0;

/* C++ modes */

#define  CPLUS_PUBLIC    1
#define  CPLUS_PRIVATE   2
#define  CPLUS_PROTECTED 3

void SWIG_typemap_lang(const char *tm_lang) {
  typemap_lang = Swig_copy_string(tm_lang);
}

/* -----------------------------------------------------------------------------
 *                           Assist functions
 * ----------------------------------------------------------------------------- */

/* Perform type-promotion for binary operators */
static int promote(int t1, int t2) {
  return t1 > t2 ? t1 : t2;
}

static String *yyrename = 0;

/* Forward renaming operator */
static Hash   *rename_hash = 0;
static Hash   *namewarn_hash = 0;
static Hash   *features_hash = 0;

static void
rename_add(char *name, SwigType *decl, char *newname) {
  String *nname;
  if (!rename_hash) rename_hash = NewHash();
  if (Classprefix) {
    nname = NewStringf("%s::%s",Classprefix, name);
  } else {
    nname = NewString(name);
  }
  Swig_name_object_set(rename_hash,nname,decl,NewString(newname));
  Delete(nname);
}

static void
namewarn_add(char *name, SwigType *decl, char *warning) {
  String *nname;
  if (!namewarn_hash) namewarn_hash = NewHash();
  if (Classprefix) {
    nname = NewStringf("%s::%s",Classprefix, name);
  } else {
    nname = NewString(name);
  }
  Swig_name_object_set(namewarn_hash,nname,decl,NewString(warning));
  Delete(nname);
}

static void
rename_inherit(String *base, String *derived) {
  Swig_name_object_inherit(rename_hash,base,derived);
  Swig_name_object_inherit(namewarn_hash,base,derived);
  Swig_name_object_inherit(features_hash,base,derived);
}

/* Generate the symbol table name for an object */
/* This is a bit of a mess. Need to clean up */

static String *make_name(String *name,SwigType *decl) {
  String *rn = 0;
  String *origname = name;
  int     destructor = 0;

  if (name && (*(Char(name)) == '~')) {
    destructor = 1;
  }
  if (yyrename) {
    String *s = yyrename;
    yyrename = 0;
    if (destructor) {
      Insert(s,0,"~");
    }
    return s;
  }
  if (!name) return 0;
  /* Check to see if the name is in the hash */
  if (!rename_hash) return origname;
  if (!destructor) {
    rn = Swig_name_object_get(rename_hash, Classprefix, name, decl);
  } else {
    rn = Swig_name_object_get(rename_hash, Classprefix, Char(name)+1,decl);
  }
  if (!rn) return name;
  if (destructor) {
    String *s = NewStringf("~%s", rn);
    return s;
  }
  return Copy(rn);
}

/* Generate an unnamed identifier */
static String *make_unnamed() {
  unnamed++;
  return NewStringf("$unnamed%d$",unnamed);
}

/* Generate the symbol table name for an object */
static String *name_warning(String *name,SwigType *decl) {
  String *rn = 0;
  if (!name) return 0;

  /* Check to see if the name is in the hash */
  if (!namewarn_hash) return 0;
  rn = Swig_name_object_get(namewarn_hash, Classprefix,name,decl);
  if (!rn) return 0;
  return rn;
}

/* Add declaration list to symbol table */
static void add_symbols(Node *n) {
  String *decl;
  char *wrn = 0;
  /* Don't add symbols for private/protected members */
  if (inclass && (cplus_mode != CPLUS_PUBLIC)) return;
  while (n) {
    String *symname;
    if (Getattr(n,"sym:name")) {
      n = nextSibling(n);
      continue;
    }
    decl = Getattr(n,"decl");
    if (!SwigType_isfunction(decl)) {
      symname = make_name(Getattr(n,"name"),0);
      wrn = name_warning(symname,0);
      Swig_features_get(features_hash, Classprefix, Getattr(n,"name"), 0, n);
    } else {
      SwigType *fdecl = Copy(decl);
      SwigType *fun = SwigType_pop_function(fdecl);
      symname = make_name(Getattr(n,"name"),fun);
      wrn = name_warning(symname,fun);
      Swig_features_get(features_hash,Classprefix,Getattr(n,"name"),fun,n);
      Delete(fdecl);
      Delete(fun);
    }
    if (!symname) {
      n = nextSibling(n);
      continue;
    }
    if (strncmp(Char(symname),"$ignore",7) == 0) {
      char *c = Char(symname)+7;
      Setattr(n,"error",NewString("ignored"));
      if (strlen(c)) {
	Printf(stderr,"%s:%d. Warning. %s\n", Getfile(n), Getline(n), c+1);
      }
    } else {
      Node *c;
      if ((wrn) && (strlen(wrn))) {
	Printf(stderr,"%s:%d. NameWarning. %s\n", Getfile(n), Getline(n), wrn);
      }
      c = Swig_symbol_add(symname,n);
      if (c != n) {
	String *e = NewString("");
	Printf(e,"%s:%d. Identifier '%s' redeclared (ignored).", Getfile(n),Getline(n),symname);
	if (Cmp(symname,Getattr(n,"name"))) {
	  Printf(e," (Renamed from '%s')", Getattr(n,"name"));
	}
	Printf(e,"\n%s:%d. Previous declaration of '%s'", Getfile(c),Getline(c),symname);
	if (Cmp(symname,Getattr(c,"name"))) {
	  Printf(e," (Renamed from '%s')", Getattr(c,"name"));
	}
	Printf(stderr,"%s\n",e);
	Setattr(n,"error",e);
      }
    }
    n = nextSibling(n);
  }
}

/* Add a declaration to the tag-space */
static void add_tag(Node *n) {
  String *symname = make_name(Getattr(n,"name"),0);
  Node *c;
  if (!symname) {
    symname = Getattr(n,"unnamed");
  }
  if (!symname) return;
  if (Cmp(symname,"$ignore") == 0) {
    Setattr(n,"error",NewString("ignored"));
    return;
  }
  c = Swig_symbol_add_tag(symname, n);
  if (c != n) {
    String *e = NewString("");
    Printf(e,"%s:%d. '%s' redeclared (ignored).\n", Getfile(n),Getline(n),symname);
    Printf(e,"%s:%d. Previous definition of tag '%s'\n", Getfile(c),Getline(c), symname);
    Printf(stderr,"%s",e);
    Setattr(n,"error",e);
  }
  Swig_features_get(features_hash, 0, Getattr(n,"name"), 0, n);
}

/* Addmethods merge.  This function is used to handle the %addmethods directive
   when it appears before a class definition.   To handle this, the %addmethods
   actually needs to take precedence.  Therefore, we will selectively nuke symbols
   from the current symbol table, replacing them with the added methods */

static void merge_addmethods(Node *am) {
  Node *n;
  Node *csym;

  n = firstChild(am);
  while (n) {
    String *symname;
    symname = Getattr(n,"sym:name");
    DohIncref(symname);
    if ((symname) && (!Getattr(n,"error"))) {
      /* Remove node from its symbol table */
      Swig_symbol_remove(n);
      csym = Swig_symbol_add(symname,n);
      if (csym != n) {
	/* Conflict with previous definition.  Nuke previous definition */
	String *e = NewString("");
	Printf(e,"%s:%d. '%s' redeclared (ignored).\n", Getfile(csym),Getline(csym),symname);
	Printf(e,"%s:%d. Previous definition of tag '%s'\n", Getfile(n),Getline(n), symname);
	Printf(stderr,"%s",e);
	Setattr(csym,"error",e);
	Swig_symbol_remove(csym);              /* Remove class definition */
	Swig_symbol_add(symname,n);            /* Insert addmethods definition */
      }
    }
    n = nextSibling(n);
  }
}

 static void check_addmethods() {
   String *key;
   if (!addmethods) return;
   for (key = Firstkey(addmethods); key; key = Nextkey(addmethods)) {
     Node *n = Getattr(addmethods,key);
     Printf(stderr,"%s:%d. %%addmethods defined for an undeclared class %s.\n",
	    Getfile(n),Getline(n),key);
   }
 }

/* Check a set of declarations to see if any are pure-abstract */

 static int pure_abstract(Node *n) {
   while (n) {
     if (Cmp(nodeType(n),"cdecl") == 0) {
       String *decl = Getattr(n,"decl");
       if (SwigType_isfunction(decl)) {
	 String *init = Getattr(n,"value");
	 if (Cmp(init,"0") == 0) {
	   return 1;
	 }
       }
     } else if (Cmp(nodeType(n),"destructor") == 0) {
       if (Cmp(Getattr(n,"value"),"0") == 0) return 1;
     }
     n = nextSibling(n);
   }
   return 0;
 }
/* Structures for handling code fragments built for nested classes */

typedef struct Nested {
  String   *code;        /* Associated code fragment */
  int      line;         /* line number where it starts */
  char     *name;        /* Name associated with this nested class */
  char     *kind;        /* Kind of class */
  SwigType *type;        /* Datatype associated with the name */
  struct Nested   *next;        /* Next code fragment in list */
} Nested;

/* Some internal variables for saving nested class information */

static Nested      *nested_list = 0;

/* Add a function to the nested list */

static void add_nested(Nested *n) {
  Nested *n1;
  if (!nested_list) nested_list = n;
  else {
    n1 = nested_list;
    while (n1->next) n1 = n1->next;
    n1->next = n;
  }
}

/* Dump all of the nested class declarations to the inline processor
 * However.  We need to do a few name replacements and other munging
 * first.  This function must be called before closing a class! */

static Node *dump_nested(char *parent) {
  Nested *n,*n1;
  Node *ret = 0;
  n = nested_list;
  if (!parent) {
    nested_list = 0;
    return 0;
  }
  while (n) {
    char temp[256];
    Node *retx;
    /* Token replace the name of the parent class */
    Replace(n->code, "$classname", parent, DOH_REPLACE_ANY);
    /* Fix up the name of the datatype (for building typedefs and other stuff) */
    sprintf(temp,"%s_%s", parent,n->name);

    Append(n->type,parent);
    Append(n->type,"_");
    Append(n->type,n->name);

    /* Add the appropriate declaration to the C++ processor */
    retx = new_node("cdecl");
    Setattr(retx,"name",n->name);
    Setattr(retx,"type",Copy(n->type));
    Setattr(retx,"nested",parent);
    /*    Printf(stdout,"%s   %s\n", n->name, yyrename);*/
    add_symbols(retx);
    if (ret) {
      set_nextSibling(retx,ret);
    }
    ret = retx;

    /* Insert a forward class declaration */
    retx = new_node("classforward");
    Setattr(retx,"kind",n->kind);
    Setattr(retx,"name",Copy(n->type));
    Setattr(retx,"sym:name", make_name(n->type,0));
    set_nextSibling(retx,ret);
    ret = retx; 

    /* Make all SWIG created typedef structs/unions/classes unnamed else 
       redefinition errors occur - nasty hack alert.*/

    {
      char* types_array[3] = {"struct", "union", "class"};
      int i;
      for (i=0; i<3; i++) {
	char* code_ptr = Char(n->code);
      while (code_ptr) {
        /* Replace struct name (as in 'struct name {' ) with whitespace
           name will be between struct and { */
	
        code_ptr = strstr(code_ptr, types_array[i]);
        if (code_ptr) {
	  char *open_bracket_pos;
          code_ptr += strlen(types_array[i]);
          open_bracket_pos = strstr(code_ptr, "{");
          if (open_bracket_pos) { 
            /* Make sure we don't have something like struct A a; */
            char* semi_colon_pos = strstr(code_ptr, ";");
            if (!(semi_colon_pos && (semi_colon_pos < open_bracket_pos)))
              while (code_ptr < open_bracket_pos)
                *code_ptr++ = ' ';
          }
        }
      }
      }
    }
    
    {
      /* Remove SWIG directive %constant which may be left in the SWIG created typedefs */
      char* code_ptr = Char(n->code);
      while (code_ptr) {
	code_ptr = strstr(code_ptr, "%constant");
	if (code_ptr) {
	  char* directive_end_pos = strstr(code_ptr, ";");
	  if (directive_end_pos) { 
            while (code_ptr <= directive_end_pos)
              *code_ptr++ = ' ';
	  }
	}
      }
    }
    {
      Node *head;
      head = new_node("insert");
      Setattr(head,"code",NewStringf("\n%s\n",n->code));
      set_nextSibling(head,ret);
      ret = head;
    }
      
    /* Dump the code to the scanner */
    start_inline(Char(n->code),n->line);

    n1 = n->next;
    Delete(n->code);
    free(n);
    n = n1;
  }
  nested_list = 0;
  return ret;
}

Node *Swig_cparse(File *f) {
  extern void scanner_file(File *);
  extern int yyparse();
  scanner_file(f);
  top = 0;
  yyparse();
  return top;
}

static void canonical_template(String *s) {
  Replaceall(s,"\n"," ");
  Replaceall(s,"\t"," ");
  Replaceall(s,"  "," ");
  /* Canonicalize whitespace around angle brackets and commas */
  while (Replaceall(s, "< ", "<"));
  while (Replaceall(s, " >", ">"));
  while (Replaceall(s, " ,", ","));
  while (Replaceall(s, ", ", ","));
  /* Canonicalize whitespace around pointers and references */
  while (Replaceall(s,"* ", "*"));
  while (Replaceall(s," *", "*"));
  while (Replaceall(s,"& ", "&"));
  while (Replaceall(s," &", "&"));
  /* Canonicalize whitespace around array brackets and parentheses */
  while (Replaceall(s,"[ ", "["));
  while (Replaceall(s," [", "["));
  while (Replaceall(s,"] ", "]"));
  while (Replaceall(s," ]", "]"));

  while (Replaceall(s,"( ", "("));
  while (Replaceall(s," (", "("));
  while (Replaceall(s,") ", ")"));
  while (Replaceall(s," )", ")"));

  /* Patch up for nested templates */

  Replace(s,">"," >", DOH_REPLACE_ANY);
}

static void patch_template_code(String *s) {
  if (templatename) {
    Replace(s, templateiname, templatename, DOH_REPLACE_ID);
  }
}

static void patch_template_type(String *s) {
  if (templatetypes) {
    if (Strstr(s,"__swig")) {
      String *key;
      for (key = Firstkey(templatetypes); key; key = Nextkey(templatetypes)) {
	Replace(s,key,Getattr(templatetypes,key), DOH_REPLACE_ID);
      }
    }
    Replace(s,templateiname,templatename, DOH_REPLACE_ID);
  }
}

%}

%union {
  char  *id;
  List  *bases;
  struct Define {
    String *val;
    String *rawval;
    int     type;
    String *qualifier;
  } dtype;
  struct {
    char *type;
    char *filename;
    int   line;
  } loc;
  struct {
    char      *id;
    SwigType  *type;
    String    *defarg;
    ParmList  *parms;
    short      have_parms;
  } decl;
  struct {
    String     *rparms;
    String     *sparms;
  } tmplstr;
  struct {
    String     *op;
    Hash       *kwargs;
  } tmap;
  struct {
    String     *type;
    String     *us;
  } ptype;
  SwigType     *type;
  String       *str;
  Parm         *p;
  ParmList     *pl;
  int           ivalue;
  Node         *node;
};

%token <id> ID
%token <str> HBLOCK
%token <id> POUND 
%token <id> STRING
%token <loc> INCLUDE IMPORT INSERT
%token <str> CHARCONST 
%token <dtype> NUM_INT NUM_FLOAT NUM_UNSIGNED NUM_LONG NUM_ULONG NUM_LONGLONG NUM_ULONGLONG
%token <ivalue> TYPEDEF
%token <type> TYPE_INT TYPE_UNSIGNED TYPE_SHORT TYPE_LONG TYPE_FLOAT TYPE_DOUBLE TYPE_CHAR TYPE_VOID TYPE_SIGNED TYPE_BOOL TYPE_TYPEDEF TYPE_RAW
%token LPAREN RPAREN COMMA SEMI EXTERN INIT LBRACE RBRACE PERIOD
%token CONST VOLATILE STRUCT UNION EQUAL SIZEOF MODULE LBRACKET RBRACKET
%token ILLEGAL CONSTANT
%token NAME RENAME NAMEWARN ADDMETHODS PRAGMA FEATURE VARARGS
%token ENUM
%token CLASS TYPENAME PRIVATE PUBLIC PROTECTED COLON STATIC VIRTUAL FRIEND THROW
%token NATIVE INLINE
%token TYPEMAP EXCEPT ECHO NEW APPLY CLEAR SWIGTEMPLATE ENDTEMPLATE STARTTEMPLATE GENCODE
%token LESSTHAN GREATERTHAN MODULO NEW DELETE
%token TYPES PARMS
%token NONID DSTAR DCNOT
%token <ivalue> TEMPLATE
%token <str> OPERATOR
%token <str> COPERATOR

%left  LOR
%left  LAND
%left  OR
%left  XOR
%left  AND
%left  LSHIFT RSHIFT
%left  PLUS MINUS
%left  STAR SLASH
%left  UMINUS NOT LNOT
%left  DCOLON

%type <node>     program interface declaration swig_directive ;

/* SWIG directives */
%type <node>     addmethods_directive apply_directive clear_directive constant_directive ;
%type <node>     echo_directive except_directive include_directive inline_directive ;
%type <node>     insert_directive gencode_directive module_directive name_directive native_directive ;
%type <node>     new_directive pragma_directive rename_directive feature_directive varargs_directive typemap_directive ;
%type <node>     types_directive template_directive endtemplate_directive starttemplate_directive ;

/* C declarations */
%type <node>     c_declaration c_decl c_decl_tail c_enum_decl;
%type <node>     enumlist edecl;

/* C++ declarations */
%type <node>     cpp_declaration cpp_class_decl cpp_forward_class_decl cpp_template_decl;
%type <node>     cpp_members cpp_member;
%type <node>     cpp_constructor_decl cpp_destructor_decl cpp_protection_decl cpp_conversion_operator;
%type <node>     cpp_swig_directive cpp_template_decl cpp_nested cpp_opt_declarators ;
%type <node>     kwargs;

/* Misc */
%type <dtype>    initializer;
%type <id>       storage_class;
%type <pl>       parms  ptail rawparms varargs_parms ;
%type <p>        parm;
%type <p>        typemap_parm tm_list tm_tail;
%type <id>       cpptype access_specifier;
%type <node>     base_specifier
%type <type>     type rawtype type_right cast_type cast_type_right;
%type <bases>    base_list inherit raw_inherit;
%type <dtype>    definetype def_args etype;
%type <dtype>    expr;
%type <id>       ename ;
%type <id>       template_decl;
%type <str>      type_qualifier cpp_const ;
%type <id>       type_qualifier_raw;
%type <id>       idstring;
%type <id>       pragma_lang;
%type <str>      pragma_arg;
%type <loc>      includetype;
%type <type>     pointer primitive_type;
%type <decl>     declarator direct_declarator parameter_declarator typemap_parameter_declarator nested_decl;
%type <decl>     abstract_declarator direct_abstract_declarator;
%type <tmap>     typemap_type;
%type <str>      idcolon idcolontail idtemplate stringbrace stringbracesemi;
%type <id>       string;
%type <tmplstr>  template_parms;
%type <ivalue>   cpp_vend;
%type <ivalue>   rename_namewarn;
%type <ptype>    type_specifier primitive_type_list ;

%%

/* ======================================================================
 *                          High-level Interface file
 *
 * An interface is just a sequence of declarations which may be SWIG directives
 * or normal C declarations.
 * ====================================================================== */

program        :  interface {
		   Setattr($1,"classes",classes);
		   Setattr($1,"name",ModuleName);
		   check_addmethods();
	           top = $1;
               }
               ;

interface      : interface declaration {  
                   appendChild($1,$2);
                   Error = 0; 
                   $$ = $1;
               }
               | empty {
                   $$ = new_node("top");
               }
               ;

declaration    : swig_directive { $$ = $1; }
               | c_declaration { $$ = $1; } 
               | cpp_declaration { $$ = $1; }
               | SEMI { $$ = 0; }
               | error {
                  $$ = 0;
		 if (!Error) {
		   static int last_error_line = -1;
		   if (last_error_line != line_number) {
		     cparse_error(input_file, line_number,"Syntax error in input.\n");
		     last_error_line = line_number;
		     skip_decl();
		   }
		   Error = 1;
		 }
               }
/* Out of class constructor/destructor declarations */
               | c_constructor_decl { $$ = 0; }
               | c_destructor_decl { $$ = 0; }
               ;


/* ======================================================================
 *                           SWIG DIRECTIVES 
 * ====================================================================== */
  
swig_directive : addmethods_directive { $$ = $1; }
               | apply_directive { $$ = $1; }
 	       | clear_directive { $$ = $1; }
               | constant_directive { $$ = $1; }
               | echo_directive { $$ = $1; }
               | except_directive { $$ = $1; }
               | include_directive { $$ = $1; }
               | inline_directive { $$ = $1; }
               | insert_directive { $$ = $1; }
               | gencode_directive { $$ = $1; }
               | module_directive { $$ = $1; }
               | name_directive { $$ = $1; }
               | native_directive { $$ = $1; }
               | new_directive { $$ = $1; }
               | pragma_directive { $$ = $1; }
               | rename_directive { $$ = $1; }
               | feature_directive { $$ = $1; }
               | varargs_directive { $$ = $1; }
               | typemap_directive { $$ = $1; }
               | types_directive  { $$ = $1; }
               | template_directive { $$ = $1; }
               | endtemplate_directive { $$ = $1; }
               | starttemplate_directive { $$ = $1; }
               ;

/* ------------------------------------------------------------
   %addmethods classname { ... } 
   ------------------------------------------------------------ */

addmethods_directive : ADDMETHODS ID LBRACE {
               Node *cls;
	       cplus_mode = CPLUS_PUBLIC;
	       if (!classes) classes = NewHash();
	       if (!addmethods) addmethods = NewHash();
	       cls = Getattr(classes,$2);
	       if (!cls) {
		 /* No previous definition. Create a new scope */
		 Node *am = Getattr(addmethods,$2);
		 if (!am) {
		   Swig_symbol_newscope();
		   prev_symtab = 0;
		 } else {
		   prev_symtab = Swig_symbol_setscope(Getattr(am,"symtab"));
		 }
		 current_class = 0;
	       } else {
		 /* Previous class definition.  Use its symbol table */
		 prev_symtab = Swig_symbol_setscope(Getattr(cls,"symtab"));
		 current_class = cls;
	       }
	       Classprefix = NewString($2);
	     } cpp_members RBRACE {
               $$ = new_node("addmethods");
	       Setattr($$,"name",$2);
	       Setattr($$,"symtab",Swig_symbol_popscope());
	       if (current_class) {
		 /* We add the addmethods to the previously defined class */
		 appendChild($$,$5);
		 appendChild(current_class,$$);
	       } else {
		 /* We store the addmethods in the addmethods hash */
		 Node *am = Getattr(addmethods,$2);
		 if (am) {
		   /* Append the members to the previous add methods */
		   appendChild(am,$5);
		 } else {
		   appendChild($$,$5);
		   Setattr(addmethods,$2,$$);
		 }
	       }
	       if (prev_symtab) {
		 Swig_symbol_setscope(prev_symtab);
	       }
	       current_class = 0;
	       Delete(Classprefix);
	       Classprefix = 0;
	       prev_symtab = 0;
	       $$ = 0;
	     }
             ;

/* ------------------------------------------------------------
   %apply
   ------------------------------------------------------------ */

apply_directive : APPLY typemap_parm LBRACE tm_list RBRACE {
                    $$ = new_node("apply");
                    Setattr($$,"pattern",Getattr($2,"pattern"));
		    appendChild($$,$4);
               };

/* ------------------------------------------------------------
   %clear
   ------------------------------------------------------------ */

clear_directive : CLEAR tm_list SEMI {
		 $$ = new_node("clear");
		 appendChild($$,$2);
               }
               ;

/* ------------------------------------------------------------
   %constant name = value;
   %constant type name = value;
   ------------------------------------------------------------ */

constant_directive :  CONSTANT ID EQUAL definetype SEMI {
		   if (($4.type != T_ERROR) && ($4.type != T_SYMBOL)) {
		     $$ = new_node("constant");
		     Setattr($$,"name",$2);
		     Setattr($$,"type",NewSwigType($4.type));
		     Setattr($$,"value",$4.val);
		     Setattr($$,"storage","%constant");
		     add_symbols($$);
		   } else {
		     if ($4.type == T_ERROR) {
		       cparse_error(input_file,line_number,"Unsupported constant value\n");
		     }
		     $$ = 0;
		   }

	       }

               | CONSTANT type declarator def_args SEMI {
		 if (($4.type != T_ERROR) && ($4.type != T_SYMBOL)) {
		   SwigType_push($2,$3.type);
		   /* Sneaky callback function trick */
		   if (SwigType_isfunction($2)) {
		     SwigType_add_pointer($2);
		   }
		   $$ = new_node("constant");
		   Setattr($$,"name",$3.id);
		   Setattr($$,"type",$2);
		   Setattr($$,"value",$4.val);
		   Setattr($$,"storage","%constant");
		   add_symbols($$);
		 } else {
		     if ($4.type == T_ERROR) {
		       cparse_error(input_file,line_number,"Unsupported constant value\n");
		     }
		   $$ = 0;
		 }
               }
               | CONSTANT error SEMI {
		 cparse_error(input_file,line_number,"Warning. Bad constant value (ignored).\n");
		 $$ = 0;
	       }
               ;

/* ------------------------------------------------------------
   %echo "text"
   %echo %{ ... %}
   ------------------------------------------------------------ */

echo_directive : ECHO HBLOCK {
		 char temp[64];
		 Replace($2,"$file",input_file, DOH_REPLACE_ANY);
		 sprintf(temp,"%d", line_number);
		 Replace($2,"$line",temp,DOH_REPLACE_ANY);
		 Printf(stderr,"%s\n", $2);
		 Delete($2);
                 $$ = 0;
	       }
               | ECHO string {
		 char temp[64];
		 String *s = NewString($2);
		 Replace(s,"$file",input_file, DOH_REPLACE_ANY);
		 sprintf(temp,"%d", line_number);
		 Replace(s,"$line",temp,DOH_REPLACE_ANY);
		 Printf(stderr,"%s\n", s);
		 Delete(s);
                 $$ = 0;
               }
               ;

/* ------------------------------------------------------------
   %except(lang) { ... }
   %except { ... }
   %except(lang);   
   %except;
   ------------------------------------------------------------ */

except_directive : EXCEPT LPAREN ID RPAREN LBRACE {
                    skip_balanced('{','}');
		    if (strcmp($3,typemap_lang) == 0) {
		      $$ = new_node("except");
		      Setattr($$,"code",Copy(scanner_ccode));
		    } else {
		      $$ = 0;
		    }
		    free($3);
	       }

               | EXCEPT LBRACE {
                    skip_balanced('{','}');
		    $$ = new_node("except");
		    Setattr($$,"code",Copy(scanner_ccode));
               }

               | EXCEPT LPAREN ID RPAREN SEMI {
		 $$ = new_node("except");
               }

               | EXCEPT SEMI {
		 $$ = new_node("except");
	       }
               ;

/* ------------------------------------------------------------
   %includefile "filename" { declarations } 
   %importfile  "filename" { declarations }
   ------------------------------------------------------------ */

include_directive: includetype string LBRACE {
                     $1.filename = Swig_copy_string(input_file);
		     $1.line = line_number;
		     input_file = Swig_copy_string($2);
		     line_number = 0;
               } interface RBRACE {
		     input_file = $1.filename;
		     line_number = $1.line;
		     if (strcmp($1.type,"include") == 0) $$ = new_node("include");
		     if (strcmp($1.type,"import") == 0) $$ = new_node("import");
		     Setattr($$,"name",$2);
		     appendChild($$,firstChild($5));
               }
               ;

includetype    : INCLUDE { $$.type = (char *) "include"; }
               | IMPORT  { $$.type = (char *) "import"; }
               ;

/* ------------------------------------------------------------
   %inline %{ ... %}
   ------------------------------------------------------------ */

inline_directive : INLINE HBLOCK {
                 String *cpps;
                 $$ = new_node("insert");
		 Setattr($$,"code",$2);
		 /* Need to run through the preprocessor */
		 Setline($2,start_line);
		 Setfile($2,input_file);
		 Seek($2,0,SEEK_SET);
		 cpps = Preprocessor_parse($2);
		 start_inline(Char(cpps), start_line);
		 Delete($2);
		 Delete(cpps);
	       }
               ;

/* ------------------------------------------------------------
   %{ ... %}
   %insert(section) "filename"
   %insert("section") "filename"
   %insert(section) %{ ... %}
   %insert("section") %{ ... %}
   ------------------------------------------------------------ */

insert_directive : HBLOCK {
                 $$ = new_node("insert");
		 Setattr($$,"code",$1);
	       }
               | INSERT LPAREN idstring RPAREN string {
		 String *code = NewString("");
		 $$ = new_node("insert");
		 Setattr($$,"section",$3);
		 Setattr($$,"code",code);
		 if (Swig_insert_file($5,code) < 0) {
		   cparse_error(input_file, line_number, "Couldn't find '%s'. Possible installation problem.\n", $5);
		   $$ = 0;
		 } 
               }
               | INSERT LPAREN idstring RPAREN HBLOCK {
		 $$ = new_node("insert");
		 Setattr($$,"section",$3);
		 Setattr($$,"code",$5);
               }
               | INSERT LPAREN idstring RPAREN LBRACE {
                 skip_balanced('{','}');
		 $$ = new_node("insert");
		 Setattr($$,"section",$3);
		 Delitem(scanner_ccode,0);
		 Delitem(scanner_ccode,DOH_END);
		 Setattr($$,"code", Copy(scanner_ccode));
	       }
               ;

/* ------------------------------------------------------------
 * %gencode %{ ... %}
 * ------------------------------------------------------------ */

gencode_directive : GENCODE HBLOCK {
                   $$ = new_node("insert");
                   Setattr($$,"code",$2);
                   Setattr($$,"generated","1");
                }
                ;
           


      
/* ------------------------------------------------------------
    %module modname
    %module "modname"
   ------------------------------------------------------------ */

module_directive: MODULE idstring {
                 $$ = new_node("module");
		 Setattr($$,"name",$2);
		 if (!ModuleName) ModuleName = NewString($2);
	       }
               ;

/* ------------------------------------------------------------
   %name(newname)    declaration
   %name("newname")  declaration
   ------------------------------------------------------------ */

name_directive : NAME LPAREN idstring RPAREN {
                 yyrename = NewString($3);
		 $$ = 0;
               }
               | NAME LPAREN RPAREN {
                   $$ = 0;
		   cparse_error(input_file,line_number,"Missing argument to %%name directive.\n");
	       }
               ;


/* ------------------------------------------------------------
   %native(scriptname) name;
   %native(scriptname) type name (parms);
   ------------------------------------------------------------ */

native_directive : NATIVE LPAREN ID RPAREN storage_class ID SEMI {
                 $$ = new_node("native");
		 Setattr($$,"name",$3);
		 Setattr($$,"wrap:name",$6);
	         add_symbols($$);
	       }
               | NATIVE LPAREN ID RPAREN storage_class type declarator SEMI {
		 if (!SwigType_isfunction($7.type)) {
		   Printf(stderr,"%s:%d. %%native declaration '%s' is not a function.\n", input_file,line_number, $7.id);
		   $$ = 0;
		 } else {
		     Delete(SwigType_pop_function($7.type));
		     /* Need check for function here */
		     SwigType_push($6,$7.type);
		     $$ = new_node("native");
	             Setattr($$,"name",$3);
		     Setattr($$,"wrap:name",$7.id);
		     Setattr($$,"type",$6);
		     Setattr($$,"parms",$7.parms);
		     Setattr($$,"decl",$7.type);
		 }
	         add_symbols($$);
	       }
               ;


/* ------------------------------------------------------------ 
   %new declaration
   ------------------------------------------------------------ */

new_directive : NEW c_declaration {
                 $$ = new_node("new");
		 appendChild($$,$2);
               }
               ;


/* ------------------------------------------------------------
   %pragma(lang) name=value
   %pragma(lang) name
   %pragma name = value
   %pragma name
   ------------------------------------------------------------ */

pragma_directive : PRAGMA pragma_lang ID EQUAL pragma_arg {
                 $$ = new_node("pragma");
		 Setattr($$,"lang",$2);
		 Setattr($$,"name",$3);
		 Setattr($$,"value",$5);
	       }
              | PRAGMA pragma_lang ID {
		$$ = new_node("pragma");
		Setattr($$,"lang",$2);
		Setattr($$,"name",$3);
	      }
              ;

pragma_arg    : string { $$ = NewString($1); }
              | HBLOCK { $$ = $1; }
              ;

pragma_lang   : LPAREN ID RPAREN { $$ = $2; }
              | empty { $$ = (char *) "swig"; }
              ;

/* ------------------------------------------------------------
   %rename identifier newname;
   %rename identifier "newname";
   ------------------------------------------------------------ */

rename_directive : rename_namewarn declarator idstring SEMI {
                    SwigType *t = $2.type;
		    if (!Len(t)) t = 0;
		    if ($1) {
		      rename_add($2.id,t,$3);
		    } else {
		      namewarn_add($2.id,t,$3);
		    }
		    $$ = 0;
              }
              | rename_namewarn LPAREN idstring RPAREN declarator cpp_const SEMI {
		SwigType *t = $5.type;
		if (!Len(t)) t = 0;
		/* Special declarator check */
		if (t) {
		  if ($6) SwigType_push(t,$6);
		  if (SwigType_isfunction(t)) {
		    SwigType *decl = SwigType_pop_function(t);
		    if (SwigType_ispointer(t)) {
		      String *nname = NewStringf("*%s",$5.id);
		      if ($1) {
			rename_add(Char(nname),decl,$3);
		      } else {
			namewarn_add(Char(nname),decl,$3);
		      }
		      Delete(nname);
		    } else {
		      if ($1) {
			rename_add($5.id,decl,$3);
		      } else {
			namewarn_add($5.id,decl,$3);
		      }
		    }
		  } else if (SwigType_ispointer(t)) {
		    String *nname = NewStringf("*%s",$5.id);
		    if ($1) {
		      rename_add(Char(nname),0,$3);
		    } else {
		      namewarn_add(Char(nname),0,$3);
		    }
		    Delete(nname);
		  }
		} else {
		  if ($1) {
		    rename_add($5.id,0,$3);
		  } else {
		    namewarn_add($5.id,0,$3);
		  }
		}
                $$ = 0;
              }
              | rename_namewarn LPAREN idstring RPAREN string SEMI {
		if ($1) {
		  rename_add($5,0,$3);
		} else {
		  namewarn_add($5,0,$3);
		}
		$$ = 0;
              }
              ;

rename_namewarn : RENAME {
		    $$ = 1;
                } 
                | NAMEWARN {
                    $$ = 0;
                };


/* ------------------------------------------------------------
   %feature(featurename) name { val }
   %feature(featurename) name "val";
   %feature(featurename) name %{ val % }
   %feature(featurename,val) name;
   ------------------------------------------------------------ */

              
feature_directive :  FEATURE LPAREN idstring RPAREN declarator cpp_const stringbracesemi {
                 String *fname;
                 Hash *n;
                 String *val;
		 String *name;
		 SwigType *t;
                 if (!features_hash) features_hash = NewHash();
		 fname = NewStringf("feature:%s",$3);
		 if (Classprefix) name = NewStringf("%s::%s", $5.id);
		 else name = NewString($5.id);
		 val = $7 ? Copy($7) : 0;
		 if ($5.parms) {
		   Setmeta(val,"parms",$5.parms);
		 }
		 t = $5.type;
		 if ($5.parms) Setmeta(val,"parms",$5.parms);
		 if (!Len(t)) t = 0;
		 if (t) {
		   if ($6) SwigType_push(t,$6);
		   if (SwigType_isfunction(t)) {
		     SwigType *decl = SwigType_pop_function(t);
		     if (SwigType_ispointer(t)) {
		       String *nname = NewStringf("*%s",name);
		       Swig_feature_set(features_hash, nname, decl, fname, val);
		       Delete(nname);
		     } else {
		       Swig_feature_set(features_hash, name, decl, fname, val);
		     }
		   } else if (SwigType_ispointer(t)) {
		     String *nname = NewStringf("*%s",name);
		     Swig_feature_set(features_hash,nname,0,fname,val);
		     Delete(nname);
		   }
		 } else {
		   Swig_feature_set(features_hash,name,0,fname,val);
		 }
		 Delete(fname);
		 Delete(name);
		 $$ = 0;
              }

              /* Special form where value is included in (...) part */

              |  FEATURE LPAREN idstring COMMA idstring RPAREN declarator cpp_const SEMI {
                 String *fname;
                 Hash *n;
                 String *val;
		 String *name;
		 SwigType *t;
                 if (!features_hash) features_hash = NewHash();
		 fname = NewStringf("feature:%s",$3);
		 if (Classprefix) name = NewStringf("%s::%s", $7.id);
		 else name = NewString($7.id);
		 val = NewString($5);
		 if ($7.parms) {
		   Setmeta(val,"parms",$7.parms);
		 }
		 t = $7.type;
		 if ($7.parms) Setmeta(val,"parms",$7.parms);
		 if (!Len(t)) t = 0;
		 if (t) {
		   if ($8) SwigType_push(t,$8);
		   if (SwigType_isfunction(t)) {
		     SwigType *decl = SwigType_pop_function(t);
		     if (SwigType_ispointer(t)) {
		       String *nname = NewStringf("*%s",name);
		       Swig_feature_set(features_hash, nname, decl, fname, val);
		       Delete(nname);
		     } else {
		       Swig_feature_set(features_hash, name, decl, fname, val);
		     }
		   } else if (SwigType_ispointer(t)) {
		     String *nname = NewStringf("*%s",name);
		     Swig_feature_set(features_hash,nname,0,fname,val);
		     Delete(nname);
		   }
		 } else {
		   Swig_feature_set(features_hash,name,0,fname,val);
		 }
		 Delete(fname);
		 Delete(name);
		 $$ = 0;
              }

              /* Global feature */

              | FEATURE LPAREN idstring RPAREN stringbracesemi {
		String *name;
		String *fname = NewStringf("feature:%s",$3);
		if (!features_hash) features_hash = NewHash();
		if (Classprefix) name = NewStringf("%s::", Classprefix);
		else name = NewString("");
		Swig_feature_set(features_hash,name,0,fname,($5 ? NewString($5) : 0));
		Delete(name);
		Delete(fname);
		$$ = 0;
              }
              ;

stringbracesemi : stringbrace { $$ = $1; }
                | SEMI { $$ = 0; }
                | PARMS LPAREN parms RPAREN SEMI { $$ = $3; } 
                ;

/* %varargs() directive. */

varargs_directive : VARARGS LPAREN varargs_parms RPAREN declarator cpp_const SEMI {
                 Hash *n;
                 Parm *val;
		 String *name;
		 SwigType *t;
                 if (!features_hash) features_hash = NewHash();
		 if (Classprefix) name = NewStringf("%s::%s", $5.id);
		 else name = NewString($5.id);
		 val = $3;
		 if ($5.parms) {
		   Setmeta(val,"parms",$5.parms);
		 }
		 t = $5.type;
		 if (!Len(t)) t = 0;
		 if (t) {
		   if ($6) SwigType_push(t,$6);
		   if (SwigType_isfunction(t)) {
		     SwigType *decl = SwigType_pop_function(t);
		     if (SwigType_ispointer(t)) {
		       String *nname = NewStringf("*%s",name);
		       Swig_feature_set(features_hash, nname, decl, "feature:varargs", val);
		       Delete(nname);
		     } else {
		       Swig_feature_set(features_hash, name, decl, "feature:varargs", val);
		     }
		   } else if (SwigType_ispointer(t)) {
		     String *nname = NewStringf("*%s",name);
		     Swig_feature_set(features_hash,nname,0,"feature:varargs",val);
		     Delete(nname);
		   }
		 } else {
		   Swig_feature_set(features_hash,name,0,"feature:varargs",val);
		 }
		 Delete(name);
		 $$ = 0;
              };

varargs_parms   : parms { $$ = $1; }
                | NUM_INT COMMA parm { 
		  int i;
		  int n;
		  Parm *p;
		  n = atoi(Char($1.val));
		  if (n <= 0) {
		    cparse_error(input_file, line_number,"Argument count must be positive.\n");
		    $$ = 0;
		  } else {
		    $$ = Copy($3);
		    Setattr($$,"name","VARARGS_SENTINEL");
		    for (i = 0; i < n; i++) {
		      p = Copy($3);
		      set_nextSibling(p,$$);
		      $$ = p;
		    }
		  }
                }
               ;


/* ------------------------------------------------------------
   %typemap(method) type { ... }
   %typemap(method) type "..."
   %typemap(method) type;    - typemap deletion
   %typemap(method) type1,type2,... = type;    - typemap copy
   ------------------------------------------------------------ */

typemap_directive :  TYPEMAP LPAREN typemap_type RPAREN tm_list stringbrace {
		   Parm *p;
		   $$ = 0;
		   if ($3.op) {
		     $$ = new_node("typemap");
		     Setattr($$,"method",$3.op);
		     Setattr($$,"code",NewString($6));
		     if ($3.kwargs) {
		       Setattr($$,"kwargs", $3.kwargs);
		     }
		     appendChild($$,$5);
		   }
	       }
               | TYPEMAP LPAREN typemap_type RPAREN tm_list SEMI {
		 Parm *p;
		 $$ = 0;
		 if ($3.op) {
		   $$ = new_node("typemap");
		   Setattr($$,"method",$3.op);
		   appendChild($$,$5);
		 }
	       }
               | TYPEMAP LPAREN typemap_type RPAREN tm_list EQUAL typemap_parm SEMI {
                   Parm *p;
		   $$ = 0;
		   if ($3.op) {
		     $$ = new_node("typemapcopy");
		     Setattr($$,"method",$3.op);
		     Setattr($$,"pattern", Getattr($7,"pattern"));
		     appendChild($$,$5);
		   }
	       }
               ;

/* typemap method type (lang,method) or (method) */

typemap_type   : kwargs {
		 Hash *p;
		 String *name;
		 p = nextSibling($1);
		 if (p && (!Getattr(p,"value"))) {
		   /* two argument typemap form */
		   name = Getattr($1,"name");
		   if (!name || (Strcmp(name,typemap_lang))) {
		     $$.op = 0;
		     $$.kwargs = 0;
		   } else {
		     $$.op = Getattr(p,"name");
		     $$.kwargs = nextSibling(p);
		   }
		 } else {
		   /* one-argument typemap-form */
		   $$.op = Getattr($1,"name");
		   $$.kwargs = p;
		 }
                }
               ;

tm_list        : typemap_parm tm_tail {
                 $$ = $1;
		 set_nextSibling($$,$2);
		}
               ;

tm_tail        : COMMA typemap_parm tm_tail {
                 $$ = $2;
		 set_nextSibling($$,$3);
                }
               | empty { $$ = 0;}
               ;

typemap_parm   : type typemap_parameter_declarator {
		  SwigType_push($1,$2.type);
		  $$ = new_node("typemapitem");
		  Setattr($$,"pattern",NewParm($1,$2.id));
		  Setattr($$,"parms", $2.parms);
		  /*		  $$ = NewParm($1,$2.id);
				  Setattr($$,"parms",$2.parms); */
                }
               | LPAREN parms RPAREN {
                  $$ = new_node("typemapitem");
		  Setattr($$,"pattern",$2);
		  /*		  Setattr($$,"multitype",$2); */
               }
               | LPAREN parms RPAREN LPAREN parms RPAREN {
		 $$ = new_node("typemapitem");
		 Setattr($$,"pattern", $2);
		 /*                 Setattr($$,"multitype",$2); */
		 Setattr($$,"parms",$5);
               }
               ;

/* ------------------------------------------------------------
   %types(parmlist); 
   ------------------------------------------------------------ */

types_directive : TYPES LPAREN parms RPAREN SEMI {
                   $$ = new_node("types");
		   Setattr($$,"parms",$3);
               }
               ;

/* ------------------------------------------------------------
   %template(name) tname<args>;
   ------------------------------------------------------------ */

template_directive: SWIGTEMPLATE LPAREN idstring RPAREN ID LESSTHAN parms GREATERTHAN SEMI {
                  Parm *p;
		  String *ts;
		  String *args;
		  String *sargs;
		  String *tds;
		  String *cpps;

		  ts = NewString("%inline %{\n");
		  args = NewString("");
		  sargs = NewString("");
		  /* Create typedef's and arguments */
		  p = $7;
		  while (p) {
		    String *value = Getattr(p,"value");
		    if (value) {
		      Printf(args,"%s",value);
		      Printf(sargs,"%s",value);
		    } else {
		      SwigType *ty = Getattr(p,"type");
		      if (ty) {
			tds = NewStringf("__swigtmpl%d",templatenum);
			templatetypes = NewHash();
			Setattr(templatetypes,Copy(tds),Copy(ty));
			templatenum++;
			Printf(ts,"typedef %s;\n", SwigType_str(ty,tds));
			Printf(args,"%s",tds);
			Printf(sargs,"%s",SwigType_str(ty,0));
			Delete(tds);
		      }
		    }
		    p = nextSibling(p);
		    if (p) {
		      Printf(args,",");
		      Printf(sargs,",");
		    }
		  }
		  templateargs = NewStringf("%s<%s>", $5, sargs);
		  canonical_template(templateargs);
		  
		  if (!templatemaps) templatemaps = NewHash();
		  Setattr(templatemaps, templateargs, $3);

		  Printf(ts,"%%}\n");
		  Printf(ts,"%%starttemplate;\n");
                  Printf(ts,"%%_template_%s(%s,%s,%s)\n",$5,$3,args,sargs);
		  /*		  Printf(ts,"%%endtemplate;\n"); */
		  Delete(args);
		  Delete(sargs);
		  Setfile(ts,input_file);
		  Setline(ts,line_number);
		  Seek(ts,0,SEEK_SET);

		  /*		  Printf(stdout,"%%template:\n%s\n", ts); */
		  cpps = Preprocessor_parse(ts);

		  if (ShowTemplates) {
		    Printf(stderr,"%s:%d. %%template(%s) %s<%s> expanded to the following:\n", input_file, line_number, $3,$5,ParmList_protostr($7));
		    Printf(stderr,"\n%s\n",cpps);
		  }
		  if (cpps && (Len(cpps) > 0)) {
		    start_inline(Char(cpps),line_number);
		  } else {
		    Printf(stderr,"%s:%d. Unable to expand template %s\n", input_file, line_number, $5);
		  }
		  Delete(ts);
		  Delete(cpps);
		  templatename = NewString($5);
		  templateiname = NewString($3);
		  $$ = 0;
               }
               ;

/* -----------------------------------------------------------------------------
 * %starttemplate
 *
 * This directive appears at the beginning of template expansion.  It's needed
 * to prevent certain types of template substitutions.
 * ----------------------------------------------------------------------------- */

starttemplate_directive: STARTTEMPLATE  SEMI {
                   templatemode = 1;
                   $$ = 0;
                }
                ;

/* -----------------------------------------------------------------------------
 * %endtemplate
 *
 * This directive appears at the end of performing a template expansion.  It's
 * needed to reset some internal variables.
 * ----------------------------------------------------------------------------- */

endtemplate_directive: ENDTEMPLATE SEMI {
                    Delete(templatetypes);
                    templatetypes = 0;
		    Delete(templatename);
		    templatename = 0;
		    Delete(templateiname);
		    templateiname = 0;
		    Delete(templateargs);
		    templateargs = 0;
                    templatemode = 0;
                    $$ = 0;
                }
                ;

/* ======================================================================
 *                              C Parsing
 * ====================================================================== */

c_declaration   : c_decl { $$ = $1; }
                | c_enum_decl { $$ = $1; }

/* A an extern C type declaration.  Does nothing, but is ignored */

                | EXTERN string LBRACE interface RBRACE { 
                   $$ = new_node("extern");
		   Setattr($$,"name",$2);
		   appendChild($$,firstChild($4));
                }
                ;

/* ------------------------------------------------------------
   A C global declaration of some kind (may be variable, function, typedef, etc.)
   ------------------------------------------------------------ */

c_decl  : storage_class type declarator initializer c_decl_tail {
              $$ = new_node("cdecl");
	      if ($4.qualifier) SwigType_push($3.type,$4.qualifier);
	      Setattr($$,"type",$2);
	      Setattr($$,"storage",$1);
	      Setattr($$,"name",$3.id);
	      Setattr($$,"decl",$3.type);
	      Setattr($$,"parms",$3.parms);
	      Setattr($$,"value",$4.val);
	      if (!$5) {
		if (Len(scanner_ccode)) {
		  patch_template_code(scanner_ccode);
		  Setattr($$,"code",Copy(scanner_ccode));
		}
	      } else {
		Node *n = $5;
		/* Inherit attributes */
		while (n) {
		  Setattr(n,"type",Copy($2));
		  Setattr(n,"storage",$1);
		  n = nextSibling(n);
		}
	      }
	      /* Look for "::" declarations (ignored) */
	      if (Strstr($3.id,"::")) {
		Delete($$);
		$$ = $5;
	      } else {
		set_nextSibling($$,$5);
	      }
	      add_symbols($$);
           }
           ;

/* Allow lists of variables and functions to be built up */

c_decl_tail    : SEMI { 
                   $$ = 0;
                   Clear(scanner_ccode); 
               }
               | COMMA declarator initializer c_decl_tail {
		 $$ = new_node("cdecl");
		 if ($3.qualifier) SwigType_push($2.type,$3.qualifier);
		 Setattr($$,"name",$2.id);
		 Setattr($$,"decl",$2.type);
		 Setattr($$,"parms",$2.parms);
		 Setattr($$,"value",$3.val);
		 if (!$4) {
		   if (Len(scanner_ccode)) {
		     patch_template_code(scanner_ccode);
		     Setattr($$,"code",Copy(scanner_ccode));
		   }
		 } else {
		   set_nextSibling($$,$4);
		 }
	       }
               | LBRACE { 
                   skip_balanced('{','}');
                   $$ = 0;
               }
              ;

initializer   : def_args { 
                   $$ = $1; 
                   $$.qualifier = 0;
              }
              | type_qualifier def_args { 
                   $$ = $2; 
		   $$.qualifier = $1;
	      }
              | THROW LPAREN parms RPAREN def_args { 
		   $$ = $5; 
                   $$.qualifier = 0;
              }
              | type_qualifier THROW LPAREN parms RPAREN def_args { 
                   $$ = $6; 
                   $$.qualifier = $1;
              }
              ;


/* ------------------------------------------------------------
   enum { ... }
 * ------------------------------------------------------------ */

c_enum_decl : storage_class ENUM ename LBRACE enumlist RBRACE SEMI {
                  $$ = new_node("enum");
		  Setattr($$,"name",$3);
		  appendChild($$,$5);
		  add_tag($$);           /* Add to tag space */
		  add_symbols($5);       /* Add enum values to id space */
	       }

               | storage_class ENUM ename LBRACE enumlist RBRACE declarator c_decl_tail {
		 Node *n;
		 SwigType *ty;
		 String   *unnamed = 0;

		 $$ = new_node("enum");
		 if ($3) {
		   Setattr($$,"name",$3);
		   ty = NewStringf("enum %s", $3);
		 } else if ($7.id){
		   unnamed = make_unnamed();
		   ty = NewStringf("enum %s", unnamed);
		   Setattr($$,"unnamed",unnamed);
		   /* WF 20/12/2001: Cannot get sym:name and symtab set without setting name - fix!
		      // I don't think sym:name should be set. */
		   Setattr($$,"name",$7.id);
		   Setattr($$,"tdname",$7.id);
		   Setattr($$,"storage",$1);
		 }
		 appendChild($$,$5);
		 n = new_node("cdecl");
		 Setattr(n,"type",ty);
		 Setattr(n,"name",$7.id);
		 Setattr(n,"storage",$1);
		 Setattr(n,"decl",$7.type);
		 Setattr(n,"parms",$7.parms);
		 Setattr(n,"unnamed",unnamed);
		 if ($8) {
		   Node *p = $8;
		   set_nextSibling(n,p);
		   while (p) {
		     Setattr(p,"type",Copy(ty));
		     Setattr(p,"unnamed",unnamed);
		     Setattr(p,"storage",$1);
		     p = nextSibling(p);
		   }
		 } else {
		   if (Len(scanner_ccode)) {
		     patch_template_code(scanner_ccode);
		     Setattr(n,"code",Copy(scanner_ccode));
		   }
		 }
		 set_nextSibling($$,n);
		 add_tag($$);           /* Add enum to tag space */
		 add_symbols($5);       /* Add to id space */
	         add_symbols(n);
	       }
               ;

c_constructor_decl : storage_class type LPAREN parms RPAREN ctor_end {
                }
                ;

c_destructor_decl : storage_class type DCNOT ID LPAREN RPAREN cpp_end { 
                }
                ;

/* ======================================================================
 *                       C++ Support
 * ====================================================================== */

cpp_declaration : cpp_class_decl { $$ = $1; }
                | cpp_forward_class_decl { $$ = $1; }
                | cpp_template_decl { $$ = $1; }
             ;

cpp_class_decl  :

/* A simple class/struct/union definition */
                storage_class cpptype idcolon inherit LBRACE {
                   class_rename = make_name($3,0);
		   
		   Classprefix = NewString($3);
		   /* Deal with renaming */
		   if ($4) {
		     int i;
		     for (i = 0; i < Len($4); i++) {
		       String *n = Getitem($4,i);
		       rename_inherit(n,$3);
		     }
		   }

                   if (strcmp($2,"class") == 0) {
		     cplus_mode = CPLUS_PRIVATE;
		   } else {
		     cplus_mode = CPLUS_PUBLIC;
		   }
		   Swig_symbol_newscope();
		   Swig_symbol_setscopename($3);
		   start_line = line_number;
		   inclass = 1;
               } cpp_members RBRACE cpp_opt_declarators {
		 Node *p;
		 SwigType *ty;
		 inclass = 0;
		 $$ = new_node("class");
		 Setline($$,start_line);
		 Setattr($$,"name",$3);
		 Setattr($$,"kind",$2);
		 Setattr($$,"baselist",$4);

		 /* Check for pure-abstract class */
		 if (pure_abstract($7)) {
		   SetInt($$,"abstract",1);
		 }
		 
		 /* This bit of code merges in a previously defined %addmethod directive (if any) */
		 if (addmethods) {
		   Node *am = Getattr(addmethods,$3);
		   if (am) {
		     merge_addmethods(am);
		     appendChild($$,am);
		     Delattr(addmethods,$3);
		   }
		 }
		 if (!classes) classes = NewHash();
		 Setattr(classes,$3,$$);
		 
		 Setattr($$,"symtab",Swig_symbol_popscope());
		 appendChild($$,$7);
		 p = $9;
		 if (p) {
		   set_nextSibling($$,p);
		 }
		 
		 if (CPlusPlus) {
		   ty = NewString($3);
		 } else {
		   ty = NewStringf("%s %s", $2,$3);
		 }
		 while (p) {
		   Setattr(p,"storage",$1);
		   Setattr(p,"type",ty);
		   p = nextSibling(p);
		 }
		 /* Dump nested classes */
		 {
		   char *name = $3;
		   if ($9) {
		     SwigType *decltype = Getattr($9,"decl");
		     if (Cmp($1,"typedef") == 0) {
		       if (!decltype || !Len(decltype)) {
			 name = Char(Getattr($9,"name"));
			 Setattr($$,"tdname",name);
			 if (!Getattr(classes,name)) {
			   Setattr(classes,name,$$);
			 }
			 Setattr($$,"decl",decltype);
		       }
		     }
		   }
		   appendChild($$,dump_nested(name));
		 }
		 yyrename = NewString(class_rename);
		 add_tag($$);

		 if ($9)
		   add_symbols($9);

		 Classprefix = 0;
	       }

/* An unnamed struct, possibly with a typedef */

             | storage_class cpptype LBRACE {
	       class_rename = make_name(0,0);
	       if (strcmp($2,"class") == 0) {
		 cplus_mode = CPLUS_PRIVATE;
	       } else {
		 cplus_mode = CPLUS_PUBLIC;
	       }
	       Swig_symbol_newscope();
	       start_line = line_number;
	       inclass = 1;
	       Classprefix = NewString("");
             } cpp_members RBRACE declarator c_decl_tail {
	       String *unnamed;
	       Node *n, *p, *pp = 0;
	       Classprefix = 0;
	       inclass = 0;
	       unnamed = make_unnamed();
	       $$ = new_node("class");
	       Setline($$,start_line);
	       Setattr($$,"kind",$2);
	       Setattr($$,"storage",$1);
	       Setattr($$,"unnamed",unnamed);
	       
	       /* Check for pure-abstract class */
	       if (pure_abstract($5)) {
		 SetInt($$,"abstract",1);
	       }

	       n = new_node("cdecl");
	       Setattr(n,"name",$7.id);
	       Setattr(n,"unnamed",unnamed);
	       Setattr(n,"type",unnamed);
	       Setattr(n,"decl",$7.type);
	       Setattr(n,"parms",$7.parms);
	       Setattr(n,"storage",$1);
	       pp = n;
	       if ($8) {
		 set_nextSibling(n,$8);
		 p = $8;
		 while (p) {
		   pp = p;
		   Setattr(p,"unnamed",unnamed);
		   Setattr(p,"type",Copy(unnamed));
		   Setattr(p,"storage",$1);
		   p = nextSibling(p);
		 }
	       }
	       set_nextSibling($$,n);
	       {
		 /* If a proper typedef name was given, we'll use it to set the scope name */
		 char *name = 0;
		 if ($1 && (strcmp($1,"typedef") == 0)) {
		   if (!Len($7.type)) {	
		     name = $7.id;
		     Setattr($$,"tdname",name);
		     Setattr($$,"name",name);
		     if (!class_rename) class_rename = name;
		     Swig_symbol_setscopename(name);

		     /* If a proper name given, we use that as the typedef, not unnamed */
		     Clear(unnamed);
		     Append(unnamed, name);
		     
		     n = nextSibling(n);
		     set_nextSibling($$,n);

		     /* Check for previous addmethods */
		     if (addmethods) {
		       Node *am = Getattr(addmethods,name);
		       if (am) {
			 /* Merge the addmethods into the symbol table */
			 merge_addmethods(am);
			 appendChild($$,am);
			 Delattr(addmethods,name);
		       }
		     }
		     if (!classes) classes = NewHash();
		     Setattr(classes,name,$$);
		   } else {
		     Swig_symbol_setscopename((char*)"<unnamed>");
		   }
		 }
		 appendChild($$,$5);
		 appendChild($$,dump_nested(name));
	       }
	       /* Pop the scope */
	       Setattr($$,"symtab",Swig_symbol_popscope());
	       if (class_rename) {
		 yyrename = NewString(class_rename);
	       }
	       add_tag($$);
	       add_symbols(n);
              }
             ;

cpp_opt_declarators :  SEMI { $$ = 0; }
                    |  declarator c_decl_tail {
                        $$ = new_node("cdecl");
                        Setattr($$,"name",$1.id);
                        Setattr($$,"decl",$1.type);
                        Setattr($$,"parms",$1.parms);
			set_nextSibling($$,$2);
                    }
                    ;
/* ------------------------------------------------------------
   class Name;
   ------------------------------------------------------------ */

cpp_forward_class_decl : storage_class cpptype idcolon SEMI {
               $$ = new_node("classforward");
	       Setattr($$,"kind",$2);
	       Setattr($$,"name",$3);
	       Setattr($$,"sym:name", make_name(Getattr($$,"name"),0));
	     }
             ;

/* ------------------------------------------------------------
   template<...> decl
   ------------------------------------------------------------ */

/* function template */
cpp_template_decl : TEMPLATE LESSTHAN template_parms GREATERTHAN type declarator initializer cpp_temp_end {
                   if ($3.rparms) {
		     String  *macrocode = NewString("");
		     Printf(macrocode, "%%_template_%s(__name,%s,%s)\n", $6.id,$3.rparms,$3.sparms);
		     /* Create function definition */
		     if ($7.qualifier) SwigType_push($6.type,$7.qualifier);
		     if (SwigType_isfunction($6.type)) {
		       String *pdecl;
		       Delete(SwigType_pop_function($6.type));
		       SwigType_push($5,$6.type);
		       pdecl = NewStringf("%s(%s)", $6.id, ParmList_str($6.parms));
		       Printf(macrocode,"%%name(__name) %s;\n", SwigType_str($5,pdecl));
		       Printf(macrocode,"%%endtemplate;\n");
		       Delete(pdecl);
		       Seek(macrocode, 0, SEEK_SET);
		       Setline(macrocode,$1);
		       Setfile(macrocode,input_file);
		       Preprocessor_define(macrocode,1);
		       /*		       	       Printf(stdout,"%s\n", macrocode);  */
		     } 
		   }
		   $$ = 0;
                }
                | TEMPLATE LESSTHAN template_parms GREATERTHAN cpptype ID raw_inherit LBRACE {
		     skip_balanced('{','}'); 
		} SEMI {
		  if ($3.rparms) {
		     String  *macrocode = NewString("");
		     Printf(macrocode, "%%_template_%s(__name,%s,%s)\n", $6,$3.rparms,$3.sparms);
		     Printf(macrocode,"%%gencode %%{\n");
		     Printf(macrocode,"typedef %s< %s > __name;\n", $6,$3.sparms);
		     Printf(macrocode,"%%}\n");
		     Printf(macrocode,"class __name ");
		     if ($7) {
		       int i;
		       Printf(macrocode,": ");
		       for (i = 0; i < Len($7); i++) {
			 Printf(macrocode,"public %s", Getitem($7,i));
			 if (i < (Len($7) - 1)) Putc(',',macrocode);
		       }
		     }
		     Replace(scanner_ccode,$6,"__name", DOH_REPLACE_ID);
		     Printf(macrocode," %s;\n", scanner_ccode);
		     /* Include a reverse typedef to associate templated version with renamed version */
		     Printf(macrocode,"%%endtemplate;\n");
		     Printf(macrocode,"typedef __name %s< %s >;\n", $6,$3.sparms);
		     Printf(macrocode,"%%types(%s< %s > *);\n", $6, $3.sparms);

		     /*		     Printf(stdout,"%s\n", macrocode); */
		     Seek(macrocode,0, SEEK_SET);
		     Setline(macrocode,$1-4);
		     Setfile(macrocode,input_file);
		     Preprocessor_define(macrocode,1);
		  }
                  $$ = 0;
		}
                /* Forward template class declaration */
                | TEMPLATE LESSTHAN template_parms GREATERTHAN cpptype ID SEMI { $$ = 0; }
                ;


cpp_temp_end    : SEMI { }
                | LBRACE { skip_balanced('{','}') }
                ;

template_parms  : rawparms {
		   /* Rip out the parameter names */
		  Parm *p = $1;
		  $$.rparms = NewString("");
		  $$.sparms = NewString("");

		  while (p) {
		    String *name = Getattr(p,"name");
		    if (!name) {
		      /* Hmmm. Maybe it's a 'class T' parameter */
		      char *type = Char(Getattr(p,"type"));
		      if ((strncmp(type,"class ",6) == 0) || (strncmp(type,"typename ", 9) == 0)) {
			char *t = strchr(type,' ');
			Printf($$.rparms,"%s",t+1);
			Printf($$.sparms,"__swig%s",t+1);
		      } else {
			 Printf(stderr,"%s:%d. Missing template parameter name\n", input_file,line_number);
			 $$.rparms = 0;
			 $$.sparms = 0;
			 break;
		      }
		    } else {
		      Printf($$.rparms,"%s", Getattr(p,"name"));
		      Printf($$.sparms,"__swig%s",Getattr(p,"name"));
		    }
		    p = nextSibling(p);
		    if (p) {
		      Putc(',',$$.rparms);
		      Putc(',',$$.sparms);
		    }
		  }
                 }
                ;

cpp_members  : cpp_member cpp_members {
                   $$ = $1;
		   if ($$) {
		     Node *p = $$;
		     Node *pp =0;
		     while (p) {
		       pp = p;
		       p = nextSibling(p);
		     }
		     set_nextSibling(pp,$2);
		   } else {
		     $$ = $2;
		   }
             }
             | ADDMETHODS LBRACE cpp_members RBRACE cpp_members {
	       $$ = new_node("addmethods");
	       appendChild($$,$3);
	       set_nextSibling($$,$5);
	     }
             | empty { $$ = 0;}
	     | error {
	       skip_decl();
		   {
		     static int last_error_line = -1;
		     if (last_error_line != line_number) {
		       cparse_error(input_file, line_number,"Syntax error in input.\n");
		       last_error_line = line_number;
		     }
		   }
	     } cpp_members { 
                $$ = $3;
             }
             ;

/* ======================================================================
 *                         C++ Class members
 * ====================================================================== */

/* A class member.  May be data or a function. Static or virtual as well */

cpp_member   : c_declaration { $$ = $1; }
             | cpp_constructor_decl { $$ = $1; }
             | cpp_destructor_decl { $$ = $1; }
             | cpp_protection_decl { $$ = $1; }
             | cpp_swig_directive { $$ = $1; }
             | cpp_conversion_operator { $$ = $1; }
             | cpp_forward_class_decl { $$ = $1; }
             | cpp_nested { $$ = $1; }
             | storage_class idcolon SEMI { $$ = 0; }
             | SEMI { $$ = 0; }
             ;

/* Possibly a constructor */
/* Note: the use of 'type' is here to resolve a shift-reduce conflict.  For example:
            typedef Foo ();
            typedef Foo (*ptr)();
*/
  
cpp_constructor_decl : storage_class type LPAREN parms RPAREN ctor_end {
		 SwigType *decl = NewString("");
		 $$ = new_node("constructor");

		 /* Since the parse performs type-corrections in template mode, we
                    have to undo the correction here.  Ugh. */

		 if ((templatemode) && (Strcmp($2,templatename) == 0)) {
		   $2 = NewString(Classprefix);
		 }
		 Setattr($$,"name",$2);
		 Setattr($$,"parms",$4);
		 SwigType_add_function(decl,$4);
		 Setattr($$,"decl",decl);
		 if (Len(scanner_ccode)) {
		   patch_template_code(scanner_ccode);
		   Setattr($$,"code",Copy(scanner_ccode));
		 }
		 Setattr($$,"feature:new","1");
		 add_symbols($$);
	       }
              ;

/* A destructor (hopefully) */

cpp_destructor_decl : NOT ID LPAREN parms RPAREN cpp_end {
               $$ = new_node("destructor");
	       Setattr($$,"name",NewStringf("~%s",$2));
	       if (Len(scanner_ccode)) {
		 patch_template_code(scanner_ccode);
		 Setattr($$,"code",Copy(scanner_ccode));
	       }
	       add_symbols($$);
	      }

/* A virtual destructor */

              | VIRTUAL NOT ID LPAREN parms RPAREN cpp_vend {
		$$ = new_node("destructor");
		Setattr($$,"storage","virtual");
		Setattr($$,"name",NewStringf("~%s",$3));
		if ($7) {
		  Setattr($$,"value","0");
		}
		if (Len(scanner_ccode)) {
		  patch_template_code(scanner_ccode);
		  Setattr($$,"code",Copy(scanner_ccode));
		}
		add_symbols($$);
	      }
              ;


/* C++ type conversion operator */
cpp_conversion_operator : storage_class COPERATOR type pointer LPAREN parms RPAREN cpp_vend {
                 $$ = new_node("cdecl");
                 Setattr($$,"type",$3);
		 Setattr($$,"name",$2);
		 SwigType_add_function($4,$6);
		 Setattr($$,"decl",$4);
		 Setattr($$,"parms",$6);
		 add_symbols($$);
	       }
              | storage_class COPERATOR type LPAREN parms RPAREN cpp_vend {
		String *t = NewString("");
		$$ = new_node("cdecl");
		Setattr($$,"type",$3);
		Setattr($$,"name",$2);
		SwigType_add_function(t,$5);
		Setattr($$,"decl",t);
		Setattr($$,"parms",$5);
		add_symbols($$);
              }
              ;

/* public: */
cpp_protection_decl : PUBLIC COLON { 
                $$ = new_node("access");
		Setattr($$,"kind","public");
                cplus_mode = CPLUS_PUBLIC;
              }

/* private: */
              | PRIVATE COLON { 
                $$ = new_node("access");
                Setattr($$,"kind","private");
		cplus_mode = CPLUS_PRIVATE;
	      }

/* protected: */

              | PROTECTED COLON { 
		$$ = new_node("access");
		Setattr($$,"kind","protected");
		cplus_mode = CPLUS_PROTECTED;
	      }
              ;


/* ----------------------------------------------------------------------
   Nested structure.    This is a sick "hack".   If we encounter
   a nested structure, we're going to grab the text of its definition and
   feed it back into the scanner.  In the meantime, we need to grab
   variable declaration information and generate the associated wrapper
   code later.  Yikes!

   This really only works in a limited sense.   Since we use the
   code attached to the nested class to generate both C/C++ code,
   it can't have any SWIG directives in it.  It also needs to be parsable
   by SWIG or this whole thing is going to puke.
   ---------------------------------------------------------------------- */

/* A struct sname { } id;  declaration */

cpp_nested : storage_class cpptype ID LBRACE { start_line = line_number; skip_balanced('{','}');
	      } nested_decl SEMI {
	        $$ = 0;
		if (cplus_mode == CPLUS_PUBLIC) {
		  if ($6.id) {
		    if (strcmp($2,"class") == 0) {
		      Printf(stderr,"%s:%d.  Warning. Nested classes not currently supported (ignored).\n", input_file, line_number);
		      /* Generate some code for a new class */
		    } else {
		      Nested *n = (Nested *) malloc(sizeof(Nested));
		      n->code = NewString("");
		      Printv(n->code, "typedef ", $2, " ",
			     Char(scanner_ccode), " $classname_", $6.id, ";\n", NULL);

		      n->name = Swig_copy_string($6.id);
		      n->line = start_line;
		      n->type = NewString("");
		      n->kind = $2;
		      SwigType_push(n->type, $6.type);
		      n->next = 0;
		      add_nested(n);
		    }
		  }
		}
	      }

/* An unnamed nested structure definition */
              | storage_class cpptype LBRACE { start_line = line_number; skip_balanced('{','}');
              } nested_decl SEMI {
	        $$ = 0;
		if (cplus_mode == CPLUS_PUBLIC) {
		  if (strcmp($2,"class") == 0) {
		    Printf(stderr,"%s:%d.  Warning. Nested classes not currently supported (ignored)\n", input_file, line_number);
		    /* Generate some code for a new class */
		  } else if ($5.id) {
		    /* Generate some code for a new class */
		    Nested *n = (Nested *) malloc(sizeof(Nested));
		    n->code = NewString("");
		    Printv(n->code, "typedef ", $2, " " ,
			    Char(scanner_ccode), " $classname_", $5.id, ";\n",NULL);
		    n->name = Swig_copy_string($5.id);
		    n->line = start_line;
		    n->type = NewString("");
		    n->kind = $2;
		    SwigType_push(n->type,$5.type);
		    n->next = 0;
		    add_nested(n);
		  }
		}
	      } 
              ;

nested_decl   : declarator { $$ = $1;}
              | empty { $$.id = 0; }
              ;


/* These directives can be included inside a class definition */

cpp_swig_directive: pragma_directive { $$ = $1; }

/* A constant (includes #defines) inside a class */
             | constant_directive { $$ = $1; }

/* This is the new style rename */

             | name_directive { $$ = $1; }

/* New mode */
             | NEW cpp_member {
	       $$ = new_node("new");
	       appendChild($$,$2);
             }
/* rename directive */
             | rename_directive { $$ = $1; }
             | feature_directive { $$ = $1; }
             | varargs_directive { $$ = $1; }
             | insert_directive { $$ = $1; }
             | typemap_directive { $$ = $1; }
             | apply_directive { $$ = $1; }
             | clear_directive { $$ = $1; }
             ;

cpp_end        : cpp_const SEMI {
	            Clear(scanner_ccode);
               }
               | cpp_const LBRACE { skip_balanced('{','}'); }
               ;

cpp_vend       : cpp_const SEMI { Clear(scanner_ccode); $$ = 0;  }
               | cpp_const EQUAL definetype SEMI { Clear(scanner_ccode); $$ = 1; }
               | cpp_const LBRACE { skip_balanced('{','}'); $$ = 0; }
               ;


/* ====================================================================== 
 *                       PRIMITIVES
 * ====================================================================== */

storage_class  : EXTERN { $$ = "extern"; }
               | EXTERN string { 
                   if (strcmp($2,"C") == 0) {
		     $$ = "externc";
		   } else {
		     Printf(stderr,"%s:%d.  Unrecognized extern type \"%s\" (ignored).\n", input_file,line_number, $2);
		     $$ = 0;
		   }
               }
               | STATIC { $$ = "static"; }
               | TYPEDEF { $$ = "typedef"; }
               | VIRTUAL { $$ = "virtual"; }
               | FRIEND { $$ = "friend"; }
               | empty { $$ = 0; }
               ;

/* ------------------------------------------------------------------------------
   Function parameter lists
   ------------------------------------------------------------------------------ */

parms          : rawparms {
                 Parm *p;
		 $$ = $1;
		 p = $1;
                 while (p) {
		   Replace(Getattr(p,"type"),"typename ", "", DOH_REPLACE_ANY);
		   p = nextSibling(p);
                 }
               }
    	       ;

rawparms          : parm ptail {
		  if (1) { 
		    set_nextSibling($1,$2);
		    $$ = $1;
		  } else {
		    $$ = $2;
		  }
		}
               | empty { $$ = 0; }
               ;

ptail          : COMMA parm ptail {
                 set_nextSibling($2,$3);
		 $$ = $2;
                }
               | empty { $$ = 0; }
               ;


parm           : rawtype parameter_declarator {
                   SwigType_push($1,$2.type);
		   $$ = NewParm($1,$2.id);
		   Setfile($$,input_file);
		   Setline($$,line_number);
		   if ($2.defarg)
		     Setattr($$,"value",$2.defarg);
		}

                | PERIOD PERIOD PERIOD {
		  SwigType *t = NewString("v(...)");
		  $$ = NewParm(t, 0);
		  Setfile($$,input_file);
		  Setline($$,line_number);


		  /*
                  cparse_error(input_file, line_number, "Variable length arguments not supported (ignored).\n");
		  $$ = NewParm(NewSwigType(T_INT),(char *) "varargs"); */

		}
		;

def_args       : EQUAL definetype { 
                  Node *n;
                  $$ = $2; 
		  /* If the value of a default argument is in the symbol table,  we replace it with it's
                     fully qualified name.  Needed for C++ enums and other features */
		  n = Swig_symbol_lookup($2.val);
		  if (n) {
		    $$.val = NewStringf("%s%s", Swig_symbol_qualified(n),$2.val);
		  }
               }
               | EQUAL AND idcolon {
		 Node *n = Swig_symbol_lookup($3);
		 if (n) {
		   $$.val = NewStringf("&%s%s", Swig_symbol_qualified(n),$3);
		 } else {
		   $$.val = NewStringf("&%s",$3);
		 }
		 $$.rawval = 0;
		 $$.type = T_USER;
	       }
               | EQUAL LBRACE {
		 skip_balanced('{','}');
		 $$.val = 0;
		 $$.rawval = 0;
                 $$.type = T_INT;
	       }
               | COLON NUM_INT { 
		 $$.val = 0;
		 $$.rawval = 0;
		 $$.type = 0;
	       }
               | empty {
                 $$.val = 0;
                 $$.rawval = 0;
                 $$.type = T_INT;
               }
               ;

parameter_declarator : declarator def_args {
                 $$ = $1;
		 $$.defarg = $2.rawval ? $2.rawval : $2.val;
            }
            | abstract_declarator def_args {
              $$ = $1;
	      $$.defarg = $2.rawval ? $2.rawval : $2.val;
            }
            | def_args {
   	      $$.type = 0;
              $$.id = 0;
	      $$.defarg = $1.rawval ? $1.rawval : $1.val;
            }
            ;

typemap_parameter_declarator : declarator {
                 $$ = $1;
		 if (SwigType_isfunction($1.type)) {
		   Delete(SwigType_pop_function($1.type));
		 } else if (SwigType_isarray($1.type)) {
		   SwigType *ta = SwigType_pop_arrays($1.type);
		   if (SwigType_isfunction($1.type)) {
		     Delete(SwigType_pop_function($1.type));
		   } else {
		     $$.parms = 0;
		   }
		   SwigType_push($1.type,ta);
		   Delete(ta);
		 } else {
		   $$.parms = 0;
		 }
            }
            | abstract_declarator {
              $$ = $1;
	      if (SwigType_isfunction($1.type)) {
		Delete(SwigType_pop_function($1.type));
	      } else if (SwigType_isarray($1.type)) {
		SwigType *ta = SwigType_pop_arrays($1.type);
		if (SwigType_isfunction($1.type)) {
		  Delete(SwigType_pop_function($1.type));
		} else {
		  $$.parms = 0;
		}
		SwigType_push($1.type,ta);
		Delete(ta);
	      } else {
		$$.parms = 0;
	      }
            }
            | empty {
   	      $$.type = 0;
              $$.id = 0;
	      $$.parms = 0;
	      }
            ;


declarator :  pointer direct_declarator {
              $$ = $2;
	      if ($$.type) {
		SwigType_push($1,$$.type);
		Delete($$.type);
	      }
	      $$.type = $1;
           }
           | pointer AND direct_declarator {
              $$ = $3;
	      SwigType_add_reference($1);
              if ($$.type) {
		SwigType_push($1,$$.type);
		Delete($$.type);
	      }
	      $$.type = $1;
           }
           | direct_declarator {
              $$ = $1;
	      if (!$$.type) $$.type = NewString("");
           }
           | AND direct_declarator { 
	     $$ = $2;
	     $$.type = NewString("");
	     SwigType_add_reference($$.type);
	     if ($2.type) {
	       SwigType_push($$.type,$2.type);
	       Delete($2.type);
	     }
           }
           | idcolon DSTAR direct_declarator { 
	     SwigType *t = NewString("");

	     $$ = $3;
	     SwigType_add_memberpointer(t,$1);
	     if ($$.type) {
	       SwigType_push(t,$$.type);
	       Delete($$.type);
	     }
	     $$.type = t;
	     } 
           | pointer idcolon DSTAR direct_declarator { 
	     SwigType *t = NewString("");
	     $$ = $4;
	     SwigType_add_memberpointer(t,$2);
	     SwigType_push($1,t);
	     if ($$.type) {
	       SwigType_push($1,$$.type);
	       Delete($$.type);
	     }
	     $$.type = $1;
	     Delete(t);
	   }
           | pointer idcolon DSTAR AND direct_declarator { 
	     $$ = $5;
	     SwigType_add_memberpointer($1,$2);
	     SwigType_add_reference($1);
	     if ($$.type) {
	       SwigType_push($1,$$.type);
	       Delete($$.type);
	     }
	     $$.type = $1;
	   }
           | idcolon DSTAR AND direct_declarator { 
	     SwigType *t = NewString("");
	     $$ = $4;
	     SwigType_add_memberpointer(t,$1);
	     SwigType_add_reference(t);
	     if ($$.type) {
	       SwigType_push(t,$$.type);
	       Delete($$.type);
	     } 
	     $$.type = t;
	   }
           ;
             
direct_declarator : idcolon {
  /* Note: This is non-standard C.  Template declarator is allowed to follow an identifier */
                 $$.id = Char($1);
		 $$.type = 0;
		 $$.parms = 0;
		 $$.have_parms = 0;
                  }

/* Technically, this should be LPAREN declarator RPAREN, but we get reduce/reduce conflicts */
                  | LPAREN pointer direct_declarator RPAREN {
		    $$ = $3;
		    if ($$.type) {
		      SwigType_push($2,$$.type);
		      Delete($$.type);
		    }
		    $$.type = $2;
                  }
                  | LPAREN idcolon DSTAR direct_declarator RPAREN {
		    SwigType *t;
		    $$ = $4;
		    t = NewString("");
		    SwigType_add_memberpointer(t,$2);
		    if ($$.type) {
		      SwigType_push(t,$$.type);
		      Delete($$.type);
		    }
		    $$.type = t;
		    }
                  | direct_declarator LBRACKET RBRACKET { 
		    SwigType *t;
		    $$ = $1;
		    t = NewString("");
		    SwigType_add_array(t,(char*)"");
		    if ($$.type) {
		      SwigType_push(t,$$.type);
		      Delete($$.type);
		    }
		    $$.type = t;
                  }
                  | direct_declarator LBRACKET expr RBRACKET { 
		    SwigType *t;
		    $$ = $1;
		    t = NewString("");
		    SwigType_add_array(t,$3.val);
		    if ($$.type) {
		      SwigType_push(t,$$.type);
		      Delete($$.type);
		    }
		    $$.type = t;
                  }
                  | direct_declarator LPAREN parms RPAREN {
	            List *l;
		    SwigType *t;
                    $$ = $1;
		    t = NewString("");
		    SwigType_add_function(t,$3);
		    if (!$$.have_parms) {
		      $$.parms = $3;
		      $$.have_parms = 1;
		    }
		    if (!$$.type) {
		      $$.type = t;
		    } else {
		      SwigType_push(t, $$.type);
		      Delete($$.type);
		      $$.type = t;
		    }
		  }
                  ;

abstract_declarator : pointer {
		    $$.type = $1;
                    $$.id = 0;
		    $$.parms = 0;
		    $$.have_parms = 0;
                  }
                  | pointer direct_abstract_declarator { 
                     $$ = $2;
                     SwigType_push($1,$2.type);
		     $$.type = $1;
		     Delete($2.type);
                  }
                  | pointer AND {
		    $$.type = $1;
		    SwigType_add_reference($$.type);
		    $$.id = 0;
		    $$.parms = 0;
		    $$.have_parms = 0;
		  }
                  | pointer AND direct_abstract_declarator {
		    $$ = $3;
		    SwigType_add_reference($1);
		    if ($$.type) {
		      SwigType_push($1,$$.type);
		      Delete($$.type);
		    }
		    $$.type = $1;
                  }
                  | direct_abstract_declarator {
		    $$ = $1;
                  }
                  | AND direct_abstract_declarator {
		    $$ = $2;
		    $$.type = NewString("");
		    SwigType_add_reference($$.type);
		    if ($2.type) {
		      SwigType_push($$.type,$2.type);
		      Delete($2.type);
		    }
                  }
                  | AND { 
                    $$.id = 0;
                    $$.parms = 0;
		    $$.have_parms = 0;
                    $$.type = NewString("");
		    SwigType_add_reference($$.type);
                  }
                  | idcolon DSTAR { 
		    $$.type = NewString("");
                    SwigType_add_memberpointer($$.type,$1);
                    $$.id = 0;
                    $$.parms = 0;
		    $$.have_parms = 0;
      	          }
                  | pointer idcolon DSTAR { 
		    SwigType *t = NewString("");
                    $$.type = $1;
		    $$.id = 0;
		    $$.parms = 0;
		    $$.have_parms = 0;
		    SwigType_add_memberpointer(t,$2);
		    SwigType_push($$.type,t);
		    Delete(t);
                  }
                  | pointer idcolon DSTAR direct_abstract_declarator { 
		    $$ = $4;
		    SwigType_add_memberpointer($1,$2);
		    if ($$.type) {
		      SwigType_push($1,$$.type);
		      Delete($$.type);
		    }
		    $$.type = $1;
                  }
                  ;

direct_abstract_declarator : direct_abstract_declarator LBRACKET RBRACKET { 
		    SwigType *t;
		    $$ = $1;
		    t = NewString("");
		    SwigType_add_array(t,(char*)"");
		    if ($$.type) {
		      SwigType_push(t,$$.type);
		      Delete($$.type);
		    }
		    $$.type = t;
                  }
                  | direct_abstract_declarator LBRACKET expr RBRACKET { 
		    SwigType *t;
		    $$ = $1;
		    t = NewString("");
		    SwigType_add_array(t,$3.val);
		    if ($$.type) {
		      SwigType_push(t,$$.type);
		      Delete($$.type);
		    }
		    $$.type = t;
                  }
                  | LBRACKET RBRACKET { 
		    $$.type = NewString("");
		    $$.id = 0;
		    $$.parms = 0;
		    $$.have_parms = 0;
		    SwigType_add_array($$.type,(char*)"");
                  }
                  | LBRACKET expr RBRACKET { 
		    $$.type = NewString("");
		    $$.id = 0;
		    $$.parms = 0;
		    $$.have_parms = 0;
		    SwigType_add_array($$.type,$2.val);
		  }
                  | LPAREN abstract_declarator RPAREN {
                    $$ = $2;
		  }
                  | direct_abstract_declarator LPAREN parms RPAREN {
	            List *l;
		    SwigType *t;
                    $$ = $1;
		    t = NewString("");
                    SwigType_add_function(t,$3);
		    if (!$$.type) {
		      $$.type = t;
		    } else {
		      SwigType_push(t,$$.type);
		      Delete($$.type);
		      $$.type = t;
		    }
		    if (!$$.have_parms) {
		      $$.parms = $3;
		      $$.have_parms = 1;
		    }
		  }
                  | LPAREN parms RPAREN {
	            List *l;
                    $$.type = NewString("");
                    SwigType_add_function($$.type,$2);
		    $$.parms = $2;
		    $$.have_parms = 1;
		    $$.id = 0;
                  }
                  ;


pointer    : STAR type_qualifier pointer { 
               $$ = NewString("");
               SwigType_add_pointer($$);
	       SwigType_push($$,$2);
	       SwigType_push($$,$3);
	       Delete($3);
           }
           | STAR pointer {
	     $$ = NewString("");
	     SwigType_add_pointer($$);
	     SwigType_push($$,$2);
	     Delete($2);
	     } 
           | STAR type_qualifier { 
	     	$$ = NewString("");	
		SwigType_add_pointer($$);
	        SwigType_push($$,$2);
           }
           | STAR {
	      $$ = NewString("");
	      SwigType_add_pointer($$);
	      }
           ;

type_qualifier : type_qualifier_raw { 
                  $$ = NewString("");
	          SwigType_add_qualifier($$,$1);
               }
               | type_qualifier_raw type_qualifier { 
                  $$ = $2; 
                  SwigType_add_qualifier($$,$1);
               }
               ;

type_qualifier_raw :  CONST { $$ = "const"; }
                   |  VOLATILE { $$ = "volatile"; }
                   ;

/* Data type must be a built in type or an identifier for user-defined types
   This type can be preceded by a modifier. */

type            : rawtype {
                   $$ = $1;
                   Replace($$,"typename ","", DOH_REPLACE_ANY);
                }
                ;

rawtype       : type_qualifier type_right {
                   $$ = $2;
	           SwigType_push($$,$1);
               }
               | type_right { $$ = $1; }
               ;

type_right     : primitive_type { $$ = $1;
                  /* Printf(stdout,"primitive = '%s'\n", $$);*/
                }
               | TYPE_BOOL { $$ = $1; }
               | TYPE_VOID { $$ = $1; }
               | TYPE_TYPEDEF template_decl { $$ = NewStringf("%s%s",$1,$2); }
               | ENUM ID { $$ = NewStringf("enum %s", $2); }
               | TYPE_RAW { $$ = $1; }
               | type_right type_qualifier {
		  $$ = $1;
	          SwigType_push($$,$2);
     	       }

               | idcolon {
                  if (templatemode) {
		    patch_template_type($1);
		  }
		  $$ = $1;
               }
               | cpptype idcolon { 
		 $$ = NewStringf("%s %s", $1, $2);
               }
               ;

primitive_type : primitive_type_list {
		 if (!$1.type) $1.type = NewString("int");
		 if ($1.us) {
		   $$ = NewStringf("%s %s", $1.us, $1.type);
		   Delete($1.us);
                   Delete($1.type);
		 } else {
                   $$ = $1.type;
		 }
		 if (Cmp($$,"signed int") == 0) {
		   Delete($$);
		   $$ = NewString("int");
                 } else if (Cmp($$,"signed long") == 0) {
		   Delete($$);
                   $$ = NewString("long");
                 } else if (Cmp($$,"signed short") == 0) {
		   Delete($$);
		   $$ = NewString("short");
		 } else if (Cmp($$,"signed long long") == 0) {
		   Delete($$);
		   $$ = NewString("long long");
		 }
               }
               ;

primitive_type_list : type_specifier { 
                 $$ = $1;
               }
               | type_specifier primitive_type_list {
                    if ($1.us && $2.us) {
		      Printf(stderr,"%s:%d. Extra %s specifier.\n", input_file, line_number, $2.us);
		    }
                    $$ = $2;
                    if ($1.us) $$.us = $1.us;
		    if ($1.type) {
		      if (!$2.type) $$.type = $1.type;
		      else {
			int err = 0;
			if ((Cmp($1.type,"long") == 0)) {
			  if ((Cmp($2.type,"long") == 0) || (Cmp($2.type,"double") == 0)) {
			    $$.type = NewStringf("long %s", $2.type);
			  } else if (Cmp($2.type,"int") == 0) {
			    $$.type = $1.type;
			  } else {
			    err = 1;
			  }
			} else if ((Cmp($1.type,"short")) == 0) {
			  if (Cmp($2.type,"int") == 0) {
			    $$.type = $1.type;
			  } else {
			    err = 1;
			  }
			} else if (Cmp($1.type,"int") == 0) {
			  $$.type = $2.type;
			} else if (Cmp($1.type,"double") == 0) {
			  if (Cmp($2.type,"long") == 0) {
			    $$.type = NewString("long double");
			  } else {
			    err = 1;
			  }
			}
			if (err) {
			  Printf(stderr,"%s:%d. Extra %s specifier.\n", input_file, line_number, $1.type);
			}
		      }
		    }
               }
               ; 


type_specifier : TYPE_INT { 
		    $$.type = NewString("int");
                    $$.us = 0;
               }
               | TYPE_SHORT { 
                    $$.type = NewString("short");
                    $$.us = 0;
                }
               | TYPE_LONG { 
                    $$.type = NewString("long");
                    $$.us = 0;
                }
               | TYPE_CHAR { 
                    $$.type = NewString("char");
                    $$.us = 0;
                }
               | TYPE_FLOAT { 
                    $$.type = NewString("float");
                    $$.us = 0;
                }
               | TYPE_DOUBLE { 
                    $$.type = NewString("double");
                    $$.us = 0;
                }
               | TYPE_SIGNED { 
                    $$.us = NewString("signed");
                    $$.type = 0;
                }
               | TYPE_UNSIGNED { 
                    $$.us = NewString("unsigned");
                    $$.type = 0;
                }
               ;

definetype     : { scanner_check_typedef(); } expr {
                   $$ = $2;
		   $$.rawval = 0;
		   scanner_ignore_typedef();
                }
                | string {
                   $$.val = NewString($1);
		   $$.rawval = NewStringf("\"%(escape)s\"",$$.val);
                   $$.type = T_STRING;
		}
                | CHARCONST {
                   $$.val = NewString($1);
		   if (Len($$.val)) {
		     $$.rawval = NewStringf("\'%(escape)s\'",$$.val);
		   } else {
		     $$.rawval = NewString("\'\\0'");
		   }
		   $$.type = T_CHAR;
		 }
                ;

/* Some stuff for handling enums */

ename          :  ID { $$ = $1; }
               |  empty { $$ = (char *) 0;}
               ;

/* SWIG enum list */

enumlist       :  enumlist COMMA edecl { 
                   Node *n = Getattr($1,"_last");
		   if (!n) {
		     set_nextSibling($1,$3);
		     Setattr($1,"_last",$3);
		   } else {
		     set_nextSibling(n,$3);
		     Setattr($1,"_last",$3);
		   }
		   $$ = $1;
               }
               |  edecl { $$ = $1; }
               ;

edecl          :  ID {
		   $$ = new_node("enumitem");
		   Setattr($$,"name",$1);
		   Setattr($$,"type",NewSwigType(T_INT));
		 }
                 | ID EQUAL etype {
		   $$ = new_node("enumitem");
		   Setattr($$,"name",$1);
	           if ($3.type == T_CHAR) {
		     Setattr($$,"value",$3.val);
		     Setattr($$,"type",NewSwigType(T_CHAR));
		   } else {
		     Setattr($$,"value",$1);
		     Setattr($$,"type",NewSwigType(T_INT));
		   }
                 }
                 | empty { $$ = 0; }
                 ;

etype            : expr {
                   $$ = $1;
		   if (($$.type != T_INT) && ($$.type != T_UINT) &&
		       ($$.type != T_LONG) && ($$.type != T_ULONG) &&
		       ($$.type != T_SHORT) && ($$.type != T_USHORT) &&
		       ($$.type != T_SCHAR) && ($$.type != T_UCHAR)) {
		     cparse_error(input_file,line_number,"Type error. Expecting an int\n");
		   }

                }
                | CHARCONST {
                   $$.val  = NewString($1);
		   $$.type = T_INT;
		 }
                ;

/* Arithmetic expressions.   Used for constants and other cool stuff.
   Really, we're not doing anything except string concatenation, but
   this does allow us to parse many constant declarations.
 */

expr           :  NUM_INT { $$ = $1; }
               |  NUM_FLOAT { $$ = $1; }
               |  NUM_UNSIGNED { $$ = $1; }
               |  NUM_LONG { $$ = $1; }
               |  NUM_ULONG { $$ = $1; }
               |  NUM_LONGLONG { $$ = $1; }
               |  NUM_ULONGLONG { $$ = $1; }
               |  SIZEOF LPAREN type parameter_declarator RPAREN {
  		  SwigType_push($3,$4.type);
		  $$.val = NewStringf("sizeof(%s)",SwigType_str($3,0));
		  $$.type = T_INT;
	       }
               | idcolon {
		 $$.val = $1;
		 $$.type = T_INT;
               }
               | expr PLUS expr {
		 $$.val = NewStringf("%s+%s",$1.val,$3.val);
		 $$.type = promote($1.type,$3.type);
	       }
               | expr MINUS expr {
		 $$.val = NewStringf("%s-%s",$1.val,$3.val);
		 $$.type = promote($1.type,$3.type);
	       }
               | expr STAR expr {
		 $$.val = NewStringf("%s*%s",$1.val,$3.val);
		 $$.type = promote($1.type,$3.type);
	       }
               | expr SLASH expr {
		 $$.val = NewStringf("%s/%s",$1.val,$3.val);
		 $$.type = promote($1.type,$3.type);
	       }
               | expr AND expr {
		 $$.val = NewStringf("%s&%s",$1.val,$3.val);
		 $$.type = promote($1.type,$3.type);
	       }
               | expr OR expr {
		 $$.val = NewStringf("%s|%s",$1.val,$3.val);
		 $$.type = promote($1.type,$3.type);
	       }
               | expr XOR expr {
		 $$.val = NewStringf("%s^%s",$1.val,$3.val);
		 $$.type = promote($1.type,$3.type);
	       }
               | expr LSHIFT expr {
		 $$.val = NewStringf("%s<<%s",$1.val,$3.val);
		 $$.type = promote($1.type,$3.type);
	       }
               | expr RSHIFT expr {
		 $$.val = NewStringf("%s>>%s",$1.val,$3.val);
		 $$.type = promote($1.type,$3.type);
	       }
               | expr LAND expr {
		 $$.val = NewStringf("%s&&%s",$1.val,$3.val);
		 $$.type = T_ERROR;
	       }
               | expr LOR expr {
		 $$.val = NewStringf("%s||%s",$1.val,$3.val);
		 $$.type = T_ERROR;
	       }
               |  MINUS expr %prec UMINUS {
		 $$.val = NewStringf("-%s",$2.val);
		 $$.type = $2.type;
	       }
               |  NOT expr {
		 $$.val = NewStringf("~%s",$2.val);
		 $$.type = $2.type;
	       }
               | LNOT expr {
                 $$.val = NewStringf("!%s",$2.val);
		 $$.type = T_ERROR;
	       }
               |  LPAREN expr RPAREN {
		 $$.val = NewStringf("(%s)",$2.val);
		 $$.type = $2.type;
	       }
               | LPAREN cast_type parameter_declarator RPAREN expr %prec UMINUS {
		 $$.val = $5.val;
		 $$.type = $5.type;
               }
               ;


cast_type      : type_qualifier cast_type_right {
                   $$ = $2;
	           SwigType_push($$,$1);
               }
               | cast_type_right { $$ = $1; }
               ;

cast_type_right:  primitive_type { $$ = $1; }
               | TYPE_BOOL { $$ = $1; }
               | TYPE_VOID { $$ = $1; }
               | cpptype ID { $$ = NewStringf("%s %s", $1, $2); }
               | TYPE_TYPEDEF template_decl { $$ = NewStringf("%s%s",$1,$2); }
               | ENUM ID { $$ = NewStringf("enum %s", $2); }
               | TYPE_RAW { $$ = $1; }
               | cast_type_right type_qualifier {
		  $$ = $1;
	          SwigType_push($$,$2);
     	       }
               ;


inherit        : raw_inherit {
		 $$ = $1;
                 if ($1) {
		   /* Patch names for templates */
		   String *name;
                   for (name = Firstitem($1); name; name = Nextitem($1)) {
		     if (classes) {
		       /* The name might be the same as a template map */
		       if (templatemaps) {
			 String *altname = Getattr(templatemaps,name);
			 if (altname) {
			   Clear(name);
			   Append(name,altname);
			 }
		       }
		     }
		   }
		 }
               }
               ;

raw_inherit     : COLON base_list { $$ = $2; }
                | empty { $$ = 0; }
                ;

base_list      : base_specifier {
	           $$ = NewList();
	           if ($1) Append($$,$1);
               }

               | base_list COMMA base_specifier {
                   $$ = $1;
                   if ($3) Append($$,$3);
               }
               ;

base_specifier : opt_virtual idcolon {
                  Printf(stderr,"%s:%d. No access specifier given for base class %s (ignored).\n",input_file,line_number,$2);
		  $$ = (char *) 0;
               }
	       | opt_virtual access_specifier opt_virtual idcolon {
		 $$ = 0;
	         if (strcmp($2,"public") == 0) {
		   $$ = $4;
		   Setfile($$, input_file);
		   Setline($$, line_number);
		 }
               }
               ;

access_specifier :  PUBLIC { $$ = (char*)"public"; }
               | PRIVATE { $$ = (char*)"private"; }
               | PROTECTED { $$ = (char*)"protected"; }
               ;


cpptype        : CLASS { $$ = (char*)"class"; }
               | STRUCT { $$ = (char*)"struct"; }
               | UNION {$$ = (char*)"union"; }
               | TYPENAME { $$ = (char *)"typename"; }
               ;

opt_virtual    : VIRTUAL
               | empty
               ;

cpp_const      : type_qualifier {
                    $$ = $1;
               }
               | THROW LPAREN {
	            skip_balanced('(',')');
		    Clear(scanner_ccode);
                    $$ = 0;
               }
               | type_qualifier THROW LPAREN {
  		    skip_balanced('(',')');
		    Clear(scanner_ccode);
                    $$ = $1;
               }
               | empty { $$ = 0; }
               ;

ctor_end       : cpp_const ctor_initializer SEMI { Clear(scanner_ccode); }
               | cpp_const ctor_initializer LBRACE { skip_balanced('{','}'); }
               ;

ctor_initializer : COLON mem_initializer_list
               | empty
               ;

mem_initializer_list : mem_initializer
               | mem_initializer_list COMMA mem_initializer
               ;

mem_initializer : ID LPAREN {
	            skip_balanced('(',')');
                    Clear(scanner_ccode);
            	}
                ;

template_decl : LESSTHAN { 
                     String *s;
                     skip_balanced('<','>');
		     s = Copy(scanner_ccode);
		     if (templatetypes) {
		       String *key;
		       for (key = Firstkey(templatetypes); key; key = Nextkey(templatetypes)) {
			 String *t = SwigType_str(Getattr(templatetypes,key),0);
			 Replace(s,key,t, DOH_REPLACE_ID);
			 Delete(t);
		       }
		     }
		     canonical_template(s);
		     $$ = Char(s);
                 }
               | empty { $$ = (char*)"";  }
               ;

idstring       : ID { $$ = $1; }
               | string { $$ = $1; }
               ;

idcolon        : idtemplate idcolontail { 
                  $$ = 0;
		  if (!$$) $$ = NewStringf("%s%s", $1,$2);
      	          Delete($2);
               }
               | NONID DCOLON idtemplate idcolontail { 
		 $$ = NewStringf("::%s%s",$3,$4);
                 Delete($4);
               }
               | idtemplate {
		 $$ = NewString($1);
   	       }     
               | NONID DCOLON idtemplate {
		 $$ = NewStringf("::%s",$3);
               }
               | OPERATOR {
                 $$ = NewString($1);
	       }
               | NONID DCOLON OPERATOR {
                 $$ = NewStringf("::%s",$3);
               }
               ;

idcolontail    : DCOLON idtemplate idcolontail {
                   $$ = NewStringf("::%s%s",$2,$3);
		   Delete($3);
               }
               | DCOLON idtemplate {
                   $$ = NewStringf("::%s",$2);
               }
               | DCOLON OPERATOR {
                   $$ = NewStringf("::%s",$2);
               }
               ;

idtemplate    : ID template_decl {
                  $$ = NewStringf("%s%s",$1,$2);
		  if (templatemode) {
		    if ((templateargs) && (Cmp($$,templateargs) == 0)) {
		      Delete($$);
		      $$ = NewString(templateiname);
		    }
		  }
                  scanner_last_id(1);
              }
              ;

/* Concatenated strings */
string         : string STRING { 
                   $$ = (char *) malloc(strlen($1)+strlen($2)+1);
                   strcpy($$,$1);
                   strcat($$,$2);
               }
               | STRING { $$ = $1;}
               ; 

stringbrace    : string {
		 $$ = NewString($1);
               }
               | LBRACE {
                  skip_balanced('{','}');
		  $$ = NewString(scanner_ccode);
               }
              | HBLOCK {
		 $$ = $1;
              }
               ;
 
/* Keyword arguments */
kwargs         : idstring EQUAL string {
		 $$ = NewHash();
		 Setattr($$,"name",$1);
		 Setattr($$,"value",$3);
               }
               | idstring EQUAL string COMMA kwargs {
		 $$ = NewHash();
		 Setattr($$,"name",$1);
		 Setattr($$,"value",$3);
		 set_nextSibling($$,$5);
               }
               | idstring {
                 $$ = NewHash();
                 Setattr($$,"name",$1);
	       }
               | idstring COMMA kwargs {
                 $$ = NewHash();
                 Setattr($$,"name",$1);
                 set_nextSibling($$,$3);
               }
               ;

empty          :   ;

%%

/* Called by the parser (yyparse) when an error is found.*/
void yyerror (const char *e) {
  /*    Printf(stderr,"Syntax error\n");  */
}

