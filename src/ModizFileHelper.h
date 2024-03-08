//
//  ModizFileHelper.h
//  modizer
//
//  Created by Yohann Magnien David on 07/03/2024.
//

#import <Foundation/Foundation.h>
#import "ModizerConstants.h"

@interface ModizFileHelper : NSObject {

}

+(bool) isGMEFileWithSubsongs:(NSString*)cpath;
+(bool) isSidFileWithSubsongs:(NSString*)cpath;
+(bool) isABrowsableArchive:(NSString*)cpath;
+(NSMutableArray*)buildListSupportFileType:(t_filetypeList)ftype;

+(int) isAcceptedFile:(NSString*)_filePath no_aux_file:(int)no_aux_file;
+(int) isAllowedFile:(NSString*)file;

+(NSString *)getFullCleanFilePath:(NSString*)filePath;
+(NSString *)getFilePathNoSubSong:(NSString*)filePath;

@end

