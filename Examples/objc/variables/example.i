/* File : example.i */
%module example
%{
#include "example.h"
%}

/* Some global variable declarations */
%inline %{
extern int              ivar;
extern short            svar;
extern long             lvar;
extern unsigned int     uivar;
extern unsigned short   usvar;
extern unsigned long    ulvar;
extern signed char      scvar;
extern unsigned char    ucvar;
extern char             cvar;
extern float            fvar;
extern double           dvar;
extern char            *strvar;
extern int             *iptrvar;
extern char             name[256];

extern _Point           *ptptr;
extern _Point            pt;
%}


/* Some read-only variables */

%immutable;

%inline %{
extern int  status;
extern char path[256];
%}

%mutable;

/* Some helper functions to make it easier to test */
%inline %{
extern void  print_vars();
extern int  *new_int(int value);
extern _Point *new_Point(int x, int y);
extern char  *Point_print(_Point *p);
extern void  pt_print();
%}

