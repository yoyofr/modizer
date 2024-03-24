//
//  ModizFileHelper.h
//  modizer
//
//  Created by Yohann Magnien David on 07/03/2024.
//

#import <Foundation/Foundation.h>
#import "ModizerConstants.h"
#include "archive.h"
#include "archive_entry.h"

@interface ModizFileHelper : NSObject {

}

+(bool) isGMEFileWithSubsongs:(NSString*)cpath;
+(bool) isSidFileWithSubsongs:(NSString*)cpath;
+(bool) isABrowsableArchive:(NSString*)cpath;
+(NSMutableArray*)buildListSupportFileType:(t_filetypeList)ftype;

+(int) isAcceptedFile:(NSString*)_filePath no_aux_file:(int)no_aux_file;
+(int) isPlayableFile:(NSString*)file;
+(int) isSingleFileType:(NSString*)_filePath;
+(int) scanarchive:(const char *)path filesList_ptr:(char***)filesList_ptr filesCount_ptr:(int*)filesCount_ptr ;
+(void) extractToPath:(const char *)archivePath path:(const char *)extractPath caller:(NSObject*)caller progress:(NSProgress*)extractProgress;
+(BOOL) addSkipBackupAttributeToItemAtPath:(NSString*)path;
+(void) updateFilesDoNotBackupAttributes;

+(NSString *)getFullCleanFilePath:(NSString*)filePath;
+(NSString *)getFilePathNoSubSong:(NSString*)filePath;
+(NSString *)getFilePathFromDocuments:(NSString*)filePath;

+(NSString*) getCorrectFileName:(const char*)archiveFilename archive:(struct archive *)a entry:(struct archive_entry *)entry;

@end

