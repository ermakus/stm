#ifndef __UTIL__H
#define __UTIL__H
#include <errno.h>
#include <sys/types.h>
#include <inttypes.h>

#define bt_assert(expr) (void)((expr) || die(__FILE__, __LINE__, #expr, 0))

#ifdef __cplusplus
extern "C" {
#endif

#if !WIN32
typedef int64_t _int64;
typedef int32_t  _int32;
typedef int16_t _int16;
typedef int8_t _int8;
#endif
_int64 htonll( _int64 src);
_int64 ntohll( _int64 src);

/* SHIFT Macros */
#define SHIFT_INT64(ptr,nboll,llval) \
    (nboll=htonll(llval), memcpy(ptr,&nboll, sizeof(_int64)), ptr+=sizeof(_int64))
#define SHIFT_INT32(ptr,nbo,ival) \
    (nbo=htonl(ival), memcpy(ptr,&nbo,sizeof(int32_t)), ptr+=sizeof(int32_t))
#define SHIFT_INT16(ptr,nbo,hval) \
    (nbo=htonl(hval), memcpy(ptr,&nbo,sizeof(int16_t)), ptr+=sizeof(int16_t))
#define SHIFT_BYTE(ptr,ival) ((*ptr = ival), ptr++)

/* UNSHIFT Macros */
#define UNSHIFT_INT64(ptr,nboll,llval) \
    (memcpy(&nboll,ptr,sizeof(_int64)), llval=ntohll(nboll), ptr+=sizeof(_int64))
#define UNSHIFT_INT32(ptr,nbo,ival) \
    (memcpy(&nbo,ptr,sizeof(int32_t)), ival=ntohl(nbo), ptr+=sizeof(int32_t))
#define UNSHIFT_INT16(ptr,nbo,hval) \
    (memcpy(&nbo,ptr,sizeof(int16_t)), hval=ntohs(nbo), ptr+=sizeof(int16_t))
#define UNSHIFT_BYTE(ptr,ival) (ival = (*((unsigned char *)(ptr)),ptr++)

void *btmalloc( size_t size);
void *btcalloc( size_t nmemb, size_t size);
void *btrealloc( void *buf, size_t size);
void btfree( void *buf);

char* strprintf(const char* fmt, ...);

const char *bts_strerror( int err);
void  bts_perror( int err, const char *msg);
int   die( const char *file, int line, const char *msg, int err);
int   makeDirs(const char* path);
int   openPath( const char *path, int flags);
int   cacheopen( const char *path, int flags, int mode);
void  cacheclose( void);
void  hexencode (const unsigned char *digest, int len, char *buf, int buflen);
int   hexdecode (unsigned char *digest, int len, const char *buf, int buflen);

//#define malloc(s) use_btmalloc_instead
//#define calloc(n,s) use_btcalloc_instead
//#define realloc(p,s) use_btrealloc_instead
//#undef free
//#define free(p) use_btfree_instead

void hexdump( void *buf, int buflen);

#ifdef __cplusplus
}
#endif


#endif
