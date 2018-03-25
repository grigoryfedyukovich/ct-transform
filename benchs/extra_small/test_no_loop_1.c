#include "../ossl.h"
#include "../smack.h"
#include "../ct-verif.h"

int fn(unsigned int s, unsigned int t) {
  if (t < 0) {
    t = s + t - 1;
    s = t * 2;
  }
  else {
    s++;
    if (s > 0) {
      t--;
    }
  }

  s = s * t;
  return 0;
}
