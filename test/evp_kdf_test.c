/*
 * Copyright 2018-2019 The OpenSSL Project Authors. All Rights Reserved.
 * Copyright (c) 2018-2019, Oracle and/or its affiliates.  All rights reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

/* Tests of the EVP_KDF_CTX APIs */

#include <stdio.h>
#include <string.h>

#include <openssl/evp.h>
#include <openssl/kdf.h>
#include "testutil.h"

static int test_kdf_tls1_prf(void)
{
    int ret;
    EVP_KDF_CTX *kctx = NULL;
    const EVP_KDF *kdf;
    unsigned char out[16];
    static const unsigned char expected[sizeof(out)] = {
        0x8e, 0x4d, 0x93, 0x25, 0x30, 0xd7, 0x65, 0xa0,
        0xaa, 0xe9, 0x74, 0xc3, 0x04, 0x73, 0x5e, 0xcc
    };

    ret =
        TEST_ptr(kdf = EVP_get_kdfbyname(SN_tls1_prf))
        && TEST_ptr(kctx = EVP_KDF_CTX_new(kdf))
        && TEST_ptr_eq(EVP_KDF_CTX_kdf(kctx), kdf)
        && TEST_str_eq(EVP_KDF_name(kdf), SN_tls1_prf)
        && TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_MD, EVP_sha256()), 0)
        && TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_TLS_SECRET,
                                    "secret", (size_t)6), 0)
        && TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_ADD_TLS_SEED, "seed",
                                    (size_t)4), 0)
        && TEST_int_gt(EVP_KDF_derive(kctx, out, sizeof(out)), 0)
        && TEST_mem_eq(out, sizeof(out), expected, sizeof(expected));

    EVP_KDF_CTX_free(kctx);
    return ret;
}

static int test_kdf_hkdf(void)
{
    int ret;
    EVP_KDF_CTX *kctx;
    unsigned char out[10];
    static const unsigned char expected[sizeof(out)] = {
        0x2a, 0xc4, 0x36, 0x9f, 0x52, 0x59, 0x96, 0xf8, 0xde, 0x13
    };

    ret =
        TEST_ptr(kctx = EVP_KDF_CTX_new_id(EVP_KDF_HKDF))
        && TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_MD, EVP_sha256()), 0)
        && TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_SALT, "salt",
                                    (size_t)4), 0)
        && TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_KEY, "secret",
                                    (size_t)6), 0)
        && TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_ADD_HKDF_INFO, "label",
                                    (size_t)5), 0)
        && TEST_int_gt(EVP_KDF_derive(kctx, out, sizeof(out)), 0)
        && TEST_mem_eq(out, sizeof(out), expected, sizeof(expected));

    EVP_KDF_CTX_free(kctx);
    return ret;
}

static int test_kdf_pbkdf2(void)
{
    int ret;
    EVP_KDF_CTX *kctx;
    unsigned char out[25];
    size_t len = 0;
    const unsigned char expected[sizeof(out)] = {
        0x34, 0x8c, 0x89, 0xdb, 0xcb, 0xd3, 0x2b, 0x2f,
        0x32, 0xd8, 0x14, 0xb8, 0x11, 0x6e, 0x84, 0xcf,
        0x2b, 0x17, 0x34, 0x7e, 0xbc, 0x18, 0x00, 0x18,
        0x1c
    };

    if (sizeof(len) > 32)
        len = SIZE_MAX;

    ret = TEST_ptr(kctx = EVP_KDF_CTX_new_id(EVP_KDF_PBKDF2))
          && TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_PASS,
                                      "passwordPASSWORDpassword",
                                      (size_t)24), 0)
          && TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_SALT,
                                      "saltSALTsaltSALTsaltSALTsaltSALTsalt",
                                      (size_t)36), 0)
          && TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_ITER, 4096), 0)
          && TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_MD, EVP_sha256()),
                         0)
          && TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_PBKDF2_PKCS5_MODE,
                                            0), 0)
          && TEST_int_gt(EVP_KDF_derive(kctx, out, sizeof(out)), 0)
          && TEST_mem_eq(out, sizeof(out), expected, sizeof(expected))
          /* A key length that is too small should fail */
          && TEST_int_eq(EVP_KDF_derive(kctx, out, 112 / 8 - 1), 0)
          /* A key length that is too large should fail */
          && (len == 0 || TEST_int_eq(EVP_KDF_derive(kctx, out, len), 0))
          /* Salt length less than 128 bits should fail */
          && TEST_int_eq(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_SALT,
                                      "123456781234567",
                                      (size_t)15), 0)
          /* A small iteration count should fail */
          && TEST_int_eq(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_ITER, 1), 0)
          && TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_PBKDF2_PKCS5_MODE,
                                      1), 0)
          /* Small salts will pass if the "pkcs5" mode is enabled */
          && TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_SALT,
                                      "123456781234567",
                                      (size_t)15), 0)
          /* A small iteration count will pass if "pkcs5" mode is enabled */
          && TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_ITER, 1), 0)
          /*
           * If the "pkcs5" mode is disabled then the small salt and iter will
           * fail when the derive gets called.
           */
          && TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_PBKDF2_PKCS5_MODE,
                                      0), 0)
          && TEST_int_eq(EVP_KDF_derive(kctx, out, sizeof(out)), 0);

    EVP_KDF_CTX_free(kctx);
    return ret;
}

#ifndef OPENSSL_NO_SCRYPT
static int test_kdf_scrypt(void)
{
    int ret;
    EVP_KDF_CTX *kctx;
    unsigned char out[64];
    static const unsigned char expected[sizeof(out)] = {
        0xfd, 0xba, 0xbe, 0x1c, 0x9d, 0x34, 0x72, 0x00,
        0x78, 0x56, 0xe7, 0x19, 0x0d, 0x01, 0xe9, 0xfe,
        0x7c, 0x6a, 0xd7, 0xcb, 0xc8, 0x23, 0x78, 0x30,
        0xe7, 0x73, 0x76, 0x63, 0x4b, 0x37, 0x31, 0x62,
        0x2e, 0xaf, 0x30, 0xd9, 0x2e, 0x22, 0xa3, 0x88,
        0x6f, 0xf1, 0x09, 0x27, 0x9d, 0x98, 0x30, 0xda,
        0xc7, 0x27, 0xaf, 0xb9, 0x4a, 0x83, 0xee, 0x6d,
        0x83, 0x60, 0xcb, 0xdf, 0xa2, 0xcc, 0x06, 0x40
    };

    ret =
        TEST_ptr(kctx = EVP_KDF_CTX_new_id(EVP_KDF_SCRYPT))
        && TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_PASS, "password",
                                    (size_t)8), 0)
        && TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_SALT, "NaCl",
                                    (size_t)4), 0)
        && TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_SCRYPT_N,
                                    (uint64_t)1024), 0)
        && TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_SCRYPT_R,
                                    (uint32_t)8), 0)
        && TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_SCRYPT_P,
                                    (uint32_t)16), 0)
        && TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_MAXMEM_BYTES,
                                    (uint64_t)16), 0)
        /* failure test */
        && TEST_int_le(EVP_KDF_derive(kctx, out, sizeof(out)), 0)
        && TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_MAXMEM_BYTES,
                                    (uint64_t)(10 * 1024 * 1024)), 0)
        && TEST_int_gt(EVP_KDF_derive(kctx, out, sizeof(out)), 0)
        && TEST_mem_eq(out, sizeof(out), expected, sizeof(expected));

    EVP_KDF_CTX_free(kctx);
    return ret;
}
#endif /* OPENSSL_NO_SCRYPT */

static int test_kdf_ss_hash(void)
{
    int ret;
    EVP_KDF_CTX *kctx = NULL;
    unsigned char out[14];
    static const unsigned char z[] = {
        0x6d,0xbd,0xc2,0x3f,0x04,0x54,0x88,0xe4,0x06,0x27,0x57,0xb0,0x6b,0x9e,
        0xba,0xe1,0x83,0xfc,0x5a,0x59,0x46,0xd8,0x0d,0xb9,0x3f,0xec,0x6f,0x62,
        0xec,0x07,0xe3,0x72,0x7f,0x01,0x26,0xae,0xd1,0x2c,0xe4,0xb2,0x62,0xf4,
        0x7d,0x48,0xd5,0x42,0x87,0xf8,0x1d,0x47,0x4c,0x7c,0x3b,0x18,0x50,0xe9
    };
    static const unsigned char other[] = {
        0xa1,0xb2,0xc3,0xd4,0xe5,0x43,0x41,0x56,0x53,0x69,0x64,0x3c,0x83,0x2e,
        0x98,0x49,0xdc,0xdb,0xa7,0x1e,0x9a,0x31,0x39,0xe6,0x06,0xe0,0x95,0xde,
        0x3c,0x26,0x4a,0x66,0xe9,0x8a,0x16,0x58,0x54,0xcd,0x07,0x98,0x9b,0x1e,
        0xe0,0xec,0x3f,0x8d,0xbe
    };
    static const unsigned char expected[sizeof(out)] = {
        0xa4,0x62,0xde,0x16,0xa8,0x9d,0xe8,0x46,0x6e,0xf5,0x46,0x0b,0x47,0xb8
    };

    ret =
        TEST_ptr(kctx = EVP_KDF_CTX_new_id(EVP_KDF_SS))
        && TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_MD, EVP_sha224()), 0)
        && TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_KEY, z, sizeof(z)), 0)
        && TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_SSKDF_INFO, other,
                                    sizeof(other)), 0)
        && TEST_int_gt(EVP_KDF_derive(kctx, out, sizeof(out)), 0)
        && TEST_mem_eq(out, sizeof(out), expected, sizeof(expected));

    EVP_KDF_CTX_free(kctx);
    return ret;
}

static int test_kdf_x963(void)
{
    int ret;
    EVP_KDF_CTX *kctx = NULL;
    unsigned char out[1024 / 8];
    /*
     * Test data from https://csrc.nist.gov/CSRC/media/Projects/
     *  Cryptographic-Algorithm-Validation-Program/documents/components/
     *  800-135testvectors/ansx963_2001.zip
     */
    static const unsigned char z[] = {
        0x00, 0xaa, 0x5b, 0xb7, 0x9b, 0x33, 0xe3, 0x89, 0xfa, 0x58, 0xce, 0xad,
        0xc0, 0x47, 0x19, 0x7f, 0x14, 0xe7, 0x37, 0x12, 0xf4, 0x52, 0xca, 0xa9,
        0xfc, 0x4c, 0x9a, 0xdb, 0x36, 0x93, 0x48, 0xb8, 0x15, 0x07, 0x39, 0x2f,
        0x1a, 0x86, 0xdd, 0xfd, 0xb7, 0xc4, 0xff, 0x82, 0x31, 0xc4, 0xbd, 0x0f,
        0x44, 0xe4, 0x4a, 0x1b, 0x55, 0xb1, 0x40, 0x47, 0x47, 0xa9, 0xe2, 0xe7,
        0x53, 0xf5, 0x5e, 0xf0, 0x5a, 0x2d
    };
    static const unsigned char shared[] = {
        0xe3, 0xb5, 0xb4, 0xc1, 0xb0, 0xd5, 0xcf, 0x1d, 0x2b, 0x3a, 0x2f, 0x99,
        0x37, 0x89, 0x5d, 0x31
    };
    static const unsigned char expected[sizeof(out)] = {
        0x44, 0x63, 0xf8, 0x69, 0xf3, 0xcc, 0x18, 0x76, 0x9b, 0x52, 0x26, 0x4b,
        0x01, 0x12, 0xb5, 0x85, 0x8f, 0x7a, 0xd3, 0x2a, 0x5a, 0x2d, 0x96, 0xd8,
        0xcf, 0xfa, 0xbf, 0x7f, 0xa7, 0x33, 0x63, 0x3d, 0x6e, 0x4d, 0xd2, 0xa5,
        0x99, 0xac, 0xce, 0xb3, 0xea, 0x54, 0xa6, 0x21, 0x7c, 0xe0, 0xb5, 0x0e,
        0xef, 0x4f, 0x6b, 0x40, 0xa5, 0xc3, 0x02, 0x50, 0xa5, 0xa8, 0xee, 0xee,
        0x20, 0x80, 0x02, 0x26, 0x70, 0x89, 0xdb, 0xf3, 0x51, 0xf3, 0xf5, 0x02,
        0x2a, 0xa9, 0x63, 0x8b, 0xf1, 0xee, 0x41, 0x9d, 0xea, 0x9c, 0x4f, 0xf7,
        0x45, 0xa2, 0x5a, 0xc2, 0x7b, 0xda, 0x33, 0xca, 0x08, 0xbd, 0x56, 0xdd,
        0x1a, 0x59, 0xb4, 0x10, 0x6c, 0xf2, 0xdb, 0xbc, 0x0a, 0xb2, 0xaa, 0x8e,
        0x2e, 0xfa, 0x7b, 0x17, 0x90, 0x2d, 0x34, 0x27, 0x69, 0x51, 0xce, 0xcc,
        0xab, 0x87, 0xf9, 0x66, 0x1c, 0x3e, 0x88, 0x16
    };

    ret =
        TEST_ptr(kctx = EVP_KDF_CTX_new_id(EVP_KDF_X963))
        && TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_MD, EVP_sha512()), 0)
        && TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_KEY, z, sizeof(z)), 0)
        && TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_SHARED_INFO, shared,
                                    sizeof(shared)), 0)
        && TEST_int_gt(EVP_KDF_derive(kctx, out, sizeof(out)), 0)
        && TEST_mem_eq(out, sizeof(out), expected, sizeof(expected));

    EVP_KDF_CTX_free(kctx);
    return ret;
}

static int test_kdf_ss_hmac(void)
{
    int ret;
    EVP_KDF_CTX *kctx;
    unsigned char out[16];
    static const unsigned char z[] = {
        0xb7,0x4a,0x14,0x9a,0x16,0x15,0x46,0xf8,0xc2,0x0b,0x06,0xac,0x4e,0xd4
    };
    static const unsigned char other[] = {
        0x34,0x8a,0x37,0xa2,0x7e,0xf1,0x28,0x2f,0x5f,0x02,0x0d,0xcc
    };
    static const unsigned char salt[] = {
        0x36,0x38,0x27,0x1c,0xcd,0x68,0xa2,0x5d,0xc2,0x4e,0xcd,0xdd,0x39,0xef,
        0x3f,0x89
    };
    static const unsigned char expected[sizeof(out)] = {
        0x44,0xf6,0x76,0xe8,0x5c,0x1b,0x1a,0x8b,0xbc,0x3d,0x31,0x92,0x18,0x63,
        0x1c,0xa3
    };

    ret =
        TEST_ptr(kctx = EVP_KDF_CTX_new_id(EVP_KDF_SS))
        && TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_MAC, "HMAC"), 0)
        && TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_MD,  EVP_sha256()), 0)
        && TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_KEY, z, sizeof(z)), 0)
        && TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_SSKDF_INFO, other,
                                    sizeof(other)), 0)
        && TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_SALT, salt,
                                    sizeof(salt)), 0)
        && TEST_int_gt(EVP_KDF_derive(kctx, out, sizeof(out)), 0)
        && TEST_mem_eq(out, sizeof(out), expected, sizeof(expected));

    EVP_KDF_CTX_free(kctx);
    return ret;
}

static int test_kdf_ss_kmac(void)
{
    int ret;
    EVP_KDF_CTX *kctx;
    unsigned char out[64];
    static const unsigned char z[] = {
        0xb7,0x4a,0x14,0x9a,0x16,0x15,0x46,0xf8,0xc2,0x0b,0x06,0xac,0x4e,0xd4
    };
    static const unsigned char other[] = {
        0x34,0x8a,0x37,0xa2,0x7e,0xf1,0x28,0x2f,0x5f,0x02,0x0d,0xcc
    };
    static const unsigned char salt[] = {
        0x36,0x38,0x27,0x1c,0xcd,0x68,0xa2,0x5d,0xc2,0x4e,0xcd,0xdd,0x39,0xef,
        0x3f,0x89
    };
    static const unsigned char expected[sizeof(out)] = {
        0xe9,0xc1,0x84,0x53,0xa0,0x62,0xb5,0x3b,0xdb,0xfc,0xbb,0x5a,0x34,0xbd,
        0xb8,0xe5,0xe7,0x07,0xee,0xbb,0x5d,0xd1,0x34,0x42,0x43,0xd8,0xcf,0xc2,
        0xc2,0xe6,0x33,0x2f,0x91,0xbd,0xa5,0x86,0xf3,0x7d,0xe4,0x8a,0x65,0xd4,
        0xc5,0x14,0xfd,0xef,0xaa,0x1e,0x67,0x54,0xf3,0x73,0xd2,0x38,0xe1,0x95,
        0xae,0x15,0x7e,0x1d,0xe8,0x14,0x98,0x03
    };

    ret =
        TEST_ptr(kctx = EVP_KDF_CTX_new_id(EVP_KDF_SS))
        && TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_MAC, "KMAC128"), 0)
        && TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_KEY, z,
                                    sizeof(z)), 0)
        && TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_SSKDF_INFO, other,
                                    sizeof(other)), 0)
        && TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_SALT, salt,
                                    sizeof(salt)), 0)
        && TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_MAC_SIZE,
                                    (size_t)20), 0)
        && TEST_int_gt(EVP_KDF_derive(kctx, out, sizeof(out)), 0)
        && TEST_mem_eq(out, sizeof(out), expected, sizeof(expected));

    EVP_KDF_CTX_free(kctx);
    return ret;
}

static int test_kdf_sshkdf(void)
{
    int ret;
    EVP_KDF_CTX *kctx;
    unsigned char out[8];
    /* Test data from NIST CAVS 14.1 test vectors */
    static const unsigned char key[] = {
        0x00, 0x00, 0x00, 0x81, 0x00, 0x87, 0x5c, 0x55, 0x1c, 0xef, 0x52, 0x6a,
        0x4a, 0x8b, 0xe1, 0xa7, 0xdf, 0x27, 0xe9, 0xed, 0x35, 0x4b, 0xac, 0x9a,
        0xfb, 0x71, 0xf5, 0x3d, 0xba, 0xe9, 0x05, 0x67, 0x9d, 0x14, 0xf9, 0xfa,
        0xf2, 0x46, 0x9c, 0x53, 0x45, 0x7c, 0xf8, 0x0a, 0x36, 0x6b, 0xe2, 0x78,
        0x96, 0x5b, 0xa6, 0x25, 0x52, 0x76, 0xca, 0x2d, 0x9f, 0x4a, 0x97, 0xd2,
        0x71, 0xf7, 0x1e, 0x50, 0xd8, 0xa9, 0xec, 0x46, 0x25, 0x3a, 0x6a, 0x90,
        0x6a, 0xc2, 0xc5, 0xe4, 0xf4, 0x8b, 0x27, 0xa6, 0x3c, 0xe0, 0x8d, 0x80,
        0x39, 0x0a, 0x49, 0x2a, 0xa4, 0x3b, 0xad, 0x9d, 0x88, 0x2c, 0xca, 0xc2,
        0x3d, 0xac, 0x88, 0xbc, 0xad, 0xa4, 0xb4, 0xd4, 0x26, 0xa3, 0x62, 0x08,
        0x3d, 0xab, 0x65, 0x69, 0xc5, 0x4c, 0x22, 0x4d, 0xd2, 0xd8, 0x76, 0x43,
        0xaa, 0x22, 0x76, 0x93, 0xe1, 0x41, 0xad, 0x16, 0x30, 0xce, 0x13, 0x14,
        0x4e
    };
    static const unsigned char xcghash[] = {
        0x0e, 0x68, 0x3f, 0xc8, 0xa9, 0xed, 0x7c, 0x2f, 0xf0, 0x2d, 0xef, 0x23,
        0xb2, 0x74, 0x5e, 0xbc, 0x99, 0xb2, 0x67, 0xda, 0xa8, 0x6a, 0x4a, 0xa7,
        0x69, 0x72, 0x39, 0x08, 0x82, 0x53, 0xf6, 0x42
    };
    static const unsigned char sessid[] = {
        0x0e, 0x68, 0x3f, 0xc8, 0xa9, 0xed, 0x7c, 0x2f, 0xf0, 0x2d, 0xef, 0x23,
        0xb2, 0x74, 0x5e, 0xbc, 0x99, 0xb2, 0x67, 0xda, 0xa8, 0x6a, 0x4a, 0xa7,
        0x69, 0x72, 0x39, 0x08, 0x82, 0x53, 0xf6, 0x42
    };
    static const unsigned char expected[sizeof(out)] = {
        0x41, 0xff, 0x2e, 0xad, 0x16, 0x83, 0xf1, 0xe6
    };

    ret =
        TEST_ptr(kctx = EVP_KDF_CTX_new_id(EVP_KDF_SSHKDF))
        && TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_MD, EVP_sha256()), 0)
        && TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_KEY, key,
                                    sizeof(key)), 0)
        && TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_SSHKDF_XCGHASH,
                                    xcghash, sizeof(xcghash)), 0)
        && TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_SSHKDF_SESSION_ID,
                                    sessid, sizeof(sessid)), 0)
        && TEST_int_gt(
                EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_SSHKDF_TYPE,
                            (int)EVP_KDF_SSHKDF_TYPE_INITIAL_IV_CLI_TO_SRV), 0)
        && TEST_int_gt(EVP_KDF_derive(kctx, out, sizeof(out)), 0)
        && TEST_mem_eq(out, sizeof(out), expected, sizeof(expected));

    EVP_KDF_CTX_free(kctx);
    return ret;
}

static int test_kdf_get_kdf(void)
{
    const EVP_KDF *kdf1, *kdf2;
    ASN1_OBJECT *obj;

    return
        TEST_ptr(obj = OBJ_nid2obj(NID_id_pbkdf2))
        && TEST_ptr(kdf1 = EVP_get_kdfbyname(LN_id_pbkdf2))
        && TEST_ptr(kdf2 = EVP_get_kdfbyobj(obj))
        && TEST_ptr_eq(kdf1, kdf2)
        && TEST_ptr(kdf1 = EVP_get_kdfbyname(SN_tls1_prf))
        && TEST_ptr(kdf2 = EVP_get_kdfbyname(LN_tls1_prf))
        && TEST_ptr_eq(kdf1, kdf2)
        && TEST_ptr(kdf2 = EVP_get_kdfbynid(NID_tls1_prf))
        && TEST_ptr_eq(kdf1, kdf2);
}

#ifndef OPENSSL_NO_CMS
static int test_kdf_x942_asn1(void)
{
    int ret;
    EVP_KDF_CTX *kctx = NULL;
    unsigned char out[24];
    /* RFC2631 Section 2.1.6 Test data */
    static const unsigned char z[] = {
        0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,
        0x0e,0x0f,0x10,0x11,0x12,0x13
    };
    static const unsigned char expected[sizeof(out)] = {
        0xa0,0x96,0x61,0x39,0x23,0x76,0xf7,0x04,
        0x4d,0x90,0x52,0xa3,0x97,0x88,0x32,0x46,
        0xb6,0x7f,0x5f,0x1e,0xf6,0x3e,0xb5,0xfb
    };

    ret =
        TEST_ptr(kctx = EVP_KDF_CTX_new_id(EVP_KDF_X942))
        && TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_MD, EVP_sha1()), 0)
        && TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_KEY, z, sizeof(z)), 0)
        && TEST_int_gt(EVP_KDF_ctrl(kctx, EVP_KDF_CTRL_SET_CEK_ALG,
                                    SN_id_smime_alg_CMS3DESwrap), 0)
        && TEST_int_gt(EVP_KDF_derive(kctx, out, sizeof(out)), 0)
        && TEST_mem_eq(out, sizeof(out), expected, sizeof(expected));

    EVP_KDF_CTX_free(kctx);
    return ret;
}
#endif /* OPENSSL_NO_CMS */

int setup_tests(void)
{
    ADD_TEST(test_kdf_get_kdf);
    ADD_TEST(test_kdf_tls1_prf);
    ADD_TEST(test_kdf_hkdf);
    ADD_TEST(test_kdf_pbkdf2);
#ifndef OPENSSL_NO_SCRYPT
    ADD_TEST(test_kdf_scrypt);
#endif
    ADD_TEST(test_kdf_ss_hash);
    ADD_TEST(test_kdf_ss_hmac);
    ADD_TEST(test_kdf_ss_kmac);
    ADD_TEST(test_kdf_sshkdf);
    ADD_TEST(test_kdf_x963);
#ifndef OPENSSL_NO_CMS
    ADD_TEST(test_kdf_x942_asn1);
#endif
    return 1;
}
