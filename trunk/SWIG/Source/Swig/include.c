/* ----------------------------------------------------------------------------- 
 * include.c
 *
 *     The functions in this file are used to manage files in the SWIG library.
 *     General purpose functions for opening, including, and retrieving pathnames
 *     are provided.
 * 
 * Author(s) : David Beazley (beazley@cs.uchicago.edu)
 *
 * Copyright (C) 1999-2000.  The University of Chicago
 * See the file LICENSE for information on usage and redistribution.	
 * ----------------------------------------------------------------------------- */

static char cvsroot[] = "$Header$";

#include "swig.h"

/* Delimeter used in accessing files and directories */

static DOH           *directories = 0;        /* List of include directories */
static DOH           *lastpath = 0;           /* Last file that was included */
static int            bytes_read = 0;         /* Bytes read */

/* -----------------------------------------------------------------------------
 * Swig_add_directory()
 *
 * Adds a directory to the SWIG search path.
 * ----------------------------------------------------------------------------- */

void 
Swig_add_directory(DOH *dirname) {
  if (!directories) directories = NewList();
  assert(directories);
  if (!String_check(dirname)) {
    dirname = NewString((char *) dirname);
    assert(dirname);
  }
  Append(directories, dirname);
}

/* -----------------------------------------------------------------------------
 * Swig_last_file()
 * 
 * Returns the full pathname of the last file opened. 
 * ----------------------------------------------------------------------------- */

DOH *
Swig_last_file() {
  assert(lastpath);
  return lastpath;
}

/* -----------------------------------------------------------------------------
 * Swig_search_path() 
 * 
 * Returns a list of the current search paths.
 * ----------------------------------------------------------------------------- */

DOH *
Swig_search_path() {
  DOH *filename;
  DOH *dirname;
  DOH *slist;
  int i;

  slist = NewList();
  assert(slist);
  filename = NewString("");
  assert(filename);
  Printf(filename,".%s", SWIG_FILE_DELIMETER);
  Append(slist,filename);
  for (i = 0; i < Len(directories); i++) {
    dirname =  Getitem(directories,i);
    filename = NewString("");
    assert(filename);
    Printf(filename, "%s%s", dirname, SWIG_FILE_DELIMETER);
    Append(slist,filename);
  }
  return slist;
}  

/* -----------------------------------------------------------------------------
 * Swig_open()
 *
 * Looks for a file and open it.  Returns an open  FILE * on success.
 * ----------------------------------------------------------------------------- */

FILE *
Swig_open(DOH *name) {
  FILE    *f;
  DOH     *filename;
  DOH     *spath = 0;
  char    *cname;
  int     i;

  if (!directories) directories = NewList();
  assert(directories);

  cname = Char(name);
  filename = NewString(cname);
  assert(filename);
  f = fopen(Char(filename),"r");
  if (!f) {
      spath = Swig_search_path();
      for (i = 0; i < Len(spath); i++) {
	  Clear(filename);
	  Printf(filename,"%s%s", Getitem(spath,i), cname);
	  f = fopen(Char(filename),"r");
	  if (f) break;
      } 
      Delete(spath);
  }
  if (f) {
    Delete(lastpath);
    lastpath = Copy(filename);
  }
  Delete(filename);
  return f;
}

/* -----------------------------------------------------------------------------
 * Swig_read_file()
 * 
 * Reads data from an open FILE * and returns it as a string.
 * ----------------------------------------------------------------------------- */

DOH *
Swig_read_file(FILE *f) {
  char buffer[4096];
  DOH *str = NewString("");
  assert(str);
  while (fgets(buffer,4095,f)) {
    Append(str,buffer);
  }
  Append(str,"\n");
  return str;
}

/* -----------------------------------------------------------------------------
 * Swig_include()
 *
 * Opens a file and returns it as a string.
 * ----------------------------------------------------------------------------- */

DOH *
Swig_include(DOH *name) {
  FILE  *f;
  DOH    *str;
  f = Swig_open(name);
  if (!f) return 0;
  str = Swig_read_file(f);
  bytes_read = bytes_read + Len(str);
  fclose(f);
  Seek(str,0,SEEK_SET);
  Setfile(str,lastpath);
  Setline(str,1);
  return str;
}







