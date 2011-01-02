/* 
 * Copyright 2010 Anton Ermak, All Right Reserved
 *
 * Based on libbt, Copyright 2003,2004,2005 Kevin Smathers
 *
 * Redistribution of this file in either its original form, or in an
 * updated form may be done under the terms of the GNU GENERAL PUBLIC LICENSE. 
 *
 * If this license is unacceptable to you then you
 * may not redistribute this work.
 * 
 * See the file COPYING for details.
 */

#ifndef SHA1_H
#define SHA1_H

/*
SHA-1 in C
By Steve Reid <sreid@sea-to-sky.net>
100% Public Domain

-----------------
23 Apr 2001 version from http://sea-to-sky.net/~sreid/
Modified slightly to take advantage of autoconf.
See sha1.c for full history comments.
*/

#define SHA_DIGEST_LENGTH 20

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>

typedef struct {
    uint32_t state[5];
    uint32_t count[2];
    unsigned char buffer[64];
} SHA1_CTX;

void SHA1Transform(uint32_t state[5], unsigned char buffer[64]);
void SHA1Init(SHA1_CTX* context);
void SHA1Update(SHA1_CTX* context, unsigned char* data, uint32_t len); /* JHB */
void SHA1Final(unsigned char digest[20], SHA1_CTX* context);


void SHA1(char *ptr,uint32_t len,char *dm);

#ifdef __cplusplus
}
#endif

#endif

