%module(ruby_minherit="1") using_composition

%warnfilter(SWIGWARN_JAVA_MULTIPLE_INHERITANCE,
	    SWIGWARN_CSHARP_MULTIPLE_INHERITANCE,
	    SWIGWARN_PHP4_MULTIPLE_INHERITANCE) FooBar;   // C#, Java, Php4 multiple inheritance
#ifdef SWIGLUA	// lua only has one numeric type, so some overloads shadow each other creating warnings
%warnfilter(SWIGWARN_LANG_OVERLOAD_SHADOW) blah;
#endif

%inline %{
class Foo {
public:
     int blah(int x) { return x; }
     char *blah(char *x) { return x; }
};

class Bar {
public:
     double blah(double x) { return x; }
};

class FooBar : public Foo, public Bar {
public:
     using Foo::blah;
     using Bar::blah;
     char *blah(char *x) { return x; }
};

%}
