smart_pointer_overload

f = Foo();
b = Bar(f);


if (f.test(3) != 1)
    error
endif
if (f.test(3.5) != 2)
    error
endif
if (f.test("hello") != 3)
    error
endif

if (b.test(3) != 1)
    error
endif
if (b.test(3.5) != 2)
    error
endif
if (b.test("hello") != 3)
    error
endif


