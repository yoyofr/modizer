//
//  TipsViewController.mm
//  modizer
//
//  Created by Yohann Magnien David on 17/11/2025.
//

#import "TipsViewController.h"
#import "StoreManager.h"
#import "ModizerConstants.h"

typedef NS_ENUM(NSInteger, TipTier) {
    TipTierSmall = 0,
    TipTierMedium,
    TipTierLarge,
    TipTierGenerous
};

@interface TipButton : UIButton
@property (nonatomic, strong) SKProduct *product;
@end

@implementation TipButton
@end

@interface TipsViewController () <StoreManagerDelegate>
@property (nonatomic, strong) UIScrollView *scrollView;
@property (nonatomic, strong) UIStackView *mainStackView;
@property (nonatomic, strong) UIActivityIndicatorView *loadingIndicator;
@property (nonatomic, strong) UILabel *headerLabel;
@property (nonatomic, strong) UILabel *subheaderLabel;
@property (nonatomic, strong) UILabel *alreadyTippedLabel;
@property (nonatomic, strong) NSArray<TipButton *> *tipButtons;
@property (nonatomic, strong) NSArray<SKProduct *> *products;
@property (nonatomic, strong) NSNumberFormatter *priceFormatter;
@property (nonatomic, strong) UIView *loadingOverlay;
@end

@implementation TipsViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    
    self.title = NSLocalizedString(@"Support with a Tip", @"");
    self.view.backgroundColor = [UIColor systemBackgroundColor];
    
    if ([NSProcessInfo processInfo].isiOSAppOnMac) {
        self.hidesBottomBarWhenPushed = YES;
    } else if (@available(iOS 18.0, *)) {
        if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad) {
            self.hidesBottomBarWhenPushed = YES;
        }
    }
    
    [self setupPriceFormatter];
    [self setupUI];
    [self loadProducts];
}

- (void)setupPriceFormatter {
    self.priceFormatter = [[NSNumberFormatter alloc] init];
    self.priceFormatter.numberStyle = NSNumberFormatterCurrencyStyle;
}

- (void)setupUI {
    // Setup scroll view
    self.scrollView = [[UIScrollView alloc] init];
    self.scrollView.translatesAutoresizingMaskIntoConstraints = NO;
    [self.view addSubview:self.scrollView];
    
    // Add pull-to-refresh
        UIRefreshControl *refreshControl = [[UIRefreshControl alloc] init];
        [refreshControl addTarget:self action:@selector(pullToRefresh:) forControlEvents:UIControlEventValueChanged];
        refreshControl.tintColor = [UIColor systemBlueColor];
        
        // For iOS 10+
        if (@available(iOS 10.0, *)) {
            self.scrollView.refreshControl = refreshControl;
        } else {
            [self.scrollView addSubview:refreshControl];
        }
        
    
    // Main stack view
    self.mainStackView = [[UIStackView alloc] init];
    self.mainStackView.axis = UILayoutConstraintAxisVertical;
    self.mainStackView.alignment = UIStackViewAlignmentFill;
    self.mainStackView.distribution = UIStackViewDistributionEqualSpacing;
    self.mainStackView.spacing = 20;
    self.mainStackView.translatesAutoresizingMaskIntoConstraints = NO;
    [self.scrollView addSubview:self.mainStackView];
    
    // Header
    self.headerLabel = [[UILabel alloc] init];
    self.headerLabel.text = NSLocalizedString(@"Enjoy Modizer ?", @"");
    self.headerLabel.font = [UIFont systemFontOfSize:28 weight:UIFontWeightBold];
    self.headerLabel.textAlignment = NSTextAlignmentCenter;
    
    self.subheaderLabel = [[UILabel alloc] init];
    self.subheaderLabel.text = NSLocalizedString(@"Your support helps keep Modizer running and ad-free!", @"");
    self.subheaderLabel.font = [UIFont systemFontOfSize:16];
    self.subheaderLabel.textColor = [UIColor secondaryLabelColor];
    self.subheaderLabel.textAlignment = NSTextAlignmentCenter;
    self.subheaderLabel.numberOfLines = 0;
    
    // Add headers to stack
    [self.mainStackView addArrangedSubview:self.headerLabel];
    [self.mainStackView addArrangedSubview:self.subheaderLabel];
    
    self.alreadyTippedLabel = [[UILabel alloc] init];
    self.alreadyTippedLabel.text = [NSString stringWithFormat:@"%@: %@",NSLocalizedString(@"Already tipped so far",@""),[[StoreManager sharedManager] formattedTipsTotalWithLocale:[NSLocale autoupdatingCurrentLocale]]];
    self.alreadyTippedLabel.font = [UIFont systemFontOfSize:16];
    self.alreadyTippedLabel.textColor = [UIColor secondaryLabelColor];
    self.alreadyTippedLabel.textAlignment = NSTextAlignmentCenter;
    self.alreadyTippedLabel.numberOfLines = 0;
    
    // Add already tipped info to stack
    [self.mainStackView addArrangedSubview:self.alreadyTippedLabel];
    
    
    
    
    // Add spacing
    UIView *spacer1 = [[UIView alloc] init];
    [spacer1.heightAnchor constraintEqualToConstant:20].active = YES;
    [self.mainStackView addArrangedSubview:spacer1];
    
    // Create placeholder tip buttons (will be updated when products load)
    [self createTipButtons];
    
    // Thank you message (initially hidden)
    UILabel *thankYouLabel = [[UILabel alloc] init];
    thankYouLabel.text = NSLocalizedString(@"Thank you for your support! ❤️", @"");
    thankYouLabel.font = [UIFont systemFontOfSize:14];
    thankYouLabel.textColor = [UIColor secondaryLabelColor];
    thankYouLabel.textAlignment = NSTextAlignmentCenter;
    thankYouLabel.hidden = YES;
    thankYouLabel.tag = 999; // Tag for later reference
    [self.mainStackView addArrangedSubview:thankYouLabel];
    
    // Loading indicator
    self.loadingIndicator = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleLarge];
    self.loadingIndicator.translatesAutoresizingMaskIntoConstraints = NO;
    self.loadingIndicator.hidesWhenStopped = YES;
    [self.view addSubview:self.loadingIndicator];
    
    // Setup constraints
    [NSLayoutConstraint activateConstraints:@[
        // Scroll view
        [self.scrollView.topAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.topAnchor],
        [self.scrollView.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor],
        [self.scrollView.trailingAnchor constraintEqualToAnchor:self.view.trailingAnchor],
        [self.scrollView.bottomAnchor constraintEqualToAnchor:self.view.bottomAnchor],
        
        // Stack view
        [self.mainStackView.topAnchor constraintEqualToAnchor:self.scrollView.topAnchor constant:30],
        [self.mainStackView.leadingAnchor constraintEqualToAnchor:self.scrollView.leadingAnchor constant:20],
        [self.mainStackView.trailingAnchor constraintEqualToAnchor:self.scrollView.trailingAnchor constant:-20],
        [self.mainStackView.bottomAnchor constraintEqualToAnchor:self.scrollView.bottomAnchor constant:-30],
        [self.mainStackView.widthAnchor constraintEqualToAnchor:self.scrollView.widthAnchor constant:-40],
        
        // Loading indicator
        [self.loadingIndicator.centerXAnchor constraintEqualToAnchor:self.view.centerXAnchor],
        [self.loadingIndicator.centerYAnchor constraintEqualToAnchor:self.view.centerYAnchor]
    ]];
    
    // Add close button if presented modally
    if (self.presentingViewController) {
        self.navigationItem.leftBarButtonItem = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemDone
                                                                                               target:self
                                                                                               action:@selector(dismissViewController)];
    }
}

- (void)pullToRefresh:(UIRefreshControl *)refreshControl {
    //NSLog(@"User triggered pull-to-refresh");
    
    // Force refresh products
    [[StoreManager sharedManager] refreshProducts];
    
    // End refreshing will be called when products are loaded
}



- (void)createTipButtons {
    NSMutableArray<TipButton *> *buttons = [NSMutableArray array];
    
    NSArray *configs = @[
        @{@"emoji": @"☕", @"title": NSLocalizedString(@"Coffee", @""), @"subtitle": NSLocalizedString(@"Loading...", @""), @"color": [UIColor systemBrownColor]},
        @{@"emoji": @"🥐", @"title": NSLocalizedString(@"Coffee & Croissant", @""), @"subtitle": NSLocalizedString(@"Loading...", @""), @"color": [UIColor systemOrangeColor]},
        //@{@"emoji": @"🍔", @"title": @"Lunch", @"subtitle": NSLocalizedString(@"Loading...", @""), @"color": [UIColor systemBlueColor]},
        @{@"emoji": @"🍽️", @"title": NSLocalizedString(@"Dinner", @""), @"subtitle": NSLocalizedString(@"Loading...", @""), @"color": [UIColor systemPurpleColor]}
    ];
    
    for (NSDictionary *config in configs) {
        TipButton *button = [self createTipButtonWithConfig:config];
        [buttons addObject:button];
        [self.mainStackView addArrangedSubview:button];
    }
    
    self.tipButtons = buttons;
}

- (TipButton *)createTipButtonWithConfig:(NSDictionary *)config {
    TipButton *button = [TipButton buttonWithType:UIButtonTypeSystem];
    
    // Container stack view
    UIStackView *containerStack = [[UIStackView alloc] init];
    containerStack.axis = UILayoutConstraintAxisHorizontal;
    containerStack.alignment = UIStackViewAlignmentCenter;
    containerStack.distribution = UIStackViewDistributionFill;
    containerStack.spacing = 15;
    containerStack.userInteractionEnabled = NO;
    containerStack.translatesAutoresizingMaskIntoConstraints = NO;
    
    // Emoji label
    UILabel *emojiLabel = [[UILabel alloc] init];
    emojiLabel.text = config[@"emoji"];
    emojiLabel.font = [UIFont systemFontOfSize:32];
    emojiLabel.textAlignment = NSTextAlignmentCenter;
    [emojiLabel.widthAnchor constraintEqualToConstant:44].active = YES;
    
    // Text stack view
    UIStackView *textStack = [[UIStackView alloc] init];
    textStack.axis = UILayoutConstraintAxisVertical;
    textStack.alignment = UIStackViewAlignmentLeading;
    textStack.distribution = UIStackViewDistributionFillEqually;
    textStack.spacing = 2;
    
    // Title label
    UILabel *titleLabel = [[UILabel alloc] init];
    titleLabel.text = config[@"title"];
    titleLabel.font = [UIFont systemFontOfSize:17 weight:UIFontWeightMedium];
    titleLabel.textColor = [UIColor labelColor];
    titleLabel.tag = 100; // Tag for updating later
    
    // Price label (initially shows "Loading...")
    UILabel *priceLabel = [[UILabel alloc] init];
    priceLabel.text = config[@"subtitle"];
    priceLabel.font = [UIFont systemFontOfSize:15];
    priceLabel.textColor = [UIColor secondaryLabelColor];
    priceLabel.tag = 101; // Tag for updating later
    
    [textStack addArrangedSubview:titleLabel];
    [textStack addArrangedSubview:priceLabel];
    
    // Arrow indicator
    UIImageView *arrowView = [[UIImageView alloc] initWithImage:[UIImage systemImageNamed:@"chevron.right"]];
    arrowView.tintColor = [UIColor tertiaryLabelColor];
    [arrowView.widthAnchor constraintEqualToConstant:20].active = YES;
    
    // Add to container
    [containerStack addArrangedSubview:emojiLabel];
    [containerStack addArrangedSubview:textStack];
    
    UIView *spacerView = [[UIView alloc] init];
    [spacerView setContentHuggingPriority:UILayoutPriorityDefaultLow forAxis:UILayoutConstraintAxisHorizontal];
    [containerStack addArrangedSubview:spacerView];
    
    [containerStack addArrangedSubview:arrowView];
    
    // Add container to button
    [button addSubview:containerStack];
    
    // Container constraints
    [NSLayoutConstraint activateConstraints:@[
        [containerStack.topAnchor constraintEqualToAnchor:button.topAnchor constant:16],
        [containerStack.leadingAnchor constraintEqualToAnchor:button.leadingAnchor constant:16],
        [containerStack.trailingAnchor constraintEqualToAnchor:button.trailingAnchor constant:-16],
        [containerStack.bottomAnchor constraintEqualToAnchor:button.bottomAnchor constant:-16]
    ]];
    
    // Button styling
    button.backgroundColor = [UIColor secondarySystemBackgroundColor];
    button.layer.cornerRadius = 12;
    button.layer.masksToBounds = YES;
    
    // Height constraint
    [button.heightAnchor constraintGreaterThanOrEqualToConstant:76].active = YES;
    
    // Disable initially
    button.enabled = NO;
    button.alpha = 0.6;
    
    // Add target
    [button addTarget:self action:@selector(tipButtonTapped:) forControlEvents:UIControlEventTouchUpInside];
    
    return button;
}

- (void)loadProducts {
    [self.loadingIndicator startAnimating];
    [StoreManager sharedManager].delegate = self;
    [[StoreManager sharedManager] loadProducts];
}

- (void)updateButtonsWithProducts {
    // Sort products by price
    NSArray *sortedProducts = [self.products sortedArrayUsingComparator:^NSComparisonResult(SKProduct *obj1, SKProduct *obj2) {
        return [obj1.price compare:obj2.price];
    }];
    
    // Update each button with corresponding product
    for (NSInteger i = 0; i < MIN(self.tipButtons.count, sortedProducts.count); i++) {
        TipButton *button = self.tipButtons[i];
        SKProduct *product = sortedProducts[i];
        
        button.product = product;
        
        // Update price label
        UILabel *priceLabel = [button viewWithTag:101];
        if (priceLabel) {
            self.priceFormatter.locale = product.priceLocale;
            NSString *formattedPrice = [self.priceFormatter stringFromNumber:product.price];
            priceLabel.text = formattedPrice;
            priceLabel.textColor = [UIColor systemBlueColor];
            priceLabel.font = [UIFont systemFontOfSize:16 weight:MDZ_UIFONT_WEIGHT];
        }
        
        // Enable button
        button.enabled = YES;
        button.alpha = 1.0;
        
        // Add hover effect for enabled buttons
        [self addHighlightEffectToButton:button];
    }
}

- (void)addHighlightEffectToButton:(UIButton *)button {
    [button addTarget:self action:@selector(buttonTouchDown:) forControlEvents:UIControlEventTouchDown];
    [button addTarget:self action:@selector(buttonTouchUp:) forControlEvents:UIControlEventTouchUpInside | UIControlEventTouchUpOutside];
}

- (void)buttonTouchDown:(UIButton *)button {
    [UIView animateWithDuration:0.1 animations:^{
        button.transform = CGAffineTransformMakeScale(0.97, 0.97);
        button.alpha = 0.8;
    }];
}

- (void)buttonTouchUp:(UIButton *)button {
    [UIView animateWithDuration:0.1 animations:^{
        button.transform = CGAffineTransformIdentity;
        button.alpha = 1.0;
    }];
}

- (void)tipButtonTapped:(TipButton *)sender {
    if (!sender.product) return;
    
    // Format price for confirmation
    self.priceFormatter.locale = sender.product.priceLocale;
    NSString *formattedPrice = [self.priceFormatter stringFromNumber:sender.product.price];
    
    // Get title from button
    UILabel *titleLabel = [sender viewWithTag:100];
    NSString *tipName = titleLabel.text ?: NSLocalizedString(@"Tip", @"");
    
    // Show confirmation
    UIAlertController *alert = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Confirm Tip", @"")
                                                                   message:[NSString stringWithFormat:NSLocalizedString(@"Send a %@ tip for %@?", @""), tipName, formattedPrice]
                                                            preferredStyle:UIAlertControllerStyleAlert];
    
    [alert addAction:[UIAlertAction actionWithTitle:NSLocalizedString(@"Cancel", @"")
                                              style:UIAlertActionStyleCancel
                                            handler:nil]];
    
    [alert addAction:[UIAlertAction actionWithTitle:NSLocalizedString(@"Send Tip", @"")
                                              style:UIAlertActionStyleDefault
                                            handler:^(UIAlertAction *action) {
        [self purchaseProduct:sender.product];
    }]];
    
    [self presentViewController:alert animated:YES completion:nil];
}

- (void)purchaseProduct:(SKProduct *)product {
    [self showLoadingOverlay];
    [[StoreManager sharedManager] purchaseProduct:product];
}

- (void)showLoadingOverlay {
    if (!self.loadingOverlay) {
        self.loadingOverlay = [[UIView alloc] initWithFrame:self.view.bounds];
        self.loadingOverlay.backgroundColor = [[UIColor blackColor] colorWithAlphaComponent:0.5];
        self.loadingOverlay.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
        
        UIActivityIndicatorView *indicator = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleLarge];
        indicator.center = self.loadingOverlay.center;
        indicator.autoresizingMask = UIViewAutoresizingFlexibleTopMargin | UIViewAutoresizingFlexibleBottomMargin |
                                     UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin;
        [indicator startAnimating];
        [self.loadingOverlay addSubview:indicator];
    }
    
    [self.view addSubview:self.loadingOverlay];
    self.loadingOverlay.alpha = 0;
    [UIView animateWithDuration:0.3 animations:^{
        self.loadingOverlay.alpha = 1.0;
    }];
}

- (void)hideLoadingOverlay {
    [UIView animateWithDuration:0.3 animations:^{
        self.loadingOverlay.alpha = 0;
    } completion:^(BOOL finished) {
        [self.loadingOverlay removeFromSuperview];
    }];
}

- (void)dismissViewController {
    [self dismissViewControllerAnimated:YES completion:nil];
}

#pragma mark - StoreManagerDelegate

// Update the productsLoaded delegate method:
- (void)productsLoaded:(NSArray<SKProduct *> *)products {
    self.products = products;
    [self.loadingIndicator stopAnimating];
    
    // End refresh control if active
    if (@available(iOS 10.0, *)) {
        [self.scrollView.refreshControl endRefreshing];
    } else {
        for (UIView *subview in self.scrollView.subviews) {
            if ([subview isKindOfClass:[UIRefreshControl class]]) {
                [(UIRefreshControl *)subview endRefreshing];
                break;
            }
        }
    }
    
    if (products.count > 0) {
        [self updateButtonsWithProducts];
    } else {
        [self showErrorMessage:NSLocalizedString(@"Unable to load products. Please check your internet connection.", @"")];
    }
}

- (void)productPurchased:(NSString *)productIdentifier {
    [self hideLoadingOverlay];
    
    // Update the already tipped label
    self.alreadyTippedLabel.text = [NSString stringWithFormat:@"%@: %@",
                                    NSLocalizedString(@"Already tipped so far",@""),
                                    [[StoreManager sharedManager] formattedTipsTotalWithLocale:[NSLocale autoupdatingCurrentLocale]]];
    
    // Show thank you with animation
    UIView *thankYouView = [self.view viewWithTag:999];
    if (thankYouView) {
        thankYouView.hidden = NO;
        thankYouView.alpha = 0;
        [UIView animateWithDuration:0.5 animations:^{
            thankYouView.alpha = 1.0;
        }];
    }
    
    // Show success alert
    UIAlertController *alert = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Thank You! ❤️",@"")
                                                                   message:NSLocalizedString(@"Your support means everything! Thank you for helping keep Modizer running.",@"")
                                                            preferredStyle:UIAlertControllerStyleAlert];
    
    [alert addAction:[UIAlertAction actionWithTitle:NSLocalizedString(@"Close",@"")
                                              style:UIAlertActionStyleDefault
                                            handler:nil]];
    
    [self presentViewController:alert animated:YES completion:nil];
}

- (void)purchaseFailed:(NSError *)error {
    [self hideLoadingOverlay];
    
    if (error.code != SKErrorPaymentCancelled) {
        [self showErrorMessage:error.localizedDescription];
    }
}

- (void)showErrorMessage:(NSString *)message {
    UIAlertController *alert = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Error", @"")
                                                                   message:message
                                                            preferredStyle:UIAlertControllerStyleAlert];
    [alert addAction:[UIAlertAction actionWithTitle:NSLocalizedString(@"OK", @"")
                                              style:UIAlertActionStyleDefault
                                            handler:nil]];
    [self presentViewController:alert animated:YES completion:nil];
}

@end
