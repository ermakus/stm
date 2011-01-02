#import <UIKit/UIKit.h>
#include "../Torrent.h"

@interface TrackersViewController : UITableViewController {
	ITorrent* torrent;
}
       
- (id)initWithArgs:(ITorrent*)tor;

@property (nonatomic) ITorrent *torrent;

@end
