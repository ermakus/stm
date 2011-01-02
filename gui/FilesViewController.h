#import <UIKit/UIKit.h>
#include "../Torrent.h"

@interface FilesViewController : UITableViewController {
	ITorrent* torrent;
}
       
- (id)initWithArgs:(ITorrent*)tor;

@property (nonatomic) ITorrent *torrent;

@end
