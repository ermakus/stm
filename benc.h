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

#ifndef __BENC__H
#define __BENC__H
#include "types.h"
#include "bts.h"

#ifdef __cplusplus
extern "C" {
#endif

/* string */

/* all functions return 0 - success, 1 - error */
int benc_put_string( struct btStream* bts, btString *s) ;
int benc_get_string( struct btStream* bts, btString *s) ;

/* integer */
int benc_put_int( struct btStream *bts, btInteger *i) ;
int benc_get_int( struct btStream *bts, btInteger *i) ;

/* list */
int benc_put_list( struct btStream *bts, btList *a) ;
int benc_get_list( struct btStream *bts, btList *a) ;

/* dictionary */
int benc_put_dict( struct btStream *bts, btDict *d) ;
int benc_get_dict( struct btStream *bts, btDict *a) ;

/* object */
int benc_put_object( btStream *bts, btObject *o) ;
int benc_get_object( btStream *bts, btObject **o) ;

#ifdef __cplusplus
}
#endif

#endif
