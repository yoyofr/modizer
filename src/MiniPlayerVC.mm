//
//  MiniPlayerVC.m
//  modizer
//
//  Created by Yohann Magnien on 12/04/2021.
//

int gesture_swipe_min_vel;
int gesture_swipe_min_trans;
int gesture_move_sub_min_trans;
int gesture_move_file_min_trans;
#define SWIPE_MIN_VELOCITY 500
#define SWIPE_MIN_TRANSLATION 50
#define TRIGGER_SUB_MIN_TRANSLATION 100
#define TRIGGER_ENTRY_MIN_TRANSLATION 150
#define TRIGGER_SUB_MIN_WIDTH_RATIO 0.25f
#define TRIGGER_ENTRY_MIN_WIDTH_RATIO 0.5f

#define LABEL_PREVNEXTENTRY_OFFSET 100

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

- (void)refreshViewDarkmode {
    if (darkMode) [mpview setBackgroundColor:[UIColor colorWithRed:0.1 green:0.1 blue:0.1 alpha:1]];
    else [mpview setBackgroundColor:[UIColor colorWithRed:0.9 green:0.9 blue:0.9 alpha:1]];
    
    if (darkMode) {
        labelPrev.textColor = [UIColor whiteColor];
        labelNext.textColor = [UIColor whiteColor];
        labelPrevEntry.textColor = [UIColor whiteColor];
        labelNextEntry.textColor = [UIColor whiteColor];
        labelMain.textColor = [UIColor whiteColor];
        labelSub.textColor = [UIColor whiteColor];
        labelArtist.textColor = [UIColor whiteColor];
        labelTime.textColor = [UIColor whiteColor];
        labelPlaylist.textColor = [UIColor whiteColor];
        //btnPlay
        [btnPlay setColor:[UIColor colorWithRed:0.1 green:0.1 blue:0.1 alpha:1]];
        [btnPause setColor:[UIColor colorWithRed:0.1 green:0.1 blue:0.1 alpha:1]];
        //btnPause
    } else {
        labelPrev.textColor = [UIColor blackColor];
        labelNext.textColor = [UIColor blackColor];
        labelPrevEntry.textColor = [UIColor blackColor];
        labelNextEntry.textColor = [UIColor blackColor];
        labelMain.textColor = [UIColor blackColor];
        labelSub.textColor = [UIColor blackColor];
        labelArtist.textColor = [UIColor blackColor];
        labelTime.textColor = [UIColor blackColor];
        labelPlaylist.textColor = [UIColor blackColor];
        [btnPlay setColor:[UIColor colorWithRed:0.9 green:0.9 blue:0.9 alpha:1]];
        [btnPause setColor:[UIColor colorWithRed:0.9 green:0.9 blue:0.9 alpha:1]];
    }
}

- (void)traitCollectionDidChange:(UITraitCollection *)previousTraitCollection {
    darkMode=false;
    if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
        
    [self refreshViewDarkmode];
}

-(void) pushedGoPlayer {
    detailVC.view.frame=self.view.frame;
    [self.parentViewController performSelector:@selector(goPlayer)];
}

-(void) swipeRight:(bool)prevFile {
    //[self.parentViewController performSelector:@selector(showWaitingLoading)];
    if (prevFile) [detailVC playPrev];
    else [detailVC playPrevSub];
    //[self.parentViewController performSelector:@selector(hideWaiting)];
}

-(void) swipeLeft:(bool)nextFile {
    //[self.parentViewController performSelector:@selector(showWaitingLoading)];
    if (nextFile) [detailVC playNext];
    else [detailVC playNextSub];
    //[self.parentViewController performSelector:@selector(hideWaiting)];
}

-(void) pushedPlay {
    [detailVC playPushed:nil];
}

-(void) pushedPause {
    [detailVC pausePushed:nil];
}

-(void) pushedTime {
    labelTime_mode++;
    if (labelTime_mode>=2) labelTime_mode=0;
}

-(void) pushedPlaylist {
    [self.parentViewController performSelector:@selector(goCurrentPlaylist)];
}

-(void) panLabels:(UIPanGestureRecognizer*)gesture {
    static CGPoint max_velocity,min_velocity,cur_velocity;
    static int org_centerx,orgPrev_centerx,orgNext_centerx;
    int translationX;
    float alpha,alpha2;
    bool bEntryInsteadOfSub=false;
    CGPoint translation = [gesture translationInView:gesture.view];
    
    
     
    switch (gesture.state) {
        case UIGestureRecognizerStateBegan:
            max_velocity.x=0;
            org_centerx=labelMain.center.x;
            orgPrev_centerx=labelPrev.center.x;
            orgNext_centerx=labelNext.center.x;
            
            gesture_swipe_min_vel=SWIPE_MIN_VELOCITY;
            gesture_swipe_min_trans=SWIPE_MIN_TRANSLATION;
            gesture_move_sub_min_trans=MIN(gestureAreaView.frame.size.width*TRIGGER_SUB_MIN_WIDTH_RATIO,TRIGGER_SUB_MIN_TRANSLATION);
            gesture_move_file_min_trans=MIN(gestureAreaView.frame.size.width*TRIGGER_ENTRY_MIN_WIDTH_RATIO,TRIGGER_ENTRY_MIN_TRANSLATION);
            
            break;
        case UIGestureRecognizerStateChanged:
            // move label
            cur_velocity=[gesture velocityInView:gesture.view];
            if (cur_velocity.x>0) {
                if (cur_velocity.x>max_velocity.x) max_velocity.x=cur_velocity.x;
            } else {
                if (cur_velocity.x<max_velocity.x) max_velocity.x=cur_velocity.x;
            }
            
            labelMain.center = CGPointMake(labelMain.center.x + translation.x, labelMain.center.y);
            labelSub.center = CGPointMake(labelSub.center.x + translation.x, labelSub.center.y);
            labelArtist.center = CGPointMake(labelArtist.center.x + translation.x, labelArtist.center.y);
            labelPrev.center= CGPointMake(labelPrev.center.x + translation.x, labelPrev.center.y);
            labelNext.center= CGPointMake(labelNext.center.x + translation.x, labelNext.center.y);
            labelPrevEntry.center= CGPointMake(labelPrevEntry.center.x + translation.x, labelPrevEntry.center.y);
            labelNextEntry.center= CGPointMake(labelNextEntry.center.x + translation.x, labelNextEntry.center.y);
            
            translationX=org_centerx-labelMain.center.x;
            alpha=(float)translationX*1.0f/(gesture_move_sub_min_trans*2);
            if (alpha<0) alpha=-alpha;
            if (alpha>0.75f) alpha=0.75f;
            
            if ((translationX<-gesture_move_file_min_trans)||(translationX>gesture_move_file_min_trans)) {
                alpha2=1;
            } else {
                alpha2=0;
            }
            
            labelPrev.alpha=(0.25f+alpha)*(1-alpha2);
            labelNext.alpha=(0.25f+alpha)*(1-alpha2);
            labelPrevEntry.alpha=(0.25f+alpha)*alpha2;
            labelNextEntry.alpha=(0.25f+alpha)*alpha2;
            labelMain.alpha=(1-alpha);
            labelSub.alpha=(1-alpha);
            labelArtist.alpha=(1-alpha);
                        
            //NSLog(@"current trans X: %d / %d",translationX,gesture_move_file_min_trans);
            break;
        case UIGestureRecognizerStateFailed:
        case UIGestureRecognizerStateCancelled:
        case UIGestureRecognizerStateEnded:
            // reset label
            
            translationX=org_centerx-labelMain.center.x;
                        
            if (translationX>gesture_move_file_min_trans) bEntryInsteadOfSub=true;
            if (translationX<-gesture_move_file_min_trans) bEntryInsteadOfSub=true;
            
            //NSLog(@"trX: %d, max velocity %f",translationX,(float)(max_velocity.x));
            if (translationX>gesture_move_sub_min_trans) [self swipeLeft:bEntryInsteadOfSub];
            else if (translationX<-gesture_move_sub_min_trans) [self swipeRight:bEntryInsteadOfSub];
            else if ((translationX>gesture_swipe_min_trans)&&(max_velocity.x<-gesture_swipe_min_vel)) [self swipeLeft:bEntryInsteadOfSub];
            else if ((translationX<-gesture_swipe_min_trans)&&(max_velocity.x>gesture_swipe_min_vel)) [self swipeRight:bEntryInsteadOfSub];
            
            [UIView beginAnimations:@"miniplayer_recenterinfoview" context:nil];
            [UIView setAnimationDelegate:self];
            [UIView setAnimationDelay:0];
            [UIView setAnimationDuration:0.2f];
            labelMain.center=CGPointMake(org_centerx,labelMain.center.y);
            labelSub.center=CGPointMake(org_centerx,labelSub.center.y);
            labelArtist.center=CGPointMake(org_centerx,labelArtist.center.y);
            labelPrev.alpha=0;
            labelNext.alpha=0;
            labelPrevEntry.alpha=0;
            labelNextEntry.alpha=0;
            labelMain.alpha=1;
            labelSub.alpha=1;
            labelArtist.alpha=1;
            labelPrev.center=CGPointMake(orgPrev_centerx,labelPrev.center.y);
            labelNext.center=CGPointMake(orgNext_centerx,labelNext.center.y);
            labelPrevEntry.center=CGPointMake(orgPrev_centerx,labelPrevEntry.center.y);
            labelNextEntry.center=CGPointMake(orgNext_centerx,labelNextEntry.center.y);
            [UIView commitAnimations];
            
            break;
    }
            
    // reset translation
    [gesture setTranslation:CGPointZero inView:labelMain];
}

-(void) refreshTime {
    if (detailVC.mPlaylist_size==0) {
        self.view.hidden=TRUE;
        return;
    }
    if (self.view.hidden) self.view.hidden=FALSE;
    
    int l=[detailVC.mplayer getSongLength]/1000;
    int t=[detailVC.mplayer getCurrentTime]/1000;
    switch (labelTime_mode) {
        case 0:
            if (l>0) labelTime.text=[NSString stringWithFormat:@"-%d:%.2d\n-\n%d:%.2d",(l-t)/60,(l-t)%60,l/60,l%60];
            else labelTime.text=[NSString stringWithFormat:@"--:--"];
            break;
        case 1:
            labelTime.text=[NSString stringWithFormat:@"%d:%.2d\n-\n%d:%.2d",(t)/60,(t)%60,l/60,l%60];
            break;
    }
    
    if ([detailVC.mplayer isPlaying]) {
        
        if (![[detailVC.mplayer getModFileTitle] isEqualToString:labelMain.text]) [self refreshCoverLabels];
        
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
    static bool no_reentrant=false;
    if (no_reentrant) return;
    no_reentrant=true;
    if ([detailVC.mplayer isPlaying]) {
        
        if ([detailVC.mplayer isPaused]) {
            btnPlay.hidden=false;
            btnPause.hidden=true;
        } else {
            btnPlay.hidden=true;
            btnPause.hidden=false;
        }
        labelMain.text=[detailVC.mplayer getModFileTitle];
        labelArtist.text=[detailVC.mplayer artist];
        if ([detailVC.mplayer isArchive]&&([detailVC.mplayer getArcEntriesCnt]>1)) {
            //archive with multiple files
            if (detailVC.mplayer.mod_subsongs>1) {
                //and also subsongs
                labelSub.text=[NSString stringWithFormat:@"(%d/%d)(%d/%d) %@",[detailVC.mplayer getArcIndex]+1,[detailVC.mplayer getArcEntriesCnt],detailVC.mplayer.mod_currentsub-detailVC.mplayer.mod_minsub+1,detailVC.mplayer.mod_subsongs,[detailVC.mplayer getModName]];
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
        labelPlaylist.text=[NSString stringWithFormat:@"%d\n-\n%d",detailVC.mPlaylist_pos+1,detailVC.mPlaylist_size];
        //labelTime.text=[NSString stringWithFormat:@"%d:%.2d",t/60,t%60];
        [self refreshTime];
    } else {
        labelMain.text=@"Initializing";
        labelSub.text=@"...";
        labelArtist.text=@"";
        labelTime.text=@"-:--";
                
        if (detailVC.mPlaylist_size) labelPlaylist.text=[NSString stringWithFormat:@"%d\n-\n%d",detailVC.mPlaylist_pos+1,detailVC.mPlaylist_size];
        else labelPlaylist.text=@"...\n-\n...";
        
        btnPause.hidden=true;
        btnPlay.hidden=false;
    }
    
    [coverView setImage:coverImg];
    no_reentrant=false;
}

- (void)viewDidLayoutSubviews {
    gesture_swipe_min_vel=SWIPE_MIN_VELOCITY;
    gesture_swipe_min_trans=SWIPE_MIN_TRANSLATION;
    gesture_move_sub_min_trans=MIN(gestureAreaView.frame.size.width*TRIGGER_SUB_MIN_WIDTH_RATIO,TRIGGER_SUB_MIN_TRANSLATION);
    gesture_move_file_min_trans=MIN(gestureAreaView.frame.size.width*TRIGGER_ENTRY_MIN_WIDTH_RATIO,TRIGGER_ENTRY_MIN_TRANSLATION);
}

- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator {
    songInfoView.frame=CGRectMake(50,0,(size.width-50-150),48);
    labelMain.frame=CGRectMake(0,0,(size.width-50-150),24);
    labelSub.frame=CGRectMake(0,24,(size.width-50-150),12);
    labelArtist.frame=CGRectMake(0,24+12,(size.width-50-150),12);
        
    labelPrev.frame=CGRectMake(-[labelPrev.text sizeWithAttributes:@{NSFontAttributeName:labelPrev.font}].width,0,[labelPrev.text sizeWithAttributes:@{NSFontAttributeName:labelPrev.font}].width,48);
    labelPrevEntry.frame=CGRectMake(-[labelPrevEntry.text sizeWithAttributes:@{NSFontAttributeName:labelPrevEntry.font}].width,0,[labelPrevEntry.text sizeWithAttributes:@{NSFontAttributeName:labelPrevEntry.font}].width,48);
    labelNextEntry.frame=CGRectMake((size.width-50-150),0,[labelNextEntry.text sizeWithAttributes:@{NSFontAttributeName:labelNextEntry.font}].width,48);
    labelNext.frame=CGRectMake((size.width-50-150),0,[labelNext.text sizeWithAttributes:@{NSFontAttributeName:labelNext.font}].width,48);
    
    labelTime.frame=CGRectMake(size.width-100,0,50,48);
    labelPlaylist.frame=CGRectMake(size.width-150,0,50,48);
    
    
    [self refreshCoverLabels];
}


- (UIViewController *)visibleViewController:(UIViewController *)rootViewController
{
    if ([rootViewController isKindOfClass:[UITabBarController class]])
    {
        UIViewController *selectedViewController = ((UITabBarController *)rootViewController).selectedViewController;
        
        return [self visibleViewController:selectedViewController];
    }
    if ([rootViewController isKindOfClass:[UINavigationController class]])
    {
        UIViewController *lastViewController = [[((UINavigationController *)rootViewController) viewControllers] lastObject];
        
        return [self visibleViewController:lastViewController];
    }
    
    if (rootViewController.presentedViewController == nil)
    {
        return rootViewController;
    }
    if ([rootViewController.presentedViewController isKindOfClass:[UINavigationController class]])
    {
        UINavigationController *navigationController = (UINavigationController *)rootViewController.presentedViewController;
        UIViewController *lastViewController = [[navigationController viewControllers] lastObject];
        
        return [self visibleViewController:lastViewController];
    }
    if ([rootViewController.presentedViewController isKindOfClass:[UITabBarController class]])
    {
        UITabBarController *tabBarController = (UITabBarController *)rootViewController.presentedViewController;
        UIViewController *selectedViewController = tabBarController.selectedViewController;
        
        return [self visibleViewController:selectedViewController];
    }
    
    UIViewController *presentedViewController = (UIViewController *)rootViewController.presentedViewController;
    
    return [self visibleViewController:presentedViewController];
}


- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    
    //issue when checking minivc directly, so check visible viewcontroller instead
    UIViewController *vc = [self visibleViewController:[UIApplication sharedApplication].keyWindow.rootViewController];
    darkMode=false;
    if (vc.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
    
    //Background color
    
    if (darkMode) [mpview setBackgroundColor:[UIColor colorWithRed:0.1 green:0.1 blue:0.1 alpha:1]];
    else [mpview setBackgroundColor:[UIColor colorWithRed:0.9 green:0.9 blue:0.9 alpha:1]];
    
    //Cover artwork
    coverView=[[UIImageView alloc] init];
    coverView.frame=CGRectMake(0,0,48,48);
    [mpview addSubview:coverView];
    
    //Labels
    int ww=self.view.frame.size.width;
    
    //if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad) {
    if (@available(iOS 14.0, *)) {
        if ([NSProcessInfo processInfo].isiOSAppOnMac) {
            UIScreen* mainscr = [UIScreen mainScreen];
            UIWindow *win=[UIApplication sharedApplication].keyWindow;
            //NSLog(@"%f %f : %f %f",win.bounds.size.width,win.bounds.size.height,win.frame.size.width,win.frame.size.height);
            
            ww=win.bounds.size.width;
        }
    } else {
        
    }
    
    songInfoView=[[UIView alloc] init];
    songInfoView.frame=CGRectMake(50,0,(ww-50-150),48);
    songInfoView.userInteractionEnabled=true;
    songInfoView.backgroundColor=[UIColor clearColor];
    songInfoView.clipsToBounds=true;
    [mpview addSubview:songInfoView];
    
    labelPrev=[[UILabel alloc] init];
    labelPrev.text=NSLocalizedString(@"Previous",@"");
    labelPrev.frame=CGRectMake(-[labelPrev.text sizeWithAttributes:@{NSFontAttributeName:labelPrev.font}].width,0,[labelPrev.text sizeWithAttributes:@{NSFontAttributeName:labelPrev.font}].width,48);
    [labelPrev setFont:[UIFont italicSystemFontOfSize:12]];
    if (darkMode) labelPrev.textColor = [UIColor whiteColor];
    else labelPrev.textColor = [UIColor blackColor];
    labelPrev.textAlignment=NSTextAlignmentCenter;
    [songInfoView addSubview:labelPrev];
    
    UIFont *font = [UIFont systemFontOfSize:12];
    UIFontDescriptor *fontDescriptor = [font fontDescriptor];
    UIFontDescriptor *styleDescriptor = [fontDescriptor fontDescriptorWithSymbolicTraits:[fontDescriptor symbolicTraits]| UIFontDescriptorTraitBold|UIFontDescriptorTraitItalic];
    UIFont *myFont = [UIFont fontWithDescriptor:styleDescriptor size:font.pointSize];
    
    
    labelPrevEntry=[[UILabel alloc] init];
    labelPrevEntry.text=NSLocalizedString(@"Previous file",@"");
    labelPrevEntry.frame=CGRectMake(-[labelPrevEntry.text sizeWithAttributes:@{NSFontAttributeName:labelPrevEntry.font}].width,0,[labelPrevEntry.text sizeWithAttributes:@{NSFontAttributeName:labelPrevEntry.font}].width,48);
    [labelPrevEntry setFont:myFont];
    if (darkMode) labelPrevEntry.textColor = [UIColor whiteColor];
    else labelPrevEntry.textColor = [UIColor blackColor];
    labelPrevEntry.textAlignment=NSTextAlignmentCenter;
    labelPrevEntry.alpha=0;
    [songInfoView addSubview:labelPrevEntry];
    
    labelNextEntry=[[UILabel alloc] init];
    labelNextEntry.text=NSLocalizedString(@"Next file",@"");
    labelNextEntry.frame=CGRectMake((ww-50-150),0,[labelNextEntry.text sizeWithAttributes:@{NSFontAttributeName:labelNextEntry.font}].width,48);
    
    
    [labelNextEntry setFont:myFont];
    if (darkMode) labelNextEntry.textColor = [UIColor whiteColor];
    else labelNextEntry.textColor = [UIColor blackColor];
    labelNextEntry.alpha=0;
    labelNextEntry.textAlignment=NSTextAlignmentCenter;
    [songInfoView addSubview:labelNextEntry];
    
    labelNext=[[UILabel alloc] init];
    labelNext.text=NSLocalizedString(@"Next",@"");
    labelNext.frame=CGRectMake((ww-50-150),0,[labelNext.text sizeWithAttributes:@{NSFontAttributeName:labelNext.font}].width,48);
    [labelNext setFont:[UIFont italicSystemFontOfSize:12]];
    if (darkMode) labelNext.textColor = [UIColor whiteColor];
    else labelNext.textColor = [UIColor blackColor];
    labelNext.textAlignment=NSTextAlignmentCenter;
    [songInfoView addSubview:labelNext];
    
    
    
    labelMain=[[CBAutoScrollLabel alloc] init];
    labelMain.frame=CGRectMake(0,0,(ww-50-150),24);
    [labelMain setFont:[UIFont systemFontOfSize:12]];
    if (darkMode) labelMain.textColor = [UIColor whiteColor];
    else labelMain.textColor = [UIColor blackColor];
    labelMain.labelSpacing = 35; // distance between start and end labels
    labelMain.pauseInterval = 3.7; // seconds of pause before scrolling starts again
    labelMain.scrollSpeed = 30; // pixels per second
    labelMain.textAlignment = NSTextAlignmentLeft; // centers text when no auto-scrolling is applied
    labelMain.fadeLength = 12.f; // length of the left and right edge fade, 0 to disable
    labelMain.userInteractionEnabled=false;
    
    labelSub=[[CBAutoScrollLabel alloc] init];
    labelSub.frame=CGRectMake(0,24,(ww-50-150),12);
    [labelSub setFont:[UIFont systemFontOfSize:10]];
    if (darkMode) labelSub.textColor = [UIColor whiteColor];
    else labelSub.textColor = [UIColor blackColor];
    labelSub.labelSpacing = 35; // distance between start and end labels
    labelSub.pauseInterval = 3.7; // seconds of pause before scrolling starts again
    labelSub.scrollSpeed = 30; // pixels per second
    labelSub.textAlignment = NSTextAlignmentLeft; // centers text when no auto-scrolling is applied
    labelSub.fadeLength = 12.f; // length of the left and right edge fade, 0 to disable
    
    labelArtist=[[CBAutoScrollLabel alloc] init];
    labelArtist.frame=CGRectMake(0,24+12,(ww-50-150),12);
    [labelArtist setFont:[UIFont systemFontOfSize:10]];
    if (darkMode) labelArtist.textColor = [UIColor whiteColor];
    else labelArtist.textColor = [UIColor blackColor];
    labelArtist.labelSpacing = 35; // distance between start and end labels
    labelArtist.pauseInterval = 3.7; // seconds of pause before scrolling starts again
    labelArtist.scrollSpeed = 30; // pixels per second
    labelArtist.textAlignment = NSTextAlignmentLeft; // centers text when no auto-scrolling is applied
    labelArtist.fadeLength = 12.f; // length of the left and right edge fade, 0 to disable
    
    labelTime=[[UILabel alloc] init];
    labelTime.frame=CGRectMake(ww-100,0,50,48);
    [labelTime setFont:[UIFont systemFontOfSize:11]];
    if (darkMode) labelTime.textColor = [UIColor whiteColor];
    else labelTime.textColor = [UIColor blackColor];
    labelTime.textAlignment = NSTextAlignmentCenter; // centers text when no auto-scrolling is applied
    labelTime.userInteractionEnabled=true;
    labelTime.numberOfLines=0;
    
    labelPlaylist=[[UILabel alloc] init];
    labelPlaylist.frame=CGRectMake(ww-150,0,50,48);
    [labelPlaylist setFont:[UIFont systemFontOfSize:10]];
    if (darkMode) labelPlaylist.textColor = [UIColor whiteColor];
    else labelPlaylist.textColor = [UIColor blackColor];
    labelPlaylist.textAlignment = NSTextAlignmentCenter; // centers text when no auto-scrolling is applied
    labelPlaylist.userInteractionEnabled=true;
    labelPlaylist.numberOfLines=0;
    [self refreshCoverLabels];
    
    
    [songInfoView addSubview:labelMain];
    [songInfoView addSubview:labelSub];
    [songInfoView addSubview:labelArtist];
    [mpview addSubview:labelTime];
    [mpview addSubview:labelPlaylist];
    
    //view to cover cover image + labels and recognize tap + swipe gesture
    gestureAreaView=[[UIView alloc] init];
    gestureAreaView.userInteractionEnabled=true;
    gestureAreaView.translatesAutoresizingMaskIntoConstraints=false;
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
    
    if (![[[self parentViewController] title] isEqualToString:NSLocalizedString(@"Now playing",@"")]) {
        //not already on the NowPlaying screen, allow button activtation
        tapGesture = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(pushedPlaylist)];
        // Set required taps and number of touches
        [tapGesture setNumberOfTapsRequired:1];
        [tapGesture setNumberOfTouchesRequired:1];
        // Add the gesture to the view
        [labelPlaylist addGestureRecognizer:tapGesture];
    }
    
    UIPanGestureRecognizer *panGesture =[[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(panLabels:)];
    [gestureAreaView addGestureRecognizer:panGesture];
    
    /*
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
    [gestureAreaView addGestureRecognizer:swipeGesture];*/
    
    
    [mpview addConstraint:[NSLayoutConstraint constraintWithItem:gestureAreaView attribute:NSLayoutAttributeHeight relatedBy:NSLayoutRelationEqual toItem:mpview attribute:NSLayoutAttributeHeight multiplier:1.0 constant:0]];
    [mpview addConstraint:[NSLayoutConstraint constraintWithItem:gestureAreaView attribute:NSLayoutAttributeTop relatedBy:NSLayoutRelationEqual toItem:mpview attribute:NSLayoutAttributeTop multiplier:1.0 constant:0]];
    [mpview addConstraint:[NSLayoutConstraint constraintWithItem:gestureAreaView attribute:NSLayoutAttributeRight relatedBy:NSLayoutRelationEqual toItem:labelMain attribute:NSLayoutAttributeRight multiplier:1.0 constant:0]];
    [mpview addConstraint:[NSLayoutConstraint constraintWithItem:gestureAreaView attribute:NSLayoutAttributeLeft relatedBy:NSLayoutRelationEqual toItem:mpview attribute:NSLayoutAttributeLeft multiplier:1.0 constant:0]];
    
    //Buttons@
    btnPlay=[[BButton alloc] initWithFrame:CGRectMake(0,1,46,46) type:BButtonTypeGray style:BButtonStyleBootstrapV4];
    [btnPlay addAwesomeIcon:FAIconPlay beforeTitle:YES];
    [btnPlay addTarget:self action:@selector(pushedPlay) forControlEvents:UIControlEventTouchUpInside];
    btnPlay.userInteractionEnabled=true;
    btnPlay.translatesAutoresizingMaskIntoConstraints = false;
    btnPlay.hidden=false;
    [btnPlay setColor:mpview.backgroundColor];
    [btnPlay setButtonCornerRadius:[NSNumber numberWithFloat:0.0f]];
    btnPlay.layer.borderWidth=0;
    [mpview addSubview:btnPlay];
    
    btnPause=[[BButton alloc] initWithFrame:CGRectMake(0,1,46,46) type:BButtonTypeGray style:BButtonStyleBootstrapV4];
    [btnPause addAwesomeIcon:FAIconPause beforeTitle:YES];
    [btnPause addTarget:self action:@selector(pushedPause) forControlEvents:UIControlEventTouchUpInside];
    btnPause.translatesAutoresizingMaskIntoConstraints = false;
    btnPause.hidden=true;
    [btnPause setButtonCornerRadius:[NSNumber numberWithFloat:0.0f]];
    [btnPause setColor:mpview.backgroundColor];
    btnPause.layer.borderWidth=0;
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
    repeatingTimer = [NSTimer scheduledTimerWithTimeInterval: 0.20f target:self selector:@selector(refreshTime) userInfo:nil repeats: YES]; //5 times/second
    
    if ([detailVC.mplayer isPlaying]) {
        if ([detailVC.mplayer isPaused]) {
            btnPlay.hidden=false;
            btnPause.hidden=true;
        } else {
            btnPlay.hidden=true;
            btnPause.hidden=false;
        }
    }
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
