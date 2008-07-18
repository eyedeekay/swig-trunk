%module pybuf

%include<pybuffer.i>
%include<cstring.i>
%pybuffer_mutable_string(char *str1);
%cstring_mutable(char *str2);

%inline %{
void upper1(char *str1) {
    while(*str1) {
        *str1 = toupper(*str1);
        str1++;
    }
}
void upper2(char *str2) {
    while(*str2) {
        *str2 = toupper(*str2);
        str2++;
    }
}
%}
