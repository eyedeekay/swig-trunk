#define CINTERFACE
#include <stdio.h>
#include <windows.h>
#include <initguid.h>
#include "virtual_poly.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
  CoInitialize(NULL);

  virtual_poly *vp;

  CoCreateInstance(&CLSID_virtual_polyImpl, NULL, CLSCTX_INPROC_SERVER, &IID_virtual_poly, (void **) &vp);

  NDouble *d = vp->lpVtbl->new_NDouble(vp, 3.5);
  NInt *i = vp->lpVtbl->new_NInt(vp, 2);

  /*
   * These two natural 'copy' forms fail because no covariant (polymorphic) return types 
   * are supported in C#. 
   *
   * NDouble dc = d.copy();
   * NInt ic = i.copy();
   *
   *
   * Unlike C++, we have to downcast instead.
   */

  NNumber *dc_as_nnumber = d->lpVtbl->copy(d);

#if 0
  // This should work when covariant return types are handled properly
  // Currently we are not able to cast dc_as_nnumber to IID_NDouble, etc.

  NDouble *dc = NULL;

  dc_as_nnumber->lpVtbl->QueryInterface(dc_as_nnumber, &IID_NDouble, (void **) &dc);
#endif

  NNumber *ic_as_nnumber = i->lpVtbl->copy(i);

#if 0
  // Same as above
  NInt *ic = NULL;

  ic_as_nnumber->lpVtbl->QueryInterface(ic_as_nnumber, &IID_NInt, (void **) &ic);

  NDouble *ddc = dc->lpVtbl->narrow(dc, dc_as_nnumber);

  NInt *dic = ic->lpVtbl->narrow(ic, dc_as_nnumber);
#endif


  // Static member calls; d and i are not really used here
  NDouble *ddc = d->lpVtbl->narrow(d, dc_as_nnumber);

  dc_as_nnumber->lpVtbl->Release(dc_as_nnumber);

  NInt *dic = i->lpVtbl->narrow(i, ic_as_nnumber);

  ic_as_nnumber->lpVtbl->Release(ic_as_nnumber);

  NInt *ic = dic;
  NDouble *dc = ddc;

  vp->lpVtbl->incr(vp, ic);

  if (i->lpVtbl->get(i) + 1 != ic->lpVtbl->get(ic)) {
    fprintf(stderr, "incr test failed\n");
    exit(1);
  }

  /*
   * Checking a pure user downcast
   */
  NNumber *n1 = d->lpVtbl->copy(d);
  NNumber *n2 = d->lpVtbl->nnumber(d);
  NDouble *dn1 = d->lpVtbl->narrow(d, n1);
  NDouble *dn2 = d->lpVtbl->narrow(d, n2);

  if (dn1->lpVtbl->get(dn1) != dn2->lpVtbl->get(dn2)) {
    fprintf(stderr, "copy/narrow test failed\n");
    exit(1);
  }

  n1->lpVtbl->Release(n1);
  n2->lpVtbl->Release(n2);
  dn1->lpVtbl->Release(dn1);
  dn2->lpVtbl->Release(dn2);

  /*
   * Checking the ref polymorphic case
   */
  NNumber *nr = d->lpVtbl->ref_this(d);
  NDouble *dr1 = d->lpVtbl->narrow(d, nr);

  /* FIXME: this should just need a QueryInterface, instead of 'narrow' */
  NNumber *dr2_as_nnumber = d->lpVtbl->ref_this(d);
  NDouble *dr2 = d->lpVtbl->narrow(d, dr2_as_nnumber);

  if (dr1->lpVtbl->get(dr1) != dr2->lpVtbl->get(dr2)) {
    fprintf(stderr, "copy/narrow test failed\n");
    exit(1);
  }

  nr->lpVtbl->Release(nr);
  dr1->lpVtbl->Release(dr1);
  dr2_as_nnumber->lpVtbl->Release(dr2_as_nnumber);
  dr2->lpVtbl->Release(dr2);

  i->lpVtbl->Release(i);
  d->lpVtbl->Release(d);

  vp->lpVtbl->Release(vp);

  return 0;
}
