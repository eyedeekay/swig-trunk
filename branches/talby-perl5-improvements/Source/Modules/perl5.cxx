/* -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
 *  vim:expandtab:shiftwidth=2:tabstop=8:smarttab:
 */

/* ----------------------------------------------------------------------------
 * See the LICENSE file for information on copyright, usage and redistribution
 * of SWIG, and the README file for authors - http://www.swig.org/release.html.
 *
 * perl5.cxx
 *
 * Perl5 language module for SWIG.
 * ------------------------------------------------------------------------- */

char cvsroot_perl5_cxx[] = "$Id$";

#include "swigmod.h"
#include "cparse.h"
static int treduce = SWIG_cparse_template_reduce(0);

#include <ctype.h>

static const char *usage = (char *) "\
Perl5 Options (available with -perl5)\n\
     -static         - Omit code related to dynamic loading\n\
     -nopm           - Do not generate the .pm file\n\
     -proxy          - Create proxy classes\n\
     -noproxy        - Don't create proxy classes\n\
     -const          - Wrap constants as constants and not variables (implies -proxy)\n\
     -nocppcast      - Disable C++ casting operators, useful for generating bugs\n\
     -cppcast        - Enable C++ casting operators\n\
     -compat         - Compatibility mode\n\n";

static int compat = 0;

static int no_pmfile = 0;

static int export_all = 0;

/*
 * pmfile
 *   set by the -pm flag, overrides the name of the .pm file
 */
static String *pmfile = 0;

/*
 * module
 *   set by the %module directive, e.g. "Xerces". It will determine
 *   the name of the .pm file, and the dynamic library, and the name
 *   used by any module wanting to %import the module.
 */
static String *module = 0;

/*
 * namespace_module
 *   the fully namespace qualified name of the module. It will be used
 *   to set the package namespace in the .pm file, as well as the name
 *   of the initialization methods in the glue library. This will be
 *   the same as module, above, unless the %module directive is given
 *   the 'package' option, e.g. %module(package="Foo::Bar") "baz"
 */
static String       *namespace_module = 0;

/*
 * dest_package
 *   an optional namespace to put all classes into. Specified by using
 *   the %module(package="Foo::Bar") "baz" syntax
 */
static String       *dest_package = 0;

static String *constant_tab = 0;

static File *f_begin = 0;
static File *f_runtime = 0;
static File *f_header = 0;
static File *f_wrappers = 0;
static File *f_init = 0;
static File *f_pm = 0;
static String *pm;		/* Package initialization code */

static int staticoption = 0;

// controlling verbose output
static int          verbose = 0;

/* The following variables are used to manage Perl5 classes */

static int blessed = 1;		/* Enable object oriented features */
static int do_constants = 0;	/* Constant wrapping */
static List *classlist = 0;	/* List of classes */

static Node *CurrentClass = 0;    /* class currently being processed */
#define ClassName       Getattr(CurrentClass, "sym:name")
static Hash *mangle_seen = 0;

static String *pcode = 0;	/* Perl code associated with each class */
						  /* static  String   *blessedmembers = 0;     *//* Member data associated with each class */
static String *const_stubs = 0;	/* Constant stubs */
static int num_consts = 0;	/* Number of constants */
static String *var_stubs = 0;	/* Variable stubs */
static String *exported = 0;	/* Exported symbols */
static String *pragma_include = 0;
static String *additional_perl_code = 0;	/* Additional Perl code from %perlcode %{ ... %} */

static Hash *operators;
static void add_operator(const char *name, const char *oper, const char *impl) {
  if (!operators) operators = NewHash();
  Hash *elt = NewHash();
  Setattr(elt, "name", name);
  Setattr(elt, "oper", oper);
  Setattr(elt, "impl", impl);
  Setattr(operators, name, elt);
}

class PERL5:public Language {
public:

  PERL5():Language () {
    Clear(argc_template_string);
    Printv(argc_template_string, "items", NIL);
    Clear(argv_template_string);
    Printv(argv_template_string, "ST(%d)", NIL);
    /* will need to evaluate the viability of MI directors later */
    director_multiple_inheritance = 0;
    director_language = 1;
    director_prot_ctor_code = Copy(director_ctor_code);
    Replaceall(director_prot_ctor_code, "$nondirector_new",
        "SWIG_croak(\"$symname is not a public method\");");
    add_operator("__eq__", "==",
        "sub { $_[0]->__eq__($_[1]) }");
    add_operator("__ne__", "!=",
        "sub { $_[0]->__ne__($_[1]) }");
    add_operator("__assign__", "=",
        "sub { $_[0]->__assign__($_[1]) }"); // untested
    add_operator("__str__", "\"\"",
        "sub { $_[0]->__str__() }");
    add_operator("__plusplus__", "++",
        "sub { $_[0]->__plusplus__() }");
    add_operator("__minmin__", "--",
        "sub { $_[0]->__minmin__() }");
    add_operator("__add__", "+",
        "sub { $_[0]->__add__($_[1]) }");
    add_operator("__sub__", "-",
        "sub {\n"
        "    return $_[0]->__sub__($_[1]) unless $_[2];\n"
        "    return $_[0]->__rsub__($_[1]) if $_[0]->can('__rsub__');\n"
        "    die(\"reverse subtraction not supported\");\n"
        "  }");
    add_operator("__mul__", "*",
        "sub { $_[0]->__mul__($_[1]) }");
    add_operator("__div__", "/",
        "sub { $_[0]->__div__($_[1]) }");
    add_operator("__mod__", "%",
        "sub { $_[0]->__mod__($_[1]) }");
    add_operator("__and__", "&",
        "sub { $_[0]->__and__($_[1]) }"); // untested
    add_operator("__or__", "|",
        "sub { $_[0]->__or__($_[1]) }"); // untested
    add_operator("__gt__", ">",
        "sub { $_[0]->__gt__($_[1]) }");
    add_operator("__ge__", ">=",
        "sub { $_[0]->__ge__($_[1]) }");
    add_operator("__not__", "!",
        "sub { $_[0]->__not__() }");
    add_operator("__lt__", "<",
        "sub { $_[0]->__lt__($_[1]) }");
    add_operator("__le__", "<=",
        "sub { $_[0]->__le__($_[1]) }");
    add_operator("__pluseq__",
        "+=", "sub { $_[0]->__pluseq__($_[1]) }");
    add_operator("__mineq__",
        "-=", "sub { $_[0]->__mineq__($_[1]) }");
    add_operator("__neg__",
        "neg",  "sub { $_[0]->__neg__() }");
    add_operator("__deref__",
        "${}", "sub { \\ $_[0]->__deref__() }"); // smart pointer unwrap
  }
  ~PERL5() {
    Delete(director_prot_ctor_code);
    Delete(operators);
  }

  /* Test to see if a type corresponds to something wrapped with a shadow class */
  Node *is_shadow(SwigType *t) {
    Node *n;
    n = classLookup(t);
    /*  Printf(stdout,"'%s' --> '%x'\n", t, n); */
    if (n) {
      if (!Getattr(n, "perl5:proxy")) {
	setclassname(n);
      }
      return Getattr(n, "perl5:proxy");
    }
    return 0;
  }
  /* Test to see if a type corresponds to a director handled class */
  Node *is_directortype(SwigType *t) {
    Node *n = classLookup(t);
    if(!n) return 0;
    return Swig_directorclass(n) ? n : 0;
  }

  /* ------------------------------------------------------------
   * main()
   * ------------------------------------------------------------ */

  virtual void main(int argc, char *argv[]) {
    int i = 1;
    int cppcast = 1;

    SWIG_library_directory("perl5");

    for (i = 1; i < argc; i++) {
      if (argv[i]) {
	if (strcmp(argv[i], "-package") == 0) {
	  Printv(stderr,
		 "*** -package is no longer supported\n*** use the directive '%module A::B::C' in your interface file instead\n*** see the Perl section in the manual for details.\n", NIL);
	  SWIG_exit(EXIT_FAILURE);
	} else if (strcmp(argv[i], "-interface") == 0) {
	  Printv(stderr,
		 "*** -interface is no longer supported\n*** use the directive '%module A::B::C' in your interface file instead\n*** see the Perl section in the manual for details.\n", NIL);
	  SWIG_exit(EXIT_FAILURE);
	} else if (strcmp(argv[i], "-exportall") == 0) {
	  export_all = 1;
	  Swig_mark_arg(i);
	} else if (strcmp(argv[i], "-static") == 0) {
	  staticoption = 1;
	  Swig_mark_arg(i);
	} else if ((strcmp(argv[i], "-shadow") == 0) || ((strcmp(argv[i], "-proxy") == 0))) {
	  blessed = 1;
	  Swig_mark_arg(i);
	} else if ((strcmp(argv[i], "-noproxy") == 0)) {
	  blessed = 0;
	  Swig_mark_arg(i);
	} else if (strcmp(argv[i], "-const") == 0) {
	  do_constants = 1;
	  blessed = 1;
	  Swig_mark_arg(i);
	} else if (strcmp(argv[i], "-nopm") == 0) {
	  no_pmfile = 1;
	  Swig_mark_arg(i);
	} else if (strcmp(argv[i], "-pm") == 0) {
	  Swig_mark_arg(i);
	  i++;
	  pmfile = NewString(argv[i]);
	  Swig_mark_arg(i);
	} else if (strcmp(argv[i],"-v") == 0) {
	    Swig_mark_arg(i);
	    verbose++;
	} else if (strcmp(argv[i], "-cppcast") == 0) {
	  cppcast = 1;
	  Swig_mark_arg(i);
	} else if (strcmp(argv[i], "-nocppcast") == 0) {
	  cppcast = 0;
	  Swig_mark_arg(i);
	} else if (strcmp(argv[i], "-compat") == 0) {
	  compat = 1;
	  Swig_mark_arg(i);
	} else if (strcmp(argv[i], "-help") == 0) {
	  fputs(usage, stdout);
	}
      }
    }

    if (cppcast) {
      Preprocessor_define((DOH *) "SWIG_CPLUSPLUS_CAST", 0);
    }

    Preprocessor_define("SWIGPERL 1", 0);
    // SWIGPERL5 is deprecated, and no longer documented.
    Preprocessor_define("SWIGPERL5 1", 0);
    SWIG_typemap_lang("perl5");
    SWIG_config_file("perl5.swg");
    allow_overloading();
  }

  /* ------------------------------------------------------------
   * top()
   * ------------------------------------------------------------ */

  virtual int top(Node *n) {

    /* Initialize all of the output files */
    String *outfile = Getattr(n, "outfile");
    String *f_director = NewString("");
    String *f_director_h = NewString("");

    f_begin = NewFile(outfile, "w", SWIG_output_files());
    if (!f_begin) {
      FileErrorDisplay(outfile);
      SWIG_exit(EXIT_FAILURE);
    }
    f_runtime = NewString("");
    f_init = NewString("");
    f_header = NewString("");
    f_wrappers = NewString("");

    /* Register file targets with the SWIG file handler */
    Swig_register_filebyname("header", f_header);
    Swig_register_filebyname("wrapper", f_wrappers);
    Swig_register_filebyname("begin", f_begin);
    Swig_register_filebyname("runtime", f_runtime);
    Swig_register_filebyname("init", f_init);
    Swig_register_filebyname("director", f_director);
    Swig_register_filebyname("director_h", f_director_h);

    classlist = NewList();

    pm = NewString("");
    var_stubs = NewString("");
    const_stubs = NewString("");
    exported = NewString("");
    pragma_include = NewString("");
    additional_perl_code = NewString("");

    constant_tab = NewString("static swig_constant_info swig_constants[] = {\n");

    Swig_banner(f_begin);

    Printf(f_runtime, "\n");
    Printf(f_runtime, "#define SWIGPERL\n");
    Printf(f_runtime, "#define SWIG_CASTRANK_MODE\n");
    Printf(f_runtime, "\n");

    // Is the imported module in another package?  (IOW, does it use the
    // %module(package="name") option and it's different than the package
    // of this module.)
    Node *mod = Getattr(n, "module");
    Node *options = Getattr(mod, "options");
    module = Copy(Getattr(n,"name"));

    if (verbose > 0) {
      fprintf(stdout, "top: using module: %s\n", Char(module));
    }

    dest_package = options ? Getattr(options, "package") : 0;
    if (dest_package) {
      namespace_module = Copy(dest_package);
      if (verbose > 0) {
	fprintf(stdout, "top: Found package: %s\n",Char(dest_package));
      }
    } else {
      namespace_module = Copy(module);
      if (verbose > 0) {
	fprintf(stdout, "top: No package found\n");
      }
    }
    String *underscore_module = Copy(module);
    Replaceall(underscore_module,":","_");

    if (verbose > 0) {
      fprintf(stdout, "top: using namespace_module: %s\n", Char(namespace_module));
    }

    if (options) {
      if (Getattr(options, "directors"))
        allow_directors();
      if (Getattr(options, "dirprot"))
        allow_dirprot();
    }
    if (directorsEnabled()) {
      Append(f_runtime, "#define SWIG_DIRECTORS\n");
      Swig_banner(f_director_h);
      Append(f_director,
          "/* ---------------------------------------------------\n"
          " * C++ director class methods\n"
          " * --------------------------------------------------- */\n\n");
    }

    /* Create a .pm file
     * Need to strip off any prefixes that might be found in
     * the module name */

    if (no_pmfile) {
      f_pm = NewString(0);
    } else {
      if (pmfile == NULL) {
	char *m = Char(module) + Len(module);
	while (m != Char(module)) {
	  if (*m == ':') {
	    m++;
	    break;
	  }
	  m--;
	}
	pmfile = NewStringf("%s.pm", m);
      }
      String *filen = NewStringf("%s%s", SWIG_output_directory(), pmfile);
      if ((f_pm = NewFile(filen, "w", SWIG_output_files())) == 0) {
	FileErrorDisplay(filen);
	SWIG_exit(EXIT_FAILURE);
      }
      Delete(filen);
      filen = NULL;
      Swig_register_filebyname("pm", f_pm);
      Swig_register_filebyname("perl", f_pm);
    }
    {
      String *boot_name = NewStringf("boot_%s", underscore_module);
      Printf(f_header,"#define SWIG_init    %s\n\n", boot_name);
      Printf(f_header,"#define SWIG_name   \"%s::%s\"\n", module, boot_name);
      Printf(f_header,"#define SWIG_prefix \"%s::\"\n", module);
      Delete(boot_name);
    }

    Swig_banner_target_lang(f_pm, "#");
    Printf(f_pm, "\n");

    Printf(f_pm, "package %s;\n", module);

    /* 
     * If the package option has been given we are placing our
     *   symbols into some other packages namespace, so we do not
     *   mess with @ISA or require for that package
     */
    if (dest_package) {
      Printf(f_pm,"use base qw(DynaLoader);\n");
    } else {
      Printf(f_pm,"use base qw(Exporter);\n");
      if (!staticoption) {
	Printf(f_pm,"use base qw(DynaLoader);\n");
      }
    }

    Printf(f_wrappers, "#ifdef __cplusplus\nextern \"C\" {\n#endif\n");

    /* emit wrappers */
    Language::top(n);

    /* TODO: shouldn't perlrun.swg handle this itself? */
    if (directorsEnabled())
      Swig_insert_file("director.swg", f_runtime);

    /* Dump out variable wrappers */

    SwigType_emit_type_table(f_runtime, f_wrappers);

    Printf(constant_tab, "{0,0,0,0,0,0}\n};\n");
    Printv(f_wrappers, constant_tab, NIL);

    Printf(f_wrappers, "#ifdef __cplusplus\n}\n#endif\n");

    Printf(f_init, "\t ST(0) = &PL_sv_yes;\n");
    Printf(f_init, "\t XSRETURN(1);\n");
    Printf(f_init, "}\n");

    if (!staticoption) {
      Printf(f_pm,"bootstrap %s;\n", module);
    } else {
      Printf(f_pm,"boot_%s();\n", underscore_module);
    }

    /* 
     * If the package option has been given we are placing our
     *   symbols into some other packages namespace, so we do not
     *   mess with @EXPORT
     */
    if (!dest_package) {
      Printf(f_pm,"@EXPORT = qw(%s);\n", exported);
    }

    Printf(f_pm, "%s", pragma_include);

    if (blessed) {

      /* Emit package code for different classes */
      Printf(f_pm, "%s", pm);

      if (num_consts > 0) {
	/* Emit constant stubs */
	Printf(f_pm, "\n# ------- CONSTANT STUBS -------\n\n");
	Printf(f_pm, "package %s;\n\n", namespace_module);
	Printf(f_pm, "%s", const_stubs);
      }

      /* Emit variable stubs */

      Printf(f_pm, "\n# ------- VARIABLE STUBS --------\n\n");
      Printf(f_pm, "package %s;\n\n", namespace_module);
      Printf(f_pm, "%s", var_stubs);
    }

    /* Add additional Perl code at the end */
    Printf(f_pm, "%s", additional_perl_code);

    Printf(f_pm, "1;\n");
    Close(f_pm);
    Delete(f_pm);
    Delete(dest_package);
    Delete(underscore_module);

    /* Close all of the files */
    Dump(f_runtime, f_begin);
    Dump(f_header, f_begin);
    if (directorsEnabled()) {
      Dump(f_director_h, f_begin);
      Dump(f_director, f_begin);
      Delete(f_director_h);
      Delete(f_director);
      f_director_h = 0; 
      f_director = 0;
    }
    Dump(f_wrappers, f_begin);
    Wrapper_pretty_print(f_init, f_begin);
    Delete(f_header);
    Delete(f_wrappers);
    Delete(f_init);
    Close(f_begin);
    Delete(f_runtime);
    Delete(f_begin);
    return SWIG_OK;
  }

  /* ------------------------------------------------------------
   * importDirective(Node *n)
   * ------------------------------------------------------------ */

  virtual int importDirective(Node *n) {
    if (blessed) {
      String *modname = Getattr(n, "module");
      if (modname) {
	Printf(f_pm, "require %s;\n", modname);
      }
    }
    return Language::importDirective(n);
  }

  /* ------------------------------------------------------------
   * functionWrapper()
   * ------------------------------------------------------------ */

  virtual int functionWrapper(Node *n) {
    String *name = Getattr(n, "name");
    String *iname = Getattr(n, "sym:name");
    String *pname = Getattr(n, "perl5:name");
    SwigType *d = Getattr(n, "type");
    ParmList *l = Getattr(n, "parms");
    String *overname = 0;

    Parm *p;
    int i;
    Wrapper *f;
    char source[256], temp[256];
    String *tm;
    String *cleanup, *outarg;
    int num_saved = 0;
    int num_arguments, num_required;
    int varargs = 0;

    if (Getattr(n, "sym:overloaded")) {
      overname = Getattr(n, "sym:overname");
    } else {
      if (!addSymbol(iname, n))
	return SWIG_ERROR;
    }
    if (!pname) {
      /* TODO: this is kind of a hack because we don't have much control
       * over the stuff in naming.c from here, and
       * Language::classDirectorDisown() constructs the node to pass
       * here internally.
       * additionally, it doesn't assert "hidden" on it's self param,
       * and that musses our usage_func(), so let's fix that.
       */
      if(CurrentClass) {
        String *tmp = NewStringf("disown_%s", ClassName);
        if(Equal(iname, tmp)) {
          pname = NewStringf("%s::_swig_disown", ClassName);
          Setattr(n, "perl5:name", NewStringf("%s::disown", ClassName));
          Setattr(Getattr(n, "parms"), "hidden", "1");
        }
        Delete(tmp);
      }
    }

    if (!pname)
      pname = iname;

    f = NewWrapper();

    String *wname = Swig_name_wrapper(iname);
    if (overname) {
      Append(wname, overname);
    }
    Setattr(n, "wrap:name", wname);
    if (GetFlag(n, "perl5:destructor")) {
      Printv(f->def,
          "SWIGCLASS_STATIC void ", wname, "(swig_perl_wrap *wrap) {\n",
          NIL);
      emit_parameter_variables(l, f);
      emit_return_variable(n, d, f);
      emit_attach_parmmaps(l, f);
      tm = Getattr(l, "tmap:in");
      Replaceall(tm, "$input", "wrap");
      Replaceall(tm, "$disown", "SWIG_POINTER_WR");
      emit_action_code(n, f->code, tm);
      l = nextSibling(l);
        String *actioncode = emit_action(n);
        Append(f->code, actioncode);

      Append(f->code,
          "fail:\n" /* this actually can't ever fail in this context */
          "  return;\n"
          "}");
      Wrapper_print(f, f_wrappers);
      return SWIG_OK;
    }
    if(GetFlag(n, "perl5:instancevariable")) {
      int addfail = 1;
      Printv(f->def,
          "SWIGCLASS_STATIC int ", wname, "(pTHX_ SV *sv, MAGIC *mg) {\n",
          NIL);
      Wrapper_add_local(f, "argvi", "int argvi = 0");
      emit_parameter_variables(l, f);
      emit_return_variable(n, d, f);
      emit_attach_parmmaps(l, f);

      /* recover self pointer */
      tm = Getattr(l, "tmap:in");
      Replaceall(tm, "$input", "mg");
      Replaceall(tm, "$disown", "SWIG_POINTER_MG");
      emit_action_code(n, f->code, tm);
      l = nextSibling(l);

      if (SwigType_type(d) == T_VOID) {
        Setattr(n, "perl5:setter", wname);
        tm = Getattr(l, "tmap:in");
        if (!tm) {
          Swig_warning(WARN_TYPEMAP_IN_UNDEF, input_file, line_number, "Unable to set an attribute of type %s.\n",
              SwigType_str(Getattr(l, "type"), 0));
          return SWIG_NOWRAP;
        }
        Replaceall(tm, "$input", "sv");
        Replaceall(tm, "$disown", "0"); /* TODO: verify this */
        Swig_director_emit_dynamic_cast(n, f);
        emit_action_code(n, f->code, tm);
        emit_action_code(n, f->code, Getattr(n, "wrap:action"));
        addfail = 1;
      } else {
        Setattr(n, "perl5:getter", wname);
        Swig_director_emit_dynamic_cast(n, f);
        String *actioncode = emit_action(n);
        SwigType *t = Getattr(n, "type");
        tm = Swig_typemap_lookup_out("out", n, "result", f, actioncode);
        if(!tm) {
          Swig_warning(WARN_TYPEMAP_OUT_UNDEF, input_file, line_number, "Unable to get an attribute of type %s.\n",
              SwigType_str(t, 0), name);
          return SWIG_NOWRAP;
        }
        Wrapper_add_local(f, "dest", "SV *dest");
        Replaceall(tm, "$result", "dest");
        //Replaceall(tm, "$source", "result");
        //Replaceall(tm, "$target", "ST(argvi)");
        //Replaceall(tm, "$result", "ST(argvi)");
        Replaceall(tm, "$owner", "0");
        Printv(f->code,
            tm, "\n",
            "SvSetSV(sv, dest);\n", NIL);
      }
      Append(f->code,
          "return 0;\n");
      if (addfail) {
        Append(f->code,
            "fail:\n"
            "croak(Nullch);\n"
            "return 0;\n");
      }
      Append(f->code,
          "}");
      Wrapper_print(f, f_wrappers);
      return SWIG_OK;
    }
    cleanup = NewString("");
    outarg = NewString("");
    Printv(f->def, "XS(", wname, ") {\n", "{\n",	/* scope to destroy C++ objects before croaking */
	   NIL);

    emit_parameter_variables(l, f);
    emit_attach_parmmaps(l, f);
    Setattr(n, "wrap:parms", l);

    num_arguments = emit_num_arguments(l);
    num_required = emit_num_required(l);
    varargs = emit_isvarargs(l);

    Wrapper_add_local(f, "argvi", "int argvi = 0");

    /* Check the number of arguments */
    if (!varargs) {
      Printf(f->code, "    if ((items < %d) || (items > %d)) {\n",
        num_required, num_arguments);
    } else {
      Printf(f->code, "    if (items < %d) {\n",
        num_required);
    }
    {
      String *usage = usage_func(n);
      Printf(f->code,
          "        SWIG_croak(\"Usage: %s\");\n"
          "}\n", usage);
      Delete(usage);
    }

    /* Write code to extract parameters. */
    i = 0;
    for (i = 0, p = l; i < num_arguments; i++) {

      /* Skip ignored arguments */

      while (checkAttribute(p, "tmap:in:numinputs", "0")) {
	p = Getattr(p, "tmap:in:next");
      }

      SwigType *pt = Getattr(p, "type");

      /* Produce string representation of source and target arguments */
      sprintf(source, "ST(%d)", i);
      String *target = Getattr(p, "lname");

      if (i >= num_required) {
	Printf(f->code, "    if (items > %d) {\n", i);
      }
      if ((tm = Getattr(p, "tmap:in"))) {
	Replaceall(tm, "$target", target);
	Replaceall(tm, "$source", source);
	Replaceall(tm, "$input", source);
	Setattr(p, "emit:input", source);	/* Save input location */

	if (Getattr(p, "wrap:disown") || (Getattr(p, "tmap:in:disown"))) {
	  Replaceall(tm, "$disown", "SWIG_POINTER_DISOWN");
	} else {
	  Replaceall(tm, "$disown", "0");
	}

	Printf(f->code, "%s\n", tm);
	p = Getattr(p, "tmap:in:next");
      } else {
	Swig_warning(WARN_TYPEMAP_IN_UNDEF, input_file, line_number, "Unable to use type %s as a function argument.\n", SwigType_str(pt, 0));
	p = nextSibling(p);
      }
      if (i >= num_required) {
	Printf(f->code, "    }\n");
      }
    }

    if (varargs) {
      if (p && (tm = Getattr(p, "tmap:in"))) {
	sprintf(source, "ST(%d)", i);
	Replaceall(tm, "$input", source);
	Setattr(p, "emit:input", source);
	Printf(f->code, "if (items >= %d) {\n", i);
	Printv(f->code, tm, "\n", NIL);
	Printf(f->code, "}\n");
      }
    }

    /* Insert constraint checking code */
    for (p = l; p;) {
      if ((tm = Getattr(p, "tmap:check"))) {
	Replaceall(tm, "$target", Getattr(p, "lname"));
	Printv(f->code, tm, "\n", NIL);
	p = Getattr(p, "tmap:check:next");
      } else {
	p = nextSibling(p);
      }
    }

    /* Insert cleanup code */
    for (i = 0, p = l; p; i++) {
      if ((tm = Getattr(p, "tmap:freearg"))) {
	Replaceall(tm, "$source", Getattr(p, "lname"));
	Replaceall(tm, "$arg", Getattr(p, "emit:input"));
	Replaceall(tm, "$input", Getattr(p, "emit:input"));
	Printv(cleanup, tm, "\n", NIL);
	p = Getattr(p, "tmap:freearg:next");
      } else {
	p = nextSibling(p);
      }
    }

    /* Insert argument output code */
    num_saved = 0;
    for (i = 0, p = l; p; i++) {
      if ((tm = Getattr(p, "tmap:argout"))) {
	Replaceall(tm, "$source", Getattr(p, "lname"));
	Replaceall(tm, "$target", "ST(argvi)");
	Replaceall(tm, "$result", "ST(argvi)");

	String *in = Getattr(p, "emit:input");
	if (in) {
	  sprintf(temp, "_saved[%d]", num_saved);
	  Replaceall(tm, "$arg", temp);
	  Replaceall(tm, "$input", temp);
	  Printf(f->code, "_saved[%d] = %s;\n", num_saved, in);
	  num_saved++;
	}
	Printv(outarg, tm, "\n", NIL);
	p = Getattr(p, "tmap:argout:next");
      } else {
	p = nextSibling(p);
      }
    }

    /* If there were any saved arguments, emit a local variable for them */
    if (num_saved) {
      sprintf(temp, "_saved[%d]", num_saved);
      Wrapper_add_localv(f, "_saved", "SV *", temp, NIL);
    }

    if (is_member_director(n) && !is_smart_pointer()) {
      Wrapper_add_local(f, "upcall", "bool upcall");
      Append(f->code,
          "{\n"
          "  Swig::Director *director = dynamic_cast<Swig::Director *>(arg1);\n"
          "  if (director) {\n"
          "    HV *outer = SvSTASH(SvRV(ST(0)));\n"
          "    HV *inner = SvSTASH(SvRV(director->getSelf()));\n"
          "    upcall = outer == inner;\n"
          "  } else {\n");
      if(dirprot_mode() && !is_public(n)) {
        Append(f->code,
            "    SWIG_croak(\"accessing protected member\");\n");
      } else {
        Append(f->code,
            "    upcall = false;\n");
      }
      Append(f->code,
          "  }\n"
          "}\n"
          "try {\n");
    }

    /* Now write code to make the function call */

    Swig_director_emit_dynamic_cast(n, f);
    String *actioncode = emit_action(n);

    if ((tm = Swig_typemap_lookup_out("out", n, "result", f, actioncode))) {
      Replaceall(tm, "$source", "result");
      Replaceall(tm, "$target", "ST(argvi)");
      Replaceall(tm, "$result", "ST(argvi)");
      Replaceall(tm, "$owner", GetFlag(n, "feature:new") ?
          "SWIG_POINTER_OWN" : "0");
      /* TODO: this NewPointerObjP stuff is a hack */
      if (blessed && Equal(nodeType(n), "constructor")) {
        Replaceall(tm, "NewPointerObj", "NewPointerObjP");
      }
      Printf(f->code, "%s\n", tm);
    } else {
      Swig_warning(WARN_TYPEMAP_OUT_UNDEF, input_file, line_number, "Unable to use return type %s in function %s.\n", SwigType_str(d, 0), name);
    }
    if (is_member_director(n) && !is_smart_pointer()) {
      Append(f->code,
          "} catch (Swig::DirectorException &e) {\n"
          "  SvSetSV(ERRSV, e.sv);\n"
          "  SWIG_fail;\n"
          "}\n");
    }
    emit_return_variable(n, d, f);

    /* If there were any output args, take care of them. */

    Printv(f->code, outarg, NIL);

    /* If there was any cleanup, do that. */

    Printv(f->code, cleanup, NIL);

    if (GetFlag(n, "feature:new")) {
      if ((tm = Swig_typemap_lookup("newfree", n, "result", 0))) {
	Replaceall(tm, "$source", "result");
	Printf(f->code, "%s\n", tm);
      }
    }

    if ((tm = Swig_typemap_lookup("ret", n, "result", 0))) {
      Replaceall(tm, "$source", "result");
      Printf(f->code, "%s\n", tm);
    }

    if(GetFlag(n, "feature:new") && is_directortype(d)) {
      Append(f->code,
          "  {\n"
          "    Swig::Director *director = dynamic_cast<Swig::Director *>(result);\n"
          "    if(director) director->setSelf(ST(0));\n"
          "  }\n");
    }

    Printv(f->code, "XSRETURN(argvi);\n", "fail:\n", cleanup, "SWIG_croak_null();\n" "}\n" "}\n", NIL);

    /* Add the dXSARGS last */

    Wrapper_add_local(f, "dXSARGS", "dXSARGS");

    /* Substitute the cleanup code */
    Replaceall(f->code, "$cleanup", cleanup);
    Replaceall(f->code, "$symname", iname);

    /* Dump the wrapper function */

    Wrapper_print(f, f_wrappers);

    /* Now register the function */

    if (!Getattr(n, "sym:overloaded")) {
      /* TODO: would be very cool to use newXSproto here, but for now
       * let's just make it work using the current call signature
       * checks. */
      Printf(f_init, "newXS(\"%s::%s\", %s, __FILE__);\n",
          namespace_module, pname, wname);
    } else if (!Getattr(n, "sym:nextSibling")) {
      /* Generate overloaded dispatch function */
      int maxargs;
      String *dispatch = Swig_overload_dispatch_cast(n, "++PL_markstack_ptr; SWIG_CALLXS(%s); return;", &maxargs);

      /* Generate a dispatch wrapper for all overloaded functions */

      Wrapper *df = NewWrapper();
      String *dname = Swig_name_wrapper(iname);

      Printv(df->def, "XS(", dname, ") {\n", NIL);

      Wrapper_add_local(df, "dXSARGS", "dXSARGS");
      Printv(df->code, dispatch, "\n", NIL);
      Printf(df->code, "croak(\"No matching function for overloaded '%s'\");\n", pname);
      Printf(df->code, "XSRETURN(0);\n");
      Printv(df->code, "}\n", NIL);
      Wrapper_print(df, f_wrappers);
      Printf(f_init, "newXS(\"%s::%s\", %s, __FILE__);\n",
          namespace_module, pname, dname);
      DelWrapper(df);
      Delete(dispatch);
      Delete(dname);
    }
    if (!Getattr(n, "sym:nextSibling")) {
      if (export_all) {
	Printf(exported, "%s ", iname);
      }
    }
    Delete(cleanup);
    Delete(outarg);
    DelWrapper(f);
    return SWIG_OK;
  }

  /* ------------------------------------------------------------
   * variableWrapper()
   * ------------------------------------------------------------ */
  virtual int variableWrapper(Node *n) {
    String *name = Getattr(n, "name");
    String *iname = Getattr(n, "sym:name");
    String *pname = Getattr(n, "perl5:name");
    String *type = Getattr(n, "type");
    Wrapper *f;
    String *getf, *setf;
    String *tm;
    String *tmp;

    if (!addSymbol(iname, n))
      return SWIG_ERROR;

    if (!pname)
      pname = iname;

    if (is_assignable(n)) {
      /* emit setter */
      f = NewWrapper();

      tmp = Swig_name_set(iname);
      setf = Swig_name_wrapper(tmp);
      Delete(tmp);
      Setattr(n, "wrap:name", setf);
      Printv(f->def,
          "SWIGCLASS_STATIC int ", setf, "(pTHX_ SV *sv, MAGIC *mg) {\n",
          NIL);
      tm = Swig_typemap_lookup("varin", n, name, f);
      if (!tm) {
        Swig_warning(WARN_TYPEMAP_VARIN_UNDEF, input_file, line_number,
            "Unable to set variable of type %s.\n",
            SwigType_str(type, 0));
        return SWIG_NOWRAP;
      }
      Replaceall(tm, "$source", "sv");
      Replaceall(tm, "$target", name);
      Replaceall(tm, "$input", "sv");
      emit_action_code(n, f->code, tm);
      Append(f->code,
          "  return 0;\n"
          "fail:\n"
          "  croak(Nullch);\n"
          "  return 0;\n"
          "}\n");
      Wrapper_print(f, f_wrappers);
      Delete(f);
    } else {
      setf = NewString("0");
    }
    {
      /* emit getter */
      int addfail;
      tmp = Swig_name_get(iname);
      getf = Swig_name_wrapper(tmp);
      Delete(tmp);
      f = NewWrapper();
      Setattr(n, "wrap:name", getf);
      Printv(f->def,
          "SWIGCLASS_STATIC int ", getf, "(pTHX_ SV *sv, MAGIC *mg) {\n",
          NIL);
      tm = Swig_typemap_lookup("varout", n, name, f);
      if (!tm) {
        Swig_warning(WARN_TYPEMAP_VAROUT_UNDEF, input_file, line_number,
            "Unable to read variable of type %s\n",
            SwigType_str(type, 0));
        return SWIG_NOWRAP;
      }
      Replaceall(tm, "$target", "sv");
      Replaceall(tm, "$result", "sv");
      Replaceall(tm, "$source", name);
      addfail = emit_action_code(n, f->code, tm);
      Append(f->code, "  return 0;\n");
      if (addfail) {
        Append(f->code,
            "fail:\n"
            "  croak(Nullch);\n"
            "  return 0;\n");
      }
      Append(f->code, "}\n");
      Wrapper_print(f, f_wrappers);
      Delete(f);
    }

    {
      String *vtbl = NewStringf("_swig_perl_vtbl_%s", iname);

      Printf(f_wrappers,
        "MGVTBL %s = SWIG_Perl_VTBL(%s, %s);\n",
        vtbl, getf, setf);

      Setattr(n, "perl5:vtbl", vtbl);

      Printf(f_init,
          "SWIG_Perl_WrapVar(get_sv(\"%s::%s\", TRUE | GV_ADDMULTI), &%s, 0);\n",
          namespace_module, pname, vtbl);
    }
    return SWIG_OK;
  }

  /* ------------------------------------------------------------
   * constantWrapper()
   * ------------------------------------------------------------ */

  virtual int constantWrapper(Node *n) {
    String *name = Getattr(n, "name");
    String *iname = Getattr(n, "sym:name");
    String *pname = Getattr(n, "perl5:name");
    SwigType *type = Getattr(n, "type");
    String *rawval = Getattr(n, "rawval");
    String *value = rawval ? rawval : Getattr(n, "value");
    String *tm;

    if (!addSymbol(iname, n))
      return SWIG_ERROR;

    if (!pname)
      pname = iname;
    Swig_require("perl5constantWrapper", n, "*sym:name", NIL);
    Setattr(n, "sym:name", pname);

    /* Special hook for member pointer */
    if (SwigType_type(type) == T_MPOINTER) {
      String *wname = Swig_name_wrapper(iname);
      Printf(f_wrappers, "static %s = %s;\n", SwigType_str(type, wname), value);
      value = Char(wname);
    }

    if ((tm = Swig_typemap_lookup("consttab", n, name, 0))) {
      Replaceall(tm, "$source", value);
      Replaceall(tm, "$target", name);
      Replaceall(tm, "$value", value);
      Printf(constant_tab, "%s,\n", tm);
    } else if ((tm = Swig_typemap_lookup("constcode", n, name, 0))) {
      Replaceall(tm, "$source", value);
      Replaceall(tm, "$target", name);
      Replaceall(tm, "$value", value);
      Printf(f_init, "%s\n", tm);
    } else {
      Swig_warning(WARN_TYPEMAP_CONST_UNDEF, input_file, line_number, "Unsupported constant value.\n");
      Swig_restore(n);
      return SWIG_NOWRAP;
    }

    if (blessed) {
      if (is_shadow(type)) {
	Printv(var_stubs,
	       "\nmy %__", iname, "_hash;\n",
	       "tie %__", iname, "_hash,\"", is_shadow(type), "\", $",
	       module, "::", iname, ";\n", "$", iname, "= \\%__", iname, "_hash;\n", "bless $", iname, ", ", is_shadow(type), ";\n", NIL);
      } else if (do_constants) {
	Printv(const_stubs, "sub ", name, " () { $", module, "::", name, " }\n", NIL);
	num_consts++;
      }
    }
    if (export_all) {
      if (do_constants && !is_shadow(type)) {
	Printf(exported, "%s ", name);
      } else {
	Printf(exported, "$%s ", iname);
      }
    }
    Swig_restore(n);
    return SWIG_OK;
  }

  /* ------------------------------------------------------------
   * usage_func()
   * ------------------------------------------------------------ */
  String *usage_func(Node *n) {
    ParmList *p = Getattr(n, "parms");
    String *pcall = 0;
    String *pname = 0;

    pname = Getattr(n, "perl5:name");
    if(!pname) pname = Getattr(n, "sym:name");

    if(p && CurrentClass) {
      if (GetFlag(p, "arg:classref")) {
        /* class method */
        String *src = NewStringf("%s::", ClassName);
        String *dst = NewStringf("%s::%s->",
            namespace_module, ClassName);
        pcall = NewStringf("%s(", pname);
        Replace(pcall, src, dst, DOH_REPLACE_FIRST);
        p = nextSibling(p);
        Delete(src);
        Delete(dst);
      } else if (GetFlag(p, "hidden") && Equal(Getattr(p, "name"), "self")) {
        String *src = NewStringf("%s::", ClassName);
        String *dst = NewStringf("[%s::%s object]->",
            namespace_module, ClassName);
        pcall = NewStringf("%s(", pname);
        Replace(pcall, src, dst, DOH_REPLACE_FIRST);
        p = nextSibling(p);
        Delete(src);
        Delete(dst);
      }
    }
    if (!pcall)
      pcall = NewStringf("%s::%s(", namespace_module, pname);

    /* Now go through and print parameters */
    for(int i = 0; p; p = nextSibling(p)) {
      SwigType *pt = Getattr(p, "type");
      String *pn = Getattr(p, "name");
      if (checkAttribute(p, "tmap:in:numinputs", "0")) continue;

      if (i > 0) Append(pcall, ",");
      /* If parameter has been named, use that.   Otherwise, just print a type  */
      if (SwigType_type(pt) != T_VOID) {
        if (Len(pn) > 0) {
          Append(pcall, pn);
        } else {
          Append(pcall, SwigType_str(pt, 0));
        }
      }
      i = 1;
    }
    Append(pcall, ");");
    return pcall;
  }

  /* ------------------------------------------------------------
   * nativeWrapper()
   * ------------------------------------------------------------ */

  virtual int nativeWrapper(Node *n) {
    String *iname = Getattr(n, "sym:name");
    String *wname = Getattr(n, "wrap:name");

    if (!addSymbol(iname, n))
      return SWIG_ERROR;

    /* %native, used to drop hand coded XS directly into .i files, does
     * not apply to classes, thus no "perl5:name" attribute will ever be
     * present. */
    Printf(f_init, "newXS(\"%s::%s\", %s, __FILE__);\n",
        namespace_module, iname, wname);

    if (export_all)
      Printf(exported, "%s ", iname);
    return SWIG_OK;
  }

/* ----------------------------------------------------------------------------
 *                      OBJECT-ORIENTED FEATURES
 *
 * These extensions provide a more object-oriented interface to C++
 * classes and structures.    The code here is based on extensions
 * provided by David Fletcher and Gary Holt.
 *
 * I have generalized these extensions to make them more general purpose
 * and to resolve object-ownership problems.
 *
 * The approach here is very similar to the Python module :
 *       1.   All of the original methods are placed into a single
 *            package like before except that a 'c' is appended to the
 *            package name.
 *
 *       2.   All methods and function calls are wrapped with a new
 *            perl function.   While possibly inefficient this allows
 *            us to catch complex function arguments (which are hard to
 *            track otherwise).
 *
 *       3.   Classes are represented as tied-hashes in a manner similar
 *            to Gary Holt's extension.   This allows us to access
 *            member data.
 *
 *       4.   Stand-alone (global) C functions are modified to take
 *            tied hashes as arguments for complex datatypes (if
 *            appropriate).
 *
 *       5.   Global variables involving a class/struct is encapsulated
 *            in a tied hash.
 *
 * ------------------------------------------------------------------------- */


  void setclassname(Node *n) {
    String *symname = Getattr(n, "sym:name");
    String *fullname;
    String *actualpackage;
    Node *clsmodule = Getattr(n, "module");

    if (!clsmodule) {
      /* imported module does not define a module name.   Oh well */
      return;
    }

    /* Do some work on the class name */
    if (verbose > 0) {
      String *modulename = Getattr(clsmodule, "name");
      fprintf(stdout, "setclassname: Found sym:name: %s\n", Char(symname));
      fprintf(stdout, "setclassname: Found module: %s\n", Char(modulename));
      fprintf(stdout, "setclassname: No package found\n");
    }

    if (dest_package) {
      fullname = NewStringf("%s::%s", namespace_module, symname);
    } else {
      actualpackage = Getattr(clsmodule,"name");

      if (verbose > 0) {
	fprintf(stdout, "setclassname: Found actualpackage: %s\n", Char(actualpackage));
      }
      if ((!compat) && (!Strchr(symname,':'))) {
	fullname = NewStringf("%s::%s",actualpackage,symname);
      } else {
	fullname = NewString(symname);
      }
    }
    if (verbose > 0) {
      fprintf(stdout, "setclassname: setting proxy: %s\n", Char(fullname));
    }
    Setattr(n, "perl5:proxy", fullname);
  }

  /* ------------------------------------------------------------
   * classDeclaration()
   * ------------------------------------------------------------ */
  virtual int classDeclaration(Node *n) {
    /* Do some work on the class name */
    if (!Getattr(n, "feature:onlychildren")) {
      if (blessed) {
	setclassname(n);
	Append(classlist, n);
      }
    }

    return Language::classDeclaration(n);
  }

  /* ------------------------------------------------------------
   * classHandler()
   * ------------------------------------------------------------ */

  virtual int classHandler(Node *n) {
    String *name = 0; /* Real name of C/C++ class */
    String *fullclassname = 0;
    String *outer_nc = 0;

    Node *outer_class = CurrentClass;
    CurrentClass = n;
    Setattr(n, "perl5:memberVariables", NewList());

    if (blessed) {

      if (!addSymbol(ClassName, n))
	return SWIG_ERROR;

      /* Use the fully qualified name of the Perl class */
      if (!compat) {
	fullclassname = NewStringf("%s::%s", namespace_module, ClassName);
      } else {
	fullclassname = NewString(ClassName);
      }

      outer_nc = none_comparison;
      none_comparison = NewStringf("strNE(SvPV_nolen(ST(0)), \"%s\") && "
          "sv_derived_from(ST(0), \"%s\")", fullclassname, fullclassname);

      name = Getattr(n, "name");
      pcode = NewString("");
      // blessedmembers = NewString("");
    }

    /* Emit all of the members */
    int rv = Language::classHandler(n);
    if(rv != SWIG_OK) return rv;


    /* Finish the rest of the class */
    if (blessed) {
      Printv(pm, "\n",
          "############# Class : ", fullclassname, " ##############\n",
          "package ", fullclassname, ";\n", NIL);

      /* tell interpreter about class inheritance */
      List *baselist = Getattr(n, "bases");
      if (baselist && Len(baselist)) {
        int begun = 0;
        for (Iterator b = First(baselist); b.item; b = Next(b)) {
          String *bname = Getattr(b.item, "perl5:proxy");
          if (!bname)
            continue;
          Printv(pm, (begun ? ", '" : "use base '"), bname, "'", NIL);
          begun = 1;
        }
        if (begun) Append(pm, ";\n");
      }

      /* tell interpreter about class attributes */
      {
        SwigType *ct = Copy(name);
        String *mang;

        SwigType_add_pointer(ct);
        mang = SwigType_manglestr(ct);
        /* mangle_seen is to avoid emitting type information for types
         * SwigType_remember_clientdata does not consider to be
         * different, it's a hack.  I think the problem this workaround
         * is needed for is in typesys/symbol management. */
        if (!mangle_seen) mangle_seen = NewHash();
        if (!Getattr(mangle_seen, mang)) {

        /* declare attribute bindings */
        int nattr = 0;
        int nfield = 0;
        List *bases = Getattr(n, "bases");
        if(bases) bases = Copy(bases);
        else      bases = NewList();
        Insert(bases, 0, n);
        Printv(pm, "use fields (", NIL);
        for (int i = Len(bases); i > 0; i--) {
          Node *iclass = Getitem(bases, i - 1);
          for (Iterator j = First(Getattr(iclass,
              "perl5:memberVariables")); j.item; j = Next(j)) {
            Node *ch = j.item;
            /* XS side */
            String *vtbl = Getattr(ch, "perl5:vtbl");
            if(vtbl) {
              vtbl = Copy(vtbl);
            } else {
              String *getf = Getattr(ch, "perl5:getter");
              if (!getf) continue;
              String *setf = Getattr(ch, "perl5:setter");
              vtbl = NewStringf("SWIG_Perl_VTBL(%s, %s)",
                  getf, setf ? setf : "0");
            }
            String *chn = Getattr(ch, "sym:name");
            if (nattr == 0) {
              Printv(f_wrappers, "static swig_perl_type_ext_var "
                  "_swigt_ext_var_", mang, "[] = {\n", NIL);
            } else {
              Append(f_wrappers, ",\n");
            }
            Printf(f_wrappers,
                "  { \"%s\", %s }", chn, vtbl);
            Delete(vtbl);
            nattr++;
            /* Perl side */
            if (iclass == n || Strncmp(chn, "_", 1) == 0) {
              /* fields.pm handles leading underscore a bit special */
              if (nfield)
                Append(pm, ", ");
              Printf(pm, "'%s'", chn);
              nfield++;
            }
          }
        }
        Append(pm, ");\n");
        {
          Node *destroy = Getattr(n, "perl5:destructors");
          if (nattr) Append(f_wrappers,
            "\n};\n");
          Printf(f_wrappers,
            "static swig_perl_type_ext _swigt_ext_%s = "
            "SWIG_Perl_TypeExt(\"%s\", %s, %d, ",
            mang,
            fullclassname,
            destroy ? Getattr(destroy, "wrap:name") : "0",
            nattr);
          if (nattr) Printf(f_wrappers,
            "_swigt_ext_var_%s);\n\n", mang);
          else Printf(f_wrappers,
            "0);\n\n");
        }
        Delete(bases);
        String *tmp = NewStringf("&_swigt_ext_%s", mang);
        SwigType_remember_clientdata(ct, tmp);
        Setattr(mangle_seen, mang, "1");
        Delete(mang);
        Delete(ct);
      }}

      List *opers = Getattr(n, "perl5:operators");
      if (opers) {
	Printf(pm, "use overload\n");
	for (Iterator ki = First(opers); ki.item; ki = Next(ki)) {
          Printf(pm, "  '%s' => %s,\n",
              Getattr(ki.item, "oper"), Getattr(ki.item, "impl"));
	}
        if (!Getattr(opers, "__assign__"))
          Append(pm, "  '=' => sub { ref($_[0])->new($_[0]) },\n");
	Append(pm, "  'fallback' => 1;\n");
      }

      /* Dump out the package methods */
      Printv(pm, pcode, NIL);
      Delete(pcode);

      /* bind core swig class methods */
      Printf(f_init, "newXS(\"%s::%s::_swig_this\", SWIG_Perl_This, __FILE__);\n",
          namespace_module, ClassName);
      Printf(f_init, "newXS(\"%s::%s::_swig_own\", SWIG_Perl_Own, __FILE__);\n",
          namespace_module, ClassName);

      Delete(none_comparison);
      none_comparison = outer_nc;
    }
    CurrentClass = outer_class; 
    return SWIG_OK;
  }

  /* ------------------------------------------------------------
   * memberfunctionCommon()
   *
   * Just hoisting the common bits of member function wrapping into a
   * common place to ease code consistency and readability
   * ------------------------------------------------------------ */
  virtual int memberfunctionCommon(Node *n, int shadow = 1) {
    if (blessed) {
      String *symname = Getattr(n, "sym:name");
      String *pname;
      String *pfunc = 0;

      if(shadow)
        pfunc = Getattr(n, "feature:shadow");
      pname = NewStringf("%s%s",
          pfunc ? "_swig_" : "",
          Equal(nodeType(n), "constructor") &&
            Equal(symname, ClassName) ? "new" : symname);
      if(pfunc) {
        String *pname_ref = NewStringf("do { \\&%s }", pname);
        Replaceall(pfunc, "$action", pname_ref);
        Delete(pname_ref);
        Append(pcode, pfunc);
      }
      Setattr(n, "perl5:name", NewStringf("%s::%s", ClassName, pname));
    }
    return SWIG_OK;
  }

  /* ------------------------------------------------------------
   * memberfunctionHandler()
   * ------------------------------------------------------------ */

  virtual int memberfunctionHandler(Node *n) {
    memberfunctionCommon(n);
    if (blessed && !Getattr(n, "sym:nextSibling")) {
      String *symname = Getattr(n, "sym:name");

      Hash *oper = Getattr(operators, symname);
      if (oper) {
        List *class_oper = Getattr(CurrentClass, "perl5:operators");
        if (!class_oper) {
          class_oper = NewList();
          Setattr(CurrentClass, "perl5:operators", class_oper);
        }
        Append(class_oper, oper);
      }
    }
    return Language::memberfunctionHandler(n);
  }

  /* ------------------------------------------------------------
   * membervariableHandler()
   *
   * Adds an instance member.
   * ----------------------------------------------------------------------------- */

  virtual int membervariableHandler(Node *n) {
    Append(Getattr(CurrentClass, "perl5:memberVariables"), n);
    Setattr(n, "perl5:instancevariable", "1");
    return Language::membervariableHandler(n);
  }

  /* ------------------------------------------------------------
   * constructorDeclaration()
   *
   * Emits a blessed constructor for our class.    In addition to our construct
   * we manage a Perl hash table containing all of the pointers created by
   * the constructor.   This prevents us from accidentally trying to free
   * something that wasn't necessarily allocated by malloc or new
   * ------------------------------------------------------------ */

  virtual int constructorHandler(Node *n) {
    int restore = 0;
    memberfunctionCommon(n);
    if (blessed && !Swig_directorclass(CurrentClass)) {
      Swig_save("perl5memberfunctionimplicit", n, "parms", NIL);
      String *type = NewString("SV");
      SwigType_add_pointer(type);
      Parm *p = NewParm(type, "proto", n);
      Delete(type);
      Setattr(p, "arg:classref", "1");
      set_nextSibling(p, Getattr(n, "parms"));
      Setattr(n, "parms", p);
      restore = 1;
    }
    int rv = Language::constructorHandler(n);
    if (restore) Swig_restore(n);
    return rv;
  }

  /* ------------------------------------------------------------ 
   * destructorHandler()
   * ------------------------------------------------------------ */

  virtual int destructorHandler(Node *n) {
    memberfunctionCommon(n);
    Setattr(CurrentClass, "perl5:destructors", n);
    Setattr(n, "perl5:destructor", "1");
    return Language::destructorHandler(n);
  }

  /* ------------------------------------------------------------
   * staticmemberfunctionHandler()
   * ------------------------------------------------------------ */

  virtual int staticmemberfunctionHandler(Node *n) {
    int restore = 0;
    memberfunctionCommon(n);
    if (blessed && !GetFlag(n, "allocate:smartpointeraccess")) {
      Swig_save("perl5memberfunctionimplicit", n, "parms", NIL);
      String *type = NewString("SV");
      SwigType_add_pointer(type);
      Parm *p = NewParm(type, "proto", n);
      Delete(type);
      Setattr(p, "arg:classref", "1");
      set_nextSibling(p, Getattr(n, "parms"));
      Setattr(n, "parms", p);
      restore = 1;
    }
    int rv = Language::staticmemberfunctionHandler(n);
    if (restore) Swig_restore(n);
    return rv;
  }

  /* ------------------------------------------------------------
   * staticmembervariableHandler()
   * ------------------------------------------------------------ */

  virtual int staticmembervariableHandler(Node *n) {
    memberfunctionCommon(n, 0);
    Append(Getattr(CurrentClass, "perl5:memberVariables"), n);
    return Language::staticmembervariableHandler(n);
  }

  /* ------------------------------------------------------------
   * memberconstantHandler()
   * ------------------------------------------------------------ */

  virtual int memberconstantHandler(Node *n) {
    memberfunctionCommon(n, 0);
    return Language::memberconstantHandler(n);
  }

  /* ------------------------------------------------------------
   * a (sloppy) crack at directors
   * ------------------------------------------------------------ */
  virtual int classDirector(Node *n) {
    return Language::classDirector(n);
  }
  virtual int classDirectorInit(Node *n) {
    String *decl = Swig_director_declaration(n);
    Printv(Swig_filebyname("director_h"), decl, "\n  public:\n", NIL);
    Delete(decl);
    return Language::classDirectorInit(n);
  }
  virtual int classDirectorConstructors(Node *n) {
    return Language::classDirectorConstructors(n);
  }
  virtual int classDirectorConstructor(Node *n) {
    ParmList *parms = Getattr(n, "parms");
    String *mdecl = NewStringf("");
    String *mdefn = NewStringf("");
    String *signature;
    String *name = NewStringf("SwigDirector_%s",
        Getattr(parentNode(n), "sym:name"));

    { /* resolve the function signature */
      String *type = NewString("SV");
      SwigType_add_pointer(type);
      Parm *p = NewParm(type, "proto", n);
      Delete(type);
      Setattr(p, "arg:classref", "1");
      set_nextSibling(p, Getattr(n, "parms"));
      Setattr(n, "parms", p);
      signature = Swig_method_decl(Getattr(n, "type"),
          Getattr(n, "decl"), "$name", p, 0, 0);
    }
    { /* prep method decl */
      String *target = Copy(signature);
      Replaceall(target, "$name", name);
      Printf(mdecl, "    %s;\n", target);
      Delete(target);
    }
    { /* prep method defn */
      String *qname;
      String *target;
      String *scall;
      qname = NewStringf("%s::%s", name, name);
      target = Copy(signature);
      Replaceall(target, "$name", qname);
      Delete(qname);
      scall = Swig_csuperclass_call(0,
          Getattr(parentNode(n), "classtype"), parms);
      Printf(mdefn,
          "%s : %s, Swig::Director() {\n"
          "}\n",
          target, scall);
      Delete(target);
      Delete(scall);
    }
    Dump(mdecl, Swig_filebyname("director_h"));
    Dump(mdefn, Swig_filebyname("director"));
    Delete(signature);
    Delete(mdecl);
    Delete(mdefn);
    return Language::classDirectorConstructor(n);
  }
  virtual int classDirectorDefaultConstructor(Node *n) {
    return Language::classDirectorDefaultConstructor(n);
  }
  virtual int classDirectorDestructor(Node *n) {
    return Language::classDirectorDestructor(n);
  }
  virtual int classDirectorMethods(Node *n) {
    return Language::classDirectorMethods(n);
  }
  virtual int classDirectorMethod(Node *n, Node *parent, String *super) {
    SwigType *type = Getattr(n, "type");
    String *decl = Getattr(n, "decl");
    String *name = Getattr(n, "name");
    ParmList *parms = Getattr(n, "parms");
    String *mdecl = NewStringf("");
    String *mdefn = NewStringf("");
    String *signature;
    bool output_director = true;

    { /* resolve the function signature */
      SwigType *ret_type = Getattr(n, "conversion_operator") ? NULL : type;
      signature = Swig_method_decl(ret_type, decl, "$name", parms, 0, 0);
      if(Getattr(n, "throw")) { /* prep throws() fragment */
        Parm *p = Getattr(n, "throws");
        String *tm;
        bool needComma = false;

        Append(signature, " throw(");
        Swig_typemap_attach_parms("throws", p, 0);
        while(p) {
          tm = Getattr(p, "tmap:throws");
          if(tm) {
            String *tmp = SwigType_str(Getattr(p, "type"), NULL);
            if(needComma)
              Append(signature, ", ");
            else
              needComma = true;
            Append(signature, tmp);
            Delete(tmp);
            Delete(tm);
          }
          p = nextSibling(p);
        }
        Append(signature, ")");
      }
    }
    { /* prep method decl */
      String *target = Copy(signature);
      Replaceall(target, "$name", name);
      Printf(mdecl, "    virtual %s;\n", target);
      Delete(target);
    }
    { /* prep method defn */
      String *qname;
      String *target;
      Wrapper *w;
      int outputs = 0;

      if(SwigType_type(type) != T_VOID) outputs = 1;

      qname = NewStringf("SwigDirector_%s::%s",
          Swig_class_name(parent), name);
      target  = Copy(signature);
      Replaceall(target, "$name", qname);
      Delete(qname);

      w = NewWrapper();
      Printf(w->def, "%s {", target);
      { /* generate method body */
        const char *retstmt = "";
        String *pstack;
        int pcount = 1;
        Wrapper_add_local(w, "SP", "dSP");
        pstack = NewString(
            "  ENTER;\n"
            "  SAVETMPS;\n"
            "  PUSHMARK(SP);\n"
            "  XPUSHs(av[0]);\n");
        Printf(w->code,
            "  av[0] = this->Swig::Director::getSelf();\n");
        if(parms) { /* convert call parms */
          Parm *p;
          String *tm;

          for(p = parms; p; p = nextSibling(p)) {
            /* really not sure why this is necessary but
             * Swig_typemap_attach_parms() didn't expand $1 without it... */
            Setattr(p, "lname", Getattr(p, "name"));
          }
          /* no clue why python checks the "in" typemaps, I imagine I'll
           * figure out soon enough once I have testcases runnning.
           * "out" typemaps might be a fallback attempt, but "in"? */
          /*Swig_typemap_attach_parms("in", parms, w);*/
          Swig_typemap_attach_parms("directorin", parms, w);
          Swig_typemap_attach_parms("directorargout", parms, w);
          for(p = parms; p; ) {
            SwigType *ptype = Getattr(p, "type");
            if (Getattr(p, "tmap:directorargout") != NULL) outputs++;
            tm = Getattr(p, "tmap:directorin");
            if(tm) {
              String *pav = NewStringf("av[%d]", pcount++);
              Printf(pstack, "  XPUSHs(%s);\n", pav);
              Replaceall(tm, "$input", pav);
              Replaceall(tm, "$owner", "0");
              if (is_shadow(ptype))
                Replaceall(tm, "$shadow", "SWIG_SHADOW");
              else
                Replaceall(tm, "$shadow", "0");
              Printf(w->code,
                  "  %s\n",
                  tm);
              p = Getattr(p, "tmap:directorin:next");
              Delete(tm);
              Delete(pav);
            } else {
              if(SwigType_type(ptype) != T_VOID) {
                Swig_warning(WARN_TYPEMAP_DIRECTORIN_UNDEF,
                    input_file, line_number,
                    "Unable to use type %s as a function argument "
                    "in director method %s (skipping method).\n",
                    SwigType_str(ptype, 0), target);
                output_director = false;
              }
              p = nextSibling(p);
            }
          }
        }
        {
          String *tmp = NewStringf("SV *av[%d]", pcount);
          Wrapper_add_local(w, "av", tmp);
          Delete(tmp);
        }
        Append(pstack, "  PUTBACK;\n");
        Append(w->code, pstack);
        Delete(pstack);
        /* This whole G_ARRAY probably needs to be rethought.  it overly
         * complicates the code and I'm not sure it DWIMs the way any
         * person should M. */
        switch(outputs) {
          case 0:
            Printf(w->code, "call_method(\"%s\", G_EVAL | G_VOID);\n",
                name);
            break;
          case 1:
            Wrapper_add_local(w, "w_count", "I32 w_count");
            Printf(w->code, "w_count = call_method(\"%s\", G_EVAL | G_SCALAR);\n", name);
            break;
          default:
            Wrapper_add_local(w, "w_count", "I32 w_count");
            Printf(w->code,
                "w_count = call_method(\"%s\", G_EVAL | G_ARRAY);\n"
                "if(w_count != %d) {\n"
                "  croak(\"expected %d values in return from %%s->%s\",\n"
                "  SvPV_nolen(this->Swig::Director::getSelf()));\n"
                "}\n", name, outputs, outputs, name);
            break;
        }
        Printf(w->code,
            "if(SvTRUE(ERRSV)) {\n"
            "  /* need to clean up the perl call stack here */\n"
            "  Swig::DirectorRunException::raise(ERRSV);\n"
            "}\n", name);
        if(outputs) {
          SwigType *ret_type;
          String   *tm;
          Parm     *p;
          char      buf[256];
          int       outnum = 0;
          Wrapper_add_local(w, "ax", "I32 ax");
          { /* return value frobnication */
            ret_type = Copy(type);
            SwigType *t = Copy(decl);
            SwigType *f = SwigType_pop_function(t);
            SwigType_push(ret_type, t);
            Delete(f);
            Delete(t);
          }
          Append(w->code,
              "  SPAGAIN;\n"
              "  SP -= w_count;\n"
              "  ax = (SP - PL_stack_base) + 1;\n");
          if(SwigType_type(type) != T_VOID) {
            {
              String *restype;
              restype = SwigType_lstr(ret_type, "w_result");
              Wrapper_add_local(w, "w_result", restype);
              Delete(restype);
            }
            p = NewParm(ret_type, "w_result", n);
            tm = Swig_typemap_lookup("directorout", p, "w_result", w);
            if(!tm) {
              Swig_warning(WARN_TYPEMAP_DIRECTOROUT_UNDEF,
                  input_file, line_number,
                  "Unable to use return type %s "
                  "in director method %s (skipping method).\n",
                  SwigType_str(type, 0), target);
              output_director = false;
            }
            Delete(p);
            sprintf(buf, "ST(%d)", outnum++);
            Replaceall(tm, "$result", "w_result");
            Replaceall(tm, "$input", buf);
            //if (Getattr(p, "wrap:disown") || (Getattr(p, "tmap:out:disown"))) {
            //  Replaceall(tm, "$disown", "SWIG_POINTER_DISOWN");
            //} else {
              Replaceall(tm, "$disown", "0");
            //}
            Printf(w->code, "%s\n", tm);
            Delete(tm);
            if(SwigType_isreference(ret_type))
              retstmt = "  return *w_result;\n";
            else
              retstmt = "  return w_result;\n";
          }
          /* now handle directorargout... */
          for(p = parms; p; ) {
            tm = Getattr(p, "tmap:directorargout");
            if(tm) {
              sprintf(buf, "ST(%d)", outnum++);
              Replaceall(tm, "$input", buf);
              Replaceall(tm, "$result", Getattr(p, "name"));
              Printf(w->code, "%s\n", tm);
              p = Getattr(p, "tmap:directorargout:next");
            } else {
              p = nextSibling(p);
            }
          }
          if(outnum != outputs) {
            Swig_warning(WARN_TYPEMAP_DIRECTOROUT_UNDEF,
                input_file, line_number,
                "expected %d outputs, but found %d typemaps "
                "in director method %s (skipping method).\n",
                outputs, outnum, target);
            output_director = false;
          }
          Append(w->code, "PUTBACK;\n");
          Delete(ret_type);
        }
        Printf(w->code,
            "  FREETMPS;\n"
            "  LEAVE;\n"
            "%s}", retstmt);
      }
      Delete(target);
      Wrapper_print(w, mdefn);
    }

    /* borrowed from python.cxx - director.cxx apparently expects us
     * to emit a "${methodname}SwigPublic" at times */
    if (dirprot_mode() && !is_public(n)) {
      if(Cmp(Getattr(n, "storage"), "virtual") ||
          Cmp(Getattr(n, "value"), "0")) {
        /* not pure virtual */
        String *target = Copy(signature);
        String *einame = NewStringf("%sSwigPublic", name);
        const char *returns = SwigType_type(type) != T_VOID ? "return " : "";
        String *upcall = Swig_method_call(super, parms);
        Replaceall(target, "$name", einame);
        Printf(mdecl,
            "  virtual %s {\n"
            "    %s%s;\n"
            "  }\n", target, returns, upcall);
        Delete(upcall);
        Delete(target);
      }
    }

    if(output_director) {
      Dump(mdecl, Swig_filebyname("director_h"));
      Dump(mdefn, Swig_filebyname("director"));
    }
    Delete(mdecl);
    Delete(mdefn);

    return Language::classDirectorMethod(n, parent, super);
  }
  virtual int classDirectorEnd(Node *n) {
    Printf(Swig_filebyname("director_h"),
        "};\n");
    return Language::classDirectorEnd(n);
  }
  virtual int classDirectorDisown(Node *n) {
    return Language::classDirectorDisown(n);
  }

  /* ------------------------------------------------------------
   * pragma()
   *
   * Pragma directive.
   *
   * %pragma(perl5) code="String"              # Includes a string in the .pm file
   * %pragma(perl5) include="file.pl"          # Includes a file in the .pm file
   * ------------------------------------------------------------ */

  virtual int pragmaDirective(Node *n) {
    String *lang;
    String *code;
    String *value;
    if (!ImportMode) {
      lang = Getattr(n, "lang");
      code = Getattr(n, "name");
      value = Getattr(n, "value");
      if (Strcmp(lang, "perl5") == 0) {
	if (Strcmp(code, "code") == 0) {
	  /* Dump the value string into the .pm file */
	  if (value) {
	    Printf(pragma_include, "%s\n", value);
	  }
	} else if (Strcmp(code, "include") == 0) {
	  /* Include a file into the .pm file */
	  if (value) {
	    FILE *f = Swig_include_open(value);
	    if (!f) {
	      Printf(stderr, "%s : Line %d. Unable to locate file %s\n", input_file, line_number, value);
	    } else {
	      char buffer[4096];
	      while (fgets(buffer, 4095, f)) {
		Printf(pragma_include, "%s", buffer);
	      }
	    }
	    fclose(f);
	  }
	} else {
	  Printf(stderr, "%s : Line %d. Unrecognized pragma.\n", input_file, line_number);
	}
      }
    }
    return Language::pragmaDirective(n);
  }

  /* ------------------------------------------------------------
   * perlcode()     - Output perlcode code into the shadow file
   * ------------------------------------------------------------ */

  String *perlcode(String *code, const String *indent) {
    String *out = NewString("");
    String *temp;
    char *t;
    if (!indent)
      indent = "";

    temp = NewString(code);

    t = Char(temp);
    if (*t == '{') {
      Delitem(temp, 0);
      Delitem(temp, DOH_END);
    }

    /* Split the input text into lines */
    List *clist = DohSplitLines(temp);
    Delete(temp);
    int initial = 0;
    String *s = 0;
    Iterator si;
    /* Get the initial indentation */

    for (si = First(clist); si.item; si = Next(si)) {
      s = si.item;
      if (Len(s)) {
	char *c = Char(s);
	while (*c) {
	  if (!isspace(*c))
	    break;
	  initial++;
	  c++;
	}
	if (*c && !isspace(*c))
	  break;
	else {
	  initial = 0;
	}
      }
    }
    while (si.item) {
      s = si.item;
      if (Len(s) > initial) {
	char *c = Char(s);
	c += initial;
	Printv(out, indent, c, "\n", NIL);
      } else {
	Printv(out, "\n", NIL);
      }
      si = Next(si);
    }
    Delete(clist);
    return out;
  }

  /* ------------------------------------------------------------
   * insertDirective()
   * 
   * Hook for %insert directive.
   * ------------------------------------------------------------ */

  virtual int insertDirective(Node *n) {
    String *code = Getattr(n, "code");
    String *section = Getattr(n, "section");

    if ((!ImportMode) && (Cmp(section, "perl") == 0)) {
      Printv(additional_perl_code, code, NIL);
    } else {
      Language::insertDirective(n);
    }
    return SWIG_OK;
  }

  String *runtimeCode() {
    String *s = NewString("");
    String *shead = Swig_include_sys("perlhead.swg");
    if (!shead) {
      Printf(stderr, "*** Unable to open 'perlhead.swg'\n");
    } else {
      Append(s, shead);
      Delete(shead);
    }
    String *serrors = Swig_include_sys("perlerrors.swg");
    if (!serrors) {
      Printf(stderr, "*** Unable to open 'perlerrors.swg'\n");
    } else {
      Append(s, serrors);
      Delete(serrors);
    }
    String *srun = Swig_include_sys("perlrun.swg");
    if (!srun) {
      Printf(stderr, "*** Unable to open 'perlrun.swg'\n");
    } else {
      Append(s, srun);
      Delete(srun);
    }
    return s;
  }

  String *defaultExternalRuntimeFilename() {
    return NewString("swigperlrun.h");
  }
};

/* -----------------------------------------------------------------------------
 * swig_perl5()    - Instantiate module
 * ----------------------------------------------------------------------------- */

static Language *new_swig_perl5() {
  return new PERL5();
}
extern "C" Language *swig_perl5(void) {
  return new_swig_perl5();
}
