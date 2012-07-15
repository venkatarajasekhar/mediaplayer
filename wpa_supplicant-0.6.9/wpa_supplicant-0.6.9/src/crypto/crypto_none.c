/*
 * WPA Supplicant / Empty template functions for crypto wrapper
 * Copyright (c) 2005, Jouni Malinen <j@w1.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 * See README and COPYING for more details.
 */

#include "includes.h"

#include "common.h"
#include "crypto.h"


void md4_vector(size_t num_elem, const u8 *addr[], const size_t *len, u8 *mac)
{
}


void des_encrypt(const u8 *clear, const u8 *key, u8 *cypher)
{
}
int crypto_mod_exp(const u8 *base, size_t base_len,
                   const u8 *power, size_t power_len,
                   const u8 *modulus, size_t modulus_len,
                   u8 *result, size_t *result_len)
{
    return 0;
}
