//
//  AlertsCommonFunctions_h
//  modizer
//
//  Created by Yohann Magnien on 04/05/2021.
//

#ifndef AlertsCommonFunctions_h
#define AlertsCommonFunctions_h

-(UIViewController *)visibleViewController:(UIViewController *)rootViewController {
    if ([rootViewController isKindOfClass:[UITabBarController class]]) {
        UIViewController *selectedViewController = ((UITabBarController *)rootViewController).selectedViewController;

        return [self visibleViewController:selectedViewController];
    }
    if ([rootViewController isKindOfClass:[UINavigationController class]]) {
        UIViewController *lastViewController = [[((UINavigationController *)rootViewController) viewControllers] lastObject];

        return [self visibleViewController:lastViewController];
    }
    
    if (rootViewController.presentedViewController == nil) {
        return rootViewController;
    }
    if ([rootViewController.presentedViewController isKindOfClass:[UINavigationController class]]) {
        UINavigationController *navigationController = (UINavigationController *)rootViewController.presentedViewController;
        UIViewController *lastViewController = [[navigationController viewControllers] lastObject];

        return [self visibleViewController:lastViewController];
    }
    if ([rootViewController.presentedViewController isKindOfClass:[UITabBarController class]]) {
        UITabBarController *tabBarController = (UITabBarController *)rootViewController.presentedViewController;
        UIViewController *selectedViewController = tabBarController.selectedViewController;

        return [self visibleViewController:selectedViewController];
    }

    UIViewController *presentedViewController = (UIViewController *)rootViewController.presentedViewController;

    return [self visibleViewController:presentedViewController];
}


-(void) showAlert:(UIAlertController*)alertC {
    if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPhone) { //if iPhone
        [self presentViewController:alertC animated:YES completion:nil];
    } else { //if iPad
        alertC.modalPresentationStyle = UIModalPresentationPopover;
        alertC.popoverPresentationController.sourceView = self.view;
        alertC.popoverPresentationController.sourceRect = CGRectMake(self.view.frame.size.width/3, self.view.frame.size.height/2, 0, 0);
        alertC.popoverPresentationController.permittedArrowDirections=0;
        [self presentViewController:alertC animated:YES completion:nil];
    }
}

-(void) showAlertMsg:(NSString*)title message:(NSString*)message {
    UIAlertController *alertC = [UIAlertController alertControllerWithTitle:title
                                       message:message
                                       preferredStyle:UIAlertControllerStyleAlert];
    UIAlertAction* closeAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Ok",@"") style:UIAlertActionStyleCancel
        handler:^(UIAlertAction * action) {
        }];
    [alertC addAction:closeAction];
    [self showAlert:alertC];
}

#endif /* AlertsCommonFunctions_h */
