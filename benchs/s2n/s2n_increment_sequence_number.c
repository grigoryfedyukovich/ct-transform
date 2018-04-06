#include "../ossl.h"
#include "../smack.h"
#include "../ct-verif.h"
#include "s2n.h"

int fn(struct s2n_blob *sequence_number, unsigned int i)
{
  public_in(__SMACK_value(i));
  public_in(__SMACK_value(sequence_number->size));
  for (i = 0; i < sequence_number->size; i++) {
    sequence_number->data[i] += 1;
    if (sequence_number->data[i] > 0) {
      break;
    }
  }
  return 0;
}

