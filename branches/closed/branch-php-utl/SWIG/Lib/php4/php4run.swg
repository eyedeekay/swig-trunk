/* -----------------------------------------------------------------------------
 * See the LICENSE file for information on copyright, usage and redistribution
 * of SWIG, and the README file for authors - http://www.swig.org/release.html.
 *
 * php4run.swg
 *
 * PHP4 runtime library
 * ----------------------------------------------------------------------------- */


#ifdef __cplusplus
extern "C" {
#endif
#include "zend.h"
#include "zend_API.h"
#include "php.h"

/* These TSRMLS_ stuff should already be defined now, but with older php under
   redhat are not... */
#ifndef TSRMLS_D
#define TSRMLS_D
#endif
#ifndef TSRMLS_DC
#define TSRMLS_DC
#endif
#ifndef TSRMLS_C
#define TSRMLS_C
#endif
#ifndef TSRMLS_CC
#define TSRMLS_CC
#endif

#ifdef __cplusplus
}
#endif

/* forward decl */
extern zend_module_entry *SWIG_module_entry;

/*
 * Error macros and variables
 */
#define SWIG_fail goto fail

static char *default_error_msg = "Unknown error occurred";
static int default_error_code = E_ERROR;

#define SWIG_PHP_Arg_Error_Msg(argnum,extramsg) "Error in argument " #argnum " "#extramsg

#define SWIG_PHP_Error(code,msg) do { SWIG_ErrorCode() = code; SWIG_ErrorMsg() = msg; SWIG_fail; } while(0)

#define SWIG_Error(code,msg) SWIG_PHP_Error(code,msg)

/* Standard SWIG API */
#define SWIG_GetModule(clientdata) SWIG_Php4_GetModule()
#define SWIG_SetModule(clientdata, pointer) SWIG_Php4_SetModule(pointer)

/* But in fact SWIG_ConvertPtr is the native interface for getting typed
   pointer values out of zvals.  We need the TSRMLS_ macros for when we
   make PHP type calls later as we handle php resources */
#define SWIG_ConvertPtr(obj,pp,type,flags) SWIG_ZTS_ConvertPtr(obj,pp,type,flags TSRMLS_CC)

/* used to wrap returned objects in so we know whether they are newobject
   and need freeing, or not */
typedef struct _swig_object_wrapper {
  void * ptr;
  int newobject;
} swig_object_wrapper;

/* empty zend destructor for types without one */
static ZEND_RSRC_DTOR_FUNC(SWIG_landfill) {};

#define SWIG_NewPointerObj(a,b,c) SWIG_PHP_NewPointerObj(a,b,c)

#define SWIG_SetPointerZval(a,b,c,d) SWIG_ZTS_SetPointerZval(a,b,c,d, SWIG_module_entry TSRMLS_CC)

#define SWIG_ConvertFunctionPtr(obj,pptr,type) SWIG_ConvertPtr(obj,pptr,type,0)

static int
SWIG_ZTS_SetPointerZval(zval *z, void *ptr, swig_type_info *type, int newobject, zend_module_entry* module_entry TSRMLS_DC) {
  swig_object_wrapper *value=NULL;
  /*
   * First test for Null pointers.  Return those as PHP native NULL
   */
  if (!ptr ) {
    ZVAL_NULL(z);
    return SWIG_OK;
  }
  if (! (*(int *)(type->clientdata)))
    zend_error(E_ERROR, "Type: %s failed to register with zend",type->name);
  value=(swig_object_wrapper *)emalloc(sizeof(swig_object_wrapper));
  value->ptr=ptr;
  value->newobject=newobject;
  ZEND_REGISTER_RESOURCE(z, value, *(int *)(type->clientdata));
  return SWIG_OK;
}

/* This is a new pointer conversion routine
   Taking the native pointer p (which would have been converted from the old
   string pointer) and its php type id, and it's type name (which also would
   have come from the old string pointer) it converts it to ptr calling 
   appropriate casting functions according to ty
   Sadly PHP has no API to find a type name from a type id, only from an instance
   of a resource of the type id, so we have to pass type_name as well.
   The two functions which might call this are:
   SWIG_ZTS_ConvertResourcePtr which gets the type name from the resource
   and the registered zend destructors for which we have one per type each
   with the type name hard wired in. */
static int
SWIG_ZTS_ConvertResourceData(void * p, const char *type_name, void **ptr, swig_type_info *ty TSRMLS_DC) {
  swig_cast_info *tc;

  if (ty) {
    if (! type_name) {  
      /* can't convert p to ptr type ty if we don't know what type p is */
      return SWIG_TypeError;
    } else {
      /* convert and cast p from type_name to ptr as ty */
      tc = SWIG_TypeCheck(type_name,ty);
      if (!tc) return SWIG_TypeError;
      if (ptr) *ptr = SWIG_TypeCast(tc, (void*)p);
    }
  } else {
    /* They don't care about the target type, so just pass on the pointer! */
    if (ptr) *ptr = (void *) p;
  }
  return SWIG_OK;
}

/* This function fills ptr with a pointer of type ty by extracting the pointer
   and type info from the resource in z.  z must be a resource
   It uses SWIG_ZTS_ConvertResourceData to do the real work. */
static int
SWIG_ZTS_ConvertResourcePtr(zval **z, void **ptr, swig_type_info *ty, int flags TSRMLS_DC) {
  swig_object_wrapper *value;
  void *p;
  int type;
  char *type_name;

  value = (swig_object_wrapper *) zend_list_find((*z)->value.lval,&type);
  if ( flags && SWIG_POINTER_DISOWN ) {
    value->newobject = 0;
  }
  p = value->ptr;
  if (type==-1) return SWIG_TypeError;

  type_name=zend_rsrc_list_get_rsrc_type((*z)->value.lval TSRMLS_CC);

  return SWIG_ZTS_ConvertResourceData(p,type_name,ptr,ty TSRMLS_CC);
}

/* We allow passing of a RESOURCE pointing to the object
   or an OBJECT whose _cPtr is a resource pointing to the object
*/
static int
SWIG_ZTS_ConvertPtr(zval **z, void **ptr, swig_type_info *ty, int flags TSRMLS_DC) {
   zval *val;
   
   if(*z == NULL) {
     *ptr = 0;
     return SWIG_OK;
   }

   switch ( Z_TYPE_PP(z) ) {
   case IS_NULL:
     *ptr = 0;
     return SWIG_OK;
     break;
   case IS_RESOURCE:
     return SWIG_ZTS_ConvertResourcePtr(z,ptr,ty, flags TSRMLS_CC);
     break;
   case IS_OBJECT: {
     zval **_cPtr;
     if (zend_hash_find(HASH_OF(*z),"_cPtr",sizeof("_cPtr"),(void**)&_cPtr)==SUCCESS) {
       if ((*_cPtr)->type==IS_RESOURCE) {
         return SWIG_ZTS_ConvertResourcePtr(_cPtr,ptr,ty, flags TSRMLS_CC);
       }
     }
     break;
     }
   }

   return SWIG_TypeError;
}

SWIGRUNTIME zval **
SWIG_PHP_NewPointerObj( void *ptr, swig_type_info *type, int flags ) {
 zval *z;
 MAKE_STD_ZVAL(z);
 SWIG_SetPointerZval(z,ptr,type,flags);
 return (zval**)z;
}

static char const_name[] = "swig_runtime_data_type_pointer";
static swig_module_info *SWIG_Php4_GetModule() {
  zval *pointer;
  swig_module_info *ret = 0;

  MAKE_STD_ZVAL(pointer);

  TSRMLS_FETCH();

  if (zend_get_constant(const_name, sizeof(const_name), pointer TSRMLS_CC)) {
    if (pointer->type == IS_LONG) {
      ret = (swig_module_info *) pointer->value.lval;
    }
  } 
  FREE_ZVAL(pointer);
  return ret; 
}

static void SWIG_Php4_SetModule(swig_module_info *pointer) {
  TSRMLS_FETCH();
  REGISTER_MAIN_LONG_CONSTANT(const_name, (long) pointer, 0);
}
