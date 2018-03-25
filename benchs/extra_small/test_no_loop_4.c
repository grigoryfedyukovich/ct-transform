#include "../ossl.h"
#include "../smack.h"
#include "../ct-verif.h"

int fn(unsigned int s, unsigned int t) {
  if (t == 0) {
    t = 1;
    return 0;
  }
  else {
    s++;
  }

  s = s * t + 3;
  return 1;
}
