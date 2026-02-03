//
//  CloudStorageManager.m
//  modizer
//
//  Created for multi-cloud storage support
//

#import "CloudStorageManager.h"
#import "ModizerConstants.h"
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

NSString * const CloudStorageSourcesDidUpdateNotification = @"CloudStorageSourcesDidUpdateNotification";
NSString * const CloudStorageReadyNotification = @"CloudStorageReadyNotification";
NSString * const CloudStorageErrorNotification = @"CloudStorageErrorNotification";
NSString * const CloudStorageFileDownloadedNotification = @"CloudStorageFileDownloadedNotification";
NSString * const CloudStorageErrorSourceKey = @"CloudStorageErrorSourceKey";
NSString * const CloudStorageErrorMessageKey = @"CloudStorageErrorMessageKey";
NSString * const CloudStorageFilePathKey = @"CloudStorageFilePathKey";

static NSString * const kCloudStorageSourcesKey = @"CloudStorageSources_v1";

@interface CloudStorageManager ()
@property (nonatomic, strong, readwrite) NSMutableArray<CloudStorageSource *> *mutableSources;
@property (nonatomic, assign, readwrite) BOOL isInitialized;
@property (nonatomic, strong, readwrite, nullable) CloudStorageSource *nativeICloudSource;
@property (nonatomic, copy, nullable) CloudStorageFolderPickerCompletionBlock folderPickerCompletion;
@property (nonatomic, strong) NSFileManager *fileManager;
@property (nonatomic, strong, nullable) UIDocumentPickerViewController *currentPicker; // Keep strong reference
@end

@implementation CloudStorageManager

#pragma mark - Singleton

+ (instancetype)sharedManager {
    static CloudStorageManager *sharedInstance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        sharedInstance = [[CloudStorageManager alloc] init];
    });
    return sharedInstance;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        _mutableSources = [NSMutableArray array];
        _isInitialized = NO;
        _fileManager = [[NSFileManager alloc] init];
    }
    return self;
}

#pragma mark - Properties

- (NSArray<CloudStorageSource *> *)sources {
    return [self.mutableSources copy];
}

#pragma mark - Initialization

- (void)initializeWithCompletion:(CloudStorageCompletionBlock)completion {
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        MDZILog("CloudStorageManager: Starting initialization...");

        // Load saved sources from NSUserDefaults
        [self loadSourceBookmarks];

        // Initialize native iCloud if available
        [self initializeNativeICloud];

        // Resolve all bookmarks
        [self resolveAllBookmarks];

        self.isInitialized = YES;

        dispatch_async(dispatch_get_main_queue(), ^{
            MDZILog("CloudStorageManager: Initialization complete with %lu sources",
                   (unsigned long)self.mutableSources.count);

            // Notify delegate
            if ([self.delegate respondsToSelector:@selector(cloudStorageManagerDidInitialize:)]) {
                [self.delegate cloudStorageManagerDidInitialize:YES];
            }

            // Post notification
            [[NSNotificationCenter defaultCenter] postNotificationName:CloudStorageReadyNotification
                                                                object:self];

            if (completion) {
                completion(YES);
            }
        });
    });
}

- (void)initializeNativeICloud {
    // Check iCloud availability
    id ubiquityToken = [self.fileManager ubiquityIdentityToken];
    if (!ubiquityToken) {
        MDZILog("CloudStorageManager: iCloud not available (no ubiquity token)");
        return;
    }

    // Get the ubiquity container URL - this is the potentially blocking call
    NSURL *containerURL = [self.fileManager URLForUbiquityContainerIdentifier:nil];
    if (!containerURL) {
        MDZILog("CloudStorageManager: iCloud container URL not available");
        return;
    }

    NSURL *documentsURL = [containerURL URLByAppendingPathComponent:@"Documents"];
    NSString *nativePath = documentsURL.path;

    // Check if we already have this exact path in our sources (loaded from bookmarks)
    for (CloudStorageSource *source in self.mutableSources) {
        if ([source.resolvedURL.path isEqualToString:nativePath]) {
            // This manually added source is actually the native iCloud container
            // Use it as the native source
            self.nativeICloudSource = source;
            source.isSecurityScoped = NO; // Native iCloud doesn't need security-scoped access
            MDZILog("CloudStorageManager: Found existing source matching native iCloud path: %@", nativePath);
            return;
        }
    }

    // Create the Documents directory if needed
    NSError *error = nil;
    if (![self.fileManager fileExistsAtPath:documentsURL.path]) {
        [self.fileManager createDirectoryAtURL:documentsURL
                   withIntermediateDirectories:YES
                                    attributes:nil
                                         error:&error];
        if (error) {
            MDZELog("CloudStorageManager: Failed to create iCloud Documents directory: %@", error);
        }
    }

    // Create placeholder file
    NSString *placeholderPath = [documentsURL.path stringByAppendingPathComponent:@"put_files_here.modizer"];
    if (![self.fileManager fileExistsAtPath:placeholderPath]) {
        [self.fileManager createFileAtPath:placeholderPath contents:nil attributes:nil];
    }

    // Create native iCloud source
    CloudStorageSource *icloudSource = [[CloudStorageSource alloc] initWithURL:documentsURL
                                                                          name:@"iCloud"
                                                                          type:CloudStorageTypeICloudNative];
    icloudSource.isSecurityScoped = NO; // Native iCloud doesn't need security-scoped access
    icloudSource.isAccessible = YES;

    @synchronized (self.mutableSources) {
        [self.mutableSources insertObject:icloudSource atIndex:0];
    }

    self.nativeICloudSource = icloudSource;

    MDZILog("CloudStorageManager: Native iCloud initialized at: %@", documentsURL.path);
}

- (void)resolveAllBookmarks {
    @synchronized (self.mutableSources) {
        for (CloudStorageSource *source in self.mutableSources) {
            if (source.type != CloudStorageTypeICloudNative && source.bookmarkData) {
                [source resolveBookmark];
            }
        }
    }
}

- (void)refreshAllSourcesWithCompletion:(CloudStorageCompletionBlock)completion {
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        [self resolveAllBookmarks];

        dispatch_async(dispatch_get_main_queue(), ^{
            [self notifySourcesUpdated];
            if (completion) {
                completion(YES);
            }
        });
    });
}

#pragma mark - Source Management

- (CloudStorageSource *)addSourceWithURL:(NSURL *)url name:(NSString *)name type:(CloudStorageType)type {
    // Check for duplicates
    for (CloudStorageSource *existing in self.mutableSources) {
        if ([existing.resolvedURL.path isEqualToString:url.path]) {
            MDZILog("CloudStorageManager: Source already exists for path: %@", url.path);
            return existing;
        }
    }

    CloudStorageSource *source = [[CloudStorageSource alloc] initWithURL:url name:name type:type];

    @synchronized (self.mutableSources) {
        [self.mutableSources addObject:source];
    }

    [self saveSourceBookmarks];
    [self notifySourcesUpdated];

    MDZILog("CloudStorageManager: Added new source: %@", source);
    return source;
}

- (CloudStorageSource *)addSourceWithURL:(NSURL *)url name:(NSString *)name {
    CloudStorageType type = [CloudStorageSource typeFromURLString:url.absoluteString];
    return [self addSourceWithURL:url name:name type:type];
}

- (void)removeSource:(CloudStorageSource *)source {
    if (!source) return;

    // Don't allow removing native iCloud source
    if (source.type == CloudStorageTypeICloudNative && source == self.nativeICloudSource) {
        MDZILog("CloudStorageManager: Cannot remove native iCloud source");
        return;
    }

    [source stopAccessing];

    @synchronized (self.mutableSources) {
        [self.mutableSources removeObject:source];
    }

    [self saveSourceBookmarks];
    [self notifySourcesUpdated];

    MDZILog("CloudStorageManager: Removed source: %@", source.name);
}

- (CloudStorageSource *)sourceForPath:(NSString *)path {
    if (!path) return nil;

    @synchronized (self.mutableSources) {
        for (CloudStorageSource *source in self.mutableSources) {
            if (source.resolvedURL && [path hasPrefix:source.resolvedURL.path]) {
                return source;
            }
        }
    }

    return nil;
}

- (CloudStorageSource *)sourceWithIdentifier:(NSString *)identifier {
    if (!identifier) return nil;

    @synchronized (self.mutableSources) {
        for (CloudStorageSource *source in self.mutableSources) {
            if ([source.identifier isEqualToString:identifier]) {
                return source;
            }
        }
    }

    return nil;
}

- (BOOL)isNativeICloudAvailable {
    return self.nativeICloudSource != nil && self.nativeICloudSource.isAccessible;
}

#pragma mark - Security-Scoped Access

- (BOOL)startAccessingSource:(CloudStorageSource *)source {
    if (!source) return NO;
    
    NSLog(@"CSM: Start accessing %@",source.name);

    // Try to resolve bookmark if we have one and source is not accessible
    if (!source.isAccessible && source.bookmarkData) {
        [source resolveBookmark];
    }

    // If still not accessible but we have a resolved URL (e.g., freshly added source without bookmark)
    // consider it accessible for this session
    if (!source.isAccessible && source.resolvedURL) {
        source.isAccessible = YES;
    }

    if (source.isAccessible && source.isSecurityScoped) {
        [source startAccessing];
        return YES;
    }

    // Native iCloud doesn't need security-scoped access
    if (source.type == CloudStorageTypeICloudNative) {
        return source.isAccessible;
    }

    return source.isAccessible;
}

- (void)stopAccessingSource:(CloudStorageSource *)source {
    if (source && source.isSecurityScoped) {
        
        NSLog(@"CSM: Stop accessing %@",source.name);
        
        [source stopAccessing];
    }
}

#pragma mark - File Operations

- (void)listContentsAtPath:(NSString *)path
                    source:(CloudStorageSource *)source
                completion:(CloudStorageListCompletionBlock)completion {
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        NSError *error = nil;
        NSArray *contents = nil;

        if ([self startAccessingSource:source]) {
            contents = [self.fileManager contentsOfDirectoryAtPath:path error:&error];
            // Note: We don't stop accessing here as the caller may need continued access
        } else {
            error = [NSError errorWithDomain:@"CloudStorageManager"
                                        code:-1
                                    userInfo:@{NSLocalizedDescriptionKey: @"Cannot access cloud source"}];
        }

        dispatch_async(dispatch_get_main_queue(), ^{
            if (completion) {
                completion(contents, error);
            }
        });
    });
}

- (BOOL)isCloudPath:(NSString *)path {
    return [self sourceForPath:path] != nil;
}

- (BOOL)isFileDownloaded:(NSURL *)url {
    // First check if it's a ubiquitous (iCloud) item
    if ([self.fileManager isUbiquitousItemAtURL:url]) {
        NSNumber *isDownloaded = nil;
        NSError *error = nil;
        BOOL success = [url getResourceValue:&isDownloaded
                                      forKey:NSURLUbiquitousItemDownloadingStatusKey
                                       error:&error];

        if (!success || error) {
            MDZELog("CloudStorageManager: Failed to check download status: %@", error);
            return NO;
        }

        // Check various download states
        NSString *status = (NSString *)isDownloaded;
        if ([status isEqualToString:NSURLUbiquitousItemDownloadingStatusCurrent]) {
            return YES;
        }

        // Also check the legacy key for compatibility
        NSNumber *isDownloadedLegacy = nil;
        [url getResourceValue:&isDownloadedLegacy forKey:NSURLUbiquitousItemIsDownloadedKey error:nil];
        if (isDownloadedLegacy && [isDownloadedLegacy boolValue]) {
            return YES;
        }

        return NO;
    }

    // For non-ubiquitous items (Google Drive, Dropbox, etc. via Files app)
    // Use only non-intrusive checks to avoid triggering downloads

    // Check file size using URL resource values (less intrusive than fileManager)
    NSNumber *fileSize = nil;
    NSError *error = nil;
    [url getResourceValue:&fileSize forKey:NSURLFileSizeKey error:&error];

    if (error) {
        // If we can't get the size, the file might not exist or be accessible
        MDZDLog("CloudStorageManager: Cannot get file size: %@", error.localizedDescription);
        return NO;
    }

    // Check if it's a placeholder (zero or very small size for cloud files)
    if (fileSize) {
        unsigned long long size = [fileSize unsignedLongLongValue];
        if (size == 0) {
            MDZDLog("CloudStorageManager: File has zero size (placeholder): %@", url.path);
            return NO;
        }
        // Files under 1KB might be placeholders for some cloud providers
        // But we'll trust them as downloaded to avoid false negatives
    }

    // Check if file is a symbolic link or alias (sometimes used for cloud placeholders)
    NSNumber *isSymlink = nil;
    [url getResourceValue:&isSymlink forKey:NSURLIsSymbolicLinkKey error:nil];
    if (isSymlink && [isSymlink boolValue]) {
        // Symbolic links in cloud folders might be placeholders
        MDZDLog("CloudStorageManager: File is symbolic link: %@", url.path);
    }

    // For cloud files, check the "offline" attribute if available
    // This is a non-intrusive check that doesn't trigger download
    NSNumber *isUbiquitous = nil;
    [url getResourceValue:&isUbiquitous forKey:NSURLIsUbiquitousItemKey error:nil];
    if (isUbiquitous && [isUbiquitous boolValue]) {
        // It's marked as ubiquitous, check download status
        NSString *downloadStatus = nil;
        [url getResourceValue:&downloadStatus forKey:NSURLUbiquitousItemDownloadingStatusKey error:nil];
        if (downloadStatus) {
            if ([downloadStatus isEqualToString:NSURLUbiquitousItemDownloadingStatusCurrent]) {
                return YES;
            }
            if ([downloadStatus isEqualToString:NSURLUbiquitousItemDownloadingStatusNotDownloaded]) {
                return NO;
            }
        }
    }

    // If we got here with a valid file size > 0, assume it's downloaded
    // This avoids opening the file which could trigger a download
    return (fileSize && [fileSize unsignedLongLongValue] > 0);
}

- (BOOL)startDownloadingFile:(NSURL *)url error:(NSError **)error {
    return [self.fileManager startDownloadingUbiquitousItemAtURL:url error:error];
}

#pragma mark - Document Picker

- (void)presentFolderPickerFrom:(UIViewController *)viewController
                     completion:(CloudStorageFolderPickerCompletionBlock)completion {
    self.folderPickerCompletion = completion;

#if TARGET_OS_MACCATALYST
    // On Mac Catalyst, use NSOpenPanel directly via runtime
    MDZILog("CloudStorageManager: Using NSOpenPanel on Mac Catalyst");

    Class NSOpenPanelClass = NSClassFromString(@"NSOpenPanel");
    if (NSOpenPanelClass) {
        id openPanel = [NSOpenPanelClass performSelector:@selector(openPanel)];

        // Configure the panel using setValue:forKey: for BOOL properties
        [openPanel setValue:@NO forKey:@"canChooseFiles"];
        [openPanel setValue:@YES forKey:@"canChooseDirectories"];
        [openPanel setValue:@NO forKey:@"allowsMultipleSelection"];
        [openPanel setValue:@NO forKey:@"canCreateDirectories"];
        [openPanel setValue:NSLocalizedString(@"Select Cloud Folder", @"") forKey:@"title"];
        [openPanel setValue:NSLocalizedString(@"Select", @"") forKey:@"prompt"];
        [openPanel setValue:NSLocalizedString(@"Select a folder to add as a cloud source", @"") forKey:@"message"];

        // Use beginWithCompletionHandler
        typedef void (^NSOpenPanelCompletionHandler)(NSInteger result);
        NSOpenPanelCompletionHandler handler = ^(NSInteger result) {
            dispatch_async(dispatch_get_main_queue(), ^{
                if (result == 1) { // NSModalResponseOK = 1
                    NSArray *urls = [openPanel valueForKey:@"URLs"];
                    NSURL *url = urls.firstObject;
                    if (url) {
                        MDZILog("CloudStorageManager: NSOpenPanel selected URL: %@", url);
                        [url startAccessingSecurityScopedResource];
                        if (self.folderPickerCompletion) {
                            self.folderPickerCompletion(url, nil);
                        }
                    } else {
                        MDZELog("CloudStorageManager: NSOpenPanel returned no URL");
                        if (self.folderPickerCompletion) {
                            self.folderPickerCompletion(nil, nil);
                        }
                    }
                } else {
                    MDZILog("CloudStorageManager: NSOpenPanel was cancelled (result: %ld)", (long)result);
                    if (self.folderPickerCompletion) {
                        self.folderPickerCompletion(nil, nil);
                    }
                }
                self.folderPickerCompletion = nil;
            });
        };

        // Call beginWithCompletionHandler using NSInvocation for block parameter
        SEL selector = @selector(beginWithCompletionHandler:);
        NSMethodSignature *signature = [openPanel methodSignatureForSelector:selector];
        NSInvocation *invocation = [NSInvocation invocationWithMethodSignature:signature];
        [invocation setTarget:openPanel];
        [invocation setSelector:selector];
        [invocation setArgument:&handler atIndex:2];
        [invocation invoke];
    } else {
        MDZELog("CloudStorageManager: NSOpenPanel class not available");
        if (completion) {
            NSError *error = [NSError errorWithDomain:@"CloudStorageManager"
                                                 code:-1
                                             userInfo:@{NSLocalizedDescriptionKey: @"Folder picker not available on this platform"}];
            completion(nil, error);
        }
    }
#else
    // iOS: use UIDocumentPickerViewController - folder selection only
    UIDocumentPickerViewController *picker;

    if (@available(iOS 14.0, *)) {
        picker = [[UIDocumentPickerViewController alloc] initForOpeningContentTypes:@[UTTypeFolder]];
    } else {
        picker = [[UIDocumentPickerViewController alloc] initWithDocumentTypes:@[@"public.folder"]
                                                                        inMode:UIDocumentPickerModeOpen];
    }

    picker.delegate = self;
    picker.allowsMultipleSelection = NO;
    self.currentPicker = picker;

    [viewController presentViewController:picker animated:YES completion:nil];
#endif
}

#pragma mark - UIDocumentPickerDelegate

- (void)documentPicker:(UIDocumentPickerViewController *)controller didPickDocumentsAtURLs:(NSArray<NSURL *> *)urls {
    MDZILog("CloudStorageManager: documentPicker:didPickDocumentsAtURLs: called with %lu URLs", (unsigned long)urls.count);

    NSURL *url = urls.firstObject;

    if (url) {
        // Start accessing security-scoped resource
        BOOL accessStarted = [url startAccessingSecurityScopedResource];
        MDZILog("CloudStorageManager: Picked folder URL: %@ (security access started: %@)", url, accessStarted ? @"YES" : @"NO");

#if !TARGET_OS_MACCATALYST
        // On iOS, check if this is a Google Drive URL and reject it
        CloudStorageType type = [CloudStorageSource typeFromURLString:url.absoluteString];
        if (type == CloudStorageTypeGoogleDrive) {
            MDZILog("CloudStorageManager: Google Drive not supported on iOS");
            [url stopAccessingSecurityScopedResource];
            if (self.folderPickerCompletion) {
                NSError *error = [NSError errorWithDomain:@"CloudStorageManager"
                                                     code:-2
                                                 userInfo:@{NSLocalizedDescriptionKey: NSLocalizedString(@"Google Drive is not supported on iOS due to system limitations. Please use iCloud or access Google Drive on Mac.", @"")}];
                self.folderPickerCompletion(nil, error);
            }
            self.folderPickerCompletion = nil;
            self.currentPicker = nil;
            return;
        }
#endif

        if (self.folderPickerCompletion) {
            MDZILog("CloudStorageManager: Calling folderPickerCompletion with URL: %@", url);
            self.folderPickerCompletion(url, nil);
        } else {
            MDZELog("CloudStorageManager: folderPickerCompletion is nil!");
        }
    } else {
        MDZELog("CloudStorageManager: No URL in picked documents");
        if (self.folderPickerCompletion) {
            NSError *error = [NSError errorWithDomain:@"CloudStorageManager"
                                                 code:-1
                                             userInfo:@{NSLocalizedDescriptionKey: @"No folder selected"}];
            self.folderPickerCompletion(nil, error);
        }
    }

    self.folderPickerCompletion = nil;
    self.currentPicker = nil;
}

- (void)documentPickerWasCancelled:(UIDocumentPickerViewController *)controller {
    MDZILog("CloudStorageManager: documentPickerWasCancelled called");
    if (self.folderPickerCompletion) {
        self.folderPickerCompletion(nil, nil);
    }
    self.folderPickerCompletion = nil;
    self.currentPicker = nil;
}

#pragma mark - Persistence

- (void)saveSourceBookmarks {
    NSMutableArray *encoded = [NSMutableArray array];

    @synchronized (self.mutableSources) {
        for (CloudStorageSource *source in self.mutableSources) {
            // Don't save the auto-detected native iCloud source - it's recreated at launch
            // But DO save manually added iCloud folders (even if detected as CloudStorageTypeICloudNative)
            if (source == self.nativeICloudSource) {
                continue;
            }

            if (source.bookmarkData) {
                NSDictionary *dict = @{
                    @"id": source.identifier ?: @"",
                    @"name": source.name ?: @"",
                    @"type": @(source.type),
                    @"bookmark": source.bookmarkData,
                    @"isSecurityScoped": @(source.isSecurityScoped)
                };
                [encoded addObject:dict];
            }
        }
    }

    [[NSUserDefaults standardUserDefaults] setObject:encoded forKey:kCloudStorageSourcesKey];
    [[NSUserDefaults standardUserDefaults] synchronize];

    MDZILog("CloudStorageManager: Saved %lu source bookmarks", (unsigned long)encoded.count);
}

- (void)loadSourceBookmarks {
    NSArray *encoded = [[NSUserDefaults standardUserDefaults] objectForKey:kCloudStorageSourcesKey];

    if (!encoded || ![encoded isKindOfClass:[NSArray class]]) {
        MDZILog("CloudStorageManager: No saved source bookmarks found");
        return;
    }

    @synchronized (self.mutableSources) {
        for (NSDictionary *dict in encoded) {
            if (![dict isKindOfClass:[NSDictionary class]]) continue;

            NSString *identifier = dict[@"id"];
            NSString *name = dict[@"name"];
            NSNumber *typeNum = dict[@"type"];
            NSData *bookmark = dict[@"bookmark"];

            if (!name || !typeNum || !bookmark) continue;

            CloudStorageType type = (CloudStorageType)[typeNum integerValue];

            CloudStorageSource *source = [[CloudStorageSource alloc] initWithBookmarkData:bookmark
                                                                                     name:name
                                                                                     type:type
                                                                               identifier:identifier];

            NSNumber *isSecurityScoped = dict[@"isSecurityScoped"];
            if (isSecurityScoped) {
                source.isSecurityScoped = [isSecurityScoped boolValue];
            }

            [self.mutableSources addObject:source];
        }
    }

    MDZILog("CloudStorageManager: Loaded %lu source bookmarks", (unsigned long)self.mutableSources.count);
}

- (void)clearAllSources {
    @synchronized (self.mutableSources) {
        // Stop accessing all sources
        for (CloudStorageSource *source in self.mutableSources) {
            [source stopAccessing];
        }

        [self.mutableSources removeAllObjects];
    }

    self.nativeICloudSource = nil;

    [[NSUserDefaults standardUserDefaults] removeObjectForKey:kCloudStorageSourcesKey];
    [[NSUserDefaults standardUserDefaults] synchronize];

    [self notifySourcesUpdated];

    MDZILog("CloudStorageManager: Cleared all sources");
}

#pragma mark - Notifications

- (void)notifySourcesUpdated {
    dispatch_async(dispatch_get_main_queue(), ^{
        if ([self.delegate respondsToSelector:@selector(cloudStorageSourcesDidUpdate)]) {
            [self.delegate cloudStorageSourcesDidUpdate];
        }

        [[NSNotificationCenter defaultCenter] postNotificationName:CloudStorageSourcesDidUpdateNotification
                                                            object:self];
    });
}

- (void)notifyErrorWithSource:(CloudStorageSource *)source error:(NSError *)error {
    dispatch_async(dispatch_get_main_queue(), ^{
        if ([self.delegate respondsToSelector:@selector(cloudStorageDidFailWithError:source:)]) {
            [self.delegate cloudStorageDidFailWithError:error source:source];
        }

        NSMutableDictionary *userInfo = [NSMutableDictionary dictionary];
        if (source) userInfo[CloudStorageErrorSourceKey] = source;
        if (error) userInfo[CloudStorageErrorMessageKey] = error.localizedDescription;

        [[NSNotificationCenter defaultCenter] postNotificationName:CloudStorageErrorNotification
                                                            object:self
                                                          userInfo:userInfo];
    });
}

@end
