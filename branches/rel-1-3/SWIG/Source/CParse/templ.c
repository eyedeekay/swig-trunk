/* ----------------------------------------------------------------------------- 
 * templ.c
 *
 *     Expands a template into a specialized version.   
 * 
 * Author(s) : David Beazley (beazley@cs.uchicago.edu)
 *
 * Copyright (C) 1999-2000.  The University of Chicago
 * See the file LICENSE for information on usage and redistribution.	
 * ----------------------------------------------------------------------------- */

#include "swig.h"

extern void canonical_template(String *s);

static void add_parms(ParmList *p, List *patchlist, List *typelist) {
  while (p) {
    SwigType *ty = Getattr(p,"type");
    SwigType *val = Getattr(p,"value");
    Append(typelist,ty);
    Append(patchlist,val);
    p = nextSibling(p);
  }
}
/* -----------------------------------------------------------------------------
 * Swig_cparse_template_expand()
 *
 * Expands a template node into a specialized version.  This is done by
 * patching typenames and other aspects of the node according to a list of
 * template parameters
 * ----------------------------------------------------------------------------- */

static int
cparse_template_expand(Node *n, String *tname, String *rname, String *templateargs, List *patchlist, List *typelist, List *cpatchlist) {
  static int expanded = 0;
  int ret;

  if (!n) return 0;
  if (Getattr(n,"error")) return 0;

  if (Strcmp(nodeType(n),"template") == 0) {
    /* Change the node type back to normal */
    if (!expanded) {
      expanded = 1;
      set_nodeType(n,Getattr(n,"templatetype"));
      ret = cparse_template_expand(n,tname, rname, templateargs, patchlist,typelist, cpatchlist);
      expanded = 0;
      return ret;
    } else {
      set_nodeType(n,Getattr(n,"templatetype"));
      ret = cparse_template_expand(n,tname, rname, templateargs, patchlist,typelist, cpatchlist);
      set_nodeType(n,"template");
      return ret;
    }
  } else if (Strcmp(nodeType(n),"cdecl") == 0) {
    /* A simple C declaration */
    SwigType *t, *v, *d;
    String   *code;
    t = Getattr(n,"type");
    v = Getattr(n,"value");
    d = Getattr(n,"decl");

    code = Getattr(n,"code");
    
    Append(typelist,t);
    Append(typelist,d);
    Append(patchlist,v);
    Append(cpatchlist,code);
    
    add_parms(Getattr(n,"parms"), patchlist, typelist);
  } else if (Strcmp(nodeType(n),"class") == 0) {
    /* Patch base classes */
    {
      List *bases = Getattr(n,"baselist");
      if (bases) {
	int i;
	for (i = 0; i < Len(bases); i++) {
	  String *name = Copy(Getitem(bases,i));
	  Setitem(bases,i,name);
	  Append(typelist,name);
	}
      }
    }
    /* Patch children */
    {
      Node *cn = firstChild(n);
      while (cn) {
	cparse_template_expand(cn,tname, rname, templateargs, patchlist,typelist,cpatchlist);
	cn = nextSibling(cn);
      }
    }
  } else if (Strcmp(nodeType(n),"constructor") == 0) {
    String *name = Getattr(n,"name");
    if (Strstr(name,"<")) {
      Append(patchlist,Getattr(n,"name"));
    } else {
      Append(name,templateargs);
    }
    name = Getattr(n,"sym:name");
    if (Strstr(name,"<")) {
      Setattr(n,"sym:name", Copy(tname));
    } else {
      Replace(name,tname,rname, DOH_REPLACE_ANY);
    }
    Append(cpatchlist,Getattr(n,"code"));
    Append(typelist, Getattr(n,"decl"));
    Setattr(n,"sym:name",name);
    add_parms(Getattr(n,"parms"), patchlist, typelist);
  } else if (Strcmp(nodeType(n),"destructor") == 0) {
    String *name = Getattr(n,"name");
    if (Strstr(name,"<")) {
      Append(patchlist,Getattr(n,"name"));
    } else {
      Append(name,templateargs);
    }
    name = Getattr(n,"sym:name");
    if (Strstr(name,"<")) {
      Setattr(n,"sym:name", Copy(tname));
    } else {
      Replace(name,tname,rname, DOH_REPLACE_ANY);
    }
    Setattr(n,"sym:name",name);
    Append(cpatchlist,Getattr(n,"code"));
  } else {
    /* Look for obvious parameters */
    Node *cn;
    Append(cpatchlist,Getattr(n,"code"));
    Append(typelist, Getattr(n,"type"));
    Append(typelist, Getattr(n,"decl"));
    add_parms(Getattr(n,"parms"), patchlist, typelist);
    add_parms(Getattr(n,"pattern"), patchlist, typelist);
    cn = firstChild(n);
    while (cn) {
      cparse_template_expand(cn,tname, rname, templateargs, patchlist, typelist, cpatchlist);
      cn = nextSibling(cn);
    }
  }
}

int
Swig_cparse_template_expand(Node *n, String *rname, ParmList *tparms) {
  List *patchlist, *cpatchlist, *typelist;
  String *templateargs;
  String *tname;
  String *iname;
  Parm *p;
  patchlist = NewList();
  cpatchlist = NewList();
  typelist = NewList();

#if 0
  templateargs = NewString("< ");
  p = tparms;
  while (p) {
    String *value = Getattr(p,"value");
    if (!value) value = SwigType_str(Getattr(p,"type"),0);
    Printf(templateargs,"%s", value);
    p = nextSibling(p);
    if (p) {
      Printf(templateargs,",");
    }
  }
  Printf(templateargs," >");
  canonical_template(templateargs);
#else
  {
    String *tmp = NewString("");
    SwigType_add_template(tmp,tparms);
    templateargs = Copy(tmp);
    Delete(tmp);
  }
#endif

  tname = Copy(Getattr(n,"name"));
  cparse_template_expand(n,tname, rname, templateargs, patchlist, typelist, cpatchlist);

  /* Set the name */
  {
    String *name = Getattr(n,"name");
    if (name) {
      Append(name,templateargs);
    }
    iname = name;
  }

  /* Patch all of the types */
  {
    Parm *tp = Getattr(n,"templateparms");
    Parm *p  = tparms;
    while (p) {
      String *name, *value, *tydef, *tmp, *tmpr;
      int     sz, i;

      name = Getattr(tp,"name");
      value = Getattr(p,"value");
      tydef = Getattr(p,"typedef");
      assert(name);
      if (!value) value = Getattr(p,"type");
      assert(value);
      
      /*      Printf(stdout,"value = %s\n", value);
      Printf(stdout,"tydef = %s\n", tydef);
      Printf(stdout,"name  = %s\n", name); */

      sz = Len(patchlist);
      for (i = 0; i < sz; i++) {
	String *s = Getitem(patchlist,i);
	Replace(s,name,value, DOH_REPLACE_ID);
      }
      sz = Len(typelist);
      for (i = 0; i < sz; i++) {
	String *s = Getitem(typelist,i);
	Replace(s,name,value, DOH_REPLACE_ID);
	SwigType_typename_replace(s,tname,iname);
	/*	canonical_template(s); */
      }
      if (!tydef) {
	tydef = value;
      }
      tmp = NewStringf("#%s",name);
      tmpr = NewStringf("\"%s\"", value);

      sz = Len(cpatchlist);
      for (i = 0; i < sz; i++) {
	String *s = Getitem(cpatchlist,i);
	Replace(s,tmp,tmpr, DOH_REPLACE_ID);
	Replace(s,name,tydef, DOH_REPLACE_ID);
      }
      Delete(tmp);
      Delete(tmpr);

      p = nextSibling(p);
      tp = nextSibling(tp);
      if (!p) p = tp;
    }
  }
  Delete(patchlist);
  Delete(cpatchlist);
  Delete(typelist);
  /*  set_nodeType(n,"template");*/
  return 0;
}




