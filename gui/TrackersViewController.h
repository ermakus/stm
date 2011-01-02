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
#include "../Torrent.h"

@interface TrackersViewController : UITableViewController {
	ITorrent* torrent;
}
       
- (id)initWithArgs:(ITorrent*)tor;

@property (nonatomic) ITorrent *torrent;

@end
