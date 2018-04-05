#include "../ossl.h"
#include "../smack.h"
#include "../ct-verif.h"

int fn (unsigned int a, unsigned int b1, unsigned int b2, unsigned int len, unsigned int* N, int sum){
  public_in(__SMACK_value(a));
  public_in(__SMACK_value(len));

  sum = -1;
  for (a = 0; a < len; a++)
  {
    if (a == 0) N[a] = 100;
    else N[a] = N[a] - 1;

    if (N[a] >= b1) sum = sum + N[a];
    if (a > b2) break;
  }
  return 0;
}

