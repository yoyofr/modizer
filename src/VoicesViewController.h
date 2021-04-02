//
//  VoicesViewController.h
//  modizer
//
//  Created by Yohann Magnien on 02/04/2021.
//

#import <UIKit/UIKit.h>

#import "ModizerConstants.h"
#import "DetailViewControllerIphone.h"
#import "BButton.h"

NS_ASSUME_NONNULL_BEGIN

@interface VoicesViewController : UIViewController {
    BButton *voices[SOUND_MAXMOD_CHANNELS];
    UILabel *voicesLbl[SOUND_MAXMOD_CHANNELS];
    BButton *voicesSolo[SOUND_MAXMOD_CHANNELS];
    BButton *voicesAllOn;
    BButton *voicesAllOff;
    UILabel *voicesAllLbl;
    
    NSString *currentPlayingFile;
    
    IBOutlet UIScrollView *scrollView;
    
    DetailViewControllerIphone *detailViewController;
}

@property (nonatomic, retain) DetailViewControllerIphone *detailViewController;
@property (nonatomic, retain) IBOutlet UIScrollView *scrollView;

- (void) removeVoicesButtons;
- (void) resetVoicesButtons;
- (void) recomputeFrames;
- (void) checkPlayer;

@end

NS_ASSUME_NONNULL_END
