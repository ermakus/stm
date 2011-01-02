#import "FSItem.h"

@implementation FSItem

@synthesize path;
@synthesize filename;
@synthesize attributes;
@synthesize children;
@synthesize path;
@dynamic filename;
@dynamic modificationDate;
@dynamic ownerName;
@dynamic groupName;
@dynamic posixPermissions;
@dynamic creationDate;
@dynamic fileSize;
@dynamic ownerAndGroup;
@dynamic isSymbolicLink;

- (BOOL)canBeFollowed {

//	if([[self posixPermissions] intValue] == 0) return NO;
	
	if(self.isDirectory) return YES;
	
	if(self.isSymbolicLink) {
		NSFileManager *fm = [NSFileManager defaultManager];
		NSError *e = nil;
		NSString *destPath = [fm destinationOfSymbolicLinkAtPath:self.path error:&e];
		if(e || !destPath) return NO;
		return [fm contentsOfDirectoryAtPath:destPath error:nil] != nil;
	}
	
	return NO;
}

- (NSArray *)children {
	if(children == nil) {
		NSArray *childrenFilenames = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:self.path error:nil];
		NSMutableArray *a = [[NSMutableArray alloc] init];
		FSItem *child = [FSItem fsItemWithDir:[path stringByDeletingLastPathComponent]];
		[a addObject:child];
		for(NSString *fn in childrenFilenames) {
			FSItem *child = [FSItem fsItemWithDir:[path stringByAppendingPathComponent:fn]];
			[a addObject:child];
		}
		self.children = a;
		[a release];
	}
	return children;
}

- (void)dealloc {
	[attributes release];
	[children release];
	[path release];
	[super dealloc];
}

- (BOOL)isDirectory {
	return [[attributes objectForKey:NSFileType] isEqualToString:NSFileTypeDirectory];
}

- (BOOL)isSymbolicLink {
	return [[attributes objectForKey:NSFileType] isEqualToString:NSFileTypeSymbolicLink];
}

- (NSString *)filename {
	return [path lastPathComponent];
}

- (void) reset {
	if( children != nil )
	{
		[children release];
		children = nil;
	}
}

+ (FSItem *)fsItemWithDir:(NSString *)dir {
	FSItem *i = [[FSItem alloc] init];
	i.path = dir;
	i.attributes = [[NSFileManager defaultManager] fileAttributesAtPath:i.path traverseLink:NO];
	return [i autorelease];
}

- (NSDate *)modificationDate {
	return [self.attributes objectForKey:NSFileModificationDate];
}

- (NSString *)ownerName {
	return [self.attributes objectForKey:NSFileOwnerAccountName];
}

- (NSString *)groupName {
	return [self.attributes objectForKey:NSFileGroupOwnerAccountName];
}

- (NSString *)posixPermissions {
	NSNumber *n = [self.attributes objectForKey:NSFilePosixPermissions];
	return [NSString stringWithFormat:@"%O", [n unsignedLongValue]];
}

- (NSDate *)creationDate {
	return [self.attributes objectForKey:NSFileCreationDate];
}

- (NSString *)fileSize {
	return [self.attributes objectForKey:NSFileSize];
}

- (NSString *)ownerAndGroup {
	return [NSString stringWithFormat:@"%@ %@", self.ownerName, self.groupName];
}

@end
