
%insert("header") {
void
SWIG_PHP_AppendOutput( zval **target, zval *o) {
  if ( (*target)->type == IS_ARRAY ) {
    /* it's already an array, just append */
    add_next_index_zval( *target, o );
    return;
  }
  if ( (*target)->type == IS_NULL ) {
    REPLACE_ZVAL_VALUE(target,o,1);
    return;
  }
  zval *tmp;
  ALLOC_INIT_ZVAL(tmp);
  *tmp = **target;
  zval_copy_ctor(tmp);
  array_init(*target);
  add_next_index_zval( *target, tmp);
  add_next_index_zval( *target, o);

}
}

%fragment("t_output_helper","header") {
#define t_output_helper SWIG_PHP_AppendOutput
}