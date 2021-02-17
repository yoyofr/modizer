//
// Util.h
//
// Copyright (c) 2018 Taner Sener
//
// This file is part of MobileFFmpeg.
//
// MobileFFmpeg is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// MobileFFmpeg is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License
//  along with MobileFFmpeg.  If not, see <http://www.gnu.org/licenses/>.
//

#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
#import "RCEasyTipView.h"

@interface Util : NSObject

+ (void)applyButtonStyle: (UIButton*) button;
+ (void)applyEditTextStyle: (UITextField*) textField;
+ (void)applyHeaderStyle: (UILabel*) label;
+ (void)applyOutputTextStyle: (UITextView*) textView;
+ (void)applyPickerViewStyle: (UIPickerView*) pickerView;
+ (void)applyVideoPlayerFrameStyle: (UILabel*) playerFrame;
+ (void)applyTooltipStyle: (RCEasyTipPreferences*)preferences;
+ (void)alert: (UIViewController*)controller withTitle:(NSString*)title message:(NSString*)message andButtonText:(NSString*)buttonText;

@end
