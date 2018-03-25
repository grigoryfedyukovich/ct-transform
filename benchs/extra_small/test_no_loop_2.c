#include "../ossl.h"
#include "../smack.h"
#include "../ct-verif.h"

int fn(unsigned int s, unsigned int t) {
  if (t > 0) {
    t--;
    if (t > 0) {
      s++;
    }
  }
  else {
    s++;
    if (s < 0) {
      s--;
    }
  }
  
  s = s + t;
  return 0;
}

