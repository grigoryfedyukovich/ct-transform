#include "../ossl.h"
#include "../smack.h"
#include "../ct-verif.h"

int foo (unsigned int a, unsigned int b) {
  if (a > b) return a - b;
  return 8;
}

int fn(unsigned int s, unsigned int t) {
  s = foo (s, t);
  return 1;
}
