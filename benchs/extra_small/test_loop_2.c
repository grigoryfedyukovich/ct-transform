#include "../ossl.h"
#include "../smack.h"
#include "../ct-verif.h"

int fn (unsigned int a, unsigned int b, unsigned int c){
  public_in(__SMACK_value(a));
  public_in(__SMACK_value(c));
  for (a = 0; a < c; a++)
  {
    if (a == b) break;
  } 
  return 0;
}
