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

