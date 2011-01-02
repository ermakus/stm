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
#import "FSItem.h"
#include "../Torrent.h"
#include "../Settings.h"

@interface BrowserViewController : UITableViewController {
	FSItem *fsItem;
        ITorrent* torrent;
	BOOL locked;
}


@property(nonatomic) ITorrent* torrent;
@property(nonatomic) BOOL lock;
@property(nonatomic, retain) FSItem *fsItem;

- (void) refresh;

@end
