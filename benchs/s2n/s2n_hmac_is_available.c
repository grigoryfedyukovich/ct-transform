#include "../ossl.h"
#include "../smack.h"
#include "../ct-verif.h"
#include "s2n.h"

/* Return 1 if hmac algorithm is available, 0 otherwise. */

int fn (s2n_hmac_algorithm hmac_alg)
{
  int is_available = 0;
  if (hmac_alg == S2N_HMAC_MD5 ||
      hmac_alg == S2N_HMAC_MD5 ||
      hmac_alg == S2N_HMAC_SSLv3_MD5 ||
      hmac_alg == S2N_HMAC_SSLv3_SHA1 )
      /* Set is_available to 0 if in FIPS mode, as MD5/SSLv3 algs are not available in FIPS mode. */
      is_available = !s2n_is_in_fips_mode();
  else
  if (hmac_alg == S2N_HMAC_NONE ||
      hmac_alg == S2N_HMAC_SHA1 ||
      hmac_alg == S2N_HMAC_SHA224 ||
      hmac_alg == S2N_HMAC_SHA256 ||
      hmac_alg == S2N_HMAC_SHA384 ||
      hmac_alg == S2N_HMAC_SHA512 )
    is_available = 1;
  else
    is_available = -1;
  
  return is_available;
}
