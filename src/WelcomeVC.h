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
    IBOutlet UIButton *leftBtn,*rightBtn;
    IBOutlet UILabel *topLabel;
    IBOutlet UIImageView *imageView1;
    IBOutlet UIImageView *imageView2;
    IBOutlet UIImageView *imageView3;
    IBOutlet UIImageView *imageView4;
    IBOutlet UILabel *messageLabel;
}

@property (nonatomic, retain) IBOutlet UIButton *exitBtn;
@property (nonatomic, retain) IBOutlet UIButton *leftBtn,*rightBtn;
@property (nonatomic, retain) IBOutlet UILabel *topLabel;
@property (nonatomic, retain) IBOutlet UIImageView *imageView1,*imageView2,*imageView3,*imageView4;
@property (nonatomic, retain) IBOutlet UILabel *messageLabel;

@end

NS_ASSUME_NONNULL_END
