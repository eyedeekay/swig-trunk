/* -----------------------------------------------------------------------------
 * java.cxx
 *
 *     Java wrapper module.
 *
 * Copyright (C) 1999-2000.  The University of Chicago
 * See the file LICENSE for information on usage and redistribution.
 * ----------------------------------------------------------------------------- */

#include <ctype.h>

#include "mod11.h"
#include "java.h"

char bigbuf[1024];

static char *usage = (char*)"\
Java Options\n\
     -jnic            - use c syntax for jni calls\n\
     -jnicpp          - use c++ syntax for jni calls\n\
     -module name     - set name of module\n\
     -package name    - set name of package\n\
     -shadow          - enable shadow classes\n\
     -nofinalize      - do not generate finalize methods in shadow classes\n\
     -rn              - generate register natives code\n\n";

static  char         *module = 0;          // Name of the module
static  char         *java_path = (char*)"java";
static  char         *package = 0;         // Name of the package
static  char         *c_pkgstr;         // Name of the package
static  char         *jni_pkgstr;         // Name of the package
static  char         *shadow_classname;
static  char         *jimport = 0;
static  char         *method_modifiers = (char*)"public final static";
static  FILE         *f_java = 0;
static  FILE         *f_shadow = 0;
static  int          shadow = 0;
static  Hash         *shadow_classes;
static  String       *shadow_classdef;
static  char         *shadow_variable_name = 0; //Name of a c struct variable or c++ public member variable (may or may not be const)
static  char         *shadow_baseclass = 0;
static  int          classdef_emitted = 0;
static  int          shadow_classdef_emitted = 0;
static  int          have_default_constructor = 0;
static  int          native_func = 0;     // Set to 1 when wrapping a native function
static  int          enum_flag = 0; // Set to 1 when wrapping an enum
static  int          static_flag = 0; // Set to 1 when wrapping a static functions or member variables
static  int          variable_wrapper_flag = 0; // Set to 1 when wrapping a member variable
static  int          wrapping_member = 0; // Set to 1 when wrapping a member variable/enum/const
static  int          jnic = -1;          // 1: use c syntax jni; 0: use c++ syntax jni
static  int          nofinalize = 0;          // for generating finalize methods
static  int          useRegisterNatives = 0;        // Set to 1 when doing stuff with register natives
static  String      *registerNativesList = 0;
static  String      *shadow_enum_code;
static  String      *java_enum_code;

/* Test to see if a type corresponds to something wrapped with a shadow class */
/* Return NULL if not otherwise the shadow name */
static String *is_shadow(SwigType *t) {
  String *r;
  SwigType *lt = Swig_clocal_type(t);
  r = Getattr(shadow_classes,lt);
  Delete(lt);
  return r;
}

// Return the type of the c array
static SwigType *get_array_type(SwigType *t) {
  SwigType *ta = 0;
  if (SwigType_type(t) == T_ARRAY) {
    SwigType *aop;
    ta = Copy(t);
    aop = SwigType_pop(ta);
  }
  return ta;
}

/* 
Return java type in java_type. 
The type returned is the java type when shadow_flag=0.
The type returned is the java shadow type when shadow_flag=1.
*/
void JAVA::SwigToJavaType(SwigType *t, String_or_char *pname, String* java_type, int shadow_flag) {
  char *jtype = 0;
  if (shadow_flag)
    jtype = JavaTypeFromTypemap((char*)"jstype", t, pname);
  if (!jtype)
    jtype = JavaTypeFromTypemap((char*)"jtype", t, pname);

  if(jtype) {
    Printf(java_type, jtype);
  }
  else {
    /* Map type here */
    switch(SwigType_type(t)) {
      case T_CHAR:    Printf(java_type, "byte"); break;
      case T_SCHAR:   Printf(java_type, "byte"); break;
      case T_UCHAR:   Printf(java_type, "short"); break;
      case T_SHORT:   Printf(java_type, "short"); break;
      case T_USHORT:  Printf(java_type, "int"); break;
      case T_INT:     Printf(java_type, "int"); break;
      case T_UINT:    Printf(java_type, "long"); break;
      case T_LONG:    Printf(java_type, "long"); break;
      case T_ULONG:   Printf(java_type, "long"); break;
      case T_FLOAT:   Printf(java_type, "float"); break;
      case T_DOUBLE:  Printf(java_type, "double"); break;
      case T_BOOL:    Printf(java_type, "boolean"); break;
      case T_STRING:  Printf(java_type, "String"); break;
      case T_VOID:    Printf(java_type, "void"); break;
      case T_POINTER:
      case T_REFERENCE:
      case T_USER:    
                      if(shadow_flag && is_shadow(t))
                        Printf(java_type, Char(is_shadow(t)));
                      else
                        Printf(java_type, "long"); 
                      break;
      case T_ARRAY:
                      if(shadow_flag && is_shadow(t))
                        Printf(java_type, "%s[]", Char(is_shadow(t)));
                      else {
                        SwigType* array_type = get_array_type(t);
                        /* Arrays of arrays not shadowed properly yet */
                        if (SwigType_type(array_type) != T_ARRAY)
                          SwigToJavaType(array_type, pname, java_type, shadow_flag);
                        else 
                          Printf(java_type, "long");  
                        Printv(java_type, "[]", 0);
                      }
                      break;
      default:
        Printf(stderr, "SwigToJavaType: unhandled data type: %s\n", SwigType_str(t,0));
        break;
    }
//  printf("SwigToJavaType %d [%s]\n", SwigType_type(t), Char(java_type));
  }
}

/* Return JNI type in jni_type. */
void JAVA::SwigToJNIType(SwigType *t, String_or_char *pname, String* jni_type) {
  char *jtype = JavaTypeFromTypemap((char*)"jni", t, pname);

  if(jtype) {
    Printf(jni_type, jtype);
  }
  else {
    /* Map type here */
    switch(SwigType_type(t)) {
      case T_CHAR:      Printf(jni_type, "jbyte"); break;
      case T_SCHAR:     Printf(jni_type, "jbyte"); break;
      case T_UCHAR:     Printf(jni_type, "jshort"); break;
      case T_SHORT:     Printf(jni_type, "jshort"); break;
      case T_USHORT:    Printf(jni_type, "jint"); break;
      case T_INT:       Printf(jni_type, "jint"); break;
      case T_UINT:      Printf(jni_type, "jlong"); break;
      case T_LONG:      Printf(jni_type, "jlong"); break;
      case T_ULONG:     Printf(jni_type, "jlong"); break;
      case T_FLOAT:     Printf(jni_type, "jfloat"); break;
      case T_DOUBLE:    Printf(jni_type, "jdouble"); break;
      case T_BOOL:      Printf(jni_type, "jboolean"); break;
      case T_STRING:    Printf(jni_type, "jstring"); break;
      case T_VOID:      Printf(jni_type, "void"); break;
      case T_POINTER:
      case T_REFERENCE:
      case T_USER:      Printf(jni_type, "jlong"); break;
      case T_ARRAY:
                        {
                        SwigType* array_type = get_array_type(t);
                        if (SwigType_type(array_type) != T_ARRAY)
                          SwigToJNIType(array_type, pname, jni_type);
                        else
                          Printf(jni_type, "jlong");  // Arrays of arrays not implemented
                        Printv(jni_type, "Array", 0);
                        }
                      break;
      default:
        Printf(stderr, "SwigToJNIType: unhandled data type: %s\n", SwigType_str(t,0));
        break;
    }
//  printf("SwigToJNIType %d [%s]\n", SwigType_type(t), Char(jni_type));
  }
}

char *JAVA::SwigToJavaArrayType(SwigType *t) {
  switch(SwigType_type(t)) {
    case T_CHAR: return (char*)"Byte";
    case T_SCHAR: return (char*)"Byte";
    case T_UCHAR: return (char*)"Short";
    case T_SHORT: return (char*)"Short";
    case T_USHORT: return (char*)"Int";
    case T_INT: return (char*)"Int";
    case T_UINT: return (char*)"Long";
    case T_LONG: return (char*)"Long";
    case T_ULONG: return (char*)"Long";
    case T_FLOAT: return (char*)"Float";
    case T_DOUBLE: return (char*)"Double";
    case T_BOOL: return (char*)"Boolean";
    case T_STRING:	return (char*)"String";
    case T_POINTER:
    case T_REFERENCE:
    case T_ARRAY:
    case T_VOID:
    case T_USER:
    default : return (char*)"Long"; // Treat as a pointer
  }
}

/* JavaMethodSignature still needs updating for changes from SWIG1.3a3 to SWIG1.3a5 */
char *JAVA::JavaMethodSignature(SwigType *t, int ret, int inShadow) {
  if(SwigType_ispointer(t) == 1) {
	  switch(SwigType_type(t)) {
	    case T_CHAR:   return (char*)"Ljava/lang/String;";
	    case T_SCHAR:  return (char*)"[B";
	    case T_UCHAR:  return (char*)"[S";
	    case T_SHORT:  return (char*)"[S";
	    case T_USHORT: return (char*)"[I";
	    case T_INT:    return (char*)"[I";
	    case T_UINT:   return (char*)"[J";
	    case T_LONG:   return (char*)"[J";
	    case T_ULONG:  return (char*)"[J";
	    case T_FLOAT:  return (char*)"[F";
	    case T_DOUBLE: return (char*)"[D";
	    case T_BOOL:   return (char*)"[Z";
	    case T_STRING:	return (char*)"Ljava/lang/String;";
	    case T_POINTER:	return (char*)"[J";
	    case T_REFERENCE:	return (char*)"[J";
	    case T_ARRAY:	return (char*)"???";
	    case T_VOID:
        case T_USER:   if(inShadow && is_shadow(t))
                         return Char(is_shadow(t));
                       else return (char*)"J";
	  }
  } else if(SwigType_ispointer(t) > 1) {
    if(ret) return (char*)"J";
    else return (char*)"[J";
  } else {
	  switch(SwigType_type(t)) {
	    case T_CHAR: return (char*)"B";
	    case T_SCHAR: return (char*)"B";
	    case T_UCHAR: return (char*)"S";
	    case T_SHORT: return (char*)"S";
	    case T_USHORT: return (char*)"I";
	    case T_INT: return (char*)"I";
	    case T_UINT: return (char*)"J";
	    case T_LONG: return (char*)"J";
	    case T_ULONG: return (char*)"J";
	    case T_FLOAT: return (char*)"F";
	    case T_DOUBLE: return (char*)"D";
	    case T_BOOL: return (char*)"Z";
	    case T_STRING:	return (char*)"Ljava/lang/String;";
	    case T_POINTER:	return (char*)"J";
	    case T_REFERENCE:	return (char*)"J";
	    case T_ARRAY:	return (char*)"???";
	    case T_VOID: return (char*)"V";
	    case T_USER: return (char*)"J";
	  }
  }
  Printf(stderr, "JavaMethodSignature: unhandled SWIG type [%d] %s\n", SwigType_type(t), SwigType_str(t,0));
  return NULL;
}

char *JAVA::JavaTypeFromTypemap(char *op, SwigType *t, String_or_char *pname) {
  char *tm;
  char *c = bigbuf;
  if(!(tm = Swig_typemap_lookup(op, t, pname, (char*)"", (char*)"", NULL))) return NULL;
  while(*tm && (isspace(*tm) || *tm == '{')) tm++;
  while(*tm && *tm != '}') *c++ = *tm++;
  *c='\0';

  return strdup(bigbuf);
}

char *JAVA::makeValidJniName(char *name) {
  char *c = name;
  char *b = bigbuf;

  while(*c) {
    *b++ = *c;
    if(*c == '_')
      *b++ = '1';
    c++;
  }
  *b = '\0';

  return strdup(bigbuf);
}

// !! this approach fails for functions without arguments
char *JAVA::JNICALL(String_or_char *func) {
  if(jnic)
	sprintf(bigbuf, "(*jenv)->%s(jenv, ", Char(func));
  else
	sprintf(bigbuf, "jenv->%s(", Char(func));

  return strdup(bigbuf);
}

void JAVA::writeRegisterNatives()
{
  if(Len(registerNativesList) == 0)
    return;

  Printf(f_wrappers,"\n");
  Printf(f_wrappers,"JNINativeMethod nativeMethods[] = {\n");
  Printv(f_wrappers, registerNativesList, 0);
  Printf(f_wrappers, "};\n");

  Printf(f_wrappers,"\nint numberOfNativeMethods=sizeof(nativeMethods)/sizeof(JNINativeMethod);\n\n");

  // The registerNatives function

  Printv(f_wrappers,
	 "jint registerNatives(JNIEnv *jenv) {", "\n",
	 tab4, "jclass nativeClass = ", JNICALL((char*)"FindClass"),
	 "\"", jni_pkgstr, module, "\");","\n",
	 0);

  Printv(f_wrappers,
	 tab4, "if (nativeClass == 0)", "\n", tab8, "return -1;", "\n",
	 tab4, "return ", JNICALL((char*)"RegisterNatives"), "nativeClass, nativeMethods, ", "numberOfNativeMethods);", "\n",
	 "}", "\n", "\n",
	 0);

  // The unregisterNatives function

  Printv(f_wrappers,
	 "jint unregisterNatives(JNIEnv *jenv) {", "\n",
	 tab4, "jclass nativeClass = ", JNICALL((char*)"FindClass"),
	 "\"", jni_pkgstr, module, "\");","\n",
	 0);

  Printv(f_wrappers,
	 tab4, "if (nativeClass == 0)", "\n", tab8, "return -1;", "\n",
	 tab4, "// Sun documentation suggests that this method should not be invoked in ",
	 "\"normal native code\".", "\n",
	 tab4, "// return ", JNICALL((char*)"UnregisterNatives"), "nativeClass);", "\n",
	 tab4, "return 0;", "\n",
	 "}", "\n",
	 0);
}

// ---------------------------------------------------------------------
// JAVA::parse_args(int argc, char *argv[])
//
// Parse my command line options and initialize by variables.
// ---------------------------------------------------------------------

void JAVA::parse_args(int argc, char *argv[]) {

  // file::set_library(java_path);
  sprintf(LibDir,"%s", "java");


  // Look for certain command line options
  for (int i = 1; i < argc; i++) {
    if (argv[i]) {
      if (strcmp(argv[i],"-module") == 0) {
	if (argv[i+1]) {
	  set_module(argv[i+1]);
	  Swig_mark_arg(i);
	  Swig_mark_arg(i+1);
	  i++;
	} else {
	  Swig_arg_error();
	}
      } else if (strcmp(argv[i],"-package") == 0) {
	if (argv[i+1]) {
	  package = new char[strlen(argv[i+1])+1];
          strcpy(package, argv[i+1]);
	  Swig_mark_arg(i);
	  Swig_mark_arg(i+1);
	  i++;
	} else {
	  Swig_arg_error();
	}
      } else if (strcmp(argv[i],"-shadow") == 0) {
	    Swig_mark_arg(i);
        shadow = 1;
      } else if (strcmp(argv[i],"-jnic") == 0) {
	    Swig_mark_arg(i);
        jnic = 1;
      } else if (strcmp(argv[i],"-nofinalize") == 0) {
	    Swig_mark_arg(i);
        nofinalize = 1;
      } else if (strcmp(argv[i],"-rn") == 0) {
	    Swig_mark_arg(i);
        useRegisterNatives = 1;
      } else if (strcmp(argv[i],"-jnicpp") == 0) {
        Swig_mark_arg(i);
	    jnic = 0;
      } else if (strcmp(argv[i],"-help") == 0) {
	    Printf(stderr,"%s\n", usage);
      }
    }
  }

  if(jnic == -1) {
    if(CPlusPlus)
	jnic = 0;
    else jnic = 1;
  }

  // Add a symbol to the parser for conditional compilation
  // cpp::define("SWIGJAVA");
  Preprocessor_define((void *) "SWIGJAVA 1",0);

  // Add typemap definitions
  typemap_lang = (char*)"java";
}

// ---------------------------------------------------------------------
// void JAVA::parse()
//
// Start parsing an interface file for JAVA.
// ---------------------------------------------------------------------

void JAVA::parse() {

  Printf(stderr,"Generating wrappers for Java\n");

  shadow_classes = NewHash();
  shadow_classdef = NewString("");
  registerNativesList = NewString("");
  java_enum_code = NewString("");

  headers();       // Emit header files and other supporting code
  yyparse();       // Run the SWIG parser
}

// ---------------------------------------------------------------------
// JAVA::set_module(char *mod_name)
//
// Sets the module name.  Does nothing if it's already set (so it can
// be overriddent as a command line option).
//
//----------------------------------------------------------------------

void JAVA::set_module(char *mod_name) {
  if (module) return;
  module = new char[strlen(mod_name)+1];
  strcpy(module,mod_name);
}

// ---------------------------------------------------------------------
// JAVA::headers(void)
//
// Generate the appropriate header files for JAVA interface.
// ----------------------------------------------------------------------

void JAVA::headers(void)
{
  Swig_banner(f_header);               // Print the SWIG banner message
  Printf(f_header,"/* Implementation : Java */\n\n");

  // Include header file code fragment into the output
  // if (file::include("java.swg",f_header) == -1) {
  if (Swig_insert_file("java.swg",f_header) == -1) {
    Printf(stderr,"Fatal Error. Unable to locate 'java.swg'.\n");
    SWIG_exit (EXIT_FAILURE);
  }
}

// --------------------------------------------------------------------
// JAVA::initialize(void)
//
// Produces an initialization function.   Assumes that the init function
// name has already been specified.
// ---------------------------------------------------------------------

void JAVA::initialize()
{
  if (!module) {
    Printf(stderr,"*** Error. No module name specified.\n");
    SWIG_exit(1);
  }

  if(package) {
    String *s = NewString(package);
    Replace(s,".","_", DOH_REPLACE_ANY);
    Append(s,"_");
    c_pkgstr = Swig_copy_string(Char(s));
    Delete(s);

    String *s2 = NewString(package);
    Replace(s2,".","/", DOH_REPLACE_ANY);
    Append(s2,"/");
    jni_pkgstr = Swig_copy_string(Char(s2));
    Delete(s2);
  } else {
    package = c_pkgstr = jni_pkgstr = (char*)"";
  }

  sprintf(bigbuf, "Java_%s%s", c_pkgstr, module);
  c_pkgstr = Swig_copy_string(bigbuf);

  if (strchr(module + strlen(module)-1, '_')) //if module has a '_' as last character
    sprintf(bigbuf, "%s1_%%f", c_pkgstr); //separate double underscore with 1
  else
    sprintf(bigbuf, "%s_%%f", c_pkgstr);

  Swig_name_register((char*)"wrapper", Swig_copy_string(bigbuf));
  Swig_name_register((char*)"set", (char*)"set_%v");
  Swig_name_register((char*)"get", (char*)"get_%v");
  Swig_name_register((char*)"member", (char*)"%c_%m");

  // Generate the java class
  sprintf(bigbuf, "%s.java", module);
  if((f_java = fopen(bigbuf, "w")) == 0) {
    Printf(stderr,"Unable to open %s\n", bigbuf);
    SWIG_exit(1);
  }

  Printf(f_header, "#define J_CLASSNAME %s\n", module);
  if(package && *package) {
    Printf(f_java, "package %s;\n\n", package);
    Printf(f_header, "#define J_PACKAGE %s\n", package);
  } else {
    Printf(f_header, "#define J_PACKAGE\n");
  }
}

void emit_banner(FILE *f) {
  Printf(f, "/* ----------------------------------------------------------------------------\n");
  Printf(f, " * This file was automatically generated by SWIG (http://www.swig.org).\n");
  Printf(f, " * Version: %s\n", SWIG_VERSION);
  Printf(f, " *\n");
  Printf(f, " * Do not make changes to this file unless you know what you are doing--modify\n");
  Printf(f, " * the SWIG interface file instead.\n");
  Printf(f, " * ----------------------------------------------------------------------------- */\n\n");
}

void JAVA::emit_classdef() {
  if(!classdef_emitted) {
    emit_banner(f_java);
    Printf(f_java, "public class %s {\n", module);
  }
  classdef_emitted = 1;
}

// ---------------------------------------------------------------------
// JAVA::close(void)
//
// Wrap things up.  Close initialization function.
// ---------------------------------------------------------------------

void JAVA::close(void)
{
  if(!classdef_emitted) emit_classdef();

  // Write the enum initialisation code in a static block.
  // These are all the enums defined in the global c context.
  if (strlen(Char(java_enum_code)) != 0 )
    Printv(f_java, "  static {\n  // Initialise java constants from c/c++ enums\n", java_enum_code, "  }\n",0);

  // Finish off the java class
  Printf(f_java, "}\n");
  fclose(f_java);

  if(useRegisterNatives)
	writeRegisterNatives();

  Delete(shadow_classes);
  Delete(shadow_classdef);
  Delete(registerNativesList);
  Delete(java_enum_code);
}

// ----------------------------------------------------------------------
// JAVA::create_command(char *cname, char *iname)
//
// Creates a JAVA command from a C function.
// ----------------------------------------------------------------------

void JAVA::create_command(char *cname, char *iname) {
}

void JAVA::add_native(char *name, char *iname, SwigType *t, ParmList *l) {
  native_func = 1;
  create_function(name, iname, t, l);
  native_func = 0;
}

// ----------------------------------------------------------------------
// JAVA::create_function(char *name, char *iname, SwigType *d, ParmList *l)
//
// Create a function declaration and register it with the interpreter.
// ----------------------------------------------------------------------

void JAVA::create_function(char *name, char *iname, SwigType *t, ParmList *l)
{
  char      source[256], target[256];
  char	 	*tm;
  char		*javaReturnSignature = 0;
  String    *jnirettype = NewString("");
  String    *javarettype = NewString("");
  String    *cleanup = NewString("");
  String    *outarg = NewString("");
  String    *body = NewString("");
  String    *javaParameterSignature = NewString("");

  /* 
  Generate the java class wrapper function ie the java shadow class. Only done for public
  member variables. That is this generates the getters/setters for member variables.
  */
  if(shadow && wrapping_member && !enum_flag) {
    String *member_function_name = NewString("");
    String *java_function_name = NewString(iname);
    if(strcmp(iname, Char(Swig_name_set(Swig_name_member(shadow_classname, shadow_variable_name)))) == 0)
      Printf(member_function_name,"set");
    else Printf(member_function_name,"get");
    Putc(toupper((int) *shadow_variable_name), member_function_name);
    Printf(member_function_name, "%s", shadow_variable_name+1);

    cpp_func(Char(member_function_name), t, l, java_function_name);

    Delete(java_function_name);
    Delete(member_function_name);
  }

  /*
  The rest of create_function deals with generating the java wrapper function (that wraps
  a c/c++ function) and generating the JNI c code. Each java wrapper function has a 
  matching JNI c function call.
  */

  // A new wrapper function object
  Wrapper *f = NewWrapper();

  if(!classdef_emitted) emit_classdef();

  // Make a wrapper name for this function
  char *jniname = makeValidJniName(iname);
  String *wname = Swig_name_wrapper(jniname);

  free(jniname);

  /* Get the java and jni types of the return */
  SwigToJNIType(t, iname, jnirettype);
  SwigToJavaType(t, iname, javarettype, 0);

  // If dumping the registerNative outputs, store the method return type
  // signature
  if (useRegisterNatives) {
      javaReturnSignature = JavaMethodSignature(t, 1, 0);
  }

  if (SwigType_type(t) != T_VOID) {
	 Wrapper_add_localv(f,"jresult", jnirettype, "jresult = 0",0);
  }

  Printf(f_java, "  %s ", method_modifiers);
  Printf(f_java, "native %s %s(", javarettype, iname);

  if(!jnic) Printv(f->def, "extern \"C\"\n", 0);
  Printv(f->def, "JNIEXPORT ", jnirettype, " JNICALL ", wname, "(JNIEnv *jenv, jclass jcls", 0);

  // Emit all of the local variables for holding arguments.
  int pcount = emit_args(t,l,f);

  int gencomma = 0;

  // Now walk the function parameter list and generate code to get arguments
  Parm *p = l;
  for (int i = 0; i < pcount ; i++, p = Getnext(p)) {
    SwigType *pt = Gettype(p);
    String   *pn = Getname(p);
    String   *javaparamtype = NewString("");
    String   *jni_param_type = NewString("");

    // Produce string representation of source and target arguments
    sprintf(target,"%s", Char(Getlname(p)));
    sprintf(source,"j%s", target);

    if (useRegisterNatives) {
      Printv(javaParameterSignature, JavaMethodSignature(pt, 0, 0), 0);
    }

    if(Getignore(p)) continue;

    /* Get the java and jni types of the parameter */
    SwigToJNIType(pt, pn, jni_param_type);
    SwigToJavaType(pt, pn, javaparamtype, 0); 

    /* Add to java function header */
      if(gencomma) Printf(f_java, ", ");
      Printf(f_java, "%s %s", javaparamtype, source);

      gencomma = 1;

      // Add to Jni function header
      Printv(f->def, ", ", jni_param_type, " ", source, 0);

      // Get typemap for this argument
      tm = Swig_typemap_lookup((char*)"in",pt,pn,source,target,f);
      if (tm) {
        Printf(f->code,"%s\n", tm);
        Replace(f->code,"$arg",source, DOH_REPLACE_ANY);
      } else {
        switch(SwigType_type(pt)) {
        case T_BOOL:
        case T_CHAR:
        case T_SCHAR:
        case T_UCHAR:
        case T_SHORT:
        case T_USHORT:
        case T_INT:
        case T_UINT:
        case T_LONG:
        case T_ULONG:
        case T_FLOAT:
        case T_DOUBLE:
          Printv(f->code, tab4, target, " = (", SwigType_lstr(pt,0), ") ", source, ";\n", 0);
          break;
        case T_STRING:
          Printv(f->code, tab4, target, " = (", source, ") ? (char *)", JNICALL((char*)"GetStringUTFChars"), source, ", 0) : NULL;\n", 0);
          break;
        case T_VOID:
          break;
        case T_USER:
          Printv(f->code, tab4, target, " = *(", SwigType_lstr(pt,0), "**)&", source, ";\n", 0);
          break;
        case T_POINTER:
        case T_REFERENCE:
          Printv(f->code, tab4, target, " = *(", SwigType_lstr(pt,0), "*)&", source, ";\n", 0);
          break;
        case T_ARRAY:
          {
          String *jni_array_type = NewString("");
          SwigType *array_type = get_array_type(pt);
          char *java_array_type = SwigToJavaArrayType(array_type);
          SwigToJNIType(array_type, pn, jni_array_type);

          String *ctype = SwigType_lstr(array_type, 0);

          String *basic_jniptrtype = NewStringf("%s*", jni_array_type);
          String *source_length = NewStringf("%s%s)", JNICALL((char*)"GetArrayLength"), source);
          String *c_array = NewStringf("%s_carray", source);
          String *array_len = NewStringf("%s_len", source);
          String *get_array_func = NewStringf("Get%sArrayElements", java_array_type);

          Wrapper_add_localv(f, "i", "int", "i", 0); // Only gets added once if called more than once
          Wrapper_add_localv(f, c_array, basic_jniptrtype, c_array, 0);
          Wrapper_add_localv(f, array_len, "jsize", array_len, "= ", source_length, 0);

          Printv(f->code, tab4, c_array, " = ", JNICALL(get_array_func), source, ", 0);\n", 0);
          Printv(f->code, tab4, target, " = (", SwigType_lstr(pt, 0), ") malloc(", array_len, " * sizeof(", ctype, "));\n", 0);
          Printv(f->code, tab4, "for(i=0; i<", array_len, "; i++)\n", 0);

            switch(SwigType_type(array_type)) {
            case T_USER:
              Printv(f->code, tab8, target, "[i] = **(", ctype, "**)&", c_array, "[i];\n", 0);
              break;
            case T_POINTER:
              Printv(f->code, tab8, target, "[i] = *(", ctype, "*)&", c_array, "[i];\n", 0);
              break;
            default:
              Printv(f->code, tab8, target, "[i] = (", ctype, ")", c_array, "[i];\n", 0);
            break;
            }

          Delete(jni_array_type);
          Delete(basic_jniptrtype);
          Delete(source_length);
          Delete(c_array);
          Delete(array_len);
          Delete(get_array_func);
          }
          break;
        default:
          Printf(stderr,"%s : Line %d. Error: Unknown typecode for type %s\n", input_file,line_number,SwigType_str(pt,0));
          break;
        }
      }

    // Check to see if there was any sort of a constaint typemap
    if ((tm = Swig_typemap_lookup((char*)"check",pt,pn,source,target,NULL))) {
      // Yep.  Use it instead of the default
      Printf(f->code,"%s\n", tm);
      Replace(f->code,"$arg",source, DOH_REPLACE_ANY);
    }

    // Check if there was any cleanup code (save it for later)
    if ((tm = Swig_typemap_lookup((char*)"freearg",pt,pn,source,target,NULL))) {
      // Yep.  Use it instead of the default
      Printf(cleanup,"%s\n", tm);
      Replace(cleanup,"$arg",source, DOH_REPLACE_ANY);
    }

    if ((tm = Swig_typemap_lookup((char*)"argout",pt,pn,source,target,NULL))) {
      // Yep.  Use it instead of the default
      Printf(outarg,"%s\n", tm);
      Replace(outarg,"$arg",source, DOH_REPLACE_ANY);
    } else {
        switch(SwigType_type(pt)) {
        case T_BOOL:
        case T_CHAR:
        case T_SCHAR:
        case T_UCHAR:
        case T_SHORT:
        case T_USHORT:
        case T_INT:
        case T_UINT:
        case T_LONG:
        case T_ULONG:
        case T_FLOAT:
        case T_DOUBLE:
          // nothing to do
          break;
        case T_STRING:
          Printv(outarg, tab4, "if(", target,") ", JNICALL((char*)"ReleaseStringUTFChars"), source, ", ", target, ");\n", 0);
          break;
        case T_VOID:
        case T_USER:
        case T_POINTER:
        case T_REFERENCE:
          // nothing to do
          break;
        case T_ARRAY:
          {
          String *jni_array_type = NewString("");
          SwigType *array_type = get_array_type(pt);
          char *java_array_type = SwigToJavaArrayType(array_type);
          SwigToJNIType(array_type, pn, jni_array_type);

          String *c_array = NewStringf("%s_carray", source);
          String *release_array_func = NewStringf("Release%sArrayElements", java_array_type);

          Printv(outarg, tab4, JNICALL(release_array_func), source, ", ", c_array, ", 0);\n", 0);
          Printv(outarg, tab4, "free(", target, ");\n", 0);

          Delete(jni_array_type);
          Delete(c_array);
          Delete(release_array_func);
          }
          break;
        default:
          Printf(stderr,"%s : Line %d. Error: Unknown typecode for type %s\n", input_file,line_number,SwigType_str(pt,0));
          break;
        }
      }
    Delete(javaparamtype);
    Delete(jni_param_type);
    }

  Printf(f_java, ");\n");
  Printf(f->def,") {");

  // Now write code to make the function call

  if(!native_func)
	emit_func_call(name,t,l,f);
  // Return value if necessary

  if((SwigType_type(t) != T_VOID) && !native_func) {
    if ((tm = Swig_typemap_lookup((char*)"out",t,iname,(char*)"result",(char*)"jresult",NULL))) {
      Printf(f->code,"%s\n", tm);
    } else {
        switch(SwigType_type(t)) {
        case T_BOOL:
        case T_CHAR:
        case T_SCHAR:
        case T_UCHAR:
        case T_SHORT:
        case T_USHORT:
        case T_INT:
        case T_UINT:
        case T_LONG:
        case T_ULONG:
        case T_FLOAT:
        case T_DOUBLE:
          Printv(f->code, tab4, "jresult = (", jnirettype, ") result;\n", 0);
          break;
        case T_STRING:
          Printv(f->code, tab4, "if(result != NULL)\n", 0);
          Printv(f->code, tab8, "jresult = (jstring)", JNICALL((char*)"NewStringUTF"),  "result);\n", 0);
          break;
        case T_VOID:
          break;
        case T_USER:
          Printv(f->code, tab4, "*(", SwigType_lstr(t,0), "**)&jresult = result;\n", 0);
          break;
        case T_POINTER:
        case T_REFERENCE:
          Printv(f->code, tab4, "*(", SwigType_lstr(t,0), "*)&jresult = result;\n", 0);
          break;
        case T_ARRAY:
            {
            // Handle return values that are one dimension arrays 
            // Todo: What about void pointers?
            // Todo: Check on array size - check user has passed correct size in

            const int dim_no = 0; // Dimension number (0 == 1st ) - currently only 1 supported
            String *jnitype_ptr = NewString("");
            String *jnicall_new = NewString("");
            String *jnicall_get = NewString("");
            String *jnicall_release = NewString("");
            String *jni_array_type = NewString("");
            SwigType *array_type = get_array_type(t);
            char *java_array_type = SwigToJavaArrayType(array_type);
            SwigToJNIType(array_type, iname, jni_array_type);

            Printv(jnitype_ptr, jni_array_type, "*", 0);
            Wrapper_add_localv(f, "jnitype_ptr", jnitype_ptr, "jnitype_ptr", "= 0", 0);
            Wrapper_add_localv(f, "k", "int", "k", 0);

            // Create a java array for return to Java subsystem, eg for int array
            // String jresult = (*jenv)->NewIntArray(jenv, array_size); 
            Printv(jnicall_new, "New", java_array_type, "Array", 0);
            Printv(f->code, tab4, "jresult = ", JNICALL((char*)jnicall_new), SwigType_array_getdim(t, dim_no), ");\n", 0);

            // Get the c memory pointer to the java array, eg for int array
            // jnitype_ptr = (*jenv)->GetIntArrayElements(jenv, jresult, 0);
            Printv(jnicall_get, "Get", java_array_type, "ArrayElements", 0);
            Printv(f->code, tab4, "jnitype_ptr = ", JNICALL((char*)jnicall_get), "jresult, 0);\n", 0);

            // Populate the java array from the c array
            Printv(f->code, tab4, "for (k=0; k<", SwigType_array_getdim(t, dim_no), "; k++)\n", 0);
            switch(SwigType_type(array_type)) {
            case T_USER:
                Printv(f->code, tab8, "*(", SwigType_lstr(array_type, 0), "**)&jnitype_ptr[k] = &result[k];\n", 0);
              break;
            case T_POINTER:
                Printv(f->code, tab8, "*(", SwigType_lstr(array_type, 0), "*)&jnitype_ptr[k] = result[k];\n", 0);
              break;
            default:
                Printv(f->code, tab8, "jnitype_ptr[k] = (", jni_array_type, ")result[k];\n", 0);
            break;
            }
            // Release reference to the array so that it may be garbage collected in the future
            // (*jenv)->ReleaseIntArrayElements(jenv, jresult, _arg00, 0);
            Printv(jnicall_release, "Release", java_array_type, "ArrayElements", 0);
            Printv(f->code, tab4, JNICALL((char*)jnicall_release), "jresult, ", "jnitype_ptr", ", 0);\n", 0);
            Delete(jnitype_ptr);
            Delete(jnicall_new);
            Delete(jnicall_get);
            Delete(jnicall_release);
            Delete(jni_array_type);
            }
          break;
        default:
          Printf(stderr,"%s : Line %d. Error: Unknown typecode for type %s\n", input_file,line_number,SwigType_str(t,0));
          break;
        }
      }
  }

  // Dump argument output code;
  Printv(f->code, outarg, 0);

  // Dump the argument cleanup code
  Printv(f->code,cleanup, 0);

  // Look for any remaining cleanup

  if (NewObject) {
    if ((tm = Swig_typemap_lookup((char*)"newfree",t,iname,(char*)"result",(char*)"",NULL))) {
      Printf(f->code,"%s\n", tm);
    }
  }

  if((SwigType_type(t) != T_VOID) && !native_func) {
    if ((tm = Swig_typemap_lookup((char*)"ret",t,iname,(char*)"result",(char*)"jresult", NULL))) {
      Printf(f->code,"%s\n", tm);
    }
  }

  // Wrap things up (in a manner of speaking)
  if(SwigType_type(t) != T_VOID)
    Printv(f->code, tab4, "return jresult;\n", 0);
  Printf(f->code, "}\n");

  // Substitute the cleanup code (some exception handlers like to have this)
  Replace(f->code,"$cleanup",cleanup, DOH_REPLACE_ANY);

  // Emit the function

  if(!native_func)
	Wrapper_print(f,f_wrappers);

 // If registerNatives is active, store the table entry for this method
  if (useRegisterNatives) {
    Printv(registerNativesList,
	   tab4, "{",
	   "\"", name, "\", \"(", javaParameterSignature, ")", javaReturnSignature, "\", ", wname,
	   "},\n",
	   0);

  }

  Delete(jnirettype);
  Delete(javarettype);
  Delete(cleanup);
  Delete(outarg);
  Delete(body);
  Delete(javaParameterSignature);
  DelWrapper(f);
}

// -----------------------------------------------------------------------
// JAVA::link_variable(char *name, char *iname, SwigType *t)
//
// Create a JAVA link to a C variable.
// -----------------------------------------------------------------------

void JAVA::link_variable(char *name, char *iname, SwigType *t)
{
  emit_set_get(name,iname, t);
}

// -----------------------------------------------------------------------
// JAVA::declare_const(char *name, char *iname, SwigType *type, char *value)
// ------------------------------------------------------------------------

void JAVA::declare_const(char *name, char *iname, SwigType *type, char *value) {
  int OldStatus = Status;
  char *tm;
  FILE *jfile;
  char *jname;
  Status = STAT_READONLY;
  String *java_type = NewString("");

  if(!classdef_emitted) emit_classdef();

  if(shadow && wrapping_member) {
    if(!shadow_classdef_emitted) emit_shadow_classdef();
    jfile = f_shadow;
    jname = shadow_variable_name;
  } else {
    jfile = f_java;
    jname = name;
  }

  if ((tm = Swig_typemap_lookup((char*)"const",type,name,name,iname,NULL))) {
    String *str = NewString(tm);
    Replace(str,"$value",value, DOH_REPLACE_ANY);
    Printf(jfile,"  %s\n\n", str);
    Delete(str);
  } else {
    SwigToJavaType(type, iname, java_type, shadow && wrapping_member);
    if(strcmp(jname, value) == 0 || strstr(value,"::") != NULL) {
      /* 
      We have found an enum. 
      The enum implementation is done using a public final static int
      in Java. They are then initialised through a JNI call to c in a Java static block.
      */
      Printf(jfile, "  public final static %s %s;\n", java_type, jname, value);

      if(shadow && wrapping_member) {
        Printv(shadow_enum_code, tab4, jname, " = ", module, ".", Swig_name_get(iname), "();\n", 0);
        /* 
        The following is a work around for an apparent bug in the swig core.
        When an enum is defined in a c++ class the emit_func_call() should 
        output ClassName::EnumName, but instead it outputs ClassName_EnumName.
        */
        Printv(f_header, "#define ", iname, " ", value, "\n", 0);
      }
      else {
        Printv(java_enum_code, tab4, jname, " = ", Swig_name_get(iname), "();\n", 0);
      }
      enum_flag = 1;
      emit_set_get(name,iname, type);
      enum_flag = 0;
    } else {
      if(SwigType_type(type) == T_STRING)
        Printf(jfile, "  public final static %s %s = \"%s\";\n", java_type, jname, value);
      else
        Printf(jfile, "  public final static %s %s = %s;\n", java_type, jname, value);
    }
  }
  Delete(java_type);
  Status = OldStatus;
}

void JAVA::pragma(char *lang, char *code, char *value) {
  if(strcmp(lang, "java") != 0) return;

  String *s = NewString(value);
  Replace(s,"\\\"", "\"", DOH_REPLACE_ANY);
  if(strcmp(code, "import") == 0) {
    jimport = Swig_copy_string(Char(s));
    Printf(f_java, "// pragma\nimport %s;\n\n", jimport);
  } else if(strcmp(code, "module") == 0) {
    if(!classdef_emitted) emit_classdef();
    Printf(f_java, "// pragma\n");
    Printf(f_java, "%s", s);
    Printf(f_java, "\n\n");
  } else if(strcmp(code, "shadow") == 0) {
    if(shadow && f_shadow) {
      Printf(f_shadow, "// pragma\n");
      Printf(f_shadow, "%s", s);
      Printf(f_shadow, "\n\n");
    }
  } else if(strcmp(code, "modifiers") == 0) {
    method_modifiers = Swig_copy_string(value);
  } else {
    Printf(stderr,"%s : Line %d. Unrecognized pragma.\n", input_file,line_number);
  }
  Delete(s);
}

// ---------------------------------------------------------------------
// C++ Handling
//
// The following functions provide some support for C++ classes and
// C structs.
// ---------------------------------------------------------------------

void JAVA::add_typedef(SwigType *t, char *name) {
  if(!shadow) return;
  if (is_shadow(t)) {
    cpp_class_decl(name,Char(is_shadow(t)),"");
  }
}

void JAVA::cpp_open_class(char *classname, char *rename, char *ctype, int strip) {

  this->Language::cpp_open_class(classname,rename,ctype,strip);

  if(!shadow) return;

  if(rename)
    shadow_classname = Swig_copy_string(rename);
  else shadow_classname = Swig_copy_string(classname);

  if (strcmp(shadow_classname, module) == 0) {
    Printf(stderr, "class name cannot be equal to module name: %s\n", shadow_classname);
    SWIG_exit(1);
  }

  Setattr(shadow_classes,classname, shadow_classname);
  if(ctype && strcmp(ctype, "struct") == 0) {
    sprintf(bigbuf, "struct %s", classname);
    Setattr(shadow_classes, bigbuf, shadow_classname);
  }

  sprintf(bigbuf, "%s.java", shadow_classname);
  if(!(f_shadow = fopen(bigbuf, "w"))) {
    Printf(stderr, "Unable to create shadow class file: %s\n", bigbuf);
  }

  emit_banner(f_shadow);

  if(*package)
	Printf(f_shadow, "package %s;\n\n", package);
  else Printf(f_shadow, "import %s;\n\n", module);
  if(jimport != NULL)
	Printf(f_shadow, "import %s;\n\n", jimport);

  Clear(shadow_classdef);
  Printv(shadow_classdef, "public class ", shadow_classname, " %BASECLASS% ", "{\n", 0);

  shadow_baseclass = (char*) "";
  shadow_classdef_emitted = 0;
  have_default_constructor = 0;
  shadow_enum_code = NewString("");
}

void JAVA::emit_shadow_classdef() {
  if(*shadow_baseclass) {
    sprintf(bigbuf, "extends %s", shadow_baseclass);
    Replace(shadow_classdef,"%BASECLASS%", bigbuf, DOH_REPLACE_ANY);
  } else {
    Replace(shadow_classdef,"%BASECLASS%", "",DOH_REPLACE_ANY);
  }
  Printv(shadow_classdef,
    "  private long _cPtr = 0;\n",
    "  private boolean _cMemOwn = false;\n",
    "\n",
    "  public $class(long cPointer, boolean cMemoryOwn) {\n",
    "    _cPtr = cPointer;\n",
    "    _cMemOwn = cMemoryOwn;\n",
    "  }\n",
    "\n",
    "  public long getCPtr() {\n",
    "    return _cPtr;\n",
    "  };\n",
    "\n", 0);
  Replace(shadow_classdef, "$class", shadow_classname, DOH_REPLACE_ANY);

  Printv(f_shadow, shadow_classdef,0);
  shadow_classdef_emitted = 1;
}

void JAVA::cpp_close_class() {
  this->Language::cpp_close_class();
  if(!shadow) return;

  if(!shadow_classdef_emitted) emit_shadow_classdef();

  if(have_default_constructor == 0) {
    Printf(f_shadow, "  protected %s() {}\n\n", shadow_classname);
  }

  // Write the enum initialisation code in a static block.
  // These are all the enums defined within the c++ class.
  if (strlen(Char(shadow_enum_code)) != 0 )
    Printv(f_shadow, "  static {\n  // Initialise java constants from c++ enums\n", shadow_enum_code, "  }\n",0);

  Printf(f_shadow, "}\n");
  fclose(f_shadow);
  f_shadow = NULL;

  free(shadow_classname);
  shadow_classname = NULL;
  Delete(shadow_enum_code);
}

void JAVA::cpp_member_func(char *name, char *iname, SwigType *t, ParmList *l) {
  this->Language::cpp_member_func(name,iname,t,l);
  String* java_function_name = Swig_name_member(shadow_classname,iname);

  cpp_func(iname, t, l, java_function_name);
}

void JAVA::cpp_static_func(char *name, char *iname, SwigType *t, ParmList *l) {
  this->Language::cpp_static_func(name,iname,t,l);
  String* java_function_name = Swig_name_member(shadow_classname,iname);

  static_flag = 1;
  cpp_func(iname, t, l, java_function_name);
  static_flag = 0;
}

/* 
Function called for creating a java class wrapper function around a c++ function in the 
java wrapper class. Used for both static and non static functions.
C++ static functions map to java static functions.
iname is the name of the java class wrapper function, which in turn will call 
java_function_name, the java function which wraps the c++ function.
*/
void JAVA::cpp_func(char *iname, SwigType *t, ParmList *l, String* java_function_name) {
  char     arg[256];
  String   *nativecall = NewString("");
  String   *shadowrettype = NewString("");
  String   *user_arrays = NewString("");

  if(!shadow) return;
  if(!shadow_classdef_emitted) emit_shadow_classdef();

  /* Get the java return type */
  SwigToJavaType(t, iname, shadowrettype, shadow);

  Printf(f_shadow, "  public %s %s %s(", (static_flag ? "static":""), shadowrettype, iname);

  if(SwigType_type(t) == T_ARRAY && is_shadow(get_array_type(t))) {
    Printf(nativecall, "long[] cArray = ");
  }
  else {
    if(SwigType_type(t) != T_VOID) {
      Printf(nativecall,"return ");
    }
    if(is_shadow(t)) {
      Printv(nativecall, "new ", shadowrettype, "(", 0);
    }
  }

  Printv(nativecall, module, ".", java_function_name, "(", 0);
  if (!static_flag)
    Printv(nativecall, "getCPtr()", 0);

  int pcount = ParmList_len(l);

  /* Output each parameter */
  Parm *p = l;
  for (int i = 0; i < pcount ; i++, p = Getnext(p)) {
    /* Ignore the 'self' or 'this' argument for variable wrappers */
    if (!(variable_wrapper_flag && i==0)) 
    {
      SwigType *pt = Gettype(p);
      String   *pn = Getname(p);
      String   *javaparamtype = NewString("");
  
      /* Produce string representation of source and target arguments */
      if(pn && *(Char(pn)))
        strcpy(arg,Char(pn));
      else {
        sprintf(arg,"arg%d",i);
      }
  
      if(!(i==0 && (static_flag || variable_wrapper_flag))) 
        Printv(nativecall, ", ", 0);

      if(SwigType_type(pt) == T_ARRAY && is_shadow(get_array_type(pt))) {
        Printv(user_arrays, tab4, "long[] $arg_cArray = new long[$arg.length];\n", 0);
        Printv(user_arrays, tab4, "for (int i=0; i<$arg.length; i++)\n", 0);
        Printv(user_arrays, tab4, "  $arg_cArray[i] = $arg[i].getCPtr();\n", 0);
        Replace(user_arrays, "$arg", pn, DOH_REPLACE_ANY);

        Printv(nativecall, arg, "_cArray", 0);
      } else if (is_shadow(pt)) {
        Printv(nativecall, arg, ".getCPtr()", 0);
      } else Printv(nativecall, arg, 0);

      /* Get the java type of the parameter */
      SwigToJavaType(pt, pn, javaparamtype, shadow); 
  
      /* Add to java shadow function header */
      Printf(f_shadow, "%s %s", javaparamtype, arg);
      if(i != pcount-1) {
        Printf(f_shadow, ", ");
      }

      Delete(javaparamtype);
    }
  }

  if(SwigType_type(t) == T_ARRAY && is_shadow(get_array_type(t))) {
    String* array_ret = NewString("");
    String* user_return_type = NewString("");
    SwigType *array_type = get_array_type(t);
    Printf(array_ret,");\n");
    Printv(array_ret, tab4, "$type[] arrayWrapper = new $type[cArray.length];\n", 0);
    Printv(array_ret, tab4, "for (int i=0; i<cArray.length; i++)\n", 0);
    Printv(array_ret, tab4, "  arrayWrapper[i] = new $type(cArray[i], false);\n", 0);
    Printv(array_ret, tab4, "return arrayWrapper;\n", 0);

    SwigToJavaType(array_type, iname, user_return_type, shadow);
    Replace(array_ret, "$type", user_return_type, DOH_REPLACE_ANY);
    Printv(nativecall, array_ret, 0);
    Delete(array_ret);
    Delete(user_return_type);
  }
  else {
    if(is_shadow(t)) {
      switch(SwigType_type(t)) {
      case T_USER:
        Printf(nativecall, "), true");
        break;
      case T_REFERENCE:
      case T_POINTER:
        Printf(nativecall, "), false");
        break;
      default:
        Printf(stderr, "Internal Error: unknown shadow type: %s\n", SwigType_str(t,0));
        break;
      }
    }
    Printf(nativecall,");\n");
  }

  Printf(f_shadow, ") {\n");
  Printv(f_shadow, user_arrays, 0);
  Printf(f_shadow, "\t%s", nativecall);
  Printf(f_shadow, "  }\n\n");
  Delete(shadowrettype);
  Delete(nativecall);
  Delete(user_arrays);
}

void JAVA::cpp_constructor(char *name, char *iname, ParmList *l) {
  this->Language::cpp_constructor(name,iname,l);

  if(!shadow) return;
  if(!shadow_classdef_emitted) emit_shadow_classdef();

  String *nativecall = NewString("");
  char arg[256];

  Printf(f_shadow, "  public %s(", shadow_classname);

  Printv(nativecall, "    if(getCPtr() == 0) {\n", 0);
  if (iname != NULL)
    Printv(nativecall, tab8, " _cPtr = ", module, ".", Swig_name_construct(iname), "(", 0);
  else
    Printv(nativecall, tab8, " _cPtr = ", module, ".", Swig_name_construct(shadow_classname), "(", 0);

  int pcount = ParmList_len(l);
  if(pcount == 0)  // We must have a default constructor
    have_default_constructor = 1;

  /* Output each parameter */
  Parm *p = l;
  for (int i = 0; i < pcount ; i++, p = Getnext(p)) {
    SwigType *pt = Gettype(p);
    String   *pn = Getname(p);
    String   *javaparamtype = NewString("");

    /* Produce string representation of source and target arguments */
    if(pn && *(Char(pn)))
      strcpy(arg,Char(pn));
    else {
      sprintf(arg,"arg%d",i);
    }

    if(is_shadow(pt)) {
	  Printv(nativecall, arg, ".getCPtr()", 0);
    } else Printv(nativecall, arg, 0);

    /* Get the java type of the parameter */
    SwigToJavaType(pt, pn, javaparamtype, shadow); 

    /* Add to java shadow function header */
    Printf(f_shadow, "%s %s", javaparamtype, arg);

    if(i != pcount-1) {
      Printf(nativecall, ", ");
      Printf(f_shadow, ", ");
    }
    Delete(javaparamtype);
  }


  Printf(f_shadow, ") {\n");
  Printv(nativecall,
	 ");\n",
	 tab8, " _cMemOwn = true;\n",
	 "    }\n",
	 0);

  Printf(f_shadow, "%s", nativecall);
  Printf(f_shadow, "  }\n\n");
  Delete(nativecall);
}

void JAVA::cpp_destructor(char *name, char *newname) {
  this->Language::cpp_destructor(name,newname);

  if(!shadow) return;
  if(!shadow_classdef_emitted) emit_shadow_classdef();

  char *realname = (newname) ? newname : name;

  if(!nofinalize) {
    Printf(f_shadow, "  protected void finalize() {\n");
    Printf(f_shadow, "    _delete();\n");
    Printf(f_shadow, "  };\n\n");
  }

  Printf(f_shadow, "  synchronized public void _delete() {\n");
  Printf(f_shadow, "    if(getCPtr() != 0 && _cMemOwn) {\n");
  Printf(f_shadow, "\t%s.%s(_cPtr);\n", module, Swig_name_destroy(shadow_classname));
  Printf(f_shadow, "\t_cPtr = 0;\n");
  Printf(f_shadow, "    }\n");
  Printf(f_shadow, "  }\n\n");
}

void JAVA::cpp_class_decl(char *name, char *rename, char *type) {
  String *stype;
/* Register the class as one for which there will be a java shadow class */
  if (shadow) {
    stype = NewString(name);
    SwigType_add_pointer(stype);
    Setattr(shadow_classes,stype,rename);
    Delete(stype);
    if (strlen(type) > 0) {
      stype = NewStringf("%s %s",type,name);
      SwigType_add_pointer(stype);
      Setattr(shadow_classes,stype,rename);
      Delete(stype);
    }
  }
}

void JAVA::cpp_inherit(char **baseclass, int) {
  this->Language::cpp_inherit(baseclass, 0);

  if(!shadow) return;

  int i = 0;
  while(baseclass[i]) i++;

  if(i > 1)
    Printf(stderr, "Warning: %s inherits from multiple base classes. Multiple inheritance is not supported.\n", shadow_classname);

  i=0;
  char *bc;
  if (baseclass[i]) {
    /* See if this is a class we know about */
    String *b = NewString(baseclass[i]);
    bc = Char(is_shadow(b));
    Delete(b);
    if (bc)
      shadow_baseclass = bc;
  }
}

void JAVA::cpp_variable(char *name, char *iname, SwigType *t) {
  shadow_variable_name = Swig_copy_string((iname) ? iname : name);

  wrapping_member = 1;
  variable_wrapper_flag = 1;
  this->Language::cpp_variable(name, iname, t);
  wrapping_member = 0;
  variable_wrapper_flag = 0;
}

void JAVA::cpp_static_var(char *name, char *iname, SwigType *t) {
  shadow_variable_name = Swig_copy_string((iname) ? iname : name);

/*
  wrapping_member = 1;
  static_flag = 1;
  variable_wrapper_flag = 1;
  this->Language::cpp_static_var(name, iname, t);
  wrapping_member = 0;
  static_flag = 0;
  variable_wrapper_flag = 0;
*/
  Printf(stderr, "Ignoring %s::%s. Static member variables are broken in SWIG!\n", shadow_classname, shadow_variable_name);
}

void JAVA::cpp_declare_const(char *name, char *iname, SwigType *type, char *value) {
  shadow_variable_name = Swig_copy_string((iname) ? iname : name);

  wrapping_member = 1;
  this->Language::cpp_declare_const(name, iname, type, value);
  wrapping_member = 0;
}



