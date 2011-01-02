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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#if !WIN32
#   if HAVE_UNISTD_H
#      include <unistd.h>
#   endif
#   if HAVE_FCNTL_H
#      include <fcntl.h>
#   endif
#   include <sys/param.h>
#endif
#include <sys/stat.h>
#include <sys/types.h>
#if WIN32
#   include <io.h>
#   include <direct.h>
#   define MAXPATHLEN 255
#   define MKDIR(p,f) mkdir(p)
#else
#   define MKDIR(p,f) mkdir(p,f)
#endif
#include "bterror.h"
#include "util.h"

#undef malloc
#undef calloc
#undef realloc
#undef free

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif /* WITH_DMALLOC */
/* Globals */
#define FILESETSIZE 50

static struct {
    int fd;
    int options;
    const char *path;
} c_fileset[ FILESETSIZE ];

static int c_index = 0;

_int64 htonll( _int64 src) {
    _int64 dst;
    unsigned char *x = (unsigned char *)&dst;
    x[0] = (src >> 56) & 0xff;
    x[1] = (src >> 48) & 0xff;
    x[2] = (src >> 40) & 0xff;
    x[3] = (src >> 32) & 0xff;
    x[4] = (src >> 24) & 0xff;
    x[5] = (src >> 16) & 0xff;
    x[6] = (src >> 8) & 0xff;
    x[7] = (src) & 0xff;
    return dst;
}

_int64 ntohll( _int64 src) {
    _int64 dst = 0;
    unsigned char *x = (unsigned char *)&src;
    dst |= ((_int64)x[0])<<56;
    dst |= ((_int64)x[1])<<48;
    dst |= ((_int64)x[2])<<40;
    dst |= ((_int64)x[3])<<32;
    dst |= ((_int64)x[4])<<24;
    dst |= ((_int64)x[5])<<16;
    dst |= ((_int64)x[6])<<8;
    dst |= ((_int64)x[7]);
    return dst;
}

void *btcalloc( size_t nmemb, size_t size) {
    void *region = calloc( nmemb, size);
    bt_assert( region );
    return region;
}

void *btmalloc( size_t size) {
    void *region = malloc( size);
    bt_assert( region );
    return region;
}

void *btrealloc( void *ptr, size_t size) {
    void *region = realloc( ptr, size);
    bt_assert( region );
    return region;
}

void btfree( void *ptr) 
{
    free( ptr);
}

char* strprintf(const char* fmt, ...) {
   va_list ap;
   char buff[1024];
   va_start(ap, fmt);
   vsnprintf(buff, 1023, fmt, ap);
   va_end(ap);
   return strdup( buff );
}

const char *bts_strerror( int err) {
    if (err) {
	if (err >= BTERR_BASE && err < BTERR_LAST) {
	    return bterror_string[err - BTERR_BASE];
	} else {
	    return strerror(err);
	}
    } 
    return "Not an error";
}

void bts_perror( int err, const char *msg) 
{
    fprintf( stderr, "%s: [%d] %s\n", msg, err, bts_strerror(err));
}

int die(const char *file, int line, const char *msg, int err) {
    static char buf[1024];
    if (err) {
	sprintf( buf, "%s+%d: (FATAL ERROR) %s ([%d] %s)\n", 
	    file, line, msg, err, bts_strerror( err));
    } else {
	sprintf( buf, "%s+%d: (FATAL ERROR) %s\n", 
	    file, line, msg);
    }
    throw buf;
}

int cacheopen(const char *path, int options, int mode) {
    int i;
    int fd;
    int err;
    static int last_i = 0;

    if (c_fileset[last_i].path == path && c_fileset[last_i].options == options) 
    {
	return c_fileset[last_i].fd;
    }
    for (i=0; i<FILESETSIZE; i++) {
        if (path == c_fileset[i].path && options == c_fileset[i].options) {
	    last_i = i;
	    return c_fileset[i].fd;
	}
    }
    printf("Opening file: %s\n", path);
    fd = open( path, options, mode);
    if (fd < 0) return fd;
    if (c_fileset[c_index].fd > 0) {
        err = close(c_fileset[c_index].fd);
	if (err) {
	    bts_perror( errno, "Error closing file");
	    fprintf( stderr, "File '%s' may be corrupt\n", c_fileset[c_index].path);
	}
    }
    c_fileset[c_index].fd = fd;
    c_fileset[c_index].path = path;
    c_fileset[c_index].options = options;
    c_index = (c_index + 1) % FILESETSIZE; 
    return fd;
}

void cacheclose( void) {
    int err;
    int i;
    for (i=0; i<FILESETSIZE; i++) {
        if (c_fileset[c_index].fd) {
	    err = close(c_fileset[c_index].fd);
	    c_fileset[c_index].path = NULL;
	    c_fileset[c_index].options = 0;
	    c_fileset[c_index].fd = 0;
	    if (err) {
		bts_perror( errno, "Error closing file");
		fprintf( stderr, "File '%s' may be corrupt\n", c_fileset[c_index].path);
	    }
	}
    }
}


int makeDirs(const char* path)
{
    char subdir[MAXPATHLEN];
    const char *sep;
    int fd;
    int len;
    int err;
    sep = path;
    sep = strchr(sep+1, '/');
    while( sep) 
    {
	    len = sep - path;
	    memcpy( subdir, path, len);
	    subdir[len] = 0;
	    err = MKDIR(subdir, 0777);
	    if (err) {
	        if (errno == EEXIST) {
		    /* check for existing directory */
		    int mderr = errno;
		    int serr;
		    struct stat sbuf;
		    serr = stat(subdir, &sbuf);
		    if (!serr && S_ISDIR(sbuf.st_mode)) err = 0;
		    errno=mderr;
		}
		if (err) {
		    bts_perror( errno, "Error creating directory");
		    fprintf( stderr, "Directory '%s' will be missing\n", path);
		    return err;
		}
	    }
	    sep = strchr(sep+1, '/');
    }
    return 0;
}

int openPath(const char *path, int options) 
{
    char subdir[MAXPATHLEN];
    const char *sep;
    int fd;
    int len;
    int err;
    fd = cacheopen( path, options, 0666);
    if (fd < 0) {
	sep = path;

	sep = strchr(sep+1, '/');

	while( sep) {
	    len = sep - path;
	    memcpy( subdir, path, len);
	    subdir[len] = 0;
	    err = MKDIR(subdir, 0777);
	    if (err) {
	        if (errno == EEXIST) {
		    /* check for existing directory */
		    int mderr = errno;
		    int serr;
		    struct stat sbuf;
		    serr = stat(subdir, &sbuf);
		    if (!serr && S_ISDIR(sbuf.st_mode)) err = 0;
		    errno=mderr;
		}
		if (err) {
		    bts_perror( errno, "Error creating directory");
		    fprintf( stderr, "Directory '%s' will be missing\n", path);
		}
	    }
	    sep = strchr(sep+1, '/');
	}
	fd = cacheopen( path, options, 0666);
    }
    if (fd < 0) {
	bts_perror( errno, "Error creating file");
	fprintf( stderr, "File '%s' will be missing\n", path);
    }
    return fd;
}

void
hexencode (const unsigned char *digest, int len, char *buf, int buflen) {
    int i;
    for (i=0; i<len; i++) {
	snprintf(buf+3*i, buflen - 3*i, "%%%02x", digest[i]);
    }
}

int
hexdecode (unsigned char *digest, int len, const char *buf, int buflen) {
    int i;
    const char *cpos = buf;
    for (i=0; i<len; i++) {
        unsigned int hexval;
	int nextchar;
	if (sscanf( cpos, "%%%02x%n", &hexval, &nextchar) < 1) {
	    return -1;
	}
	digest[i] = (unsigned char)hexval;
        cpos += nextchar;
    }
    if (*cpos != 0 && *cpos != '\n' && *cpos != '\r') return -2;
    return 0;
}

static void putch( int c) {
    if (c < 32 || c >= 127) printf("?");
    else putchar(c);
}

void hexdump( void *xbuf, int len) {
    char *buf = (char*)xbuf;
    int addr, i,j;
    for (addr = 0; addr < len; addr+=20) {
	printf("%08x: ", addr);
	for (i = 0; i < 20; i+= 4) {
	    for (j = 0; j < 4; j++) {
		if (addr+i+j >= len) {
		    printf("..");
		} else {
		    printf("%02x", (unsigned char)buf[addr+i+j]);
		}
	    }
	    printf(" ");
	}
	printf(" |");
	for (i = 0; i < 20; i++) {
	    if (addr+i > len) {
		putch('.');
	    } else {
		putch((unsigned char)buf[addr+i+j]);
	    }
        } 
	printf("|\n");
    }
}
