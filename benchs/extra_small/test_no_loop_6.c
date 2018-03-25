#include "../ossl.h"
#include "../smack.h"
#include "../ct-verif.h"

int foo (unsigned int a, unsigned int b) {
  if (a > b) return a - b;
  return 8;
}

int bar (unsigned int a, unsigned int b) {
  if (a == b) return -1;
  return a*b;
}

int fn(unsigned int s, unsigned int t, unsigned int u) {

  if (u == 0) s = foo (s, t);
  else s = bar (s, t);

  return 84;
}
