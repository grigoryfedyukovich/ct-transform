#include "../ossl.h"
#include "../smack.h"
#include "../ct-verif.h"
#include "s2n.h"

int fn(struct s2n_stuffer *stuffer, uint32_t n)
{
  if (s2n_stuffer_data_available(stuffer) < n) {
    return -1;
  }
  
  stuffer->read_cursor += n;
  return 0;
}
