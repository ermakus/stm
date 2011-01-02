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

#ifndef __PEEREXCHANGE_H__
#define __PEEREXCHANGE_H__

#include <netinet/in.h>
#include "types.h"

// Message defines
#define BT_MSG_EXTENDED 20
#define BT_EXT_HANDSHAKE 0
#define BT_EXT_UTPEX 1

#ifdef __cplusplus
extern "C" {
#endif

// Def to 1 for debug stderr output
//#define PEX_DEBUG 1

enum {
    TRACKER=0,
    CACHE,
    PEX};

typedef struct btPeerAddress {
    struct in_addr ip; // ip address in network order
    uint16_t port;
} btPeerAddress;

typedef struct btPeerCache {
    int len;
    struct btPeerAddress **address;
    int allocated;
} btPeerCache;

struct btDownload;
struct btPeer;

// Peer Exchange
int sendExtendedHandshake(struct btPeer *peer, int listenPort, int privateflag);
int buildDictFromCurrentPeersForPeer(btDict *targetDict, struct btPeer *peer);
int sendPeerExchange(struct btDownload *dl, struct btPeer *peer);

// Peer Cache
btPeerCache *btPeerCache_Create(btPeerCache *cache);
int btPeerCache_AddPeer(btPeerCache *cache, struct btPeer *peer);
int btPeerCache_GetPosition(btPeerCache *cache, struct btPeer *peer);
int btPeerCache_DelPeer(btPeerCache *cache, int position);
void btPeerCache_Destroy(btPeerCache *cache);

#ifdef __cplusplus
}
#endif

#endif
