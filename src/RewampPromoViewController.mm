//
//  RewampPromoViewController.mm
//  modizer
//

#import "RewampPromoViewController.h"
#import "ModizerConstants.h"

#define REWAMP_SHOTS_COUNT 4

@implementation RewampPromoViewController

+ (void)openRewamp {
    BOOL onMac=[NSProcessInfo processInfo].isiOSAppOnMac;
#if TARGET_OS_MACCATALYST
    onMac=YES;
#endif
    NSURL *url=[NSURL URLWithString:(onMac?REWAMP_WEB_URL:REWAMP_APPSTORE_URL)];
    [[UIApplication sharedApplication] openURL:url options:@{} completionHandler:nil];
}

- (instancetype)init {
    self=[super initWithNibName:nil bundle:nil];
    if (self) {
        if (UI_USER_INTERFACE_IDIOM()==UIUserInterfaceIdiomPhone) self.modalPresentationStyle=UIModalPresentationPageSheet;
        else self.modalPresentationStyle=UIModalPresentationFormSheet;
    }
    return self;
}

- (UIStatusBarStyle)preferredStatusBarStyle {
    return UIStatusBarStyleLightContent;
}

- (UILabel *)labelWithText:(NSString *)text font:(UIFont *)font color:(UIColor *)color {
    UILabel *lbl=[[UILabel alloc] init];
    lbl.text=text;
    lbl.font=font;
    lbl.textColor=color;
    lbl.numberOfLines=0;
    lbl.adjustsFontForContentSizeCategory=NO;
    return lbl;
}

- (void)viewDidLoad {
    [super viewDidLoad];

    //Rewamp look: always dark, deep purple background & pink accent
    self.overrideUserInterfaceStyle=UIUserInterfaceStyleDark;
    UIColor *bgColor=[UIColor colorWithRed:0x16/255.0 green:0x0B/255.0 blue:0x20/255.0 alpha:1];
    UIColor *accentColor=[UIColor colorWithRed:0xF5/255.0 green:0x38/255.0 blue:0x8F/255.0 alpha:1];
    UIColor *textColor=[UIColor colorWithWhite:1 alpha:0.92];
    UIColor *dimColor=[UIColor colorWithWhite:1 alpha:0.6];
    self.view.backgroundColor=bgColor;

    //Localized message is 3 paragraphs: pitch / price & platforms / where to find the link later
    NSString *msg=[NSString stringWithFormat:NSLocalizedString(@"Rewamp is the new retro music player from the author of Modizer: game soundtracks, chiptunes, tracker and demoscene music, in a modern, redesigned interface with a huge online library.\n\nFree and ad-free, available on iOS, Android, macOS, Windows and Linux.\n\nYou can find this link later in the '%@' tab.",@""),NSLocalizedString(@"More",@"")];
    NSArray<NSString*> *parts=[msg componentsSeparatedByString:@"\n\n"];
    NSString *pitchTxt=(parts.count>0?parts[0]:msg);
    NSString *platformsTxt=(parts.count>1?parts[1]:@"");
    NSString *laterTxt=(parts.count>2?parts[2]:@"");

    //Everything scrolls vertically if it does not fit (small screens, landscape)
    UIScrollView *scroll=[[UIScrollView alloc] init];
    scroll.translatesAutoresizingMaskIntoConstraints=NO;
    scroll.alwaysBounceVertical=NO;
    scroll.showsVerticalScrollIndicator=NO;
    [self.view addSubview:scroll];

    UIStackView *content=[[UIStackView alloc] init];
    content.translatesAutoresizingMaskIntoConstraints=NO;
    content.axis=UILayoutConstraintAxisVertical;
    content.alignment=UIStackViewAlignmentFill;
    content.spacing=16;
    [scroll addSubview:content];

    UILayoutGuide *safe=self.view.safeAreaLayoutGuide;
    [NSLayoutConstraint activateConstraints:@[
        [scroll.topAnchor constraintEqualToAnchor:safe.topAnchor],
        [scroll.bottomAnchor constraintEqualToAnchor:safe.bottomAnchor],
        [scroll.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor],
        [scroll.trailingAnchor constraintEqualToAnchor:self.view.trailingAnchor],
        [content.topAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.topAnchor constant:24],
        [content.bottomAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.bottomAnchor constant:-12],
        [content.leadingAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.leadingAnchor],
        [content.trailingAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.trailingAnchor],
        [content.widthAnchor constraintEqualToAnchor:scroll.frameLayoutGuide.widthAnchor],
    ]];

    const CGFloat margin=22;

    //Header: app icon + name + tagline
    UIImageView *icon=[[UIImageView alloc] initWithImage:[UIImage imageNamed:@"RewampIcon"]];
    icon.contentMode=UIViewContentModeScaleAspectFill;
    icon.layer.cornerRadius=16;
    icon.layer.cornerCurve=kCACornerCurveContinuous;
    icon.layer.borderWidth=1;
    icon.layer.borderColor=[UIColor colorWithWhite:1 alpha:0.15].CGColor;
    icon.clipsToBounds=YES;
    [icon.widthAnchor constraintEqualToConstant:72].active=YES;
    [icon.heightAnchor constraintEqualToConstant:72].active=YES;

    UILabel *nameLbl=[self labelWithText:@"Rewamp" font:[UIFont systemFontOfSize:32 weight:UIFontWeightHeavy] color:[UIColor whiteColor]];
    UILabel *taglineLbl=[self labelWithText:NSLocalizedString(@"A modern alternative to Modizer",@"") font:[UIFont systemFontOfSize:15 weight:UIFontWeightSemibold] color:accentColor];
    UIStackView *titles=[[UIStackView alloc] initWithArrangedSubviews:@[nameLbl,taglineLbl]];
    titles.axis=UILayoutConstraintAxisVertical;
    titles.spacing=2;

    UIStackView *header=[[UIStackView alloc] initWithArrangedSubviews:@[icon,titles]];
    header.axis=UILayoutConstraintAxisHorizontal;
    header.alignment=UIStackViewAlignmentCenter;
    header.spacing=14;
    header.layoutMarginsRelativeArrangement=YES;
    header.directionalLayoutMargins=NSDirectionalEdgeInsetsMake(0, margin, 0, margin);
    [content addArrangedSubview:header];

    //Screenshots carousel
    UIScrollView *shotsScroll=[[UIScrollView alloc] init];
    shotsScroll.showsHorizontalScrollIndicator=NO;
    shotsScroll.alwaysBounceHorizontal=YES;
    shotsScroll.clipsToBounds=NO;

    UIStackView *shots=[[UIStackView alloc] init];
    shots.translatesAutoresizingMaskIntoConstraints=NO;
    shots.axis=UILayoutConstraintAxisHorizontal;
    shots.spacing=12;
    [shotsScroll addSubview:shots];

    for (int i=1;i<=REWAMP_SHOTS_COUNT;i++) {
        UIImage *img=[UIImage imageNamed:[NSString stringWithFormat:@"RewampShot%d",i]];
        if (!img) continue;
        UIImageView *shot=[[UIImageView alloc] initWithImage:img];
        shot.contentMode=UIViewContentModeScaleAspectFill;
        shot.layer.cornerRadius=16;
        shot.layer.cornerCurve=kCACornerCurveContinuous;
        shot.layer.borderWidth=1;
        shot.layer.borderColor=[UIColor colorWithWhite:1 alpha:0.18].CGColor;
        shot.clipsToBounds=YES;
        shot.isAccessibilityElement=NO;
        //size is driven by the carousel height, not by the image native size
        [shot setContentCompressionResistancePriority:UILayoutPriorityDefaultLow forAxis:UILayoutConstraintAxisVertical];
        [shot setContentCompressionResistancePriority:UILayoutPriorityDefaultLow forAxis:UILayoutConstraintAxisHorizontal];
        [shot setContentHuggingPriority:UILayoutPriorityDefaultLow forAxis:UILayoutConstraintAxisVertical];
        [shot setContentHuggingPriority:UILayoutPriorityDefaultLow forAxis:UILayoutConstraintAxisHorizontal];
        [shot.widthAnchor constraintEqualToAnchor:shot.heightAnchor multiplier:img.size.width/img.size.height].active=YES;
        [shots addArrangedSubview:shot];
    }

    [content addArrangedSubview:shotsScroll];
    
    //carousel height follows the available height, with a floor to stay readable
    NSLayoutConstraint *shotsH=[shotsScroll.heightAnchor constraintEqualToAnchor:self.view.heightAnchor multiplier:0.42];
    shotsH.priority=999;
    [NSLayoutConstraint activateConstraints:@[
        shotsH,
        [shotsScroll.heightAnchor constraintGreaterThanOrEqualToConstant:220],
        [shots.topAnchor constraintEqualToAnchor:shotsScroll.contentLayoutGuide.topAnchor],
        [shots.bottomAnchor constraintEqualToAnchor:shotsScroll.contentLayoutGuide.bottomAnchor],
        [shots.leadingAnchor constraintEqualToAnchor:shotsScroll.contentLayoutGuide.leadingAnchor constant:margin],
        [shots.trailingAnchor constraintEqualToAnchor:shotsScroll.contentLayoutGuide.trailingAnchor constant:-margin],
        [shots.heightAnchor constraintEqualToAnchor:shotsScroll.frameLayoutGuide.heightAnchor],
    ]];

    //Texts
    UILabel *pitchLbl=[self labelWithText:pitchTxt font:[UIFont systemFontOfSize:16 weight:UIFontWeightRegular] color:textColor];
    UILabel *platformsLbl=[self labelWithText:platformsTxt font:[UIFont systemFontOfSize:13 weight:UIFontWeightMedium] color:dimColor];
    UIStackView *texts=[[UIStackView alloc] initWithArrangedSubviews:@[pitchLbl,platformsLbl]];
    texts.axis=UILayoutConstraintAxisVertical;
    texts.spacing=8;
    texts.layoutMarginsRelativeArrangement=YES;
    texts.directionalLayoutMargins=NSDirectionalEdgeInsetsMake(0, margin, 0, margin);
    [content addArrangedSubview:texts];

    //Buttons
    UIButton *getBtn=[UIButton buttonWithType:UIButtonTypeSystem];
    [getBtn setTitle:NSLocalizedString(@"Get Rewamp",@"") forState:UIControlStateNormal];
    [getBtn setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    getBtn.titleLabel.font=[UIFont systemFontOfSize:18 weight:UIFontWeightBold];
    getBtn.backgroundColor=accentColor;
    getBtn.layer.cornerRadius=14;
    getBtn.layer.cornerCurve=kCACornerCurveContinuous;
    [getBtn.heightAnchor constraintEqualToConstant:52].active=YES;
    [getBtn addTarget:self action:@selector(getPushed) forControlEvents:UIControlEventTouchUpInside];

    UIButton *laterBtn=[UIButton buttonWithType:UIButtonTypeSystem];
    [laterBtn setTitle:(self.showFindLaterNote?NSLocalizedString(@"Later",@""):NSLocalizedString(@"Close",@"")) forState:UIControlStateNormal];
    [laterBtn setTitleColor:dimColor forState:UIControlStateNormal];
    laterBtn.titleLabel.font=[UIFont systemFontOfSize:16 weight:UIFontWeightMedium];
    [laterBtn.heightAnchor constraintEqualToConstant:40].active=YES;
    [laterBtn addTarget:self action:@selector(laterPushed) forControlEvents:UIControlEventTouchUpInside];

    UIStackView *buttons=[[UIStackView alloc] initWithArrangedSubviews:@[getBtn,laterBtn]];
    buttons.axis=UILayoutConstraintAxisVertical;
    buttons.spacing=4;
    buttons.layoutMarginsRelativeArrangement=YES;
    buttons.directionalLayoutMargins=NSDirectionalEdgeInsetsMake(4, margin, 0, margin);

    if (self.showFindLaterNote && laterTxt.length) {
        UILabel *laterLbl=[self labelWithText:laterTxt font:[UIFont systemFontOfSize:12 weight:UIFontWeightRegular] color:[UIColor colorWithWhite:1 alpha:0.45]];
        laterLbl.textAlignment=NSTextAlignmentCenter;
        [buttons addArrangedSubview:laterLbl];
    }
    [content addArrangedSubview:buttons];
}

- (void)getPushed {
    [self dismissViewControllerAnimated:YES completion:^{
        [RewampPromoViewController openRewamp];
    }];
}

- (void)laterPushed {
    [self dismissViewControllerAnimated:YES completion:nil];
}

@end
