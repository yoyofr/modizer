//
//  CloudStorageSource.m
//  modizer
//
//  Created for multi-cloud storage support
//

#import "CloudStorageSource.h"
#import "ModizerConstants.h"

@interface CloudStorageSource ()
@property (nonatomic, strong, readwrite, nullable) NSURL *resolvedURL;
@property (nonatomic, assign) BOOL isCurrentlyAccessing;
@end

@implementation CloudStorageSource

#pragma mark - NSSecureCoding

+ (BOOL)supportsSecureCoding {
    return YES;
}

- (void)encodeWithCoder:(NSCoder *)coder {
    [coder encodeObject:self.identifier forKey:@"identifier"];
    [coder encodeObject:self.name forKey:@"name"];
    [coder encodeInteger:self.type forKey:@"type"];
    [coder encodeObject:self.bookmarkData forKey:@"bookmarkData"];
    [coder encodeBool:self.isSecurityScoped forKey:@"isSecurityScoped"];
    [coder encodeBool:self.isFileBasedAccess forKey:@"isFileBasedAccess"];
}

- (instancetype)initWithCoder:(NSCoder *)coder {
    self = [super init];
    if (self) {
        _identifier = [coder decodeObjectOfClass:[NSString class] forKey:@"identifier"];
        _name = [coder decodeObjectOfClass:[NSString class] forKey:@"name"];
        _type = [coder decodeIntegerForKey:@"type"];
        _bookmarkData = [coder decodeObjectOfClass:[NSData class] forKey:@"bookmarkData"];
        _isSecurityScoped = [coder decodeBoolForKey:@"isSecurityScoped"];
        _isFileBasedAccess = [coder decodeBoolForKey:@"isFileBasedAccess"];
        _isAccessible = NO;
        _isCurrentlyAccessing = NO;
        _tintColor = [CloudStorageSource tintColorForType:_type darkMode:NO];
    }
    return self;
}

#pragma mark - Initialization

- (instancetype)initWithURL:(NSURL *)url name:(NSString *)name type:(CloudStorageType)type {
    self = [super init];
    if (self) {
        _identifier = [[NSUUID UUID] UUIDString];
        _name = name;
        _type = type;
        _isAccessible = NO;
        _isCurrentlyAccessing = NO;
        _isSecurityScoped = YES;
        _tintColor = [CloudStorageSource tintColorForType:type darkMode:NO];

        [self createBookmarkFromURL:url];
    }
    return self;
}

- (instancetype)initWithBookmarkData:(NSData *)bookmarkData name:(NSString *)name type:(CloudStorageType)type identifier:(NSString *)identifier {
    self = [super init];
    if (self) {
        _identifier = identifier ?: [[NSUUID UUID] UUIDString];
        _name = name;
        _type = type;
        _bookmarkData = bookmarkData;
        _isAccessible = NO;
        _isCurrentlyAccessing = NO;
        _isSecurityScoped = YES;
        _tintColor = [CloudStorageSource tintColorForType:type darkMode:NO];
    }
    return self;
}

#pragma mark - Bookmark Management

- (BOOL)createBookmarkFromURL:(NSURL *)url {
    NSError *error = nil;

    NSURLBookmarkCreationOptions options = 0;
#if !TARGET_OS_MACCATALYST
    options = NSURLBookmarkCreationMinimalBookmark;
#endif

    // For security-scoped resources (only available on macOS/Mac Catalyst)
#if TARGET_OS_MACCATALYST || TARGET_OS_OSX
    if (self.isSecurityScoped) {
        options |= NSURLBookmarkCreationWithSecurityScope;
    }
#endif

    NSData *bookmark = [url bookmarkDataWithOptions:options
                     includingResourceValuesForKeys:nil
                                      relativeToURL:nil
                                              error:&error];

    // Store the resolved URL - handle file-based access case
    if (self.isFileBasedAccess) {
        self.securityScopedURL = url;  // Keep file URL for security-scoped access
        self.resolvedURL = [url URLByDeletingLastPathComponent];  // Parent folder for browsing
        MDZILog("Created file-based bookmark: file=%@, folder=%@", url.path, self.resolvedURL.path);
    } else {
        self.resolvedURL = url;
    }

    if (error || !bookmark) {
        MDZELog("Failed to create bookmark for URL %@: %@", url, error.localizedDescription ?: @"No bookmark data returned");
        // Still mark as accessible temporarily since we have the URL
        self.isAccessible = YES;
        return NO;
    }

    self.bookmarkData = bookmark;
    self.isAccessible = YES;

    MDZILog("Created bookmark for cloud source: %@ (isFileBasedAccess: %d)", self.name, self.isFileBasedAccess);
    return YES;
}

- (BOOL)resolveBookmark {
    if (!self.bookmarkData) {
        MDZELog("No bookmark data for cloud source: %@", self.name);
        self.isAccessible = NO;
        return NO;
    }

    NSError *error = nil;
    BOOL isStale = NO;

    NSURLBookmarkResolutionOptions options = 0;
#if TARGET_OS_MACCATALYST || TARGET_OS_OSX
    if (self.isSecurityScoped) {
        options |= NSURLBookmarkResolutionWithSecurityScope;
    }
#endif

    NSURL *resolved = [NSURL URLByResolvingBookmarkData:self.bookmarkData
                                                options:options
                                          relativeToURL:nil
                                    bookmarkDataIsStale:&isStale
                                                  error:&error];

    if (error || !resolved) {
        MDZELog("Failed to resolve bookmark for cloud source %@: %@", self.name, error.localizedDescription);
        self.isAccessible = NO;
        self.resolvedURL = nil;
        return NO;
    }

    if (isStale) {
        MDZILog("Bookmark is stale for cloud source: %@, attempting to refresh", self.name);
        // Try to refresh the bookmark
        if ([self createBookmarkFromURL:resolved]) {
            MDZILog("Successfully refreshed stale bookmark for: %@", self.name);
        }
    }

    // If this is file-based access, bookmark points to a file, but we want to browse its parent folder
    if (self.isFileBasedAccess) {
        self.securityScopedURL = resolved; // Keep file URL for security-scoped access
        self.resolvedURL = [resolved URLByDeletingLastPathComponent]; // Parent folder for browsing
        MDZILog("Resolved file-based bookmark for cloud source: %@ -> file: %@, folder: %@",
                self.name, resolved.path, self.resolvedURL.path);
    } else {
        self.resolvedURL = resolved;
        MDZILog("Resolved bookmark for cloud source: %@ -> %@", self.name, resolved.path);
    }

    self.isAccessible = YES;
    return YES;
}

#pragma mark - Security-Scoped Access

- (void)startAccessing {
    if (self.isCurrentlyAccessing) {
        MDZILog("Already accessing security-scoped resource: %@", self.name);
        return;
    }

    // Use securityScopedURL if available (original file/folder selected), otherwise use resolvedURL
    NSURL *urlToAccess = self.securityScopedURL ?: self.resolvedURL;

    if (urlToAccess && self.isSecurityScoped) {
        BOOL success = [urlToAccess startAccessingSecurityScopedResource];
        if (success) {
            self.isCurrentlyAccessing = YES;
            MDZILog("Started accessing security-scoped resource: %@ (URL: %@)", self.name, urlToAccess);

            // Check if we have write access
            NSFileManager *fm = [NSFileManager defaultManager];
            BOOL isWritable = [fm isWritableFileAtPath:urlToAccess.path];
            MDZILog("Write access check for %@: %@", self.name, isWritable ? @"YES" : @"NO");
        } else {
            MDZELog("Failed to start accessing security-scoped resource: %@ (URL: %@)", self.name, urlToAccess);
        }
    } else {
        MDZILog("No security-scoped access needed for: %@ (isSecurityScoped: %d, url: %@)",
                self.name, self.isSecurityScoped, urlToAccess);
    }
}

- (void)stopAccessing {
    if (!self.isCurrentlyAccessing) {
        return;
    }

    // Use securityScopedURL if available (original file/folder selected), otherwise use resolvedURL
    NSURL *urlToAccess = self.securityScopedURL ?: self.resolvedURL;

    if (urlToAccess && self.isSecurityScoped) {
        [urlToAccess stopAccessingSecurityScopedResource];
        self.isCurrentlyAccessing = NO;
        MDZDLog("Stopped accessing security-scoped resource: %@ (URL: %@)", self.name, urlToAccess);
    }
}

#pragma mark - Type Utilities

+ (UIColor *)tintColorForType:(CloudStorageType)type darkMode:(BOOL)darkMode {
    switch (type) {
        case CloudStorageTypeICloudNative:
        case CloudStorageTypeICloudDrive:
            if (darkMode) {
                return [UIColor colorWithRed:ICLOUD_COLOR_RED_DARKMODE
                                       green:ICLOUD_COLOR_GREEN_DARKMODE
                                        blue:ICLOUD_COLOR_BLUE_DARKMODE
                                       alpha:1.0];
            } else {
                return [UIColor colorWithRed:ICLOUD_COLOR_RED
                                       green:ICLOUD_COLOR_GREEN
                                        blue:ICLOUD_COLOR_BLUE
                                       alpha:1.0];
            }

        case CloudStorageTypeGoogleDrive:
            // Google Drive blue: #4285F4
            return [UIColor colorWithRed:0x42/255.0
                                   green:0x85/255.0
                                    blue:0xF4/255.0
                                   alpha:1.0];

        case CloudStorageTypeDropbox:
            // Dropbox blue: #0061FF
            return [UIColor colorWithRed:0x00/255.0
                                   green:0x61/255.0
                                    blue:0xFF/255.0
                                   alpha:1.0];

        case CloudStorageTypeOneDrive:
            // OneDrive/Microsoft blue: #0078D4
            return [UIColor colorWithRed:0x00/255.0
                                   green:0x78/255.0
                                    blue:0xD4/255.0
                                   alpha:1.0];

        case CloudStorageTypeOther:
        default:
            // Generic gray
            return [UIColor colorWithRed:0.5 green:0.5 blue:0.5 alpha:1.0];
    }
}

+ (NSString *)displayNameForType:(CloudStorageType)type {
    switch (type) {
        case CloudStorageTypeICloudNative:
            return @"iCloud";
        case CloudStorageTypeICloudDrive:
            return @"iCloud Drive";
        case CloudStorageTypeGoogleDrive:
            return @"Google Drive";
        case CloudStorageTypeDropbox:
            return @"Dropbox";
        case CloudStorageTypeOneDrive:
            return @"OneDrive";
        case CloudStorageTypeOther:
        default:
            return @"Cloud Storage";
    }
}

+ (CloudStorageType)typeFromURLString:(NSString *)urlString {
    NSString *lowercased = [urlString lowercaseString];

    // iCloud detection - check various patterns used on iOS and macOS
    if ([lowercased containsString:@"icloud"] ||
        [lowercased containsString:@"ubiquitycontainer"] ||
        [lowercased containsString:@"mobile documents"] ||      // macOS iCloud path
        [lowercased containsString:@"mobile%20documents"] ||    // URL encoded
        [lowercased containsString:@"clouddocs"] ||
        [lowercased containsString:@"com~apple~"]) {            // iCloud container prefix
        if ([lowercased containsString:@"com~apple~clouddocs"] ||
            [lowercased containsString:@"clouddocs"]) {
            return CloudStorageTypeICloudDrive;
        }
        return CloudStorageTypeICloudNative;
    }

    if ([lowercased containsString:@"google"] || [lowercased containsString:@"gdrive"]) {
        return CloudStorageTypeGoogleDrive;
    }

    if ([lowercased containsString:@"dropbox"]) {
        return CloudStorageTypeDropbox;
    }

    if ([lowercased containsString:@"onedrive"] || [lowercased containsString:@"microsoft"]) {
        return CloudStorageTypeOneDrive;
    }

    return CloudStorageTypeOther;
}

#pragma mark - Description

- (NSString *)description {
    return [NSString stringWithFormat:@"<CloudStorageSource: %@ (%@) - %@ - accessible: %@>",
            self.name,
            [CloudStorageSource displayNameForType:self.type],
            self.resolvedURL.path ?: @"(not resolved)",
            self.isAccessible ? @"YES" : @"NO"];
}

@end
