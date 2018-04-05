#include "../ossl.h"
#include "../smack.h"
#include "../ct-verif.h"

int fn (unsigned int a, unsigned int b, unsigned int len, unsigned int d, int *N){
  public_in(__SMACK_value(a));
  public_in(__SMACK_value(len));
  public_in(__SMACK_value(d));

  for (a = 0; a < len; a++)
  {
    if (a == b) break;
    d = d + N[a];
  }
  return 0;
}

