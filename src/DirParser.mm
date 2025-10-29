//
//  DirParser.mm
//  modizer
//
//  Created by Yohann Magnien David on 26/10/2025.
//

#define MDZ_PLAYLIST_MAX_RETRY 32

#import "DirParser.h"
#import "ModizerConstants.h"

extern int glob_notidle;

@implementation FileNode

- (instancetype)initWithPath:(NSString *)path {
    self = [super init];
    if (self) {
        _path = path;
        _name = [path lastPathComponent];
        _children = [NSMutableArray array];
        _isSelected = TRUE;
        _selectedChildren = 0;
        _isFullySelected = FALSE;
        _shouldPropagateStatus = FALSE;
        _isFavorite = FALSE;
        
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

- (void)flattenNode:(FileNode *)node selected:(bool)filterSelected favorite:(bool)filterFav intoArray:(NSMutableArray<FileNode *> *)array {
    bool toAdd=true;
    //Not add directory
    if (node.isDirectory) toAdd=false;
    else  {
        //file
        //Selected filter
        if (filterSelected && !node.isSelected) toAdd=false;
        //Favorite filter
        if (filterFav && !node.isFavorite) toAdd=false;
    }
    if (toAdd) [array addObject:node];
    
    for (FileNode *child in node.children) {
        [self flattenNode:child selected:filterSelected favorite:filterFav intoArray:array];
    }
}


- (NSArray*) getSelectedPlaylist {
    NSMutableArray<FileNode *> *result = [NSMutableArray array];
    [self flattenNode:self selected:true favorite:false intoArray:result];
    
    return [result copy];
}

- (NSArray*) getFavoritePlaylist {
    NSMutableArray<FileNode *> *result = [NSMutableArray array];
    [self flattenNode:self selected:false favorite:true intoArray:result];
    
    return [result copy];
}

@end


void MDZOnPresetSwitchRequested(bool isHardCut, void* userData)
{
    MDZILog("should change preset");
    if (userData==NULL) return;
    MDZPlaylist *mdzPL=(__bridge MDZPlaylist *)userData;
    [mdzPL next:isHardCut];
}

void MDZOnPresetSwitchFailed(const char* presetFilename, const char* message, void* userData)
{
    MDZELog("couldnt switch to preset %s, reason %s",presetFilename,message)
    if (userData==NULL) return;
    MDZPlaylist *mdzPL=(__bridge MDZPlaylist *)userData;
    mdzPL.lastFailed=true;
}



@implementation MDZPlaylist

//-----------------------------
//
//-----------------------------
- (instancetype)init:(projectm_handle)pmh; {
    _items=[[NSMutableArray alloc] init];
    _position=0;
    _size=[_items count];
    _shuffle=false;
    _history=[[NSMutableArray alloc] init];
    _pmh=pmh;
    _curEntryLbl = @"";
    [self loadIdlePreset];
    
    projectm_set_preset_switch_requested_event_callback(_pmh, &MDZOnPresetSwitchRequested, (__bridge void*)self);
    projectm_set_preset_switch_failed_event_callback(_pmh, &MDZOnPresetSwitchFailed, (__bridge void*)self);
    _lastFailed=false;

    return self;
}


//-----------------------------
//
//-----------------------------
- (instancetype)initWithArray:(NSArray*)array pmh:(projectm_handle)pmh;{
    _items=[NSMutableArray arrayWithArray:array];
    _position=0;
    _size=[_items count];
    _shuffle=false;
    _history=[[NSMutableArray alloc] init];
    _pmh=pmh;
    _curEntryLbl = @"";
    [self loadIdlePreset];
    
    projectm_set_preset_switch_requested_event_callback(_pmh, &MDZOnPresetSwitchRequested, NULL);
    projectm_set_preset_switch_failed_event_callback(_pmh, &MDZOnPresetSwitchFailed, NULL);
    _lastFailed=false;
    return self;
}

- (void)loadIdlePreset {
    glob_notidle=0;
    projectm_load_preset_file(_pmh,"idle://Geiss & Sperl - Feedback (projectM idle HDR mix).milk",NULL);
    _curEntryLbl=[NSString stringWithFormat:@"No preset found. Activate bundled presets or copy milk files in '%s/presets' & images in '%s/textures' folders.",PM_ROOT_FOLDER_CUSTOM,PM_ROOT_FOLDER_CUSTOM];
}

//-----------------------------
//
//-----------------------------
- (void)setItems:(NSArray*)array {
    _items=[NSMutableArray arrayWithArray:array];
    _position=0;
    _size=[_items count];
    _history=[[NSMutableArray alloc] init];
}

//-----------------------------
//
//-----------------------------
- (void)addItems:(NSArray*)array {
    [_items addObjectsFromArray:array];
    _position=0;
    _size=[_items count];
}


//-----------------------------
//
//-----------------------------
- (void)setShuffle:(bool)active {
    _shuffle=active;
}

//-----------------------------
//
//-----------------------------
- (void)loadCurrentPreset:(bool)cut {
    //Load new preset
    int retry_counter=0;
    FileNode *item;
    while (1) {
        _lastFailed=false;
        item=[_items objectAtIndex:_position];
        glob_notidle=1;
        projectm_load_preset_file(_pmh, [item.path UTF8String], !cut);
        //if it hasnt failed, break to continue
        if (!_lastFailed) break;
        //Issue with last preset, remove from the list
        [self remove:_position];
        //If list empty, exit
        if (_size==0) break;
        //If too many attempt, exit, to avoid freezing app
        retry_counter++;
        if (retry_counter>MDZ_PLAYLIST_MAX_RETRY) break;
    }
    
    if (_size==0) {
        [self loadIdlePreset];
    } else {
        item=[_items objectAtIndex:_position];
        _curEntryLbl = [NSString stringWithFormat:@"(%d/%d) %@",_position+1,_size,item.name];
    }
}
//-----------------------------
//
//-----------------------------
- (void)next:(bool)cut {
    if (!_size) return;
    if (_shuffle) {
        _position=arc4random_uniform(_size);
    } else {
        _position++;
        if (_position>=_size) _position=0;
    }
    [self loadCurrentPreset:cut];
}

//-----------------------------
//
//-----------------------------
- (void)last:(bool)cut {
    if (!_size) return;
    if (_shuffle) {
        _position=arc4random_uniform(_size);
    } else {
        if (_position>0) _position--;
        else _position=_size-1;
    }
    
    [self loadCurrentPreset:cut];
}

//-----------------------------
//
//-----------------------------
- (void)prev:(bool)cut {
    if (!_size) return;
    if (_shuffle) {
        _position=arc4random_uniform(_size);
    } else {
        if (_position>0) _position--;
        else _position=_size-1;
    }
    
    [self loadCurrentPreset:cut];
}

//-----------------------------
//
//-----------------------------
- (int)getPos {
    return _position;
}

//-----------------------------
//
//-----------------------------
- (void)setPos:(int)pos cut:(bool)cut {
    if (pos<_size) _position=pos;
    
    [self loadCurrentPreset:cut];
}

//-----------------------------
//
//-----------------------------
- (unsigned long)remove:(int)index {
    if (index<_size) {
        [_items removeObjectAtIndex:index];
        _size--;
        if ((_position>0) && (index<=_position)) _position--;
    }
    if (_size>0) {
        FileNode *item=[_items objectAtIndex:index];
        _curEntryLbl = [NSString stringWithFormat:@"(%d/%d) %@",_position+1,_size,item.name];
    } else {
        
    }
}

//-----------------------------
//
//-----------------------------
- (void)clear {
    [_items removeAllObjects];
    _size=0;
    _position=0;
}

//-----------------------------
//
//-----------------------------
- (const char *)getCurPresetCleanTitle {
    const char *filename=[[(FileNode*)[_items objectAtIndex:_position] path] UTF8String];
    const char *title=strchr(filename,'/');
    while (title) {
        if (strncasecmp(title+1,"presets/",strlen("presets/"))==0) {
            title=strchr(title+1,'/')+1;
            break;
        }
        title=strchr(title+1,'/');
    }
    if (!title) title=filename;
    return title;
}

//-----------------------------
//
//-----------------------------
- (const char *)getPresetCleanTitle:(int)index {
    if ((index<0)||(index>=_size)) return NULL;
    const char *filename=[[(FileNode*)[_items objectAtIndex:index] path] UTF8String];
    const char *title=strchr(filename,'/');
    while (title) {
        if (strncasecmp(title+1,"presets/",strlen("presets/"))==0) {
            title=strchr(title+1,'/')+1;
            break;
        }
        title=strchr(title+1,'/');
    }
    if (!title) title=filename;
    return title;
}


//-----------------------------
//
//-----------------------------
- (const char*)getPath:(int)index {
    if ((index<0)||(index>=_size)) return NULL;
    FileNode *item=[_items objectAtIndex:index];
    
    return [item.path UTF8String];
}


//-----------------------------
//
//-----------------------------
- (const char*)getTitle:(int)index {
    if ((index<0)||(index>=_size)) return NULL;
    FileNode *item=[_items objectAtIndex:index];
    return [item.name UTF8String];
}

//-----------------------------
//
//-----------------------------
- (const char*)getCurLabel {
    return [_curEntryLbl UTF8String];
}

//-----------------------------
//
//-----------------------------
- (int)getSize {
    return _size;
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
