
#include "example.h"

int get_Astruct_a( struct Astruct *p ) {
  return p->a;
}

int get_Astruct_b( struct Astruct *p ) {
  return p->b;
}

int Astruct::get_b() {
  return this->b;
}
