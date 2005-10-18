/*
  Typemaps for FILE*

  From the ideas of Luigi 
  luigi.ballabio@fastwebnet.it
*/

%types(FILE *);

/* defining basic methods */
%fragment("SWIG_AsValFilePtr","header") {
SWIGINTERN int
SWIG_AsValFilePtr(PyObject *obj, FILE **val) {
  static swig_type_info* desc = 0;
  FILE *ptr = 0;
  if (!desc) desc = SWIG_TypeQuery("FILE *");
  if ((SWIG_ConvertPtr(obj,(void **)(&ptr), desc, 0)) == SWIG_OK) {
    if (val) *val = ptr;
    return SWIG_OK;
  } 
  if (PyFile_Check(obj)) {
    if (val) *val =  PyFile_AsFile(obj);
    return SWIG_OK;
  }
  return SWIG_TypeError;
}
}


%fragment("SWIG_AsFilePtr","header",fragment="SWIG_AsValFilePtr") {
SWIGINTERNINLINE FILE*
SWIG_AsFilePtr(PyObject *obj) {
  FILE *val = 0;
  SWIG_AsValFilePtr(obj, &val);
  return val;
}
}

/* defining the typemaps */
%typemap_asval(SWIG_CCode(POINTER), SWIG_AsValFilePtr, "SWIG_AsValFilePtr", FILE*);
