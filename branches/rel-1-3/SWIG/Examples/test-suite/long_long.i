/* This interface file tests whether SWIG handles the new ISO C
   long long types.
*/

%module long_long

%inline %{
void foo1(long long x) {}
void foo2(long long int x) {}
void foo3(signed long long int x) {}
void foo4(unsigned long long int x) {}
void foo5(signed long long x) {}
void foo6(unsigned long long x) {}

long long bar1() {return 0;}
long long int bar2() {return 0;}
signed long long int bar3() {return 0;}
unsigned long long int bar4() {return 0;}
signed long long bar5() {return 0;}
unsigned long long bar6() {return 0;}

long long ll;
unsigned long long ull;
%}

#ifdef SWIGJAVA
/* Prevent LL and ULL from appearing in Java code */
#define LL 
#define ULL
/* Can't initialise BigInteger with a number :( */
%ignore lconst2;
#endif

%constant long long  lconst1 = 1234567890LL;
%constant unsigned long long lconst2 = 1234567890ULL;

%constant lconst3 = 1234567LL;
%constant lconst4 = 1234567ULL;

#define lconst5 987654321LL
#define lconst6 987654321ULL
