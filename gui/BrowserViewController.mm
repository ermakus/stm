//
//  RootViewController.m
//  FSWalker
//
//  Created by Nicolas Seriot on 17.08.08.
//  Copyright Sen:te 2008. All rights reserved.
//

#import "BrowserViewController.h"

static NSString* CFG_DIR_KEY = @"TorrentDir";

@implementation BrowserViewController

@dynamic fsItem;
@synthesize torrent;
@synthesize lock;

- (id)initWithArgs:(ITorrent*)tor {
   if( self = [super init] ) {
	self.torrent = tor;
   	self.tabBarItem.image = load_image(@"home.png");
    	self.title = @"Save To";
	self.lock = NO;

        NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
        NSString* dir  = [prefs stringForKey:CFG_DIR_KEY];
        if( dir ) self.torrent->setDir( [dir UTF8String] );
   }
   return self;
}

- (void)loadView {
   [super loadView];
//   self.tabBarItem.image = load_image(@"home.png");
}

- (void)viewDidLoad {
    [super viewDidLoad];
    self.fsItem = [FSItem fsItemWithDir:[NSString stringWithUTF8String: self.torrent->getDir()]];
}

- (void)setLock:(BOOL)state {
	if( locked == state ) return;
	locked = state;
	if( locked ) {
	//	UIImageView *icon = [[UIImageView alloc] initWithImage:load_image(@"lock.png")];
	//	icon.tag = 1001;
	//	[self.tableView addSubview:icon];
	} else {
	//	UIView * icon = [self.tableView viewWithTag:1001];
	//	[icon removeFromSuperview];
	//	[icon release];
	}
}
	
- (void)setFsItem:(FSItem *)item {
	if(item != fsItem) {
		[item retain];
		[fsItem release];
		fsItem = item;
		[self.tableView reloadData];
	} else {
	}
}

- (FSItem *)fsItem {
	return fsItem;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	return 1;
}


- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	return [fsItem.children count];
}


- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    static NSString *CellIdentifier = @"CellBrowse";

    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:CellIdentifier];
    if (cell == nil) {
        cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CellIdentifier] autorelease];
        cell.selectionStyle = UITableViewCellSelectionStyleBlue;
    }
	
    FSItem *child = [fsItem.children objectAtIndex:indexPath.row];

    if(child.canBeFollowed) cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
                       else cell.accessoryType = UITableViewCellAccessoryNone;

    if( indexPath.row == 0 ) cell.textLabel.text = @"[Up level]"; else
    			     cell.textLabel.text = child.filename;
 
    return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {

	FSItem *child = [fsItem.children objectAtIndex:indexPath.row];
	
	if([child.posixPermissions intValue] == 0) return;

	if(locked) {
		UIAlertView *alert = [[UIAlertView alloc] initWithTitle: @"Busy"
                             message: @"Please stop downloading first"
                             delegate: self
                             cancelButtonTitle: @"OK"
                             otherButtonTitles: nil];
    		[alert show];
    		[alert release];
		return;
	}
		
        const char* path = [child.path UTF8String]; 

	if(child.canBeFollowed) {

		NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
    		[prefs setObject:child.path forKey:CFG_DIR_KEY];
		[prefs synchronize];
		

		self.fsItem = child;
		self.torrent->setDir( path );
		self.torrent->notify( "Save to: %s", path ); 
	} else {
		self.torrent->notify("File: %s", path);	
	}
}

- (void)refresh {
	[fsItem reset];
        [self.tableView reloadData];
}

- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath {
		
	if (editingStyle == UITableViewCellEditingStyleDelete) {
		FSItem *child = [fsItem.children objectAtIndex:indexPath.row];
		NSFileManager *fileManager = [NSFileManager defaultManager];
		[fileManager removeItemAtPath:child.path error:NULL];
		[fsItem reset];
	        [self.tableView reloadData];
	}	
	if (editingStyle == UITableViewCellEditingStyleInsert) {
		// Create a new instance of the appropriate class, insert it into the array, and add a new row to the table view
	}	
}

- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath {
        if( indexPath.row == 0 ) return NO;
	return YES;
}


/*
 Override if you support rearranging the list
- (void)tableView:(UITableView *)tableView moveRowAtIndexPath:(NSIndexPath *)fromIndexPath toIndexPath:(NSIndexPath *)toIndexPath {
}
*/


/*
 Override if you support conditional rearranging of the list
- (BOOL)tableView:(UITableView *)tableView canMoveRowAtIndexPath:(NSIndexPath *)indexPath {
	// Return NO if you do not want the item to be re-orderable.
	return YES;
}
 */ 


- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
}

- (void)viewDidAppear:(BOOL)animated {
	[super viewDidAppear:animated];
}

- (void)viewWillDisappear:(BOOL)animated {
}

- (void)viewDidDisappear:(BOOL)animated {
}


- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
	// Return YES for supported orientations
	return YES;//(interfaceOrientation == UIInterfaceOrientationPortrait);
}


- (void)didReceiveMemoryWarning {
	[super didReceiveMemoryWarning]; // Releases the view if it doesn't have a superview
	// Release anything that's not essential, such as cached data
}


- (void)dealloc {
	[fsItem release];	
	[super dealloc];
}


@end

