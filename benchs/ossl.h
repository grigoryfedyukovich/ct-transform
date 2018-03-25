/*
 * Copyright 2014-2016 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

unsigned int constant_time_msb(unsigned int a)
{
  return 0 - (a >> (sizeof(a) * 8 - 1));
}

unsigned int constant_time_is_zero(unsigned int a)
{
  return constant_time_msb((~a) & (a - 1));
}

unsigned int constant_time_eq(unsigned int a, unsigned int b)
{
  return constant_time_is_zero(a ^ b);
}

unsigned int constant_time_eq_GF(unsigned int a, unsigned int b)
{
  return constant_time_eq(a, b) & 1;
}

unsigned int constant_time_lt(unsigned int a, unsigned int b)
{
  return constant_time_msb(a ^ ((a ^ b) | ((a - b) ^ b)));
}

unsigned int constant_time_lt_GF(unsigned int a, unsigned int b)
{
  return constant_time_lt(a, b) & 1;
}

unsigned int constant_time_gt(unsigned int a, unsigned int b)
{
  return constant_time_lt(b, a);
}

unsigned int constant_time_gt_GF(unsigned int a, unsigned int b)
{
  return constant_time_gt(a, b) & 1;
}

unsigned int constant_time_ge(unsigned int a, unsigned int b)
{
  return ~constant_time_lt(a, b);
}

unsigned int constant_time_ge_GF(unsigned int a, unsigned int b)
{
  return constant_time_ge(a, b) & 1;
}

unsigned int constant_time_le(unsigned int a, unsigned int b)
{
  return ~constant_time_gt(a, b);
}

unsigned int constant_time_le_GF(unsigned int a, unsigned int b)
{
  return constant_time_le(a, b) & 1;
}

unsigned int constant_time_select(unsigned int mask, unsigned int a, unsigned int b)
{
  return (mask & a) | (~mask & b);
}
