#include "../ossl.h"
#include "../smack.h"
#include "../ct-verif.h"

int fn(unsigned int s, unsigned int t, unsigned int u) {
  if (s == t) u = s + 1;
  if (s == u) t = s + 1;

  s = s + t + u;
  return 0;
}

