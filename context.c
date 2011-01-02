/* 
 * Copyright 2003,2004,2005 Kevin Smathers, All Rights Reserved
 *
 * This file may be used or modified without the need for a license.
 *
 * Redistribution of this file in either its original form, or in an
 * updated form may be done under the terms of the GNU LIBRARY GENERAL
 * PUBLIC LICENSE.  If this license is unacceptable to you then you
 * may not redistribute this work.
 * 
 * See the file COPYING.LGPL for details.
 */

#define USE_CURL

/* context.c */
#include "config.h"
#include "random.h"
#include "udpproto.h"

#include <curl/curl.h>
#include <curl/easy.h>
#include "sha.h"
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#if WIN32
#   include <winsock2.h>
#else
#   include <sys/param.h>
#   include <sys/socket.h>
#   include <unistd.h>
#   include <netdb.h>
#endif
#include <time.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "context.h"
#include "udpproto.h"
#include "bts.h"
#include "types.h"
#include "benc.h"
#include "random.h"
#include "peer.h"
#include "stream.h"
#include "util.h"
#include "segmenter.h"

#define MINPORT 6881
#define MAXPORT 6889
#define UNKNOWN_ID "--unknown--peer-id--"
#if WIN32
#   define snprintf _snprintf
#endif

#include "httpget.h"

static int parse_config_int( 
	const char *file, int lineno, const char *token, 
	int *value,
	const char *linebuf) 
{
    int tlen = strlen(token);
    const char *line;
    char *endp;
    int res=-1;
    if (strncmp( linebuf, token, tlen) == 0) {
	line = linebuf + tlen;
	line += strspn( line, " \t");
	*value = strtol( line, &endp, 10);

	if (*endp != 0) {
	    res = -1;
	    //printf("In configuration file %s, line %d, the token %s failed to decode (%d)\n", file, lineno, token, res);
	} else {
	    res = 0;
	}
    }
    return res;
}

static int parse_config_digest( 
	const char *file, int lineno, const char *token, 
	unsigned char *digest, int len, 
	const char *linebuf) 
{
    int tlen = strlen(token);
    const char *line;
    int res=-1;
    if (strncmp( linebuf, token, tlen) == 0) {
	line = linebuf + tlen;
	line += strspn( line, " \t");
	res = hexdecode( digest, len, line, strlen(line));
	if (res) {
	    printf("In configuration file %s, line %d, the token %s failed to decode (%d)\n", file, lineno, token, res);
	}
    }
    return res;
}

btContext *btContext_create( btContext *ctx, float ulfactor, char *rcfile) {
    FILE *config;
    char *env;
    char hexbuf[80];
    int lineno = 0;
    int i;

    if (!ctx) {
        ctx = btmalloc( sizeof(*ctx));
    }
    /* default initialize the entire context */
    memset( ctx, 0, sizeof(*ctx));
    /* initialize the socket to status map */
    for (i=0; i<SOCKID_MAX; i++) {
	ctx->statmap[i] = -1;
    }

    randomid( ctx->myid, IDSIZE);
    randomid( ctx->mykey, KEYSIZE);
    ctx->minport = MINPORT;
    ctx->maxport = MAXPORT;
    ctx->ulfactor = ulfactor;
    ctx->listenport = ctx->minport;
    return ctx;
}

void ctx_closedownload(btContext *ctx, unsigned download) {
  int i;
  btDownload *dl=ctx->downloads[download];
  btPeerset *pset = &dl->peerset;

  for (i=0; i<pset->len; i++) {
      if (pset->peer[i] != NULL) {
	  peer_shutdown (ctx, pset->peer[i], "exiting");
	  btfree(pset->peer[i]);
	  pset->peer[i] = NULL;
      }
  }
  btfree(pset->peer);
  pset->peer = NULL;

  for (i=0; i<dl->nurl; i++) {
     btfree (dl->url[i]);
     btfree (dl->fault[i]);
  }
  btFileSet_destroy( &dl->fileset);
  kBitSet_finit( &dl->requested);
  kBitSet_finit( &dl->interested);
  if(dl->md)
    btObject_destroy( dl->md);
  ctx->downloads[download]=NULL;
  btfree(dl);
}

void btContext_destroy( btContext *ctx) {
    int i;

    for(i=0; i<ctx->downloadcount; i++) {
      if(ctx->downloads[i])
	ctx_closedownload(ctx, i);
    }

    /* Note: btfree(NULL) is a no-op */
    btfree (ctx->downloads);
    /* Shouldn't be necessary - one must recreate the context */
    ctx->downloads=NULL;
    ctx->downloadcount=0;
}

static char* hexdigest(unsigned char *digest, int len) {
    int i;
    char *buf = btmalloc(3*len+1);
    for (i=0; i<len; i++) {
	sprintf(buf+3*i, "%%%02x", digest[i]);
    }
    return buf;
}

int
btresponse( btContext *ctx, int download, btObject *resp) {
    int ret = 0;
    btDownload *dl=ctx->downloads[download];
    btString *err;
    btPeer *p;
    int i,j, skip=0;
    struct sockaddr_in target;
    time_t now;
  
    if (!dl) 
    {
	/* fix for thread timing in interactive clients */
	fprintf(stderr, "Tracker response does not map to active torrent\n");
    }
	
    if (dl->peerset.len >= TORRENT_MAXCONN)
    {
	fprintf(stderr, "Skipping peers - max connections reached for this torrent\n");
	return ret;
    }
    
    if (!resp)
    {
	fprintf(stderr, "Error from tracker: null response\n");
	ret = -1;
    } else { 
	err = BTSTRING(btObject_val( resp, "failure reason"));
    }
    if (err) {
	printf("Error from tracker: %s\n", err->buf);
	ret = -1;
    } else {
        btObject *peersgen;

	peersgen = btObject_val( resp, "added");
	if (peersgen) {
	    /* if from peer */
	} else {
	    /* if from tracker */
	    peersgen = btObject_val( resp, "peers");
	    btInteger *interval;
	    time(&now);
	    interval = BTINTEGER( btObject_val( resp, "interval"));
            if( !interval ) return -1;
	    dl->reregister_interval = (int)interval->ival;
	    dl->reregister_timer = now;
            btfree( dl->fault[0] );
            dl->fault[0] = strprintf( "CONNECTED: Next update in %.0f sec.", dl->reregister_interval / 60.0 ); 
        } 

	if (peersgen->t == BT_LIST) {
	    btList *peers = BTLIST( peersgen);
	    if (peers->len == 0) { ret = -1; }
	    for (i=0; i<peers->len; i++) {
		btObject *o = peers->list[i];
		btString *peerid = BTSTRING( btObject_val( o, "peer id"));
		btString *ip = BTSTRING( btObject_val( o, "ip"));
		btInteger *port = BTINTEGER( btObject_val( o, "port"));
                if( !port ) return -1;
		int iport = (int)port->ival;
		const struct addrinfo ai_template={
		  .ai_family=AF_INET,	/* PF_INET */
		  .ai_socktype=SOCK_STREAM,
		  /*.ai_protocol=IPPROTO_TCP,*/
		};
		struct addrinfo *ai=NULL;

		skip=0;

		/* Detect collisions properly (incl. ourselves!) */
		if (memcmp(ctx->myid, peerid->buf, IDSIZE)==0) {
		    printf("Skipping myself %s:%d\n", ip->buf, iport);
		    continue;
		}
		
		for (j = 0; j < dl->peerset.len; j++) {
		    btPeer *p = dl->peerset.peer[j];
		    if ( memcmp( p->id, peerid->buf, IDSIZE)==0) {
			/* Simply tests by ID. */
			fprintf( stderr, 
				   "Skipping old peer: %s:%d", inet_ntoa(target.sin_addr), ntohs(target.sin_port));
			skip = 1;
			break;
		    }
		}
		if (skip) continue;

		printf( "Contacting peer %s:%d\n", ip->buf, iport);
		if(getaddrinfo(ip->buf, NULL, &ai_template, &ai)==0) {	
		    /* Just pick the first one, they are returned in varying order */
		    struct sockaddr_in sa;
		    if (ai->ai_addr->sa_family == AF_INET) {
			/*result of lookup is an IPV4 address */
			sa = *(struct sockaddr_in *)ai->ai_addr;
			sa.sin_port = htons(iport);
			p = peer_add( ctx, download, peerid->buf, &sa);
		    } else {
		        printf( "Peer lookup returned unsupported address type %d. \n", 
				ai->ai_addr->sa_family);
		    }
		}
		if(ai) {
		    freeaddrinfo(ai);
		}
	    } /* for each peer */
	} else if (peersgen->t == BT_STRING) {
	    btString *peers=BTSTRING(peersgen);

	    fprintf( stderr, "Parsing compact peer list (%d)\n", peers->len/6);
	    if (peers->len == 0) { ret = -1; }
	    for (i=0; i<=peers->len - 6; i += 6) {
		struct sockaddr_in target;

		skip = 0;

		/* Collect the address */
		target.sin_family=AF_INET;
		memcpy(&target.sin_addr, peers->buf+i, 4);
		memcpy(&target.sin_port, peers->buf+i+4, 2); /* already nbo */
		
		// Skip peers on port 0
		if (!ntohs(target.sin_port))
		{
		    fprintf(stderr, "Skipping peer %s on port 0", 
			    inet_ntoa(target.sin_addr));
		    continue;
		}
		
		if (peer_seen( ctx, download, &target)) continue;
		p = peer_add( ctx, download, UNKNOWN_ID, &target);
	    } /* for */
	
	}
/*	peer_dump( &ctx->peerset); */
    }
    return ret;
}

static btObject* btrequest( 
	const char *announce,
	unsigned char myid[IDSIZE], 
	unsigned char mykey[KEYSIZE],
	unsigned char digest[SHA_DIGEST_LENGTH], 
	int port,
	_int64 downloaded, 
	_int64 uploaded,
	_int64 left,
	const char  *event,
	char **pfault) 
{
    /*
    * Tracker GET requests have the following keys urlencoded -
    *   req = {
    *REQUEST
    *      info_hash => 'hash'
    *      peer_id => 'random-20-character-name'
    *      port => '12345'
    *      ip => 'ip-address' -or- 'dns-name'  iff ip
    *      uploaded => '12345'
    *      downloaded => '12345'
    *      left => '12345'
    *
    *      last => last iff last
    *      trackerid => trackerid iff trackerid
    *      numwant => 0 iff howmany() >= maxpeers
    *      event => 'started', 'completed' -or- 'stopped' iff event != 'heartbeat'
    *
    *   }
    */

    /* contact tracker */
    CURL *hdl;
    char url[2048];
    char *dgurl;
    char *idurl;
    char *keyurl;
    btStream *io;
    btObject *result;
    int curlret;
    dgurl=hexdigest(digest, SHA_DIGEST_LENGTH);
    idurl=hexdigest(myid, IDSIZE);
    keyurl=hexdigest(mykey, KEYSIZE);
    char delimer = strchr( announce, '?' ) ? '&':'?';
    if (event) {
	snprintf( url, sizeof(url)-1, "%s%cinfo_hash=%s&peer_id=%s&key=%s&port=%d&uploaded=%lld&downloaded=%lld&left=%lld&event=%s&compact=1", 
		announce,
                delimer,
		dgurl,
		idurl,
		keyurl,
		port,
		uploaded, 
		downloaded,
		left,
		event
	    );
    } else {
	snprintf( url, sizeof(url)-1, "%s%cinfo_hash=%s&peer_id=%s&key=%s&port=%d&uploaded=%lld&downloaded=%lld&left=%lld&compact=1", 
		announce,
                delimer,
		dgurl,
		idurl,
		keyurl,
		port,
		uploaded, 
		downloaded,
		left
	    );
    }
    url[sizeof(url)-1]=0; 
    fprintf(stderr, "Tracker request: %s\n", url );
    btfree(idurl); btfree(dgurl); btfree(keyurl);

#ifdef USE_CURL
    hdl = curl_easy_init();
    curl_easy_setopt( hdl, CURLOPT_URL, url);
    io = bts_create_strstream( BTS_OUTPUT);
    curl_easy_setopt( hdl, CURLOPT_FILE, io);

    if( event && strcmp( event, "stopped" ) == 0 ) {
    	curl_easy_setopt( hdl, CURLOPT_CONNECTTIMEOUT, 5 );
    	curl_easy_setopt( hdl, CURLOPT_TIMEOUT, 5 );
    } else {
    	curl_easy_setopt( hdl, CURLOPT_CONNECTTIMEOUT, 30 );
    	curl_easy_setopt( hdl, CURLOPT_TIMEOUT, 30 );
    }

    curl_easy_setopt( hdl, CURLOPT_NOSIGNAL, 1 );
    curl_easy_setopt( hdl, CURLOPT_WRITEFUNCTION, writebts);

    if ((curlret = curl_easy_perform( hdl)) != CURLE_OK)
    {
	btfree( *pfault );
	*pfault = strprintf("Connection error: %d", curlret);
        result=NULL;
    }
    else
    {
      /* parse the response */
      if (bts_rewind( io, BTS_INPUT)) return NULL;
      if (benc_get_object( io, &result)) {
        bts_rewind( io, BTS_INPUT);
        struct btstrbuf  buf = bts_get_buf( io );
	btfree( *pfault );
        if( buf.len == 0 ) 
	   *pfault = strprintf("Empty response");
        else
 	   *pfault = strprintf("Invalid response");
        result = NULL;
      }
    }
    bts_destroy (io);
    curl_easy_cleanup( hdl);
#else
    size_t size = 16384;
    char* data = (char*)btmalloc( size );

    if( curlret = http_get( url, data, &size ) ) {
	btfree( *pfault );
	*pfault = strprintf("Connection error: %d", curlret);
        result=NULL;
    } 
    else 
    {
        btStream *io = bts_create_strstream( BTS_OUTPUT );
        bt_assert( io );
        bts_write( io, (char*)data, size );
        if(benc_get_object( io, &result)) 
        {
	   btfree( *pfault );
           if( size == 0 )  *pfault = strprintf("Empty response");
           else *pfault = strprintf("Invalid response");
           result = NULL;
        }
        bts_destroy( io );
    }
    btfree( data );
#endif
    return result;
}

btObject* btannounce(btContext *ctx, btDownload *dl, char *state, int err)
{
    btObject *resp = NULL;
    int i;
    
    if (err) dl->tracker++;
    for (;dl->tracker < dl->nurl; dl->tracker++) {
        if (strncmp(dl->url[dl->tracker], "udp://", 6) == 0) {
	    continue;
	} else {
	    resp = btrequest( 
		dl->url[dl->tracker], ctx->myid, ctx->mykey, dl->infohash, 
		ctx->listenport, dl->fileset.dl, 
		dl->fileset.ul * ctx->ulfactor, dl->fileset.left, 
		state,
		&dl->fault[dl->tracker]
	    );
	    if (resp) break;
	}
    }

    if (!resp) {
	dl->tracker=0;
	return NULL;
    }

    if (resp && dl->tracker != 0 && dl->tracker < dl->nurl) {
	btString *err = BTSTRING(btObject_val( resp, "failure reason"));
	if (!err) {
	    char *url = dl->url[dl->tracker];
            char *flt = dl->fault[dl->tracker];
	    for (i = dl->tracker; i>0; i--) {
		dl->url[i] = dl->url[i-1];
		dl->fault[i] = dl->fault[i-1];
	    }
	    dl->url[0] = url;
            dl->fault[0] = flt;
	    dl->tracker=0;
	}
        else
	{
	    btfree( dl->fault[dl->tracker] );
            dl->fault[dl->tracker] = strprintf( "Tracker error: %s", err->buf );
            btString_destroy( err );
	    btObject_destroy( resp );
	    return NULL;
	}
    }

    return resp;
}

int ctx_addstatus( btContext *ctx, int fd) {
    int statblock;

    bt_assert(fd>=0 && fd<=SOCKID_MAX); /* include TMPLOC */

    /* allocate status bits */
    statblock = ctx->nstatus;
    if (statblock >= MAXCONN) {
	return -1;
    }
    ctx->nstatus++;

    ctx->statmap[ fd] = statblock;
    ctx->status[ statblock].fd = fd;
    ctx->status[ statblock].events = 0;;
    return 0;
}

void 
ctx_setevents( btContext *ctx, int fd, int events) {
    int statblock;

    bt_assert(fd>=0 && fd<SOCKID_MAX);

    statblock = ctx->statmap[ fd];
    ctx->status[ statblock].events |= events;
}

void 
ctx_clrevents( btContext *ctx, int fd, int events) {
    int statblock;

    bt_assert(fd>=0 && fd<SOCKID_MAX);

    statblock = ctx->statmap[ fd];
    ctx->status[ statblock].events &= ~events;
}

void
ctx_delstatus( btContext *ctx, int fd) {
    /* free up the status slot */
    int sid;
    int i;

    bt_assert(fd>=0 && fd<=SOCKID_MAX);

    sid = ctx->statmap[fd];
    for (i=sid; i<ctx->nstatus; i++) {
        ctx->status[i] = ctx->status[i+1];
    }
    ctx->nstatus--;
    for (i=0; i<SOCKID_MAX; i++) {
	if (ctx->statmap[i] > sid) ctx->statmap[i]--;
    }
}

void
ctx_fixtmp( btContext *ctx, int fd) {
    int statblock;
    
    bt_assert(fd>=0 && fd<SOCKID_MAX);

    statblock = ctx->statmap[ TMPLOC];

    bt_assert(statblock >= 0 && statblock < MAXCONN);
    bt_assert(ctx->status[ statblock].fd == TMPLOC);

    /* relink the status block to the statmap */
    ctx->statmap[ fd] = statblock;
    ctx->status[ statblock].fd = fd;
    ctx->statmap[ TMPLOC] = -1;
}

int ctx_register( struct btContext *ctx, unsigned download)
    /*
     * Contact the tracker and update it on our status.  Also
     * add any new peers that the tracker reports back to us.
     */
{
    btDownload *dl=ctx->downloads[download];
    btObject *resp;
    int nok=0;

    bt_assert(download<ctx->downloadcount);
    do {
	/* contact tracker */
	resp = btannounce( ctx, dl, "started", nok);
	if(!resp)
	    return -EAGAIN;
	nok = btresponse( ctx, download, resp);
	btObject_destroy( resp);
    } while (nok);
    return 0;
}

int ctx_startserver( btContext *ctx) {
    struct sockaddr_in sin;
    /* open server socket */
    ctx->ss = socket( PF_INET, SOCK_STREAM, IPPROTO_TCP);
    for ( ctx->listenport = ctx->minport;
          ctx->listenport <= ctx->maxport; 
          ctx->listenport++) 
    {
	sin.sin_family = AF_INET;
	sin.sin_port = htons( ctx->listenport);
	sin.sin_addr.s_addr = INADDR_ANY;
	if (!bind( ctx->ss, (struct sockaddr *)&sin, sizeof(sin))) { 
	    ctx->udpsock = udp_init( ctx->listenport);
	    if ( ctx->udpsock > 0) {
		break;
	    }
	}
    }
    if (ctx->listenport > ctx->maxport) {
        bts_perror(errno, "bind");
        return 1;
    }
    if (listen( ctx->ss, 10)) { bts_perror(errno, "listen"); return 1; }

    /* setup for select */
    ctx_addstatus( ctx, ctx->ss);
    ctx_setevents( ctx, ctx->ss, POLLIN);

    ctx_addstatus( ctx, ctx->udpsock);
    ctx_setevents( ctx, ctx->udpsock, POLLIN);
    return 0;    
}

int ctx_shutdown( btContext *ctx, unsigned download) {
    int result;
    btString *err;
    btObject *resp;
    btDownload *dl=ctx->downloads[download];
    bt_assert(download<ctx->downloadcount);
    resp = btannounce( ctx, dl, "stopped", 0);
    if(resp) {
	btObject_destroy( resp);
	return 0;
    } else {
        return -1;
    }
}

int ctx_complete( btContext *ctx, unsigned download) {
    btString *err;
    btObject *resp;
    btDownload *dl=ctx->downloads[download];
    bt_assert(download<ctx->downloadcount);
    dl->complete=1;
    if(dl->fileset.dl==0)	/* don't send complete if we seeded */
        return 0;
    /* contact tracker */
    resp = btannounce( ctx, dl, "completed", 0);
    if(!resp) {
	return -EAGAIN;
    }
    btObject_destroy( resp);
    return 0; 
}

int ctx_reregister( btContext *ctx, unsigned download) {
    btObject *resp;
    int nok = 0;
    btDownload *dl=ctx->downloads[download];
    bt_assert(download<ctx->downloadcount);
    do {
	/* contact tracker */
	resp = btannounce( ctx, dl, NULL, nok);
	if(!resp)
	    return -EAGAIN;
	nok = btresponse( ctx, download, resp);
	btObject_destroy( resp);
    } while (nok);
    return 0; 
}

static int addurl( btDownload *dl, btString *a, int rank, char *ignorepattern) {
	if (!a) return 1;
    if (ignorepattern && strstr(a->buf, ignorepattern)) return 0;
    if (dl->nurl >= TRACKER_MAX) {
    	return 0;
    }
    dl->fault[dl->nurl] = strdup("Idle");
    dl->url[dl->nurl] = btmalloc( a->len+1);
    memcpy( dl->url[dl->nurl], a->buf, a->len);
    dl->url[dl->nurl][a->len] = '\0';
    dl->urlrank[dl->nurl] = rank;
    dl->nurl++;
    return 0;
}

static void shuffle( btDownload *dl) {
    int rank = 0;
    int start = 0;
    int count = 0;
    int i, pick;
    char *tmp;
    while (start < dl->nurl) {
        rank = dl->urlrank[ start];
        for (count = 0; dl->urlrank[ start+count] == rank && start+count < dl->nurl; count++);
	for (i=0; i<count; i++) {
	    pick = rnd(count);

	    tmp = dl->fault[ start+pick];
	    dl->fault[ start+pick] = dl->fault[ start+i];
	    dl->fault[ start+i] = tmp;

	    tmp = dl->url[ start+pick];
	    dl->url[ start+pick] = dl->url[ start+i];
	    dl->url[ start+i] = tmp;
	}
	start += count;
    }
}

int ctx_loadfile( 
	btStream *bts, 
	struct btContext *ctx, 
	char *downloaddir, 
	int assumeok, 
	char *ignorepattern) 
{
    btStream *infostr;
    btString *announce;
    btList *announcelist;
    btInteger *size;
    struct btstrbuf strbuf;
    btString *hashdata;
    btInteger *piecelen;
    int npieces;
    int i, j, dlid;
    btDownload *dl;
    btInteger *isprivate;
    
    printf("Dir: %s\n", downloaddir );

    /* Allocate the new download */
    for(dlid=0; dlid<ctx->downloadcount; dlid++) {
	if (!ctx->downloads[dlid]) break;
    }


    if(dlid==ctx->downloadcount) {
	btDownload **l=btrealloc(ctx->downloads, sizeof(btDownload*)*++ctx->downloadcount);
	if(l==NULL) {
	    --ctx->downloadcount;
	    return -ENOMEM;
	}
	ctx->downloads=l;
    }

    dl=btcalloc(1, sizeof(btDownload));
    if(!dl) return -ENOMEM;

    /* load the metadata file */
    if (benc_get_object( bts, &dl->md)) {
      btfree(dl);
      return -EINVAL;
    }

    /* calculate infohash */
    infostr = bts_create_strstream( BTS_OUTPUT);
    benc_put_object( infostr, btObject_val( dl->md, "info"));
    strbuf = bts_get_buf( infostr);
    SHA1( strbuf.buf, strbuf.len, dl->infohash);
    bts_destroy( infostr);

    /* copy out url */
    announcelist = BTLIST( btObject_val( dl->md, "announce-list"));
    if (announcelist) {
        for (i = 0; i < announcelist->len; i++) {
	    btObject *o = btList_index( announcelist, i);
	    btString *a;
	    if (o->t == BT_LIST) {
	        btList *l = BTLIST( o);
	    	for (j = 0; j < l->len; j++) {
		    a = BTSTRING( btList_index( l, j));
		    addurl( dl, a, i, ignorepattern);
		}
	    } else if (o->t == BT_STRING) {
	        a = BTSTRING( o);
		addurl( dl, a, i, ignorepattern);
	    } else {
      		return -EINVAL;
	    }
	}
	shuffle( dl);
    } else {
	announce = BTSTRING( btObject_val( dl->md, "announce"));
	if (addurl( dl, announce, 0, NULL))
      		return -EINVAL;
    } 
	
    // set private flag
    dl->privateflag = 0;
    isprivate = BTINTEGER(btObject_val( dl->md, "info/private"));
    if (isprivate) dl->privateflag = isprivate->ival;	
	
    /* set up the fileset and */
    /* calculate download size */
    dl->fileset.tsize = 0;
    size = BTINTEGER( btObject_val( dl->md, "info/length"));
    hashdata = BTSTRING( btObject_val( dl->md, "info/pieces"));
    piecelen = BTINTEGER( btObject_val( dl->md, "info/piece length"));
    npieces = hashdata->len / SHA_DIGEST_LENGTH;
    btFileSet_create( &dl->fileset, npieces, (int)piecelen->ival, hashdata->buf);
    kBitSet_create( &dl->requested, npieces);
    kBitSet_create( &dl->interested, npieces);
    for (i=0; i<npieces; i++) {
	bs_set( &dl->interested, i);
    }

    if (size) {
	/* single file mode */
	btString *file = BTSTRING( btObject_val( dl->md, "info/name"));
	dl->fileset.tsize=size->ival;
	char *unixPath = btmalloc(strlen(downloaddir)+file->len+2);
	sprintf(unixPath, "%s/%s", downloaddir, file->buf);
	btFileSet_addfile( &dl->fileset, unixPath, dl->fileset.tsize);
        btfree( unixPath );
    } else {
	/* directory mode */
	btList *files;
	btFileSet *fs = &dl->fileset;
	btString *dir;

	dir = BTSTRING( btObject_val( dl->md, "info/name"));
	files = BTLIST( btObject_val( dl->md, "info/files"));
	if (!files) return -EINVAL;
	for (i=0; i<files->len; i++) {
	    btInteger *fsize;
	    btList *filepath;
	    kStringBuffer path;

	    /* get file size */
	    fsize = BTINTEGER( btObject_val( files->list[i], "length"));
	    dl->fileset.tsize += fsize->ival;

	    /* get file path */
	    kStringBuffer_create( &path);
	    sbcat(&path, downloaddir, strlen(downloaddir));
	    sbputc( &path, '/');
	    sbcat( &path, dir->buf, dir->len);

	    filepath = BTLIST( btObject_val( files->list[i], "path"));

	    for (j=0; j<filepath->len; j++) {
	        btString *el = BTSTRING( filepath->list[j]);
		sbputc( &path, '/');
		sbcat( &path, el->buf, el->len);
	    }

	    /* add the file */
	    btFileSet_addfile( fs, path.buf, fsize->ival);

	    /* clean up */
	    kStringBuffer_finit( &path);
	}
    }
    dl->fileset.left = dl->fileset.tsize;
    
    ctx->downloads[dlid]=dl;
    return dlid;
}

int ctx_removetorrent(btContext *ctx, unsigned download)
{
    int i = download, j;
    btPeerset *pset;
    
    // remove from context
    ctx_closedownload(ctx, download);
    
    //Deincrement downloadcount and shift other downloads
    ctx->downloadcount--;
    ctx->downloads[download] = NULL;
    
    while (i<ctx->downloadcount)
    {   
        ctx->downloads[i] = ctx->downloads[i+1];
        pset = &ctx->downloads[i]->peerset;
        for (j=0; j<pset->len; j++)
        {
            pset->peer[j]->download = i;
        }
        i++; 
    }
    
    btDownload **l=btrealloc(ctx->downloads, sizeof(btDownload*)*ctx->downloadcount);
    if(l==NULL) 
    {
        return -ENOMEM;
    }
    ctx->downloads=l;
    
    return 0;   
    
}

#pragma mark -
#pragma mark Fastresume

int ctx_readfastresume(btDownload *dl, kBitSet *partialData, const char* basedir)
{
    char *resumename, *torrentname;
    FILE *resumestream;    
    int res, code = -1;
    //build resume data filename - use infohash eventually
    torrentname = ctx_stringfrommd(dl->md, "info/name");
    if(!torrentname) return -1;
    resumename = strprintf("%s/.torrent/%s.resume", basedir, torrentname);
    if ((resumestream = fopen(resumename,"r")) != NULL)
    {  
        //set up temp variables
        int bytes = (dl->fileset.completed.nbits+7)/8;
        char *encodedcomplete = btmalloc(bytes*2+1);
        unsigned char *decodedcomplete = btmalloc(bytes);
        // read file and decode
        fscanf(resumestream, "%s", encodedcomplete);
        res = ctx_hexdecode(decodedcomplete, bytes, encodedcomplete);
        if (!res)
        {
            // setup bitset based on fast resume data
            memcpy(partialData->bits, decodedcomplete, bytes);
        }

        btfree(decodedcomplete);
        btfree(encodedcomplete);

         // delete old fast-resume data
        if (fclose(resumestream) == 0)
        {
	
            remove(resumename);
	    code = 0;
        } 
    }
    btfree(torrentname );
    btfree(resumename);
    return code;
}

int ctx_hexdecode(unsigned char *digest, int len, char *buf) 
{
    int i;
    const char *cpos = buf;
    for (i=0; i<len; i++) 
    {
        unsigned int hexval;
	int nextchar;
	if (sscanf(cpos, "%02x%n", &hexval, &nextchar) < 1) 
	{
	    return -1;
	}
	digest[i] = (unsigned char)hexval;
	cpos += nextchar;
    }
    if (*cpos != 0 && *cpos != '\n' && *cpos != '\r') return -2;
    return 0;
}

int ctx_writefastresume(btDownload *dl, const char* basedir)
{
    // write resume
    FILE *resumestream;
    int i, err = -1;
    char *torrentname;
    
    torrentname = ctx_stringfrommd(dl->md, "info/name");
    if(!torrentname) return -1;
    char* resumename = strprintf("%s/.torrent/%s.resume", basedir, torrentname);
    if(!makeDirs( resumename ) )
    {
      if ((resumestream = fopen(resumename,"w")) != NULL)
      {  
        // hex encode completion data
        int bytes = (dl->fileset.completed.nbits+7)/8;
        char encodedcomplete[bytes*2+1], tempchar[3];
        for (i=0; i < bytes; i++)
        {
            if (i == 0) {
		sprintf(encodedcomplete, "%02x", 
		    (unsigned char)dl->fileset.completed.bits[i]);
	    } else {
                sprintf(tempchar, "%02x", 
                        (unsigned char)dl->fileset.completed.bits[i]);
                strcat(encodedcomplete, tempchar);
            }
        }
        fprintf(resumestream, "%s", encodedcomplete);
        fclose(resumestream);
        err = 0;
      } else {
        err = -1;
      }
    }
    btfree( torrentname );
    btfree(resumename);
    return err;
}

int ctx_deletefastresume(btDownload *dl, const char* basedir)
{
    int res = -1;
    char *torrentname = ctx_stringfrommd(dl->md, "info/name");
    if(!torrentname) return res;
    char *resumename = strprintf("%s/.torrent/%s.resume", basedir, torrentname);
    if (remove(resumename)) 
    {
         res = 0;
    }
    btfree( torrentname );
    btfree( resumename );
    return res;
}

#pragma mark -
#pragma mark Hash checking

int ctx_hashpartialdata(btFileSet *templateFileSet, kBitSet *writeCache)
{
    int i, nextpiece, igood;
    //hash
    igood = i = 0;
    while (i<writeCache->nbits) 
    {
        nextpiece = (ctx_checkhashforpiece(templateFileSet, writeCache, i));
        i += (nextpiece > 0 ?nextpiece : 1);
        if (nextpiece == 0) igood++;

        if (i%10==0 || i > writeCache->nbits-5) 
        {
            printf("\r%d of %d completed (%d ok)", i, writeCache->nbits, igood);
            fflush(stdout);
        }
        
    }
    return igood;
}

int ctx_checkhashforpiece(btFileSet *templateFileSet, kBitSet *writeCache, int i)
{
    int ok, offset=-1;
    ok = seg_review(templateFileSet, writeCache, i);
    
    if (ok < 0) 
    {
        if (errno == ENOENT) 
        {
            int ni;
            btFile *f = seg_findFile(templateFileSet, i);
            if (!f) 
            {
                printf("couldn't find block %d\n", i);
                return 0;
            }
            ni = (int)((f->start + f->len) / templateFileSet->blocksize);
            if (ni > i) 
            {
                offset = ni;
#if 1
                printf("Skipping %d blocks at offset %d\n", offset, i);
#endif
            }
        }
    }
    return (ok < 0 ? offset : 0);
}

int ctx_writehashtodownload(btDownload *dl, kBitSet *partialData)
/* Writes partial hash data to download
* Only part of hash checking that directly accesses context, ie needs lock */
{
    int igood=0, i;
    
    for (i=0; i < dl->fileset.npieces; i++)
    {
        if (bs_isSet(partialData, i))
        {
            igood++;
            dl->fileset.left -= seg_piecelen( &dl->fileset, i);
            bs_set(&dl->fileset.completed, i);
            bs_set(&dl->requested, i);
            bs_clr(&dl->interested, i);
        }
    }
    
    printf("\n");
    printf("Total good pieces %d (%d%%)\n", igood, igood * 100 / dl->fileset.npieces);
    printf("Total archive size %lld\n", dl->fileset.tsize);
    bs_dump( "completed", &dl->fileset.completed);
    return igood;   
}

#pragma mark -
#pragma mark Utilities

char *ctx_stringfrommd(btObject *md, char *mdkey)
{
    btString *mdstring;
    mdstring = BTSTRING(btObject_val(md, mdkey));
    char *returnstring = btmalloc(mdstring->len+1);
    sprintf(returnstring, "%s", mdstring->buf);
    return returnstring;
}


