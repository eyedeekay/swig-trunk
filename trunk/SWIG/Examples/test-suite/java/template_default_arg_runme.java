

import template_default_arg.*;

public class template_default_arg_runme {

  static {
    try {
	System.loadLibrary("template_default_arg");
    } catch (UnsatisfiedLinkError e) {
      System.err.println("Native code library failed to load. See the chapter on Dynamic Linking Problems in the SWIG Java documentation for help.\n" + e);
      System.exit(1);
    }
  }

  public static void main(String argv[]) {
    {
      Hello_int helloInt = new Hello_int();
      helloInt.foo(Hello_int.Hi.hi);
    }
    {
      X_int x = new X_int();
      if (x.meth(20.0, 200) != 200)
        throw new RuntimeException("X_int test 1 failed");
      if (x.meth(20) != 20)
        throw new RuntimeException("X_int test 2 failed");
      if (x.meth() != 0)
        throw new RuntimeException("X_int test 3 failed");
    }

    {
      Y_unsigned y = new Y_unsigned();
      if (y.meth(20.0, 200) != 200)
        throw new RuntimeException("Y_unsigned test 1 failed");
      if (y.meth(20) != 20)
        throw new RuntimeException("Y_unsigned test 2 failed");
      if (y.meth() != 0)
        throw new RuntimeException("Y_unsigned test 3 failed");
    }

    {
      X_longlong x = new X_longlong();
      x = new X_longlong(20.0);
      x = new X_longlong(20.0, 200L);
    }
    {
      X_int x = new X_int();
      x = new X_int(20.0);
      x = new X_int(20.0, 200);
    }
    {
      X_hello_unsigned x = new X_hello_unsigned();
      x = new X_hello_unsigned(20.0);
      x = new X_hello_unsigned(20.0, new Hello_int());
    }
    {
      Y_hello_unsigned y = new Y_hello_unsigned();
      y.meth(20.0, new Hello_int());
      y.meth(new Hello_int());
      y.meth();
    }
  }
}

