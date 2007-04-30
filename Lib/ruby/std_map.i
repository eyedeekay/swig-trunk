//
//   Maps
//
%fragment("StdMapTraits","header",fragment="StdSequenceTraits")
{
  namespace swig {
    template <class RubySeq, class K, class T >
    inline void
    assign(const RubySeq& rubyseq, std::map<K,T > *map) {
      typedef typename std::map<K,T>::value_type value_type;
      typename RubySeq::const_iterator it = rubyseq.begin();
      for (;it != rubyseq.end(); ++it) {
	map->insert(value_type(it->first, it->second));
      }
    }

    template <class K, class T>
    struct traits_asptr<std::map<K,T> >  {
      typedef std::map<K,T> map_type;
      static int asptr(VALUE obj, map_type **val) {
	int res = SWIG_ERROR;
	if ( TYPE(obj) == T_HASH ) {
	  static ID id_to_a = rb_intern("to_a");
	  VALUE items = rb_funcall(obj, id_to_a, 0);
	  res = traits_asptr_stdseq<std::map<K,T>, std::pair<K, T> >::asptr(items, val);
	} else {
	  map_type *p;
	  res = SWIG_ConvertPtr(obj,(void**)&p,swig::type_info<map_type>(),0);
	  if (SWIG_IsOK(res) && val)  *val = p;
	}
	return res;
      }
    };
      
    template <class K, class T >
    struct traits_from<std::map<K,T> >  {
      typedef std::map<K,T> map_type;
      typedef typename map_type::const_iterator const_iterator;
      typedef typename map_type::size_type size_type;
            
      static VALUE from(const map_type& map) {
	swig_type_info *desc = swig::type_info<map_type>();
	if (desc && desc->clientdata) {
	  return SWIG_NewPointerObj(new map_type(map), desc, SWIG_POINTER_OWN);
	} else {
	  size_type size = map.size();
	  int rubysize = (size <= (size_type) INT_MAX) ? (int) size : -1;
	  if (rubysize < 0) {
	    SWIG_RUBY_THREAD_BEGIN_BLOCK;
	    rb_raise( rb_eRuntimeError, "map size not valid in Ruby");
	    SWIG_RUBY_THREAD_END_BLOCK;
	    return Qnil;
	  }
	  VALUE obj = rb_hash_new();
	  for (const_iterator i= map.begin(); i!= map.end(); ++i) {
	    VALUE key = swig::from(i->first);
	    VALUE val = swig::from(i->second);
	    rb_hash_aset(obj, key, val);
	  }
	  return obj;
	}
      }
    };

    template <class ValueType>
    struct from_key_oper 
    {
      typedef const ValueType& argument_type;
      typedef  VALUE result_type;
      result_type operator()(argument_type v) const
      {
	return swig::from(v.first);
      }
    };

    template <class ValueType>
    struct from_value_oper 
    {
      typedef const ValueType& argument_type;
      typedef  VALUE result_type;
      result_type operator()(argument_type v) const
      {
	return swig::from(v.second);
      }
    };

    template<class OutIterator, class FromOper, class ValueType = typename OutIterator::value_type>
    struct RubyMapIterator_T : RubySwigIteratorClosed_T<OutIterator, ValueType, FromOper>
    {
      RubyMapIterator_T(OutIterator curr, OutIterator first, OutIterator last, VALUE seq)
	: RubySwigIteratorClosed_T<OutIterator,ValueType,FromOper>(curr, first, last, seq)
      {
      }
    };


    template<class OutIterator,
	     class FromOper = from_key_oper<typename OutIterator::value_type> >
    struct RubyMapKeyIterator_T : RubyMapIterator_T<OutIterator, FromOper>
    {
      RubyMapKeyIterator_T(OutIterator curr, OutIterator first, OutIterator last, VALUE seq)
	: RubyMapIterator_T<OutIterator, FromOper>(curr, first, last, seq)
      {
      }
    };

    template<typename OutIter>
    inline RubySwigIterator*
    make_output_key_iterator(const OutIter& current, const OutIter& begin, const OutIter& end, VALUE seq = 0)
    {
      return new RubyMapKeyIterator_T<OutIter>(current, begin, end, seq);
    }

    template<class OutIterator,
	     class FromOper = from_value_oper<typename OutIterator::value_type> >
    struct RubyMapValueIterator_T : RubyMapIterator_T<OutIterator, FromOper>
    {
      RubyMapValueIterator_T(OutIterator curr, OutIterator first, OutIterator last, VALUE seq)
	: RubyMapIterator_T<OutIterator, FromOper>(curr, first, last, seq)
      {
      }
    };
    

    template<typename OutIter>
    inline RubySwigIterator*
    make_output_value_iterator(const OutIter& current, const OutIter& begin, const OutIter& end, VALUE seq = 0)
    {
      return new RubyMapValueIterator_T<OutIter>(current, begin, end, seq);
    }
  }
}

%define %swig_map_common(Map...)
  %swig_sequence_iterator(Map);
  %swig_container_methods(Map)

  %extend {
    VALUE __getitem__(const key_type& key) const {
      Map::const_iterator i = self->find(key);
      if ( i != self->end() )
	return swig::from( i->second );
      else
	return Qnil;
    }
    
    VALUE __delitem__(const key_type& key) {
      Map::iterator i = self->find(key);
      if (i != self->end()) {
	self->erase(i);
	return swig::from( key );
      }
      else {
	return Qnil;
      }
    }
    
    %rename("has_key?") has_key;
    bool has_key(const key_type& key) const {
      Map::const_iterator i = self->find(key);
      return i != self->end();
    }
    
    VALUE keys() {
      Map::size_type size = self->size();
      int rubysize = (size <= (Map::size_type) INT_MAX) ? (int) size : -1;
      if (rubysize < 0) {
	SWIG_RUBY_THREAD_BEGIN_BLOCK;
	rb_raise(rb_eRuntimeError, "map size not valid in Ruby");
	SWIG_RUBY_THREAD_END_BLOCK;
	return Qnil;
      }
      VALUE ary = rb_ary_new2(rubysize);
      Map::const_iterator i = self->begin();
      Map::const_iterator e = self->end();
      for ( ; i != e; ++i ) {
	rb_ary_push( ary, swig::from(i->first) );
      }
      return ary;
    }
    
    VALUE values() {
      Map::size_type size = self->size();
      int rubysize = (size <= (Map::size_type) INT_MAX) ? (int) size : -1;
      if (rubysize < 0) {
	SWIG_RUBY_THREAD_BEGIN_BLOCK;
	rb_raise(rb_eRuntimeError, "map size not valid in Ruby");
	SWIG_RUBY_THREAD_END_BLOCK;
	return Qnil;
      }
      VALUE ary = rb_ary_new2(rubysize);
      Map::const_iterator i = self->begin();
      Map::const_iterator e = self->end();
      for ( ; i != e; ++i ) {
	rb_ary_push( ary, swig::from(i->second) );
      }
      return ary;
    }
    
    VALUE entries() {
      Map::size_type size = self->size();
      int rubysize = (size <= (Map::size_type) INT_MAX) ? (int) size : -1;
      if (rubysize < 0) {
	SWIG_RUBY_THREAD_BEGIN_BLOCK;
	rb_raise(rb_eRuntimeError, "map size not valid in Ruby");
	SWIG_RUBY_THREAD_END_BLOCK;
	return Qnil;
      }
      VALUE ary = rb_ary_new2(rubysize);
      Map::const_iterator i = self->begin();
      Map::const_iterator e = self->end();
      for ( ; i != e; ++i ) {
	rb_ary_push( ary, swig::from(*i) );
      }
      return ary;
    }
    
    %rename("include?") __contains__;
    bool __contains__(const key_type& key) {
      return self->find(key) != self->end();
    }

    %newobject key_iterator(VALUE *RUBY_SELF);
    swig::RubySwigIterator* key_iterator(VALUE *RUBY_SELF) {
      return swig::make_output_key_iterator(self->begin(), self->begin(), self->end(), *RUBY_SELF);
    }

    %newobject value_iterator(VALUE *RUBY_SELF);
    swig::RubySwigIterator* value_iterator(VALUE *RUBY_SELF) {
      return swig::make_output_value_iterator(self->begin(), self->begin(), self->end(), *RUBY_SELF);
    }

  }
%enddef

%define %swig_map_methods(Map...)
  %swig_map_common(Map)
  %extend {
    void __setitem__(const key_type& key, const mapped_type& x) throw (std::out_of_range) {
      (*self)[key] = x;
    }
  }
%enddef


%include <std/std_map.i>
