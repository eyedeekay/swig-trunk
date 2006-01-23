/* File : example.h */

#include <string>
#ifndef SWIG
struct A {
};
#endif

class Exc {
public:
  Exc(int c, const char *m) {
    code = c;
    strncpy(msg,m,256);
  }
  int code;
  char msg[256];
};

class Test {
public:
  int simple() throw(int) {
      throw(37);
      return 1;
  }
  int message() throw(const char *) {
      throw("I died.");
      return 1;
  }
  int hosed() throw(Exc) {
      throw(Exc(42,"Hosed"));
      return 1;
  } 
  int unknown() throw(A*) {
      static A a;
      throw &a;
      return 1;
  }
  int multi(int x) throw(int, const char *, Exc) {
     if (x == 1) throw(37);
     if (x == 2) throw("Bleah!");
     if (x == 3) throw(Exc(42,"No-go-diggy-die"));
     return 1;
  }
};





  
