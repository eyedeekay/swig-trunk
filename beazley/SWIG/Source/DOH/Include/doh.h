/*******************************************************************************
 * DOH (Dynamic Object Hack)
 * 
 * Author : David Beazley
 *
 * Department of Computer Science        
 * University of Chicago
 * 1100 E 58th Street
 * Chicago, IL  60637
 * beazley@cs.uchicago.edu
 *
 * Please read the file LICENSE for the copyright and terms by which SWIG
 * can be used and distributed.
 *******************************************************************************/

/***********************************************************************
 * $Header$
 *
 * doh.h
 ***********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>

typedef void DOH;                

#define DOH_BEGIN    -1
#define DOH_END      -2
#define DOH_CUR      -3

#define DOH_MAGIC  0x04

/* -----------------------------------------------------------------------------
 * Object classes
 * ----------------------------------------------------------------------------- */

/* Mapping Objects */
typedef struct {
  DOH    *(*doh_getattr)(DOH *obj, DOH *name);              /* Get attribute */
  int     (*doh_setattr)(DOH *obj, DOH *name, DOH *value);  /* Set attribute */
  int     (*doh_delattr)(DOH *obj, DOH *name);              /* Del attribute */
  DOH    *(*doh_firstkey)(DOH *obj);                        /* First key     */
  DOH    *(*doh_nextkey)(DOH *obj);                         /* Next key      */
} DohMappingMethods;

/* Sequence methods */
typedef struct {
  DOH      *(*doh_getitem)(DOH *obj, int index);
  int       (*doh_setitem)(DOH *obj, int index, DOH *value);
  int       (*doh_delitem)(DOH *obj, int index);
  int       (*doh_insitem)(DOH *obj, int index, DOH *value);
  int       (*doh_insertf)(DOH *obj, int index, char *format, va_list ap);
  DOH      *(*doh_first)(DOH *obj);
  DOH      *(*doh_next)(DOH *obj);
} DohSequenceMethods;

/* File methods */
typedef struct {
  int       (*doh_read)(DOH *obj, void *buffer, int nbytes);
  int       (*doh_write)(DOH *obj, void *buffer, int nbytes);
  int       (*doh_seek)(DOH *obj, long offset, int whence);
  long      (*doh_tell)(DOH *obj);
  int       (*doh_printf)(DOH *obj, char *format, va_list ap);
  int       (*doh_scanf)(DOH *obj, char *format, va_list ap);
  int       (*doh_close)(DOH *obj);
} DohFileMethods;

/* -----------------------------------------------------------------------------
 * DohObjInfo
 * 
 * Included in all DOH types.  
 * ----------------------------------------------------------------------------- */

typedef struct DohObjInfo {
  char       *objname;                     /* Object name        */
  int         objsize;                     /* Object size        */

  /* Basic object methods */
  void      (*doh_del)(DOH *obj);          /* Delete object      */
  DOH      *(*doh_copy)(DOH *obj);         /* Copy and object    */
  void      (*doh_clear)(DOH *obj);        /* Clear an object    */

  /* Output methods */
  DOH       *(*doh_str)(DOH *obj);         /* Make a full string */
  void      *(*doh_data)(DOH *obj);        /* Return raw data    */

  /* Length and hash values */
  int       (*doh_len)(DOH *obj);          
  int       (*doh_hash)(DOH *obj); 

  /* Compare */ 
  int       (*doh_cmp)(DOH *obj1, DOH *obj2);

  DohMappingMethods  *doh_mapping;         /* Mapping methods    */
  DohSequenceMethods *doh_sequence;        /* Sequence methods   */
  DohFileMethods     *doh_file;            /* File methods       */
  void               *reserved2;           /* Number methods     */
  void               *reserved3;           
  void               *reserved4;
  void               *user[16];            /* User extensions    */
} DohObjInfo;

/* Low-level doh methods.  Do not call directly (well, unless you want to). */

extern void    DohDestroy(DOH *obj);
extern DOH    *DohCopy(DOH *obj);
extern void    DohClear(DOH *obj);
extern int     DohCmp(DOH *obj1, DOH *obj2);
extern DOH    *DohStr(DOH *obj);
extern DOH    *DohGetattr(DOH *obj, DOH *name);
extern int     DohSetattr(DOH *obj, DOH *name, DOH *value);
extern void    DohDelattr(DOH *obj, DOH *name);
extern int     DohHashval(DOH *obj);
extern DOH    *DohGetitem(DOH *obj, int index);
extern void    DohSetitem(DOH *obj, int index, DOH *value);
extern void    DohDelitem(DOH *obj, int index);
extern void    DohInsertitem(DOH *obj, int index, DOH *value);
extern void    DohInsertf(DOH *obj, int index, char *format, ...);
extern void    DohvInsertf(DOH *obj, int index, char *format, va_list ap);
extern void    DohAppendf(DOH *obj, char *format, ...);
extern void    DohvAppendf(DOH *obj, char *format, va_list ap);
extern int     DohLen(DOH *obj);
extern DOH    *DohFirst(DOH *obj);
extern DOH    *DohNext(DOH *obj);
extern DOH    *DohFirstkey(DOH *obj);
extern DOH    *DohNextkey(DOH *obj);
extern void   *DohData(DOH *obj);
extern int     DohGetline(DOH *obj);
extern void    DohSetline(DOH *obj, int line);
extern DOH    *DohGetfile(DOH *obj);
extern void    DohSetfile(DOH *obj, DOH *file);
extern int     DohCheck(DOH *obj);
extern void    DohInit(DOH *obj);

/* File methods */

extern int     DohWrite(DOH *obj, void *buffer, int length);
extern int     DohRead(DOH *obj, void *buffer, int length);
extern int     DohSeek(DOH *obj, long offser, int whence);
extern long    DohTell(DOH *obj);
extern int     DohPrintf(DOH *obj, char *format, ...);
extern int     DohvPrintf(DOH *obj, char *format, va_list ap);
extern int     DohScanf(DOH *obj, char *format, ...);
extern int     DohvScanf(DOH *obj, char *format, va_list ap);

/* Macros to invoke the above functions.  Includes the location of
   the caller to simplify debugging if something goes wrong */

#define Delete             DohDestroy
#define Copy               DohCopy
#define Clear              DohClear
#define Str                DohStr
#define Signature          DohSignature
#define Getattr            DohGetattr
#define Setattr            DohSetattr
#define Delattr            DohDelattr
#define Hashval            DohHashval
#define Getitem            DohGetitem
#define Setitem            DohSetitem
#define Delitem            DohDelitem
#define Insert             DohInsertitem
#define Insertf            DohInsertitemf
#define vInsertf           DohvInsertitemf
#define Append(s,x)        DohInsertitem(s,DOH_END,x)
#define Appendf            DohAppendf
#define vAppendf           DohvAppendf
#define Push(s,x)          DohInsertitem(s,DOH_BEGIN,x)
#define Len                DohLen
#define First              DohFirst
#define Next               DohNext
#define Firstkey           DohFirstkey
#define Nextkey            DohNextkey
#define Data               DohData
#define Char               (char *) Data
#define Cmp                DohCmp
#define Setline            DohSetline
#define Getline            DohGetline
#define Setfile            DohSetfile
#define Getfile            DohGetfile
#define Write              DohWrite
#define Read               DohRead
#define Seek               DohSeek
#define Tell               DohTell
#define Printf             DohPrintf
#define vPrintf            DohvPrintf
#define Scanf              DohScanf
#define vScanf             DohvScanf

/* -----------------------------------------------------------------------------
 * DohBase
 *
 * DohBase object type.  Attributes common to all objects.
 * ----------------------------------------------------------------------------- */

#define  DOHCOMMON      \
   char           magic; \
   char           moremagic[3]; \
   DohObjInfo    *objinfo; \
   int            refcount; \
   int            line; \
   DOH           *file 

typedef struct {
  DOHCOMMON;
} DohBase;

/* Macros for decrefing and increfing (safe for null objects). */
#define Decref(a)        if (a) ((DohBase *) a)->refcount--;
#define Incref(a)        if (a) ((DohBase *) a)->refcount++;
#define Objname(a)       ((DohBase *) a)->objinfo->objname

/* -----------------------------------------------------------------------------
 * Strings.   
 * ----------------------------------------------------------------------------- */

extern DOH   *NewString(char *c);
extern int    String_check(DOH *s);
extern void   String_replace(DOH *s, DOH *token, DOH *rep, int flags);
extern DohObjInfo *String_type();

/* String replacement flags */

#define   DOH_REPLACE_ANY         0x00
#define   DOH_REPLACE_NOQUOTE     0x01
#define   DOH_REPLACE_ID          0x02
#define   DOH_REPLACE_FIRST       0x04

/* -----------------------------------------------------------------------------
 * Files
 * ----------------------------------------------------------------------------- */

extern DOH *NewFile(char *file, char *mode);
extern DOH *NewFileFromFile(FILE *f);
extern DohObjInfo *File_type();
/* -----------------------------------------------------------------------------
 * List
 * ----------------------------------------------------------------------------- */

extern DOH  *NewList();
extern int  List_check(DOH *);
extern void List_sort(DOH *);
extern DohObjInfo *List_type();

/* -----------------------------------------------------------------------------
 * Hash
 * ----------------------------------------------------------------------------- */

extern DOH   *NewHash();
extern int    Hash_check(DOH *h);
extern DOH   *Hash_keys(DOH *);
extern DohObjInfo *Hash_type();






