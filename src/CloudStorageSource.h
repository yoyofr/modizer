//
//  CloudStorageSource.h
//  modizer
//
//  Created for multi-cloud storage support
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, CloudStorageType) {
    CloudStorageTypeICloudNative = 0,
    CloudStorageTypeICloudDrive,
    CloudStorageTypeGoogleDrive,
    CloudStorageTypeDropbox,
    CloudStorageTypeOneDrive,
    CloudStorageTypeOther
};

@interface CloudStorageSource : NSObject <NSSecureCoding>

@property (nonatomic, strong) NSString *identifier;
@property (nonatomic, strong) NSString *name;
@property (nonatomic, assign) CloudStorageType type;
@property (nonatomic, strong, nullable) NSData *bookmarkData;
@property (nonatomic, strong, readonly, nullable) NSURL *resolvedURL;
@property (nonatomic, strong, nullable) NSURL *securityScopedURL; // Original URL for security access (may differ from resolvedURL if file was selected)
@property (nonatomic, assign) BOOL isFileBasedAccess; // YES if a file was selected to access its parent folder
@property (nonatomic, strong, nullable) UIColor *tintColor;
@property (nonatomic, assign) BOOL isAccessible;
@property (nonatomic, assign) BOOL isSecurityScoped;

- (instancetype)initWithURL:(NSURL *)url name:(NSString *)name type:(CloudStorageType)type;
- (instancetype)initWithBookmarkData:(NSData *)bookmarkData name:(NSString *)name type:(CloudStorageType)type identifier:(NSString *)identifier;

- (BOOL)resolveBookmark;
- (BOOL)createBookmarkFromURL:(NSURL *)url;
- (void)startAccessing;
- (void)stopAccessing;

+ (UIColor *)tintColorForType:(CloudStorageType)type darkMode:(BOOL)darkMode;
+ (NSString *)displayNameForType:(CloudStorageType)type;
+ (CloudStorageType)typeFromURLString:(NSString *)urlString;

@end

NS_ASSUME_NONNULL_END
