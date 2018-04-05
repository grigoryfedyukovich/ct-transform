#include "../ossl.h"
#include "../smack.h"
#include "../ct-verif.h"

int fn (unsigned int a, unsigned int b, unsigned int len, unsigned int* M, unsigned int* N){
  public_in(__SMACK_value(a));
  public_in(__SMACK_value(len));

  for (a = 0; a < len; a++)
  {
    M[a] = N[a];
    if (a == b) break;
  }
  return 0;
}

