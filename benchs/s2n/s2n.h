#include <stdint.h>
#include <stdlib.h>

// unified header

#define S2N_TLS_CIPHER_SUITE_LEN        2

const int MARKER_BYTE_LENGTH = 1;

typedef enum {
  S2N_HASH_NONE,
  S2N_HASH_MD5,
  S2N_HASH_SHA1,
  S2N_HASH_SHA224,
  S2N_HASH_SHA256,
  S2N_HASH_SHA384,
  S2N_HASH_SHA512,
  S2N_HASH_MD5_SHA1,
  /* Don't add any hash algorithms below S2N_HASH_SENTINEL */
  S2N_HASH_SENTINEL
} s2n_hash_algorithm;

typedef enum {
  S2N_HMAC_NONE,
  S2N_HMAC_MD5,
  S2N_HMAC_SHA1,
  S2N_HMAC_SHA224,
  S2N_HMAC_SHA256,
  S2N_HMAC_SHA384,
  S2N_HMAC_SHA512,
  S2N_HMAC_SSLv3_MD5,
  S2N_HMAC_SSLv3_SHA1
} s2n_hmac_algorithm;

struct s2n_hash_state {
  s2n_hash_algorithm alg;
  int currently_in_hash_block;
};

enum {
  PER_BLOCK_COST = 1000,
  PER_BYTE_COST = 1,
  BLOCK_SIZE = 64,
  LENGTH_FIELD_SIZE = 8,
  DIGEST_SIZE = 20
};

enum {
  SUCCESS = 0,
  FAILURE = -1
};

struct s2n_blob {
  unsigned int *data;
  unsigned int size;
  unsigned int allocated;
  unsigned int mlocked:1;
};

static int s2n_fips_mode = 0;

int s2n_is_in_fips_mode(void)
{
  return s2n_fips_mode;
}

int num_blocks(unsigned int numBytes) {
  if (numBytes <  1*BLOCK_SIZE) return 0;
  if (numBytes <  2*BLOCK_SIZE) return 1;
  if (numBytes <  3*BLOCK_SIZE) return 2;
  if (numBytes <  4*BLOCK_SIZE) return 3;
  if (numBytes <  5*BLOCK_SIZE) return 4;
  if (numBytes <  6*BLOCK_SIZE) return 5;
  if (numBytes <  7*BLOCK_SIZE) return 6;
  if (numBytes <  8*BLOCK_SIZE) return 7;
  if (numBytes <  9*BLOCK_SIZE) return 8;
  if (numBytes < 10*BLOCK_SIZE) return 9;
  if (numBytes < 11*BLOCK_SIZE) return 10;
  if (numBytes < 12*BLOCK_SIZE) return 11;
  if (numBytes < 13*BLOCK_SIZE) return 12;
  if (numBytes < 14*BLOCK_SIZE) return 13;
  if (numBytes < 15*BLOCK_SIZE) return 14;
  if (numBytes < 16*BLOCK_SIZE) return 15;
  if (numBytes < 17*BLOCK_SIZE) return 16;
  if (numBytes == 17*BLOCK_SIZE) return 17;
  return 0; //unreachable
}

int s2n_hash_update(struct s2n_hash_state *state, const void *data, uint32_t size)
{
  state->currently_in_hash_block += size;
  int num_filled_blocks = num_blocks(state->currently_in_hash_block);
  state->currently_in_hash_block = state->currently_in_hash_block - num_filled_blocks*BLOCK_SIZE;
  return SUCCESS;
}

// shortened version
struct s2n_cipher_suite {
  /* Is there an implementation available? Set in s2n_cipher_suites_init() */
  unsigned int available:1;
};

struct s2n_cipher_preferences {
  uint8_t count;
  struct s2n_cipher_suite **suites;
  int minimum_protocol_version;
};

#define TLS_HASH_ALGORITHM_SHA1             2
#define TLS_HASH_ALGORITHM_SHA224           3
#define TLS_HASH_ALGORITHM_SHA256           4
#define TLS_HASH_ALGORITHM_SHA384           5
#define TLS_HASH_ALGORITHM_SHA512           6

/* Our own order of preference for signature hashes. No MD5 to avoid
 * SLOTH.
 */
static uint8_t s2n_preferred_hashes[] = {
  TLS_HASH_ALGORITHM_SHA256,
  TLS_HASH_ALGORITHM_SHA384,
  TLS_HASH_ALGORITHM_SHA512,
  TLS_HASH_ALGORITHM_SHA224,
  TLS_HASH_ALGORITHM_SHA1 };

struct s2n_stuffer {
  /* The data for the s2n_stuffer */
  struct s2n_blob blob;
  
  /* Cursors to the current read/write position in the s2n_stuffer */
  uint32_t read_cursor;
  uint32_t write_cursor;
  
  /* The total size of the data segment */
  /* Has the stuffer been wiped? */
  unsigned int wiped:1;
  
  /* Was this stuffer alloc()'d ? */
  unsigned int alloced:1;
  
  /* Is this stuffer growable? */
  unsigned int growable:1;
  
  /* A growable stuffer can also be temporarily tainted */
  unsigned int tainted:1;
};

#define s2n_stuffer_data_available( s )   ((s)->write_cursor - (s)->read_cursor)


