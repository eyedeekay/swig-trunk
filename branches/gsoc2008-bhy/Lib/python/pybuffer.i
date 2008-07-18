/* Impelementing buffer protocol typemaps */

/* %pybuffer_mutable_binary(TYPEMAP, SIZE)
 *
 * Macro for functions accept mutable buffer pointer with a size.
 * This can be used for both input and output. For example:
 * 
 *      %pybuffer_mutable_size(char *buff, int size);
 *      void foo(char *buff, int size) {
 *        for(int i=0; i<size; ++i)
 *          buff[i]++;  
 *      }
 */

%define %pybuffer_mutable_binary(TYPEMAP, SIZE)
%typemap(in) (TYPEMAP, SIZE)
  (int res, Py_ssize_t size = 0, void *buf = 0) {
  res = PyObject_AsWriteBuffer($input, &buf, &size);
  if (res<0) {
    %argument_fail(res, "(TYPEMAP, SIZE)", $symname, $argnum);
  }
  $1 = ($1_ltype) buf;
  $2 = ($2_ltype) size;
}
%enddef

/* %pybuffer_mutable_string(TYPEMAP, SIZE)
 *
 * Macro for functions accept mutable zero terminated string pointer.
 * This can be used for both input and output. For example:
 * 
 *      %pybuffer_mutable(char *str);
 *      void foo(char *str) {
 *        while(*str) {
 *          *str = toupper(*str);
 *          str++;
 *      }
 */

%define %pybuffer_mutable_string(TYPEMAP)
%typemap(in) (TYPEMAP)
  (int res, Py_ssize_t size = 0, void *buf = 0) {
  res = PyObject_AsWriteBuffer($input, &buf, &size);
  if (res<0) {
    %argument_fail(res, "(TYPEMAP, SIZE)", $symname, $argnum);
  }
  $1 = ($1_ltype) buf;
}
%enddef


