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
    UIAlertAction* closeAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Close",@"") style:UIAlertActionStyleCancel
        handler:^(UIAlertAction * action) {
        }];
    [alertC addAction:closeAction];
    [self showAlert:alertC];
}

-(void) showAlertMsgAction:(NSString*)title message:(NSString*)message block:(void (^)(UIAlertAction *action))block {
    UIAlertController *alertC = [UIAlertController alertControllerWithTitle:title
                                       message:message
                                       preferredStyle:UIAlertControllerStyleAlert];
    UIAlertAction* closeAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Close",@"") style:UIAlertActionStyleCancel
        handler:block];
    [alertC addAction:closeAction];
    [self showAlert:alertC];
}


- (void)showToast:(NSString *)message
         duration:(NSTimeInterval)duration
nearPoint:(CGPoint)touchPoint {
    
    UILabel *toastLabel = [[UILabel alloc] init];
    toastLabel.text = message;
    toastLabel.font = [UIFont systemFontOfSize:14 weight:UIFontWeightMedium];
    toastLabel.textColor = [UIColor whiteColor];
    toastLabel.numberOfLines = 0;
    
    // Adaptation dark mode
    if (@available(iOS 13.0, *)) {
        toastLabel.backgroundColor = [[UIColor labelColor] colorWithAlphaComponent:0.9];
        toastLabel.textColor = [UIColor systemBackgroundColor];
    } else {
        toastLabel.backgroundColor = [[UIColor blackColor] colorWithAlphaComponent:0.8];
    }
    
    toastLabel.textAlignment = NSTextAlignmentCenter;
    toastLabel.layer.cornerRadius = 12;
    toastLabel.clipsToBounds = YES;
    
    // Calculer la taille
    CGFloat padding = 16;
    CGSize maxSize = CGSizeMake(self.view.bounds.size.width - (padding * 4), 1000);
    CGSize textSize = [message boundingRectWithSize:maxSize
                                            options:NSStringDrawingUsesLineFragmentOrigin
                                         attributes:@{NSFontAttributeName: toastLabel.font}
                                            context:nil].size;
    
    CGFloat width = textSize.width + (padding * 2);
    CGFloat height = textSize.height + (padding * 1.5);
    
    // Déterminer la position optimale en fonction du touch
    CGFloat yPosition;
    CGFloat screenHeight = self.view.bounds.size.height;
    CGFloat screenMidpoint = screenHeight / 2;
    CGFloat offset = 20; // Espace entre le toast et le point de touch
    
    // Si le touch est dans la moitié supérieure de l'écran
    if (touchPoint.y < screenMidpoint) {
        // Placer le toast EN DESSOUS du point de touch
        yPosition = touchPoint.y + offset;
        
        // Vérifier qu'on ne dépasse pas en bas
        if (yPosition + height > screenHeight - 40) {
            yPosition = screenHeight - height - 40;
        }
    } else {
        // Touch dans la moitié inférieure : placer le toast AU-DESSUS
        yPosition = touchPoint.y - height - offset;
        
        // Vérifier qu'on ne dépasse pas en haut
        if (yPosition < 40) {
            yPosition = 40;
        }
    }
    
    // Position X centrée
    CGFloat xPosition = touchPoint.x - width;//(self.view.bounds.size.width - width) / 2;
    if (xPosition<0) xPosition=0;
    
    toastLabel.frame = CGRectMake(xPosition, yPosition, width, height);
    toastLabel.alpha = 0.0;
    toastLabel.transform = CGAffineTransformMakeScale(0.8, 0.8);
    
    [self.view addSubview:toastLabel];
    
    // Animation d'apparition
    [UIView animateWithDuration:0.3 delay:0.0
         usingSpringWithDamping:0.7
          initialSpringVelocity:0.5
                        options:UIViewAnimationOptionCurveEaseOut
                     animations:^{
        toastLabel.alpha = 1.0;
        toastLabel.transform = CGAffineTransformIdentity;
    } completion:^(BOOL finished) {
        // Disparition après la durée spécifiée
        [UIView animateWithDuration:0.3 delay:duration options:0 animations:^{
            toastLabel.alpha = 0.0;
            toastLabel.transform = CGAffineTransformMakeScale(0.9, 0.9);
        } completion:^(BOOL finished) {
            [toastLabel removeFromSuperview];
        }];
    }];
}


#endif /* AlertsCommonFunctions_h */
