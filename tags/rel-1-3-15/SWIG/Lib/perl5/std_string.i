//
// SWIG typemaps for std::string
// Luigi Ballabio
// May 7, 2002
//
// Perl implementation


// ------------------------------------------------------------------------
// std::string is typemapped by value
// This can prevent exporting methods which return a string
// in order for the user to modify it.
// However, I think I'll wait until someone asks for it...
// ------------------------------------------------------------------------

%include exception.i

%{
#include <string>
%}

namespace std {

    class string;

    /* Overloading check */
    %typemap(typecheck) string = char *;
    %typemap(typecheck) const string & = char *;

    %typemap(in) string {
        if (!SvOK((SV*) $input)) {
            $1 = std::string();
        } else {
            STRLEN len;
            char  *ptr = SvPV($input,len);
            $1 = std::string(ptr,len);
        }
    }

    %typemap(in) string *INPUT(std::string temp), const string & (std::string temp) {
        if (!SvOK((SV*) $input)) {
            temp = std::string();
        } else {
            STRLEN len;
            char *ptr = SvPV($input,len);
            temp.assign(ptr,len);
        }
        $1 = &temp;
    }

    %typemap(out) string {
        $result = sv_newmortal();
        sv_setpv((SV*)ST(argvi++), const_cast<char*>($1.c_str()));
    }

    %typemap(out) const string & {
        $result = sv_newmortal();
        sv_setpv((SV*)ST(argvi++), const_cast<char*>($1->c_str()));
    }

}

