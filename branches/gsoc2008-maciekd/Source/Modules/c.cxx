/* -----------------------------------------------------------------------------
 * See the LICENSE file for information on copyright, usage and redistribution
 * of SWIG, and the README file for authors - http://www.swig.org/release.html.
 *
 * c.cxx
 *
 * C language module for SWIG.
 * ----------------------------------------------------------------------------- */

char cvsroot_c_cxx[] = "$Id$";

#include "swigmod.h"

class C:public Language {
  static const char *usage;

  File *f_runtime;
  File *f_header;
  File *f_wrappers;
  File *f_init;
  File *f_shadow_c;

  String *f_shadow;

  bool shadow_flag;

public:

  /* -----------------------------------------------------------------------------
   * C()
   * ----------------------------------------------------------------------------- */

  C() : shadow_flag(true) {
  }

  /* ------------------------------------------------------------
   * main()
   * ------------------------------------------------------------ */

  virtual void main(int argc, char *argv[]) {

    SWIG_library_directory("c");

    // Add a symbol to the parser for conditional compilation
    Preprocessor_define("SWIGC 1", 0);

    // Add typemap definitions
    SWIG_typemap_lang("c");
    SWIG_config_file("c.swg");

    // Look for certain command line options
    for (int i = 1; i < argc; i++) {
      if (argv[i]) {
        if (strcmp(argv[i], "-help") == 0) {
          Printf(stdout, "%s\n", usage);
        } else if ((strcmp(argv[i], "-shadow") == 0) || (strcmp(argv[i], "-proxy") == 0)) {
          shadow_flag = true;
        } else if (strcmp(argv[i], "-noproxy") == 0) {
          shadow_flag = false;
        }
      }
    }
  }

  /* ---------------------------------------------------------------------
   * top()
   * --------------------------------------------------------------------- */

  virtual int top(Node *n) {
    String *module = Getattr(n, "name");
    String *outfile = Getattr(n, "outfile");

    /* initialize I/O */
    f_runtime = NewFile(outfile, "w");
    if (!f_runtime) {
      FileErrorDisplay(outfile);
      SWIG_exit(EXIT_FAILURE);
    }
    f_init = NewString("");
    f_header = NewString("");
    f_wrappers = NewString("");

    /* generate shadow file if enabled */
    if (shadow_flag) {
      f_shadow = NewString("");

      /* create shadow file with appropriate name */
      String *shadow_filename = NewStringf("%s%s_proxy.c", SWIG_output_directory(), Char(module));
      if ((f_shadow_c = NewFile(shadow_filename, "w")) == 0) {
        FileErrorDisplay(shadow_filename);
        SWIG_exit(EXIT_FAILURE);
      }

      Swig_register_filebyname("shadow", f_shadow);

      Printf(f_shadow, "/* This file was automatically generated by SWIG (http://www.swig.org).\n");
      Printf(f_shadow, " * Version %s\n", Swig_package_version());
      Printf(f_shadow, " * \n");
      Printf(f_shadow, " * Don't modify this file, modify the SWIG interface instead.\n");
      Printf(f_shadow, " */\n\n");
    }

    Swig_register_filebyname("header", f_header);
    Swig_register_filebyname("wrappers", f_wrappers);
    Swig_register_filebyname("runtime", f_runtime);
    Swig_register_filebyname("init", f_init);

    /* emit code for children */
    Language::top(n);

    /* finalize generating shadow file */
    if (shadow_flag) {
      Printv(f_shadow_c, f_shadow, "\n", NIL);
      Close(f_shadow_c);
      Delete(f_shadow);
    }

    /* write all to file */
    Dump(f_header, f_runtime);
    Dump(f_wrappers, f_runtime);
    Wrapper_pretty_print(f_init, f_runtime);

    /* cleanup */
    Delete(f_header);
    Delete(f_wrappers);
    Delete(f_init);
    Close(f_runtime);
    Delete(f_runtime);

    return SWIG_OK;
  }

  virtual int functionWrapper(Node *n) {
    String *name = Getattr(n, "sym:name");
    SwigType *type = Getattr(n, "type");
    ParmList *parms = Getattr(n, "parms");

    /* create new wrapper name */
    Wrapper *wrapper = NewWrapper();
    String *wname = Swig_name_wrapper(name);
    Setattr(n, "wrap:name", wname);

    /* create wrapper function prototype */
    Printv(wrapper->def, type, " ", wname, "(", NIL);

    /* prepare parameter list */
    Parm *p, *np;
    for (p = parms; p; ) {
      np = nextSibling(p);
      Printv(wrapper->def, Getattr(p, "type"), " ", Getattr(p, "lname"), np ? ", " : "", NIL);
      p = np;
    }
    Printv(wrapper->def, ") {", NIL);

    /* declare wrapper function local variables */
    emit_return_variable(n, type, wrapper);

    /* emit action code */
    String *action = emit_action(n);
    Append(wrapper->code, action);
    Append(wrapper->code, "return result;\n}\n");

    Wrapper_print(wrapper, f_wrappers);

    /* take care of shadow function */
    if (shadow_flag) {
      String *proto = ParmList_str(parms);
      String *arg_names = NewString("");
      Printv(f_shadow, type, " ", name, "(", proto, ") {\n", NIL);

      Parm *p, *np;
      for (p = parms; p; ) {
        np = nextSibling(p);
        Printv(arg_names, Getattr(p, "name"), np ? ", " : "", NIL);
        p = np;
      }

      /* handle 'prepend' feature */
      String *prepend_str = Getattr(n, "feature:prepend");
      if (prepend_str) {
        char *t = Char(prepend_str);
        if (*t == '{') {
          Delitem(prepend_str, 0);
          Delitem(prepend_str, DOH_END);
        }
        Printv(f_shadow, prepend_str, "\n", NIL);
      }

      Printv(f_shadow, "  return ", wname, "(", arg_names, ");\n", NIL);
      Printv(f_shadow, "}\n", NIL);
    }

    Delete(wname);
    DelWrapper(wrapper);

    return SWIG_OK;
  }

};				/* class C */

/* -----------------------------------------------------------------------------
 * swig_c()    - Instantiate module
 * ----------------------------------------------------------------------------- */

static Language *new_swig_c() {
  return new C();
}

extern "C" Language *swig_c(void) {
  return new_swig_c();
}

/* -----------------------------------------------------------------------------
 * Static member variables
 * ----------------------------------------------------------------------------- */

const char *C::usage = (char *) "\
C Options (available with -c)\n\
\n";

