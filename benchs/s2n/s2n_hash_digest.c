#include "../ossl.h"
#include "../smack.h"
#include "../ct-verif.h"
#include "s2n.h"

int fn(struct s2n_hash_state *state, void *out, uint32_t size)
{
  // append the bit '1' to the message e.g. by adding 0x80 if message length is a multiple of 8 bits.
  uint32_t min_bytes_to_add = MARKER_BYTE_LENGTH;
  min_bytes_to_add += LENGTH_FIELD_SIZE;
  
  int bytes_to_add;
  if(state->currently_in_hash_block + min_bytes_to_add <= BLOCK_SIZE){
    bytes_to_add = BLOCK_SIZE - state->currently_in_hash_block;
  } else {
    bytes_to_add = BLOCK_SIZE + (BLOCK_SIZE - state->currently_in_hash_block);
  }
  
  s2n_hash_update(state, NULL, bytes_to_add);
  return SUCCESS;
}
