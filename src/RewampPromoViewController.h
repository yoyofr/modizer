//
//  RewampPromoViewController.h
//  modizer
//
//  Visual promotion sheet for Rewamp (shown once at first launch of v4.8,
//  and on demand from the "More" tab).
//

#import <UIKit/UIKit.h>

@interface RewampPromoViewController : UIViewController

// YES for the one-time launch popup: adds a note telling where to find the link later
@property (nonatomic, assign) BOOL showFindLaterNote;

// Opens the App Store page (iOS/iPadOS) or the website (when running on a Mac)
+ (void)openRewamp;

@end
