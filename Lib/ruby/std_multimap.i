/*
  Multimaps
*/
%include <std_map.i>

%fragment("StdMultimapTraits","header",fragment="StdSequenceTraits")
{
  namespace swig {
    template <class RubySeq, class K, class T >
    inline void 
    assign(const RubySeq& rubyseq, std::multimap<K,T > *multimap) {
      typedef typename std::multimap<K,T>::value_type value_type;
      typename RubySeq::const_iterator it = rubyseq.begin();
      for (;it != rubyseq.end(); ++it) {
	multimap->insert(value_type(it->first, it->second));
      }
    }

    template <class K, class T>
    struct traits_asptr<std::multimap<K,T> >  {
      typedef std::multimap<K,T> multimap_type;
      static int asptr(VALUE obj, std::multimap<K,T> **val) {
	int res = SWIG_ERROR;
	if ( TYPE(obj) == T_HASH ) {
	  static ID id_to_a = rb_intern("to_a");
	  VALUE items = rb_funcall(obj, id_to_a, 0);
	  return traits_asptr_stdseq<std::multimap<K,T>, std::pair<K, T> >::asptr(items, val);
	} else {
	  multimap_type *p;
	  res = SWIG_ConvertPtr(obj,(void**)&p,swig::type_info<multimap_type>(),0);
	  if (SWIG_IsOK(res) && val)  *val = p;
	}
	return res;
      }
    };
      
    template <class K, class T >
    struct traits_from<std::multimap<K,T> >  {
      typedef std::multimap<K,T> multimap_type;
      typedef typename multimap_type::const_iterator const_iterator;
      typedef typename multimap_type::size_type size_type;
            
      static VALUE from(const multimap_type& multimap) {
	swig_type_info *desc = swig::type_info<multimap_type>();
	if (desc && desc->clientdata) {
	  return SWIG_NewPointerObj(new multimap_type(multimap), desc, SWIG_POINTER_OWN);
	} else {
	  size_type size = multimap.size();
	  int rubysize = (size <= (size_type) INT_MAX) ? (int) size : -1;
	  if (rubysize < 0) {
	    SWIG_RUBY_THREAD_BEGIN_BLOCK;
	    rb_raise(rb_eRuntimeError,
		     "multimap size not valid in Ruby");
	    SWIG_RUBY_THREAD_END_BLOCK;
	    return Qnil;
	  }
	  VALUE obj = rb_hash_new();
	  for (const_iterator i= multimap.begin(); i!= multimap.end(); ++i) {
	    VALUE key = swig::from(i->first);
	    VALUE val = swig::from(i->second);

	    VALUE oldval = rb_hash_aref( obj, key );
	    if ( oldval == Qnil )
	      rb_hash_aset(obj, key, val);
	    else {
	      // Multiple values for this key, create array if needed
	      // and add a new element to it.
	      VALUE ary;
	      if ( TYPE(oldval) == T_ARRAY )
		ary = oldval;
	      else
		{
		  ary = rb_ary_new2(2);
		  rb_ary_push( ary, oldval );
		  rb_hash_aset( obj, key, ary );
		}
	      rb_ary_push( ary, val );
	    }
	    
	  }
	  return obj;
	}
      }
    };
  }
}

%define %swig_multimap_methods(Type...) 
  %swig_map_common(Type);
  %extend {
    void __setitem__(const key_type& key, const mapped_type& x) throw (std::out_of_range) {
      self->insert(Type::value_type(key,x));
    }
  }
%enddef

%include <std/std_multimap.i>

