//
//  EQViewController.m
//  modizer
//
//  Created by Yohann Magnien on 07/08/13.
//
//

extern BOOL is_ios7;

#import "EQViewController.h"
#import "QuartzCore/CAGradientLayer.h"

/*
 */

//NVDSP
#import "Novocaine.h"
#import "NVDSP.h"
#import "NVPeakingEQFilter.h"
#import "NVSoundLevelMeter.h"
#import "NVClippingDetection.h"

extern NVPeakingEQFilter *nvdsp_PEQ[10];
extern BOOL nvdsp_EQ;


@interface EQViewController ()

@end

@implementation EQViewController

@synthesize detailViewController;

-(IBAction) goPlayer {
	[self.navigationController pushViewController:detailViewController animated:(detailViewController.mSlowDevice?NO:YES)];
}



- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (void)sliderEQGainChanged:(id)sender {
    UISlider *slider=(UISlider *)sender;
    float delta=slider.value-eqGlobalGainLastValue;
    eqGlobalGainLastValue=slider.value;
    for (int i=0;i<EQUALIZER_NB_BANDS;i++) {
        nvdsp_PEQ[i].G+=delta;
        if (nvdsp_PEQ[i].G>12) nvdsp_PEQ[i].G=12;
        if (nvdsp_PEQ[i].G<-12) nvdsp_PEQ[i].G=-12;
        slider=(UISlider*)[self.view viewWithTag:i+1];
        slider.value=nvdsp_PEQ[i].G;
    }
}

-(void)switchChanged:(id)sender {
    nvdsp_EQ=((UISwitch*)sender).on;
}


- (void)sliderChanged:(id)sender {
    UISlider *slider=(UISlider *)sender;
    nvdsp_PEQ[slider.tag-1].G=slider.value;
    eqLabelValue[slider.tag-1].text=[NSString stringWithFormat:@"%.1fdB",nvdsp_PEQ[slider.tag-1].G];
}

+ (void) restoreEQSettings {
    NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
	NSNumber *valNb;
    NSString *str;
    
    for (int i=0;i<EQUALIZER_NB_BANDS;i++) {
        str=[NSString stringWithFormat:@"eq_centerFrequencies_%d",i];
        valNb=[prefs objectForKey:str];
        if (valNb!=nil) nvdsp_PEQ[i].centerFrequency=[valNb floatValue];
        
        str=[NSString stringWithFormat:@"eq_Q_%d",i];
        valNb=[prefs objectForKey:str];
        if (valNb!=nil) nvdsp_PEQ[i].Q=[valNb floatValue];
        
        str=[NSString stringWithFormat:@"eq_G_%d",i];
        valNb=[prefs objectForKey:str];
        if (valNb!=nil) nvdsp_PEQ[i].G=[valNb floatValue];
    }
    
    valNb=[prefs objectForKey:@"nvdsp_EQ"];
    if (valNb!=nil) nvdsp_EQ=[valNb boolValue];
}

+ (void) backupEQSettings {
    NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
	NSNumber *valNb;
    NSString *str;
    
	for (int i=0;i<EQUALIZER_NB_BANDS;i++) {
        str=[NSString stringWithFormat:@"eq_centerFrequencies_%d",i];
        valNb=[[NSNumber alloc] initWithFloat:nvdsp_PEQ[i].centerFrequency];
        [prefs setObject:valNb forKey:str];[valNb autorelease];
    
        str=[NSString stringWithFormat:@"eq_Q_%d",i];
        valNb=[[NSNumber alloc] initWithFloat:nvdsp_PEQ[i].Q];
        [prefs setObject:valNb forKey:str];[valNb autorelease];
    
        str=[NSString stringWithFormat:@"eq_G_%d",i];
        valNb=[[NSNumber alloc] initWithFloat:nvdsp_PEQ[i].G];
        [prefs setObject:valNb forKey:str];[valNb autorelease];
    }
    
    valNb=[[NSNumber alloc] initWithBool:nvdsp_EQ];
    [prefs setObject:valNb forKey:@"nvdsp_EQ"];[valNb autorelease];
    
    [prefs synchronize];
}



- (void)viewDidLoad
{
    [super viewDidLoad];
    
    CGAffineTransform sliderRotation = CGAffineTransformIdentity;
    sliderRotation = CGAffineTransformRotate(sliderRotation, -(M_PI / 2));
    
    for (int i=0;i<EQUALIZER_NB_BANDS;i++) {
        eqSlider[i]=[[UISlider alloc] init];
        [eqSlider[i] addTarget:self action:@selector(sliderChanged:) forControlEvents:UIControlEventValueChanged];
        
        [eqSlider[i] setThumbImage:[UIImage imageNamed:@"slider.png" ] forState:UIControlStateNormal];

        
        eqSlider[i].tag=i+1;
        eqSlider[i].maximumValue=12;
        eqSlider[i].minimumValue=-12;
        eqSlider[i].transform=sliderRotation;
        eqSlider[i].frame=CGRectMake(10+(i+1)*(self.view.frame.size.width-8)/(EQUALIZER_NB_BANDS+2),32,16,self.view.frame.size.height/2);
        [self.view addSubview:eqSlider[i]];
        [eqSlider[i] release];
        
        eqLabelFreq[i]=[[UILabel alloc] initWithFrame:CGRectMake(8+(i+1)*(self.view.frame.size.width-8)/(EQUALIZER_NB_BANDS+2),16,32,16)];
        eqLabelFreq[i].font=[UIFont boldSystemFontOfSize:8];
        eqLabelFreq[i].backgroundColor=[UIColor clearColor];
        eqLabelFreq[i].textColor=[UIColor whiteColor];
        
        [self.view addSubview:eqLabelFreq[i]];
        [eqLabelFreq[i] release];
        
        eqLabelValue[i]=[[UILabel alloc] initWithFrame:CGRectMake(8+(i+1)*(self.view.frame.size.width-8)/(EQUALIZER_NB_BANDS+2),self.view.frame.size.height/2+32,32,16)];
        eqLabelValue[i].font=[UIFont systemFontOfSize:7];
        eqLabelValue[i].backgroundColor=[UIColor clearColor];
        eqLabelValue[i].textColor=[UIColor whiteColor];
        
        [self.view addSubview:eqLabelValue[i]];
        [eqLabelValue[i] release];
    }
    
    eqGlobalGain=[[UISlider alloc] init];
    [eqGlobalGain addTarget:self action:@selector(sliderEQGainChanged:) forControlEvents:UIControlEventValueChanged];
    
    [eqGlobalGain setThumbImage:[UIImage imageNamed:@"slider.png" ] forState:UIControlStateNormal];
    
    eqGlobalGain.tag=100;
    eqGlobalGain.maximumValue=12;
    eqGlobalGain.minimumValue=-12;
    eqGlobalGain.value=0;
    eqGlobalGainLastValue=0;
    eqGlobalGain.transform=sliderRotation;
    eqGlobalGain.frame=CGRectMake(10+(self.view.frame.size.width-34),32,16,self.view.frame.size.height/2);
    [self.view addSubview:eqGlobalGain];
    [eqGlobalGain release];
    
    minus12DB=[[UILabel alloc] initWithFrame:CGRectMake(4,self.view.frame.size.height/2+32,28,20)];
    minus12DB.font=[UIFont boldSystemFontOfSize:8];
    minus12DB.text=@"-12dB";
    minus12DB.backgroundColor=[UIColor clearColor];
    minus12DB.textColor=[UIColor whiteColor];
    
    [self.view addSubview:minus12DB];
    [minus12DB release];
    
    plus12DB=[[UILabel alloc] initWithFrame:CGRectMake(4,32,28,20)];
    plus12DB.font=[UIFont boldSystemFontOfSize:8];
    plus12DB.text=@"+12dB";
    plus12DB.backgroundColor=[UIColor clearColor];
    plus12DB.textColor=[UIColor whiteColor];
    
    [self.view addSubview:plus12DB];
    [plus12DB release];
    
    zeroDB=[[UILabel alloc] initWithFrame:CGRectMake(4,self.view.frame.size.height/4+32-10,28,20)];
    zeroDB.font=[UIFont boldSystemFontOfSize:8];
    zeroDB.text=@"+0dB";
    zeroDB.backgroundColor=[UIColor clearColor];
    zeroDB.textColor=[UIColor whiteColor];
    
    [self.view addSubview:zeroDB];
    [zeroDB release];
    
    globalGain=[[UILabel alloc] initWithFrame:CGRectMake(10+(self.view.frame.size.width-34),self.view.frame.size.height/2+32,32,16)];
    globalGain.font=[UIFont systemFontOfSize:8];
    globalGain.text=@"Gbl";
    globalGain.backgroundColor=[UIColor clearColor];
    globalGain.textColor=[UIColor whiteColor];
    [self.view addSubview:globalGain];
    [globalGain release];
    
    eqOnOff=[[UISwitch alloc] init];
    eqOnOff.frame=CGRectMake(10,self.view.frame.size.height/2+32,32,32);
    [eqOnOff addTarget:self action:@selector(switchChanged:) forControlEvents:UIControlEventValueChanged];
    eqOnOff.on=nvdsp_EQ;
    [self.view addSubview:eqOnOff];
    [eqOnOff release];
    
    eqOnOffLbl=[[UILabel alloc] initWithFrame:CGRectMake(4,self.view.frame.size.height/2+64,100,20)];
    eqOnOffLbl.font=[UIFont boldSystemFontOfSize:10];
    eqOnOffLbl.text=@"Apply equalizer";
    eqOnOffLbl.backgroundColor=[UIColor clearColor];
    eqOnOffLbl.textColor=[UIColor whiteColor];
    
    [self.view addSubview:eqOnOffLbl];
    [eqOnOffLbl release];
    
}

- (void) recomputeFrames {
    for (int i=0;i<EQUALIZER_NB_BANDS;i++) {
        eqSlider[i].frame=CGRectMake(10+(i+1)*(self.view.frame.size.width-8)/(EQUALIZER_NB_BANDS+2),32,16,self.view.frame.size.height/2);
        eqLabelFreq[i].frame=CGRectMake(8+(i+1)*(self.view.frame.size.width-8)/(EQUALIZER_NB_BANDS+2),16,32,16);
        eqLabelValue[i].frame=CGRectMake(8+(i+1)*(self.view.frame.size.width-8)/(EQUALIZER_NB_BANDS+2),self.view.frame.size.height/2+32,32,16);
    }
    minus12DB.frame=CGRectMake(4,self.view.frame.size.height/2+32-23,28,20);
    plus12DB.frame=CGRectMake(4,32+1,28,20);
    zeroDB.frame=CGRectMake(4,self.view.frame.size.height/4+32-11,28,20);
    
    globalGain.frame=CGRectMake(10+(self.view.frame.size.width-34),self.view.frame.size.height/2+32,32,16);
    eqGlobalGain.frame=CGRectMake(10+(self.view.frame.size.width-34),32,16,self.view.frame.size.height/2);
    
    eqOnOff.frame=CGRectMake(80+10,self.view.frame.size.height/2+64,32,20);

    eqOnOffLbl.frame=CGRectMake(4,self.view.frame.size.height/2+64+2,80,20);
}

- (void) viewWillDisappear:(BOOL)animated {
    [EQViewController backupEQSettings];
    [super viewWillDisappear:animated];
}

- (void) viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    
    if (!is_ios7) {
        [self.navigationController.navigationBar setBarStyle:UIBarStyleBlack];
    } else {
        [self.navigationController.navigationBar setBarStyle:UIBarStyleDefault];
        [[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleDefault animated:YES];
    }
    
    
    [self recomputeFrames];
    
    for (int i=0;i<EQUALIZER_NB_BANDS;i++) {
        eqSlider[i].value=nvdsp_PEQ[i].G;
        if (nvdsp_PEQ[i].centerFrequency>=1000) {
            eqLabelFreq[i].text=[NSString stringWithFormat:@"%.0fKhz",nvdsp_PEQ[i].centerFrequency/1000];
        } else {
            eqLabelFreq[i].text=[NSString stringWithFormat:@"%.0fHz",nvdsp_PEQ[i].centerFrequency];
        }
        eqLabelValue[i].text=[NSString stringWithFormat:@"%.1fdB",nvdsp_PEQ[i].G];
        
    }
}

- (NSUInteger)supportedInterfaceOrientations {
    return UIInterfaceOrientationMaskAll;
}

- (BOOL)shouldAutorotate {
    [self shouldAutorotateToInterfaceOrientation:self.interfaceOrientation];
    return TRUE;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    [self recomputeFrames];
    [self.view setNeedsDisplay];
	return YES;
}

- (void)dealloc {
    [super dealloc];
}


- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

@end
