//
//  DirParser.mm
//  modizer
//
//  Created by Yohann Magnien David on 26/10/2025.
//

#import "DirParser.h"

@implementation FileNode

- (instancetype)initWithPath:(NSString *)path {
    self = [super init];
    if (self) {
        _path = path;
        _name = [path lastPathComponent];
        _children = [NSMutableArray array];
        _isSelected = TRUE;
        
        NSFileManager *fm = [NSFileManager defaultManager];
        NSDictionary *attrs = [fm attributesOfItemAtPath:path error:nil];
        
        if (attrs) {
            _fileSize = [attrs fileSize];
            _modificationDate = [attrs fileModificationDate];
            
            BOOL isDir = NO;
            [fm fileExistsAtPath:path isDirectory:&isDir];
            _isDirectory = isDir;
        }
    }
    return self;
}

- (NSString *)formattedSize {
    if (self.isDirectory) {
        return @"--";
    }
    
    double size = (double)self.fileSize;
    NSArray *units = @[@"B", @"KB", @"MB", @"GB", @"TB"];
    NSInteger unitIndex = 0;
    
    while (size >= 1024.0 && unitIndex < units.count - 1) {
        size /= 1024.0;
        unitIndex++;
    }
    
    return [NSString stringWithFormat:@"%.2f %@", size, units[unitIndex]];
}

- (void)printStructureWithIndent:(NSInteger)indent {
    NSMutableString *indentStr = [NSMutableString string];
    for (NSInteger i = 0; i < indent; i++) {
        [indentStr appendString:@"  "];
    }
    
    NSString *icon = self.isDirectory ? @"📁" : @"📄";
    NSLog(@"%@%@ %@ (%@)", indentStr, icon, self.name, [self formattedSize]);
    
    for (FileNode *child in self.children) {
        [child printStructureWithIndent:indent + 1];
    }
}

- (bool)filterNodes:(NSString *)pattern filterDir:(bool)filterDir {
    bool result=false;
    
    if (filterDir) {
        // Filter dir like files
        if ([[self.name lowercaseString] containsString:[pattern lowercaseString]]) {
            result=true;
        }
    } else if (!self.isDirectory) {
        // Filter files only
        if ([[self.name lowercaseString] containsString:[pattern lowercaseString]]) {
            result=true;
        }
    }
    
    for (FileNode *child in self.children) {
        bool ret=[child filterNodes:pattern filterDir:filterDir];
        if (ret) { //found a child matching
            result=true;
        }
    }
    
    if (result) self.isMatchingFilter=YES;
    else self.isMatchingFilter=NO;
    
    return result;
}

@end

@implementation DirParser

- (instancetype)init {
    self = [super init];
    if (self) {
        _includeHiddenFiles = NO;
        _maxDepth = -1; // -1 means unlimited
        _filterExt = nil;
    }
    return self;
}

- (FileNode *)parseDirectoryAtPath:(NSString *)path error:(NSError **)error {
    return [self parseDirectoryAtPath:path depth:0 error:error];
}

- (FileNode *)parseDirectoryAtPath:(NSString *)path depth:(NSInteger)depth error:(NSError **)error {
    NSFileManager *fm = [NSFileManager defaultManager];
    
    // Check if path exists
    BOOL exists = [fm fileExistsAtPath:path];
    if (!exists) {
        if (error) {
            *error = [NSError errorWithDomain:NSCocoaErrorDomain
                                         code:NSFileReadNoSuchFileError
                                     userInfo:@{NSLocalizedDescriptionKey: @"Path does not exist"}];
        }
        return nil;
    }
    
    FileNode *node = [[FileNode alloc] initWithPath:path];
    
    // If it's a file or we've reached max depth, return the node
    if (!node.isDirectory || (self.maxDepth >= 0 && depth >= self.maxDepth)) {
        if (self.filterExt && [[node.name pathExtension] caseInsensitiveCompare:self.filterExt]) return nil;
        return node;
    }
    
    // Get directory contents
    NSArray *contents = [fm contentsOfDirectoryAtPath:path error:error];
    if (!contents) {
        return node; // Return node even if we can't read contents
    }
    
    // Sort contents alphabetically
    contents = [contents sortedArrayUsingSelector:@selector(localizedCaseInsensitiveCompare:)];
    
    for (NSString *item in contents) {
        // Skip hidden files if specified
        if (!self.includeHiddenFiles && [item hasPrefix:@"."]) {
            continue;
        }
        
        NSString *itemPath = [path stringByAppendingPathComponent:item];
        FileNode *childNode = [self parseDirectoryAtPath:itemPath depth:depth + 1 error:nil];
        
        if (childNode) {
            [node.children addObject:childNode];
        }
    }
    
    return node;
}

- (NSArray<FileNode *> *)flattenTree:(FileNode *)root {
    NSMutableArray<FileNode *> *result = [NSMutableArray array];
    [self flattenNode:root intoArray:result];
    return [result copy];
}

- (void)flattenNode:(FileNode *)node intoArray:(NSMutableArray<FileNode *> *)array {
    [array addObject:node];
    for (FileNode *child in node.children) {
        [self flattenNode:child intoArray:array];
    }
}

@end
