#include "../ossl.h"
#include "../smack.h"
#include "../ct-verif.h"

int fn (unsigned int a, unsigned int b, unsigned int c){
  public_in(__SMACK_value(a));
  public_in(__SMACK_value(c));
  a = 0;
  while (a < c)
  {
    if (a == b) break;
    a++;
  }
  return 0;
}

