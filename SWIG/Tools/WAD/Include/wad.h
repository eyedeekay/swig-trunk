/* ----------------------------------------------------------------------------- 
 * wad.h
 *
 *     WAD header file (obviously)
 *    
 * Author(s) : David Beazley (beazley@cs.uchicago.edu)
 *
 * Copyright (C) 2000.  The University of Chicago
 * See the file LICENSE for information on usage and redistribution.	
 * ----------------------------------------------------------------------------- */

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/mman.h>
#include <ctype.h>
#include <sys/ucontext.h>

#ifdef WAD_SOLARIS
#include <procfs.h>

#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MAX_PATH
#define MAX_PATH 1024
#endif

/* --- Low level memory management --- */

extern int   wad_memory_init();
extern void *wad_malloc(int nbytes);
extern void *wad_pmalloc(int nbytes);
extern void  wad_release_memory();
extern char *wad_strdup(const char *c);
extern char *wad_pstrdup(const char *c);

/* --- I/O, Debugging --- */

extern void wad_printf(const char *fmt, ...);

/* --- Memory segments --- */
typedef struct WadSegment {
  char              *base;                  /* Base address for symbol lookup */
  char              *vaddr;                 /* Virtual address start          */
  unsigned long      size;                  /* Size of the segment (bytes)    */
  unsigned long      offset;                /* Offset into mapped object      */
  char              *mapname;               /* Filename mapped to this region */
  char              *mappath;               /* Full path to mapname           */
  struct WadSegment *next;                  /* Next segment                   */
} WadSegment;

extern int         wad_segment_read();
extern WadSegment *wad_segment_find(void *vaddr);

/* --- Object file handling --- */
typedef struct WadObjectFile {
  void             *ptr;            /* Pointer to data           */
  int               len;            /* Length of data            */
  int               type;           /* Type of the object file   */
  char             *path;           /* Path name of this object  */
  struct WadObjectFile *next;
} WadObjectFile;

extern void            wad_object_reset();
extern WadObjectFile  *wad_object_load(const char *path);

/* --- Symbol table information --- */
  
typedef struct WadSymbol {
  char *name;               /* Symbol table name    */
  char *file;               /* Source file (if any) */
  int   type;               /* Symbol type          */
  int   bind;               /* Symbol bind          */
  unsigned long value;      /* Symbol value         */
} WadSymbol;

#define WS_LOCAL     1
#define WS_GLOBAL    2

extern char *wad_find_symbol(WadObjectFile *wo, void *ptr, unsigned base, WadSymbol *ws);

/* Signal handling */
extern void wad_init();
extern void wad_signalhandler(int, siginfo_t *, void *);
extern void wad_signal_init();
extern void wad_set_return(const char *name, long value);
extern void wad_set_return_value(long value);
extern void wad_set_return_func(void (*f)(void));

extern int  wad_elf_check(WadObjectFile *wo);
extern void wad_elf_debug(WadObjectFile *wo);

typedef struct WadLocal {
  char              *name;       /* Name of the local */
  int                loc;        /* Location: register or stack */
  int                type;       /* Argument type               */
  unsigned long      position;   /* Position on the stack       */
  struct WadLocal   *next;
} WadLocal;

/* Debugging information */
typedef struct WadDebug {
  int         found;               /* Whether or not debugging information was found */
  char        srcfile[1024];       /* Source file */
  char        objfile[1024];       /* Object file */
  int         line_number;         /* Line number */
  int         nargs;               /* Number of function arguments */
  WadLocal   *args;                /* Arguments */
  WadLocal   *lastarg;
} WadDebug;

#define PARM_REGISTER 1
#define PARM_STACK    2

extern int wad_search_stab(void *stab, int size, char *stabstr, WadSymbol *symbol, unsigned long offset, WadDebug *wd);
extern int wad_debug_info(WadObjectFile *wo, WadSymbol *wsym, unsigned long offset, WadDebug *wd);

/* Data structure containing information about each stack frame */

typedef struct WadFrame {
  long             frameno;       /* Frame number */
  long             pc;            /* Real PC */
  long             sp;            /* Real SP */
  long             fp;            /* Real FP */
  char            *psp;           /* Pointer to where the actual stack data is stored */
  int              stack_size;    /* Stack frame size */
  char            *symbol;        /* Symbol name */
  long             sym_base;      /* Symbol base address        */
  char            *objfile;       /* Object file */
  char            *srcfile;       /* Source file */
  int              line_number;   /* Source line */
  int              nargs;         /* Number of arguments */
  WadLocal        *args;          /* Function arguments */
  WadLocal        *lastarg;       /* Last argument */
  int              nlocals;       /* Number of locals */
  int              last;          /* Last frame flag */
  struct WadFrame *next;  /* Next frame up the stack */
  struct WadFrame *prev;  /* Previous frame down the stack */
} WadFrame;

extern WadFrame *wad_stack_trace(unsigned long, unsigned long, unsigned long);
extern char *wad_strip_dir(char *);
extern void  wad_default_callback(int signo, WadFrame *frame, char *ret);
extern void wad_set_callback(void (*h)(int, WadFrame *, char *));
extern char *wad_load_source(char *, int line);
extern void wad_release_source();
extern void wad_release_trace();
extern long wad_steal_arg(WadFrame *f, char *symbol, int argno, int *error);
extern long wad_steal_outarg(WadFrame *f, char *symbol, int argno, int *error);

extern char *wad_arg_string(WadFrame *f);

typedef struct {
  char        name[128];
  long        value;
} WadReturnFunc;

extern void wad_set_returns(WadReturnFunc *rf);
extern WadReturnFunc *wad_check_return(const char *name);

/* --- Debugging Interface --- */

#define DEBUG_SEGMENT        0x1
#define DEBUG_SYMBOL         0x2
#define DEBUG_STABS          0x4
#define DEBUG_OBJECT         0x8
#define DEBUG_FILE           0x10
#define DEBUG_HOLD           0x20
#define DEBUG_RETURN         0x40
#define DEBUG_SYMBOL_SEARCH  0x80
#define DEBUG_INIT           0x100
#define DEBUG_NOSTACK        0x200
#define DEBUG_ONESHOT        0x400
#define DEBUG_STACK          0x800
#define DEBUG_UNWIND         0x1000
#define DEBUG_SIGNAL         0x2000

extern int wad_debug_mode;
extern int wad_heap_overflow;

#ifdef __cplusplus
}
#endif

