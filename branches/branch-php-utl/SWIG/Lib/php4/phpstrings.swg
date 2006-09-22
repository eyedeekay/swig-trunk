/* SWIG_AsCharPtrAndSize */
%fragment("SWIG_AsCharPtrAndSize","header") {
SWIGINTERN int
SWIG_AsCharPtrAndSize( zval** obj, char **cptr, size_t* psize, int *alloc )
{
  /* We're always going to return a pointer into internal data. */
  convert_to_string_ex( obj );
  if ( cptr ) *cptr = Z_STRVAL_PP( obj );
  if ( psize ) *psize = Z_STRLEN_PP( obj ) + 1;
  if ( alloc ) *alloc = SWIG_OLDOBJ;
  return SWIG_OK;
}
}

%fragment("SWIG_FromCharPtrAndSize","header") {
SWIGINTERNINLINE zval **
SWIG_FromCharPtrAndSize(const char *carray, size_t size )
{
  zval *o;
  MAKE_STD_ZVAL(o);
  ZVAL_STRINGL(o,(char*)carray,size,1);
  return %static_cast(o,zval**);
}
}
