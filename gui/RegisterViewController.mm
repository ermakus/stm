#import "RegisterViewController.h"
#import <UIKit/UIKit.h>
#include "../Settings.h"

@implementation RegisterViewController
    
static NSString * REGISTER_URL  = @"http://itorrent.ermak.us/banner";

- (void)loadView {
    [super loadView];
    self.title = @"Home";
    self.tabBarItem.image = load_image(@"register.png");
    webView = [[UIWebView alloc] initWithFrame:CGRectMake(0,0,100,100)];
    [webView setScalesPageToFit:NO];
    [webView setUserInteractionEnabled:YES];
    webView.autoresizingMask = (UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight);
    webView.delegate = self;
    webView.alpha = 0.0;
    firstLoad = YES;
    self.view = webView;
}

-(void)viewWillAppear:(BOOL)animated { 
	[super viewWillAppear:animated];
        NSURL *url = [NSURL URLWithString:REGISTER_URL];
        NSURLRequest *requestObj = [NSURLRequest requestWithURL:url];
        [webView loadRequest:requestObj];
}

- (void)viewDidLoad {
    [super viewDidLoad];
}

- (void)webViewDidFinishLoad:(UIWebView *)webView {
  if (firstLoad) {
    firstLoad = NO;
    [UIView beginAnimations:@"web" context:nil];
    webView.alpha = 1.0;
    [UIView commitAnimations];
    }
}

- (BOOL)webView:(UIWebView *)webView2 shouldStartLoadWithRequest:(NSURLRequest *)request navigationType:(UIWebViewNavigationType)navigationType {
	
	NSLog(@"Click on %@", [[request URL] absoluteString] ); 

	NSString *scheme = [[request URL] scheme];
	if( [scheme isEqualToString:@"itorrent"] ) {
/*		NSString* serial = [[request URL] host];
		NSString* device = [[UIDevice currentDevice] uniqueIdentifier];
		if( [serial isEqualToString:device] ) {
		   	UIAlertView *alert = [[[UIAlertView alloc] initWithTitle:@"Thank You!" message:@"Your copy is now registered." delegate:self cancelButtonTitle:@"OK" otherButtonTitles:nil] autorelease];
    			[alert show];
		} else {
			UIAlertView *alert = [[[UIAlertView alloc] initWithTitle:@"Error" message:@"Serial number is invalid" delegate:self cancelButtonTitle:@"OK" otherButtonTitles:nil] autorelease];
    			[alert show];
		}
*/
 	        [self.view removeFromSuperview];
		return NO;
	}
	
	if (navigationType == UIWebViewNavigationTypeLinkClicked) {
                [[UIApplication sharedApplication] openURL:[request URL]];
    		return NO;
  	}
      
	return YES;
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation
{
}

- (void)didReceiveMemoryWarning {
	[super didReceiveMemoryWarning]; // Releases the view if it doesn't have a superview
}


- (void)dealloc {
	[super dealloc];
}

@end
