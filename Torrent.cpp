#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "bts.h"
#include "benc.h"
#include "random.h"
#include "peer.h"
#include "stream.h"
#include "segmenter.h"
#include "context.h"
#include "bterror.h"
#include "types.h"
#include "udpproto.h"
#include "peerexchange.h"

#include "Torrent.h"

#define READABLE 1
#define EXECUTE  2

#define DOWNLOAD_DIR "/var/mobile/Library/Downloads" 

class CTorrent : public ITorrent {

   ITorrentListener* m_pListener;
   int               m_nState;
   btContext         m_Context;
   char              m_szDownloadDir[512];
   char              m_strStatus[512];
   bool              m_bRunning;
   bool              m_bStopping;
   const void*       m_pMetaData;
   size_t            m_nMetaSize;

   int  clientRun(int cs, int cmd);
   int  clientWrite(int cs );
   void clientError(const char *activity, int cs);


public:
   CTorrent(ITorrentListener* l) 
	: m_pListener( l )
        , m_nState( STATE_IDLE )
        , m_bRunning( false )
        , m_bStopping( false )
	, m_pMetaData( NULL )
   {
         btContext_create( &m_Context, 1.0, NULL );
	 strcpy( m_szDownloadDir, DOWNLOAD_DIR );
	*m_strStatus = 0;
   }

   virtual ~CTorrent() {
         btContext_destroy( &m_Context );
   }

   void notify(const char* format, ... );

   void error(const char* format, ... );

   void init( const void* data, size_t size );

   void destroy();

   bool check();

   void run();

   int getRunningState() {
      return m_nState;
   }

   const char* getStatus() {
       return m_strStatus;
   }

   int getProgress();

   void updateStatus();

   void stop() {
       m_bStopping = true;
       m_nState = STATE_STOPPING;
   }

   bool isRunning() {
      return m_bRunning;
   }

   bool isSelected(int index) {
	return true;
   }

   void setSelected(int index, bool state) {
   }

   const char* getFileName(int index);

   int         getFileCount();

   const char* getTracker(int index) {
    	if( m_nState == STATE_IDLE) return "Nope";
    	return m_Context.downloads[0]->url[ index ];
    }

   const char* getTrackerStatus(int index) {
    	if( m_nState == STATE_IDLE) return "Nope";
    	return m_Context.downloads[0]->fault[ index ];
   }

   int  getTrackerCount() {
    	if( m_nState == STATE_IDLE) return 0;
    	return m_Context.downloads[0]->nurl;
   }

   const char* getDir() {
	return m_szDownloadDir;
   }

   int setDir(const char* szDir) {
	if( m_pMetaData ) destroy();
  	strncpy( m_szDownloadDir, szDir , 511 );
	if( m_pMetaData ) init( m_pMetaData, m_nMetaSize );
   }

   ITorrentListener& listener() {
       return *m_pListener;
   }
};

ITorrent* ITorrent::create(ITorrentListener* listener) {
    return new CTorrent( listener );
}

void ITorrent::release(ITorrent* torrent) {
    torrent->destroy();
    delete torrent;
}


void CTorrent::notify(const char* message, ...) 
{
   va_list ap;
   va_start(ap, message);
   vsnprintf(m_strStatus, 511, message, ap);
   va_end(ap);
   m_pListener->notify( m_strStatus );
}

void CTorrent::error(const char* message, ...) 
{
   va_list ap;
   va_start(ap, message);
   vsnprintf(m_strStatus, 511, message, ap);
   va_end(ap);
   m_pListener->error( m_strStatus );
}


void CTorrent::init( const void* data, size_t size ) {
       int res;
       int optseed = 0;
       char* optignore = NULL;
       m_pMetaData = data;
       m_nMetaSize = size;
       btStream *io;
       io = bts_create_strstream( BTS_OUTPUT );
       bt_assert( io );
       res = bts_write( io, (char*)data, size );
       bt_assert( res == 0 );
       res = ctx_loadfile( io, &m_Context, (char*)m_szDownloadDir, optseed, optignore);
       bts_destroy( io );
       if (res < 0) {
            notify("Bad torrent");
            return;
       }
       m_nState = STATE_STOPPED;
       notify("Torrent loaded");
}

void CTorrent::destroy() {
   if( isRunning() ) stop();
   int i = 0;
   while( isRunning() ) {
      usleep( 100 * 1000 ); 
      i++; 
      if( i > 70 ) {
         error("Can't stop downloader");
	 break;
      }
   }
   ctx_closedownload( &m_Context, 0 );
}

const char* CTorrent::getFileName(int index) {
    if( m_nState == STATE_IDLE) return "Nope";
    return &m_Context.downloads[0]->fileset.file[index]->path[strlen(m_szDownloadDir) + 1 ];
}

int  CTorrent::getFileCount() {
    if( m_nState == STATE_IDLE) return 0;
    return m_Context.downloads[0]->fileset.nfiles;
}
   
int CTorrent::getProgress()  {
    if( m_nState == STATE_IDLE) return 0;
    int complt = bs_countBits( &m_Context.downloads[0]->fileset.completed );
    return ( complt * 100 / m_Context.downloads[0]->fileset.npieces );
}

void CTorrent::updateStatus() 
{
    int i;
    int npeers = 0;
    float rtime, ttime;
    float rbytes, tbytes;
    float rrate = 0, trate = 0;

    int dl = 0;
    btContext *ctx = &m_Context;
    btPeerset* pset =  &ctx->downloads[dl]->peerset;

    for (i=0; i<pset->len; i++) {
	btPeer *p = pset->peer[i];
	if (p == NULL) continue;
	if (p->state == PEER_GOOD) npeers++;
	if (!p->remote.choked && p->local.interested) {
	    rtime = (float)rate_timer( &p->remote, time(NULL));
	    rbytes = (float)p->ios.read_count;
	    rrate += rbytes / rtime;
	}

	if (!p->local.choked && p->remote.interested) {
	    ttime = (float)rate_timer( &p->local, time(NULL));
	    tbytes = (float)p->ios.write_count;
	    trate += tbytes / ttime;
	}
    }

    char rate[16];
    if (rrate >= 1000000) {
	sprintf( rate, "%.1fMbs", rrate / 1000000);
    } else if (rrate >= 1000) {
	sprintf( rate, "%.0fkbs", rrate / 1000);
    } else {
	sprintf( rate, "%.0fbps", rrate);
    }
 
    notify("%d%% (%d peers %s)",  getProgress(), npeers, rate );
}



/*
 * Return 0 - no more work
 * Return 1 - more work to do
 * Return <0 - error
 */
int CTorrent::clientRun( int cs, int cmd) {

    int res = 0;
    int err;

    btContext *ctx = &m_Context;  

    btPeer *p = ctx->sockpeer[cs];

    /* check for new messages */
    if (p->state == PEER_ERROR) return -1;
    do {
        err = peer_recv_message( ctx, p);
    } while (err == 1);
#if 0
    printf("Still have %d bytes buffered\n", kStream_iqlen(&p->ios));
#endif
    if (err < 0 && p->ios.error != EAGAIN) return -1;

    /* do queue processing */
    if(p->download!=INT_MAX)
        err = peer_process_queue( &ctx->downloads[p->download]->fileset, p);
    if (err < 0 && p->ios.error != EAGAIN) return -1;
    if (err == 1 && cmd==EXECUTE) res = 1;
    return res;
}

int CTorrent::clientWrite( int cs ) 
{
    /* writable player socket */
    int res = 0;
    int err;
    btContext *ctx = &m_Context;  
    btPeer *p = ctx->sockpeer[cs];
    if (p->state == PEER_INIT) {
        /* unconnected socket */
        err = peer_connect_complete( ctx, p);
        if (err<0 && p->ios.error == EAGAIN) {
            return 1;   /* keep waiting */
        }
        if (err) return -1;

        /* connection complete, add to read set */
        ctx_setevents( ctx, cs, POLLIN);
        peer_send_handshake( ctx, p);
        peer_send_bitfield( ctx, p);
        p->state = PEER_OUTGOING;
    } else {
        /* connected/good/etc. socket */
        err = kStream_flush( &p->ios);
        if (err < 0 && p->ios.error != EAGAIN) {
            return -1;
        }
    }

    if (kStream_oqlen( &p->ios) > 0) {
        res = 1;
    }
    return res;
}

void CTorrent::clientError(const char *activity, int cs) 
{
    int i, j;
    char buf[128];
    btContext *ctx = &m_Context;

    const char *err = "connection closed";
    btPeer *p = ctx->sockpeer[cs];
    if (p->ios.error != 0) err = bts_strerror( p->ios.error);

    /* errors on a socket */
    //sprintf( buf, "%d: Peer %s shutdown %s (%s)", cs, inet_ntoa(ctx->sockpeer[cs]->sa.sin_addr), activity, err);
    //m_pListener->notify( buf );

    int dl=p->download;
    peer_shutdown( ctx, p, err);
    /* find where peer is */
    if(dl!=INT_MAX) {
        btPeerset *pset=&ctx->downloads[dl]->peerset;
        for (i=0; i < pset->len; i++) {
            if (pset->peer[i] == p) {
                j = i;
                pset->peer[i] = NULL;
                break;
            }
        }
        /* shift down the rest */
        for (i=j; i < pset->len-1; i++) {
            pset->peer[i] = pset->peer[i+1];
        }
        pset->len--;
    }
    btfree(p);
    close(cs);

    /* clear the execute bit if it was set */
    if (ctx->x_set[cs]) {
        ctx->xsock--;
        ctx->x_set[cs] = 0;
    }
}


bool CTorrent::check()
{
       int i, nextpiece, igood, dl = 0;
       kBitSet partialData;
       btContext *ctx = &m_Context;

       m_nState = STATE_CHECKING;
       notify("Integrity checking");
 
       kBitSet_create(&partialData, ctx->downloads[dl]->fileset.npieces);
       igood = ctx_readfastresume(ctx->downloads[dl], &partialData, (char*) m_szDownloadDir );
       if (igood < 0)
       {
         igood = i = 0;
         while (i<partialData.nbits)
         {
            if( m_bStopping ) return false;
            nextpiece = (ctx_checkhashforpiece(&ctx->downloads[dl]->fileset, &partialData, i));
            i += (nextpiece > 0 ?nextpiece : 1);
            if (nextpiece == 0) igood++;
            if (i%10==0 || i > partialData.nbits-5)
            {
                notify("Checking: %d%%", 100 * i / partialData.nbits);
            }

        }
      }
      ctx_writehashtodownload(ctx->downloads[dl], &partialData);
      kBitSet_finit(&partialData);
      return true;
}

void CTorrent::run()
{

  m_bRunning = true;
  m_bStopping = false;

  try 
  {

   if( !check() ) { 
      m_nState = STATE_STOPPED;
      m_bStopping = m_bRunning = false;
      notify("Checking canceled");
      return;
    }

    btContext* ctx = &m_Context;
    int dl = 0;

    m_pListener->notify("Starting server..");
    ctx_startserver(ctx);

    m_pListener->notify("Registering...");
    ctx_register(ctx, dl);
      
    m_nState = STATE_RUNNING;
    m_pListener->notify("Download started");

  int ttv;
  int tv_slow = 1; /*ms*/
  int tv_fast = 0; /*ms*/
  btPeer *peer;
  int cs;
  struct sockaddr csin;
  int err;

  time_t choke_timer;
  time_t report_timer;
  time_t now;

  int i;

  time( &now );
  choke_timer = report_timer = now;

  while( !m_bStopping )
  {
	int readerr;
	int writeerr;
	int execerr;
	int pollerr;
	socklen_t sa_len;

        /*
	 * Select a socket or time out
	 */
	if (ctx->xsock) {
	    ttv = tv_fast;
	} else {
	    ttv = tv_slow;
	}

        err = poll( ctx->status, ctx->nstatus, ttv);

	if (err < 0) { 
		bts_perror(errno, "poll");
		m_nState = STATE_ERROR;
                break;
	}

	time(&now);

	for (cs=0; cs < SOCKID_MAX; cs++) {
	    /* loop through executable sockets */
            if (ctx->x_set[ cs]) {
		btPeer *p = ctx->sockpeer[cs];
		execerr = clientRun( cs, EXECUTE);
		if (execerr == 0) {
		    if ( ctx->x_set[ cs]) {
			ctx->x_set[ cs] = 0;
			ctx->xsock--;
		    }
		}
		
		if ( kStream_oqlen( &p->ios)) {
		    /* data is waiting on the output buffer */
                    ctx_setevents( ctx, cs, POLLOUT);
		}

		if (execerr < 0) {
		    clientError("execute", cs);
		} 
	    } 
	} 


        for (i=0; i<ctx->nstatus; i++) {
	    /* for each poll event */
	    cs = ctx->status[i].fd;

	    readerr=0;
	    writeerr=0;
	    execerr=0;
	    pollerr=0;

	    if (CTX_STATUS_REVENTS( ctx, i) & POLLIN) 
            {
		bt_assert( ctx->status[i].events & POLLIN);
		/* readable */
	        if (cs == ctx->ss) {
		    /* service socket */
		    sa_len = sizeof(csin);
		    cs = accept( ctx->ss, &csin, &sa_len);
		    if (cs < 0) {
			bts_perror( errno, "accept");
		    } else {
                        peer_answer( ctx, cs);
		    } 
		} else if (cs == ctx->udpsock) {
		    int err = udp_ready( ctx);
		    if (err) {
			printf("Error %d processing UDP packet.\n", err);
		    }
		} else {
		    btPeer *p = ctx->sockpeer[ cs];
		    readerr = clientRun( cs, READABLE);
		    if (readerr == 1) {
			/* more to read */
			if (!ctx->x_set[cs]) {
			    ctx->x_set[cs] = 1;
			    ctx->xsock++;
			}
		    }
		    if ( kStream_oqlen( &p->ios)) {
		        /* data is waiting on the output buffer */
			ctx_setevents( ctx, cs, POLLOUT);
		    }
		}
	    } /* if */

	    if (CTX_STATUS_REVENTS( ctx, i) & POLLOUT) {
		writeerr = clientWrite( cs );
		if (writeerr == 0) {
		    /* output drained */
		    ctx_clrevents( ctx, cs, POLLOUT);
		    if (!ctx->x_set[ cs]) {
			/* output buffer is empty, check for more work */
			ctx->x_set[ cs] = 1;
			ctx->xsock++;
		    }
		}
	    } /* if */

	    if (CTX_STATUS_REVENTS( ctx, i) & (POLLERR | POLLHUP | POLLNVAL)) 
	    {
	        int events = CTX_STATUS_REVENTS( ctx, i);
		if (events & POLLHUP) {
		    ctx->sockpeer[cs]->ios.error = BTERR_POLLHUP;
		} else if (events & POLLERR) {
		    ctx->sockpeer[cs]->ios.error = BTERR_POLLERR;
		} else if (events & POLLNVAL) {
		    ctx->sockpeer[cs]->ios.error = BTERR_POLLNVAL;
		}
		pollerr = -1;
	    }

	    if (readerr < 0 || writeerr < 0 || execerr < 0 || pollerr < 0) 
            {
	        const char *act = NULL;
		if (readerr<0) act = "read";
		if (writeerr<0) act = "write";
		if (execerr<0) act = "execute";
		if (pollerr<0) act = "poll";
		clientError( act, cs);
	    } 

	    peer = ctx->sockpeer[cs];

	    if (  peer && !peer->remote.choked && peer->local.interested && !peer->local.snubbed && now - peer->lastreceived > 120) 
	    {
		peer->local.snubbed = 1;
	    }
	    
	    if (peer && peer->pex_supported > 0 && peer->pex_timer > 0 && now - peer->pex_timer >= 60)
	    {
		sendPeerExchange(ctx->downloads[peer->download], peer);
	    }
	    
	} 

        if (ctx->downloads[dl]->reregister_interval != 0 &&  now - ctx->downloads[dl]->reregister_timer > ctx->downloads[dl]->reregister_interval) 
        {
                notify("Updating...");
		ctx->downloads[dl]->reregister_timer = now;
		ctx_reregister( ctx, dl);
        }

	if (now - report_timer > 0) 
        {
	    int complt = bs_countBits( &ctx->downloads[dl]->fileset.completed);
       	    if ((complt == ctx->downloads[dl]->fileset.npieces) && !ctx->downloads[dl]->complete) 
            {
		notify("Completing...");
                ctx_complete (ctx, dl);
                m_nState = STATE_COMPLETE;
                break;
            }
            report_timer=now;
	    updateStatus();
	}

	if (now - choke_timer > 30) {
	    /* recalculate favorite peers */
	    choke_timer=now;
	    peer_favorites( ctx, &ctx->downloads[dl]->peerset);
	}
    }

   } 
   catch(const char* ex) 
   {
       m_pListener->error( ex );
       m_nState = STATE_ERROR;
   }

   notify("Disconnecting...");

   int dl = 0;

   ctx_writefastresume(m_Context.downloads[dl], (char*) m_szDownloadDir );
   ctx_shutdown( &m_Context, dl );
   cacheclose();

   m_bRunning = false;

   if( m_bStopping ) {
      notify("Download stopped");
      m_bStopping = false;
      m_nState = STATE_STOPPED;
   } 
   else
   if( m_nState == STATE_COMPLETE ) {
      notify("Download complete");
   }
   else
   if( m_nState == STATE_ERROR ) {
      notify("Download failed");
   }
   else
       m_nState = STATE_STOPPED;

}


