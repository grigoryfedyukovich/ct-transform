#include "../ossl.h"
#include "../smack.h"
#include "../ct-verif.h"
#include "s2n.h"

int fn(struct s2n_cipher_preferences * prefs, unsigned int i, int num_available_suites, unsigned int pcount)
{
  public_in(__SMACK_value(pcount));
  public_in(__SMACK_value(i));
  prefs->count = (uint8_t) pcount;
  num_available_suites = 0;
  for (i = 0; i < prefs->count; i++) {
    if (prefs->suites[i]->available) {
      num_available_suites++;
    }
  }
  
  return 0;
}
