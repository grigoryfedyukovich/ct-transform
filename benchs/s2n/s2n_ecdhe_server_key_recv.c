#include "../ossl.h"
#include "../smack.h"
#include "../ct-verif.h"
#include "s2n.h"

int fn(uint8_t hash_algorithm, int len, unsigned int i)
{
  public_in(__SMACK_value(i));
  public_in(__SMACK_value(len));
  unsigned int matched = 0;
  for (i = 0; i < len; i++) {
    if (s2n_preferred_hashes[i] == hash_algorithm) {
      matched = 1;
      break;
    }
  }
  
  if (0 == matched) {
    return -1;
  }
  return 0;
}
