%fragment(SWIG_AsVal_frag(bool),"header") {
SWIGINTERN int
SWIG_AsVal_dec(bool)(zval **obj, bool* val)
{
  if ( Z_TYPE_PP(obj) == IS_BOOL ) {
    if (val) *val = Z_BVAL_PP(obj);
    return SWIG_OK;
  }

  if ( val ) {
    convert_to_boolean_ex( obj );
    *val = Z_BVAL_PP( obj );
  }

  return SWIG_AddCast(SWIG_OK);
}
}

%fragment(SWIG_From_frag(bool),"header") {
SWIGINTERNINLINE zval **
SWIG_From_dec(bool)( bool val )
{
  zval *o;
  MAKE_STD_ZVAL(o);
  ZVAL_BOOL(o,val);
  return %static_cast(o,zval **);
}
}

%fragment(SWIG_AsVal_frag(long),"header") {
SWIGINTERN int
SWIG_AsVal_dec(long)(zval **obj, long* val)
{
  if ( Z_TYPE_PP(obj) == IS_LONG ) {
    if (val) *val = Z_LVAL_PP(obj);
    return SWIG_OK;
  }

  if ( val ) {
    convert_to_long_ex( obj );
    *val = Z_LVAL_PP( obj );
  }

  return SWIG_AddCast(SWIG_OK);
}
}

%fragment(SWIG_From_frag(long),"header") {
SWIGINTERNINLINE zval **
SWIG_From_dec(long)( long val )
{
  zval *o;
  MAKE_STD_ZVAL(o);
  ZVAL_LONG(o,val);
  return %static_cast(o,zval **);
}
}

%fragment(SWIG_AsVal_frag(unsigned long),"header") {
SWIGINTERN int
SWIG_AsVal_dec(unsigned long)(zval **obj, unsigned long* val)
{
  if ( Z_TYPE_PP(obj) == IS_LONG ) {
    long phpval = Z_LVAL_PP(obj);
    if (phpval < 0 )
      return SWIG_TypeError;
    if (val) *val = (unsigned long) phpval;
    return SWIG_OK;
  }

  if ( val ) {
    convert_to_long_ex( obj );
    long phpval = Z_LVAL_PP(obj);
    if ( phpval < 0 )
      return SWIG_TypeError;
    *val = (unsigned long) phpval;
  }

  return SWIG_AddCast(SWIG_OK);
}
}

%fragment(SWIG_From_frag(unsigned long),"header") {
SWIGINTERNINLINE zval **
SWIG_From_dec(unsigned long)( unsigned long val )
{
  zval *o;
  MAKE_STD_ZVAL(o);
  ZVAL_LONG(o,val);
  return %static_cast(o,zval **);
}
}

%fragment(SWIG_AsVal_frag(long long),"header",fragment="<limits.h>") {
SWIGINTERN int
SWIG_AsVal_dec(long long)(zval **obj, long long* val)
{
  if ( Z_TYPE_PP(obj) == IS_LONG ) {
    long phpval = Z_LVAL_PP(obj);
    if (phpval > LONG_MAX )
      return SWIG_TypeError;
    if (val) *val = (long long) phpval;
    return SWIG_OK;
  }

  if ( val ) {
    convert_to_long_ex( obj );
    long phpval = Z_LVAL_PP(obj);
    if ( phpval > LONG_MAX )
      return SWIG_TypeError;
    *val = (long long) phpval;
  }

  return SWIG_AddCast(SWIG_OK);
}
}

%fragment(SWIG_From_frag(long long),"header") {
SWIGINTERNINLINE zval **
SWIG_From_dec(long long)( long long val )
{
  zval *o;
  MAKE_STD_ZVAL(o);
  ZVAL_LONG(o,val);
  return %static_cast(o,zval **);
}
}

%fragment(SWIG_AsVal_frag(double),"header") {
SWIGINTERN int
SWIG_AsVal_dec(double)(zval **obj, double* val)
{
  if ( Z_TYPE_PP(obj) == IS_DOUBLE ) {
    if (val) *val = Z_DVAL_PP( obj );
    return SWIG_OK;
  }

  if ( val ) {
    convert_to_double_ex( obj );
    if ( val ) *val = Z_DVAL_PP( obj );
  }
  return SWIG_AddCast(SWIG_OK);
}
}

%fragment(SWIG_From_frag(double),"header") {
SWIGINTERNINLINE zval **
SWIG_From_dec(double)( double val )
{
  zval *o;
  MAKE_STD_ZVAL(o);
  ZVAL_DOUBLE(o,val);
  return %static_cast(o,zval **);
}
}

