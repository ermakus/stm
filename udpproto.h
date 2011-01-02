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

#ifndef __UDPPROTO__H
#define __UDPPROTO__H
#include "sha.h"
#include <inttypes.h>

#include "util.h"
#include "context.h"

#ifdef __cplusplus
extern "C" {
#endif

int udp_init( short port) ;
int udp_ready( struct btContext *ctx) ;
int udp_connect( struct  btContext *ctx, btDownload *dl);
int udp_announce( struct btContext *ctx, btDownload *dl, char *state);

#ifdef __cplusplus
}
#endif


#endif
