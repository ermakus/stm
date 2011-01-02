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

#ifndef __BTS__H
#define __BTS__H
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { BTS_INPUT, BTS_OUTPUT, BTS_FREE } btsIo;
struct btStream;
typedef struct btStream {
    int type;
    btsIo iodir;
    int (*read)(struct btStream* bts, char *buf, int len);
    int (*write)(struct btStream* bts, char *buf, int len);
    int (*peek)(struct btStream* bts);
    int (*rewind)(struct btStream* bts, btsIo iodir);
    void (*destroy)(struct btStream* bts);
} btStream;

struct btstrbuf {
    char *buf;
    int len;
};

btStream* bts_create_strstream( btsIo iodir);
btStream* bts_create_filestream_fp( FILE *fp, btsIo iodir) ;
btStream* bts_create_filestream( char *fname, btsIo iodir) ;
void bts_destroy( btStream* bts);
int bts_rewind( btStream* bts, btsIo iodir);

struct btstrbuf bts_get_buf( btStream *bts) ;

int bts_read( btStream* bts, char *buf, int len) ;
int bts_write( btStream* bts, char *buf, int len) ;
size_t writebts( void *buf, size_t size, size_t nmemb, void* stream );
int bts_peek( btStream* bts) ;
int bts_printf( btStream* bts, char *fmt, ...) ;

int bts_scanbreak( btStream* bts, char *cset, char *bset, char *buf, int bufsz);

#ifdef __cplusplus
}
#endif


#endif
