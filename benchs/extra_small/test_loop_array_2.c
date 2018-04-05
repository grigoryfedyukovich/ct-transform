#include "../ossl.h"
#include "../smack.h"
#include "../ct-verif.h"

int fn (unsigned int a, unsigned int b1, unsigned int b2, unsigned int len, unsigned int d, int *N, int *M){
  public_in(__SMACK_value(a));
  public_in(__SMACK_value(len));
  public_in(__SMACK_value(d));

  d = 1000;
  for (a = 0; a < len; a++)
  {
    if (a == b1) break;
    if (a == b2) d = N[a] + M[a];
  }
  return 0;
}

