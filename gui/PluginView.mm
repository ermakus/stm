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

#import "PluginView.h"
#import "CrashReportSender.h"
#import "FSItem.h"

#define CRASH_REPORTER_URL [NSURL URLWithString:@"http://cydia.ermak.us/crashreport"]
#define BUY_URL [NSURL URLWithString:@"http://cydia.ermak.us/buy.html"]

@class DOMElement;

int static registered = false;

@interface PluginView (Internal)
- (id)initPlugin:(NSDictionary *)arguments;
- (void)loadData:(NSDictionary *)arguments;
- (void)initGUI;
- (void)updateGUI;
- (void)showError;
@end

void CppListener::notify(const char* msg) {
    NSLog(@"DEBUG: %s", msg);
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
    delegate.errorMessage = [NSString stringWithUTF8String:msg];
    [pool release];
    [delegate performSelectorOnMainThread:@selector(updateGUI) withObject:nil waitUntilDone:false];
}

void CppListener::error(const char* msg) {
    NSLog(@"ERROR: %%s", msg );
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
    delegate.errorMessage = [NSString stringWithUTF8String:msg];
    [pool release];
    [delegate performSelectorOnMainThread:@selector(showError) withObject:nil waitUntilDone:true];
}
         
@implementation PluginView

@synthesize container;
@synthesize documentURL;
@synthesize documentData;
@synthesize errorMessage;
@synthesize status;
@synthesize control;
@synthesize activity;
@synthesize progress;
@synthesize header;
@synthesize torrent;
@synthesize tabs;
@synthesize files;
@synthesize reg;
@synthesize browser;
@synthesize trackers;

- (id)initWithFrame:(CGRect)frame {
    if ((self = [super initWithFrame:frame])) 
    {
        [[CrashReportSender sharedCrashReportSender] sendCrashReportToURL:CRASH_REPORTER_URL delegate:nil activateFeedback:YES];
        self.torrent = ITorrent::create( new CppListener( self ) ); 
        self.frame = frame;
	lastState = !self.torrent->isRunning();
        [self initGUI];	
    }
    return self;
}

- (void) dealloc {
	NSLog(@"Torrent Plugin: dealloc");
	[super dealloc];
}

+ (UIView *)plugInViewWithArguments:(NSDictionary *)newArguments {
	NSLog(@"Torrent plugin: init %@", [newArguments description]);
        return [[[self alloc] initPlugin:newArguments] autorelease];
}

- (void)webPlugInInitialize {
       NSLog(@"Torrent Plugin: loaded");
       [[ UIApplication sharedApplication ] setIdleTimerDisabled: YES ];
}

- (void)webPlugInStart {
	NSLog(@"Torrent Plugin: start");
}

- (void)webPlugInStop {
	NSLog(@"Torrent Plugin: stop");
}

- (void)webPlugInDestroy {
	NSLog(@"Torrent Plugin: destroy");
        ITorrent::release( self.torrent );
        self.torrent = nil;
}

- (void)webPlugInSetIsSelected:(BOOL)isSelected {
}

- (id)objectForWebScript {
    return self;
}

- (void)webPlugInMainResourceDidReceiveData:(NSData *)data {
	if (self.documentData) {
		NSLog(@"Torrent Plugin: additional data arrived, appending to existing data");
		[self.documentData appendData:data];
	} else {
		NSLog(@"Torrent Plugin: initial data arrived");
		self.documentData = [NSMutableData dataWithData:data]; 
	}
}

- (void)webPlugInMainResourceDidReceiveResponse:(NSURLResponse *)response {
}


- (void)webPlugInMainResourceDidFinishLoading {
    NSLog(@"Torrent Plugin: finish loading, data length %u", [self.documentData length]);
    self.torrent->init( [self.documentData mutableBytes], [self.documentData length] );
}


- (void)webPlugInMainResourceDidFailWithError:(NSError *)error {
    NSLog(@"Torrent View Plugin: Error: %@", error);
}

@end

@implementation PluginView (Internal)

- (id)initPlugin:(NSDictionary *) arguments {
    if (self = [super init])
         [self loadData:arguments];


    return self;
}

- (void)loadData:(NSDictionary *) arguments {

	self.container = [arguments objectForKey:@"WebPlugInContainer"];
 
	self.documentURL = [NSURL URLWithString:[arguments valueForKeyPath:@"WebPlugInAttributesKey.src"]];

	id pluginShouldLoad = [arguments objectForKey:@"WebPlugInShouldLoadMainResourceKey"];
	if (![pluginShouldLoad boolValue]) {
		// if the key is present and tells us not to load the data, this
		// method should not continue. Instead, the webPlugInMainResourceDidReceiveData:
		// method gets the data already loaded.
		NSLog(@"Torrent Plugin: plugin should not load data");
		return;
	}

	NSLog(@"Torrent Plugin: WebPlugInShouldLoadMainResourceKey is YES");

	if (!documentURL) {
		self.status.text = @"Unable to load data, no URL";
		return;
	}

	self.documentData = [NSData dataWithContentsOfURL:documentURL];
	if (!documentData) {
		self.status.text = [NSString stringWithFormat:@"Unable to load XML data from %@", documentURL];
		return;
	}
}

- (void)initGUI {
    [self setNeedsLayout];

    self.backgroundColor = [UIColor blackColor];
    self.status = [[[UILabel alloc] initWithFrame:CGRectMake(0,0,0,0)] autorelease];
    self.status.text = @"Torrent Downloader";
    self.status.textColor = [UIColor whiteColor];
    self.status.backgroundColor = [UIColor clearColor];
    [self.status setNeedsDisplay];
    [self addSubview:self.status];

    self.control = [[UIButton buttonWithType:UIButtonTypeRoundedRect] autorelease]; 
    [self.control setTitle:@"Download" forState:UIControlStateNormal];
    [self.control addTarget:self action:@selector(controlButtonClick) forControlEvents:UIControlEventTouchUpInside];
    [self addSubview:self.control];

    self.activity = [[[UIActivityIndicatorView alloc]initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleGray] autorelease];
    [self addSubview:self.activity];

    self.progress = [[[UIProgressView alloc] initWithProgressViewStyle:UIProgressViewStyleBar] autorelease];
    [self.progress setProgress: 0.0f];
    [self addSubview:self.progress];

    self.tabs      = [[UITabBarController alloc] init];
    self.files     = [[[FilesViewController alloc] initWithArgs:self.torrent] autorelease]; 
    self.trackers  = [[[TrackersViewController alloc] initWithArgs:self.torrent] autorelease];
    self.browser   = [[[BrowserViewController alloc] initWithArgs:self.torrent] autorelease]; 
    self.reg       = [[[RegisterViewController alloc] init] autorelease]; 
    self.tabs.viewControllers = [NSArray arrayWithObjects:self.files, self.trackers, self.browser];
    self.tabs.selectedIndex = 0;
    [self addSubview:self.tabs.view];
    
    self.header = [CAGradientLayer layer];
    self.header.colors =
		[NSArray arrayWithObjects:
			(id)[UIColor colorWithRed:0.0 green:0.0 blue:0.0 alpha:1.0].CGColor,
			(id)[UIColor colorWithRed:0.5 green:0.5 blue:0.5 alpha:1.0].CGColor,
		nil];
    [self.layer insertSublayer:self.header atIndex:0];
}
 
-(void)layoutSubviews {
    CGRect frame = self.frame; //[UIScreen mainScreen].applicationFrame;
    CGSize size = frame.size;
    self.progress.frame = CGRectMake(50, 30, size.width - 150, 10);
    self.control.frame = CGRectMake(size.width - 95, 5, 90, 40);
    self.activity.frame = CGRectMake(10,10,30,30);
    self.status.frame = CGRectMake(50,5,size.width-150,25);
    self.reg.view.frame = CGRectMake((size.width-250)/2,(size.height-250)/2,250,250);
    self.tabs.view.frame = CGRectMake(0,50,size.width,size.height-50);
    self.header.frame = CGRectMake(0,0,size.width,50);
}

-(void)controlButtonClick {

    if( !self.torrent->isRunning()  ) {
        	[NSThread detachNewThreadSelector:@selector(download) toTarget:self withObject:nil];
        	[self.activity startAnimating];
		self.browser.lock = YES;
     } else {
     	self.torrent->stop();
     }
}

- (void)showError {
        UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Error" message:self.errorMessage delegate:nil cancelButtonTitle:@"Ok" otherButtonTitles:nil, nil];
	[alert show];
	[alert release];
}

- (void)showPopup {
        BOOL fileExists = [[NSFileManager defaultManager] fileExistsAtPath:@"/var/mobile/Library/nazdorovie"];
        if( fileExists ) return;
	[self addSubview:self.reg.view];
}

- (void)removePopup {
        [self.reg.view removeFromSuperview];
}

- (void)updateGUI {
 
    self.status.text = self.errorMessage;

    if( self.torrent == nil ) return;
 
    if( lastState != self.torrent->isRunning() ) {

	lastState = self.torrent->isRunning();

    	if( lastState )
    	{
		[self showPopup];
        	[self.control setTitle:@"Stop" forState:UIControlStateNormal];
    	}
    	else
    	{
		[self removePopup];
    		if( self.torrent->getRunningState() == STATE_COMPLETE  ) 
		{
        		[self.control setTitle:@"Seed" forState:UIControlStateNormal];
		}
		else
		{
        		[self.control setTitle:@"Start" forState:UIControlStateNormal];
		}
        	[self.activity stopAnimating];
		self.browser.lock = NO;
 	}
	[self.browser refresh];
    }
 
    [self.progress setProgress:(self.torrent->getProgress() / 100.0)];
    [self.files.tableView reloadData];
    [self.trackers.tableView reloadData];
}

-(void)download {
       self.torrent->run();
}

@end
