//
//  WelcomeVC.h
//  modizer
//
//  Created by Yohann Magnien David on 16/11/2025.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface WelcomeVC : UIViewController {
    IBOutlet UIButton *exitBtn;
    
    IBOutlet UILabel *topLabel;
    IBOutlet UIImageView *image;
    IBOutlet UILabel *messageLabel;
}

@property (nonatomic, retain) IBOutlet UIButton *exitBtn;
@property (nonatomic, retain) IBOutlet UILabel *topLabel;
@property (nonatomic, retain) IBOutlet UIImageView *image;
@property (nonatomic, retain) IBOutlet UILabel *messageLabel;

@end

NS_ASSUME_NONNULL_END
