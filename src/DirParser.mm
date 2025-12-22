//
//  DirParser.mm
//  modizer
//
//  Created by Yohann Magnien David on 26/10/2025.
//
 
//flag to activate optimized version, not blocking rendering.
//load presets in 2 steps,
// 1. compiles the shader in a background thread (no opengl calls except compiling shader)
// 2. second initialize opengl rendering stuff and load textures

#define PM_LOAD_MODE_ASYNC

#define MDZ_PLAYLIST_MAX_RETRY 32
#define MDZ_PLAYLIST_MAX_PRELOAD_RETRY 4
#define MDZ_PMPLAYLIST_VERSION 1

typedef struct {
    int version;
    int itemsNb;
    int pltype;
    char name[64];
} MDZPlaylist_Header_t;

typedef struct {
    int version;
    int itemsNb;
    char name[64];
} MDZFavorites_Header_t;

extern bool _pmPresetNewLoaded;

#import "DirParser.h"
#import "ModizerConstants.h"
#import "ModizFileHelper.h"

#include "zlib.h"

#include "RenderUtils.h"

#include <pthread.h>
extern pthread_mutex_t pm_mutex,gl_mutex;

@implementation FileNode

- (instancetype)initWithPath:(NSString *)localpath root:(NSString *)rootpath type:(uint8_t)presetType {
    self = [super init];
    if (self) {
        _localpath = localpath;
        _rootpath = rootpath;
        _name = [[_localpath lastPathComponent] stringByDeletingPathExtension];
        _children = nil;
        _isSelected = TRUE;
        _selectedChildren = 0;
        _isFullySelected = FALSE;
        _shouldPropagateStatus = FALSE;
        _isFavorite = FALSE;
        _isMissing = FALSE;
        _entries = 0;
        _presetType=presetType;
        
        NSError *error;
        NSFileManager *fm = [NSFileManager defaultManager];
        
        BOOL isDir = NO;
        if (![fm fileExistsAtPath:[_rootpath stringByAppendingString:_localpath] isDirectory:&isDir]) _isMissing=true;
        _isDirectory = isDir;
    }
    return self;
}

- (instancetype)initWithPathIsFile:(NSString *)localpath root:(NSString *)rootpath type:(uint8_t)presetType {
    self = [super init];
    if (self) {
        _localpath = localpath;
        _rootpath = rootpath;
        _name = [[_localpath lastPathComponent] stringByDeletingPathExtension];
        _children = nil;
        _isSelected = TRUE;
        _selectedChildren = 0;
        _isFullySelected = FALSE;
        _shouldPropagateStatus = FALSE;
        _isFavorite = FALSE;
        _isMissing = FALSE;
        _entries = 0;
        _presetType=presetType;
        
        _isDirectory = false;
    }
    return self;
}

- (instancetype)initWithPathIsDir:(NSString *)localpath root:(NSString *)rootpath type:(uint8_t)presetType {
    self = [super init];
    if (self) {
        _localpath = localpath;
        _rootpath = rootpath;
        _name = [[_localpath lastPathComponent] stringByDeletingPathExtension];
        _children = nil;
        _isSelected = TRUE;
        _selectedChildren = 0;
        _isFullySelected = FALSE;
        _shouldPropagateStatus = FALSE;
        _isFavorite = FALSE;
        _isMissing = FALSE;
        _entries = 0;
        _presetType=presetType;
        
        _isDirectory = true;
    }
    return self;
}

- (void) printNodeTree {
    MDZILog("%@",_localpath);
    for (FileNode *child in _children) {
        [child printNodeTree];
    }
}

- (NSString*)getFullPath {
    return [_rootpath stringByAppendingString:_localpath];
}

- (NSString*)getLocalPath {
    return _localpath;
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
    
    NSArray *filterStrings=[[pattern stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]] componentsSeparatedByString:@" "];
    
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

- (void)clearFavorites {
    _isFavorite=false;
    for (FileNode *child in _children) [child clearFavorites];
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
- (void)setSelectedFromPL:(NSArray *)plNodes{
    int plSize;
    //1st get all paths in an array, filter by preset type (bundle/custom)
    NSMutableArray *pathsPL=[NSMutableArray arrayWithCapacity:[plNodes count]];
    for (FileNode *node in plNodes) {
        if (node.presetType==self.presetType) [pathsPL addObject:node.localpath];
    }
    //Sort it and remove potential duplicates
    //NSOrderedSet *orderedPL=[NSOrderedSet orderedSetWithArray:pathsPL];
    
    [pathsPL sortUsingComparator:^NSComparisonResult(NSString *str1, NSString *str2) {
        NSString *strtmp1;
        NSString *strtmp2;
        strtmp1=[str1 stringByReplacingOccurrencesOfString:@"/" withString:@"\0"];
        strtmp2=[str2 stringByReplacingOccurrencesOfString:@"/" withString:@"\0"];
        return [strtmp1 caseInsensitiveCompare:strtmp2];
    }];
    
    //Build an array of FileNode to update
    NSArray *fnodes=[self getFilesArray];
    
    int posPL=0;
    int sizePL=(int)[pathsPL count];
    if (!sizePL) return;
    NSString *plPath=[pathsPL objectAtIndex:posPL];
    plPath=[plPath stringByReplacingOccurrencesOfString:@"/" withString:@"\0"];
    for (FileNode *node in fnodes) {
        NSString *filePath=node.localpath;
        filePath=[filePath stringByReplacingOccurrencesOfString:@"/" withString:@"\0"];
        if ([filePath isEqualToString:plPath]) {
            //file is matching PL entry, move to next PL entry
            node.isSelected=true;
            posPL++;
            if (posPL>=sizePL) break;
            plPath=[pathsPL objectAtIndex:posPL];
            plPath=[plPath stringByReplacingOccurrencesOfString:@"/" withString:@"\0"];
        } else while ([filePath  caseInsensitiveCompare:plPath]==NSOrderedDescending){
            //file is after pl entry, move pl entry to next one
            posPL++;
            if (posPL>=sizePL) break;
            plPath=[pathsPL objectAtIndex:posPL];
            plPath=[plPath stringByReplacingOccurrencesOfString:@"/" withString:@"\0"];
        }
    }
}

- (void)setFavoritesFromFL:(NSArray *)orderedFL {
    //Build an array of FileNode to update
    NSArray *fnodes=[self getFilesArray];
    
    
    
    int posFL=0;
    int sizeFL=(int)[orderedFL count];
    if (!sizeFL) return;
    NSString *flPath=[[orderedFL objectAtIndex:posFL] substringFromIndex:3];
    flPath=[flPath stringByReplacingOccurrencesOfString:@"/" withString:@"\0"];
//    MDZILog("fav to match: %@",flPath);
    for (FileNode *node in fnodes) {
        NSString *filePath=node.localpath;
        filePath=[filePath stringByReplacingOccurrencesOfString:@"/" withString:@"\0"];
//        MDZILog("node to match %@",filePath);
        if ([filePath isEqualToString:flPath]) {
            //file is matching FL entry, move to next PL entry
            node.isFavorite=true;
            posFL++;
            if (posFL>=sizeFL) break;
            flPath=[[orderedFL objectAtIndex:posFL] substringFromIndex:3];
            flPath=[flPath stringByReplacingOccurrencesOfString:@"/" withString:@"\0"];
//            MDZILog("found, next fav to match: %@",flPath);
        } else while ([filePath  caseInsensitiveCompare:flPath]==NSOrderedDescending){
            //file is after fl entry, move fl entry to next one
            posFL++;
            if (posFL>=sizeFL) break;
            flPath=[[orderedFL objectAtIndex:posFL] substringFromIndex:3];
            flPath=[flPath stringByReplacingOccurrencesOfString:@"/" withString:@"\0"];
//            MDZILog("not found, next fav to match: %@",flPath);
        }
    }
}


@end

int moveToNextPresetRequest;

void MDZOnPresetSwitchRequested(bool isHardCut, void* userData) {
    //MDZILog("should change preset, hard cut %d, userData %s",isHardCut,(userData?"yes":"no"));
    if (userData==NULL) return;
    MDZPlaylist *mdzPL=(__bridge MDZPlaylist *)userData;
    moveToNextPresetRequest=1;
    [mdzPL next:isHardCut];
}

void MDZOnPresetSwitchFailed(const char* presetFilename, const char* message, void* userData) {
    MDZELog("couldnt switch to preset %s, reason %s",presetFilename,message);
    if (userData==NULL) return;
    MDZPlaylist *mdzPL=(__bridge MDZPlaylist *)userData;
    mdzPL.lastFailed=true;
}



@implementation MDZPlaylist

- (instancetype)init:(projectm_handle)pmh name:(NSString*)name {
    self = [super init];
    moveToNextPresetRequest=0;
    _retry_counter=0;
    _retry_preLoadcounter=0;
    _items=[[NSMutableArray alloc] init];
    _position=0;
    _nextPosition=-1;
    _playlistName=name;
    _size=(int)[_items count];
    _shuffle=false;
    _history=[[NSMutableArray alloc] init];
    _pmh=pmh;
    _compP=0; _nextCompP=0;
    _warpP=0; _nextWarpP=0;
    _curEntryLbl = @"";
    [self loadIdlePreset];
    
    projectm_set_preset_switch_requested_event_callback(_pmh, nullptr, nullptr);
    projectm_set_preset_switch_failed_event_callback(_pmh, nullptr, nullptr);
    
    projectm_set_preset_switch_requested_event_callback(_pmh, &MDZOnPresetSwitchRequested, (__bridge void*)self);
    projectm_set_preset_switch_failed_event_callback(_pmh, &MDZOnPresetSwitchFailed, (__bridge void*)self);
    _lastFailed=false;

    return self;
}


- (instancetype)initWithArray:(NSArray*)array pmh:(projectm_handle)pmh name:(NSString*)name;{
    self = [super init];
    moveToNextPresetRequest=0;
    _retry_counter=0;
    _retry_preLoadcounter=0;
    _items=[NSMutableArray arrayWithArray:array];
    _position=0;
    _nextPosition=-1;
    _playlistName=name;
    _size=(int)[_items count];
    _shuffle=false;
    _history=[[NSMutableArray alloc] init];
    _pmh=pmh;
    _curEntryLbl = @"";
    [self loadIdlePreset];
    
    projectm_set_preset_switch_requested_event_callback(_pmh, nullptr, nullptr);
    projectm_set_preset_switch_failed_event_callback(_pmh, nullptr, nullptr);
    
    projectm_set_preset_switch_requested_event_callback(_pmh, &MDZOnPresetSwitchRequested, (__bridge void*)self);
    projectm_set_preset_switch_failed_event_callback(_pmh, &MDZOnPresetSwitchFailed, (__bridge void*)self);
    _lastFailed=false;
    return self;
}

- (void)loadIdlePreset {
    pthread_mutex_lock(&pm_mutex);
    projectm_load_preset_file(_pmh,"idle://Geiss & Sperl - Feedback (projectM idle HDR mix).milk",NULL);
    pthread_mutex_unlock(&pm_mutex);
    _curEntryLbl = [NSString stringWithFormat:@"No preset found. Activate bundled presets or copy milk files in '%s/presets' & images in '%s/textures' folders.",PM_ROOT_FOLDER_CUSTOM,PM_ROOT_FOLDER_CUSTOM];
    _pmPresetNewLoaded=true;
    
}

- (void)setItems:(NSArray*)array {
    _items=[NSMutableArray arrayWithArray:array];
    _position=0;
    _nextPosition=-1;
    _size=(int)[_items count];
    [_history removeAllObjects];
}

- (void)addItems:(NSArray*)array {
    [_items addObjectsFromArray:array];
    _position=0;
    _nextPosition=-1;
    _size=(int)[_items count];
}


- (void)setShuffle:(bool)active {
    if (active!=_shuffle) {
        _shuffle=active;
        //change of mode, recompute next position but don't change current one
        [self computeNext:false];
        //preload the one coming after
        [self releaseNextPreset];
        if ((_nextPosition>=0)&&(_nextPosition<_size)) [self preloadNextPreset:[_items objectAtIndex:_nextPosition]];
    }
}

- (void)releaseNextPreset {
    if (self.nextCompP) RenderUtils::releaseProgram(self.nextCompP);
    self.nextCompP=0;
    if (self.nextWarpP) RenderUtils::releaseProgram(self.nextWarpP);
    self.nextWarpP=0;
    self.nextFilepath=nil;
}

- (void)uncompressIfNeeded:(NSString *)filePath newPath:(const char**)newPath {
    if ([[[filePath pathExtension] lowercaseString] isEqualToString:@"milkz"]) {
        // Create temporary file URL
        NSString *newFilePath = [NSString stringWithFormat:@"%@tmpPreset.milk",NSTemporaryDirectory()];
        
        int chunk_size=32768;
        
        gzFile inFile;
        FILE *tmpFile;
        inFile=gzopen([filePath UTF8String],"rb");
        if (inFile==NULL) {
            MDZELog("cannot open compressed preset %@",filePath);
            *newPath=NULL;
            return;
        }
        tmpFile=fopen([newFilePath UTF8String],"wb");
        if (tmpFile==NULL) {
            MDZELog("cannot open temp preset file to uncompress %@",filePath);
            gzclose(inFile);
            *newPath=NULL;
            return;
        }
        char *buffer=(char*)malloc(chunk_size);
        for (;;) {
            int read_bytes=gzread(inFile, buffer, chunk_size);
            if (read_bytes) fwrite(buffer, read_bytes, 1, tmpFile);
            if (read_bytes<chunk_size) break;
        }
        free(buffer);
        fclose(tmpFile);
        gzclose(inFile);
        
        *newPath=[newFilePath UTF8String];
    }
}

- (void)preloadNextPreset:(FileNode*)item {
    
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
        //Load new preset
        const char *filepath=[[item getFullPath] UTF8String];
        pthread_mutex_lock(&pm_mutex);
        [self releaseNextPreset];
        if (self.size) {
            self.nextFilepath=[NSString stringWithString:[item getFullPath] ];
            self.lastFailed=false;
            
            //check if compressed milk preset, if so uncompress and update filepath accordingly
            [self uncompressIfNeeded:[item getFullPath] newPath:&filepath];
            
            projectm_preload_preset_file(self.pmh, filepath, &self->_nextWarpP, &self->_nextCompP);
        }
        
        [[NSOperationQueue mainQueue] addOperationWithBlock:^{
            pthread_mutex_unlock(&pm_mutex);
            if (!self.lastFailed) {
                if (self.nextWarpP && self.nextCompP) {
                    //MDZILog("compiling next preset: ok | warp %d comp %d",self.nextWarpP,self.nextCompP);
                    self.retry_preLoadcounter=0;
                } else {
                    MDZFLog("compiling next preset: ko | warp %d comp %d",self.nextWarpP,self.nextCompP);
                    //Clean if warp or comp program was compiled
                    [self releaseNextPreset];
                    //Try to reload
                    MDZILog("retry");
                    self.retry_preLoadcounter++;
                    if (self.retry_preLoadcounter<MDZ_PLAYLIST_MAX_PRELOAD_RETRY) [self preloadNextPreset:item];
                }
            }
        }];
    });
}

- (void)loadASyncCurrentPreset:(bool)cut {
    FileNode *item;
    if (self.size==0) {
        moveToNextPresetRequest=0;
        return;
    }
    item=[self.items objectAtIndex:self.position];
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
        const char *filepath=[[item getFullPath] UTF8String];
        //Load new preset
        pthread_mutex_lock(&pm_mutex);
        if ((self.nextWarpP && self.nextCompP && [self.nextFilepath isEqualToString:[item getFullPath]])) {
            self.warpP=self.nextWarpP;
            self.compP=self.nextCompP;
            self.nextFilepath=nil;
            self.nextWarpP=0;
            self.nextCompP=0;
            self.lastFailed=false;
            //MDZILog("shortcut, already compiled");
            
            //check if compressed milk preset, if so uncompress and update filepath accordingly
            [self uncompressIfNeeded:[item getFullPath] newPath:&filepath];
        } else {
            self.warpP=0;
            self.compP=0;
            self.lastFailed=false;
            
            //check if compressed milk preset, if so uncompress and update filepath accordingly
            [self uncompressIfNeeded:[item getFullPath] newPath:&filepath];
            
            projectm_preload_preset_file(self.pmh, filepath, &self->_warpP, &self->_compP);
        }
        
        [[NSOperationQueue mainQueue] addOperationWithBlock:^{
            const char *filepath=[[item getFullPath] UTF8String];
            if (!self.lastFailed) {
                if (self.warpP && self.compP) {
                    moveToNextPresetRequest=0;
                    //MDZILog("preload ok, warpP %d compP %d",self.warpP,self.compP);
                    START_PROFILE
                    self.lastFailed=false;
                    //pthread_mutex_lock(&pm_mutex);
                    
                    if ([[[[item getFullPath] pathExtension] lowercaseString] isEqualToString:@"milkz"]) {
                        filepath=[[NSString stringWithFormat:@"%@tmpPreset.milk",NSTemporaryDirectory()] UTF8String];
                    }
                    
                    projectm_loadpreload_preset_file(self.pmh, filepath, self.warpP, self.compP, !cut);
                    
                    CHECK_PROFILE("preset loaded fast")
                    END_PROFILE
                    
                    if (!self.lastFailed) {
                        _pmPresetNewLoaded=true;
                        self.retry_counter=0;
                        FileNode *item=[self.items objectAtIndex:self.position];
                        self.curEntryLbl = [NSString stringWithFormat:@"(%d/%d) (%c)%@",self.position+1,self.size,
                                        (item.presetType==MDZ_PLAYLIST_FNODE_Bundle?'B':'C'),
                                        item.localpath];
                    }
                } else self.lastFailed=true;
            }
            //free the mem allocated by strdup
            pthread_mutex_unlock(&pm_mutex);
            //if it has failed, remove from list and try another one.
            //if list is empty, load idle preset
            if (self.lastFailed) {
                //Issue with last preset, remove from the list
                [self remove:self.position];
                //If list empty, exit
                if (self.size==0) {
                    moveToNextPresetRequest=0;
                    [self loadIdlePreset];
                    return;
                }
                //If too many attempt, exit, to avoid freezing app
                self.retry_counter++;
                if (self.retry_counter>MDZ_PLAYLIST_MAX_RETRY) {
                    moveToNextPresetRequest=0;
                    MDZFLog("Critical issue, cannot move to next projectm presets");
                    return;
                }
                [self loadASyncCurrentPreset:cut];
            }
        }];
    });
}

- (void)loadASyncPreset:(FileNode*)item cut:(bool)cut {
    
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
        const char *filepath=[[item getFullPath] UTF8String];
        //Load new preset
        pthread_mutex_lock(&pm_mutex);
        if (self.size) {
            self.warpP=0;
            self.compP=0;
            self.lastFailed=false;
            
            //check if compressed milk preset, if so uncompress and update filepath accordingly
            [self uncompressIfNeeded:[item getFullPath] newPath:&filepath];
            
            projectm_preload_preset_file(self.pmh, filepath, &self->_warpP, &self->_compP);
        }
        
        [[NSOperationQueue mainQueue] addOperationWithBlock:^{
            const char *filepath=[[item getFullPath] UTF8String];
            if (!self.lastFailed) {
                if (self.warpP && self.compP) {
                    
                    if ([[[[item getFullPath] pathExtension] lowercaseString] isEqualToString:@"milkz"]) {
                        filepath=[[NSString stringWithFormat:@"%@tmpPreset.milk",NSTemporaryDirectory()] UTF8String];
                    }
                    
                    //MDZILog("warpP %d compP %d",self.warpP,self.compP);
                    START_PROFILE
                    self.lastFailed=false;
                    projectm_loadpreload_preset_file(self.pmh, filepath, self.warpP, self.compP, !cut);
                    CHECK_PROFILE("preset loaded fast")
                    END_PROFILE
                    
                    if (!self.lastFailed) {
                        _pmPresetNewLoaded=true;
                        self.retry_counter=0;
                        self.curEntryLbl = [NSString stringWithFormat:@"(%c)%@",
                                        (item.presetType==MDZ_PLAYLIST_FNODE_Bundle?'B':'C'),
                                        item.localpath];
                    }
                }
            }
            pthread_mutex_unlock(&pm_mutex);
            //if it has failed, remove from list and try another one.
            //if list is empty, load idle preset
            if (self.lastFailed) {
                //Cannot load preset, do nothing as this method isn't use to move to next one
            }
        }];
    });
}

- (void)loadPreset:(FileNode*)file cut:(bool)cut {
#ifdef PM_LOAD_MODE_ASYNC
    [self loadASyncPreset:file cut:cut];
#else
    //Load new preset
    _lastFailed=false;
    START_PROFILE
    
    const char *filepath=[[file getFullPath] UTF8String];
    //check if compressed milk preset, if so uncompress and update filepath accordingly
    [self uncompressIfNeeded:[item getFullPath]  newPath:&filepath];
    
    projectm_load_preset_file(_pmh, filepath,!cut);
    CHECK_PROFILE("preset loaded normal")
    END_PROFILE
    
    if (!_lastFailed) {
        _pmPresetNewLoaded=true;
        _curEntryLbl = [NSString stringWithFormat:@"(%c)%@",
                        (file.presetType==MDZ_PLAYLIST_FNODE_Bundle?'B':'C'),
                        file.localpath];
    }
#endif
}


- (void)loadCurrentPreset:(bool)cut {
#ifdef PM_LOAD_MODE_ASYNC
    [self loadASyncCurrentPreset:cut];
#else
    //Load new preset
    FileNode *item;
    if (_size) {
        int retry_counter=0;
        while (1) {
            _lastFailed=false;
            item=[_items objectAtIndex:_position];
            START_PROFILE
            
            const char *filepath=[[item getFullPath] UTF8String];
            //check if compressed milk preset, if so uncompress and update filepath accordingly
            [self uncompressIfNeeded:[item getFullPath]  newPath:&filepath];
            
            projectm_load_preset_file(_pmh, filepath,!cut);
            CHECK_PROFILE("preset loaded normal")
            END_PROFILE
            
            
            if (!_lastFailed) {
                moveToNextPresetRequest=0;
                _pmPresetNewLoaded=true;
                /*
const char *strdata="\
img=MilkDrop3_024\n\
SpriteColorKey=0x000000\n\
SpriteLayer=1;\n\
SpriteBlend=3;\n\
SpriteAlpha=1.000000;\n\
SpriteSpeed=0.100000;\n\
init_1=burn=1;\n\
init_2=x=0.500000;\n\
init_3=y=0.500000;\n\
init_4=sx=-0.200000;\n\
init_5=sy=-0.200000;\n\
init_6=rot=1.000000;\n\
init_7=repeatX=1.000000;\n\
init_8=repeatY=1.000000;\n\
init_9=blendmode=3;\n\
code_1=a=1.0;";
*/
/*                const char *strdata="\
img=rose1\n\
SpriteColorKey=0x000000\n\
SpriteLayer=1\n\
SpriteBlend=7\n\
SpriteAlpha=1.000000\n\
SpriteSpeed=0.500000;\n\
init_1=burn=1.0;\n\
init_2=x=0.500000;\n\
init_3=y=0.500000;\n\
init_4=sx=-0.310000;\n\
init_5=sy=-0.310000;\n\
init_6=rot=1.000000;\n\
init_7=blendmode=3;\n\
init_8=repeatx=1.000000;\n\
init_9=repeaty=1.000000;\n\
code_1=new_scale=0.5+0.03*bass_att;\n\
code_2=sx=new_scale;\n\
code_3=sy=new_scale;\n\
code_4=a=1.0;\n\
";*/
//                projectm_sprite_create(_pmh,"milkdrop",strdata);
                break;
            }
            break;  //TO REVIEW
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
        moveToNextPresetRequest=0;
        [self loadIdlePreset];
    } else {
        item=[_items objectAtIndex:_position];
        _curEntryLbl = [NSString stringWithFormat:@"(%d/%d) (%c)%@",_position+1,_size,
                        (item.presetType==MDZ_PLAYLIST_FNODE_Bundle?'B':'C'),
                        item.localpath];
    }
#endif
}

- (void)computeNext:(bool)movePosition {
    if (_shuffle) {
        if (movePosition) {
            if (_nextPosition>=0) _position=_nextPosition;
            else _position=arc4random_uniform(_size);
        }
        _nextPosition=arc4random_uniform(_size);
    } else {
        if (movePosition) {
            if (_nextPosition>=0) _position=_nextPosition;
            else _position++;
            if (_position>=_size) _position=0;
        }
        _nextPosition=_position+1;
        if (_nextPosition>=_size) _nextPosition=0;
    }
}

- (void)next:(bool)cut {
    if (!_size) {
        moveToNextPresetRequest=0;
        [self loadIdlePreset];
        return;
    }
    //Store to history
    [_history addObject:[NSNumber numberWithInt:_position]];
    //update position to next one and compute the following one
    [self computeNext:true];
    //load next preset
    [self loadCurrentPreset:cut];
    //preload the one coming after
    if ((_nextPosition>=0)&&(_nextPosition<_size)) [self preloadNextPreset:[_items objectAtIndex:_nextPosition]];
}

- (void)last:(bool)cut {
    if (!_size) {
        [self loadIdlePreset];
        return;
    }
    //Is there something in history ?
    if ([_history count]) {
        //yes, we use it and remove entry
        _position = [(NSNumber*)[_history lastObject] intValue];
        [_history removeLastObject];
        //ensure position is valid
        if (_position<0) _position=0;
        if (_position>=_size) _position=_size-1;
        
        [self loadCurrentPreset:cut];
        [self computeNext:false];
        [self releaseNextPreset];
        if ((_nextPosition>=0)&&(_nextPosition<_size)) [self preloadNextPreset:[_items objectAtIndex:_nextPosition]];
    } else {
        //nothing in history, do prev
        [self prev:cut];
    }
}

- (void)prev:(bool)cut {
    if (!_size) {
        [self loadIdlePreset];
        return;
    }
    if (_shuffle) {
        _position=arc4random_uniform(_size);
    } else {
        if (_position>0) _position--;
        else _position=_size-1;
    }
    
    [self loadCurrentPreset:cut];
    [self computeNext:false];
    [self releaseNextPreset];
    if ((_nextPosition>=0)&&(_nextPosition<_size)) [self preloadNextPreset:[_items objectAtIndex:_nextPosition]];
}

- (int)getPos {
    return _position;
}

- (void)setPos:(int)pos cut:(bool)cut {
    if (!_size) {
        [self loadIdlePreset];
        return;
    }
    if (pos<_size) _position=pos;
    
    [self loadCurrentPreset:cut];
    [self computeNext:false];
    [self releaseNextPreset];
    if ((_nextPosition>=0)&&(_nextPosition<_size)) [self preloadNextPreset:[_items objectAtIndex:_nextPosition]];
}

- (void)remove:(int)index {
    if ((index<0)||(index>=_size)) return;
    
    _nextPosition=-1;
        
    [_items removeObjectAtIndex:index];
    _size--;
    if ((_position>0) && (index<=_position)) _position--;
    
    if (_size>0) {
        FileNode *item=[_items objectAtIndex:_position];
        _curEntryLbl = [NSString stringWithFormat:@"(%d/%d) (%c)%@",_position+1,_size,
                        ((item.presetType)==MDZ_PLAYLIST_FNODE_Bundle?'B':'C'),
                        item.name];
    }
}
- (void)removeCurEntry {
    if (_size==0) return;
    [self remove:_position];
}
- (void)loadCurEntry {
    if (_size==0) [self loadIdlePreset];
    [self loadCurrentPreset:true];
    [self computeNext:false];
    [self releaseNextPreset];
    if ((_nextPosition>=0)&&(_nextPosition<_size)) [self preloadNextPreset:[_items objectAtIndex:_nextPosition]];
}
- (void)moveTo:(FileNode*)node cut:(bool)cut {
    for (int i=0;i<[self.items count];i++) {
        FileNode *item=[self.items objectAtIndex:i];
        if ([node.name isEqualToString:item.name]) {
            [self setPos:i cut:cut];
            break;
        }
    }
}

- (void)clear {
    [_items removeAllObjects];
    _size=0;
    _position=0;
    _nextPosition=-1;
}

- (const char *)getCurPresetCleanTitle {
    if (_size) {
        const char *filename=[[(FileNode*)[_items objectAtIndex:_position] localpath] UTF8String];
        FileNode *fnode=[_items objectAtIndex:_position];
        const char *title=[[NSString stringWithFormat:@"(%c)%s",(fnode.presetType==MDZ_PLAYLIST_FNODE_Bundle?'B':'C'),filename] UTF8String];
        return title;
    } else {
        return [_curEntryLbl UTF8String];
    }
}

- (const char *)getPresetCleanTitle:(int)index {
    if ((index<0)||(index>=_size)) return NULL;
    FileNode *fnode=[_items objectAtIndex:index];
    const char *filename=[[fnode localpath] UTF8String];
    const char *title=[[NSString stringWithFormat:@"(%c)%s",(fnode.presetType==MDZ_PLAYLIST_FNODE_Bundle?'B':'C'),filename] UTF8String];
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

- (const char*)getCurFullpath {
    if (_size==0) return NULL;
    FileNode *item=[_items objectAtIndex:_position];
    return [item.localpath UTF8String];
}

- (int)getCurType {
    if (_size==0) return -1;
    FileNode *item=[_items objectAtIndex:_position];
    return item.presetType;
}

- (NSString*)getCurFullpathNS {
    if (_size==0) return NULL;
    FileNode *item=[_items objectAtIndex:_position];
    return item.localpath;
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
    return NULL;
}

- (NSString*)getFullPathDocument:(NSString*)localPath {
    return NULL;
}

- (int)savePlaylist {
    gzFile f;
    f=gzopen([[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:@"Documents/modizerPresetsPL.pmpl"] UTF8String],"wb");
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
                MDZELog("PM playlist/Saving: too long file path for saving (> 1023)");
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
    NSString *pmCustomDir = [NSString stringWithFormat:@"%@/Documents%s/presets",[ModizFileHelper getAppHomeDirectory],PM_ROOT_FOLDER_CUSTOM];
    
    gzFile f;
    f=gzopen([[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:@"Documents/modizerPresetsPL.pmpl"] UTF8String],"rb");
    if (f) {
        int readBytes;
        //Read header
        MDZPlaylist_Header_t header;
        readBytes=gzread(f,&header,sizeof(MDZPlaylist_Header_t));
        
        if (readBytes<sizeof(MDZPlaylist_Header_t)) {
            MDZELog("PM playlist/Loading: cannot read  file (modizerPresetsPL.pmpl)");
            gzclose(f);
            return -1;
        }
        if (header.version!=MDZ_PMPLAYLIST_VERSION) {
            MDZELog("PM playlist/Loading: wrong version");
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
                MDZELog("PM playlist/Loading: wrong data (isFav) for entry %d, aborting",i);
                gzclose(f);
                return -3;
            }
            uint8_t presetType;
            readBytes=gzread(f,&presetType,sizeof(char));
            if (readBytes!=sizeof(char)) {
                MDZELog("PM playlist/Loading: wrong data (presetType) for entry %d, aborting",i);
                gzclose(f);
                return -3;
            }
            
            int strLen;
            readBytes=gzread(f,&strLen,sizeof(int));
            if (strLen>1023) {
                MDZELog("PM playlist/Loading: too long path string (>1023) for entry %d, limiting",i);
            }
            readBytes=gzread(f,&str,strLen);
            if (readBytes!=strLen) {
                MDZELog("PM playlist/Loading: cannot read string data for entry %d, aborting",i);
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
        MDZILog("PM playlist/Loading: %d entries are missing in filesystem",missing_counter);
    }
    _size=(int)[_items count];
    _nextPosition=-1;
    return missing_counter;
}

- (void)updateFileNodeStatus:(FileNode*)fnode {
    [fnode clearSelected];
    if (_size) [fnode setSelectedFromPL:_items];
}

- (bool)setPosForPreset:(const char*)localPath type:(int)type{
    bool ret=false;
    NSString *str=[NSString stringWithUTF8String:localPath];
    int pos=0;
    for (FileNode *item in _items) {
        if ( ([str isEqualToString:item.localpath]) && (item.presetType==type)) {
            
            _curEntryLbl = [NSString stringWithFormat:@"(%d/%d) (%c)%@",pos+1,_size,
                            (item.presetType==MDZ_PLAYLIST_FNODE_Bundle?'B':'C'),
                            item.localpath];
            
            break;
        } else pos++;
    }
    if (pos<_size) {
        _position=pos;
        ret=true;
    }
    return ret;
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

- (FileNode *)parseFastDirectoryAtPath:(NSString *)path type:(uint8_t)type error:(NSError **)error {
    FileNode *startNode,*currentNode,*childNode;
    NSMutableArray *dirNodeStack;
    NSFileManager *fm = [NSFileManager defaultManager];
    NSArray *dirContent;
    
    START_PROFILE
    
    startNode=NULL;
    //List all entries
    NSURL *directoryURL = [NSURL fileURLWithPath:path];
    NSDirectoryEnumerator *directoryEnumerator =
    [fm enumeratorAtURL:directoryURL
                    includingPropertiesForKeys:@[NSURLPathKey, NSURLIsDirectoryKey]
                    options:NSDirectoryEnumerationSkipsHiddenFiles/*|NSDirectoryEnumerationSkipsSubdirectoryDescendants*/
                    errorHandler:nil];
    dirContent=[directoryEnumerator allObjects];
    
    CHECK_PROFILE("dir enum")
    
    if ([dirContent count]) {
        //Create first node
        startNode = [[FileNode alloc] initWithPathIsDir:@"/" root:path type:type];
        currentNode = startNode;

        //Sort dir content using path
        NSArray *sortedDirContent;
        if (type==MDZ_PLAYLIST_FNODE_Bundle) {
            //Bundle preset, can go faster assuming no exotic char in filenames
            sortedDirContent= [dirContent sortedArrayUsingComparator:^(id obj1, id obj2) {
                NSString *str1;
                NSString *str2;
                [(NSURL*)obj1 getResourceValue:&str1 forKey:NSURLPathKey error:nil];
                [(NSURL*)obj2 getResourceValue:&str2 forKey:NSURLPathKey error:nil];
                
                const char *cstr1=[str1 UTF8String];
                const char *cstr2=[str2 UTF8String];
                int compResult=0;
                char c1,c2;
                int pos=0;
                while (1) {
                    c1=cstr1[pos];
                    c2=cstr2[pos++];
                    if ((c1>='A') && (c1<='Z')) c1=c1+'a'-'A';
                    else if (c1=='/') c1=1;
                    if ((c2>='A') && (c2<='Z')) c2=c2+'a'-'A';
                    else if (c2=='/') c2=1;
                    compResult=c1-c2;
                    if (compResult) break;
                    if (!c1 || !c2) break;
                }
                if (compResult<0) return NSOrderedAscending;
                else if (compResult>0) return NSOrderedDescending;
                return NSOrderedSame;
            }];
        } else {
            sortedDirContent= [dirContent sortedArrayUsingComparator:^(id obj1, id obj2) {
                NSString *str1;
                NSString *str2;
                
                [(NSURL*)obj1 getResourceValue:&str1 forKey:NSURLPathKey error:nil];
                [(NSURL*)obj2 getResourceValue:&str2 forKey:NSURLPathKey error:nil];
                
                str1=[str1 stringByReplacingOccurrencesOfString:@"/" withString:@"\0"];
                str2=[str2 stringByReplacingOccurrencesOfString:@"/" withString:@"\0"];
                return [str1 caseInsensitiveCompare:str2];
            }];
        }
        CHECK_PROFILE("dir sort")
        
        //Prepare variables
        NSNumber *isDirectory = nil;
        NSString *entryPath,*currentLocalPath;
        int localPathStartPos=(int)[path length];
        currentLocalPath=@"/";
        //Stack to keep track of directories
        dirNodeStack=[[NSMutableArray alloc] init];
        
        //Start parsing
        for (NSURL *entryURL in sortedDirContent) {
            //Get dir flag, path and name
            [entryURL getResourceValue:&isDirectory forKey:NSURLIsDirectoryKey error:nil];
            [entryURL getResourceValue:&entryPath forKey:NSURLPathKey error:nil];
            
            //MDZILog("%@",[entryPath substringFromIndex:localPathStartPos]);
            
            if ([isDirectory boolValue]) {
                //Dir management
                NSString *childPath=[entryPath substringFromIndex:localPathStartPos];
                
                //ensure current node is the right one, if not pop back to the right one
                NSString *childPathDir=[childPath stringByDeletingLastPathComponent];
                while (![childPathDir isEqualToString:currentLocalPath]) {
                    if ([dirNodeStack count]==0) {
                        MDZFLog("Error in fast parser, no more dir in stack");
                        return NULL;
                    }
                    int entries_tmp=currentNode.entries;
                    currentNode=[dirNodeStack lastObject];
                    currentNode.entries+=entries_tmp;
                    currentLocalPath=[currentNode getLocalPath];
                    [dirNodeStack removeLastObject];
                }
                
                //new dir, push previous one to stack
                [dirNodeStack addObject:currentNode];
                
                
                childNode = [[FileNode alloc] initWithPathIsDir:childPath root:path type:type];
                if (currentNode.children==nil) currentNode.children=[NSMutableArray arrayWithObject:childNode];
                else [currentNode.children addObject:childNode];
                //and move dir as new current node
                currentNode=childNode;
                currentLocalPath=childPath;
            } else {
                //File, add it to children if matching the filter
                bool addToList=true;
                if (self.filterExt) {
                    bool isMatching=false;
                    for (NSString *ext in self.filterExt) {
                        if ( [[[entryPath pathExtension] lowercaseString] isEqualToString:ext] ) {
                            isMatching=true;
                            break;
                        }
                    }
                    addToList=isMatching;
                }
                
                if (addToList) {
                    NSString *childPath=[entryPath substringFromIndex:localPathStartPos];
                    //ensure current node is the right one, if not pop back to the right one
                    NSString *childPathDir=[childPath stringByDeletingLastPathComponent];
                    while (![childPathDir isEqualToString:currentLocalPath]) {
                        if ([dirNodeStack count]==0) {
                            MDZFLog("Error in fast parser, no more dir in stack");
                            return NULL;
                        }
                        int entries_tmp=currentNode.entries;
                        currentNode=[dirNodeStack lastObject];
                        currentNode.entries+=entries_tmp;
                        currentLocalPath=[currentNode getLocalPath];
                        [dirNodeStack removeLastObject];
                    }
                    
                    childNode = [[FileNode alloc] initWithPathIsFile:childPath root:path type:type];
                    if (currentNode.children==nil) currentNode.children=[NSMutableArray arrayWithObject:childNode];
                    else [currentNode.children addObject:childNode];
                    
                    currentNode.entries++;
                }
            }
        }
        
    }
    CHECK_PROFILE("process")
    [self removeEmptyNodes:startNode];
    CHECK_PROFILE("remove empty nodes")
    
    END_PROFILE
    return startNode;
}

-(void) removeEmptyNodes:(FileNode*)item {
    if (item.isDirectory==false) return;
    if (item==nil) return;
    int node_nb=[item.children count];
    for (int i=0;i<node_nb;i++) {
        FileNode *node=[item.children objectAtIndex:i];
        if (node.isDirectory) {
            if (node.entries==0) {
                //MDZILog("removing empty dir %@",node.localpath);
                [item.children removeObjectAtIndex:i];
                node_nb--;
                i--;
                if (node_nb==0) break;
            }
            else [self removeEmptyNodes:node];
        }
    }
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

@implementation MDZFavorites

- (instancetype)init {
    self = [super init];
    _bundlePresets=[[NSMutableOrderedSet alloc] init];
    _customPresets=[[NSMutableOrderedSet alloc] init];
    return self;
}

- (void)addFavStatusFor:(NSString*)name bundleFN:(FileNode*)bundleFN customFN:(FileNode*)customFN {
    if (name==nil) return;
    if ([name length]<4) return;
    if ([name characterAtIndex:1]=='B') {
        //Look for the entry in bundleFN
        NSString *filename=[name substringFromIndex:3];
        for (FileNode *item in bundleFN.children) {
            if ([item.name isEqualToString:filename]) {
                //found it
                item.isFavorite=true;
                break;
            }
        }
    }
    if ([name characterAtIndex:1]=='C') {
        //Look for the entry in customFN
        NSString *filename=[name substringFromIndex:3];
        for (FileNode *item in customFN.children) {
            if ([item.name isEqualToString:filename]) {
                //found it
                item.isFavorite=true;
                break;
            }
        }
    }
}
- (void)remFavStatusFor:(NSString*)name bundleFN:(FileNode*)bundleFN customFN:(FileNode*)customFN {
    if (name==nil) return;
    if ([name length]<4) return;
    if ([name characterAtIndex:1]=='B') {
        //Look for the entry in bundleFN
        NSString *filename=[name substringFromIndex:3];
        for (FileNode *item in bundleFN.children) {
            if ([item.name isEqualToString:filename]) {
                //found it
                item.isFavorite=false;
                break;
            }
        }
    }
    if ([name characterAtIndex:1]=='C') {
        //Look for the entry in customFN
        NSString *filename=[name substringFromIndex:3];
        for (FileNode *item in customFN.children) {
            if ([item.name isEqualToString:filename]) {
                //found it
                item.isFavorite=false;
                break;
            }
        }
    }
}


- (void)addFavoritePreset:(NSString *)path {
    if (path==nil) return;
    if ([path length]<4) return;
    if ([path characterAtIndex:1]=='B') {
        [_bundlePresets addObject:path];
        [_bundlePresets sortUsingComparator:^NSComparisonResult(NSString *str1, NSString *str2) {
            NSString *strtmp1;
            NSString *strtmp2;
            strtmp1=[str1 stringByReplacingOccurrencesOfString:@"/" withString:@"\0"];
            strtmp2=[str2 stringByReplacingOccurrencesOfString:@"/" withString:@"\0"];
            return [strtmp1 caseInsensitiveCompare:strtmp2];
        }];
    }
    if ([path characterAtIndex:1]=='C') {
        [_customPresets addObject:path];
        [_customPresets sortUsingComparator:^NSComparisonResult(NSString *str1, NSString *str2) {
            NSString *strtmp1;
            NSString *strtmp2;
            strtmp1=[str1 stringByReplacingOccurrencesOfString:@"/" withString:@"\0"];
            strtmp2=[str2 stringByReplacingOccurrencesOfString:@"/" withString:@"\0"];
            return [strtmp1 caseInsensitiveCompare:strtmp2];
        }];
    }
}

- (void)remFavoritePreset:(NSString *)path {
    if (path==nil) return;
    if ([path length]<4) return;
    if ([path characterAtIndex:1]=='B') {
        [_bundlePresets removeObject:path];
    }
    if ([path characterAtIndex:1]=='C') {
        [_customPresets removeObject:path];
    }
}

- (bool)isFavoritePreset:(NSString *)path {
    if (path==nil) return false;
    if ([path length]<4) return false;
    if ([path characterAtIndex:1]=='B') {
        return [_bundlePresets containsObject:path];
    }
    if ([path characterAtIndex:1]=='C') {
        return [_customPresets containsObject:path];
    }
    return false;
}

- (int)favoritesTotalSize {
    return (int)[_bundlePresets count]+[_customPresets count];
}
- (int)favoritesBundleSize {
    return (int)[_bundlePresets count];
}
- (int)favoritesCustomSize {
    return (int)[_customPresets count];
}

- (void)updateFileNodeStatus:(FileNode*)fnode type:(int)type {
    //[self listFavorites];
    [fnode clearFavorites];
    if ( (type==MDZ_PLAYLIST_FNODE_Bundle) && ([_bundlePresets count]) ) [fnode setFavoritesFromFL:[_bundlePresets array]];
    if ( (type==MDZ_PLAYLIST_FNODE_Custom) && ([_customPresets count]) ) [fnode setFavoritesFromFL:[_customPresets array]];
}

- (void)listFavorites {
    MDZILog("====================");
    MDZILog("=== Bundle favorites");
    MDZILog("====================");
    for (NSString *str in _bundlePresets) {
        MDZILog("%@",str);
    }
    MDZILog("====================");
    MDZILog("=== Custom favorites");
    MDZILog("====================");
    for (NSString *str in _customPresets) {
        MDZILog("%@",str);
    }
}

- (int)loadFavorites {
    int missing_counter=0;
    NSString *pmBundleDir = [NSString stringWithFormat:@"%@/projectm/assets/presets",[[NSBundle mainBundle] resourcePath]];
    NSString *pmCustomDir = [NSString stringWithFormat:@"%@/Documents%s/presets",[ModizFileHelper getAppHomeDirectory],PM_ROOT_FOLDER_CUSTOM];
    
    gzFile f;
    f=gzopen([[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:@"Documents/modizerFavorites.pmfav"] UTF8String],"rb");
    if (f) {
        int readBytes;
        //Read header
        MDZFavorites_Header_t header;
        readBytes=gzread(f,&header,sizeof(MDZFavorites_Header_t));
        
        if (readBytes<sizeof(MDZFavorites_Header_t)) {
            MDZELog("PM favorites/Loading: cannot read  file (modizerPresetsPL.pmpl)");
            gzclose(f);
            return -1;
        }
        if (header.version!=MDZ_PMPLAYLIST_VERSION) {
            MDZELog("PM favorites/Loading: wrong version");
            gzclose(f);
            return -2;
        }
        
        [_bundlePresets removeAllObjects];
        [_customPresets removeAllObjects];
        
        char str[1024];
        for (int i=0;i<header.itemsNb;i++) {
            uint8_t presetType;
            readBytes=gzread(f,&presetType,sizeof(char));
            if (readBytes!=sizeof(char)) {
                MDZELog("PM favorites/Loading: wrong data (presetType) for entry %d, aborting",i);
                gzclose(f);
                return -3;
            }
            
            int strLen;
            readBytes=gzread(f,&strLen,sizeof(int));
            if (strLen>1023) {
                MDZELog("PM favorites/Loading: too long path string (>1023) for entry %d, limiting",i);
            }
            readBytes=gzread(f,&str,strLen);
            if (readBytes!=strLen) {
                MDZELog("PM favorites/Loading: cannot read string data for entry %d, aborting",i);
                gzclose(f);
                return -3;
            }
            str[strLen]=0;
            NSString *rootPath;
            if (presetType==MDZ_PLAYLIST_FNODE_Bundle) rootPath=pmBundleDir;
            else if (presetType==MDZ_PLAYLIST_FNODE_Custom) rootPath=pmCustomDir;
            else rootPath=pmCustomDir; //default to custom
            //skip first 3 char, i.e. (B) or (C)
            FileNode *node=[[FileNode alloc] initWithPath:[NSString stringWithUTF8String:str+3] root:rootPath type:presetType];
            if (node.isMissing) {
                //File doesn't exist anymore, remove from favorites
                missing_counter++;
                MDZILog("not found fav: %s",str);
            } else {
                MDZILog("found fav: %s",str);
                //File exits, put in the right list
                if (presetType==MDZ_PLAYLIST_FNODE_Bundle) [_bundlePresets addObject:[NSString stringWithUTF8String:str]];
                if (presetType==MDZ_PLAYLIST_FNODE_Custom) [_customPresets addObject:[NSString stringWithUTF8String:str]];
            }
        }
        gzclose(f);
    }
    if (missing_counter) {
        MDZILog("PM favorites/Loading: %d entries are missing in filesystem",missing_counter);
    }
    
    //Sort arrays, in case something went wrong earlier
    [_bundlePresets sortUsingComparator:^NSComparisonResult(NSString *str1, NSString *str2) {
        return [str1 caseInsensitiveCompare:str2];
    }];
    [_customPresets sortUsingComparator:^NSComparisonResult(NSString *str1, NSString *str2) {
        return [str1 caseInsensitiveCompare:str2];
    }];
    
    return missing_counter;
}
- (int)saveFavorites {
    gzFile f;
    f=gzopen([[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:@"Documents/modizerFavorites.pmfav"] UTF8String],"wb");
    if (f) {
        //Write header
        MDZFavorites_Header_t header;
        header.version=MDZ_PMPLAYLIST_VERSION;
        header.itemsNb=[self favoritesTotalSize];
        snprintf(header.name,64,"%s","Favorites");
        gzwrite(f,&header,sizeof(MDZFavorites_Header_t));
        
        //Write path for each entries
        for (NSString *path in _bundlePresets) {
            uint8_t presetType=MDZ_PLAYLIST_FNODE_Bundle;
            const char *str=[path UTF8String];
            int strLen=(int)strlen(str);
            if (strLen>1023) {
                strLen=1023;
                MDZELog("PM favorites/Saving: too long file path for saving (> 1023)");
            }
            gzwrite(f,&presetType,sizeof(uint8_t));
            gzwrite(f,&strLen,sizeof(int));
            gzwrite(f,str,strLen);
        }
        for (NSString *path in _customPresets) {
            uint8_t presetType=MDZ_PLAYLIST_FNODE_Custom;
            const char *str=[path UTF8String];
            int strLen=(int)strlen(str);
            if (strLen>1023) {
                strLen=1023;
                MDZELog("PM favorites/Saving: too long file path for saving (> 1023)");
            }
            gzwrite(f,&presetType,sizeof(uint8_t));
            gzwrite(f,&strLen,sizeof(int));
            gzwrite(f,str,strLen);
            
        }
        gzclose(f);
    } else {
        MDZELog("PM favorites/Saving: cannot open saving file for writing");
        return -1;
    }
    return 0;
}



@end
