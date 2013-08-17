//
//  SettingsGenViewController.m
//  modizer
//
//  Created by Yohann Magnien on 10/08/13.
//
//

#import "SettingsGenViewController.h"
#import "MNEValueTrackingSlider.h"

@interface SettingsGenViewController ()
@end

@implementation SettingsGenViewController

@synthesize tableView,detailViewController;

volatile t_settings settings[MAX_SETTINGS];

-(IBAction) goPlayer {
	[self.navigationController pushViewController:detailViewController animated:(detailViewController.mSlowDevice?NO:YES)];
}

+ (void) loadSettings {
    memset((char*)settings,0,sizeof(settings));
    /////////////////////////////////////
    //GLOBAL
    /////////////////////////////////////
    settings[MDZ_SETTINGS_FAMILY_GLOBAL].type=MDZ_FAMILY;
    settings[MDZ_SETTINGS_FAMILY_GLOBAL].label="Global";
    settings[MDZ_SETTINGS_FAMILY_GLOBAL].description=NULL;
    settings[MDZ_SETTINGS_FAMILY_GLOBAL].family=MDZ_SETTINGS_ROOT;
    settings[MDZ_SETTINGS_FAMILY_GLOBAL].sub_family=MDZ_SETTINGS_GLOBAL;
    
    settings[GLOB_ForceMono].label="Force Mono";
    settings[GLOB_ForceMono].description=NULL;
    settings[GLOB_ForceMono].family=MDZ_SETTINGS_GLOBAL;
    settings[GLOB_ForceMono].sub_family=0;
    settings[GLOB_ForceMono].type=MDZ_BOOLSWITCH;
    settings[GLOB_ForceMono].detail.mdz_boolswitch.switch_value=1;
    
    settings[GLOB_Panning].label="Panning";
    settings[GLOB_Panning].description=NULL;
    settings[GLOB_Panning].family=MDZ_SETTINGS_GLOBAL;
    settings[GLOB_Panning].sub_family=0;
    settings[GLOB_Panning].type=MDZ_BOOLSWITCH;
    settings[GLOB_Panning].detail.mdz_boolswitch.switch_value=1;
    
    settings[GLOB_PanningValue].label="Panning Value";
    settings[GLOB_PanningValue].description=NULL;
    settings[GLOB_PanningValue].family=MDZ_SETTINGS_GLOBAL;
    settings[GLOB_PanningValue].sub_family=0;
    settings[GLOB_PanningValue].type=MDZ_SLIDER_CONTINUOUS;
    settings[GLOB_PanningValue].detail.mdz_slider.slider_value=0.7;
    settings[GLOB_PanningValue].detail.mdz_slider.slider_min_value=0;
    settings[GLOB_PanningValue].detail.mdz_slider.slider_max_value=1;

    
    settings[GLOB_DefaultLength].label="Default Length";
    settings[GLOB_DefaultLength].description=NULL;
    settings[GLOB_DefaultLength].family=MDZ_SETTINGS_GLOBAL;
    settings[GLOB_DefaultLength].sub_family=0;
    settings[GLOB_DefaultLength].type=MDZ_SLIDER_DISCRETE;
    settings[GLOB_DefaultLength].detail.mdz_slider.slider_value=SONG_DEFAULT_LENGTH/1000;
    settings[GLOB_DefaultLength].detail.mdz_slider.slider_min_value=10;
    settings[GLOB_DefaultLength].detail.mdz_slider.slider_max_value=1200;
    
    settings[GLOB_DefaultMODPlayer].type=MDZ_SWITCH;
    settings[GLOB_DefaultMODPlayer].label="Default mod player";
    settings[GLOB_DefaultMODPlayer].description=NULL;
    settings[GLOB_DefaultMODPlayer].family=MDZ_SETTINGS_GLOBAL;
    settings[GLOB_DefaultMODPlayer].sub_family=0;
    settings[GLOB_DefaultMODPlayer].detail.mdz_switch.switch_value=1;
    settings[GLOB_DefaultMODPlayer].detail.mdz_switch.switch_value_nb=3;
    settings[GLOB_DefaultMODPlayer].detail.mdz_switch.switch_labels=(char**)malloc(settings[0].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_DefaultMODPlayer].detail.mdz_switch.switch_labels[0]="MODPLUG";
    settings[GLOB_DefaultMODPlayer].detail.mdz_switch.switch_labels[1]="DUMB";
    settings[GLOB_DefaultMODPlayer].detail.mdz_switch.switch_labels[2]="UADE";
    
    settings[GLOB_TitleFilename].label="Filename as title";
    settings[GLOB_TitleFilename].description=NULL;
    settings[GLOB_TitleFilename].family=MDZ_SETTINGS_GLOBAL;
    settings[GLOB_TitleFilename].sub_family=0;
    settings[GLOB_TitleFilename].type=MDZ_BOOLSWITCH;
    settings[GLOB_TitleFilename].detail.mdz_boolswitch.switch_value=0;
    
    settings[GLOB_StatsUpload].label="Send statistics";
    settings[GLOB_StatsUpload].description=NULL;
    settings[GLOB_StatsUpload].family=MDZ_SETTINGS_GLOBAL;
    settings[GLOB_StatsUpload].sub_family=0;
    settings[GLOB_StatsUpload].type=MDZ_BOOLSWITCH;
    settings[GLOB_StatsUpload].detail.mdz_boolswitch.switch_value=1;
    
    settings[GLOB_BackgroundMode].type=MDZ_SWITCH;
    settings[GLOB_BackgroundMode].label="Background mode";
    settings[GLOB_BackgroundMode].description=NULL;
    settings[GLOB_BackgroundMode].family=MDZ_SETTINGS_GLOBAL;
    settings[GLOB_BackgroundMode].sub_family=0;
    settings[GLOB_BackgroundMode].detail.mdz_switch.switch_value=1;
    settings[GLOB_BackgroundMode].detail.mdz_switch.switch_value_nb=3;
    settings[GLOB_BackgroundMode].detail.mdz_switch.switch_labels=(char**)malloc(settings[0].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_BackgroundMode].detail.mdz_switch.switch_labels[0]="Off";
    settings[GLOB_BackgroundMode].detail.mdz_switch.switch_labels[1]="Play";
    settings[GLOB_BackgroundMode].detail.mdz_switch.switch_labels[2]="Full";
    
    settings[GLOB_EnqueueMode].type=MDZ_SWITCH;
    settings[GLOB_EnqueueMode].label="Enqueue mode";
    settings[GLOB_EnqueueMode].description=NULL;
    settings[GLOB_EnqueueMode].family=MDZ_SETTINGS_GLOBAL;
    settings[GLOB_EnqueueMode].sub_family=0;
    settings[GLOB_EnqueueMode].detail.mdz_switch.switch_value=2;
    settings[GLOB_EnqueueMode].detail.mdz_switch.switch_value_nb=3;
    settings[GLOB_EnqueueMode].detail.mdz_switch.switch_labels=(char**)malloc(settings[0].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_EnqueueMode].detail.mdz_switch.switch_labels[0]="First";
    settings[GLOB_EnqueueMode].detail.mdz_switch.switch_labels[1]="Current";
    settings[GLOB_EnqueueMode].detail.mdz_switch.switch_labels[2]="Last";
    
    settings[GLOB_PlayEnqueueAction].type=MDZ_SWITCH;
    settings[GLOB_PlayEnqueueAction].label="Default Action";
    settings[GLOB_PlayEnqueueAction].description=NULL;
    settings[GLOB_PlayEnqueueAction].family=MDZ_SETTINGS_GLOBAL;
    settings[GLOB_PlayEnqueueAction].sub_family=0;
    settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value=0;
    settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value_nb=3;
    settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_labels=(char**)malloc(settings[0].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_labels[0]="Play";
    settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_labels[1]="Enqueue";
    settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_labels[2]="Enq.&Play";
    
    settings[GLOB_AfterDownloadAction].type=MDZ_SWITCH;
    settings[GLOB_AfterDownloadAction].label="Post download action";
    settings[GLOB_AfterDownloadAction].description=NULL;
    settings[GLOB_AfterDownloadAction].family=MDZ_SETTINGS_GLOBAL;
    settings[GLOB_AfterDownloadAction].sub_family=0;
    settings[GLOB_AfterDownloadAction].detail.mdz_switch.switch_value=1;
    settings[GLOB_AfterDownloadAction].detail.mdz_switch.switch_value_nb=3;
    settings[GLOB_AfterDownloadAction].detail.mdz_switch.switch_labels=(char**)malloc(settings[0].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_AfterDownloadAction].detail.mdz_switch.switch_labels[0]="Nothing";
    settings[GLOB_AfterDownloadAction].detail.mdz_switch.switch_labels[1]="Enqueue";
    settings[GLOB_AfterDownloadAction].detail.mdz_switch.switch_labels[2]="Play";
    
    /////////////////////////////////////
    //PLUGINS
    /////////////////////////////////////
    settings[MDZ_SETTINGS_FAMILY_PLUGINS].type=MDZ_FAMILY;
    settings[MDZ_SETTINGS_FAMILY_PLUGINS].label="Plugins";
    settings[MDZ_SETTINGS_FAMILY_PLUGINS].description=NULL;
    settings[MDZ_SETTINGS_FAMILY_PLUGINS].family=MDZ_SETTINGS_ROOT;
    settings[MDZ_SETTINGS_FAMILY_PLUGINS].sub_family=MDZ_SETTINGS_PLUGINS;
    
    /////////////////////////////////////
    //DUMB
    /////////////////////////////////////
    settings[MDZ_SETTINGS_FAMILY_DUMB].type=MDZ_FAMILY;
    settings[MDZ_SETTINGS_FAMILY_DUMB].label="Dumb";
    settings[MDZ_SETTINGS_FAMILY_DUMB].description=NULL;
    settings[MDZ_SETTINGS_FAMILY_DUMB].family=MDZ_SETTINGS_PLUGINS;
    settings[MDZ_SETTINGS_FAMILY_DUMB].sub_family=MDZ_SETTINGS_DUMB;
    
    settings[DUMB_MasterVolume].label="Master Volume";
    settings[DUMB_MasterVolume].description=NULL;
    settings[DUMB_MasterVolume].family=MDZ_SETTINGS_DUMB;
    settings[DUMB_MasterVolume].sub_family=0;
    settings[DUMB_MasterVolume].type=MDZ_SLIDER_CONTINUOUS;
    settings[DUMB_MasterVolume].detail.mdz_slider.slider_value=0.5;
    settings[DUMB_MasterVolume].detail.mdz_slider.slider_min_value=0;
    settings[DUMB_MasterVolume].detail.mdz_slider.slider_max_value=1;
    
    settings[DUMB_Resampling].type=MDZ_SWITCH;
    settings[DUMB_Resampling].label="Resampling";
    settings[DUMB_Resampling].description=NULL;
    settings[DUMB_Resampling].family=MDZ_SETTINGS_DUMB;
    settings[DUMB_Resampling].sub_family=0;
    settings[DUMB_Resampling].detail.mdz_switch.switch_value=1;
    settings[DUMB_Resampling].detail.mdz_switch.switch_value_nb=3;
    settings[DUMB_Resampling].detail.mdz_switch.switch_labels=(char**)malloc(settings[0].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[DUMB_Resampling].detail.mdz_switch.switch_labels[0]="Alias";
    settings[DUMB_Resampling].detail.mdz_switch.switch_labels[1]="Lin";
    settings[DUMB_Resampling].detail.mdz_switch.switch_labels[2]="Cubic";
    
    /////////////////////////////////////
    //TIMIDITY
    /////////////////////////////////////
    settings[MDZ_SETTINGS_FAMILY_TIMIDITY].type=MDZ_FAMILY;
    settings[MDZ_SETTINGS_FAMILY_TIMIDITY].label="Timidity";
    settings[MDZ_SETTINGS_FAMILY_TIMIDITY].description=NULL;
    settings[MDZ_SETTINGS_FAMILY_TIMIDITY].family=MDZ_SETTINGS_PLUGINS;
    settings[MDZ_SETTINGS_FAMILY_TIMIDITY].sub_family=MDZ_SETTINGS_TIMIDITY;
    
    settings[TIM_Polyphony].label="Midi polyphony";
    settings[TIM_Polyphony].description=NULL;
    settings[TIM_Polyphony].family=MDZ_SETTINGS_TIMIDITY;
    settings[TIM_Polyphony].sub_family=0;
    settings[TIM_Polyphony].type=MDZ_SLIDER_DISCRETE;
    settings[TIM_Polyphony].detail.mdz_slider.slider_value=128;
    settings[TIM_Polyphony].detail.mdz_slider.slider_min_value=64;
    settings[TIM_Polyphony].detail.mdz_slider.slider_max_value=256;
    
    settings[TIM_Chorus].type=MDZ_BOOLSWITCH;
    settings[TIM_Chorus].label="Chorus";
    settings[TIM_Chorus].description=NULL;
    settings[TIM_Chorus].family=MDZ_SETTINGS_TIMIDITY;
    settings[TIM_Chorus].sub_family=0;
    settings[TIM_Chorus].detail.mdz_boolswitch.switch_value=1;
    
    settings[TIM_Reverb].type=MDZ_BOOLSWITCH;
    settings[TIM_Reverb].label="Reverb";
    settings[TIM_Reverb].description=NULL;
    settings[TIM_Reverb].family=MDZ_SETTINGS_TIMIDITY;
    settings[TIM_Reverb].sub_family=0;
    settings[TIM_Reverb].detail.mdz_boolswitch.switch_value=1;
    
    settings[TIM_LPFilter].type=MDZ_BOOLSWITCH;
    settings[TIM_LPFilter].label="LPFilter";
    settings[TIM_LPFilter].description=NULL;
    settings[TIM_LPFilter].family=MDZ_SETTINGS_TIMIDITY;
    settings[TIM_LPFilter].sub_family=0;
    settings[TIM_LPFilter].detail.mdz_boolswitch.switch_value=1;

    settings[TIM_Resample].type=MDZ_SWITCH;
    settings[TIM_Resample].label="Resampling";
    settings[TIM_Resample].description=NULL;
    settings[TIM_Resample].family=MDZ_SETTINGS_TIMIDITY;
    settings[TIM_Resample].sub_family=0;
    settings[TIM_Resample].detail.mdz_switch.switch_value=1;
    settings[TIM_Resample].detail.mdz_switch.switch_value_nb=5;
    settings[TIM_Resample].detail.mdz_switch.switch_labels=(char**)malloc(settings[0].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[TIM_Resample].detail.mdz_switch.switch_labels[0]="None";
    settings[TIM_Resample].detail.mdz_switch.switch_labels[1]="Line";
    settings[TIM_Resample].detail.mdz_switch.switch_labels[2]="Spli";
    settings[TIM_Resample].detail.mdz_switch.switch_labels[3]="Gaus";
    settings[TIM_Resample].detail.mdz_switch.switch_labels[4]="Newt";

}

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
        current_family=MDZ_SETTINGS_ROOT;
    }
    return self;
}


- (void)viewDidLoad
{
    [super viewDidLoad];
    
    UIButton *btn = [[UIButton alloc] initWithFrame: CGRectMake(0, 0, 61, 31)];
    [btn setBackgroundImage:[UIImage imageNamed:@"nowplaying_fwd.png"] forState:UIControlStateNormal];
    btn.adjustsImageWhenHighlighted = YES;
    [btn addTarget:self action:@selector(goPlayer) forControlEvents:UIControlEventTouchUpInside];
    UIBarButtonItem *item = [[[UIBarButtonItem alloc] initWithCustomView: btn] autorelease];
    self.navigationItem.rightBarButtonItem = item;

    //TODO: a faire dans le delegate
//    if (current_family==MDZ_SETTINGS_ROOT) [self loadSettings];
    
    //Build current mapping
    cur_settings_nb=0;
    for (int i=0,idx=0;i<MAX_SETTINGS;i++) {
        if (settings[i].family==current_family) {
            cur_settings_idx[cur_settings_nb++]=i;
            
        }
    }

    

    // Uncomment the following line to preserve selection between presentations.
    // self.clearsSelectionOnViewWillAppear = NO;
 
    // Uncomment the following line to display an Edit button in the navigation bar for this view controller.
    // self.navigationItem.rightBarButtonItem = self.editButtonItem;
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

#pragma mark - Table view data source

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return cur_settings_nb;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return 1;
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    NSString *title=nil;
    return title;
}




- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section {
    NSString *footer=(settings[cur_settings_idx[section]].description?[NSString stringWithFormat:@"%s",settings[cur_settings_idx[section]].description]:nil);
    return footer;
}

- (void)boolswitchChanged:(id)sender {
    int refresh=0;
    UISwitch *sw=(UISwitch*)sender;
    NSIndexPath *indexPath = [tableView indexPathForRowAtPoint:[[sender superview] center]];
    if (settings[cur_settings_idx[indexPath.section]].detail.mdz_boolswitch.switch_value != sw.on) refresh=1;
    settings[cur_settings_idx[indexPath.section]].detail.mdz_boolswitch.switch_value=sw.on;
    
    if (refresh) [tableView reloadData];
}
- (void)segconChanged:(id)sender {
    int refresh=0;
    
    NSIndexPath *indexPath = [tableView indexPathForRowAtPoint:[[sender superview] center]];
    if (settings[cur_settings_idx[indexPath.section]].detail.mdz_switch.switch_value !=[(UISegmentedControl*)sender selectedSegmentIndex]) refresh=1;
    settings[cur_settings_idx[indexPath.section]].detail.mdz_switch.switch_value=[(UISegmentedControl*)sender selectedSegmentIndex];
    
    if (refresh) [tableView reloadData];    
}
- (void)sliderChanged:(id)sender {
    NSIndexPath *indexPath = [tableView indexPathForRowAtPoint:[[sender superview] center]];
    
    settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_value=((MNEValueTrackingSlider*)sender).value;
//    if (OPTION(video_fskip)==10) [((MNEValueTrackingSlider*)sender) setValue:10 sValue:@"AUTO"];
//    [tableView reloadData];
}


- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    static NSString *CellIdentifier = @"Cell";
    NSString *cellValue;
    const NSInteger TOP_LABEL_TAG = 1001;
    UILabel *topLabel;
    
    UISwitch *switchview;
    UISegmentedControl *segconview;
    NSMutableArray *tmpArray;
    MNEValueTrackingSlider *sliderview;
    
    
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:CellIdentifier];
    if (cell == nil) {
        cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CellIdentifier] autorelease];
        
        cell.frame=CGRectMake(0,0,tableView.frame.size.width,50);
        
        [cell setBackgroundColor:[UIColor clearColor]];
        CAGradientLayer *gradient = [CAGradientLayer layer];
        gradient.frame = cell.bounds;
        gradient.colors = [NSArray arrayWithObjects:
                           (id)[[UIColor colorWithRed:255.0/255.0 green:255.0/255.0 blue:255.0/255.0 alpha:1] CGColor],
                           (id)[[UIColor colorWithRed:255.0/255.0 green:255.0/255.0 blue:255.0/255.0 alpha:1] CGColor],
                           (id)[[UIColor colorWithRed:235.0/255.0 green:235.0/255.0 blue:235.0/255.0 alpha:1] CGColor],
                           (id)[[UIColor colorWithRed:240.0/255.0 green:240.0/255.0 blue:240.0/255.0 alpha:1] CGColor],
                           (id)[[UIColor colorWithRed:200.0/255.0 green:200.0/255.0 blue:200.0/255.0 alpha:1] CGColor],
                           (id)[[UIColor colorWithRed:200.0/255.0 green:200.0/255.0 blue:200.0/255.0 alpha:1] CGColor],
                           nil];
        gradient.locations = [NSArray arrayWithObjects:
                              (id)[NSNumber numberWithFloat:0.00f],
                              (id)[NSNumber numberWithFloat:0.03f],
                              (id)[NSNumber numberWithFloat:0.03f],
                              (id)[NSNumber numberWithFloat:0.97f],
                              (id)[NSNumber numberWithFloat:0.97f],
                              (id)[NSNumber numberWithFloat:1.00f],
                              nil];
        [cell setBackgroundView:[[UIView alloc] init]];
        [cell.backgroundView.layer insertSublayer:gradient atIndex:0];
        
        CAGradientLayer *selgrad = [CAGradientLayer layer];
        selgrad.frame = cell.bounds;
        float rev_col_adj=1.2f;
        selgrad.colors = [NSArray arrayWithObjects:
                          (id)[[UIColor colorWithRed:rev_col_adj-255.0/255.0 green:rev_col_adj-255.0/255.0 blue:rev_col_adj-255.0/255.0 alpha:1] CGColor],
                          (id)[[UIColor colorWithRed:rev_col_adj-255.0/255.0 green:rev_col_adj-255.0/255.0 blue:rev_col_adj-255.0/255.0 alpha:1] CGColor],
                          (id)[[UIColor colorWithRed:rev_col_adj-235.0/255.0 green:rev_col_adj-235.0/255.0 blue:rev_col_adj-235.0/255.0 alpha:1] CGColor],
                          (id)[[UIColor colorWithRed:rev_col_adj-240.0/255.0 green:rev_col_adj-240.0/255.0 blue:rev_col_adj-240.0/255.0 alpha:1] CGColor],
                          (id)[[UIColor colorWithRed:rev_col_adj-200.0/255.0 green:rev_col_adj-200.0/255.0 blue:rev_col_adj-200.0/255.0 alpha:1] CGColor],
                          (id)[[UIColor colorWithRed:rev_col_adj-200.0/255.0 green:rev_col_adj-200.0/255.0 blue:rev_col_adj-200.0/255.0 alpha:1] CGColor],
                          nil];
        selgrad.locations = [NSArray arrayWithObjects:
                             (id)[NSNumber numberWithFloat:0.00f],
                             (id)[NSNumber numberWithFloat:0.03f],
                             (id)[NSNumber numberWithFloat:0.03f],
                             (id)[NSNumber numberWithFloat:0.97f],
                             (id)[NSNumber numberWithFloat:0.97f],
                             (id)[NSNumber numberWithFloat:1.00f],
                             nil];
        
        [cell setSelectedBackgroundView:[[UIView alloc] init]];
        [cell.selectedBackgroundView.layer insertSublayer:selgrad atIndex:0];
 
        //
        // Create the label for the top row of text
        //
        topLabel = [[[UILabel alloc] init] autorelease];
        [cell.contentView addSubview:topLabel];
        //
        // Configure the properties for the text that are the same on every row
        //
        topLabel.tag = TOP_LABEL_TAG;
        topLabel.backgroundColor = [UIColor clearColor];
        topLabel.textColor = [UIColor colorWithRed:0.1 green:0.1 blue:0.1 alpha:1.0];
        topLabel.highlightedTextColor = [UIColor colorWithRed:0.9 green:0.9 blue:0.9 alpha:1.0];
        topLabel.font = [UIFont boldSystemFontOfSize:14];
        topLabel.lineBreakMode=UILineBreakModeMiddleTruncation;
        topLabel.opaque=TRUE;
        topLabel.numberOfLines=0;
        
        cell.accessoryView=nil;
        cell.accessoryType = UITableViewCellAccessoryNone;
    } else {
        topLabel = (UILabel *)[cell viewWithTag:TOP_LABEL_TAG];
    }
    topLabel.textColor = [UIColor colorWithRed:0.1 green:0.1 blue:0.1 alpha:1.0];
    topLabel.highlightedTextColor = [UIColor colorWithRed:0.9 green:0.9 blue:0.9 alpha:1.0];
    
    topLabel.frame= CGRectMake(4,
                               0,
                               tableView.bounds.size.width*4/10,
                               50);
    
    
    
    topLabel.text=[NSString stringWithFormat:@"%s",settings[cur_settings_idx[indexPath.section]].label];
    
    switch (settings[cur_settings_idx[indexPath.section]].type) {
        case MDZ_FAMILY:
            cell.accessoryView=nil;
            cell.accessoryType=UITableViewCellAccessoryDisclosureIndicator;
            break;
        case MDZ_BOOLSWITCH:
            switchview = [[UISwitch alloc] initWithFrame:CGRectMake(0,0,tableView.bounds.size.width*5.5f/10,36)];
            [switchview addTarget:self action:@selector(boolswitchChanged:) forControlEvents:UIControlEventValueChanged];
            cell.accessoryView = switchview;
            [switchview release];
            switchview.on=settings[cur_settings_idx[indexPath.section]].detail.mdz_boolswitch.switch_value;
            break;
        case MDZ_SWITCH:{
            tmpArray=[[[NSMutableArray alloc] init] autorelease];
            for (int i=0;i<settings[cur_settings_idx[indexPath.section]].detail.mdz_switch.switch_value_nb;i++) {
                [tmpArray addObject:[NSString stringWithFormat:@"%s",settings[cur_settings_idx[indexPath.section]].detail.mdz_switch.switch_labels[i]]];
            }
            segconview = [[UISegmentedControl alloc] initWithItems:tmpArray];
            segconview.frame=CGRectMake(0,0,tableView.bounds.size.width*5.5f/10,50);
//            segconview.
            UIFont *font = [UIFont boldSystemFontOfSize:12.0f];
            NSDictionary *attributes = [NSDictionary dictionaryWithObject:font
                                                                   forKey:UITextAttributeFont];
            [segconview setTitleTextAttributes:attributes
                                            forState:UIControlStateNormal];
            
//            segconview.segmentedControlStyle = UISegmentedControlStyleBar;
            [segconview addTarget:self action:@selector(segconChanged:) forControlEvents:UIControlEventValueChanged];
            cell.accessoryView = segconview;
            [segconview release];
            segconview.selectedSegmentIndex=settings[cur_settings_idx[indexPath.section]].detail.mdz_switch.switch_value;
        }
            break;
        case MDZ_SLIDER_CONTINUOUS:
            sliderview = [[MNEValueTrackingSlider alloc] initWithFrame:CGRectMake(0,0,tableView.bounds.size.width*5.5f/10,50)];
            sliderview.integerMode=0;
            [sliderview setMaximumValue:settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_max_value];
            [sliderview setMinimumValue:settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_min_value];
            [sliderview setContinuous:true];
            sliderview.value=settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_value;
            [sliderview addTarget:self action:@selector(sliderChanged:) forControlEvents:UIControlEventValueChanged];
            cell.accessoryView = sliderview;
            [sliderview release];
            break;
        case MDZ_SLIDER_DISCRETE:
            sliderview = [[MNEValueTrackingSlider alloc] initWithFrame:CGRectMake(0,0,tableView.bounds.size.width*5.5f/10,50)];
            sliderview.integerMode=1;
            [sliderview setMaximumValue:settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_max_value];
            [sliderview setMinimumValue:settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_min_value];
            [sliderview setContinuous:true];
            sliderview.value=settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_value;
            [sliderview addTarget:self action:@selector(sliderChanged:) forControlEvents:UIControlEventValueChanged];
            cell.accessoryView = sliderview;
            [sliderview release];
            break;
        case MDZ_TEXTBOX:
            break;
    }
    
    return cell;
}

/*
// Override to support conditional editing of the table view.
- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath
{
    // Return NO if you do not want the specified item to be editable.
    return YES;
}
*/

/*
// Override to support editing the table view.
- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath
{
    if (editingStyle == UITableViewCellEditingStyleDelete) {
        // Delete the row from the data source
        [tableView deleteRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationFade];
    }   
    else if (editingStyle == UITableViewCellEditingStyleInsert) {
        // Create a new instance of the appropriate class, insert it into the array, and add a new row to the table view
    }   
}
*/

/*
// Override to support rearranging the table view.
- (void)tableView:(UITableView *)tableView moveRowAtIndexPath:(NSIndexPath *)fromIndexPath toIndexPath:(NSIndexPath *)toIndexPath
{
}
*/

/*
// Override to support conditional rearranging of the table view.
- (BOOL)tableView:(UITableView *)tableView canMoveRowAtIndexPath:(NSIndexPath *)indexPath
{
    // Return NO if you do not want the item to be re-orderable.
    return YES;
}
*/

#pragma mark - Table view delegate

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    SettingsGenViewController *settingsVC;
    
    if (settings[cur_settings_idx[indexPath.section]].type==MDZ_FAMILY) {
        settingsVC=[[[SettingsGenViewController alloc] initWithNibName:@"SettingsViewController" bundle:[NSBundle mainBundle]] autorelease];
        settingsVC->detailViewController=detailViewController;
        settingsVC.title=NSLocalizedString(@"General Settings",@"");
        settingsVC->current_family=settings[cur_settings_idx[indexPath.section]].sub_family;
        [self.navigationController pushViewController:settingsVC animated:YES];
    }
}

@end
