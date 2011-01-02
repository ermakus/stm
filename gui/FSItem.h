//
//  FSItem.h
//  FSWalker
//
//  Created by Nicolas Seriot on 17.08.08.
//  Copyright 2008 Sen:te. All rights reserved.
//

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
