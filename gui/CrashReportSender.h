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

#import <Foundation/Foundation.h>

#define kCrashReportAnalyzerStarted @"CrashReportAnalyzerStarted"			// flags if the crashlog analyzer is started. since this may crash we need to track it
#define kCrashReportActivated @"CrashReportActivated"						// flags if the crashreporter is activated at all
#define kAutomaticallySendCrashReports @"AutomaticallySendCrashReports"		// flags if the crashreporter should automatically send crashes without asking the user again

typedef enum CrashAlertType {
	CrashAlertTypeSend = 0,
	CrashAlertTypeFeedback = 1,
} CrashAlertType;

typedef enum CrashReportStatus {
	CrashReportStatusFailureVersionDiscontinued = -30,          // This app version is set to discontinued, no new crash reports accepted by the server
	CrashReportStatusFailureXMLSenderVersionNotAllowed = -21,   // XML: Sender ersion string contains not allowed characters, only alphanumberical including space and . are allowed
	CrashReportStatusFailureXMLVersionNotAllowed = -20,         // XML: Version string contains not allowed characters, only alphanumberical including space and . are allowed
	CrashReportStatusFailureSQLAddSymbolicateTodo = -18,        // SQL for adding a symoblicate todo entry in the database failed
	CrashReportStatusFailureSQLAddCrashlog = -17,               // SQL for adding crash log in the database failed
	CrashReportStatusFailureSQLAddVersion = -16,                // SQL for adding a new version in the database failed
	CrashReportStatusFailureSQLCheckVersionExists = -15,        // SQL for checking if the version is already added in the database failed
	CrashReportStatusFailureSQLAddPattern = -14,                // SQL for creating a new pattern for this bug and set amount of occurrances to 1 in the database failed
	CrashReportStatusFailureSQLCheckBugfixStatus = -13,         // SQL for checking the status of the bugfix version in the database failed
	CrashReportStatusFailureSQLUpdatePatternOccurances = -12,   // SQL for updating the occurances of this pattern in the database failed
	CrashReportStatusFailureSQLFindKnownPatterns = -11,         // SQL for getting all the known bug patterns for the current app version in the database failed
	CrashReportStatusFailureSQLSearchAppName = -10,             // SQL for finding the bundle identifier in the database failed
	CrashReportStatusFailureInvalidPostData = -3,               // the post request didn't contain valid data
	CrashReportStatusFailureInvalidIncomingData = -2,           // incoming data may not be added, because e.g. bundle identifier wasn't found
	CrashReportStatusFailureDatabaseNotAvailable = -1,          // database cannot be accessed, check hostname, username, password and database name settings in config.php
	CrashReportStatusUnknown = 0,
	CrashReportStatusAssigned = 1,
	CrashReportStatusSubmitted = 2,
	CrashReportStatusAvailable = 3,
} CrashReportStatus;

// This protocol is used to send the image updates
@protocol CrashReportSenderDelegate <NSObject>

@optional

-(NSString *) crashReportUserID;					// Return the userid the crashreport should contain, empty by default
-(NSString *) crashReportContact;					// Return the contact value (e.g. email) the crashreport should contain, empty by default
-(NSString *) crashReportDescription;				// Return the description the crashreport should contain, empty by default

-(void) connectionOpened;							// Invoked when the internet connection is started, to let the app enable the activity indicator
-(void) connectionClosed;							// Invoked when the internet connection is closed, to let the app disable the activity indicator

@end

@interface CrashReportSender : NSObject {
	NSTimer *_submitTimer;
	
	NSMutableString *_contentOfProperty;
	CrashReportStatus _serverResult;
	
	BOOL _crashReportActivated;
	BOOL _crashReportFeedbackActivated;
	
	int _crashReportAnalyzerStarted;
	NSString *_crashesDir;
	
	int _amountCrashes;
	BOOL _crashIdenticalCurrentVersion;
	
	id <CrashReportSenderDelegate> _delegate;
	
	NSMutableArray *_crashFiles;
	
	NSURL *_submissionURL;
	NSMutableData *_responseData;
	NSInteger _statusCode;
}

+ (CrashReportSender *)sharedCrashReportSender;

- (void)sendCrashReportToURL:(NSURL *)submissionURL delegate:(id <CrashReportSenderDelegate>)delegate activateFeedback:(BOOL)activateFeedback;


@end
