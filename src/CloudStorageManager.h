//
//  CloudStorageManager.h
//  modizer
//
//  Created for multi-cloud storage support
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "CloudStorageSource.h"

NS_ASSUME_NONNULL_BEGIN

extern NSString * const CloudStorageSourcesDidUpdateNotification;
extern NSString * const CloudStorageReadyNotification;
extern NSString * const CloudStorageErrorNotification;
extern NSString * const CloudStorageFileDownloadedNotification;
extern NSString * const CloudStorageErrorSourceKey;
extern NSString * const CloudStorageErrorMessageKey;
extern NSString * const CloudStorageFilePathKey;

@class CloudStorageManager;

@protocol CloudStorageManagerDelegate <NSObject>
@optional
- (void)cloudStorageSourcesDidUpdate;
- (void)cloudStorageDidFailWithError:(NSError *)error source:(nullable CloudStorageSource *)source;
- (void)cloudStorageManagerDidInitialize:(BOOL)success;
@end

typedef void (^CloudStorageCompletionBlock)(BOOL success);
typedef void (^CloudStorageListCompletionBlock)(NSArray * _Nullable items, NSError * _Nullable error);
typedef void (^CloudStorageFolderPickerCompletionBlock)(NSURL * _Nullable url, NSError * _Nullable error);

@interface CloudStorageManager : NSObject <UIDocumentPickerDelegate>

@property (nonatomic, weak, nullable) id<CloudStorageManagerDelegate> delegate;
@property (nonatomic, strong, readonly) NSArray<CloudStorageSource *> *sources;
@property (nonatomic, assign, readonly) BOOL isInitialized;
@property (nonatomic, strong, readonly, nullable) CloudStorageSource *nativeICloudSource;

+ (instancetype)sharedManager;

#pragma mark - Initialization

/// Initialize the cloud storage manager asynchronously (call on app launch from background thread)
- (void)initializeWithCompletion:(nullable CloudStorageCompletionBlock)completion;

/// Reinitialize and refresh all sources
- (void)refreshAllSourcesWithCompletion:(nullable CloudStorageCompletionBlock)completion;

#pragma mark - Source Management

/// Add a new cloud source from a URL (typically from UIDocumentPicker)
- (CloudStorageSource *)addSourceWithURL:(NSURL *)url name:(NSString *)name type:(CloudStorageType)type;

/// Add a source with auto-detected type
- (CloudStorageSource *)addSourceWithURL:(NSURL *)url name:(NSString *)name;

/// Remove a cloud source
- (void)removeSource:(CloudStorageSource *)source;

/// Get source for a given file path
- (nullable CloudStorageSource *)sourceForPath:(NSString *)path;

/// Get source by identifier
- (nullable CloudStorageSource *)sourceWithIdentifier:(NSString *)identifier;

/// Check if native iCloud is available
- (BOOL)isNativeICloudAvailable;

#pragma mark - Security-Scoped Access

/// Start accessing a security-scoped source (must call before file operations)
- (BOOL)startAccessingSource:(CloudStorageSource *)source;

/// Stop accessing a security-scoped source (must call after file operations)
- (void)stopAccessingSource:(CloudStorageSource *)source;

#pragma mark - File Operations (async)

/// List contents at path within a cloud source
- (void)listContentsAtPath:(NSString *)path
                    source:(CloudStorageSource *)source
                completion:(CloudStorageListCompletionBlock)completion;

/// Check if a path is a cloud path (any registered source)
- (BOOL)isCloudPath:(NSString *)path;

/// Check if a file has been downloaded locally (for ubiquitous items)
- (BOOL)isFileDownloaded:(NSURL *)url;

/// Trigger download for a ubiquitous item
- (BOOL)startDownloadingFile:(NSURL *)url error:(NSError * _Nullable * _Nullable)error;

#pragma mark - Document Picker

/// Present folder picker to add a new cloud source
- (void)presentFolderPickerFrom:(UIViewController *)viewController
                     completion:(CloudStorageFolderPickerCompletionBlock)completion;

#pragma mark - Persistence

/// Save all source bookmarks to NSUserDefaults
- (void)saveSourceBookmarks;

/// Load source bookmarks from NSUserDefaults
- (void)loadSourceBookmarks;

/// Clear all saved sources (for debugging/reset)
- (void)clearAllSources;

@end

NS_ASSUME_NONNULL_END
