%module(directors="1") director_enum

%warnfilter(801) EnumDirector::hi; /* Ruby, wrong constant name */
%warnfilter(801) EnumDirector::hello; /* Ruby, wrong constant name */
%warnfilter(801) EnumDirector::yo; /* Ruby, wrong constant name */
%warnfilter(801) EnumDirector::awright; /* Ruby, wrong constant name */


%feature("director") Foo;

%rename(Hallo) EnumDirector::Hello;

%inline %{
namespace EnumDirector {
  class A;

  enum Hello {
    hi, hello, yo, awright
  };

  class Foo {
  public:
    virtual ~Foo() {}
    virtual Hello say_hi(Hello h){ return h;}
    virtual Hello say_hello(Hello){ return hello;}
    virtual Hello say_hi(A *a){ return hi;}

    Hello ping(Hello h){ return say_hi(h);}
  };
}
%}

%feature("director");

%inline %{
namespace EnumDirector {
enum FType{ SA = -1, NA=0, EA=1};

struct A{
    A(const double a, const double b, const FType c)
    {}    

    virtual ~A() {}
    
    virtual int f(int i=0) {return i;}
};

struct B : public A{
    B(const double a, const double b, const FType c)
        : A(a, b, c) 
    {}    
};
}
%}


%inline %{
namespace EnumDirector {
struct A2{
    A2(const FType c = NA) {}    

    virtual ~A2() {}
    
    virtual int f(int i=0) {return i;}
};

struct B2 : public A2{
    B2(const FType c) : A2(c) {}
};
}
 
%}
