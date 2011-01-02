#include "httpget.h"

#import <UIKit/UIKit.h>

int http_get( const char *urlStr, char* buffer, size_t * pSize ) {

	NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
	NSURL* url = [[NSURL URLWithString:[NSString stringWithUTF8String:urlStr]] autorelease];
	NSURLRequest * urlRequest = [[NSURLRequest requestWithURL:url] autorelease];
	NSURLResponse * response = nil;
	NSError * error = nil;
	NSData * data = [[NSURLConnection sendSynchronousRequest:urlRequest returningResponse:&response error:&error] autorelease];
	int err = 0;
	if( error ) {
		err = error.code;
	}
	if( data ) {
		size_t size = [data length];
		if( size > *pSize ) size = *pSize; else *pSize = size;
		memcpy( buffer, data.bytes, size );
	}
    	[pool release];
	return  err;
}

