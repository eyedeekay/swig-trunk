#include <stdio.h>
#include <string.h>
#include "wad.h"
#include <assert.h>

int seg_crash(int n) {
  int *a = 0;
  if (n > 0) seg_crash(n-1);
  *a = 3;
  return 1;
}

int bus_crash(int n) {
  int b;
  int *a = &b;
  a = (int *) ((int) a | 0x1);
  if (n > 0) bus_crash(n-1);
  *a = 3;
  printf("well, well, well.\n");
  return 1;
}

int abort_crash(int n) {
  assert(n > 0);
  abort_crash(n-1);
  return 1;
}

double double_crash(double a, double b) {
  double *c;
  *c = a+b;
  return *c;
}

int math_crash(int x, int y) {
  return x/y;
}

int call_func(int n, int (*f)(int)) {

  int ret;
  ret = (*f)(n);
  if (ret <= 0) {
    printf("An error occurred!\n");
  }
  return 0;
}

static int multi(char a, short b, int c, double d) {
  a = 'x';
  b = 15236;
  c = 12345678;
  d = 3.14159;
  return c;
}

static int test(int x, int (*f)(int)) {
  return (*f)(-x);
}

int main(int argc, char **argv) {
  int n;
  int (*f)(int);

  printf("starting.\n");
  
  if (strcmp(argv[1],"abort") == 0) {
    abort_crash(0);
  } else if (strcmp(argv[1],"seg") ==0) {
    seg_crash(0);
  } else if (strcmp(argv[1],"bus") == 0) {
    bus_crash(0);
  } else if (strcmp(argv[1],"ret") == 0) {
    call_func(4,seg_crash);
  } else if (strcmp(argv[1],"test") == 0) {
    test(-1000,seg_crash);
  } else if (strcmp(argv[1],"double") == 0) {
    double_crash(3.14159,2.1828);
  } else if (strcmp(argv[1],"math") == 0) {
    math_crash(3,0);
  }
  multi(3,5,10,3.14);
}
