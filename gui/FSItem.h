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

#import <UIKit/UIKit.h>


@interface FSItem : NSObject {
	NSDictionary *attributes;
	NSArray *children;
	NSString *path;
}


@property(retain) NSDictionary *attributes;
@property(retain) NSArray *children;
@property(retain) NSString *path;
@property(retain) NSString *filename;

@property(readonly) NSDate *modificationDate;
@property(readonly) NSString *ownerName;
@property(readonly) NSString *groupName;
@property(readonly) NSString *posixPermissions;
@property(readonly) NSDate *creationDate;
@property(readonly) NSString *fileSize;
@property(readonly) NSString *ownerAndGroup;

@property(readonly) BOOL isDirectory;
@property(readonly) BOOL isSymbolicLink;
@property(readonly) BOOL canBeFollowed;

+ (FSItem *)fsItemWithDir:(NSString *)path;
- (void) reset;

@end
