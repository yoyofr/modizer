//
//  MiniPlayerVC.m
//  modizer
//
//  Created by Yohann Magnien on 12/04/2021.
//

#import "ModizerConstants.h"
#import "MiniPlayerVC.h"

#import "DetailViewControllerIphone.h"

@interface MiniPlayerVC ()

@end

@implementation MiniPlayerVC

@synthesize detailVC;
@synthesize coverImg;
@synthesize mpview;

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
    mpview=[[MiniPlayerView alloc] init];
    mpview.translatesAutoresizingMaskIntoConstraints=false;
    [self.view addSubview:mpview];
    
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view attribute:NSLayoutAttributeCenterX relatedBy:NSLayoutRelationEqual toItem:mpview attribute:NSLayoutAttributeCenterX multiplier:1.0 constant:0]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view attribute:NSLayoutAttributeCenterY relatedBy:NSLayoutRelationEqual toItem:mpview attribute:NSLayoutAttributeCenterY multiplier:1.0 constant:0]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view attribute:NSLayoutAttributeWidth relatedBy:NSLayoutRelationEqual toItem:mpview attribute:NSLayoutAttributeWidth multiplier:1.0 constant:0]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view attribute:NSLayoutAttributeHeight relatedBy:NSLayoutRelationEqual toItem:mpview attribute:NSLayoutAttributeHeight multiplier:1.0 constant:0]];
    
    mpview.userInteractionEnabled=true;
    
    labelTime_mode=0;
}

- (void)traitCollectionDidChange:(UITraitCollection *)previousTraitCollection {
    darkMode=false;
    if (SYSTEM_VERSION_GREATER_THAN_OR_EQUAL_TO(@"12.0")) {
        if (@available(iOS 12.0, *)) {
            if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
        }
    }
    if (darkMode) [mpview setBackgroundColor:[UIColor colorWithRed:0.1 green:0.1 blue:0.1 alpha:1]];
    else [mpview setBackgroundColor:[UIColor colorWithRed:0.9 green:0.9 blue:0.9 alpha:1]];
    
    if (darkMode) {
        labelMain.textColor = [UIColor lightGrayColor];
        labelSub.textColor = [UIColor lightGrayColor];
        labelTime.textColor = [UIColor lightGrayColor];
        labelPlaylist.textColor = [UIColor lightGrayColor];
        //btnPlay
        [btnPlay setColor:[UIColor colorWithRed:0.1 green:0.1 blue:0.1 alpha:1]];
        [btnPause setColor:[UIColor colorWithRed:0.1 green:0.1 blue:0.1 alpha:1]];
        //btnPause
    } else {
        labelMain.textColor = [UIColor darkGrayColor];
        labelSub.textColor = [UIColor darkGrayColor];
        labelTime.textColor = [UIColor darkGrayColor];
        labelPlaylist.textColor = [UIColor darkGrayColor];
        [btnPlay setColor:[UIColor colorWithRed:0.9 green:0.9 blue:0.9 alpha:1]];
        [btnPause setColor:[UIColor colorWithRed:0.9 green:0.9 blue:0.9 alpha:1]];
    }
}

-(void) pushedGoPlayer {
    [self.parentViewController performSelector:@selector(goPlayer)];
}

-(void) swipeRight {
    [detailVC playPrevSub];
}

-(void) swipeLeft {
    [detailVC playNextSub];
}

-(void) pushedPlay {
    [detailVC playPushed:nil];
}

-(void) pushedPause {
    [detailVC pausePushed:nil];
}

-(void) pushedTime {
    labelTime_mode++;
    if (labelTime_mode>=3) labelTime_mode=0;
}

-(void) refreshTime {
    int l=[detailVC.mplayer getSongLength]/1000;
    int t=[detailVC.mplayer getCurrentTime]/1000;
    switch (labelTime_mode) {
        case 0:
            if (l>0) labelTime.text=[NSString stringWithFormat:@"-%d:%.2d",(l-t)/60,(l-t)%60];
            else labelTime.text=[NSString stringWithFormat:@"--:--"];
            break;
        case 1:
            labelTime.text=[NSString stringWithFormat:@"%d:%.2d",(t)/60,(t)%60];
            break;
        case 2:
            if (l>0) labelTime.text=[NSString stringWithFormat:@"%d:%.2d",(l)/60,(l)%60];
            else labelTime.text=[NSString stringWithFormat:@"--:--"];
            break;
    }
    
    if ([detailVC.mplayer isPlaying]) {
        if ([detailVC.mplayer isPaused]) {
            btnPlay.hidden=false;
            btnPause.hidden=true;
        } else {
            btnPlay.hidden=true;
            btnPause.hidden=false;
        }
    } else {
        btnPlay.hidden=false;
        btnPause.hidden=true;
    }
}

-(void) refreshCoverLabels {
    if ([detailVC.mplayer isPlaying]) {
        if ([detailVC.mplayer isPaused]) {
            btnPlay.hidden=false;
            btnPause.hidden=true;
        } else {
            btnPlay.hidden=true;
            btnPause.hidden=false;
        }
        labelMain.text=[detailVC.mplayer getModFileTitle];
        if ([detailVC.mplayer isArchive]&&([detailVC.mplayer getArcEntriesCnt]>1)) {
            //archive with multiple files
            if (detailVC.mplayer.mod_subsongs>1) {
                //and also subsongs
                labelSub.text=[NSString stringWithFormat:@"(%d/%d)(%d/%d) %@",[detailVC.mplayer getArcIndex]+1,[detailVC.mplayer getArcEntriesCnt],detailVC.mplayer.mod_currentsub,detailVC.mplayer.mod_maxsub-detailVC.mplayer.mod_minsub,[detailVC.mplayer getModName]];
            } else {
                //no subsong
                labelSub.text=[NSString stringWithFormat:@"(%d/%d) %@",[detailVC.mplayer getArcIndex]+1,[detailVC.mplayer getArcEntriesCnt],[detailVC.mplayer getModName]];
            }
        } else {
            if (detailVC.mplayer.mod_subsongs>1) {
                //subsongs
                labelSub.text=[NSString stringWithFormat:@"(%d/%d) %@",detailVC.mplayer.mod_currentsub-detailVC.mplayer.mod_minsub+1,detailVC.mplayer.mod_subsongs,[detailVC.mplayer getModName]];
            } else {
                //no subsong
                labelSub.text=[NSString stringWithFormat:@"%@",[detailVC.mplayer getModName]];
            }
        }
        labelPlaylist.text=[NSString stringWithFormat:@"%d/%d",detailVC.mPlaylist_pos+1,detailVC.mPlaylist_size];
        //labelTime.text=[NSString stringWithFormat:@"%d:%.2d",t/60,t%60];
        [self refreshTime];
    } else {
        labelMain.text=@"Initializing";
        labelSub.text=@"...";
        labelTime.text=@"0:00";
        labelPlaylist.text=@"...";
        btnPause.hidden=true;
        btnPlay.hidden=false;
    }
    
    [coverView setImage:coverImg];
}

- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator {
    labelMain.frame=CGRectMake(50,0,(size.width-50-100),24);
    labelSub.frame=CGRectMake(50,24,(size.width-50-100),24);
    labelTime.frame=CGRectMake(size.width-100,0,50,24);
    labelPlaylist.frame=CGRectMake(size.width-100,24,50,24);
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
        
    darkMode=false;
    if (SYSTEM_VERSION_GREATER_THAN_OR_EQUAL_TO(@"12.0")) {
        if (@available(iOS 12.0, *)) {
            if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
        }
    }
    //Background color
    if (darkMode) [mpview setBackgroundColor:[UIColor colorWithRed:0.1 green:0.1 blue:0.1 alpha:1]];
    else [mpview setBackgroundColor:[UIColor colorWithRed:0.9 green:0.9 blue:0.9 alpha:1]];
    
    //Cover artwork
    coverView=[[UIImageView alloc] init];
    coverView.frame=CGRectMake(0,0,48,48);
    [mpview addSubview:coverView];
    
    
    //Labels
    int ww=self.view.frame.size.width;
    labelMain=[[CBAutoScrollLabel alloc] init];
    labelMain.frame=CGRectMake(50,0,(ww-50-100),24);
    [labelMain setFont:[UIFont systemFontOfSize:12]];
    if (darkMode) labelMain.textColor = [UIColor whiteColor];
    else labelMain.textColor = [UIColor blackColor];
    labelMain.labelSpacing = 35; // distance between start and end labels
    labelMain.pauseInterval = 3.7; // seconds of pause before scrolling starts again
    labelMain.scrollSpeed = 30; // pixels per second
    labelMain.textAlignment = NSTextAlignmentLeft; // centers text when no auto-scrolling is applied
    labelMain.fadeLength = 12.f; // length of the left and right edge fade, 0 to disable
    
    labelSub=[[CBAutoScrollLabel alloc] init];
    labelSub.frame=CGRectMake(50,24,(ww-50-100),24);
    [labelSub setFont:[UIFont systemFontOfSize:10]];
    if (darkMode) labelSub.textColor = [UIColor whiteColor];
    else labelSub.textColor = [UIColor blackColor];
    labelSub.labelSpacing = 35; // distance between start and end labels
    labelSub.pauseInterval = 3.7; // seconds of pause before scrolling starts again
    labelSub.scrollSpeed = 30; // pixels per second
    labelSub.textAlignment = NSTextAlignmentLeft; // centers text when no auto-scrolling is applied
    labelSub.fadeLength = 12.f; // length of the left and right edge fade, 0 to disable
    
    labelTime=[[UILabel alloc] init];
    labelTime.frame=CGRectMake(ww-100,0,50,24);
    [labelTime setFont:[UIFont systemFontOfSize:11]];
    if (darkMode) labelTime.textColor = [UIColor whiteColor];
    else labelTime.textColor = [UIColor blackColor];
    labelTime.textAlignment = NSTextAlignmentCenter; // centers text when no auto-scrolling is applied
    labelTime.userInteractionEnabled=true;
    
    labelPlaylist=[[UILabel alloc] init];
    labelPlaylist.frame=CGRectMake(ww-100,24,50,24);
    [labelPlaylist setFont:[UIFont systemFontOfSize:10]];
    if (darkMode) labelPlaylist.textColor = [UIColor whiteColor];
    else labelPlaylist.textColor = [UIColor blackColor];
    labelPlaylist.textAlignment = NSTextAlignmentCenter; // centers text when no auto-scrolling is applied
    labelPlaylist.userInteractionEnabled=true;
    
    [self refreshCoverLabels];
    
    
    [mpview addSubview:labelMain];
    [mpview addSubview:labelSub];
    [mpview addSubview:labelTime];
    [mpview addSubview:labelPlaylist];
    
    //view to cover cover image + labels and recognize tap + swipe gesture
    gestureAreaView=[[UIView alloc] init];
    gestureAreaView.userInteractionEnabled=true;
    [gestureAreaView setBackgroundColor:[UIColor clearColor]];
    [mpview addSubview:gestureAreaView];
    // new gesture recognizer
    UITapGestureRecognizer *tapGesture = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(pushedGoPlayer)];
    // Set required taps and number of touches
    [tapGesture setNumberOfTapsRequired:1];
    [tapGesture setNumberOfTouchesRequired:1];
    // Add the gesture to the view
    [gestureAreaView addGestureRecognizer:tapGesture];
    
    tapGesture = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(pushedTime)];
    // Set required taps and number of touches
    [tapGesture setNumberOfTapsRequired:1];
    [tapGesture setNumberOfTouchesRequired:1];
    // Add the gesture to the view
    [labelTime addGestureRecognizer:tapGesture];
    
    tapGesture = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(pushedTime)];
    // Set required taps and number of touches
    [tapGesture setNumberOfTapsRequired:1];
    [tapGesture setNumberOfTouchesRequired:1];
    // Add the gesture to the view
    [labelPlaylist addGestureRecognizer:tapGesture];
    
    // new gesture recognizer
    UISwipeGestureRecognizer *swipeGesture = [[UISwipeGestureRecognizer alloc] initWithTarget:self action:@selector(swipeLeft)];
    // Set required direction and number of touches
    [swipeGesture setDirection:UISwipeGestureRecognizerDirectionLeft];
    [swipeGesture setNumberOfTouchesRequired:1];
    // Add the gesture to the view
    [gestureAreaView addGestureRecognizer:swipeGesture];
    
    swipeGesture = [[UISwipeGestureRecognizer alloc] initWithTarget:self action:@selector(swipeRight)];
    // Set required direction and number of touches
    [swipeGesture setDirection:UISwipeGestureRecognizerDirectionRight];
    [swipeGesture setNumberOfTouchesRequired:1];
    // Add the gesture to the view
    [gestureAreaView addGestureRecognizer:swipeGesture];
    gestureAreaView.translatesAutoresizingMaskIntoConstraints=false;
    
    [mpview addConstraint:[NSLayoutConstraint constraintWithItem:gestureAreaView attribute:NSLayoutAttributeHeight relatedBy:NSLayoutRelationEqual toItem:mpview attribute:NSLayoutAttributeHeight multiplier:1.0 constant:0]];
    [mpview addConstraint:[NSLayoutConstraint constraintWithItem:gestureAreaView attribute:NSLayoutAttributeWidth relatedBy:NSLayoutRelationEqual toItem:mpview attribute:NSLayoutAttributeWidth multiplier:1.0 constant:-100]];
    
    //Buttons@
    btnPlay=[[BButton alloc] initWithFrame:CGRectMake(0,1,46,46) type:BButtonTypeGray style:BButtonStyleBootstrapV3];
    [btnPlay addAwesomeIcon:FAIconPlay beforeTitle:YES];
    [btnPlay addTarget:self action:@selector(pushedPlay) forControlEvents:UIControlEventTouchUpInside];
    btnPlay.userInteractionEnabled=true;
    btnPlay.translatesAutoresizingMaskIntoConstraints = false;
    btnPlay.hidden=false;
    [btnPlay setColor:mpview.backgroundColor];
    [btnPlay setButtonCornerRadius:[NSNumber numberWithFloat:0.0f]];
    [mpview addSubview:btnPlay];
    
    btnPause=[[BButton alloc] initWithFrame:CGRectMake(0,1,46,46) type:BButtonTypeGray style:BButtonStyleBootstrapV3];
    [btnPause addAwesomeIcon:FAIconPause beforeTitle:YES];
    [btnPause addTarget:self action:@selector(pushedPause) forControlEvents:UIControlEventTouchUpInside];
    btnPause.translatesAutoresizingMaskIntoConstraints = false;
    btnPause.hidden=true;
    [btnPause setButtonCornerRadius:[NSNumber numberWithFloat:0.0f]];
    [btnPause setColor:mpview.backgroundColor];
    [mpview addSubview:btnPause];
    
    NSDictionary *views = NSDictionaryOfVariableBindings(btnPlay);
    // width constraint
    [mpview addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:[btnPlay(46)]" options:0 metrics:nil views:views]];
    // height constraint
    [mpview addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"V:[btnPlay(46)]" options:0 metrics:nil views:views]];
    // center align
    [mpview addConstraint:[NSLayoutConstraint constraintWithItem:btnPlay attribute:NSLayoutAttributeRight relatedBy:NSLayoutRelationEqual toItem:mpview attribute:NSLayoutAttributeRight multiplier:1.0 constant:0]];
    [mpview addConstraint:[NSLayoutConstraint constraintWithItem:btnPlay attribute:NSLayoutAttributeBottom relatedBy:NSLayoutRelationEqual toItem:mpview attribute:NSLayoutAttributeBottom multiplier:1.0 constant:-1]];
    
    views = NSDictionaryOfVariableBindings(btnPause);
    // width constraint
    [mpview addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:[btnPause(46)]" options:0 metrics:nil views:views]];
    // height constraint
    [mpview addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"V:[btnPause(46)]" options:0 metrics:nil views:views]];
    // center align
    [mpview addConstraint:[NSLayoutConstraint constraintWithItem:btnPause attribute:NSLayoutAttributeRight relatedBy:NSLayoutRelationEqual toItem:mpview attribute:NSLayoutAttributeRight multiplier:1.0 constant:0]];
    [mpview addConstraint:[NSLayoutConstraint constraintWithItem:btnPause attribute:NSLayoutAttributeBottom relatedBy:NSLayoutRelationEqual toItem:mpview attribute:NSLayoutAttributeBottom multiplier:1.0 constant:-1]];
    
    //Timer
    repeatingTimer = [NSTimer scheduledTimerWithTimeInterval: 0.10f target:self selector:@selector(refreshTime) userInfo:nil repeats: YES]; //10 times/second
    
    
}

-(void)viewDidDisappear:(BOOL)animated {
    [coverView removeFromSuperview];coverView=nil;
    [labelMain removeFromSuperview];labelMain=nil;
    [labelTime removeFromSuperview];labelTime=nil;
    [labelPlaylist removeFromSuperview];labelPlaylist=nil;
    [gestureAreaView removeFromSuperview];gestureAreaView=nil;
    [repeatingTimer invalidate];
    repeatingTimer = nil; // ensures we never invalidate an already invalid Timer
}

/*
#pragma mark - Navigation

// In a storyboard-based application, you will often want to do a little preparation before navigation
- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender {
    // Get the new view controller using [segue destinationViewController].
    // Pass the selected object to the new view controller.
}
*/

@end
