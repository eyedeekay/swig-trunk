// -*- C++ -*-
// SWIG typemaps for std::string
// Art Yerkes
// Modified from: Luigi Ballabio
// Apr 8, 2002
//
// Ocaml implementation

// ------------------------------------------------------------------------
// std::string is typemapped by value
// This can prevent exporting methods which return a string
// in order for the user to modify it.
// However, I think I'll wait until someone asks for it...
// ------------------------------------------------------------------------

%include exception.i

%{
#include <string>
#include <vector>
%}

%include std_vector.i

namespace std {
    template <class charT> class basic_string {
    public:
	typedef charT *pointer;
	typedef charT &reference;
	typedef const charT &const_reference;
	typedef size_t size_type;
	typedef ptrdiff_t difference_type;
	basic_string();
	basic_string( charT *str );
	size_t size();
	charT operator []( int pos ) const;
	charT *c_str() const;
	basic_string<charT> &operator = ( const basic_string &ws );
	basic_string<charT> &operator = ( const charT *str );
	basic_string<charT> &append( const basic_string<charT> &other );
	basic_string<charT> &append( const charT *str );
	void push_back( charT c );
	void clear();
	void reserve( size_type t );
	void resize( size_type n, charT c = charT() );
	int compare( const basic_string<charT> &other ) const;
	int compare( const charT *str ) const;
	basic_string<charT> &insert( size_type pos, 
				     const basic_string<charT> &str );
	size_type find( const basic_string<charT> &other, int pos = 0 ) const;
	size_type find( charT c, int pos = 0 ) const;
	%extend {
	    bool operator == ( const basic_string<charT> &other ) const {
		return self->compare( other ) == 0;
	    }
	    bool operator != ( const basic_string<charT> &other ) const {
		return self->compare( other ) != 0;
	    }
	    bool operator < ( const basic_string<charT> &other ) const {
		return self->compare( other ) == -1;
	    }
	    bool operator > ( const basic_string<charT> &other ) const {
		return self->compare( other ) == 1;
	    }
	    bool operator <= ( const basic_string<charT> &other ) const {
		return self->compare( other ) != 1;
	    }
	    bool operator >= ( const basic_string<charT> &other ) const {
		return self->compare( other ) != -1;
	    }
	}
    };

    %template(string) basic_string<char>;
    %template(wstring) basic_string<wchar_t>;
    typedef basic_string<char> string;
    typedef basic_string<wchar_t> wstring;

    /* Overloading check */
    %typemap(in) string {
        if (caml_ptr_check($input))
            $1 = std::string((char *)caml_ptr_val($input,0));
        else
            SWIG_exception(SWIG_TypeError, "string expected");
    }

    %typemap(in) const string & (std::string temp) {
        if (caml_ptr_check($input)) {
            temp = std::string((char *)caml_ptr_val($input,0));
            $1 = &temp;
        } else {
            SWIG_exception(SWIG_TypeError, "string expected");
        }
    }

    %typemap(in) string & (std::string temp) {
        if (caml_ptr_check($input)) {
            temp = std::string((char *)caml_ptr_val($input,0));
            $1 = &temp;
        } else {
            SWIG_exception(SWIG_TypeError, "string expected");
        }
    }

    %typemap(argout) string & {
	caml_list_append(swig_result,caml_val_string_len($1->c_str(),
							 $1->size()));
    }

    %typemap(out) string {
        $result = caml_val_string_len($1.c_str(),$1.size());
    }

    %typemap(out) const string & {
        $result = caml_val_string_len($1.c_str(),$1.size());
    }
}

%template (StringVector) std::vector<string >;

char **c_charptr_array( const std::vector <string > &str_v );

%{
  SWIGEXT char **c_charptr_array( const std::vector <string > &str_v ) {
    char **out = new char *[str_v.size() + 1];
    out[str_v.size()] = 0;
    for( int i = 0; i < str_v.size(); i++ ) {
      out[i] = (char *)str_v[i].c_str();
    }
    return out;
  }
%}

%insert(ml) %{
  (* Some STL convenience items *)

  let string_array_to_vector sa = 
    let nv = _new_StringVector C_void in
      array_to_vector nv (fun x -> C_string x) sa ; nv
	
  let c_string_array ar = 
    _c_charptr_array (string_array_to_vector ar)
%}

%insert(mli) %{
  val c_string_array: string array -> c_obj
%}
