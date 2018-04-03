#include "../ossl.h"
#include "../smack.h"
#include "../ct-verif.h"

int fn (unsigned int a, unsigned int b1, unsigned int b2, unsigned int c){
  public_in(__SMACK_value(a));
  public_in(__SMACK_value(c));
  for (a = 0; a < c; a++)
  {
    if (a == b1) break;
    if (a == b2) break;
  }
  return 0;
}

