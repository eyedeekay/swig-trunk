// Tests typedef through member pointers

%module typedef_mptr

#ifdef SWIGPYTHON

%inline %{

class Foo {
public:
    int add(int x, int y) {
        return x+y;
    }
    int sub(int x, int y) {
        return x-y;
    }
    int do_op(int x, int y, int (Foo::*op)(int, int)) {
	return (this->*op)(x,y);
    }
};

typedef Foo FooObj;
typedef int Integer;

Integer do_op(Foo *f, Integer x, Integer y, Integer (FooObj::*op)(Integer, Integer)) {
    return f->do_op(x,y,op);
}
%}
#endif

#ifdef SWIGPYTHON
%constant int (Foo::*add)(int,int) = &Foo::add;
%constant Integer (FooObj::*sub)(Integer,Integer) = &FooObj::sub;
#endif
