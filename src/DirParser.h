//
//  DirParser.h
//  modizer
//
//  Created by Yohann Magnien David on 26/10/2025.
//

#import <Foundation/Foundation.h>

@interface FileNode : NSObject

@property (nonatomic, strong) NSString *name;
@property (nonatomic, strong) NSString *path;
@property (nonatomic, assign) BOOL isSelected;
@property (nonatomic, assign) BOOL isMatchingFilter;
@property (nonatomic, assign) BOOL isDirectory;
@property (nonatomic, assign) unsigned long long fileSize;
@property (nonatomic, strong) NSDate *modificationDate;
@property (nonatomic, strong) NSMutableArray<FileNode *> *children;

- (instancetype)initWithPath:(NSString *)path;
- (NSString *)formattedSize;
- (void)printStructureWithIndent:(NSInteger)indent;

- (bool)filterNodes:(NSString *)pattern filterDir:(bool)filterDir;

@end

@interface DirParser : NSObject

@property (nonatomic, assign) BOOL includeHiddenFiles;
@property (nonatomic, strong) NSString *filterExt;
@property (nonatomic, assign) NSInteger maxDepth;

- (FileNode *)parseDirectoryAtPath:(NSString *)path error:(NSError **)error;
- (FileNode *)parseDirectoryAtPath:(NSString *)path depth:(NSInteger)depth error:(NSError **)error;
- (NSArray<FileNode *> *)flattenTree:(FileNode *)root;

@end
