/******************************************************************************
 * Simplified Wrapper and Interface Generator  (SWIG)
 *
 * Modified for Ocaml from mzscheme.cxx by Art Yerkes
 * Modifications from Benedikt Grundmann (bgrundmann@gmx.net):
 *
 *  Constants as values
 *  Enums as proper ocaml types
 *
 * Original Author : David Beazley
 *
 * Department of Computer Science
 * University of Chicago
 * 1100 E 58th Street
 * Chicago, IL  60637
 * beazley@cs.uchicago.edu
 *
 * Please read the file LICENSE for the copyright and terms by which SWIG
 * can be used and distributed.
 *****************************************************************************/

static char cvsroot[] = "$Header$";

/***********************************************************************
 * $Header$
 *
 * ocaml.cxx
 *
 * Definitions for adding functions to Ocaml 101
 ***********************************************************************/

#include "swigmod.h"
#include <ctype.h>

/* Extern items from the swig system */
extern int SwigType_isenum(SwigType *t);
extern void emit_one( Node *n );

static void lcase( char *c ) { while( *c ) { *c = tolower( *c ); c++; } }

static char *ocaml_usage = (char*)
    ("\n"
     "Ocaml Options (available with -ocaml)\n"
     "-help           - Print this help\n"
     "-objects        - Use objects\n"
     "-onlyobjects    - Only export an object interface to a C++ object\n"
     "-modwrap name   - Wrap in module 'Name'\n"
     "-mlout name     - Specify the ML output file name\n"
     "\n");
static const char *ocaml_path = "ocaml";

/* Data used by more than one handler */

static int const_enum;           /* Is this constant an enum label */
static int classmode;            /* Are we in a class definition */
static int error_count;          /* Number of errors encountered */
static int objects = 0;		 /* Use objects. */
static int onlyobjects = 0;	 /* Do not export functions that back
				    methods. */
static int mliout = 0;		 /* Are we in .mli file */

String *classname = NULL;        /* Name of the current class */

/* Wrapmod */

String *wrapmod = NULL;          /* Non-null if we want an additional module
				    wrapper. */
String *module = NULL;           /* Module name */
String *modfile_cmd = NULL;      /* Command-line specified .ml output file */

/* C(++) output */
static String *f_init = NULL;
static String *f_header = NULL;
static String *f_init_fn = NULL;
static String *f_runtime = NULL;
static String *f_wrappers = NULL;

/* Ocaml output */
static String *f_module = NULL;       /* Main body of the module file */
static String *f_module_file = NULL;  /* Module file itself */
static String *f_modclass = NULL;     /* Class definitions */
static String *f_pvariant_def = NULL; /* Polymorphic variant definition at
					 top of file. */
static String *f_pvariant_value = NULL; /* Function to get the int value of a
					   polymorphic variant label */
static String *f_pvariant_to_int = NULL; /* enum_to_int definition. */
static String *f_pvariant_from_int = NULL; /* int_to_enum definition. */

/* Lists */
static Hash *seen_deleters = NULL;    /* A delete function is in here if we
					 have touched it before.  This allows
					 the deleter to be installed in the
					 custom_operations block on new */
static Hash *seen_types = NULL;       /* Types we have emitted so far. */
static Hash *seen_classes = NULL;     /* Classes emitted so far. */
static Hash *seen_labels = NULL;      /* Enum labels emitted already. */
static Hash *seen_names = NULL;       /* Symbols emitted. */

static char *simple_types[][2] = {
    { "_enum", "enum" },
    { "_bool", "bool" },
    { "_void", "unit" },
    { "_int", "int" },
    { "_short", "int" },
    { "_long", "int64" },
    { "_unsigned_long", "int64" },
    { "_char", "char" },
    { "_p_char", "string" },
    { "_float", "float" },
    { "_double", "float" },
    { "_oc_bool", "bool" },
    { "_unsigned_int", "int32" },
    { "_unsigned_short", "int" },
    { "_unsigned_char", "char" }, 
    { "_long_long", "int64" },
    { "_unisgned_long_long", "int64" },
    { NULL, NULL }
};

// ---------------------------------------------------------------------
// main()
//
// Parse arguments.
// ---------------------------------------------------------------------

class OCAML : public Language {
    virtual void main (int argc, char *argv[]) {
	int i;
	
	SWIG_library_directory(ocaml_path);
	
	wrapmod = NULL;
	modfile_cmd = NULL;
	
	onlyobjects = 0;
	objects = 0;
	
	// Look for certain command line options
	for (i = 1; i < argc; i++) {
	    if (argv[i]) {
		if (strcmp (argv[i], "-help") == 0) {
		    fputs (ocaml_usage, stderr);
		    SWIG_exit (0);
		}
		else if (strcmp (argv[i], "-mlout") == 0) {
		    if( argv[i + 1] ) {
			modfile_cmd = NewString( argv[i + 1] );
			Swig_mark_arg (i);
			Swig_mark_arg (i + 1);
			i++;
		    } else {
			Swig_arg_error();
		    }
		}
		else if (strcmp (argv[i], "-onlyobjects") == 0) {
		    onlyobjects = 1;
		    Swig_mark_arg (i);
		}
		else if (strcmp (argv[i], "-objects") == 0) {
		    objects = 1;
		    Swig_mark_arg (i);
		}
		else if (strcmp (argv[i], "-modwrap") == 0) {
		    if( argv[i + 1] ) {
			wrapmod = NewString( argv[i + 1] );
			Swig_mark_arg (i);
			Swig_mark_arg (i + 1);
			i++;
		    } else {
			Swig_arg_error();
		    }
		}
	    }
	}
	
	// Add a symbol for this module
	
	Preprocessor_define ((void *) "SWIGOCAML",0);
	
	// Set name of typemaps
	SWIG_typemap_lang("ocaml");
	
	// Read in default typemaps */
	SWIG_config_file("ocaml.i");
    }

    void print_unseen_type( String *type ) {
	String *rtype = SwigType_manglestr(SwigType_typedef_resolve_all(type));
	String *ntype = SwigType_manglestr(type);

	if( Getattr(seen_types,ntype) ) return;
	else {
	    if( Getattr(seen_types,rtype) ) {
		Setattr(seen_types,ntype,rtype);
		Printf(f_module,"  type %s = %s\n", ntype, rtype );
	    } else {
		Setattr(seen_types,ntype,ntype);
		Printf(f_module,"  type %s\n", ntype );
	    }
	}
    }

// --------------------------------------------------------------------
// top()
// --------------------------------------------------------------------
    int 
    top(Node *n) {
	String *modfile = NULL;

	assert(Getattr(n,"name"));
	set_module(Char(Getattr(n,"name")));

	if( modfile_cmd ) {
	    if( modfile ) Delete( modfile );
	    modfile = Copy(modfile_cmd);
	}

	if( !modfile ) {
	    modfile = Copy(module);
	    Printf(modfile, ".ml");
	}

	mliout = 0;
	processInterface( modfile, n );

	String *tmp = NewString(Char(modfile));
	Delete(modfile);
	modfile = NewString("");
	Printf(modfile,"%si", tmp); // Append 'i' to make .mli
	Delete(tmp);

	mliout = 1;
	return processInterface( modfile, n );
    }

    int
    processInterface( String *modfile, Node *n )
	{
	    /* Initialize all of the output files */
	    String *outfile = Getattr(n,"outfile");

	    classmode = 0;
	    const_enum = 0;

	    if( !outfile || !modfile ) {
		Printf(stderr,"%s name was not set.\n", 
		       (!outfile) ? "Output file" : "Module file");
		SWIG_exit(EXIT_FAILURE);
	    }

	    if( mliout ) 
		f_runtime = NewString("");
	    else
		f_runtime = NewFile(outfile,"w");

	    if (!f_runtime) {
		Printf(stderr,"*** Can't open '%s'\n", outfile);
		SWIG_exit(EXIT_FAILURE);
	    }

	    f_module_file = NewFile(modfile,"w");
	    if (!f_module_file) {
		Printf(stderr,"*** Can't open '%s'\n", modfile);
		SWIG_exit(EXIT_FAILURE);
	    }

	    classname = NewString("");
	    f_module = NewString("");

	    Printf(f_module_file,
		   "(* -*- buffer-read-only: t -*- vi: set ro: *)\n");
	    if( !mliout ) {
		Printf(f_module_file,"open Int32\n" );
		Printf(f_module_file,"open Int64\n" );
	    }

	    if( wrapmod )
		Printf(f_module_file,"module %s = struct\n",wrapmod);

	    seen_types = NewHash();
	    seen_deleters = NewHash();
	    seen_classes = NewHash();
	    seen_labels = NewHash();
	    seen_names = NewHash();

	    int i;

	    // I starts at one here to skip the _enum type.
	    for( i = 1; simple_types[i][0]; i++ ) {
		Printf(f_module, "type %s = %s\n", 
		       simple_types[i][0], simple_types[i][1] );
		Setattr(seen_types,simple_types[i][0],
			NewString(simple_types[i][1]));
	    }
	    // Now add the _enum type to seen_types
	    Setattr(seen_types,simple_types[0][0],
		    NewString(simple_types[0][1]));

	    f_init = NewString("");
	    f_header = NewString("");
	    f_init_fn = NewString("");
	    f_wrappers = NewString("");
	    f_modclass = NewString("");

	    f_pvariant_value = NewString("");

	    if( mliout ) {
		f_pvariant_to_int = NewString("");
		f_pvariant_from_int = NewString("");
	    } else {
		f_pvariant_to_int = 
		    NewString("let enum_to_int _v_ =\nmatch _v_ with\n"
					      "`int x -> x\n");
		f_pvariant_from_int = NewString("let int_to_enum _v_ =\n");
	    }

	    f_pvariant_def = NewString("type _enum =\n[ `int of int\n");

	    /* Register file targets with the SWIG file handler */
	    Swig_register_filebyname("header",f_header);
	    Swig_register_filebyname("wrapper",f_wrappers);
	    Swig_register_filebyname("runtime",f_runtime);
	    Swig_register_filebyname("init",f_init_fn);

	    Printf(f_runtime, 
		   "/* -*- buffer-read-only: t -*- vi: set ro: */\n");
	    Swig_banner (f_runtime);

	    if (NoInclude) {
		Printf(f_runtime, "#define SWIG_NOINCLUDE\n");
	    }
	    
	    Printf( f_init_fn,
		    "value _wrapper_init_fn(value v) {\n"
		    "  CAMLparam1(v);\n");

	    Language::top(n);

	    Printf( f_init_fn,
		    "\n"
		    "  CAMLreturn(Val_unit);\n"
		    "}\n\n" );
	    
	    Printf(f_pvariant_def,"]\n");

	    if( !mliout ) {
		Printf(f_pvariant_to_int,
		       "let _ = Callback.register \"%s_enum_to_int\" "
		       "enum_to_int\n",
		       module);
		Printf(f_pvariant_from_int,"`int _v_\n");
		Printf(f_pvariant_to_int,
		       "let enum_bits _v_ = int_to_enum "
		       "(List.fold_left (fun a b -> a lor (enum_to_int b)) "
		       "0 _v_)\n");
		Printf(f_pvariant_to_int,
		       "let check_enum_bit f _v_ = "
		       "(enum_to_int _v_) land (enum_to_int f) == "
		       "(enum_to_int f)\n");
		Printf(f_pvariant_to_int,
		       "let bits_enum _v_ f_list = "
		       "List.filter (fun f -> check_enum_bit f _v_) f_list\n");
	
		Printf(f_pvariant_from_int,
		       "let _ = Callback.register \"%s_int_to_enum\" "
		       "int_to_enum\n",
		       module);
	    } else {
		Printf(f_pvariant_to_int,
		       "val enum_bits : _enum list -> _enum\n");
		Printf(f_pvariant_to_int,
		       "val check_enum_bit : _enum -> _enum -> bool\n");
		Printf(f_pvariant_to_int,
		       "val bits_enum : _enum -> _enum list -> _enum list\n");
	    }

	    SwigType_emit_type_table (f_runtime, f_wrappers);

	    Dump(f_pvariant_def,f_module_file);
	    Dump(f_module,f_module_file);
	    Dump(f_modclass,f_module_file);

	    // *** Polymorphic variant support ***
	    Dump(f_pvariant_from_int,f_module_file);
	    Dump(f_pvariant_to_int,f_module_file);

	    /* Produce the enum_to_int and int_to_enum functions */
	    Printf(f_wrappers,
		   "static int enum_to_int( value v ) {\n"
		   "  value *en_to_int_cb = "
		   "caml_named_value(\"%s_enum_to_int\");\n"
		   "  return Int_val(callback(*en_to_int_cb,v));\n"
		   "}\n"
		   "static value int_to_enum( int v ) {\n"
		   "  value *int_to_en_cb = "
		   "caml_named_value(\"%s_int_to_enum\");\n"
		   "  return callback(*int_to_en_cb,Val_int(v));\n"
		   "}\n",module,module);

	    /* Close all of the files */
	    if( wrapmod )
		Printf(f_module_file,"end\n",wrapmod);

	    Dump(f_header,f_runtime);

	    Dump(f_wrappers,f_runtime);
	    Dump(f_init_fn,f_runtime);
	    Wrapper_pretty_print(f_init,f_runtime);

	    // Init function support in module
	    if( !mliout )
		Printf(f_module_file,
		       "external _wrapper_init_fn : unit -> unit = "
		       "\"_wrapper_init_fn\"\n"
		       "let _ = _wrapper_init_fn ()\n");

	    Delete(f_header);
	    Delete(f_wrappers);
	    Delete(f_init);
	    Delete(f_init_fn);

	    if( !mliout ) 
		Close(f_runtime);

	    Close(f_module_file);
	    Delete(f_runtime);
	    Delete(f_module);
	    Delete(f_module_file);

	    return SWIG_OK;
	}

// ---------------------------------------------------------------------
// set_module(char *mod_name)
//
// Sets the module name.
// Does nothing if it's already set (so it can be overridden as a command
// line option).
//
//----------------------------------------------------------------------

    void
    set_module (char *mod_name)
	{
	    module = NewString(mod_name);
	}

    String *uniqueify(char *word) {
	int x = 1;
	String *out = NewString(word);
    
	while( Getattr(seen_names,out) ) {
	    Delete( out );
	    out = NewString( word );
	    Printf( out, "_%d", x );
	}

	Setattr(seen_names,out,out);

	return out;
    }

    int banned_word(char *word) {
	char *keywords[] = { "type", "class", "object", "true", "false",
			     "val", "function", "fun", "let", "rec",
			     "char", "int", "bool", "string", "end",
			     "begin", "in", "match", "with", "try",
			     "new",
			     NULL };
	int i;

	for( i = 0; keywords[i]; i++ )
	    if( !strcmp( word, keywords[i] ) ) return 1;
	return 0;
    }

// ----------------------------------------------------------------------
// functionWrapper()
// Create a function declaration and register it with the interpreter.
// ----------------------------------------------------------------------

    void throw_unhandled_ocaml_type_error (SwigType *d) {
	Printf (stderr, "%s : Line %d. Unable to handle type %s.\n", 
		input_file, line_number, SwigType_str(d,0));
	error_count++;
	}

    void delete_replacement(SwigType *tm, SwigType *d) {
	String *delete_fn = NewString("");
	SwigType *dcaml_deref = Copy(d);
	String *dcaml_delete = SwigType_manglestr(dcaml_deref);
    
	if( Getattr(seen_deleters, dcaml_delete) )
	    Printf( delete_fn, "%s", Getattr(seen_deleters,dcaml_delete) );
	else
	    Printf( delete_fn, "_wrap_delete_void" );
    
	Replaceall( tm, "$delete_fn", delete_fn );

	Delete(delete_fn);
	Delete(dcaml_deref);
	Delete(dcaml_delete);
    }

/* Return true iff T is a pointer type */

    static int is_a_pointer (SwigType *t) {
	return SwigType_ispointer(SwigType_typedef_resolve_all(t));
    }
    
    static int is_simple (SwigType *t) {
	int i;
	String *str = SwigType_manglestr(SwigType_typedef_resolve_all(t));
	
	for( i = 0; simple_types[i][0]; i++ )
	    if( !strcmp( Char(str), simple_types[i][0] ) ) return 1;
	
	return 0;
    }
    
    static int is_enum(SwigType *t) {
	return !strncmp(Char(SwigType_typedef_resolve_all(t)),"enum ",5);    
    }

    static SwigType *process_type( SwigType *t ) { 
        // Return a SwigType that's liveable for ocaml
	if( is_enum(t) ) return(NewString("enum"));
	if( !is_simple(t) && !is_a_pointer(t) ) {
	    SwigType *tt = Copy(t);
	    SwigType_add_pointer(tt);
	    return tt;
	} else {
	    return Copy(t);
	}
    }

    int closureWrapper(Node *n) {
	return SWIG_OK;
    }

    int functionWrapper(Node *n) {
	char *iname = GetChar(n,"sym:name");
	SwigType *d = Getattr(n,"type");
	SwigType *dcaml = process_type(d);
	ParmList *l = Getattr(n,"parms");
	Parm *p;
	Wrapper *f = NewWrapper();
	String *vname_class = NewString("");
	String *sizetuple = NewString("");
	String *source = NewString("");
	String *target = NewString("");
	String *arg = NewString("");
	String *cleanup = NewString("");
	String *outarg = NewString("");
	String *build = NewString("");
	String *ml_vname = NewString("");
	String *ml_iname = NewString(GetChar(n,"name"));

	String   *tm;
	int argout_set = 0;
	int i = 0;
	int numargs;
	int numreq;
	int is_static = 1;

	// Make a wrapper name for this
	char *wname = Char(Swig_name_wrapper(iname));

	ml_vname = Copy(Swig_name_wrapper(iname));
	/* Skip the _wrap_ and then lowercase the rest */

	char *vname = Char(ml_vname) + strlen("_wrap_");

	// Register this deleter as one we recognize 
	if( !strncmp( vname, "new_", strlen( "new_" ) ) ) {
	    String *del_vname = NewString("");
	    SwigType *dcaml_deref = Copy(d);
	    String *dcaml_delete = SwigType_manglestr(dcaml_deref);

	    Printf(del_vname,"_wrap_delete_%s",vname+strlen("new_"));
	    Setattr( seen_deleters, dcaml_delete, del_vname );
	    Delete(del_vname);
	}

	lcase(vname);
	lcase(Char(ml_iname));

	String *vname_final = uniqueify(vname);

	if( classmode ) {
	    char *proposed_str = vname + strlen(Char(classname)) + 1;
	    String *s = NewString("");
	    if( banned_word(proposed_str) )
		Printf(s,"_%s",proposed_str);
	    else
		Printf(s,proposed_str);
	    vname_class = s;
	}
    
	print_unseen_type( dcaml );

	// writing the function wrapper function
	Printf(f->def, 
	       "#ifdef __cplusplus\n"
	       "extern \"C\"\n"
	       "#endif\n"
	       "value %s( value args )\n"
	       "{\n"
	       "\tCAMLparam1(args);\n", wname);
    
	// Declare return variable and arguments
	// number of parameters
	// they are called arg0, arg1, ...
	// the return value is called result
    
	emit_args(d, l, f);

	/* Attach the standard typemaps */
	emit_attach_parmmaps(l,f);

	numargs = emit_num_arguments(l);
	numreq  = emit_num_required(l);

	// adds local variables
	Wrapper_add_local(f, "swig_result", "CAMLlocal1(swig_result)");

	// Now write code to extract the parameters (this is super ugly)

	if( numargs > 0 ) {
	    SwigType *pt = Getattr(l,"type");
	    SwigType *ptcaml = process_type(pt);
	    String *tn = SwigType_manglestr(ptcaml);

	    lcase(Char(tn));
	    if( !strncmp( Char(tn), "_p_", 3 ) &&
		!strcmp( Char(tn) + 3, Char(classname) ) ) is_static = 0;

	    Delete(ptcaml);
	} else is_static = 1;

	for (i = 0, p = l; i < numargs; i++) {
	    /* Skip ignored arguments */
	    while (Getattr(p,"tmap:ignore")) {
		p = Getattr(p,"tmap:ignore:next");
	    }

	    SwigType *pt = Getattr(p,"type");
	    SwigType *ptcaml = process_type(pt);
	    String   *ln = Getattr(p,"lname");

	    print_unseen_type( ptcaml );

	    // Produce names of source and target
	    Clear(source);
	    Clear(target);
	    Clear(arg);
	    if( numargs == 1 ) 
		Printf(source, "args");
	    else 
		Printf(source, "Field(args,%d)", i);
	    Printf(target, "%s",ln);
	    Printv(arg, Getattr(p,"name"),0);

	    // Handle parameter types.
	    if ((tm = Getattr(p,"tmap:in"))) {
		Replace(tm,"$input",source,DOH_REPLACE_ANY);
		Setattr(p,"emit:input",source);
		Printv(f->code, tm, "\n", 0);
		p = Getattr(p,"tmap:in:next");
	    } else {
		// no typemap found
		// check if typedef and resolve
		Printf(f->code,"\tfailwith(\"Type error\");\n");
		p = nextSibling(p);
	    }
	}

	for(p = l; p;) {
	    SwigType *tcaml = process_type(Getattr(p,"type"));
	    print_unseen_type( tcaml );
	    p = nextSibling(p);
	    Delete(tcaml);
	}

	// Write out signature for function in module file
	if( !classmode || !onlyobjects || !mliout ||
	    !strncmp( vname, "new_", 4 ) ||
	    !strncmp( vname, "delete_", 7 ) ) {
	    Printf(f_module, "  external %s : ", vname_final);
	
	    if( numargs == 0 ) 
		Printf(f_module," unit");
	    else if( numargs != 1 ) Printf(f_module,"(");
	
	    for(p = l; p;) {
		SwigType *tcaml = process_type(Getattr(p,"type"));
		Printf(f_module, "%s", SwigType_manglestr(tcaml));
		p = nextSibling(p);
		if( p ) Printf(f_module," * ");
		Delete(tcaml);
	    }
	
	    if( numargs > 1 ) 
		Printf(f_module,")");
	
	    Printf(f_module, " -> %s = \"%s\"\n", SwigType_manglestr(dcaml), 
		   wname );
	}

	if( classmode ) {
	    if( strncmp( vname, "new_", 4 ) &&
		strncmp( vname, "delete_", 7 ) ) {
		Printf( f_modclass, "  method %s", 
			vname_class );

		if( mliout ) {
		    if( numargs > 1 ) {
			Printf( f_modclass, " : (" );
		    
			p = l; if( p ) p = nextSibling(p);
			while( p ) {
			    SwigType *tcaml = process_type(Getattr(p,"type"));
			    Printf(f_modclass, "%s", 
				   SwigType_manglestr(tcaml));
			    p = nextSibling(p);
			    if( p ) Printf(f_modclass," * ");
			    Delete(tcaml);
			}

			Printf( f_modclass, ") -> %s\n",
				SwigType_manglestr(dcaml) );
		    } else {
			Printf( f_modclass, " : %s\n", 
				SwigType_manglestr(dcaml) );
		    }
		} else {
		    char x = 'a';

		    if( numargs > 1 ) 
			Printf( f_modclass, " (" );

		    p = l; if( p ) p = nextSibling(p);
		    while( p ) {
			Printf(f_modclass, "%c", x);
			++x;
			p = nextSibling(p);
			if( p ) Printf(f_modclass,",");
		    }
		
		    Printf( f_modclass, "%s", numargs > 1 ? ")" : "" );
		
		    Printf( f_modclass, " = %s ", 
			    vname_final );

		    if( numargs != 1 )
			Printf( f_modclass, "(" );
		    if( numargs > 0 ) 
			Printf( f_modclass, "c_self" );
		    if( numargs > 1 ) 
			Printf( f_modclass, "," );
		
		    x = 'a';
		
		    p = l; if( p ) p = nextSibling(p);
		    while( p ) {
			Printf(f_modclass, "%c", x);
			++x;
			p = nextSibling(p);
			if( p ) Printf(f_modclass,",");
		    }

		    if( numargs != 1 ) 
			Printf( f_modclass, ")" );

		    Printf( f_modclass, "\n" );
		}
	    }
	}

	/* Insert constraint checking code */
	for (p = l; p;) {
	    if ((tm = Getattr(p,"tmap:check"))) {
		Replace(tm,"$target",Getattr(p,"lname"),DOH_REPLACE_ANY);
		Printv(f->code,tm,"\n",0);
		p = Getattr(p,"tmap:check:next");
	    } else {
		p = nextSibling(p);
	    }
	}

	// Pass output arguments back to the caller.

	for (p = l; p;) {
	    if ((tm = Getattr(p,"tmap:argout"))) {
		Replace(tm,"$arg",Getattr(p,"emit:input"), DOH_REPLACE_ANY);
		Replace(tm,"$input",Getattr(p,"emit:input"), DOH_REPLACE_ANY);
		Printv(outarg,tm,"\n",0);
		p = Getattr(p,"tmap:argout:next");
		argout_set = 1;
	    } else {
		p = nextSibling(p);
	    }
	}

	// Free up any memory allocated for the arguments.

	/* Insert cleanup code */
	for (p = l; p;) {
	    if ((tm = Getattr(p,"tmap:freearg"))) {
		Replace(tm,"$target",Getattr(p,"lname"),DOH_REPLACE_ANY);
		Printv(cleanup,tm,"\n",0);
		p = Getattr(p,"tmap:freearg:next");
	    } else {
		p = nextSibling(p);
	    }
	}

	// Now write code to make the function call

	emit_action(n,f);

	// Now have return value, figure out what to do with it.

	if ((tm = Swig_typemap_lookup_new("out",n,"result",0))) {
	    delete_replacement(tm,d);
	    Replaceall(tm,"$source","result");
	    Replaceall(tm,"$target","swig_result");
	    Replaceall(tm,"$result","swig_result");

	    Printv(f->code, tm, "\n",0);
	} else {
	    throw_unhandled_ocaml_type_error (dcaml);
	}

	// Dump the argument output code
	Printv(f->code, Char(outarg),0);

	// Dump the argument cleanup code
	Printv(f->code, Char(cleanup),0);

	// Look for any remaining cleanup

	if ((NewObject) || (Getattr(n,"feature:new"))) {
	    if ((tm = Swig_typemap_lookup_new("newfree",n,"result",0))) {
		Replaceall(tm,"$source","result");
		Printv(f->code, tm, "\n",0);
	    }
	}

	// Free any memory allocated by the function being wrapped..

	if ((tm = Swig_typemap_lookup_new("ret",n,"result",0))) {
	    Replaceall(tm,"$source","result");
	    Printv(f->code, tm, "\n",0);
	}

	// Wrap things up (in a manner of speaking)

	Printf(f->code, "\tCAMLreturn(swig_result);\n");
	Printv(f->code, "}\n",0);

	Wrapper_print(f, f_wrappers);

	Delete(vname_final);
	Delete(source);
	Delete(target);
	Delete(arg);
	Delete(outarg);
	Delete(cleanup);
	Delete(build);
	Delete(ml_vname);
	Delete(ml_iname);
	Delete(dcaml);
	DelWrapper(f);

	return SWIG_OK;
    }

// -----------------------------------------------------------------------
// variableWrapper()
//
// Create a link to a C variable.
// This creates a single function _wrap_swig_var_varname().
// This function takes a single optional argument.   If supplied, it means
// we are setting this variable to some value.  If omitted, it means we are
// simply evaluating this variable.  Either way, we return the variables
// value.
// -----------------------------------------------------------------------

    int variableWrapper(Node *n) {
	char *name  = GetChar(n,"name");
	char *iname = GetChar(n,"sym:name");
	SwigType *t = Getattr(n,"type");
	SwigType *tcaml = process_type(t);
	
	char  var_name[256];
	String *tm;
	String *vname_class;
	String *ml_iname = NewString(iname);
	String *ml_vname = NewString("");
	String *tm2 = NewString("");
	String *argnum = NewString("0");
	String *arg = NewString("Field(args,0)");
	String *mangled_tcaml = SwigType_manglestr(t);
	Wrapper *f;
	
	if( Getattr(n,"feature:closure") ) return closureWrapper(n);
	
	if( strlen(Char(mangled_tcaml)) == 0 ) return SWIG_OK;
	
	f = NewWrapper();
	
	print_unseen_type( t );
	
	// evaluation function names
	
	strcpy(var_name, Char(Swig_name_wrapper(iname)));
	
	Printf (f->def, 
		"#ifdef __cplusplus\n"
		"extern \"C\"\n"
		"#endif\n"
		"value %s (value args)\n"
		"{\n"
		"\tCAMLparam1(args);\n", var_name);
	
	Wrapper_add_local (f, "swig_result", "value swig_result");
	
	ml_vname = Copy(Swig_name_wrapper(iname));
	/* Skip the _wrap_ and then lowercase the rest */
	
	char *vname = Char(ml_vname) + strlen("_wrap_");
	lcase(vname);
	lcase(Char(ml_iname));
	
	String *name_no_get = NewString(Char(vname));
	char *get_part = strstr( Char(name_no_get), "_get" );
	
	if( get_part )
	    *get_part = 0;
	
	// Polymorphic variant support
	if( const_enum )
	    f_pvariant_value = Copy(name_no_get);
	
	if( classmode ) {
	    String *s = NewString("");
	    if( banned_word(vname + strlen(Char(classname)) + 1) )
		Printf(s,vname + strlen(Char(classname)) + 1);
	    else
		Printf(s,vname);
	    vname_class = s;
	}
	
	String *vname_final = uniqueify(vname);
	
	// Write out signature for module file
	if(Getattr(n,"feature:immutable") || const_enum) {
	    if( mliout )
		Printf( f_module, "  val %s : %s\n", 
			vname_final, mangled_tcaml );
	    else {
		Printf( f_module, "  external _%s : unit -> %s = \"%s\"\n", 
			vname_final, mangled_tcaml, var_name );
		
		Printf( f_module, "  let %s = _%s ()\n", 
			name_no_get,
			vname_final );
	    }
	} else {
	    if( !onlyobjects || !classmode || !mliout ) {
		Printf( f_module, 
			"  external %s_set : %s -> %s = \"%s\"\n"
			"  external %s_get : unit -> %s = \"%s\"\n",
			vname_final, mangled_tcaml, mangled_tcaml, var_name,
			vname_final, mangled_tcaml, var_name );
	    }
	    
	    if( classmode ) {
		if( mliout ) {
		    Printf( f_modclass, 
			    "  method %s_set : %s -> %s\n"
			    "  method %s_get : unit -> %s\n",
			    vname_class, mangled_tcaml, mangled_tcaml,
			    vname_class, mangled_tcaml );
		} else {
		    Printf( f_modclass, 
			    "  method %s_set = %s_set\n"
			    "  method %s_get = %s_get\n",
			    vname_class, vname_final,
			    vname_class, vname_final );
		}
	    }
	    
	    /* Check for a setting of the variable value */
	    Printf (f->code, "if (args != (value)1) {\n");
	    if ((tm = Swig_typemap_lookup_new("varin",n,name,0))) {
		Replaceall(tm,"$source","args");
		Replaceall(tm,"$target",name);
		Replaceall(tm,"$input","args");
		Printv(f->code, tm, "\n",0);
	    } else {
		throw_unhandled_ocaml_type_error (t);
	    }
	    Printf(f->code, "}\n");
	}
	
	// Now return the value of the variable (regardless
	// of evaluating or setting)
	
	if ((tm = Swig_typemap_lookup_new("varout",n,name,0))) {
	    Replaceall(tm,"$source",name);
	    Replaceall(tm,"$target","swig_result");
	    Replaceall(tm,"$result","swig_result");
	    Printf (f->code, "%s\n", tm);
	} else {
	    throw_unhandled_ocaml_type_error (t);
	}
	Printf (f->code, "\nCAMLreturn(swig_result);\n");
	Printf (f->code, "}\n");
	
	Wrapper_print (f, f_wrappers);
	
	Delete(vname_final);
	Delete(name_no_get);
	Delete(argnum);
	Delete(arg);
	Delete(tm2);
	Delete(ml_vname);
	DelWrapper(f);
	return SWIG_OK;
    }

    /* Benedikt Grundmann inspired --> Enum wrap styles */

    int enumvalueDeclaration(Node *n) {
	String *value = Getattr(n,"value");
	String *name  = Getattr(n,"name");
	String *tmpValue;
  
	if (value)
	    tmpValue = NewString(value);
	else
	    tmpValue = NewString(name);
	Setattr(n, "value", tmpValue);

	// Explanation: Do everything as usual, but dump f_pvariant_value
	// into this string, so we can retrieve it.  This is to avoid some
	// duplication of logic above. 

	if(f_pvariant_value) { 
	    Delete(f_pvariant_value);
	    f_pvariant_value = NULL;
	}

	const_enum = 1;
	Language::enumvalueDeclaration(n); // Output 
	const_enum = 0;

	if(f_pvariant_value) {
	    int suffix_digit = 1;
	    String *output_name = Copy(Getattr(n,"name"));
	    while( Getattr(seen_labels,output_name) ) {
		Delete(output_name);
		output_name = NewString("");
		Printf(output_name,"%s_%d",Getattr(n,"name"),suffix_digit);
		suffix_digit++;
	    }

	    // Never output overlapping names
	    Setattr(seen_labels,output_name,output_name);

	    // Implementation only
	    if( !mliout ) {
		Printf(f_pvariant_to_int,"| `%s -> %s\n", 
		       Getattr(n,"name"), f_pvariant_value);
		Printf(f_pvariant_from_int,"if _v_ == %s then `%s else\n", 
		       f_pvariant_value, Getattr(n,"name"));
	    }
	    // Both
	    Printf(f_pvariant_def,"| `%s\n", Getattr(n,"name"));
	}

	return SWIG_OK;
    }

// -----------------------------------------------------------------------
// constantWrapper()
// ------------------------------------------------------------------------

    int constantWrapper(Node *n) {
	char *name      = GetChar(n,"name");
	char *iname     = GetChar(n,"sym:name");
	SwigType *type  = Getattr(n,"type");
	String   *value = Getattr(n,"value");

	String *var_name = NewString("");
	String *rvalue = NewString("");
	String *temp = NewString("");
	String *tm;

	print_unseen_type( type );

	// Make a static variable;

	Printf (var_name, "_wrap_const_%s", Swig_name_mangle(iname));

	// Build the name for scheme.

	if ((SwigType_type(type) == T_USER) && (!is_a_pointer(type))) {
	    fprintf (stderr, "%s : Line %d.  Unsupported constant value.\n",
		     input_file, line_number);
	    return SWIG_NOWRAP;
	}

	// See if there's a typemap

	Printv(rvalue, value,0);
	if ((SwigType_type(type) == T_CHAR) && (is_a_pointer(type) == 1)) {
	    temp = Copy(rvalue);
	    Clear(rvalue);
	    Printv(rvalue, "\"", temp, "\"",0);
	}
	if ((SwigType_type(type) == T_CHAR) && (is_a_pointer(type) == 0)) {
	    Delete(temp);
	    temp = Copy(rvalue);
	    Clear(rvalue);
	    Printv(rvalue, "'", temp, "'",0);
	}
	if ((tm = Swig_typemap_lookup_new("constant",n,name,0))) {
	    Replaceall(tm,"$source",rvalue);
	    Replaceall(tm,"$value",rvalue);
	    Replaceall(tm,"$target",name);
	    Printf (f_init, "%s\n", tm);
	} else {
	    // Create variable and assign it a value

	    Printf (f_header, "static %s %s = ", 
		    SwigType_str(type,0), var_name);
	    if ((SwigType_type(type) == T_STRING)) {
		Printf (f_header, "\"%s\";\n", value);
	    } else if (SwigType_type(type) == T_CHAR) {
		Printf (f_header, "\'%s\';\n", value);
	    } else {
		Printf (f_header, "%s;\n", value);
	    }

	    // Now create a variable declaration

	    {
		/* Hack alert: will cleanup later -- Dave */
		Node *n = NewHash();
		Setattr(n,"name",var_name);
		Setattr(n,"sym:name",iname);
		Setattr(n,"type", type);
		Setattr(n,"feature:immutable","true");
		variableWrapper(n);
		Delete(n);
	    }
	}
	Delete(rvalue);
	Delete(temp);
	return SWIG_OK;
    }

    void
    import_start(char *modname) {
    }

    void 
    import_end() {
    }
};

extern "C" Language *swig_ocaml(void) {
    return new OCAML();
}
