#include "../ossl.h"
#include "../smack.h"
#include "../ct-verif.h"
#include "s2n.h"

int fn(s2n_hmac_algorithm hmac_alg, s2n_hash_algorithm *out)
{
  // GF: switch replaced by if-then-else

  if (hmac_alg == S2N_HMAC_NONE) *out = S2N_HASH_NONE;
  if (hmac_alg == S2N_HMAC_MD5)  *out = S2N_HASH_MD5;
  if (hmac_alg == S2N_HMAC_SHA1) *out = S2N_HASH_SHA1;
  if (hmac_alg == S2N_HMAC_SHA224)  *out = S2N_HASH_SHA224;
  if (hmac_alg == S2N_HMAC_SHA256)  *out = S2N_HASH_SHA256;
  if (hmac_alg == S2N_HMAC_SHA384)  *out = S2N_HASH_SHA384;
  if (hmac_alg == S2N_HMAC_SHA512)  *out = S2N_HASH_SHA512;
  if (hmac_alg == S2N_HMAC_SSLv3_MD5)  *out = S2N_HASH_MD5;
  if (hmac_alg == S2N_HMAC_SSLv3_SHA1) *out = S2N_HASH_SHA1;
  return 0;
}
