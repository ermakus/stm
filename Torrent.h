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

#ifndef CTORRENT_H
#define CTORRENT_H

#define STATE_IDLE     0
#define STATE_STOPPED  1
#define STATE_CHECKING 2
#define STATE_RUNNING  3
#define STATE_STOPPING 4
#define STATE_COMPLETE 5
#define STATE_ERROR    6

struct ITorrentListener {
   virtual void notify( const char* msg ) = 0;
   virtual void error( const char* msg ) = 0;
};

struct ITorrent {
   static ITorrent* create( ITorrentListener * listener );
   static void      release( ITorrent* torrent );

   virtual ~ITorrent() {};

   virtual void init( const void* data, size_t size) = 0;
   virtual void destroy() = 0;
   virtual void run() = 0;
   virtual void stop() = 0;

   virtual int         getRunningState() = 0;

   virtual bool        isSelected(int index) = 0;
   virtual void        setSelected(int index, bool state) = 0;
   virtual const char* getFileName(int index) = 0;
   virtual int         getFileCount() = 0;

   virtual const char* getTracker(int index) = 0;
   virtual const char* getTrackerStatus(int index) = 0;
   virtual int         getTrackerCount() = 0;


   virtual int         getProgress() = 0;
   virtual const char* getStatus() = 0;
   virtual bool        isRunning() = 0;

   virtual const char* getDir() = 0;
   virtual int         setDir(const char* szDir) = 0;


   virtual void        notify(const char*, ...)=0;
   virtual ITorrentListener& listener() = 0;
};

#endif
