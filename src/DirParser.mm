//
//  DirParser.mm
//  modizer
//
//  Created by Yohann Magnien David on 26/10/2025.
//

#define MDZ_PMPLAYLIST_VERSION 1

typedef struct {
    int version;
    int itemsNb;
    int pltype;
    char name[64];
} MDZPlaylist_Header_t;

#define MDZ_PLAYLIST_MAX_RETRY 32

#import "DirParser.h"
#import "ModizerConstants.h"

#include "zlib.h"


@implementation FileNode

- (instancetype)initWithPath:(NSString *)localpath root:(NSString *)rootpath type:(uint8_t)presetType {
    self = [super init];
    if (self) {
        _localpath = localpath;
        _rootpath = rootpath;
        _name = [_localpath lastPathComponent];
        _children = nil;
        _isSelected = TRUE;
        _selectedChildren = 0;
        _isFullySelected = FALSE;
        _shouldPropagateStatus = FALSE;
        _isFavorite = FALSE;
        _isMissing = FALSE;
        _presetType=presetType;
        
        NSError *error;
        NSFileManager *fm = [NSFileManager defaultManager];
        NSDictionary *attrs = [fm attributesOfItemAtPath:[_rootpath stringByAppendingString:_localpath] error:&error];
        
        if (error) {
            _isMissing=TRUE;
            MDZELog("FileNode error for %@: %@",_localpath,error)
        } else {
            if (attrs) {
                _fileSize = [attrs fileSize];
                _modificationDate = [attrs fileModificationDate];
                
                BOOL isDir = NO;
                [fm fileExistsAtPath:[_rootpath stringByAppendingString:_localpath] isDirectory:&isDir];
                _isDirectory = isDir;
            }
        }
    }
    return self;
}

- (NSString*)getFullPath {
    return [_rootpath stringByAppendingString:_localpath];
}

-(bool)isStringInArray:(NSString *)string array:(NSArray*)arr {
    bool ret=true;
    for (NSString *filter in arr) {
        if (![[string lowercaseString] containsString:[filter lowercaseString]]) {
            ret=false;
            break;
        }
    }
    return ret;
}

- (bool)filterNodes:(NSString *)pattern filterDir:(bool)filterDir {
    bool result=false;
    
    NSArray *filterStrings=[pattern componentsSeparatedByString:@" "];
    
    if (filterDir) {
        // Filter dir like files
        if ([self isStringInArray:self.name array:filterStrings]) {
            result=true;
        }
    } else if (!self.isDirectory) {
        // Filter files only
        if ([self isStringInArray:self.name array:filterStrings]) {
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

- (void)clearSelected {
    _isSelected=false;
    for (FileNode *child in _children) [child clearSelected];
}

- (NSArray<FileNode *> *)getFilesArray {
    NSMutableArray<FileNode *> *result = [NSMutableArray array];
    [self flattenFileNode:self intoArray:result];
    return [result copy];
}

- (void)flattenFileNode:(FileNode *)node intoArray:(NSMutableArray<FileNode *> *)array {
    if (!node.isDirectory) [array addObject:node];
    for (FileNode *child in node.children) {
        [self flattenFileNode:child intoArray:array];
    }
}
- (void)setSelectedFromPL:(NSArray *)plNodes {
    int plSize;
    //1st get all paths in an array
    NSMutableArray *pathsPL=[NSMutableArray arrayWithCapacity:[plNodes count]];
    for (FileNode *node in plNodes) {
        [pathsPL addObject:node.localpath];
    }
    //Sort it and remove potential duplicates
    NSOrderedSet *orderedPL=[NSOrderedSet orderedSetWithArray:pathsPL];
    
    //Build an array of FileNode to update
    NSArray *fnodes=[self getFilesArray];
    
    int posPL=0;
    int sizePL=(int)[orderedPL count];
    if (!sizePL) return;
    NSString *plPath=[orderedPL objectAtIndex:posPL];
    for (FileNode *node in fnodes) {
        NSString *filePath=node.localpath;
        if ([filePath isEqualToString:plPath]) {
            //file is matching PL entry, move to next PL entry
            node.isSelected=true;
            posPL++;
            if (posPL>=sizePL) break;
            plPath=[orderedPL objectAtIndex:posPL];
        } else while ([filePath compare:plPath]==NSOrderedDescending){
            //file is after pl entry, move pl entry to next one
            posPL++;
            if (posPL>=sizePL) break;
            plPath=[orderedPL objectAtIndex:posPL];
        }
    }
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

- (instancetype)init:(projectm_handle)pmh name:(NSString*)name {
    _items=[[NSMutableArray alloc] init];
    _position=0;
    _playlistName=name;
    _size=(int)[_items count];
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


- (instancetype)initWithArray:(NSArray*)array pmh:(projectm_handle)pmh name:(NSString*)name;{
    _items=[NSMutableArray arrayWithArray:array];
    _position=0;
    _playlistName=name;
    _size=(int)[_items count];
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
    projectm_load_preset_file(_pmh,"idle://Geiss & Sperl - Feedback (projectM idle HDR mix).milk",NULL);
    _curEntryLbl=[NSString stringWithFormat:@"No preset found. Activate bundled presets or copy milk files in '%s/presets' & images in '%s/textures' folders.",PM_ROOT_FOLDER_CUSTOM,PM_ROOT_FOLDER_CUSTOM];
}

- (void)setItems:(NSArray*)array {
    _items=[NSMutableArray arrayWithArray:array];
    _position=0;
    _size=(int)[_items count];
    _history=[[NSMutableArray alloc] init];
}

- (void)addItems:(NSArray*)array {
    [_items addObjectsFromArray:array];
    _position=0;
    _size=(int)[_items count];
}


- (void)setShuffle:(bool)active {
    _shuffle=active;
}

- (void)loadCurrentPreset:(bool)cut {
    //Load new preset
    FileNode *item;
    if (_size) {
        int retry_counter=0;
        while (1) {
            _lastFailed=false;
            item=[_items objectAtIndex:_position];
            projectm_load_preset_file(_pmh, [[item getFullPath] UTF8String], !cut);
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
    }
    
    if (_size==0) {
        [self loadIdlePreset];
    } else {
        item=[_items objectAtIndex:_position];
        _curEntryLbl = [NSString stringWithFormat:@"(%d/%d) %@",_position+1,_size,item.name];
    }
}
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

- (int)getPos {
    return _position;
}

- (void)setPos:(int)pos cut:(bool)cut {
    if (pos<_size) _position=pos;
    
    [self loadCurrentPreset:cut];
}

- (void)remove:(int)index {
    if ((index<0)||(index>=_size)) return;
        
    [_items removeObjectAtIndex:index];
    _size--;
    if ((_position>0) && (index<=_position)) _position--;
    
    if (_size>0) {
        FileNode *item=[_items objectAtIndex:_position];
        _curEntryLbl = [NSString stringWithFormat:@"(%d/%d) %@",_position+1,_size,item.name];
    }
}

- (void)clear {
    [_items removeAllObjects];
    _size=0;
    _position=0;
}

- (const char *)getCurPresetCleanTitle {
    if (_size) {
        const char *filename=[[(FileNode*)[_items objectAtIndex:_position] localpath] UTF8String];
        const char *title=strchr(filename,'/');
        while (title) {
            if (strncasecmp(title+1,"presets/./",strlen("presets/./"))==0) {
                title=strchr(title+1,'/')+1;
                break;
            }
            title=strchr(title+1,'/');
        }
        if (!title) title=filename;
        return title;
    } else {
        return [_curEntryLbl UTF8String];
    }
}

- (const char *)getPresetCleanTitle:(int)index {
    if ((index<0)||(index>=_size)) return NULL;
    const char *filename=[[(FileNode*)[_items objectAtIndex:index] localpath] UTF8String];
    const char *title=strchr(filename,'/');
    while (title) {
        if (strncasecmp(title+1,"presets/./",strlen("presets/./"))==0) {
            title=strchr(title+1,'/')+1;
            break;
        }
        title=strchr(title+1,'/');
    }
    if (!title) title=filename;
    return title;
}


- (const char*)getPath:(int)index {
    if ((index<0)||(index>=_size)) return NULL;
    FileNode *item=[_items objectAtIndex:index];
    
    return [item.localpath UTF8String];
}


- (const char*)getTitle:(int)index {
    if ((index<0)||(index>=_size)) return NULL;
    FileNode *item=[_items objectAtIndex:index];
    return [item.name UTF8String];
}

- (const char*)getCurLabel {
    return [_curEntryLbl UTF8String];
}

- (int)getSize {
    return _size;
}


- (bool)getFavStatus {
    bool ret=false;
    if (_size>0) {
        FileNode *_entry=[_items objectAtIndex:_position];
        ret=_entry.isFavorite;
    }
    return ret;
}

- (void)setFavStatus:(bool)favorite {
    if (_size>0) {
        FileNode *_entry=[_items objectAtIndex:_position];
        _entry.isFavorite=favorite;
    }
}

- (NSString*)getLocalPathBundle:(NSString*)fullPath {
    
}

- (NSString*)getFullPathDocument:(NSString*)localPath {
    
}

- (int)savePlaylist {
    gzFile f;
    f=gzopen([[NSHomeDirectory() stringByAppendingPathComponent:@"Documents/modizerPresetsPL.pmpl"] UTF8String],"wb");
    if (f) {
        //Write header
        MDZPlaylist_Header_t header;
        header.version=MDZ_PMPLAYLIST_VERSION;
        header.pltype=0;
        header.itemsNb=_size;
        snprintf(header.name,64,"%s",[_playlistName UTF8String]);
        gzwrite(f,&header,sizeof(MDZPlaylist_Header_t));
        
        //Write path for each entries
        for (FileNode *node in _items) {
            char isFav=node.isFavorite;
            uint8_t presetType=node.presetType;
            const char *str=[node.localpath UTF8String];
            int strLen=(int)strlen(str);
            if (strLen>1023) {
                strLen=1023;
                MDZELog("PM playlist/Saving: too long file path for saving (> 1023)")
            }
            gzwrite(f,&isFav,sizeof(char));
            gzwrite(f,&presetType,sizeof(uint8_t));
            gzwrite(f,&strLen,sizeof(int));
            gzwrite(f,str,strLen);
            
        }
        gzclose(f);
    } else {
        MDZELog("PM playlist/Saving: cannot open saving file for writing");
        return -1;
    }
    return 0;
}

- (int)loadPlaylist {
    int missing_counter=0;
    NSString *pmBundleDir = [NSString stringWithFormat:@"%@/projectm/assets/presets",[[NSBundle mainBundle] resourcePath]];
    NSString *pmCustomDir = [NSString stringWithFormat:@"%@/Documents%s/presets",NSHomeDirectory(),PM_ROOT_FOLDER_CUSTOM];
    
    gzFile f;
    f=gzopen([[NSHomeDirectory() stringByAppendingPathComponent:@"Documents/modizerPresetsPL.pmpl"] UTF8String],"rb");
    if (f) {
        int readBytes;
        
        //REad header
        MDZPlaylist_Header_t header;
        readBytes=gzread(f,&header,sizeof(MDZPlaylist_Header_t));
        
        if (readBytes<sizeof(MDZPlaylist_Header_t)) {
            MDZELog("PM playlist/Loading: cannot read  file (modizerPresetsPL.pmpl)")
            gzclose(f);
            return -1;
        }
        if (header.version!=MDZ_PMPLAYLIST_VERSION) {
            MDZELog("PM playlist/Loading: wrong version")
            gzclose(f);
            return -2;
        }
        
        [self clear];
        _playlistName=[NSString stringWithUTF8String:header.name];
        
        char str[1024];
        for (int i=0;i<header.itemsNb;i++) {
            char isFav;
            readBytes=gzread(f,&isFav,sizeof(char));
            if (readBytes!=sizeof(char)) {
                MDZELog("PM playlist/Loading: wrong data (isFav) for entry %d, aborting",i)
                gzclose(f);
                return -3;
            }
            uint8_t presetType;
            readBytes=gzread(f,&presetType,sizeof(char));
            if (readBytes!=sizeof(char)) {
                MDZELog("PM playlist/Loading: wrong data (presetType) for entry %d, aborting",i)
                gzclose(f);
                return -3;
            }
            
            int strLen;
            readBytes=gzread(f,&strLen,sizeof(int));
            if (strLen>1023) {
                MDZELog("PM playlist/Loading: too long path string (>1023) for entry %d, limiting",i)
            }
            readBytes=gzread(f,&str,strLen);
            if (readBytes!=strLen) {
                MDZELog("PM playlist/Loading: cannot read string data for entry %d, aborting",i)
                gzclose(f);
                return -3;
            }
            str[strLen]=0;
            NSString *rootPath;
            if (presetType==MDZ_PLAYLIST_FNODE_Bundle) rootPath=pmBundleDir;
            else if (presetType==MDZ_PLAYLIST_FNODE_Custom) rootPath=pmCustomDir;
            else rootPath=pmCustomDir; //default to custom

            FileNode *node=[[FileNode alloc] initWithPath:[NSString stringWithUTF8String:str] root:rootPath type:presetType];
            if (node.isMissing) {
                missing_counter++;
            } else {
                node.isSelected=TRUE;
                node.isFavorite=(isFav?TRUE:FALSE);
                [_items addObject:node];
            }
        }
        gzclose(f);
    }
    if (missing_counter) {
        MDZILog("PM playlist/Loading: %d entries are missing in filesystem",missing_counter)
    }
    _size=(int)[_items count];
    return missing_counter;
}

- (void)updateFileNodeStatus:(FileNode*)fnode {
    [fnode clearSelected];
    if (_size) [fnode setSelectedFromPL:_items];
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

- (FileNode *)parseDirectoryAtPath:(NSString *)path type:(uint8_t)type error:(NSError **)error {
    // Check if path exists
    NSFileManager *fm = [NSFileManager defaultManager];
    BOOL exists = [fm fileExistsAtPath:path];
    if (!exists) {
        if (error) {
            *error = [NSError errorWithDomain:NSCocoaErrorDomain
                                         code:NSFileReadNoSuchFileError
                                     userInfo:@{NSLocalizedDescriptionKey: @"Path does not exist"}];
        }
        return nil;
    }
    
    return [self parseDirectoryAtPathInternal:@"/." root:path type:type depth:0 error:error];
}

- (FileNode *)parseDirectoryAtPathInternal:(NSString *)path root:(NSString *)rootPath type:(uint8_t)type depth:(NSInteger)depth error:(NSError **)error {
    NSFileManager *fm = [NSFileManager defaultManager];
    
    FileNode *node = [[FileNode alloc] initWithPath:path root:rootPath type:type];
    // If it's a file or we've reached max depth, return the node
    if (!node.isDirectory || (self.maxDepth >= 0 && depth >= self.maxDepth)) {
        if (self.filterExt && [[node.name pathExtension] caseInsensitiveCompare:self.filterExt]) return nil;
        return node;
    }
    
    // Get directory contents
    NSArray *contents = [fm contentsOfDirectoryAtPath:[rootPath stringByAppendingString:path] error:error];
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
        FileNode *childNode = [self parseDirectoryAtPathInternal:itemPath root:rootPath type:type depth:depth + 1 error:nil];
        
        if (childNode) {
            if (node.children==nil) node.children=[NSMutableArray array];
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
