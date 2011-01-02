//
//  RootViewController.h
//  FSWalker
//
//  Created by Nicolas Seriot on 17.08.08.
//  Copyright Sen:te 2008. All rights reserved.
//

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
