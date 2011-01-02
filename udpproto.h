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
