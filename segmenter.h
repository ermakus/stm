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

#ifndef __SEGMENTER__H
#define __SEGMENTER__H
#include "sha.h"
#include "util.h"
#include "bitset.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct btFile {
    _int64 start;
    _int64 len;
    char *path;
} btFile;

typedef struct btPartialPiece {
    struct btPartialPiece *next;
    int piecenumber;
    kBitSet filled;
    int nextbyteReq;		/* next request from here */
    int isdone;			/* 0-filling, 1-retry */

    char buffer[0];		/* blocksize - allocation trick when creating piece */
} btPartialPiece;

typedef struct btFileSet {
    _int64 tsize;		/* total size */
    _int64 ul;			/* bytes uploaded */
    _int64 dl;			/* bytes downloaded */
    _int64 left;		/* bytes left */

    kBitSet completed;		/* completed blocks */
    int nfiles;
    int blocksize;
    btFile** file;
    int npieces;
    char *hashes;		/* SHA data */
    btPartialPiece *partial;
} btFileSet;

btFileSet* btFileSet_create( btFileSet *fs, int npieces, int blocksize, const char *hashbuf) ;
void btFileSet_destroy( btFileSet *fs);

int btFileSet_addfile( btFileSet *fs, const char *path, _int64 len) ;

btFile *seg_findFile( btFileSet *fs, int piece);

int seg_piecelen( btFileSet *fs, int piece);

/* finds or allocates the given piece */
btPartialPiece *seg_getPiece( btFileSet *fs, int piece);

int seg_writebuf( btFileSet *fs, int piece, int offset, char *buf, int len) ;

int seg_readbuf( btFileSet *fs, kBitSet *writeCache, int piece, int start, char *buf, int len);
int seg_review( btFileSet *fs, kBitSet *writeCache, int piece);

void seg_markFile( btFileSet *fs, char *filename, kBitSet *interest);

#ifdef __cplusplus
}
#endif

#endif
