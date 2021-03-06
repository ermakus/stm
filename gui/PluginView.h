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
#import <QuartzCore/QuartzCore.h>

#include "FilesViewController.h"
#include "TrackersViewController.h"
#include "RegisterViewController.h"
#include "BrowserViewController.h"

#include "../Torrent.h"

@interface PluginView : UIView
{
    class CppListener : public ITorrentListener {
    public:
        CppListener( PluginView* pv ) : delegate( pv ) {
        }
        virtual ~CppListener() {
        }

        void notify(const char* msg);
        void error(const char* msg);

        PluginView *delegate;
    };

    NSObject      *container;
    NSMutableData *documentData;
    NSURL         *documentURL;
    NSString      *errorMessage;
    // GUI controls
    UILabel       *status;
    UIButton      *control;

    UIActivityIndicatorView *activity;

    UIProgressView  *progress;
    CAGradientLayer *header;

    UITabBarController     *tabs;

    FilesViewController*    files;
    TrackersViewController* trackers;
    RegisterViewController* reg;
    BrowserViewController*  browser;

    int lastState;

    ITorrent        *torrent;

}

@property (nonatomic, retain) NSObject      *container;
@property (nonatomic, retain) NSURL         *documentURL;
@property (nonatomic, retain) NSMutableData *documentData;
@property (nonatomic, retain) NSString      *errorMessage;
@property (nonatomic, retain) UITabBarController *tabs;
@property (nonatomic, retain) FilesViewController* files;
@property (nonatomic, retain) TrackersViewController* trackers;
@property (nonatomic, retain) RegisterViewController* reg;
@property (nonatomic, retain) BrowserViewController*  browser;

@property (nonatomic) ITorrent *torrent;

@property (nonatomic, retain) UILabel                 *status;
@property (nonatomic, retain) UIButton                *control;
@property (nonatomic, retain) UIActivityIndicatorView *activity;
@property (nonatomic, retain) UIProgressView          *progress;
@property (nonatomic, retain) CAGradientLayer         *header;

@end
