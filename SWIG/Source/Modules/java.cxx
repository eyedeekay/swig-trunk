/* -----------------------------------------------------------------------------
 * java.cxx
 *
 *     Java wrapper module.
 *
 * Author(s) : William Fulton
 *             Harco de Hilster
 *
 * Copyright (C) 1999-2002.  The University of Chicago
 * See the file LICENSE for information on usage and redistribution.
 * ----------------------------------------------------------------------------- */

char cvsroot_java_cxx[] = "$Header$";

#include <limits.h> // for INT_MAX
#include "swigmod.h"
#ifndef MACSWIG
#include "swigconfig.h"
#endif
#include <ctype.h>

/* Hash type used for JNI upcall data */
typedef DOH UpcallData;

extern SwigType *cplus_value_type(SwigType *t);

/* External functions borrowed from python.cxx module: */

extern String *Swig_csuperclass_call(String* base, String* method, ParmList* l);
extern String *Swig_class_declaration(Node *n, String *name);
extern String *Swig_class_name(Node *n);
extern String *Swig_director_declaration(Node *n);
extern String *Swig_method_call(String_or_char *name, ParmList *parms);
extern String *method_decl(SwigType *s, const String_or_char *id, List *args, int strip, int values);

//------------------------------------------------------------------------

class JAVA : public Language {
  static const char *usage;
  const  String *empty_string;

  Hash   *swig_types_hash;
  File   *f_runtime;
  File   *f_runtime_h;
  File   *f_header;
  File   *f_wrappers;
  File   *f_init;
  File   *f_directors;
  File   *f_directors_h;

  bool   proxy_flag; // Flag for generating proxy classes
  bool   have_default_constructor_flag;
  bool   native_function_flag;     // Flag for when wrapping a native function
  bool   enum_constant_flag; // Flag for when wrapping an enum or constant
  bool   static_flag; // Flag for when wrapping a static functions or member variables
  bool   variable_wrapper_flag; // Flag for when wrapping a nonstatic member variable
  bool   wrapping_member_flag; // Flag for when wrapping a member variable/enum/const
  bool   global_variable_flag; // Flag for when wrapping a global variable
  bool   member_func_flag;      // flag set when wrapping a member function

  String *module;               // module name
  String *imclass_name;  // intermediary class name
  String *module_class_name;  // module class name
  String *imclass_class_code; // intermediary class code
  String *proxy_class_def;
  String *proxy_class_code;
  String *module_class_code;
  String *proxy_class_name;
  String *variable_name; //Name of a variable being wrapped
  String *proxy_class_constants_code;
  String *module_class_constants_code;
  String *package; // Package name
  String *jnipackage; // Package name used in the JNI code
  String *package_path; // Package name used internally by JNI (slashes)
  String *imclass_imports; //intermediary class imports from %pragma
  String *module_imports; //module imports from %pragma
  String *imclass_baseclass; //inheritance for intermediary class class from %pragma
  String *module_baseclass; //inheritance for module class from %pragma
  String *imclass_interfaces; //interfaces for intermediary class class from %pragma
  String *module_interfaces; //interfaces for module class from %pragma
  String *imclass_class_modifiers; //class modifiers for intermediary class overriden by %pragma
  String *module_class_modifiers; //class modifiers for module class overriden by %pragma
  String *upcasts_code; //C++ casts for inheritance hierarchies C++ code
  String *imclass_cppcasts_code; //C++ casts up inheritance hierarchies intermediary class code
  String *imclass_directors; // Intermediate class director Java code
  String *destructor_call; //C++ destructor call if any

  // Director method stuff:
  List   *dmethods_seq;
  Hash   *dmethods_table;
  int     n_dmethods;
  int     n_directors;
  bool    emitted_connect;
  int     first_class_dmethod;
  int     curr_class_dmethod;

  enum type_additions {none, pointer, reference};

  public:

  /* -----------------------------------------------------------------------------
   * JAVA()
   * ----------------------------------------------------------------------------- */

  JAVA() : 
    empty_string(NewString("")),

    swig_types_hash(NULL),
    f_runtime(NULL),
    f_runtime_h(NULL),
    f_header(NULL),
    f_wrappers(NULL),
    f_init(NULL),
    f_directors(NULL),
    f_directors_h(NULL),

    proxy_flag(true),
    have_default_constructor_flag(false),
    native_function_flag(false),
    enum_constant_flag(false),
    static_flag(false),
    variable_wrapper_flag(false),
    wrapping_member_flag(false),
    global_variable_flag(false),
    member_func_flag(false),

    imclass_name(NULL),
    module_class_name(NULL),
    imclass_class_code(NULL),
    proxy_class_def(NULL),
    proxy_class_code(NULL),
    module_class_code(NULL),
    proxy_class_name(NULL),
    variable_name(NULL),
    proxy_class_constants_code(NULL),
    module_class_constants_code(NULL),
    package(NULL),
    jnipackage(NULL),
    package_path(NULL),
    imclass_imports(NULL),
    module_imports(NULL),
    imclass_baseclass(NULL),
    module_baseclass(NULL),
    imclass_interfaces(NULL),
    module_interfaces(NULL),
    imclass_class_modifiers(NULL),
    module_class_modifiers(NULL),
    upcasts_code(NULL),
    imclass_cppcasts_code(NULL),
    imclass_directors(NULL),
    destructor_call(NULL),

    dmethods_seq(NULL),
    dmethods_table(NULL),
    n_dmethods(0),
    n_directors(0),
    emitted_connect(false)
    {
    }

  /* -----------------------------------------------------------------------------
   * getProxyName()
   *
   * Test to see if a type corresponds to something wrapped with a proxy class
   * Return NULL if not otherwise the proxy class name
   * ----------------------------------------------------------------------------- */

  String *getProxyName(SwigType *t) {
    if (proxy_flag) {
      Node *n = classLookup(t);
      if (n) {
        return Getattr(n,"sym:name");
      }
    }
    return NULL;
  }

  /* -----------------------------------------------------------------------------
   * makeValidJniName()
   * ----------------------------------------------------------------------------- */

  String *makeValidJniName(const String *name) {
    String *valid_jni_name = NewString(name);
    Replaceall(valid_jni_name,"_","_1");
    return valid_jni_name;
  }

  /* ------------------------------------------------------------
   * main()
   * ------------------------------------------------------------ */

  virtual void main(int argc, char *argv[]) {

    SWIG_library_directory("java");

    // Look for certain command line options
    for (int i = 1; i < argc; i++) {
      if (argv[i]) {
        if (strcmp(argv[i],"-package") == 0) {
          if (argv[i+1]) {
            package = NewString("");
            Printf(package, argv[i+1]);
            Swig_mark_arg(i);
            Swig_mark_arg(i+1);
            i++;
          } else {
            Swig_arg_error();
          }
        } else if ((strcmp(argv[i],"-shadow") == 0) || ((strcmp(argv[i],"-proxy") == 0))) {
          Printf(stderr,"Deprecated command line option: %s. Proxy classes are now generated by default.\n", argv[i]);
          Swig_mark_arg(i);
          proxy_flag = true;
        } else if ((strcmp(argv[i],"-noproxy") == 0)) {
          Swig_mark_arg(i);
          proxy_flag = false;
        } else if (strcmp(argv[i],"-jnic") == 0) {
          Swig_mark_arg(i);
          Printf(stderr,"Deprecated command line option: -jnic. C JNI calling convention now used when -c++ not specified.\n");
        } else if (strcmp(argv[i],"-nofinalize") == 0) {
          Swig_mark_arg(i);
          Printf(stderr,"Deprecated command line option: -nofinalize. Use the new javafinalize typemap instead.\n");
        } else if (strcmp(argv[i],"-jnicpp") == 0) {
          Swig_mark_arg(i);
          Printf(stderr,"Deprecated command line option: -jnicpp. C++ JNI calling convention now used when -c++ specified.\n");
        } else if (strcmp(argv[i],"-help") == 0) {
          Printf(stderr,"%s\n", usage);
        }
      }
    }

    // Add a symbol to the parser for conditional compilation
    Preprocessor_define("SWIGJAVA 1",0);

    // Add typemap definitions
    SWIG_typemap_lang("java");
    SWIG_config_file("java.swg");

    allow_overloading();
  }

  /* ---------------------------------------------------------------------
   * top()
   * --------------------------------------------------------------------- */

  virtual int top(Node *n) {
    // Make the intermediary class and module class names. The
    // intermediary class name can be set in the module directive.

    Node* optionsnode = Getattr( Getattr(n,"module"), "options");

    if (optionsnode) {
      if (Getattr(optionsnode,"jniclassname"))
        imclass_name = Copy(Getattr(optionsnode,"jniclassname"));
      /* check if directors are enabled for this module.  note: this 
       * is a "master" switch, without which no director code will be
       * emitted.  %feature("director") statements are also required
       * to enable directors for individual classes or methods.
       *
       * use %module(directors="1") modulename at the start of the 
       * interface file to enable director generation.
       */
      if (Getattr(optionsnode, "directors")) {
        allow_directors();
      }
    }

    /* Initialize all of the output files */
    String *outfile = Getattr(n,"outfile");
    String *outfile_h = Getattr(n, "outfile_h");

    f_runtime = NewFile(outfile,"w");
    if (!f_runtime) {
      Printf(stderr,"Unable to open %s\n", outfile);
      SWIG_exit(EXIT_FAILURE);
    }

    if (directorsEnabled()) {
      f_runtime_h = NewFile(outfile_h,"w");
      if (!f_runtime_h) {
        Printf(stderr,"*** Can't open '%s'\n", outfile_h);
        SWIG_exit(EXIT_FAILURE);
      }
    }

    f_init = NewString("");
    f_header = NewString("");
    f_wrappers = NewString("");
    f_directors_h = NewString("");
    f_directors = NewString("");

    /* Register file targets with the SWIG file handler */
    Swig_register_filebyname("header",f_header);
    Swig_register_filebyname("wrapper",f_wrappers);
    Swig_register_filebyname("runtime",f_runtime);
    Swig_register_filebyname("init",f_init);
    Swig_register_filebyname("director",f_directors);
    Swig_register_filebyname("director_h",f_directors_h);

    swig_types_hash = NewHash();

    if (!imclass_name) {
      imclass_name = NewStringf("%sJNI", Getattr(n,"name"));
      module_class_name = Copy(Getattr(n,"name"));
    } else {
      // Rename the module name if it is the same as intermediary class name - a backwards compatibility solution
      if (Cmp(imclass_name, Getattr(n,"name")) == 0)
        module_class_name = NewStringf("%sModule", Getattr(n,"name"));
      else
        module_class_name = Copy(Getattr(n,"name"));
    }

    module = Copy(Getattr(n,"name"));
    imclass_class_code = NewString("");
    proxy_class_def = NewString("");
    proxy_class_code = NewString("");
    module_class_constants_code = NewString("");
    imclass_baseclass = NewString("");
    imclass_interfaces = NewString("");
    imclass_class_modifiers = NewString(""); // package access only to the intermediary class by default
    module_class_code = NewString("");
    module_baseclass = NewString("");
    module_interfaces = NewString("");
    module_imports = NewString("");
    module_class_modifiers = NewString("public");
    imclass_imports = NewString("");
    imclass_cppcasts_code = NewString("");
    imclass_directors = NewString("");
    upcasts_code = NewString("");
    dmethods_seq = NewList();
    dmethods_table = NewHash();
    n_dmethods = 0;
    n_directors = 0;
    emitted_connect = false;
    if (!package) package = NewString("");
    jnipackage = NewString("");
    package_path = NewString("");

    Swig_banner(f_runtime);               // Print the SWIG banner message

    if (NoInclude) {
      Printf(f_runtime,"#define SWIG_NOINCLUDE\n");
    }

    if (directorsEnabled()) {
      Printf(f_runtime,"#define SWIG_DIRECTORS\n");
    }

    /* Emit initial director header and director code: */
    if (directorsEnabled()) {
      Swig_banner(f_directors_h);
      Printf(f_directors_h, "#ifndef SWIG_%s_WRAP_H_\n", module);
      Printf(f_directors_h, "#define SWIG_%s_WRAP_H_\n\n", module);
      Printf(f_directors_h, "class Swig::Director;\n\n");

      Printf(f_directors, "\n\n");
      Printf(f_directors, "/* ---------------------------------------------------\n");
      Printf(f_directors, " * C++ director class methods\n");
      Printf(f_directors, " * --------------------------------------------------- */\n\n");
      Printf(f_directors, "#include \"%s\"\n\n", outfile_h);
    }

    String *wrapper_name = NewString("");

    if(Len(package)) {
      String *jniname = makeValidJniName(package);
      Printv(jnipackage, jniname, NIL);
      Delete(jniname);
      Replaceall(jnipackage,".","_");
      Append(jnipackage, "_");
      Printv(package_path, package, NIL);
      Replaceall(package_path, ".", "/");
    }
    String *jniname = makeValidJniName(imclass_name);
    Printf(wrapper_name, "Java_%s%s_%%f", Char(jnipackage), jniname);
    Delete(jniname);

    Swig_name_register((char*)"wrapper", Char(wrapper_name));
    Swig_name_register((char*)"set", (char*)"set_%v");
    Swig_name_register((char*)"get", (char*)"get_%v");
    Swig_name_register((char*)"member", (char*)"%c_%m");

    Delete(wrapper_name);

    Printf(f_wrappers,"\n#ifdef __cplusplus\n");
    Printf(f_wrappers,"extern \"C\" {\n");
    Printf(f_wrappers,"#endif\n\n");

    /* Emit code */
    Language::top(n);

    if (directorsEnabled()) {
      // Insert director runtime into the f_runtime file (make it
      // occur before %header section.
      Swig_insert_file("director.swg", f_runtime);
    }

    // Generate the intermediary class
    {
      String *filen = NewStringf("%s%s.java", SWIG_output_directory(), imclass_name);
      File *f_im = NewFile(filen,"w");
      if(!f_im) {
        Printf(stderr,"Unable to open %s\n", filen);
        SWIG_exit(EXIT_FAILURE);
      }
      Delete(filen); filen = NULL;

      // Start writing out the intermediary class
      if(Len(package) > 0)
        Printf(f_im, "package %s;\n\n", package);

      emitBanner(f_im);
      if(imclass_imports)
        Printf(f_im, "%s\n", imclass_imports);

      if (Len(imclass_class_modifiers) > 0)
        Printf(f_im, "%s ", imclass_class_modifiers);
      Printf(f_im, "class %s ", imclass_name);

      if (imclass_baseclass && *Char(imclass_baseclass))
        Printf(f_im, "extends %s ", imclass_baseclass);
      if (Len(imclass_interfaces) > 0)
        Printv(f_im, "implements ", imclass_interfaces, " ", NIL);
      Printf(f_im, "{\n");

      // Add the intermediary class methods
      Printv(f_im, imclass_class_code, NIL);
      Printv(f_im, imclass_cppcasts_code, NIL);
      if (Len(imclass_directors) > 0) {
        Printf(f_im, "\n/* Director upcall methods: */\n\n");
        Printv(f_im, imclass_directors, NIL);
      }

      if (n_dmethods > 0) {
        Putc('\n', f_im);
        Printf(f_im, "  private final static native void swig_module_init();\n");
        Printf(f_im, "  static {\n");
        Printf(f_im, "    swig_module_init();\n");
        Printf(f_im, "  }\n");
      }

      // Finish off the class
      Printf(f_im, "}\n");
      Close(f_im);
    }

    // Generate the Java module class
    {
      String *filen = NewStringf("%s%s.java", SWIG_output_directory(), module_class_name);
      File *f_module = NewFile(filen,"w");
      if(!f_module) {
        Printf(stderr,"Unable to open %s\n", filen);
        SWIG_exit(EXIT_FAILURE);
      }
      Delete(filen); filen = NULL;

      // Start writing out the module class
      if(Len(package) > 0)
        Printf(f_module, "package %s;\n\n", package);

      emitBanner(f_module);
      if(module_imports)
        Printf(f_module, "%s\n", module_imports);

      if (Len(module_class_modifiers) > 0)
        Printf(f_module, "%s ", module_class_modifiers);
      Printf(f_module, "class %s ", module_class_name);

      if (module_baseclass && *Char(module_baseclass))
        Printf(f_module, "extends %s ", module_baseclass);
      if (Len(module_interfaces) > 0) {
        if (Len(module_class_constants_code) != 0 )
          Printv(f_module, "implements ", Getattr(n, "name"), "Constants, ", module_interfaces, " ", NIL);
        else
          Printv(f_module, "implements ", module_interfaces, " ", NIL);
      } else {
        if (Len(module_class_constants_code) != 0 )
          Printv(f_module, "implements ", Getattr(n, "name"), "Constants ", NIL);
      }
      Printf(f_module, "{\n");

      // Add the wrapper methods
      Printv(f_module, module_class_code, NIL);

      // Finish off the class
      Printf(f_module, "}\n");
      Close(f_module);
    }

    // Generate the Java constants interface
    if (Len(module_class_constants_code) != 0 ) {
      String *filen = NewStringf("%s%sConstants.java", SWIG_output_directory(), module_class_name);
      File *f_module = NewFile(filen,"w");
      if(!f_module) {
        Printf(stderr,"Unable to open %s\n", filen);
        SWIG_exit(EXIT_FAILURE);
      }
      Delete(filen); filen = NULL;

      // Start writing out the Java constants interface
      if(Len(package) > 0)
        Printf(f_module, "package %s;\n\n", package);

      emitBanner(f_module);
      if(module_imports)
        Printf(f_module, "%s\n", module_imports);

      if (Len(module_class_modifiers) > 0)
        Printf(f_module, "%s ", module_class_modifiers);
      Printf(f_module, "interface %sConstants {\n", module_class_name);

      // Write out all the enums constants
      Printv(f_module, "  // enums and constants\n", module_class_constants_code, NIL);

      // Finish off the Java interface
      Printf(f_module, "}\n");
      Close(f_module);
    }

    if(upcasts_code)
      Printv(f_wrappers,upcasts_code,NIL);

    emitDirectorUpcalls();

    Printf(f_wrappers,"#ifdef __cplusplus\n");
    Printf(f_wrappers,"}\n");
    Printf(f_wrappers,"#endif\n");

    // Output a Java type wrapper class for each SWIG type
    for (Iterator swig_type = First(swig_types_hash); swig_type.key; swig_type = Next(swig_type)) {
      emitTypeWrapperClass(swig_type.key, swig_type.item);
    }

    /* Close all of the files */
    Dump(f_header,f_runtime);

    if (directorsEnabled()) {
      Dump(f_directors, f_runtime);
      Dump(f_directors_h, f_runtime_h);

      Printf(f_runtime_h, "\n");
      Printf(f_runtime_h, "#endif\n");
    }

    Delete(swig_types_hash); swig_types_hash = NULL;
    Delete(imclass_name); imclass_name = NULL;
    Delete(imclass_class_code); imclass_class_code = NULL;
    Delete(proxy_class_def); proxy_class_def = NULL;
    Delete(proxy_class_code); proxy_class_code = NULL;
    Delete(module_class_constants_code); module_class_constants_code = NULL;
    Delete(imclass_baseclass); imclass_baseclass = NULL;
    Delete(imclass_interfaces); imclass_interfaces = NULL;
    Delete(imclass_class_modifiers); imclass_class_modifiers = NULL;
    Delete(module_class_name); module_class_name = NULL;
    Delete(module_class_code); module_class_code = NULL;
    Delete(module_baseclass); module_baseclass = NULL;
    Delete(module_interfaces); module_interfaces = NULL;
    Delete(module_imports); module_imports = NULL;
    Delete(module_class_modifiers); module_class_modifiers = NULL;
    Delete(imclass_imports); imclass_imports = NULL;
    Delete(imclass_cppcasts_code); imclass_cppcasts_code = NULL;
    Delete(imclass_directors); imclass_directors = NULL;
    Delete(upcasts_code); upcasts_code = NULL;
    Delete(package); package = NULL;
    Delete(jnipackage); jnipackage = NULL;
    Delete(package_path); package_path = NULL;
    Delete(dmethods_seq); dmethods_seq = NULL;
    Delete(dmethods_table); dmethods_table = NULL;
    n_dmethods = 0;
    emitted_connect = false;

    if (f_runtime_h) {
      Close(f_runtime_h);
      Delete(f_runtime_h); f_runtime_h = NULL;
    }

    Delete(f_directors); f_directors = NULL;
    Delete(f_directors_h); f_directors_h = NULL;

    Dump(f_wrappers,f_runtime);
    Wrapper_pretty_print(f_init,f_runtime);
    Delete(f_header);
    Delete(f_wrappers);
    Delete(f_init);
    Close(f_runtime);
    Delete(f_runtime);
    return SWIG_OK;
  }

  /* -----------------------------------------------------------------------------
   * emitBanner()
   * ----------------------------------------------------------------------------- */

  void emitBanner(File *f) {
    Printf(f, "/* ----------------------------------------------------------------------------\n");
    Printf(f, " * This file was automatically generated by SWIG (http://www.swig.org).\n");
    Printf(f, " * Version: %s\n", PACKAGE_VERSION);
    Printf(f, " *\n");
    Printf(f, " * Do not make changes to this file unless you know what you are doing--modify\n");
    Printf(f, " * the SWIG interface file instead.\n");
    Printf(f, " * ----------------------------------------------------------------------------- */\n\n");
  }

  /*-----------------------------------------------------------------------
   * Add new director upcall signature
   *----------------------------------------------------------------------*/

  UpcallData *
  addUpcallMethod(String *imclass_method, String *class_method, String *imclass_desc, String *class_desc,
                  String *decl)
  {
    UpcallData         *udata;
    String             *imclass_methodidx;
    String             *class_methodidx;
    Hash               *new_udata;
    String             *key = NewStringf("%s|%s", imclass_method, decl);
    
    /* Do we know about this director class already? */
    if ((udata = Getattr(dmethods_table, key)) != NULL) {
      Delete(key);
      return Getattr(udata, "methodoff");
    }

    imclass_methodidx = NewStringf("%d", n_dmethods);
    class_methodidx = NewStringf("%d", n_dmethods - first_class_dmethod);
    n_dmethods++;

    new_udata = NewHash();
    Append(dmethods_seq, new_udata);
    Setattr(dmethods_table, key, new_udata);

    Setattr(new_udata, "method", Copy(class_method));
    Setattr(new_udata, "fdesc", Copy(class_desc));
    Setattr(new_udata, "imclass_method", Copy(imclass_method));
    Setattr(new_udata, "imclass_fdesc", Copy(imclass_desc));
    Setattr(new_udata, "imclass_methodidx", imclass_methodidx);
    Setattr(new_udata, "class_methodidx", class_methodidx);
    Setattr(new_udata, "decl", Copy(decl));

    Delete(key);
    return new_udata;
  }

  /*-----------------------------------------------------------------------
   * Get director upcall signature
   *----------------------------------------------------------------------*/

  UpcallData *
  getUpcallMethodData(String *director_class, String *decl)
  {
    String             *key = NewStringf("%s|%s", director_class, decl);
    UpcallData         *udata = Getattr(dmethods_table, key);

    Delete(key);
    return udata;
  }

  /* ----------------------------------------------------------------------
   * nativeWrapper()
   * ---------------------------------------------------------------------- */

  virtual int nativeWrapper(Node *n) {
    String *wrapname = Getattr(n,"wrap:name");

    if (!addSymbol(wrapname,n)) return SWIG_ERROR;

    if (Getattr(n,"type")) {
      Swig_save("nativeWrapper",n,"name",NIL);
      Setattr(n,"name", wrapname);
      native_function_flag = true;
      functionWrapper(n);
      Swig_restore(n);
      native_function_flag = false;
    } else {
      Printf(stderr,"%s : Line %d. No return type for %%native method %s.\n", input_file, line_number, Getattr(n,"wrap:name"));
    }

    return SWIG_OK;
  }

  /* ----------------------------------------------------------------------
   * functionWrapper()
   * ---------------------------------------------------------------------- */

  virtual int functionWrapper(Node *n) {
    String   *symname = Getattr(n,"sym:name");
    Node     *parent = parentNode(n);
    SwigType *t = Getattr(n,"type");
    ParmList *l = Getattr(n,"parms");
    String    *tm;
    Parm      *p;
    int       i;
    String    *c_return_type = NewString("");
    String    *im_return_type = NewString("");
    String    *cleanup = NewString("");
    String    *outarg = NewString("");
    String    *body = NewString("");
    int       num_arguments = 0;
    int       num_required = 0;
    int       gencomma = 0;
    bool      is_void_return;
    String    *overloaded_name = getOverloadedName(n);

    String *nodeType = nodeType(n);
    int constructor = !(Cmp(nodeType, "constructor") && Cmp(nodeType, "class")); 
    int destructor = (!Cmp(nodeType, "destructor")); 
    String *storage   = Getattr(n,"storage");
    int isVirtual = (!Cmp(storage,"virtual"));
    bool pure_virtual = false;
    bool director_method = false;
    bool feature_director = false;
    bool recursive_upcall = false;
    bool feature_extend = !Cmp(Getattr(n, "feature:extend"), "1");
    String *director_class = NULL;
    String *director_uargs = NewString("director->swig_get_self()");
    String *dirimclass_meth = NewStringf("SwigDirector_%s", overloaded_name);
    UpcallData *udata = NULL;
    String *class_methodidx = NULL;
    String *imclass_methodidx = NULL;

    /* Do we have to emit the director hair version of the code? */

    feature_director = (parent && Swig_directorclass(n) && Cmp(Getattr(n, "feature:nodirector"), "1"));

    if (member_func_flag && isVirtual && feature_director && !(static_flag || constructor || destructor)) {
      director_method = true;
      director_class = NewStringf("SwigDirector_%s", proxy_class_name);
      udata = getUpcallMethodData(dirimclass_meth, Getattr(n, "decl")); 
      imclass_methodidx = Getattr(udata, "imclass_methodidx");
      class_methodidx = Getattr(udata, "class_methodidx");

      recursive_upcall = !(Cmp(Getattr(n, "feature:director:recursive"), "1")
                           && Cmp(Getattr(parent, "feature:director:recursive"), "1"));
    }

    if (!Cmp(Getattr(n, "value"), "0"))
      pure_virtual = true;

    if (!Getattr(n,"sym:overloaded")) {
      if (!addSymbol(Getattr(n,"sym:name"),n)) return SWIG_ERROR;
    }

    /* 
     * Generate the proxy class getters/setters for public member variables.
     * Not for enums and constants.
     */
    if(proxy_flag && wrapping_member_flag && !enum_constant_flag) {
      // Capitalize the first letter in the variable to create a JavaBean type getter/setter function name
      bool getter_flag = Cmp(symname, Swig_name_set(Swig_name_member(proxy_class_name, variable_name))) != 0;

      String *getter_setter_name = NewString("");
      if(!getter_flag)
        Printf(getter_setter_name,"set");
      else 
        Printf(getter_setter_name,"get");
      Putc(toupper((int) *Char(variable_name)), getter_setter_name);
      Printf(getter_setter_name, "%s", Char(variable_name)+1);

      Setattr(n,"proxyfuncname", getter_setter_name);
      Setattr(n,"imfuncname", symname);

      proxyClassFunctionHandler(n);
      Delete(getter_setter_name);
    }

    /*
       The rest of this function deals with generating the intermediary class wrapper function (that wraps
       a c/c++ function) and generating the JNI c code. Each Java wrapper function has a 
       matching JNI c function call.
       */

    // A new wrapper function object
    Wrapper *f = NewWrapper();

    // Make a wrapper name for this function
    String *jniname = makeValidJniName(overloaded_name);
    String *wname = Swig_name_wrapper(jniname);

    Delete(jniname);

    /* Attach the non-standard typemaps to the parameter list. */
    Swig_typemap_attach_parms("jni", l, f);
    Swig_typemap_attach_parms("jtype", l, f);
    if (director_method) {
      Parm             *jni_pm = NewParm(t, (String*) empty_string);
      String           *jni_tm = Swig_typemap_lookup_new("jni", jni_pm, "", 0);
      Parm             *din_pm = NewParm(jni_tm, (String*) empty_string);
      String           *din_tm = Swig_typemap_lookup_new("directorin", din_pm, "", 0);

      Setattr(n, "tmap:directorin", din_tm);
      Setattr(n, "tmap:directorin:parse", Getattr(din_pm, "tmap:directorin:parse"));

      Delete(jni_pm);
      Delete(jni_tm);
      Delete(din_pm);
      Delete(din_tm);

      Swig_typemap_attach_parms("javadirectorin", l, 0);
    }

    /* Get return types */
    if ((tm = Swig_typemap_lookup_new("jni",n,"",0))) {
      Printf(c_return_type,"%s", tm);
    } else {
      Swig_warning(WARN_JAVA_TYPEMAP_JNI_UNDEF, input_file, line_number, 
          "No jni typemap defined for %s\n", SwigType_str(t,0));
    }

    if ((tm = Swig_typemap_lookup_new("jtype",n,"",0))) {
      Printf(im_return_type,"%s", tm);
    } else {
      Swig_warning(WARN_JAVA_TYPEMAP_JTYPE_UNDEF, input_file, line_number, 
          "No jtype typemap defined for %s\n", SwigType_str(t,0));
    }

    is_void_return = (Cmp(c_return_type, "void") == 0);
    if (!is_void_return)
      Wrapper_add_localv(f,"jresult", c_return_type, "jresult = 0",NIL);

    Printf(imclass_class_code, "  public final static native %s %s(", im_return_type, overloaded_name);
    Printv(f->def, "JNIEXPORT ", c_return_type, " JNICALL ", wname, "(JNIEnv *jenv, jclass jcls", NIL);

    // Usually these function parameters are unused - The code below ensures
    // that compilers do not issue such a warning if configured to do so.

    Printv(f->code,"    (void)jenv;\n",NIL);
    Printv(f->code,"    (void)jcls;\n",NIL);

    /* Add the class's java object, when applicable */
    if (proxy_flag && member_func_flag && !(static_flag || feature_extend || constructor || destructor)) {
      if (director_method) {
        String *director_decl = NewStringf("%s *", director_class);
        Wrapper_add_localv(f, "director", director_decl, "director = 0", NIL);
        Delete(director_decl);
      }
    }

    // Emit all of the local variables for holding arguments.
    emit_args(t,l,f);

    /* Attach the standard typemaps */
    emit_attach_parmmaps(l,f);
    Setattr(n,"wrap:parms",l);

    /* Get number of required and total arguments */
    num_arguments = emit_num_arguments(l);
    num_required  = emit_num_required(l);

    // Now walk the function parameter list and generate code to get arguments
    for (i = 0, p=l; i < num_arguments; i++) {

      while (checkAttribute(p,"tmap:in:numinputs","0")) {
        p = Getattr(p,"tmap:in:next");
      }

      SwigType *pt = Getattr(p,"type");
      String   *ln = Getattr(p,"lname");
      String   *im_param_type = NewString("");
      String   *c_param_type = NewString("");
      String   *arg = NewString("");

      Printf(arg,"j%s", ln);

      /* Get the JNI C types of the parameter */
      if ((tm = Getattr(p,"tmap:jni"))) {
        Printv(c_param_type, tm, NIL);
      } else {
        Swig_warning(WARN_JAVA_TYPEMAP_JNI_UNDEF, input_file, line_number, 
            "No jni typemap defined for %s\n", SwigType_str(pt,0));
      }

      /* Get the intermediary class parameter types of the parameter */
      if ((tm = Getattr(p,"tmap:jtype"))) {
        Printv(im_param_type, tm, NIL);
      } else {
        Swig_warning(WARN_JAVA_TYPEMAP_JTYPE_UNDEF, input_file, line_number, 
            "No jtype typemap defined for %s\n", SwigType_str(pt,0));
      }

      /* Add parameter to intermediary class method */
      if(gencomma) Printf(imclass_class_code, ", ");
      Printf(imclass_class_code, "%s %s", im_param_type, arg);

      // Add parameter to C function
      Printv(f->def, ", ", c_param_type, " ", arg, NIL);

      if (director_method && gencomma > 0) {
        String *base_type = SwigType_base(pt);
        Node *tn;

        /* Make sure base_type isn't actually a class */
        tn = Swig_symbol_clookup(base_type, Getattr(n, "sym:symtab"));
        if (tn != NULL) {
          Delete(base_type);
          base_type = Copy(Getattr(tn, "sym:name"));
        }

        /* Add argument to director call arguments */

        Append(director_uargs, ", ");
        Append(director_uargs, arg);
        Delete(base_type);
      }
      
      ++gencomma;

      // Get typemap for this argument
      if ((tm = Getattr(p,"tmap:in"))) {
        addThrows(n, "tmap:in", p);
        Replaceall(tm,"$source",arg); /* deprecated */
        Replaceall(tm,"$target",ln); /* deprecated */
        Replaceall(tm,"$arg",arg); /* deprecated? */
        Replaceall(tm,"$input", arg);
        Setattr(p,"emit:input", arg);
        Printf(f->code,"%s\n", tm);
        p = Getattr(p,"tmap:in:next");
      } else {
        Swig_warning(WARN_TYPEMAP_IN_UNDEF, input_file, line_number, 
            "Unable to use type %s as a function argument.\n",SwigType_str(pt,0));
        p = nextSibling(p);
      }

      /* Don't make director methods do work if all the code will do is
       * call Java next, encapsulate the ordinary argument marshalling,
       * but make sure that the first argument is emitted. */

      if (director_method && i == 0) {
        Printf(f->code, "director = dynamic_cast<%s *>(arg1);\n", director_class);
        Printf(f->code, "if (director == NULL || !director->swig_overrides(%s)) {\n", class_methodidx);
      }

      Delete(im_param_type);
      Delete(c_param_type);
      Delete(arg);
    }

    /* Insert constraint checking code */
    for (p = l; p;) {
      if ((tm = Getattr(p,"tmap:check"))) {
        addThrows(n, "tmap:check", p);
        Replaceall(tm,"$target",Getattr(p,"lname")); /* deprecated */
        Replaceall(tm,"$arg",Getattr(p,"emit:input")); /* deprecated? */
        Replaceall(tm,"$input",Getattr(p,"emit:input"));
        Printv(f->code,tm,"\n",NIL);
        p = Getattr(p,"tmap:check:next");
      } else {
        p = nextSibling(p);
      }
    }

    /* Insert cleanup code */
    for (p = l; p;) {
      if ((tm = Getattr(p,"tmap:freearg"))) {
        addThrows(n, "tmap:freearg", p);
        Replaceall(tm,"$source",Getattr(p,"emit:input")); /* deprecated */
        Replaceall(tm,"$arg",Getattr(p,"emit:input")); /* deprecated? */
        Replaceall(tm,"$input",Getattr(p,"emit:input"));
        Printv(cleanup,tm,"\n",NIL);
        p = Getattr(p,"tmap:freearg:next");
      } else {
        p = nextSibling(p);
      }
    }

    /* Insert argument output code */
    for (p = l; p;) {
      if ((tm = Getattr(p,"tmap:argout"))) {
        addThrows(n, "tmap:argout", p);
        Replaceall(tm,"$source",Getattr(p,"emit:input")); /* deprecated */
        Replaceall(tm,"$target",Getattr(p,"lname")); /* deprecated */
        Replaceall(tm,"$arg",Getattr(p,"emit:input")); /* deprecated? */
        Replaceall(tm,"$result","jresult");
        Replaceall(tm,"$input",Getattr(p,"emit:input"));
        Printv(outarg,tm,"\n",NIL);
        p = Getattr(p,"tmap:argout:next");
      } else {
        p = nextSibling(p);
      }
    }

    // Get any Java exception classes in the throws typemap
    ParmList *throw_parm_list = NULL;
    if ((throw_parm_list = Getattr(n,"throws"))) {
      Swig_typemap_attach_parms("throws", throw_parm_list, f);
      for (p = throw_parm_list; p; p=nextSibling(p)) {
        if ((tm = Getattr(p,"tmap:throws"))) {
          addThrows(n, "tmap:throws", p);
        }
      }
    }

    if (Cmp(nodeType(n), "constant") == 0) {
      // Wrapping a constant hack
      Swig_save("functionWrapper",n,"wrap:action",NIL);

      // below based on Swig_VargetToFunction()
      SwigType *ty = Swig_wrapped_var_type(Getattr(n,"type"));
      Setattr(n,"wrap:action", NewStringf("result = (%s) %s;\n", SwigType_lstr(ty,0), Getattr(n, "value")));
    }

    // Now write code to make the function call
    if(!native_function_flag)
      emit_action(n,f);

    if (Cmp(nodeType(n), "constant") == 0)
      Swig_restore(n);

    /* Return value if necessary  */
    if(!native_function_flag) {
      if ((tm = Swig_typemap_lookup_new("out",n,"result",0))) {
        addThrows(n, "tmap:out", n);
        Replaceall(tm,"$source", "result"); /* deprecated */
        Replaceall(tm,"$target", "jresult"); /* deprecated */
        Replaceall(tm,"$result","jresult");
        Printf(f->code,"%s", tm);
        if (Len(tm))
          Printf(f->code,"\n");
      } else {
        Swig_warning(WARN_TYPEMAP_OUT_UNDEF, input_file, line_number,
            "Unable to use return type %s in function %s.\n", SwigType_str(t,0), Getattr(n,"name"));
      }
    }

    /* Output argument output code */
    Printv(f->code,outarg,NIL);

    /* Output cleanup code */
    Printv(f->code,cleanup,NIL);

    /* Look to see if there is any newfree cleanup code */
    if (Getattr(n,"feature:new")) {
      if ((tm = Swig_typemap_lookup_new("newfree",n,"result",0))) {
        addThrows(n, "tmap:newfree", n);
        Replaceall(tm,"$source","result"); /* deprecated */
        Printf(f->code,"%s\n",tm);
      }
    }

    /* See if there is any return cleanup code */
    if(!native_function_flag) {
      if ((tm = Swig_typemap_lookup_new("ret", n, "result", 0))) {
        Replaceall(tm,"$source","result"); /* deprecated */
        Printf(f->code,"%s\n",tm);
      }
    }

    /* Director code generation */
    if (director_method) {
      String   *jdescrip, *upcall_method;

      Printf(f->code, "} else {\n");

      if ((tm= Getattr(n, "tmap:directorin")) != NULL && (jdescrip = Getattr(n, "tmap:directorin:parse")) != NULL) {
        String *jni_canon = canonicalJNIFDesc(jdescrip, n, proxy_class_name);

        Delete(jdescrip);
        jdescrip = jni_canon;

        upcall_method = getUpcallJNIMethod(jdescrip);

        if (upcall_method != NULL) {
          if (!recursive_upcall) {
            Printf(f->code, "  if (!director->swig_get_ricochet(%s)) {\n", class_methodidx);
            Printf(f->code, "    director->swig_set_ricochet(%s);\n", class_methodidx);
          }

          if (!is_void_return)
            Printf(f->code, "  jresult = (%s) jenv->%s(jcls, Swig::director_methids[%s], %s);\n",
                   c_return_type, upcall_method, imclass_methodidx, director_uargs);
          else
            Printf(f->code, "  jenv->%s(jcls, Swig::director_methids[%s], %s);\n",
                   upcall_method, imclass_methodidx, director_uargs);

          if (!recursive_upcall)
            Printf(f->code, "    director->swig_clear_ricochet(%s);\n", class_methodidx);

          if (!recursive_upcall) {
            Printf(f->code, "  } else {\n");

            if (!pure_virtual) {
              Printf(f->code, "    SWIG_JavaThrowException(jenv, SWIG_JavaDirectorRicochet,\n");
              Printf(f->code, "      \"Recursive loop into director method %s::%s detected.\");\n",
                     proxy_class_name, Getattr(n, "name"));
            } else {
              Printf(f->code, "    SWIG_JavaThrowException(jenv, SWIG_JavaDirectorPureVirtual,\n");
              Printf(f->code, "      \"Pure virtual director method %s::%s invoked.\");\n",
                     proxy_class_name, Getattr(n, "name"));
            }

            Printf(f->code, "}");
          }

          Delete(upcall_method);
        } else {
          Swig_warning(WARN_JAVA_TYPEMAP_NODIRECTOR_CODE, input_file, line_number,
                       "Cannot determine Java method for %s, no upcall generated.\n", jdescrip);
        }

        Delete(jdescrip);
      } else {
        Swig_warning(WARN_TYPEMAP_DIRECTORIN_UNDEF, input_file, line_number, 
                     "No or improper directorin typemap defined for %s\n", c_return_type);
      }

      Printf(f->code, "  }\n");
    }

    /* Finish C function and intermediary class function definitions */
    Printf(imclass_class_code, ")");
    generateThrowsClause(n, imclass_class_code);
    Printf(imclass_class_code, ";\n");

    Printf(f->def,") {");

    if(!is_void_return)
      Printv(f->code, "    return jresult;\n", NIL);
    Printf(f->code, "}\n");

    /* Substitute the cleanup code */
    Replaceall(f->code,"$cleanup",cleanup);

    if(!is_void_return)
      Replaceall(f->code,"$null","0");
    else
      Replaceall(f->code,"$null","");

    /* Dump the function out */
    if(!native_function_flag)
      Wrapper_print(f,f_wrappers);

    Setattr(n,"wrap:name", wname);

    /* Emit warnings for the few cases that can't be overloaded in Java */
    if (Getattr(n,"sym:overloaded")) {
      if (!Getattr(n,"sym:nextSibling"))
        Swig_overload_rank(n);
    }

    if(!(proxy_flag && is_wrapping_class()) && !enum_constant_flag) {
      moduleClassFunctionHandler(n);
    }

    if (director_class)
      Delete(director_class);
    Delete(dirimclass_meth);
    Delete(c_return_type);
    Delete(im_return_type);
    Delete(cleanup);
    Delete(outarg);
    Delete(body);
    Delete(overloaded_name);
    DelWrapper(f);
    return SWIG_OK;
  }

  /* -----------------------------------------------------------------------
   * variableWrapper()
   * ----------------------------------------------------------------------- */

  virtual int variableWrapper(Node *n) {
    variable_wrapper_flag = true;
    Language::variableWrapper(n);           /* Default to functions */
    variable_wrapper_flag = false;
    return SWIG_OK;
  }

  /* -----------------------------------------------------------------------
   * globalvariableHandler()
   * ------------------------------------------------------------------------ */

  virtual int globalvariableHandler(Node *n) {

    variable_name = Getattr(n,"sym:name");
    global_variable_flag = true;
    int ret = Language::globalvariableHandler(n);
    global_variable_flag = false;
    return ret;
  }

  /* -----------------------------------------------------------------------
   * constantWrapper()
   * ------------------------------------------------------------------------ */

  virtual int constantWrapper(Node *n) {
    String *symname = Getattr(n,"sym:name");
    SwigType *t     = Getattr(n,"type");
    ParmList  *l    = Getattr(n,"parms");
    String *tm;
    String *return_type = NewString("");
    String *constants_code = NewString("");
    SwigType *original_type = t;

    if (!addSymbol(symname,n)) return SWIG_ERROR;

    bool is_enum_item = (Cmp(nodeType(n), "enumitem") == 0);

    /* Adjust the enum type for the Swig_typemap_lookup. We want the same jstype typemap for all the enum items.
     * The type of each enum item depends on what value it is assigned, but is usually a C int. */
    if (is_enum_item) {
      t = NewStringf("enum %s", Getattr(parentNode(n), "sym:name"));
      Setattr(n,"type", t);
    }

    /* Attach the non-standard typemaps to the parameter list. */
    Swig_typemap_attach_parms("jstype", l, NULL);

    /* Get Java return types */
    bool classname_substituted_flag = false;
    
    if ((tm = Swig_typemap_lookup_new("jstype",n,"",0))) {
      classname_substituted_flag = substituteClassname(t, tm);
      Printf(return_type, "%s", tm);
    } else {
      Swig_warning(WARN_JAVA_TYPEMAP_JSTYPE_UNDEF, input_file, line_number, 
          "No jstype typemap defined for %s\n", SwigType_str(t,0));
    }

    // Add the stripped quotes back in
    String *new_value = NewString("");
    Swig_save("constantWrapper",n,"value",NIL);
    if(SwigType_type(t) == T_STRING) {
      Printf(new_value, "\"%s\"", Copy(Getattr(n, "value")));
      Setattr(n, "value", new_value);
    }
    else if(SwigType_type(t) == T_CHAR) {
      Printf(new_value, "\'%s\'", Copy(Getattr(n, "value")));
      Setattr(n, "value", new_value);
    }

    // The %javaconst feature determines how the constant value is obtained
    String *const_feature = Getattr(n,"feature:java:const");
    bool const_feature_flag = const_feature && Cmp(const_feature, "0") != 0;

    // enums are wrapped using a public final static int in java.
    // Other constants are wrapped using a public final static [jstype] in Java.
    Printf(constants_code, "  public final static %s %s = ", return_type, ((proxy_flag && wrapping_member_flag) ? variable_name : symname));

    if ((is_enum_item && Getattr(n,"enumvalue") == 0) || !const_feature_flag) {
      // Enums without value and default constant handling will work with any type of C constant and initialises the Java variable from C through a JNI call.

      if(classname_substituted_flag) // This handles function pointers using the %constant directive
        Printf(constants_code, "new %s(%s.%s(), false);\n", return_type, imclass_name, Swig_name_get(symname));
      else
        Printf(constants_code, "%s.%s();\n", imclass_name, Swig_name_get(symname));

      // Each constant and enum value is wrapped with a separate JNI function call
      enum_constant_flag = true;
      variableWrapper(n);
      enum_constant_flag = false;
    } else if (is_enum_item) {
      // Alternative enum item handling will use the explicit value of the enum item and hope that it compiles as Java code
      Printf(constants_code, "%s;\n", Getattr(n,"enumvalue"));
    } else {
      // Alternative constant handling will use the C syntax to make a true Java constant and hope that it compiles as Java code
      Printf(constants_code, "%s;\n", Getattr(n,"value"));
    }

    if(proxy_flag && wrapping_member_flag)
      Printv(proxy_class_constants_code, constants_code, NIL);
    else
      Printv(module_class_constants_code, constants_code, NIL);

    Swig_restore(n);
    if (is_enum_item) {
      Delete(Getattr(n,"type"));
      Setattr(n,"type", original_type);
    }
    Delete(new_value);
    Delete(return_type);
    Delete(constants_code);
    return SWIG_OK;
  }

  /* -----------------------------------------------------------------------------
   * pragmaDirective()
   *
   * Valid Pragmas:
   * jniclassbase            - base (extends) for the intermediary class
   * jniclassclassmodifiers  - class modifiers for the intermediary class
   * jniclasscode            - text (java code) is copied verbatim to the intermediary class
   * jniclassimports         - import statements for the intermediary class
   * jniclassinterfaces      - interface (implements) for the intermediary class
   *
   * modulebase              - base (extends) for the module class
   * moduleclassmodifiers    - class modifiers for the module class
   * modulecode              - text (java code) is copied verbatim to the module class
   * moduleimports           - import statements for the module class
   * moduleinterfaces        - interface (implements) for the module class
   *
   * ----------------------------------------------------------------------------- */

  virtual int pragmaDirective(Node *n) {
    if (!ImportMode) {
      String *lang = Getattr(n,"lang");
      String *code = Getattr(n,"name");
      String *value = Getattr(n,"value");

      if(Strcmp(lang, "java") == 0) {

        String *strvalue = NewString(value);
        Replaceall(strvalue,"\\\"", "\"");

        if(Strcmp(code, "jniclassbase") == 0) {
          Delete(imclass_baseclass);
          imclass_baseclass = Copy(strvalue);
        } 
        else if(Strcmp(code, "jniclassclassmodifiers") == 0) {
          Delete(imclass_class_modifiers);
          imclass_class_modifiers = Copy(strvalue);
        } 
        else if(Strcmp(code, "jniclasscode") == 0) {
          Printf(imclass_class_code, "%s\n", strvalue);
        } 
        else if(Strcmp(code, "jniclassimports") == 0) {
          Delete(imclass_imports);
          imclass_imports = Copy(strvalue);
        } 
        else if(Strcmp(code, "jniclassinterfaces") == 0) {
          Delete(imclass_interfaces);
          imclass_interfaces = Copy(strvalue);
        } 
        else if(Strcmp(code, "modulebase") == 0) {
          Delete(module_baseclass);
          module_baseclass = Copy(strvalue);
        } 
        else if(Strcmp(code, "moduleclassmodifiers") == 0) {
          Delete(module_class_modifiers);
          module_class_modifiers = Copy(strvalue);
        } 
        else if(Strcmp(code, "modulecode") == 0) {
          Printf(module_class_code, "%s\n", strvalue);
        } 
        else if(Strcmp(code, "moduleimports") == 0) {
          Delete(module_imports);
          module_imports = Copy(strvalue);
        } 
        else if(Strcmp(code, "moduleinterfaces") == 0) {
          Delete(module_interfaces);
          module_interfaces = Copy(strvalue);
        } 
        else if(Strcmp(code, "moduleimport") == 0) {
          Printf(stderr,"%s : Line %d. Ignored: Deprecated pragma. Please use the moduleimports pragma.\n", input_file, line_number);
        } 
        else if(Strcmp(code, "moduleinterface") == 0) {
          Printf(stderr,"%s : Line %d. Ignored: Deprecated pragma. Please use the moduleinterfaces pragma.\n", input_file, line_number);
        } 
        else if(Strcmp(code, "modulemethodmodifiers") == 0) {
          Printf(stderr,"%s : Line %d. Ignored: Deprecated pragma. Please use %javamethodmodifiers.\n", input_file, line_number);
        } 
        else if(Strcmp(code, "allshadowimport") == 0) {
          Printf(stderr,"%s : Line %d. Ignored: Deprecated pragma. Please use %typemap(javaimports).\n", input_file, line_number);
        } 
        else if(Strcmp(code, "allshadowcode") == 0) {
          Printf(stderr,"%s : Line %d. Ignored: Deprecated pragma. Please use %typemap(javacode).\n", input_file, line_number);
        } 
        else if(Strcmp(code, "allshadowbase") == 0) {
          Printf(stderr,"%s : Line %d. Ignored: Deprecated pragma. Please use %typemap(javabase).\n", input_file, line_number);
        } 
        else if(Strcmp(code, "allshadowinterface") == 0) {
          Printf(stderr,"%s : Line %d. Ignored: Deprecated pragma. Please use %typemap(javainterfaces).\n", input_file, line_number);
        } 
        else if(Strcmp(code, "allshadowclassmodifiers") == 0) {
          Printf(stderr,"%s : Line %d. Ignored: Deprecated pragma. Please use %typemap(javaclassmodifiers).\n", input_file, line_number);
        } 
        else if (proxy_flag) {
          if (Strcmp(code,"shadowcode") == 0) {
            Printf(stderr,"%s : Line %d. Ignored: Deprecated pragma. Please use %typemap(javacode).\n", input_file, line_number);
          } 
          else if (Strcmp(code,"shadowimport") == 0) {
            Printf(stderr,"%s : Line %d. Ignored: Deprecated pragma. Please use %typemap(javaimports).\n", input_file, line_number);
          } 
          else if (Strcmp(code,"shadowbase") == 0) {
            Printf(stderr,"%s : Line %d. Ignored: Deprecated pragma. Please use %typemap(javabase).\n", input_file, line_number);
          } 
          else if (Strcmp(code,"shadowinterface") == 0) {
            Printf(stderr,"%s : Line %d. Ignored: Deprecated pragma. Please use %typemap(javainterfaces).\n", input_file, line_number);
          } 
          else if (Strcmp(code,"shadowclassmodifiers") == 0) {
            Printf(stderr,"%s : Line %d. Ignored: Deprecated pragma. Please use %typemap(javaclassmodifiers).\n", input_file, line_number);
          }  else {
            Printf(stderr,"%s : Line %d. Unrecognized pragma.\n", input_file, line_number);
          }
        } else {
          Printf(stderr,"%s : Line %d. Unrecognized pragma.\n", input_file, line_number);
        }
        Delete(strvalue);
      }
    }
    return Language::pragmaDirective(n);
  }

  /* -----------------------------------------------------------------------------
   * emitProxyClassDefAndCPPCasts()
   * ----------------------------------------------------------------------------- */

  void emitProxyClassDefAndCPPCasts(Node *n) {
    String *c_classname = SwigType_namestr(Getattr(n,"name"));
    String *c_baseclass = NULL;
    String *baseclass = NULL;
    String *c_baseclassname = NULL;
    String *classDeclarationName = Getattr(n,"classDeclaration:name");

    /* Deal with inheritance */
    List *baselist = Getattr(n,"bases");
    if (baselist) {
      Iterator base = First(baselist);
      c_baseclassname = Getattr(base.item,"name");
      baseclass = Copy(getProxyName(c_baseclassname));
      if (baseclass){
        c_baseclass = SwigType_namestr(Getattr(base.item,"name"));
      }
      base = Next(base);
      if (base.item) {
        Swig_warning(WARN_JAVA_MULTIPLE_INHERITANCE, input_file, line_number, 
            "Warning for %s proxy: Base %s ignored. Multiple inheritance is not supported in Java.\n", classDeclarationName, Getattr(base.item,"name"));
      }
    }

    bool derived = baseclass && getProxyName(c_baseclassname);
    if (!baseclass)
      baseclass = NewString("");

    // Inheritance from pure Java classes
    const String *pure_baseclass = typemapLookup("javabase", classDeclarationName, WARN_NONE);
    if (Len(pure_baseclass) > 0 && Len(baseclass) > 0) {
      Swig_warning(WARN_JAVA_MULTIPLE_INHERITANCE, input_file, line_number, 
          "Warning for %s proxy: Base %s ignored. Multiple inheritance is not supported in Java.\n", classDeclarationName, pure_baseclass);
    }

    // Pure Java interfaces
    const String *pure_interfaces = typemapLookup("javainterfaces", classDeclarationName, WARN_NONE);

    // Start writing the proxy class
    Printv(proxy_class_def,
        typemapLookup("javaimports", classDeclarationName, WARN_NONE), // Import statements
        "\n",
        typemapLookup("javaclassmodifiers", classDeclarationName, WARN_JAVA_TYPEMAP_CLASSMOD_UNDEF), // Class modifiers
        " class $javaclassname",       // Class name and bases
        (derived || *Char(pure_baseclass)) ?
        " extends " : 
        "",
        baseclass,
        pure_baseclass,
        *Char(pure_interfaces) ?  // Pure Java interfaces
        " implements " :
        "",
        pure_interfaces,
        " {\n",
        "  private long swigCPtr;\n",  // Member variables for memory handling
        derived ? 
        "" : 
        "  protected boolean swigCMemOwn;\n",
        "\n",
        "  ",
        typemapLookup("javaptrconstructormodifiers", classDeclarationName, WARN_JAVA_TYPEMAP_PTRCONSTMOD_UNDEF), // pointer constructor modifiers
        " $javaclassname(long cPtr, boolean cMemoryOwn) {\n", // Constructor used for wrapping pointers
        derived ? 
        "    super($imclassname.SWIG$javaclassnameTo$baseclass(cPtr), cMemoryOwn);\n" : 
        "    swigCMemOwn = cMemoryOwn;\n",
        "    swigCPtr = cPtr;\n",
        "  }\n",
        NIL);

    if(!have_default_constructor_flag) { // All proxy classes need a constructor
      Printv(proxy_class_def, 
          "\n",
          "  protected $javaclassname() {\n",
          "    this(0, false);\n", 
          "  }\n",
          NIL);
    }

    // C++ destructor is wrapped by the delete method
    // Note that the method name is specified in a typemap attribute called methodname
    String *destruct = NewString("");
    const String *tm = NULL;
    Node *attributes = NewHash();
    String *destruct_methodname = NULL;
    if (derived) {
      tm = typemapLookup("javadestruct_derived", classDeclarationName, WARN_NONE, attributes);
      destruct_methodname = Getattr(attributes, "tmap:javadestruct_derived:methodname");
    } else {
      tm = typemapLookup("javadestruct", classDeclarationName, WARN_NONE, attributes);
      destruct_methodname = Getattr(attributes, "tmap:javadestruct:methodname");
    }
    if (!destruct_methodname) {
      Swig_error(input_file, line_number, 
          "No methodname attribute defined in javadestruct%s typemap for %s\n", (derived ? "_derived" : ""), proxy_class_name);
    }

    // Emit the finalize and delete methods
    if (tm) {
      // Finalize method
      if (*Char(destructor_call)) {
        Printv(proxy_class_def, 
            typemapLookup("javafinalize", classDeclarationName, WARN_NONE),
            NIL);
      }
      // delete method
      Printv(destruct, tm, NIL);
      if (*Char(destructor_call))
        Replaceall(destruct, "$jnicall", destructor_call);
      else
        Replaceall(destruct, "$jnicall", "");
      if (*Char(destruct))
        Printv(proxy_class_def, "\n  ", *Char(destructor_call) ? "public": "protected", " void ", destruct_methodname, "() ", destruct, "\n", NIL);
    }
    Delete(attributes);
    Delete(destruct);

    // Emit various other methods
    Printv(proxy_class_def, 
        typemapLookup("javagetcptr", classDeclarationName, WARN_JAVA_TYPEMAP_GETCPTR_UNDEF), // getCPtr method
        typemapLookup("javacode", classDeclarationName, WARN_NONE), // extra Java code
        "\n",
        NIL);

    /* Insert declaration for swig_director_connect(), if this class has directors enabled */
    if (parentNode(n) && Swig_directorclass(n)) {
      Printf(proxy_class_def, "  protected void swigDirectorDisconnect() {\n");
      Printf(proxy_class_def, "    swigCMemOwn = false;\n");
      Printf(proxy_class_def, "    %s();\n", destruct_methodname);
      Printf(proxy_class_def, "  }\n\n");

      /* Emit for the class itself */
      Printf(proxy_class_def, "  public static %s javaObject%s(%s obj) {\n", proxy_class_name, proxy_class_name, proxy_class_name);
      Printf(proxy_class_def, "    return (%s) %s.javaObject%s(%s.getCPtr(obj));\n", proxy_class_name, imclass_name,
             proxy_class_name, proxy_class_name);
      Printf(proxy_class_def, "  }\n\n");

      /* Walk through inheritance hierarchy (hack: we know that only the first
         base in list is significant) */

      List *bases = Getattr(n, "bases");
      while (bases != NULL) {
        Iterator base = First(bases);
        Node *bn = base.item;

        if (bn != NULL) {
          String *base_name = Getattr(bn, "sym:name");

          Printf(proxy_class_def, "  public static %s javaObject%s(%s obj) {\n", proxy_class_name, proxy_class_name, base_name);
          Printf(proxy_class_def, "    return (%s) %s.javaObject%s(%s.getCPtr(obj));\n", proxy_class_name, imclass_name,
                 proxy_class_name, base_name);
          Printf(proxy_class_def, "  }\n\n");

          bases = Getattr(bn, "bases");
        } else
          bases = NULL;
      }
    }

    // Substitute various strings into the above template
    Replaceall(proxy_class_code, "$javaclassname", proxy_class_name);
    Replaceall(proxy_class_def,  "$javaclassname", proxy_class_name);

    Replaceall(proxy_class_def,  "$baseclass", baseclass);
    Replaceall(proxy_class_code, "$baseclass", baseclass);

    Replaceall(proxy_class_def,  "$imclassname", imclass_name);
    Replaceall(proxy_class_code, "$imclassname", imclass_name);

    // Add code to do C++ casting to base class (only for classes in an inheritance hierarchy)
    if(derived){
      Printv(imclass_cppcasts_code,"  public final static native long ",
          "SWIG$javaclassnameTo$baseclass(long jarg1);\n",
          NIL);

      Replaceall(imclass_cppcasts_code, "$javaclassname", proxy_class_name);
      Replaceall(imclass_cppcasts_code, "$baseclass", baseclass);

      Printv(upcasts_code,
          "JNIEXPORT jlong JNICALL Java_$jnipackage$imimclass_SWIG$imclazznameTo$imbaseclass",
          "(JNIEnv *jenv, jclass jcls, jlong jarg1) {\n",
          "    jlong baseptr = 0;\n"
          "    (void)jenv;\n"
          "    (void)jcls;\n"
          "    *($cbaseclass **)&baseptr = *($cclass **)&jarg1;\n"
          "    return baseptr;\n"
          "}\n",
          "\n",
          NIL); 

      String *imimclass  = makeValidJniName(imclass_name);
      String *imclazzname = makeValidJniName(proxy_class_name);
      String *imbaseclass = makeValidJniName(baseclass);
      Replaceall(upcasts_code, "$imbaseclass", imbaseclass);
      Replaceall(upcasts_code, "$cbaseclass",  c_baseclass);
      Replaceall(upcasts_code, "$imclazzname", imclazzname);
      Replaceall(upcasts_code, "$cclass",      c_classname);
      Replaceall(upcasts_code, "$jnipackage",  jnipackage);
      Replaceall(upcasts_code, "$imimclass",   imimclass);

      Delete(imbaseclass);
      Delete(imclazzname);
      Delete(imimclass);
    }
    Delete(baseclass);
  }

  /* ----------------------------------------------------------------------
   * classHandler()
   * ---------------------------------------------------------------------- */

  virtual int classHandler(Node *n) {

    File *f_proxy = NULL;
    if (proxy_flag) {
      proxy_class_name = NewString(Getattr(n,"sym:name"));

      if (!addSymbol(proxy_class_name,n)) return SWIG_ERROR;

      if (Cmp(proxy_class_name, imclass_name) == 0) {
        Printf(stderr, "Class name cannot be equal to intermediary class name: %s\n", proxy_class_name);
        SWIG_exit(EXIT_FAILURE);
      }

      if (Cmp(proxy_class_name, module_class_name) == 0) {
        Printf(stderr, "Class name cannot be equal to module class name: %s\n", proxy_class_name);
        SWIG_exit(EXIT_FAILURE);
      }

      String *filen = NewStringf("%s%s.java", SWIG_output_directory(), proxy_class_name);
      f_proxy = NewFile(filen,"w");
      if(!f_proxy) {
        Printf(stderr, "Unable to create proxy class file: %s\n", filen);
        SWIG_exit(EXIT_FAILURE);
      }
      Delete(filen); filen = NULL;

      emitBanner(f_proxy);

      if(Len(package) > 0)
        Printf(f_proxy, "package %s;\n\n", package);

      Clear(proxy_class_def);
      Clear(proxy_class_code);

      have_default_constructor_flag = false;
      destructor_call = NewString("");
      proxy_class_constants_code = NewString("");
    }

    Language::classHandler(n);

    if (proxy_flag) {

      emitProxyClassDefAndCPPCasts(n);

      Printv(f_proxy, proxy_class_def, proxy_class_code, NIL);

      // Write out all the enums and constants
      if (Len(proxy_class_constants_code) != 0 )
        Printv(f_proxy, "  // enums and constants\n", proxy_class_constants_code, NIL);

      Printf(f_proxy, "}\n");
      Close(f_proxy);
      f_proxy = NULL;

      Delete(proxy_class_name); proxy_class_name = NULL;
      Delete(destructor_call); destructor_call = NULL;
      Delete(proxy_class_constants_code); proxy_class_constants_code = NULL;
    }

    /* Reinitialize per-class director vars */
    emitted_connect = false;

    return SWIG_OK;
  }

  /* ----------------------------------------------------------------------
   * memberfunctionHandler()
   * ---------------------------------------------------------------------- */

  virtual int memberfunctionHandler(Node *n) {
    member_func_flag = true;
    Language::memberfunctionHandler(n);

    if (proxy_flag) {
      String *overloaded_name = getOverloadedName(n);
      String *intermediary_function_name = Swig_name_member(proxy_class_name, overloaded_name);
      Setattr(n,"proxyfuncname", Getattr(n, "sym:name"));
      Setattr(n,"imfuncname", intermediary_function_name);
      proxyClassFunctionHandler(n);
      Delete(overloaded_name);
    }
    member_func_flag = false;
    return SWIG_OK;
  }

  /* ----------------------------------------------------------------------
   * staticmemberfunctionHandler()
   * ---------------------------------------------------------------------- */

  virtual int staticmemberfunctionHandler(Node *n) {

    static_flag = true;
    member_func_flag = true;
    Language::staticmemberfunctionHandler(n);

    if (proxy_flag) {
      String *overloaded_name = getOverloadedName(n);
      String *intermediary_function_name = Swig_name_member(proxy_class_name, overloaded_name);
      Setattr(n,"proxyfuncname", Getattr(n,"sym:name"));
      Setattr(n,"imfuncname", intermediary_function_name);
      proxyClassFunctionHandler(n);
      Delete(overloaded_name);
    }
    static_flag = false;
    member_func_flag = false;

    return SWIG_OK;
  }

  /* -----------------------------------------------------------------------------
   * proxyClassFunctionHandler()
   *
   * Function called for creating a Java wrapper function around a c++ function in the 
   * proxy class. Used for both static and non-static C++ class functions.
   * C++ class static functions map to Java static functions.
   * Two extra attributes in the Node must be available. These are "proxyfuncname" - 
   * the name of the Java class proxy function, which in turn will call "imfuncname" - 
   * the intermediary (JNI) function name in the intermediary class.
   * ----------------------------------------------------------------------------- */

  void proxyClassFunctionHandler(Node *n) {
    SwigType  *t = Getattr(n,"type");
    ParmList  *l = Getattr(n,"parms");
    String    *intermediary_function_name = Getattr(n,"imfuncname");
    String    *proxy_function_name = Getattr(n,"proxyfuncname");
    String    *tm;
    Parm      *p;
    int       i;
    String    *imcall = NewString("");
    String    *return_type = NewString("");
    String    *function_code = NewString("");

    if(!proxy_flag) return;

    if (l) {
      if (SwigType_type(Getattr(l,"type")) == T_VOID) {
        l = nextSibling(l);
      }
    }

    /* Attach the non-standard typemaps to the parameter list */
    Swig_typemap_attach_parms("in", l, NULL);
    Swig_typemap_attach_parms("jstype", l, NULL);
    Swig_typemap_attach_parms("javain", l, NULL);

    /* Get return types */
    if ((tm = Swig_typemap_lookup_new("jstype",n,"",0))) {
      substituteClassname(t, tm);
      Printf(return_type, "%s", tm);
    } else {
      Swig_warning(WARN_JAVA_TYPEMAP_JSTYPE_UNDEF, input_file, line_number, 
          "No jstype typemap defined for %s\n", SwigType_str(t,0));
    }

    /* Start generating the proxy function */
    Printf(function_code, "  %s ", Getattr(n,"feature:java:methodmodifiers"));
    if (static_flag)
      Printf(function_code, "static ");
    Printf(function_code, "%s %s(", return_type, proxy_function_name);

    Printv(imcall, imclass_name, ".", intermediary_function_name, "(", NIL);
    if (!static_flag)
      Printf(imcall, "swigCPtr");

    emit_mark_varargs(l);

    int        gencomma = !static_flag;

    /* Output each parameter */
    for (i = 0, p=l; p; i++) {

      /* Ignored varargs */
      if (checkAttribute(p,"varargs:ignore","1")) {
        p = nextSibling(p);
        continue;
      }

      /* Ignored parameters */
      if (checkAttribute(p,"tmap:in:numinputs","0")) {
        p = Getattr(p,"tmap:in:next");
        continue;
      }

      /* Ignore the 'this' argument for variable wrappers */
      if (!(variable_wrapper_flag && i==0) || static_flag)
      {
        SwigType *pt = Getattr(p,"type");
        String   *param_type = NewString("");

        /* Get the Java parameter type */
        if ((tm = Getattr(p,"tmap:jstype"))) {
          substituteClassname(pt, tm);
          Printf(param_type, "%s", tm);
        } else {
          Swig_warning(WARN_JAVA_TYPEMAP_JSTYPE_UNDEF, input_file, line_number, 
              "No jstype typemap defined for %s\n", SwigType_str(pt,0));
        }

        if (gencomma)
          Printf(imcall, ", ");

        String *arg = makeParameterName(n, p, i);

        // Use typemaps to transform type used in Java proxy wrapper (in proxy class) to type used in JNI function (in intermediary class)
        if ((tm = Getattr(p,"tmap:javain"))) {
          addThrows(n, "tmap:javain", p);
          substituteClassname(pt, tm);
          Replaceall(tm, "$javainput", arg);
          Printv(imcall, tm, NIL);
        } else {
          Swig_warning(WARN_JAVA_TYPEMAP_JAVAIN_UNDEF, input_file, line_number, 
              "No javain typemap defined for %s\n", SwigType_str(pt,0));
        }

        /* Add parameter to proxy function */
        if (gencomma >= 2)
          Printf(function_code, ", ");
        gencomma = 2;
        Printf(function_code, "%s %s", param_type, arg);

        Delete(arg);
        Delete(param_type);
      }
      p = Getattr(p,"tmap:in:next");
    }

    Printf(imcall, ")");
    Printf(function_code, ")");

    // Transform return type used in JNI function (in intermediary class) to type used in Java wrapper function (in proxy class)
    if ((tm = Swig_typemap_lookup_new("javaout",n,"",0))) {
      addThrows(n, "tmap:javaout", n);
      if (Getattr(n,"feature:new"))
        Replaceall(tm,"$owner","true");
      else
        Replaceall(tm,"$owner","false");
      substituteClassname(t, tm);
      Replaceall(tm, "$jnicall", imcall);
    } else {
      Swig_warning(WARN_JAVA_TYPEMAP_JAVAOUT_UNDEF, input_file, line_number, 
          "No javaout typemap defined for %s\n", SwigType_str(t,0));
    }

    generateThrowsClause(n, function_code);
    Printf(function_code, " %s\n\n", tm ? tm : empty_string);
    Printv(proxy_class_code, function_code, NIL);

    Delete(function_code);
    Delete(return_type);
    Delete(imcall);
  }

  /* ----------------------------------------------------------------------
   * constructorHandler()
   * ---------------------------------------------------------------------- */

  virtual int constructorHandler(Node *n) {

    ParmList *l = Getattr(n,"parms");
    String    *tm;
    Parm      *p;
    int       i;
    bool      feature_director = (parentNode(n) && Swig_directorclass(n));

    Language::constructorHandler(n);

    if(proxy_flag) {
      String *overloaded_name = getOverloadedName(n);
      String *mangled_overname = Swig_name_construct(overloaded_name);
      String *imcall = NewString("");

      Printf(proxy_class_code, "  %s %s(", Getattr(n,"feature:java:methodmodifiers"), proxy_class_name);
      Printv(imcall, "this(", imclass_name, ".", mangled_overname, "(", NIL);

      /* Attach the non-standard typemaps to the parameter list */
      Swig_typemap_attach_parms("in", l, NULL);
      Swig_typemap_attach_parms("jstype", l, NULL);
      Swig_typemap_attach_parms("javain", l, NULL);

      emit_mark_varargs(l);

      int gencomma = 0;
      int ctor_arg_cnt = 0;

      /* Output each parameter */
      for (i = 0, p=l; p; i++) {

        /* Ignored varargs */
        if (checkAttribute(p,"varargs:ignore","1")) {
          p = nextSibling(p);
          continue;
        }

        /* Ignored parameters */
        if (checkAttribute(p,"tmap:in:numinputs","0")) {
          p = Getattr(p,"tmap:in:next");
          continue;
        }

        SwigType *pt = Getattr(p,"type");
        String   *param_type = NewString("");

        /* Get the Java parameter type */
        if ((tm = Getattr(p,"tmap:jstype"))) {
          substituteClassname(pt, tm);
          Printf(param_type, "%s", tm);
        } else {
          Swig_warning(WARN_JAVA_TYPEMAP_JSTYPE_UNDEF, input_file, line_number, 
              "No jstype typemap defined for %s\n", SwigType_str(pt,0));
        }

        if (gencomma)
          Printf(imcall, ", ");

        String *arg = makeParameterName(n, p, i);

        // Use typemaps to transform type used in Java wrapper function (in proxy class) to type used in JNI function (in intermediary class)
        if ((tm = Getattr(p,"tmap:javain"))) {
          addThrows(n, "tmap:javain", p);
          substituteClassname(pt, tm);
          Replaceall(tm, "$javainput", arg);
          Printv(imcall, tm, NIL);
        } else {
          Swig_warning(WARN_JAVA_TYPEMAP_JAVAIN_UNDEF, input_file, line_number, 
              "No javain typemap defined for %s\n", SwigType_str(pt,0));
        }

        /* Add parameter to proxy function */
        if(gencomma)
          Printf(proxy_class_code, ", ");
        Printf(proxy_class_code, "%s %s", param_type, arg);
        ++gencomma;
        ctor_arg_cnt++;

        Delete(arg);
        Delete(param_type);
        p = Getattr(p,"tmap:in:next");
      }

      Printf(imcall, "), true);\n");

      Printf(proxy_class_code, ")");
      generateThrowsClause(n, proxy_class_code);
      Printf(proxy_class_code, " {\n");
      Printf(proxy_class_code, "    %s", imcall);

      /* Add director connection call if this class has directors. */

      if (feature_director) {
        String *swig_director_connect = NewStringf("%s_director_connect", proxy_class_name);
        Printv(proxy_class_code, "    ", imclass_name, ".", swig_director_connect, "(this, swigCPtr);\n", NIL);

        if (!emitted_connect) {
          String  *jni_imclass_name = makeValidJniName(imclass_name);
          String  *jni_class_name = makeValidJniName(proxy_class_name);
          String  *swig_director_connect_jni = makeValidJniName(swig_director_connect);
          String  *norm_name = SwigType_namestr(Getattr(n, "name"));
          Wrapper *conn_wrap;

          Printf(imclass_class_code, "  public final static native void %s(%s obj, long cptr);\n", swig_director_connect,
                 proxy_class_name);

          conn_wrap = NewWrapper();
          Printf(conn_wrap->def, "JNIEXPORT void JNICALL Java_%s%s_%s(JNIEnv *jenv, jclass jcls, jobject jself, jlong objarg) {",
                 jnipackage, jni_imclass_name, swig_director_connect_jni);
          Printf(conn_wrap->code, "  %s *obj = *((%s **) &objarg);\n", norm_name, norm_name);
          Printf(conn_wrap->code, "  SwigDirector_%s *director = dynamic_cast<SwigDirector_%s *>(obj);\n",
                 Getattr(n, "sym:name"), Getattr(n, "sym:name"));
          Printf(conn_wrap->code, "  if (director) {\n");
          Printf(conn_wrap->code, "    director->swig_connect_director(jenv, jself, jenv->GetObjectClass(jself));\n");
          Printf(conn_wrap->code, "  }\n");
          Printf(conn_wrap->code, "}\n");

          Wrapper_print(conn_wrap, f_wrappers);
          DelWrapper(conn_wrap);

          Printf(imclass_class_code, "  public final static native %s javaObject%s(long cptr);\n", proxy_class_name,
                 proxy_class_name);

          Wrapper *self_wrap = NewWrapper();

          Printf(self_wrap->def, "JNIEXPORT jobject JNICALL Java_%s%s_javaObject%s(JNIEnv *jenv, jclass jcls, jlong objarg) {",
                 jnipackage, jni_imclass_name, jni_class_name);
          Printf(self_wrap->code, "  %s *obj = *((%s **) &objarg);\n", norm_name, norm_name);
          Printf(self_wrap->code, "  SwigDirector_%s *director = dynamic_cast<SwigDirector_%s *>(obj);\n",
                 Getattr(n, "sym:name"), Getattr(n, "sym:name"));
          Printf(self_wrap->code, "  return (director != NULL ? director->swig_get_self() : NULL);\n");
          Printf(self_wrap->code, "}\n");

          Wrapper_print(self_wrap, f_wrappers);
          DelWrapper(self_wrap);

          Delete(swig_director_connect_jni);
          Delete(norm_name);
          Delete(jni_class_name);
          Delete(jni_imclass_name);
          emitted_connect = true;
        }
        Delete(swig_director_connect);
      }

      Printf(proxy_class_code, "  }\n\n");

      if(!ctor_arg_cnt)  // We must have a default constructor
        have_default_constructor_flag = true;

      Delete(overloaded_name);
      Delete(imcall);
    }

    return SWIG_OK;
  }

  /* ----------------------------------------------------------------------
   * destructorHandler()
   * ---------------------------------------------------------------------- */

  virtual int destructorHandler(Node *n) {
    Language::destructorHandler(n);
    String *symname = Getattr(n,"sym:name");

    if(proxy_flag) {
      Printv(destructor_call, imclass_name, ".", Swig_name_destroy(symname), "(swigCPtr)", NIL);
    }
    return SWIG_OK;
  }

  /* ----------------------------------------------------------------------
   * membervariableHandler()
   * ---------------------------------------------------------------------- */

  virtual int membervariableHandler(Node *n) {
    variable_name = Getattr(n,"sym:name");
    wrapping_member_flag = true;
    variable_wrapper_flag = true;
    Language::membervariableHandler(n);
    wrapping_member_flag = false;
    variable_wrapper_flag = false;
    return SWIG_OK;
  }

  /* ----------------------------------------------------------------------
   * staticmembervariableHandler()
   * ---------------------------------------------------------------------- */

  virtual int staticmembervariableHandler(Node *n) {
    variable_name = Getattr(n,"sym:name");
    wrapping_member_flag = true;
    static_flag = true;
    Language::staticmembervariableHandler(n);
    wrapping_member_flag = false;
    static_flag = false;
    return SWIG_OK;
  }

  /* ----------------------------------------------------------------------
   * memberconstantHandler()
   * ---------------------------------------------------------------------- */

  virtual int memberconstantHandler(Node *n) {
    variable_name = Getattr(n,"sym:name");
    wrapping_member_flag = true;
    Language::memberconstantHandler(n);
    wrapping_member_flag = false;
    return SWIG_OK;
  }

  /* -----------------------------------------------------------------------------
   * getOverloadedName()
   * ----------------------------------------------------------------------------- */

  String *getOverloadedName(Node *n) {

    /* Although JNI functions are designed to handle overloaded Java functions,
     * a Java long is used for all classes in the SWIG intermediary class.
     * The intermediary class methods are thus mangled when overloaded to give
     * a unique name. */

    String *overloaded_name = NewStringf("%s", Getattr(n,"sym:name"));

    if (Getattr(n,"sym:overloaded")) {
      Printv(overloaded_name, Getattr(n,"sym:overname"), NIL);
    }

    return overloaded_name;
  }

  /* -----------------------------------------------------------------------------
   * moduleClassFunctionHandler()
   * ----------------------------------------------------------------------------- */

  void moduleClassFunctionHandler(Node *n) {
    SwigType  *t = Getattr(n,"type");
    ParmList  *l = Getattr(n,"parms");
    String    *tm;
    Parm      *p;
    int       i;
    String    *imcall = NewString("");
    String    *return_type = NewString("");
    String    *function_code = NewString("");
    int       num_arguments = 0;
    int       num_required = 0;
    String    *overloaded_name = getOverloadedName(n);
    String    *func_name = NULL;
    bool      setter_flag = false;

    if (l) {
      if (SwigType_type(Getattr(l,"type")) == T_VOID) {
        l = nextSibling(l);
      }
    }

    /* Attach the non-standard typemaps to the parameter list */
    Swig_typemap_attach_parms("jstype", l, NULL);
    Swig_typemap_attach_parms("javain", l, NULL);

    /* Get return types */
    if ((tm = Swig_typemap_lookup_new("jstype",n,"",0))) {
      substituteClassname(t, tm);
      Printf(return_type, "%s", tm);
    } else {
      Swig_warning(WARN_JAVA_TYPEMAP_JSTYPE_UNDEF, input_file, line_number, 
          "No jstype typemap defined for %s\n", SwigType_str(t,0));
    }

    /* Change function name for global variables */
    if (proxy_flag && global_variable_flag) {
      // Capitalize the first letter in the variable to create a JavaBean type getter/setter function name
      func_name = NewString("");
      setter_flag = (Cmp(Getattr(n,"sym:name"), Swig_name_set(variable_name)) == 0);
      if(setter_flag)
        Printf(func_name,"set");
      else 
        Printf(func_name,"get");
      Putc(toupper((int) *Char(variable_name)), func_name);
      Printf(func_name, "%s", Char(variable_name)+1);
    } else {
      func_name = Copy(Getattr(n,"sym:name"));
    }

    /* Start generating the function */
    Printf(function_code, "  %s static %s %s(", Getattr(n,"feature:java:methodmodifiers"), return_type, func_name);
    Printv(imcall, imclass_name, ".", overloaded_name, "(", NIL);

    /* Get number of required and total arguments */
    num_arguments = emit_num_arguments(l);
    num_required  = emit_num_required(l);

    int gencomma = 0;

    /* Output each parameter */
    for (i = 0, p=l; i < num_arguments; i++) {

      /* Ignored parameters */
      while (checkAttribute(p,"tmap:in:numinputs","0")) {
        p = Getattr(p,"tmap:in:next");
      }

      SwigType *pt = Getattr(p,"type");
      String   *param_type = NewString("");

      /* Get the Java parameter type */
      if ((tm = Getattr(p,"tmap:jstype"))) {
        substituteClassname(pt, tm);
        Printf(param_type, "%s", tm);
      } else {
        Swig_warning(WARN_JAVA_TYPEMAP_JSTYPE_UNDEF, input_file, line_number, 
            "No jstype typemap defined for %s\n", SwigType_str(pt,0));
      }

      if (gencomma)
        Printf(imcall, ", ");

      String *arg = makeParameterName(n, p, i);

      // Use typemaps to transform type used in Java wrapper function (in proxy class) to type used in JNI function (in intermediary class)
      if ((tm = Getattr(p,"tmap:javain"))) {
        addThrows(n, "tmap:javain", p);
        substituteClassname(pt, tm);
        Replaceall(tm, "$javainput", arg);
        Printv(imcall, tm, NIL);
      } else {
        Swig_warning(WARN_JAVA_TYPEMAP_JAVAIN_UNDEF, input_file, line_number, 
            "No javain typemap defined for %s\n", SwigType_str(pt,0));
      }

      /* Add parameter to module class function */
      if (gencomma >= 2)
        Printf(function_code, ", ");
      gencomma = 2;
      Printf(function_code, "%s %s", param_type, arg);

      p = Getattr(p,"tmap:in:next");
      Delete(arg);
      Delete(param_type);
    }

    Printf(imcall, ")");
    Printf(function_code, ")");

    // Transform return type used in JNI function (in intermediary class) to type used in Java wrapper function (in module class)
    if ((tm = Swig_typemap_lookup_new("javaout",n,"",0))) {
      addThrows(n, "tmap:javaout", n);
      if (Getattr(n,"feature:new"))
        Replaceall(tm,"$owner","true");
      else
        Replaceall(tm,"$owner","false");
      substituteClassname(t, tm);
      Replaceall(tm, "$jnicall", imcall);
    } else {
      Swig_warning(WARN_JAVA_TYPEMAP_JAVAOUT_UNDEF, input_file, line_number, 
          "No javaout typemap defined for %s\n", SwigType_str(t,0));
    }

    generateThrowsClause(n, function_code);
    Printf(function_code, " %s\n\n", tm ? tm : empty_string);
    Printv(module_class_code, function_code, NIL);

    Delete(function_code);
    Delete(return_type);
    Delete(imcall);
    Delete(func_name);
  }

  /* -----------------------------------------------------------------------------
   * substituteClassname()
   *
   * Substitute $javaclassname with the proxy class name for classes/structs/unions that SWIG knows about.
   * Otherwise use the $descriptor name for the Java class name. Note that the $&javaclassname substitution
   * is the same as a $&descriptor substitution, ie one pointer added to descriptor name.
   * Inputs:
   *   pt - parameter type
   *   tm - jstype typemap
   * Outputs:
   *   tm - jstype typemap with $javaclassname substitution
   * Return:
   *   substitution_performed - flag indicating if a substitution was performed
   * ----------------------------------------------------------------------------- */

  bool substituteClassname(SwigType *pt, String *tm) {
    bool substitution_performed = false;
    if (Strstr(tm, "$javaclassname") || Strstr(tm,"$&javaclassname")) {
      String *classname = getProxyName(pt);
      if (classname) {
        Replaceall(tm,"$&javaclassname", classname); // getProxyName() works for pointers to classes too
        Replaceall(tm,"$javaclassname", classname);
      }
      else { // use $descriptor if SWIG does not know anything about this type. Note that any typedefs are resolved.
        String *descriptor = NULL;
        SwigType *type = Copy(SwigType_typedef_resolve_all(pt));

        if (Strstr(tm, "$&javaclassname")) {
          SwigType_add_pointer(type);
          descriptor = NewStringf("SWIGTYPE%s", SwigType_manglestr(type));
          Replaceall(tm, "$&javaclassname", descriptor);
        }
        else { // $javaclassname
          descriptor = NewStringf("SWIGTYPE%s", SwigType_manglestr(type));
          Replaceall(tm, "$javaclassname", descriptor);
        }

        // Add to hash table so that the type wrapper classes can be created later
        Setattr(swig_types_hash, descriptor, type);
        Delete(descriptor);
        Delete(type);
      }
      substitution_performed = true;
    }
    return substitution_performed;
  }

  /* -----------------------------------------------------------------------------
   * makeParameterName()
   *
   * Inputs: 
   *   n - Node
   *   p - parameter node
   *   arg_num - parameter argument number
   * Return:
   *   arg - a unique parameter name
   * ----------------------------------------------------------------------------- */

  String *makeParameterName(Node *n, Parm *p, int arg_num) {

    // Use C parameter name unless it is a duplicate or an empty parameter name
    String   *pn = Getattr(p,"name");
    int count = 0;
    ParmList *plist = Getattr(n,"parms");
    while (plist) {
      if ((Cmp(pn, Getattr(plist,"name")) == 0))
        count++;
      plist = nextSibling(plist);
    }
    String *arg = (!pn || (count > 1)) ? NewStringf("arg%d",arg_num) : Copy(Getattr(p,"name"));

    return arg;
  }

  /* -----------------------------------------------------------------------------
   * emitTypeWrapperClass()
   * ----------------------------------------------------------------------------- */

  void emitTypeWrapperClass(String *classname, SwigType *type) {
    String *filen = NewStringf("%s%s.java", SWIG_output_directory(), classname);
    File *f_swigtype = NewFile(filen,"w");
    String *swigtype = NewString("");

    // Emit banner and package name
    emitBanner(f_swigtype);
    if(Len(package) > 0)
      Printf(f_swigtype, "package %s;\n\n", package);

    // Pure Java baseclass and interfaces
    const String *pure_baseclass = typemapLookup("javabase", type, WARN_NONE);
    const String *pure_interfaces = typemapLookup("javainterfaces", type, WARN_NONE);

    // Emit the class
    Printv(swigtype,
        typemapLookup("javaimports", type, WARN_NONE), // Import statements
        "\n",
        typemapLookup("javaclassmodifiers", type, WARN_JAVA_TYPEMAP_CLASSMOD_UNDEF), // Class modifiers
        " class $javaclassname",       // Class name and bases
        *Char(pure_baseclass) ?
        " extends " : 
        "",
        pure_baseclass,
        *Char(pure_interfaces) ?  // Interfaces
        " implements " :
        "",
        pure_interfaces,
        " {\n",
        "  private long swigCPtr;\n",
        "\n",
        "  ",
        typemapLookup("javaptrconstructormodifiers", type, WARN_JAVA_TYPEMAP_PTRCONSTMOD_UNDEF), // pointer constructor modifiers
        " $javaclassname(long cPtr, boolean bFutureUse) {\n", // Constructor used for wrapping pointers
        "    swigCPtr = cPtr;\n",
        "  }\n",
        "\n",
        "  protected $javaclassname() {\n", // Default constructor
        "    swigCPtr = 0;\n",
        "  }\n",
        typemapLookup("javagetcptr", type, WARN_JAVA_TYPEMAP_GETCPTR_UNDEF), // getCPtr method
        typemapLookup("javacode", type, WARN_NONE), // extra Java code
        "}\n",
        "\n",
        NIL);

        Replaceall(swigtype, "$javaclassname", classname);
        Printv(f_swigtype, swigtype, NIL);

        Close(f_swigtype);
        Delete(filen);
        Delete(swigtype);
  }

  /* -----------------------------------------------------------------------------
   * typemapLookup()
   * ----------------------------------------------------------------------------- */

  const String *typemapLookup(const String *op, String *type, int warning, Node *typemap_attributes=NULL) {
    String *tm = NULL;
    const String *code = NULL;

    if((tm = Swig_typemap_search(op, type, NULL, NULL))) {
      code = Getattr(tm,"code");
      if (typemap_attributes)
        Swig_typemap_attach_kwargs(tm,op,typemap_attributes);
    }

    if (!code) {
      code = empty_string;
      if (warning != WARN_NONE)
        Swig_warning(warning, input_file, line_number, "No %s typemap defined for %s\n", op, type);
    }

    return code ? code : empty_string;
  }

  /* -----------------------------------------------------------------------------
   * addThrows()
   * ----------------------------------------------------------------------------- */

  void addThrows(Node *n, const String *typemap, Node *parameter) {
    // Get the comma separated throws clause - held in "throws" attribute in the typemap passed in
    String *throws_attribute = NewStringf("%s:throws", typemap);
    String *throws = Getattr(parameter,throws_attribute);

    if (throws) {
      String *throws_list = Getattr(n,"java:throwslist");
      if (!throws_list) {
        throws_list = NewList();
        Setattr(n,"java:throwslist", throws_list);
      }

      // Put the exception classes in the throws clause into a temporary List
      List *temp_classes_list = Split(throws,',',INT_MAX);

      // Add the exception classes to the node throws list, but don't duplicate if already in list
      if (temp_classes_list && Len(temp_classes_list) > 0) {
        for (Iterator cls = First(temp_classes_list); cls.item; cls = Next(cls)) {
          String *exception_class = NewString(cls.item);
          Replaceall(exception_class," ","");  // remove spaces
          Replaceall(exception_class,"\t",""); // remove tabs
          if (Len(exception_class) > 0) {
            // $javaclassname substitution
            SwigType *pt = Getattr(parameter,"type");
            substituteClassname(pt, exception_class);

            // Don't duplicate the Java exception class in the throws clause
            bool found_flag = false;
            for (Iterator item = First(throws_list); item.item; item = Next(item)) {
              if (Strcmp(item.item, exception_class) == 0)
                found_flag = true;
            }
            if (!found_flag)
              Append(throws_list, exception_class);
          }
          Delete(exception_class);
        } 
      }
      Delete(temp_classes_list);
    } 
    Delete(throws_attribute);
  }

  /* -----------------------------------------------------------------------------
   * generateThrowsClause()
   * ----------------------------------------------------------------------------- */

  void generateThrowsClause(Node *n, String *code) {
    // Add the throws clause into code
    List *throws_list = Getattr(n,"java:throwslist");
    if (throws_list) {
      Iterator cls = First(throws_list);
      Printf(code, " throws %s", cls.item);
      while ( (cls = Next(cls)).item)
        Printf(code, ", %s", cls.item);
    }
  }

  /*----------------------------------------------------------------------
   * getUpcallJNIMethod()
   *--------------------------------------------------------------------*/

  String * getUpcallJNIMethod(String *descrip)
  {
    static struct {
      char      code;
      const char *method;
    } upcall_methods[] = {
      { 'B', "CallStaticByteMethod" },
      { 'C', "CallStaticCharMethod" },
      { 'D', "CallStaticDoubleMethod" },
      { 'F', "CallStaticFloatMethod" },
      { 'I', "CallStaticIntMethod" },
      { 'J', "CallStaticLongMethod" },
      { 'L', "CallStaticObjectMethod" },
      { 'S', "CallStaticShortMethod" },
      { 'V', "CallStaticVoidMethod" },
      { 'Z', "CallStaticBooleanMethod" },
      { '[', "CallStaticObjectMethod" }
    };

    char code;
    int i;

    code = *Char(descrip);
    for (i = 0; i < (int) (sizeof(upcall_methods)/sizeof(upcall_methods[0])); ++i)
      if (code == upcall_methods[i].code)
        return NewString(upcall_methods[i].method);
    return NULL;
  }

  /*----------------------------------------------------------------------
   * emitDirectorUpcalls()
   *--------------------------------------------------------------------*/

  void emitDirectorUpcalls()
  {
    if (n_dmethods) {
      Wrapper          *w = NewWrapper();
      String           *jni_imclass_name = makeValidJniName(imclass_name);
      String           *swig_module_init = NewString("swig_module_init");
      String           *swig_module_init_jni = makeValidJniName(swig_module_init);
      String           *dmethod_data = NewString("");
      int               n_methods = 0;
      Iterator          udata_iter;

      udata_iter = First(dmethods_seq);
      while (udata_iter.item) {
        UpcallData     *udata = udata_iter.item;
        Printf(dmethod_data, "  { \"%s\", \"%s\" }", Getattr(udata, "imclass_method"), Getattr(udata, "imclass_fdesc"));
        ++n_methods;

        udata_iter = Next(udata_iter);

        if (udata_iter.item)
          Putc(',', dmethod_data);
        Putc('\n', dmethod_data);
      }

      Printf(f_runtime, "namespace Swig {\n");
      Printf(f_runtime, "  static jclass jclass_%s = NULL;\n", imclass_name);
      Printf(f_runtime, "  static jmethodID director_methids[%d];\n", n_methods);
      Printf(f_runtime, "}\n");

      Printf(w->def, "JNIEXPORT void JNICALL Java_%s%s_%s(JNIEnv *jenv, jclass jcls) {",
             jnipackage, jni_imclass_name, swig_module_init_jni);
      Printf(w->code, "static struct {\n");
      Printf(w->code, "  const char *method;\n");
      Printf(w->code, "  const char *signature;\n");
      Printf(w->code, "} methods[%d] = {\n", n_methods);
      Printv(w->code, dmethod_data, NIL);
      Printf(w->code, "};\n");

      Wrapper_add_local(w, "i", "int i");

      Printf(w->code, "Swig::jclass_%s = (jclass) jenv->NewGlobalRef(jcls);\n", imclass_name);
      Printf(w->code, "if (Swig::jclass_%s == NULL) return;\n", imclass_name);
      Printf(w->code, "for (i = 0; i < (int) (sizeof(methods)/sizeof(methods[0])); ++i) {\n");
      Printf(w->code, "  Swig::director_methids[i] = jenv->GetStaticMethodID(jcls, methods[i].method, methods[i].signature);\n");
      Printf(w->code, "  if (Swig::director_methids[i] == NULL) return;\n");
      Printf(w->code, "}\n");

      Printf(w->code, "}\n");

      Wrapper_print(w, f_wrappers);
      Delete(dmethod_data);
      Delete(swig_module_init_jni);
      Delete(swig_module_init);
      Delete(jni_imclass_name);
      DelWrapper(w);
    }
  }

  /* ---------------------------------------------------------------
   * Reduce and canonicalize the input type
   *
   * (scottm) There is probably a SWIG function to do this, but I
   * haven't found it yet.
   * --------------------------------------------------------------- */

  Node *canonicalizeType(Node *n, String *classtype)
  {
    String *reduced_type = SwigType_typedef_resolve_all(classtype);
    String *base_type = SwigType_base(reduced_type);
    Node   *classnode = Swig_symbol_clookup(base_type, Getattr(n, "sym:symtab"));

    if (classnode != NULL) {
      /* This takes care of the case when there are forward declared
       * classes that Language::classHandler() hasn't encountered yet.
       *
       * Swig_symbol_clookup() will find <<a node>> attached to
       * the base_type name, and there will be a csym:nextSibling node
       * chain.  Most of the time, there **wont** be * a "class"-type
       * node. Mostly, what I've seen are "constructor"-type nodes.
       * Eventutally, however, a node is encountered with its
       * sym:name set and this happens to be the right name.
       *
       * The worst that can happen here is that base_type is unchanged,
       * which tends also to be the right thing to do.
       *
       * Dunno, this seems to work, so... contemplate what the "right"
       * SWIG thing is to do...
       */

      while (classnode && Getattr(classnode, "sym:name") == NULL) {
        classnode = Getattr(classnode, "csym:nextSibling");
      }
    }

    Delete(reduced_type);
    Delete(base_type);

    return classnode;
  }

  /* ---------------------------------------------------------------
   * Canonicalize the JNI field descriptor
   *
   * Take the input type name, reduce the typedef to its fundamental
   * type if necessary, normalize its name.
   *
   * See if the resulting typdef-reduced, normalized type name also
   * has a "javapackage" typemap, and use it instead of the default
   * package_path.
   * 
   * !!SFM!! If $packagepath occurs in the field descriptor, but
   * package_path isn't set (length == 0), then strip it and the
   * optional trailing '/' from the resulting name.
   * 
   * --------------------------------------------------------------- */

  String *canonicalJNIFDesc(String *in_desc, Node *n, String *classtype)
  {
    Node       *classnode = canonicalizeType(n, classtype);
    String     *name = (classnode ? Getattr(classnode, "name") : classtype);
    String     *symname = (classnode ? Getattr(classnode, "sym:name") : classtype);
    Parm       *tp = NewParm(name, (String *) empty_string);
    String     *pkg_path;

    pkg_path = Swig_typemap_lookup_new("javapackage", tp, "", 0);
    Delete(tp);

    if (pkg_path != NULL && Len(pkg_path) != 0) {
      Replaceall(pkg_path, ".", "/");
    } else
      pkg_path = package_path;

    String *mod_desc = Copy(in_desc);

    if (Len(pkg_path) > 0) {
      Replaceall(mod_desc, "$packagepath", pkg_path);
    } else {
      Replaceall(mod_desc, "$packagepath/", empty_string);
      Replaceall(mod_desc, "$packagepath", empty_string);
    }

    Replaceall(mod_desc, "$javaclassname", symname);

    if (pkg_path != package_path)
      Delete(pkg_path);

    return mod_desc;
  }

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // (scottm) Class director mods:
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  /* ---------------------------------------------------------------
   * classDirectorMethod()
   *
   * Emit a virtual director method to pass a method call on to the 
   * underlying Java object.
   *
   * --------------------------------------------------------------- */

  int classDirectorMethod(Node *n, Node *parent, String *super)
  {
    String     *empty_str = NewString("");
    String     *classname = Getattr(parent, "sym:name");
    String     *name = Getattr(n, "name");
    String     *symname = Getattr(n, "sym:name");
    String     *type = Getattr(n, "type");
    String     *c_classname = NULL;
    String     *overloaded_name = getOverloadedName(n);
    String     *storage = Getattr(n, "storage");
    String     *value = Getattr(n, "value");
    String     *decl = Getattr(n, "decl");
    String     *declaration = NewString("");
    String     *return_type = Copy(type);
    String     *tm;
    Parm       *p;
    int         i, num_arguments, num_required;
    Wrapper    *w = NewWrapper();
    ParmList   *l = Getattr(n, "parms");
    bool        is_void = !(Cmp(type, "void"));
    bool        pure_virtual = (!(Cmp(storage, "virtual")) && !(Cmp(value, "0")));
    bool        is_const = SwigType_isconst(decl);
    int         status = SWIG_OK;

    bool        output_director = true;
    bool        recursive_upcall = false;
    String     *dirclassname = NewStringf("SwigDirector_%s", classname);
    String     *qualified_name = NewStringf("%s::%s", dirclassname, name);
    String     *jnidesc = NewString("");
    String     *classdesc = NewString("");
    String     *jniret_desc = NewString("");
    String     *classret_desc = NewString("");
    String     *jniret_type = NULL;
    String     *jupcall_args = NewString("swig_get_self()");
    String     *imclass_dmethod;
    Wrapper    *imw = NewWrapper();
    String     *imcall_args = NewString("");
    int         gencomma = 0;
    int         classmeth_off = curr_class_dmethod - first_class_dmethod;

    /* Skip out of here if nodirector is set */

    if (!Cmp(Getattr(n, "feature:nodirector"), "1"))
      return SWIG_OK;

    // This is a kludge: functionWrapper has sym:overload set properly, but it
    // isn't at this point, so we have to manufacture it ourselves. At least
    // we're consistent with the sym:overload name in functionWrapper. (?? when
    // does the overloaded method name get set?)

    imclass_dmethod = NewStringf("SwigDirector_%s", Swig_name_member(classname, overloaded_name));

    // Get the proper name for the parent node (should be a class... hint)

    c_classname = Getattr(parent, "name");
    if (!(Cmp(type, "class")))
      c_classname = classname;

    /* Check if the user wants upcall recursion */
    recursive_upcall = !(Cmp(Getattr(n, "feature:director:recursive"), "1")
                         && Cmp(Getattr(parent, "feature:director:recursive"), "1"));

    /* Handle and form complete return type, including the modification
       to a pointer, if return type is a reference. */

    if (return_type) {
      if (!is_void) {
        SwigType *t = Copy(decl);
        SwigType *f = SwigType_pop_function(t);

        SwigType_push(return_type, t);

        Delete(f);
        Delete(t);

        if (!SwigType_isclass(return_type)) {
          Wrapper_add_localv(w, "result", SwigType_lstr(return_type, "result"), NIL);
        } else {
          SwigType *vt;

          vt = cplus_value_type(return_type);
          if (vt == NULL) {
            Wrapper_add_localv(w, "result", SwigType_lstr(return_type, "result"), NIL);
          } else {
            Wrapper_add_localv(w, "result", SwigType_lstr(vt, "result"), "= 0", NIL);
            Delete(vt);
          }
        }

        /* Create the intermediate class wrapper */
        Parm *tp = NewParm(return_type, empty_str);

        tm = Swig_typemap_lookup_new("jtype", tp, "", 0);
        if (tm != NULL) {
          Printf(imw->def, "public static %s %s(%s self", tm, imclass_dmethod, classname);
        } else {
          Swig_warning(WARN_JAVA_TYPEMAP_JTYPE_UNDEF, input_file, line_number, 
              "No jtype typemap defined for %s\n", SwigType_str(t,0));
        }
      } else
        Printf(imw->def, "public static void %s(%s self", imclass_dmethod, classname);

      /* Get the JNI field descriptor for this return type, add the JNI field descriptor
         to jniret_desc */

      Parm       *retpm = NewParm(return_type, empty_str);
      
      if ((jniret_type = Swig_typemap_lookup_new("jni", retpm, "", 0)) != NULL) {
        String *jdesc;
        Parm *tp = NewParm(jniret_type, empty_str);

        if (!is_void) {
          String *jretval_decl = NewStringf("%s jresult", jniret_type);
          Wrapper_add_localv(w, "jresult", jretval_decl, " = 0", NIL);
          Delete(jretval_decl);
        }

        if ((tm = Swig_typemap_lookup_new("directorin", tp, "", 0)) != NULL
            && (jdesc = Getattr(tp, "tmap:directorin:parse")) != NULL) {
          String *jnidesc_canon;

          jnidesc_canon = canonicalJNIFDesc(jdesc, n, jniret_type);
          Append(jniret_desc, jnidesc_canon);
          Delete(jnidesc_canon);
        } else {
          Swig_warning(WARN_TYPEMAP_DIRECTORIN_UNDEF, input_file, line_number, 
                       "No or improper directorin typemap defined for %s\n", SwigType_str(jniret_type,0));
          output_director = false;
        }

        Delete(tp);
      } else {
        Swig_warning(WARN_JAVA_TYPEMAP_JNI_UNDEF, input_file, line_number, 
            "No jni typemap defined for %s\n", SwigType_str(type,0));
        output_director = false;
      }

      String *jdesc;

      if ((tm = Swig_typemap_lookup_new("directorin", retpm, "", 0)) != NULL
          && (jdesc = Getattr(retpm, "tmap:directorin:parse")) != NULL) {
        String *jnidesc_canon;

        jnidesc_canon = canonicalJNIFDesc(jdesc, n, return_type);
        Append(classret_desc, jnidesc_canon);
        Delete(jnidesc_canon);
      } else {
        Swig_warning(WARN_TYPEMAP_DIRECTORIN_UNDEF, input_file, line_number, 
                     "No or improper directorin typemap defined for %s\n", SwigType_str(jniret_type,0));
        output_director = false;
      }

      Delete(retpm);
    }

    /* Go through argument list, attach lnames for arguments */
    for (i = 0, p=l; p; p = nextSibling(p), ++i) {
      String *arg = Getattr(p, "name");
      String *lname = NewString("");

      if (arg == NULL && Cmp(Getattr(p, "type"), "void")) {
        lname = NewStringf("arg%d", i);
        Setattr(p, "name", lname);
      } else
        lname = arg;

      Setattr(p, "lname", lname);
    }

    /* Attach the standard typemaps */
    emit_attach_parmmaps(l, w);
    Swig_typemap_attach_parms("in", l, w);
    Swig_typemap_attach_parms("out", l, w);
    Swig_typemap_attach_parms("jni", l, w);
    Swig_typemap_attach_parms("jtype", l, w);
    Swig_typemap_attach_parms("directorin", l, 0);
    Swig_typemap_attach_parms("javadirectorin", l, 0);

    /* header declaration, start wrapper definition */
    {
      String *target;

      target = method_decl(decl, qualified_name, l, 0, 0);
      String *rtype = SwigType_str(type, 0);

      Printf(w->def, "%s %s {", rtype, target);
      Delete(qualified_name);
      Delete(target);

      target = method_decl(decl, name, l, 0, 1);
      Printf(declaration, "  virtual %s %s;\n", rtype, target);
      Delete(rtype);
      Delete(target);
    }

    /* Add Java environment pointer to wrapper */
    {
      String *jenvstr = NewString("jenv");

      Wrapper_add_localv(w, jenvstr, "JNIEnv *", jenvstr, "= (JNIEnv *) NULL", NIL);
      Delete(jenvstr);
    }

    /* Preamble code */

    Printf(w->code, "jenv = swig_acquire_jenv();\n");
    Printf(w->code, "if (!swig_override[%d]) {\n", classmeth_off);

    if (!pure_virtual) {
      if (is_void) {
        Printf(w->code, "%s;\n", Swig_method_call(super,l));
        Printf(w->code, "return;\n");
      } else {
        String *super_call = Swig_method_call(super, l);
        Printf(w->code, "return %s;\n", super_call);
        Delete(super_call);
      }
    } else {
      Printf(w->code, "SWIG_JavaThrowException(jenv, SWIG_JavaDirectorPureVirtual,\n");
      Printf(w->code, "      \"Attempted to invoke pure virtual method %s::%s.\");\n",
             c_classname, name);

      /* Make sure that we return something in the case of a pure
       * virtual method call for syntactical reasons. */
      if (!is_void)
        Printf(w->code, "return %s;\n", SwigType_rcaststr(return_type, "result"));
      else
        Printf(w->code, "return;\n");
    }

    Printf(w->code, "}\n");

    /* Start the Java field descriptor for the intermediate class's upcall (insert self object) */
    {
      Parm *tp = NewParm(classname, empty_str);
      String *jdesc;

      if ((tm = Swig_typemap_lookup_new("directorin", tp, "", 0)) != NULL
          && (jdesc = Getattr(tp, "tmap:directorin:parse")) != NULL) {
        String *jni_canon;
          
        jni_canon = canonicalJNIFDesc(jdesc, n, classname);
        Append(jnidesc, jni_canon);
        Delete(jni_canon);
        Delete(tm);
      } else {
        Swig_warning(WARN_TYPEMAP_DIRECTORIN_UNDEF, input_file, line_number, 
                     "No or improper directorin typemap defined for %s\n", SwigType_str(classname, 0));
        output_director = false;
      }

      Delete(tp);
    }

    if (!recursive_upcall) {
      if (!is_const)
        Printf(w->code, "if (!swig_get_ricochet(%d)) {\n", classmeth_off);
      else
        Printf(w->code, "if (!const_cast<%s *>(this)->swig_get_ricochet(%d)) {\n",
               dirclassname, classmeth_off);
    }

    /* Get number of required and total arguments */
    num_arguments = emit_num_arguments(l);
    num_required  = emit_num_required(l);

    /* Go through argument list, convert from native to Java */
    for (i = 0, p=l; i < num_arguments; i++) {
      while (checkAttribute(p,"tmap:in:numinputs","0")) {
        p = Getattr(p,"tmap:in:next");
      }

      SwigType *pt = Getattr(p,"type");
      String   *ln = Copy(Getattr(p,"name"));
      String   *c_param_type = NULL;
      String   *c_decl = NewString("");
      String   *arg = NewString("");

      Printf(arg, "j%s", ln);

      /* Add various typemap's 'throws' clauses */
      addThrows(n, "tmap:in", p);
      addThrows(n, "tmap:out", p);

      /* And add to the upcall args */
      Printf(jupcall_args, ", %s", arg);

      /* Get parameter's JNI C type */
      if ((c_param_type = Getattr(p, "tmap:jni")) != NULL) {
        Parm *tp = NewParm(c_param_type, empty_str);
        String *desc_tm, *jdesc, *cdesc;

        /* Add to local variables */
        Printf(c_decl, "%s %s", c_param_type, arg);
        Wrapper_add_local(w, arg, c_decl);

        /* Add input marshalling code and update JNI field descriptor */
        if ((desc_tm = Swig_typemap_lookup_new("directorin", tp, "", 0)) != NULL
            && (jdesc = Getattr(tp, "tmap:directorin:parse")) != NULL) {
          if ((tm = Getattr(p, "tmap:directorin")) != NULL
              && (cdesc = Getattr(p, "tmap:directorin:parse")) != NULL) {
            String *jni_canon;
          
            jni_canon = canonicalJNIFDesc(jdesc, n, c_param_type);
            Append(jnidesc, jni_canon);
            Delete(jni_canon);

            Replaceall(tm,"$input", arg);

            if (Len(tm))
              Printf(w->code,"%s\n", tm);

            Delete(tm);

            /* Add parameter to the intermediate class code if inherited and
               generating the intermediate's upcall code */
            if ((tm = Getattr(p, "tmap:jtype")) != NULL) {
              String         *din;

              if ((din = Copy(Getattr(p, "tmap:javadirectorin"))) != NULL) {
                String *darg = NewStringf("d%s", ln);
                String *darg_init = NULL;
                Node *canon_node = canonicalizeType(n, pt);
                String *canon_type = (canon_node ? Getattr(canon_node, "sym:name") : pt);

                Replaceall(din, "$javaclassname", canon_type);
                Replaceall(din, "$jniinput", ln);

                Printf(imw->def, ", %s %s", tm, ln);
                if (++gencomma > 1)
                  Printf(imcall_args, ", ");

                if (Cmp(din, ln)) {
                  darg_init = NewStringf("%s = %s", darg, din);
                  Wrapper_add_localv(imw, darg, canon_type, darg_init, NIL);
                  Printv(imcall_args, darg, NIL);
                } else
                  Printv(imcall_args, ln, NIL);

                jni_canon = canonicalJNIFDesc(cdesc, n, pt);
                Append(classdesc, jni_canon);

                Delete(jni_canon);
                Delete(darg);
                if (darg_init)
                  Delete(darg_init);
              } else {
                Swig_warning(WARN_JAVA_TYPEMAP_JAVADIRECTORIN_UNDEF, input_file, line_number, 
                             "No javadirectorin typemap defined for %s\n", SwigType_str(pt,0));
                output_director = false;
              }
            } else {
              Swig_warning(WARN_JAVA_TYPEMAP_JTYPE_UNDEF, input_file, line_number, 
                  "No jtype typemap defined for %s\n", SwigType_str(pt,0));
              output_director = false;
            }

            p = Getattr(p, "tmap:in:next");
          } else {
            Swig_warning(WARN_TYPEMAP_DIRECTORIN_UNDEF, input_file, line_number, 
                         "No or improper directorin typemap defined for %s\n", SwigType_str(pt, 0));
            output_director = false;
            p = nextSibling(p);
          }

          Delete(desc_tm);
        } else {
          Swig_warning(WARN_TYPEMAP_DIRECTORIN_UNDEF, input_file, line_number, 
                       "No or improper directorin typemap defined for %s\n", SwigType_str(c_param_type, 0));
          output_director = false;
          p = nextSibling(p);
        }

        Delete(tp);
      } else {
        Swig_warning(WARN_JAVA_TYPEMAP_JNI_UNDEF, input_file, line_number, 
            "No jni typemap defined for %s\n", SwigType_str(pt,0));
        output_director = false;
        p = nextSibling(p);
      }

      Delete(arg);
      Delete(c_decl);
      Delete(c_param_type);
    }

    {
      // Get any Java exception classes in the throws typemap
      ParmList *throw_parm_list = NULL;
      if ((throw_parm_list = Getattr(n,"throws"))) {
        Swig_typemap_attach_parms("throws", throw_parm_list, 0);
        for (p = throw_parm_list; p; p=nextSibling(p)) {
          if ((tm = Getattr(p,"tmap:throws"))) {
            addThrows(n, "tmap:throws", p);
          }
        }
      }
    }

    /* Finish off the inherited upcall's definition */

    Putc(')', imw->def);
    generateThrowsClause(n, imw->def);
    Printf(imw->def, " {");
    
    /* Emit the intermediate class's upcall to the actual class */

    String *upcall = NewStringf("self.%s(%s)", symname, imcall_args);

    if (!is_void) {
      Parm   *tp = NewParm(return_type, empty_str);

      tm = Swig_typemap_lookup_new("javadirectorout", tp, "", 0);
      if (tm != NULL) {
        String *base_type = SwigType_base(return_type);
        Node *canon_node = canonicalizeType(n, return_type);
        String *canon_type = (canon_node ? Getattr(canon_node, "sym:name") : base_type);

        Replaceall(tm, "$javaclassname", canon_type);
        Replaceall(tm, "$javacall", upcall);

        Printf(imw->code, "return %s;\n", tm);
      }

      Delete(tm);
    } else
      Printf(imw->code, "%s;", upcall);

    Printf(imw->code, " }");
    Delete(upcall);

    /* Emit the actual upcall through JNI */
    String *imclass_desc = NewStringf("(%s)%s", jnidesc, jniret_desc);
    String *class_desc = NewStringf("(%s)%s", classdesc, classret_desc);
    UpcallData *udata = addUpcallMethod(imclass_dmethod, symname, imclass_desc, class_desc, decl);
    String *methid = Getattr(udata, "imclass_methodidx");
    String *methop = getUpcallJNIMethod(jniret_desc);

    if (!recursive_upcall) {
      if (!is_const)
        Printf(w->code, "  swig_set_ricochet(%d);\n", classmeth_off);
      else
        Printf(w->code, "  const_cast<%s *>(this)->swig_set_ricochet(%d);\n",
               dirclassname, classmeth_off);
    }

    if (!is_void) {
      Printf(w->code, "jresult = (%s) jenv->%s(Swig::jclass_%s, Swig::director_methids[%s], %s);\n",
             jniret_type, methop, imclass_name, methid, jupcall_args);
    } else {
      Printf(w->code, "jenv->%s(Swig::jclass_%s, Swig::director_methids[%s], %s);\n",
             methop, imclass_name, methid, jupcall_args);
    }

    if (!recursive_upcall) {
      if (!is_const)
        Printf(w->code, "swig_clear_ricochet(%d);\n", classmeth_off);
      else
        Printf(w->code, "  const_cast<%s *>(this)->swig_clear_ricochet(%d);\n",
               dirclassname, classmeth_off);
    }

    Printf(w->code, "if (jenv->ExceptionOccurred()) return $null;\n");

    if (!is_void) {
      String *jresult_str = NewString("jresult");
      String *result_str = NewString("result");
      Parm *tp = NewParm(return_type, result_str);

      /* Copy jresult into result... */
      if ((tm = Swig_typemap_lookup_new("in", tp, result_str, w))) {
        addThrows(n, "tmap:in", tp);
        Replaceall(tm,"$source", jresult_str); /* deprecated */
        Replaceall(tm,"$target", result_str); /* deprecated */
        Replaceall(tm,"$arg", jresult_str); /* deprecated? */
        Replaceall(tm,"$input", jresult_str);
        Printf(w->code, "%s\n", tm);
      } else {
        Swig_warning(WARN_TYPEMAP_IN_UNDEF, input_file, line_number, 
            "Unable to use type %s as a function argument.\n",SwigType_str(tp,0));
        output_director = false;
      }

      Delete(tp);
      Delete(jresult_str);
      Delete(result_str);
    }

    if (!recursive_upcall) {
      Printf(w->code, "} else {\n");
      Printf(w->code, "    SWIG_JavaThrowException(jenv, SWIG_JavaDirectorRicochet,\n");
      Printf(w->code, "      \"Recursive loop into director method %s::%s detected.\");\n",
             c_classname, name);
      Printf(w->code, "}\n");
    }

    Delete(imclass_desc);
    Delete(class_desc);

    /* Terminate wrapper code */
    if (!is_void)
      Printf(w->code, "return (%s);\n", SwigType_rcaststr(return_type, "result"));

    Printf(w->code, "}");

    /* emit code */
    if (status == SWIG_OK && output_director) {
      if(!is_void)
        Replaceall(w->code,"$null", SwigType_rcaststr(return_type, "result"));
      else
        Replaceall(w->code,"$null","");
      Wrapper_print(w, f_directors);
      Wrapper_print(imw, imclass_directors);
      Printv(f_directors_h, declaration, NIL);
    }

    Delete(jnidesc);
    Delete(jniret_type);
    Delete(jniret_desc);
    Delete(declaration);
    DelWrapper(w);

    ++curr_class_dmethod;
    return status;
  }

  /* ------------------------------------------------------------
   * directorPrefixArgs()
   * ------------------------------------------------------------ */

  void directorPrefixArgs(Node *n)
  {
    Parm     *p;

    /* Need to prepend 'jenv' to the director constructor's argument list */

    String   *jenv_type = NewString("JNIEnv");

    SwigType_add_pointer(jenv_type);

    p = NewParm(jenv_type, NewString("jenv"));
    Setattr(p, "arg:byname", "1");
    set_nextSibling(p, NULL);

    Setattr(n, "director:prefix_args", p);
  }

  /* ------------------------------------------------------------
   * classDirectorConstructor()
   * ------------------------------------------------------------ */

  int classDirectorConstructor(Node *n) {
    Node *parent = parentNode(n);
    String *decl = Getattr(n, "decl");;
    String *supername = Swig_class_name(parent);
    String *classname = NewStringf("SwigDirector_%s", supername);
    String *sub = NewString("");
    Parm *p;
    ParmList *superparms = Getattr(n, "parms");
    ParmList *parms;
    int argidx = 0;

    /* Assign arguments to superclass's parameters, if not already done */
    for (p = superparms; p; p = nextSibling(p)) {
      String   *pname = Getattr(p, "name");

      if (pname == NULL) {
        pname = NewStringf("arg%d", argidx++);
        Setattr(p, "name", pname);
      }
    }

    /* insert jenv prefix argument */
    parms = CopyParmList(superparms);

    String   *jenv_type = NewString("JNIEnv");
    SwigType_add_pointer(jenv_type);
    p = NewParm(jenv_type, NewString("jenv"));
    set_nextSibling(p, parms);
    parms = p;

    directorPrefixArgs(n);

    /* constructor */
    {
      Wrapper *w = NewWrapper();
      String *basetype = Getattr(parent, "classtype");
      String *target = method_decl(decl, classname, parms, 0, 0);
      String *call = Swig_csuperclass_call(0, basetype, superparms);

      Printf(w->def, "%s::%s: %s, Swig::Director(jenv) {}", classname, target, call);
      Wrapper_print(w, f_directors);

      Delete(target);
      Delete(call);
      DelWrapper(w);
    }
   
    /* constructor header */
    {
      String *target = method_decl(decl, classname, parms, 0, 1);
      Printf(f_directors_h, "  %s;\n", target);
      Delete(target);
    }

    Delete(sub);
    Delete(classname);
    Delete(supername);
    Delete(jenv_type);
    Delete(parms);
    return Language::classDirectorConstructor(n);
  }

  /* ------------------------------------------------------------
   * classDirectorDefaultConstructor()
   * ------------------------------------------------------------ */
   
  int classDirectorDefaultConstructor(Node *n) {
    String *classname;
    classname = Swig_class_name(n);
    {
      Wrapper *w = NewWrapper();
      Printf(w->def, "SwigDirector_%s::SwigDirector_%s(JNIEnv *jenv) : Swig::Director(jenv) {",
             classname, classname);
      Printf(w->code, "}\n");
      Wrapper_print(w, f_directors);
      DelWrapper(w);
    }
    Printf(f_directors_h, "    SwigDirector_%s(JNIEnv *jenv);\n", classname);
    Delete(classname);
    directorPrefixArgs(n);
    return Language::classDirectorDefaultConstructor(n);
  }


  /* ------------------------------------------------------------
   * classDirectorInit()
   * ------------------------------------------------------------ */

  int classDirectorInit(Node *n) {
    String *declaration = Swig_director_declaration(n);
    String *classname = Getattr(n, "sym:name");
    String *director_classname = NewStringf("SwigDirector_%s", classname);

    Delete(none_comparison);
    none_comparison = NewString("");            // not used

    Delete(director_ctor_code);
    director_ctor_code = NewString("$director_new");

    Printf(f_directors_h, "\n");
    Printf(f_directors_h, "%s\n", declaration);
    Printf(f_directors_h, "\npublic:\n");
    Printf(f_directors_h, "  virtual ~%s();\n", director_classname);
    Printf(f_directors_h, "  void swig_connect_director(JNIEnv *jenv, jobject jself, jclass jcls);\n");
    Printf(f_directors_h, "  bool swig_get_ricochet(int n) {\n");
    Printf(f_directors_h, "    bool ric = swig_ricochet[n];\n");
    Printf(f_directors_h, "    swig_ricochet[n] = false;\n");
    Printf(f_directors_h, "    return ric;\n");
    Printf(f_directors_h, "  }\n");
    Printf(f_directors_h, "  void swig_set_ricochet(int n) {\n");
    Printf(f_directors_h, "    swig_ricochet[n] = true;\n");
    Printf(f_directors_h, "  }\n");
    Printf(f_directors_h, "  void swig_clear_ricochet(int n) {\n");
    Printf(f_directors_h, "    swig_ricochet[n] = false;\n");
    Printf(f_directors_h, "  }\n");
    Printf(f_directors_h, "  bool swig_overrides(int n) {\n");
    Printf(f_directors_h, "    return swig_override[n];\n");
    Printf(f_directors_h, "  }\n");

    Delete(declaration);

    Printf(f_directors, "%s::~%s() {}\n", director_classname, director_classname);

    Delete(director_classname);

    /* Keep track of the director methods for this class */
    first_class_dmethod = curr_class_dmethod = n_dmethods;
 
    return Language::classDirectorInit(n);
  }

  /* ------------------------------------------------------------
   * classDirectorEnd()
   * ------------------------------------------------------------ */

  int classDirectorEnd(Node *n) {
    String *classname = Getattr(n, "sym:name");
    String *director_classname = NewStringf("SwigDirector_%s", classname);
    String *internal_classname;

    Wrapper *w = NewWrapper();

    if (Len(package_path) > 0)
      internal_classname = NewStringf("%s/%s", package_path, classname);
    else
      internal_classname = NewStringf("%s", classname);

    Wrapper_add_localv(w, "baseclass", "static jclass baseclass", " = 0", NIL);
    Printf(w->def, "void %s::swig_connect_director(JNIEnv *jenv, jobject jself, jclass jcls) {", director_classname);

    if (first_class_dmethod != curr_class_dmethod) {
      Printf(w->def, "static struct {\n");
      Printf(w->def, "const char *mname;\n");
      Printf(w->def, "const char *mdesc;\n");
      Printf(w->def, "jmethodID base_methid;\n");
      Printf(w->def, "} methods[] = {\n");

      for (int i = first_class_dmethod; i < curr_class_dmethod; ++i) {
        UpcallData       *udata = Getitem(dmethods_seq, i);

        Printf(w->def, "{ \"%s\", \"%s\" }", Getattr(udata, "method"), Getattr(udata, "fdesc"));
        if (i != curr_class_dmethod - 1)
          Putc(',', w->def);
        Putc('\n', w->def);
      }

      Printf(w->def, "};\n");
    }

    Wrapper_add_local(w, "derived", "bool derived");

    Printf(w->code, "swig_set_self(jenv, jself);\n");
    Printf(w->code, "if (baseclass == NULL) {\n");
    Printf(w->code, "baseclass = jenv->FindClass(\"%s\");\n", internal_classname);
    Printf(w->code, "if (baseclass == NULL) return;\n");
    Printf(w->code, "baseclass = (jclass) jenv->NewGlobalRef(baseclass);\n");
    Printf(w->code, "}\n");
    Printf(w->code, "derived = (jenv->IsSameObject(baseclass, jcls) ? false : true);\n");

    if (first_class_dmethod != curr_class_dmethod) {
      int               n_methods = curr_class_dmethod - first_class_dmethod;

      /* Emit the code to look up the class's methods, initialize the override and ricochet arrays */
      Printf(f_directors_h, "protected:\n");
      Printf(f_directors_h, "  bool swig_ricochet[%d];\n", n_methods);
      Printf(f_directors_h, "  bool swig_override[%d];\n", n_methods);

      Printf(w->code, "for (int i = 0; i < %d; ++i) {\n", n_methods);
      Printf(w->code, "  swig_ricochet[i] = false;\n");
      Printf(w->code, "  if (methods[i].base_methid == NULL)\n");
      Printf(w->code, "    methods[i].base_methid = jenv->GetMethodID(baseclass, methods[i].mname, methods[i].mdesc);\n");
      Printf(w->code, "  if (derived) {\n");
      Printf(w->code, "    jmethodID methid;\n");
      Printf(w->code, "    methid = jenv->GetMethodID(jcls, methods[i].mname, methods[i].mdesc);\n");
      Printf(w->code, "    swig_override[i] = (methid && jenv->IsSameObject((jobject) methid, (jobject) methods[i].base_methid) ? false : true);\n");
      Printf(w->code, "    jenv->ExceptionClear();\n");
      Printf(w->code, "  } else\n");
      Printf(w->code, "    swig_override[i] = false;\n");
      Printf(w->code, "}\n");
    }

    Printf(f_directors_h, "};\n\n");
    Printf(w->code, "}\n");

    Wrapper_print(w, f_directors);

    DelWrapper(w);
    Delete(director_classname);
    Delete(internal_classname);

    return Language::classDirectorEnd(n);
  }

  /* --------------------------------------------------------------------
   * classDirectorDisown()
   * ------------------------------------------------------------------*/
  virtual int classDirectorDisown(Node *n)
  {
    /* NOP */
    return SWIG_OK;
  }

  /* -----------------------------------------------------------------------------
   * abstractClassTest()
   *
   * Make sure that the constructors are always generated for director-based
   * classes derived from abstract classes, since Java always allocates the
   * director-based class.
   * ----------------------------------------------------------------------------- */

  virtual int JAVA::abstractClassTest(Node *n)
  {
    if (!Language::abstractClassTest(n))
      return 0;
    else if (!Cmp(Getattr(n, "feature:director"), "1"))
      return 0;
    return 1;
  }
};   /* class JAVA */

/* -----------------------------------------------------------------------------
 * swig_java()    - Instantiate module
 * ----------------------------------------------------------------------------- */

extern "C" Language *
swig_java(void) {
  return new JAVA();
}

/* -----------------------------------------------------------------------------
 * Static member variables
 * ----------------------------------------------------------------------------- */

const char *JAVA::usage = (char*)"\
Java Options (available with -java)\n\
     -package <name> - set name of the Java package to <name>\n\
     -noproxy        - Generate the low-level functional interface instead\n\
                       of proxy classes\n\
\n";

