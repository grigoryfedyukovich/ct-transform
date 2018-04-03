#include "../ossl.h"
#include "../smack.h"
#include "../ct-verif.h"

int fn (unsigned int a, unsigned int b1, unsigned int b2, unsigned int c, unsigned int d){
  public_in(__SMACK_value(a));
  public_in(__SMACK_value(c));
  public_in(__SMACK_value(d));
  for (a = 0; a < c; a++)
  {
    d--;
    if (a == b1) break;
    if (a == b2) break;
  }
  return 0;
}

