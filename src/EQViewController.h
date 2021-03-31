//
//  EQViewController.h
//  modizer
//
//  Created by Yohann Magnien on 07/08/13.
//
//


#import <UIKit/UIKit.h>
#import "ModizerConstants.h"
#import "DetailViewControllerIphone.h"

@interface EQViewController : UIViewController {
    UISlider *eqSlider[EQUALIZER_NB_BANDS];
    UISlider *eqGlobalGain;
    UISwitch *eqOnOff;
    UISwitch *voices[SOUND_MAXMOD_CHANNELS];
    UILabel *voicesLbl[SOUND_MAXMOD_CHANNELS];
    float eqGlobalGainLastValue;
    UILabel *eqLabelFreq[EQUALIZER_NB_BANDS];
    UILabel *eqLabelValue[EQUALIZER_NB_BANDS];
    UILabel *minus12DB,*plus12DB,*zeroDB,*globalGain,*eqOnOffLbl;
@public
    DetailViewControllerIphone *detailViewController;
}
@property (nonatomic, retain) DetailViewControllerIphone *detailViewController;

+ (void) backupEQSettings;
+ (void) restoreEQSettings;


@end
