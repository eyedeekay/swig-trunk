// This file tests SWIG pass/return by value for
// a class with no default constructor

%module cpp_nodefault

%inline %{

class Foo {
public:
   int a;
   Foo(int x, int y) { }
  ~Foo() {
      printf("Destroying foo\n");
   }
};

Foo create(int x, int y) {
    return Foo(x,y);
}

typedef Foo Foo_t;

void consume(Foo f, Foo_t g) {
}


%}

%{
Foo gvar = Foo(3,4);
%}

Foo gvar;
