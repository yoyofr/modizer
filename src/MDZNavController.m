//
//  MDZNavController.m
//  modizer
//
//  Created by Yohann Magnien on 17/04/2021.
//

#import "MDZNavController.h"

@interface MDZNavController ()

@end

@implementation MDZNavController

-(UIViewController *)childViewControllerForStatusBarStyle {
     return self.visibleViewController;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
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
