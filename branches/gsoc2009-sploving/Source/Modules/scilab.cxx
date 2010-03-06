/* -----------------------------------------------------------------------------
 * This file is part of SWIG, which is licensed as a whole under version 3 
 * (or any later version) of the GNU General Public License. Some additional
 * terms also apply to certain portions of SWIG. The full details of the SWIG
 * license and copyrights can be found in the LICENSE and COPYRIGHT files
 * included with the SWIG source code as distributed by the SWIG developers
 * and at http://www.swig.org/legal.html.
 *
 * scilab.cxx
 *
 * Scilab language module for SWIG.
 * ----------------------------------------------------------------------------- */

char cvsroot_scilab_cxx[] = "$Id$";

#include "swigmod.h"

static const char *usage = (char *) "\
Scilab Options (available with -scilab)\n\
     (none yet)\n\n";


class SCILAB:public Language {

private:
  File *f_begin;
  File *f_runtime;
  File *f_header;
  File *f_wrappers;
  File *f_init;
  
  String *f_builder_code;
  bool hasfunction_flag;
  
public:
  SCILAB():
    f_builder_code(NewString("")), hasfunction_flag(false) {
    allow_overloading();
  }
  
    
  /* ------------------------------------------------------------
   * main()
   * ------------------------------------------------------------ */
  
  virtual void main(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
      if (argv[i]) {
	if (strcmp(argv[i], "-help") == 0) {
	  fputs(usage, stderr);
	}
      }
    }
    
    /* Set language-specific subdirectory in SWIG library */ 
    SWIG_library_directory("scilab");
   
    /* Add a symbol to the parser for conditional compilation */
    Preprocessor_define("SWIGSCILAB 1", 0);
    
    /* Set scilab configuration file */
    SWIG_config_file("scilab.swg");
    
    /* Set typemap for scilab */
    SWIG_typemap_lang("scilab");
  }

  /* ---------------------------------------------------------------------
   * top()
   * --------------------------------------------------------------------- */

  virtual int top(Node *n) {
    
    /* Get the name of the module */ 
    String *module = Getattr(n, "name");
    
    /* One output file for as the wrapper file */
    String *outfile = Getattr(n, "outfile");
    f_begin = NewFile(outfile, "w", SWIG_output_files());
    
    /* Initialize the output files */
    if (!f_begin) {
      FileErrorDisplay(outfile);
      SWIG_exit(EXIT_FAILURE);
     }
    f_runtime = NewString("");
    f_header = NewString("");
    f_wrappers = NewString("");
    f_init = NewString("");
    
    /* Register file targets with the SWIG file handler */
    Swig_register_filebyname("begin", f_begin);
    Swig_register_filebyname("runtime", f_runtime);
    Swig_register_filebyname("header", f_header);
    Swig_register_filebyname("wrapper", f_wrappers);
    Swig_register_filebyname("init", f_init);
     
    /* Insert the banner at the beginning */
    Swig_banner(f_begin);

    /* Include some header file of scilab */
    if (CPlusPlus)
      Printf(f_runtime, "extern \"C\" {\n");
    
    Printf(f_runtime, "#include \"stack-c.h\"\n");
    Printf(f_runtime, "#include \"sciprint.h\"\n");
    Printf(f_runtime, "#include \"Scierror.h\"\n");
    Printf(f_runtime, "#include \"api_scilab.h\"\n");
    Printf(f_runtime, "#include \"localization.h\"\n");
    
    if (CPlusPlus)
      Printf(f_runtime, "}\n");
   
    /* Initialize the builder.sce file code */
    Printf(f_builder_code, "ilib_name = \"%slib\";\n", module);
    Printf(f_builder_code, "files = [\"%s\",\"%s.o\"];\n", outfile, module);
    Printf(f_builder_code, "libs = [];\n");
    Printf(f_builder_code, "table = ["); 

    if (CPlusPlus)
      Printf(f_wrappers, "extern \"C\" {\n");
    
    /* Emit code for children */
    Language::top(n);

    if (CPlusPlus)
      Printf(f_wrappers, "}\n");
    
    /* Create the file to generate the module: "builder.sce" */
    if(hasfunction_flag) {
      Printf(f_builder_code, "];\n");
      Printf(f_builder_code, "ilib_build(ilib_name,table,files,libs);\n");
      Printf(f_builder_code, "exit");
      File *f_builder = NewFile(NewStringf("%sbuilder.sce", SWIG_output_directory()), "w", SWIG_output_files());
      Printv(f_builder, f_builder_code, NIL);
      Close(f_builder);
      Delete(f_builder);
      Delete(f_builder_code);
    }
    else {
      Delete(f_builder_code);
    }
    
    /* Dump out all the files */
    SwigType_emit_type_table(f_runtime, f_wrappers);
    Dump(f_runtime, f_begin);
    Dump(f_header, f_begin);
    Dump(f_wrappers, f_begin);
    Wrapper_pretty_print(f_init, f_begin);
    
    /* Close all of the files */
    Delete(f_init);
    Delete(f_wrappers);
    Delete(f_header);
    Delete(f_runtime);
    Close(f_begin);
    Delete(f_begin);
    
    return SWIG_OK;
  }

  /* ----------------------------------------------------------------------
   * functionWrapper()
   * ---------------------------------------------------------------------- */

  virtual int functionWrapper(Node *n) {
    
    hasfunction_flag = true;
   
    /* A new wrapper function object */
    Wrapper *f = NewWrapper();
    Parm *p;
    String *tm;
    int j;

    /* Determine whether the function is overloaded or not */
    bool overloaded = !!Getattr(n, "sym:overloaded");
    
    /* Determine whether the function is the last overloaded */   
    bool last_overload = overloaded && !Getattr(n, "sym:nextSibling");
    
    String *iname = Getattr(n, "sym:name");
    String *wname = Swig_name_wrapper(iname);
    String *overname = Copy(wname);
    SwigType *d = Getattr(n, "type");
    ParmList *l = Getattr(n, "parms");

    if (!overloaded && !addSymbol(iname, n))
      return SWIG_ERROR;

    if (overloaded)
      Append(overname, Getattr(n, "sym:overname"));

    Printv(f->def, "int ", overname, " (char *fname, unsigned long fname_len) {\n", NIL);
   
    /* Emit all of the local variables for holding arguments */
    emit_parameter_variables(l, f);
    
    /* Attach typemaps to the parameter list */
    emit_attach_parmmaps(l, f);
    Setattr(n, "wrap:parms", l);
     
    /* Get number of required and total arguments */
    int num_arguments = emit_num_arguments(l);
    int num_required = emit_num_required(l);
    
    /* The number of the output */
    int out_required = 0;
    
    /* Walk the function parameter list and generate code to get arguments */
    for (j = 0, p = l; j < num_arguments; ++j) {
      while (checkAttribute(p, "tmap:in:numinputs", "0")) {
	p = Getattr(p, "tmap:in:next");
      }

      SwigType *pt = Getattr(p, "type");

      /* Get typemap for this argument */
      String *tm = Getattr(p, "tmap:in");
       
      if (tm) {
	if (!tm || checkAttribute(p, "tmap:in:numinputs", "0")) {
	  p = nextSibling(p);
	  continue;
	}
      String *getargs = NewString("");
      
      /* The paremeter is variable */
      if (j >= num_required)
	Printf(getargs, "if (Rhs > %d) {\n%s\n}", j, tm);
      else
	Printv(getargs, tm, NIL);
      Printv(f->code, getargs, "\n", NIL);
      Delete(getargs);
      p = Getattr(p, "tmap:in:next");
      continue;
      } 
      else {
	Swig_warning(WARN_TYPEMAP_IN_UNDEF, input_file, line_number, "Unable to use type %s as a function argument.\n", SwigType_str(pt, 0));
	break;
      }
    }
    
    Setattr(n, "wrap:name", overname);   
   
    /* Now write code to make the function call */
    Swig_director_emit_dynamic_cast(n, f);
    String *actioncode = emit_action(n);
    
    /* Insert the return variable */ 
    emit_return_variable(n, d, f);

    if ((tm = Swig_typemap_lookup_out("out", n, "result", f, actioncode))) {
      Replaceall(tm, "$result", "result");
      
      /* There are more than one output */
      if (out_required > 0)
        Printf(f->code, "iOutNum ++;\niVarOut ++;\n");
      Printf(f->code, "%s\n", tm);
      if (strlen(Char(tm)) != 0)
        out_required ++;
    } 
    else {
    Swig_warning(WARN_TYPEMAP_OUT_UNDEF, input_file, line_number, "Unable to use return type %s in function %s.\n", SwigType_str(d, 0), iname);
    }
    
    /* Insert argument output code */
    String *outarg = NewString("");
    for (p = l; p;) {
      if ((tm = Getattr(p, "tmap:argout"))) {
	if (out_required > 0)
          Printf(f->code,"iOutNum ++;\niVarOut ++;\n");
        Printv(outarg, tm, "\n", NIL);
	p = Getattr(p, "tmap:argout:next");
        out_required ++;
      } else {
	p = nextSibling(p);
      }
    }
    Printv(f->code, outarg, NIL);

    /* Insert constraint checking code */
    for (p = l; p;) {
      if ((tm = Getattr(p, "tmap:check"))) {
	Printv(f->code, tm, "\n", NIL);
	p = Getattr(p, "tmap:check:next");
      } else {
	p = nextSibling(p);
      }
    }

    /* Insert cleanup code */
    String *cleanup = NewString("");
    for (p = l; p;) {
      if ((tm = Getattr(p, "tmap:freearg"))) {
	if (tm && (Len(tm) != 0)) {
	  Printv(cleanup, tm, "\n", NIL);
	}
	p = Getattr(p, "tmap:freearg:next");
      } else {
	p = nextSibling(p);
      }
    }
  
    /* Output cleanup code */
    Printv(f->code, cleanup, NIL);
    Delete(cleanup);

    /* Insert the code checking for the number of input and output */
    int flag;
    if (out_required == 0) {
      out_required = 1;
      flag = 0;
    }
    else {
      flag = 1;
    }
    
    /* Insert the code checking the number of input */
    Printf(f->def, "CheckRhs(%d, %d);\n",num_required,num_arguments);
    Printf(f->def, "CheckLhs(%d, %d);\n",out_required,out_required);
    Printf(f->def, "SciErr sciErr;\n");
   
    /* Insert the order of output parameters*/
    if (flag)
      Printf(f->def, "\nint iOutNum = 1;\nint iVarOut = Rhs + 1;");
   
    /* Finish the the code for the function  */
    if (flag)
      Printf(f->code, "LhsVar(iOutNum) = iVarOut;\n");
    Printf(f->code, "return 0;\n");	
    Printf(f->code, "}\n");

    Replaceall(f->code, "$symname", iname);
    
    /* Dump the wrapper function */
    Wrapper_print(f, f_wrappers);
    DelWrapper(f);
    
    if (last_overload)
      dispatchFunction(n);
    
    Printf(f_builder_code, "\"%s\",\"%s\";", iname, wname);

    Delete(overname);
    Delete(wname);
    Delete(outarg);
  
    return SWIG_OK;
  }
  
  /* -----------------------------------------------------------------------
   * dispatchFunctionWrapper()
   * ----------------------------------------------------------------------- */
  
  void dispatchFunction(Node *n) {
    Wrapper *f = NewWrapper();

    String *iname = Getattr(n, "sym:name");
    String *wname = Swig_name_wrapper(iname);
    int maxargs;
   
    /* Generate the dispatch function */
    String *dispatch = Swig_overload_dispatch(n, "return %s(fname, fname_len);", &maxargs);
    String *tmp = NewString("");

    Printv(f->def, "int ", wname, " (char *fname, unsigned long fname_len) {\n", NIL);
    
    /* Get the number of the parameters */
    Wrapper_add_local(f, "argc", "int argc = Rhs;");
    
    /* Dump the dispatch function */
    Printv(f->code, dispatch, "\n", NIL);
    Printf(f->code, "Scierror(999, _(\"No matching function for overload\"));\n");
    Printf(f->code, "LhsVar(iOutNum) = iVarOut;\n");
    Printf(f->code, "return 0;\n");
    Printv(f->code, "}\n", NIL);
    Wrapper_print(f, f_wrappers);
    
    Delete(tmp);
    DelWrapper(f);
    Delete(dispatch);
    Delete(wname);
  }
 
  /* -----------------------------------------------------------------------
   * variableWrapper()
   * ----------------------------------------------------------------------- */

  virtual int variableWrapper(Node *n) {
    
    hasfunction_flag = true;
    
    /* Get the useful information from the node */
    String *name = Getattr(n, "name");
    String *iname = Getattr(n, "sym:name");
    SwigType *t = Getattr(n, "type");
    
    if (!addSymbol(iname, n))
      return SWIG_ERROR;
    
    /* The rows and cols name of the variable */
    String *rowname = NewString("");
    String *colname = NewString("");
    String *iscomplexname = NewString("");
    Printf(rowname, "iRows_%s", iname);
    Printf(colname, "iCols_%s", iname);
    Printf(iscomplexname, "isComplex_%s", iname);
    
    /* Two wrapper function to get and set the variable */
    String *tm;
    String *globalVar = NewString("");
    Wrapper *getf = NewWrapper();
    Wrapper *setf = NewWrapper();

    String *getname = Swig_name_get(iname);
    String *setname = Swig_name_set(iname);
    
    Printf(globalVar, "int %s = 1;\n", rowname);
    Printf(globalVar, "int %s = 1;\n", colname);
    if(!Strcmp(t, "p.double"))
      Printf(globalVar, "int %s = 0;\n\n", iscomplexname);
    else
      Printf(globalVar, "\n");
    Printv(setf->def, "int ", setname, " (char *fname, unsigned long fname_len) {\n", NIL);
    
    /* Check the number of input and output */
    Printf(setf->def, "CheckRhs(1, 1);\n");
    Printf(setf->def, "CheckLhs(1, 1);\n");
    Printf(setf->def, "SciErr sciErr;\n");
    /* Add the local variable */
    Wrapper_add_local(setf, "piAddrVar", "int *piAddrVar");
   
    /* Deal with the set function */
    if (is_assignable(n)) {
      Setattr(n, "wrap:name", setname);
      if (Getattr(n, "unnamedinstance"))
        Setattr(n, "type", "int");
      if ((tm = Swig_typemap_lookup("varin", n, name, 0))) {
	Replaceall(tm, "$argnum", "1");
        Replaceall(tm, "iRows", rowname);
        Replaceall(tm, "iCols", colname);
        Replaceall(tm, "isComplex", iscomplexname);
	emit_action_code(n, setf->code, tm);
	Delete(tm);
      } else {
	Swig_warning(WARN_TYPEMAP_VARIN_UNDEF, input_file, line_number, "Unable to set variable of type %s.\n", SwigType_str(t, 0));
      }
    } 
    else {
      Append(setf->code, "SWIG_Error(999, \"attempt to set immutable member variable\");");
    }
    Append(setf->code, "}\n");
    Wrapper_print(setf, f_wrappers);
    Printf(f_builder_code, "\"%s\",\"%s\";", setname, setname);
    
    /* Deal with the get function */
    Setattr(n, "wrap:name", getname);
    int addfail = 0;
    Printv(getf->def, "int ", getname, " (char *fname, unsigned long fname_len){\n", NIL);
   
    /* Check the number of input and output */
    Printf(getf->def, "CheckRhs(0, 0);\n");
    Printf(getf->def, "CheckLhs(1, 1);\n");
    Printf(getf->def, "SciErr sciErr;\n"); 
    /* Insert the order of output parameters */
    Printf(getf->def, "\nint iOutNum=1;\nint iVarOut=Rhs+1;");
   
    if ((tm = Swig_typemap_lookup("varout", n, name, 0))) {
      Replaceall(tm, "$result", name);
      if (is_assignable(n)) {
        Replaceall(tm, "iRowsOut", rowname);
        Replaceall(tm, "iColsOut", colname);
      } else {
        Replaceall(tm, "iRowsOut", "1");
        Replaceall(tm, "iColsOut", "1");
      }
      Replaceall(tm, "isComplex", iscomplexname); 
      addfail = emit_action_code(n, getf->code, tm);
      Delete(tm);
    } 
    else {
      Swig_warning(WARN_TYPEMAP_VAROUT_UNDEF, input_file, line_number, "Unable to read variable of type %s\n", SwigType_str(t, 0));
    }
   
    /* Dump the wrapper function */ 
    Printf(getf->code, "LhsVar(iOutNum) = iVarOut;\n");
    Append(getf->code, "}\n");
    Wrapper_print(getf, f_wrappers);
    Printf(f_header,"%s", globalVar);
    Printf(f_builder_code, "\"%s\",\"%s\";", getname, getname);
    
    Delete(rowname);
    Delete(colname);
    Delete(iscomplexname);
    Delete(globalVar);
    DelWrapper(setf);
    DelWrapper(getf);
    
    return SWIG_OK;
  }

  /* -----------------------------------------------------------------------
   * constantWrapper()
   * ----------------------------------------------------------------------- */

  virtual int constantWrapper(Node *n) {
    
    hasfunction_flag = true;
    
    /* Get the useful information from the node */
    String *name = Getattr(n, "name");
    String *iname = Getattr(n, "sym:name");
    SwigType *type = Getattr(n, "type");
    String *rawval = Getattr(n, "rawval");
    String *value = rawval ? rawval : Getattr(n, "value");
    String *tm;
    
    if (!addSymbol(iname, n))
      return SWIG_ERROR;
    
    /* Use the get function to get the constant value */
    Wrapper *getf = NewWrapper();
    String *getname = Swig_name_get(iname);
    Setattr(n, "wrap:name", getname);
    int addfail = 0;
    Printv(getf->def, "int ", getname, " (char *fname, unsigned long fname_len){\n", NIL);
   
    /* Check the number of input and output */
    Printf(getf->def, "CheckRhs(0, 0);\n");
    Printf(getf->def, "CheckLhs(1, 1);\n");
    Printf(getf->def, "SciErr sciErr;\n"); 
    /* Insert the order of output parameters*/
    Printf(getf->def, "\nint iOutNum=1;\nint iVarOut=Rhs+1;");
   
    if ((tm = Swig_typemap_lookup("varout", n, name, 0))) {
      Replaceall(tm, "$result", value);
      Replaceall(tm, "iRowsOut", "1");
      Replaceall(tm, "iColsOut", "1");
      addfail = emit_action_code(n, getf->code, tm);
      Delete(tm);
    } else {
      Swig_warning(WARN_TYPEMAP_VAROUT_UNDEF, input_file, line_number, "Unable to read variable of type %s\n", SwigType_str(type, 0));
    }
   
    /* Dump the wrapper function */ 
    Printf(getf->code, "LhsVar(iOutNum) = iVarOut;\n");
    Append(getf->code, "}\n");
    Wrapper_print(getf, f_wrappers);
    Printf(f_builder_code, "\"%s\",\"%s\";", getname, getname);
    
    DelWrapper(getf);

    return SWIG_OK;
  }
  
  /* ---------------------------------------------------------------------
   * enumDeclaration()
   * --------------------------------------------------------------------- */
  
  virtual int enumDeclaration(Node *n) {
    return Language::enumDeclaration(n);
  }

  /* ---------------------------------------------------------------------
   * enumvalueDeclaration()
   * --------------------------------------------------------------------- */
  
  virtual int enumvalueDeclaration(Node *n) {
    return Language::enumvalueDeclaration(n);
  }
};

extern "C" Language *swig_scilab(void) {
  return new SCILAB();
}
