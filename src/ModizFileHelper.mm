//
//  ModizFileHelper.mm
//  modizer
//
//  Created by Yohann Magnien David on 07/03/2024.
//

extern void *ExtractProgressObserverContext;
extern NSURL *icloudURL;
extern bool icloud_available;


#import "ModizFileHelper.h"

#include <sys/sysctl.h>
#include <sys/xattr.h>

//GME
#include "gme.h"
//SID2
#include "sidplayfp/SidTune.h"
#include "sidplayfp/SidTuneInfo.h"

//UnrarKIT
#import "UnrarKit.h"


@implementation ModizFileHelper

+(bool) isGMEFileWithSubsongs:(NSString*)cpath {
    bool ret=false;
    Music_Emu* gme_emu;
    gme_emu=NULL;
    
    NSArray *filetypes_ext=[SUPPORTED_FILETYPE_GME_MULTISONGS componentsSeparatedByString:@","];
    if ([filetypes_ext indexOfObject:[[cpath pathExtension] uppercaseString]]==NSNotFound) return ret;
    
    return true;
    
    gme_err_t gme_err=gme_open_file( [cpath UTF8String], &gme_emu, gme_info_only );
    if (gme_err) {
        NSLog(@"gme_open_file error: %s",gme_err);
    } else {
        gme_info_t *gme_info;
        
        //is a m3u available ?
        NSString *tmpStr=[NSString stringWithFormat:@"%@.m3u",[cpath stringByDeletingPathExtension]];
        gme_err=gme_load_m3u(gme_emu,[tmpStr UTF8String] );
        if (gme_err) {
            NSString *tmpStr=[NSString stringWithFormat:@"%@.M3U",[cpath stringByDeletingPathExtension]];
            gme_err=gme_load_m3u(gme_emu,[tmpStr UTF8String] );
        }
        int total_trackNb=gme_track_count( gme_emu );
        if (total_trackNb>1) ret=true;
        gme_delete(gme_emu);
    }
    
    return ret;
}

+(bool) isSidFileWithSubsongs:(NSString*)cpath {
    bool ret=false;
    
    NSArray *filetypes_ext=[SUPPORTED_FILETYPE_SID componentsSeparatedByString:@","];
    if ([filetypes_ext indexOfObject:[[cpath pathExtension] uppercaseString]]==NSNotFound) return ret;
    
    //return true;
    
    SidTune *mSidTune=new SidTune([cpath UTF8String],0,true);
    
    if (mSidTune!=NULL) {
        if (mSidTune->getStatus()) {
            const SidTuneInfo *sidtune_info;
            sidtune_info=mSidTune->getInfo();
            if (sidtune_info->songs()>1) ret=true;
        }
        delete mSidTune;
        mSidTune=NULL;
    }
    
    return ret;
}

+(bool) isABrowsableArchive:(NSString*)cpath {
    bool ret=false;
    
    NSArray *filetypes_ext=[SUPPORTED_FILETYPE_ARCHIVE componentsSeparatedByString:@","];
    if ([filetypes_ext indexOfObject:[[cpath pathExtension] uppercaseString]]==NSNotFound) return ret;
    
    //Specific case for RAR files as libarchive is buggy for them and UnrarKIT is used instead
    if ( ([[[cpath pathExtension] uppercaseString] isEqualToString:@"RAR"]) ||
        ([[[cpath pathExtension] uppercaseString] isEqualToString:@"RSN"]) ) {
        NSError *error = nil;
        URKArchive *archive = [[URKArchive alloc] initWithPath:cpath error:&error];
        if (!archive) {
            NSLog(@"Error: %ld %@",error.code,error.localizedDescription);
            return false;
        }
        NSArray *filesInArchive = [archive listFilenames:&error];
        if (!filesInArchive) {
            NSLog(@"Error: %ld %@",error.code,error.localizedDescription);
            return false;
        }
        return true;
    }
    
    struct archive *a = archive_read_new();
    struct archive_entry *entry;
    int r;
    
    archive_read_support_filter_all(a);
    archive_read_support_format_raw(a);
    archive_read_support_format_all(a);
    r = archive_read_open_filename(a, [cpath UTF8String], 16384);
        
    if (r==ARCHIVE_OK) {
        ret=true;
    } else {
        NSLog( @"Skipping unsupported archive: %s\n", [cpath UTF8String] );
    }
    r = archive_read_free(a);
        
    return ret;
}

+(NSMutableArray*)buildListSupportFileType:(t_filetypeList)ftype {
    NSMutableArray *filetype_ext;
    
    
    switch (ftype) {
        case FTYPE_PLAYABLEFILE: {
            NSArray *filetype_extMDX=[SUPPORTED_FILETYPE_MDX componentsSeparatedByString:@","];
            NSArray *filetype_extPMD=[SUPPORTED_FILETYPE_PMD componentsSeparatedByString:@","];
            NSArray *filetype_extSID=[SUPPORTED_FILETYPE_SID componentsSeparatedByString:@","];
            NSArray *filetype_extSTSOUND=[SUPPORTED_FILETYPE_STSOUND componentsSeparatedByString:@","];
            NSArray *filetype_extATARISOUND=[SUPPORTED_FILETYPE_ATARISOUND componentsSeparatedByString:@","];
            NSArray *filetype_extSC68=[SUPPORTED_FILETYPE_SC68 componentsSeparatedByString:@","];
            NSArray *filetype_extPT3=[SUPPORTED_FILETYPE_PT3 componentsSeparatedByString:@","];
            NSArray *filetype_extGBS=[SUPPORTED_FILETYPE_GBS componentsSeparatedByString:@","];
            NSArray *filetype_extNSFPLAY=[SUPPORTED_FILETYPE_NSFPLAY componentsSeparatedByString:@","];
            NSArray *filetype_extPIXEL=[SUPPORTED_FILETYPE_PIXEL componentsSeparatedByString:@","];
            NSArray *filetype_extUADE=[SUPPORTED_FILETYPE_UADE componentsSeparatedByString:@","];
            NSArray *filetype_extMODPLUG=[SUPPORTED_FILETYPE_OMPT componentsSeparatedByString:@","];
            NSArray *filetype_extXMP=[SUPPORTED_FILETYPE_XMP componentsSeparatedByString:@","];
            NSArray *filetype_extGME=[SUPPORTED_FILETYPE_GME componentsSeparatedByString:@","];
            NSArray *filetype_extADPLUG=[SUPPORTED_FILETYPE_ADPLUG componentsSeparatedByString:@","];
            NSArray *filetype_ext2SF=[SUPPORTED_FILETYPE_2SF componentsSeparatedByString:@","];
            NSArray *filetype_extV2M=[SUPPORTED_FILETYPE_V2M componentsSeparatedByString:@","];
            NSArray *filetype_extVGMSTREAM=[SUPPORTED_FILETYPE_VGMSTREAM componentsSeparatedByString:@","];
            NSArray *filetype_extHC=[SUPPORTED_FILETYPE_HC componentsSeparatedByString:@","];
            NSArray *filetype_extEUP=[SUPPORTED_FILETYPE_EUP componentsSeparatedByString:@","];
            NSArray *filetype_extHVL=[SUPPORTED_FILETYPE_HVL componentsSeparatedByString:@","];
            NSArray *filetype_extS98=[SUPPORTED_FILETYPE_S98 componentsSeparatedByString:@","];
            NSArray *filetype_extKSS=[SUPPORTED_FILETYPE_KSS componentsSeparatedByString:@","];
            NSArray *filetype_extGSF=[SUPPORTED_FILETYPE_GSF componentsSeparatedByString:@","];
            NSArray *filetype_extASAP=[SUPPORTED_FILETYPE_ASAP componentsSeparatedByString:@","];
            NSArray *filetype_extWMIDI=[SUPPORTED_FILETYPE_WMIDI componentsSeparatedByString:@","];
            NSArray *filetype_extVGM=[SUPPORTED_FILETYPE_VGM componentsSeparatedByString:@","];
            
            filetype_ext=[NSMutableArray arrayWithCapacity:[filetype_extMDX count]+
                          [filetype_extPMD count]+
                          [filetype_extSID count]+
                          [filetype_extSTSOUND count]+
                          [filetype_extATARISOUND count]+
                          [filetype_extSC68 count]+
                          [filetype_extPT3 count]+
                          [filetype_extGBS count]+
                          [filetype_extNSFPLAY count]+
                          [filetype_extPIXEL count]+
                          [filetype_extUADE count]+
                          [filetype_extMODPLUG count]+
                          [filetype_extXMP count]+
                          [filetype_extGME count]+
                          [filetype_extADPLUG count]+
                          [filetype_ext2SF count]+
                          [filetype_extV2M count]+
                          [filetype_extVGMSTREAM count]+
                          [filetype_extHC count]+
                          [filetype_extEUP count]+
                          [filetype_extHVL count]+
                          [filetype_extS98 count]+
                          [filetype_extKSS count]+
                          [filetype_extGSF count]+
                          [filetype_extASAP count]+
                          [filetype_extWMIDI count]+
                          [filetype_extVGM count]];
            [filetype_ext addObjectsFromArray:filetype_extMDX];
            [filetype_ext addObjectsFromArray:filetype_extPMD];
            [filetype_ext addObjectsFromArray:filetype_extSID];
            [filetype_ext addObjectsFromArray:filetype_extSTSOUND];
            [filetype_ext addObjectsFromArray:filetype_extATARISOUND];
            [filetype_ext addObjectsFromArray:filetype_extSC68];
            [filetype_ext addObjectsFromArray:filetype_extPT3];
            [filetype_ext addObjectsFromArray:filetype_extGBS];
            [filetype_ext addObjectsFromArray:filetype_extNSFPLAY];
            [filetype_ext addObjectsFromArray:filetype_extPIXEL];
            [filetype_ext addObjectsFromArray:filetype_extUADE];
            [filetype_ext addObjectsFromArray:filetype_extMODPLUG];
            [filetype_ext addObjectsFromArray:filetype_extXMP];
            [filetype_ext addObjectsFromArray:filetype_extGME];
            [filetype_ext addObjectsFromArray:filetype_extADPLUG];
            [filetype_ext addObjectsFromArray:filetype_ext2SF];
            [filetype_ext addObjectsFromArray:filetype_extV2M];
            [filetype_ext addObjectsFromArray:filetype_extVGMSTREAM];
            [filetype_ext addObjectsFromArray:filetype_extHC];
            [filetype_ext addObjectsFromArray:filetype_extEUP];
            [filetype_ext addObjectsFromArray:filetype_extHVL];
            [filetype_ext addObjectsFromArray:filetype_extS98];
            [filetype_ext addObjectsFromArray:filetype_extKSS];
            [filetype_ext addObjectsFromArray:filetype_extGSF];
            [filetype_ext addObjectsFromArray:filetype_extASAP];
            [filetype_ext addObjectsFromArray:filetype_extWMIDI];
            [filetype_ext addObjectsFromArray:filetype_extVGM];
            break;
        }
        case FTYPE_PLAYABLEFILE_AMIGA: { //specific case for file named with extension first, i.e. mod.unreal rather than unreal.mod
            NSArray *filetype_extUADE=[SUPPORTED_FILETYPE_UADE componentsSeparatedByString:@","];            
            filetype_ext=[NSMutableArray arrayWithCapacity:[filetype_extUADE count]];
            [filetype_ext addObjectsFromArray:filetype_extUADE];
            break;
        }
        case FTYPE_PLAYABLEFILE_AND_DATAFILE: {
            NSArray *filetype_extMDX=[SUPPORTED_FILETYPE_MDX_EXT componentsSeparatedByString:@","];
            NSArray *filetype_extPMD=[SUPPORTED_FILETYPE_PMD componentsSeparatedByString:@","];
            NSArray *filetype_extSID=[SUPPORTED_FILETYPE_SID componentsSeparatedByString:@","];
            NSArray *filetype_extSTSOUND=[SUPPORTED_FILETYPE_STSOUND componentsSeparatedByString:@","];
            NSArray *filetype_extATARISOUND=[SUPPORTED_FILETYPE_ATARISOUND componentsSeparatedByString:@","];
            NSArray *filetype_extSC68=[SUPPORTED_FILETYPE_SC68 componentsSeparatedByString:@","];
            NSArray *filetype_extPT3=[SUPPORTED_FILETYPE_PT3 componentsSeparatedByString:@","];
            NSArray *filetype_extGBS=[SUPPORTED_FILETYPE_GBS componentsSeparatedByString:@","];
            NSArray *filetype_extNSFPLAY=[SUPPORTED_FILETYPE_NSFPLAY_EXT componentsSeparatedByString:@","];
            NSArray *filetype_extPIXEL=[SUPPORTED_FILETYPE_PIXEL componentsSeparatedByString:@","];
            NSArray *filetype_extUADE=[SUPPORTED_FILETYPE_UADE_EXT componentsSeparatedByString:@","];
            NSArray *filetype_extMODPLUG=[SUPPORTED_FILETYPE_OMPT componentsSeparatedByString:@","];
            NSArray *filetype_extXMP=[SUPPORTED_FILETYPE_XMP componentsSeparatedByString:@","];
            NSArray *filetype_extGME=[SUPPORTED_FILETYPE_GME_EXT componentsSeparatedByString:@","];
            NSArray *filetype_extADPLUG=[SUPPORTED_FILETYPE_ADPLUG componentsSeparatedByString:@","];
            NSArray *filetype_ext2SF=[SUPPORTED_FILETYPE_2SF_EXT componentsSeparatedByString:@","];
            NSArray *filetype_extV2M=[SUPPORTED_FILETYPE_V2M componentsSeparatedByString:@","];
            NSArray *filetype_extVGMSTREAM=[SUPPORTED_FILETYPE_VGMSTREAM componentsSeparatedByString:@","];
            NSArray *filetype_extHC=[SUPPORTED_FILETYPE_HC_EXT componentsSeparatedByString:@","];
            NSArray *filetype_extEUP=[SUPPORTED_FILETYPE_EUP_EXT componentsSeparatedByString:@","];
            NSArray *filetype_extHVL=[SUPPORTED_FILETYPE_HVL componentsSeparatedByString:@","];
            NSArray *filetype_extS98=[SUPPORTED_FILETYPE_S98 componentsSeparatedByString:@","];
            NSArray *filetype_extKSS=[SUPPORTED_FILETYPE_KSS componentsSeparatedByString:@","];
            NSArray *filetype_extGSF=[SUPPORTED_FILETYPE_GSF_EXT componentsSeparatedByString:@","];
            NSArray *filetype_extASAP=[SUPPORTED_FILETYPE_ASAP componentsSeparatedByString:@","];
            NSArray *filetype_extWMIDI=[SUPPORTED_FILETYPE_WMIDI_EXT componentsSeparatedByString:@","];
            NSArray *filetype_extVGM=[SUPPORTED_FILETYPE_VGM componentsSeparatedByString:@","];
            
            filetype_ext=[NSMutableArray arrayWithCapacity:[filetype_extMDX count]+
                          [filetype_extPMD count]+
                          [filetype_extSID count]+
                          [filetype_extSTSOUND count]+
                          [filetype_extATARISOUND count]+
                          [filetype_extSC68 count]+
                          [filetype_extPT3 count]+
                          [filetype_extGBS count]+
                          [filetype_extNSFPLAY count]+
                          [filetype_extPIXEL count]+
                          [filetype_extUADE count]+
                          [filetype_extMODPLUG count]+
                          [filetype_extXMP count]+
                          [filetype_extGME count]+
                          [filetype_extADPLUG count]+
                          [filetype_ext2SF count]+
                          [filetype_extV2M count]+
                          [filetype_extVGMSTREAM count]+
                          [filetype_extHC count]+
                          [filetype_extEUP count]+
                          [filetype_extHVL count]+
                          [filetype_extS98 count]+
                          [filetype_extKSS count]+
                          [filetype_extGSF count]+
                          [filetype_extASAP count]+
                          [filetype_extWMIDI count]+
                          [filetype_extVGM count]];
            [filetype_ext addObjectsFromArray:filetype_extMDX];
            [filetype_ext addObjectsFromArray:filetype_extPMD];
            [filetype_ext addObjectsFromArray:filetype_extSID];
            [filetype_ext addObjectsFromArray:filetype_extSTSOUND];
            [filetype_ext addObjectsFromArray:filetype_extATARISOUND];
            [filetype_ext addObjectsFromArray:filetype_extSC68];
            [filetype_ext addObjectsFromArray:filetype_extPT3];
            [filetype_ext addObjectsFromArray:filetype_extGBS];
            [filetype_ext addObjectsFromArray:filetype_extNSFPLAY];
            [filetype_ext addObjectsFromArray:filetype_extPIXEL];
            [filetype_ext addObjectsFromArray:filetype_extUADE];
            [filetype_ext addObjectsFromArray:filetype_extMODPLUG];
            [filetype_ext addObjectsFromArray:filetype_extXMP];
            [filetype_ext addObjectsFromArray:filetype_extGME];
            [filetype_ext addObjectsFromArray:filetype_extADPLUG];
            [filetype_ext addObjectsFromArray:filetype_ext2SF];
            [filetype_ext addObjectsFromArray:filetype_extV2M];
            [filetype_ext addObjectsFromArray:filetype_extVGMSTREAM];
            [filetype_ext addObjectsFromArray:filetype_extHC];
            [filetype_ext addObjectsFromArray:filetype_extEUP];
            [filetype_ext addObjectsFromArray:filetype_extHVL];
            [filetype_ext addObjectsFromArray:filetype_extS98];
            [filetype_ext addObjectsFromArray:filetype_extKSS];
            [filetype_ext addObjectsFromArray:filetype_extGSF];
            [filetype_ext addObjectsFromArray:filetype_extASAP];
            [filetype_ext addObjectsFromArray:filetype_extWMIDI];
            [filetype_ext addObjectsFromArray:filetype_extVGM];
            break;
        }
        case FTYPE_PLAYABLEFILE_SUBSONGS: {
            NSArray *filetype_extGMEMULTI=[SUPPORTED_FILETYPE_GME_MULTISONGS componentsSeparatedByString:@","];
            NSArray *filetype_extSID=[SUPPORTED_FILETYPE_SID componentsSeparatedByString:@","];
            filetype_ext=[NSMutableArray arrayWithCapacity:[filetype_extGMEMULTI count]+[filetype_extSID count]];
            [filetype_ext addObjectsFromArray:filetype_extGMEMULTI];
            [filetype_ext addObjectsFromArray:filetype_extSID];
            break;
        }
        case FTYPE_ARCHIVE: {
            NSArray *filetype_extArchive=[SUPPORTED_FILETYPE_ARCHIVE componentsSeparatedByString:@","];
            filetype_ext=[NSMutableArray arrayWithCapacity:[filetype_extArchive count]];
            [filetype_ext addObjectsFromArray:filetype_extArchive];
            break;
        }
        case FTYPE_BROWSABLEARCHIVE: {
            NSArray *filetype_extArchive=[SUPPORTED_FILETYPE_ARCHIVE componentsSeparatedByString:@","];
            filetype_ext=[NSMutableArray arrayWithCapacity:[filetype_extArchive count]];
            [filetype_ext addObjectsFromArray:filetype_extArchive];
            break;
        }
        case FTYPE_SINGLE_FILETYPE:{
            filetype_ext=[NSMutableArray arrayWithCapacity:64];
            [filetype_ext addObjectsFromArray:[SUPPORTED_FILETYPE_PMD componentsSeparatedByString:@","]];
            [filetype_ext addObjectsFromArray:[SUPPORTED_FILETYPE_SID componentsSeparatedByString:@","]];
            [filetype_ext addObjectsFromArray:[SUPPORTED_FILETYPE_STSOUND componentsSeparatedByString:@","]];
            [filetype_ext addObjectsFromArray:[SUPPORTED_FILETYPE_ATARISOUND componentsSeparatedByString:@","]];
            [filetype_ext addObjectsFromArray:[SUPPORTED_FILETYPE_SC68 componentsSeparatedByString:@","]];
            [filetype_ext addObjectsFromArray:[SUPPORTED_FILETYPE_PT3 componentsSeparatedByString:@","]];
            [filetype_ext addObjectsFromArray:[SUPPORTED_FILETYPE_GBS componentsSeparatedByString:@","]];
            [filetype_ext addObjectsFromArray:[SUPPORTED_FILETYPE_NSFPLAY componentsSeparatedByString:@","]];
            [filetype_ext addObjectsFromArray:[SUPPORTED_FILETYPE_ASAP componentsSeparatedByString:@","]];
            [filetype_ext addObjectsFromArray:[SUPPORTED_FILETYPE_PIXEL componentsSeparatedByString:@","]];
            [filetype_ext addObjectsFromArray:[SUPPORTED_FILETYPE_VGM componentsSeparatedByString:@","]];
            [filetype_ext addObjectsFromArray:[SUPPORTED_FILETYPE_HVL componentsSeparatedByString:@","]];
            [filetype_ext addObjectsFromArray:[SUPPORTED_FILETYPE_OMPT componentsSeparatedByString:@","]];
            [filetype_ext addObjectsFromArray:[SUPPORTED_FILETYPE_XMP componentsSeparatedByString:@","]];
            [filetype_ext addObjectsFromArray:[SUPPORTED_FILETYPE_KSS componentsSeparatedByString:@","]];
            [filetype_ext addObjectsFromArray:[SUPPORTED_FILETYPE_ADPLUG componentsSeparatedByString:@","]];
            [filetype_ext addObjectsFromArray:[SUPPORTED_FILETYPE_V2M componentsSeparatedByString:@","]];
            break;
        
        }
    }
    return filetype_ext;
}

+(int) isSingleFileType:(NSString*)_filePath {
    NSString *extension;
    NSString *file_no_ext;
    
    NSMutableArray *temparray_filepath=[NSMutableArray arrayWithArray:[[_filePath lastPathComponent] componentsSeparatedByString:@"."]];
    extension = (NSString *)[temparray_filepath lastObject];
    file_no_ext=[temparray_filepath firstObject];
    
    
    NSMutableArray *supported_ext=[ModizFileHelper buildListSupportFileType:FTYPE_SINGLE_FILETYPE];
    
    for (int i=0;i<[supported_ext count];i++) {
        if ([extension caseInsensitiveCompare:[supported_ext objectAtIndex:i]]==NSOrderedSame) {
            return 1;
        }
    }
        
    return 0;
}

+(int) isPlayableFile:(NSString*)_filePath {
    NSString *extension;
    NSString *file_no_ext;
    
    NSMutableArray *temparray_filepath=[NSMutableArray arrayWithArray:[[_filePath lastPathComponent] componentsSeparatedByString:@"."]];
    extension = (NSString *)[temparray_filepath lastObject];
    file_no_ext=[temparray_filepath firstObject];
    
    
    NSMutableArray *supported_ext=[ModizFileHelper buildListSupportFileType:FTYPE_PLAYABLEFILE];
    //check playable file types
    for (int i=0;i<[supported_ext count];i++) {
        if ([extension caseInsensitiveCompare:[supported_ext objectAtIndex:i]]==NSOrderedSame) {
            return 1;
        }
    }
    //check archive
    supported_ext=[ModizFileHelper buildListSupportFileType:FTYPE_ARCHIVE];
    for (int i=0;i<[supported_ext count];i++) {
        if ([extension caseInsensitiveCompare:[supported_ext objectAtIndex:i]]==NSOrderedSame) {
            return 1;
        }
    }
    //check amiga format, i.e. extension first
    supported_ext=[ModizFileHelper buildListSupportFileType:FTYPE_PLAYABLEFILE_AMIGA];
    for (int i=0;i<[supported_ext count];i++) {
        if ([file_no_ext caseInsensitiveCompare:[supported_ext objectAtIndex:i]]==NSOrderedSame) {
            return 1;
        }
    }
        
    return 0;
}


+(int) isAcceptedFile:(NSString*)_filePath no_aux_file:(int)no_aux_file {
    NSArray *filetype_extMDX=(no_aux_file?[SUPPORTED_FILETYPE_MDX componentsSeparatedByString:@","]:[SUPPORTED_FILETYPE_MDX_EXT componentsSeparatedByString:@","]);
    NSArray *filetype_extPMD=[SUPPORTED_FILETYPE_PMD componentsSeparatedByString:@"."];
    NSArray *filetype_extSID=[SUPPORTED_FILETYPE_SID componentsSeparatedByString:@","];
    NSArray *filetype_extSTSOUND=[SUPPORTED_FILETYPE_STSOUND componentsSeparatedByString:@","];
    NSArray *filetype_extATARISOUND=[SUPPORTED_FILETYPE_ATARISOUND componentsSeparatedByString:@","];
    NSArray *filetype_extSC68=[SUPPORTED_FILETYPE_SC68 componentsSeparatedByString:@","];
    NSArray *filetype_extPT3=[SUPPORTED_FILETYPE_PT3 componentsSeparatedByString:@","];
    NSArray *filetype_extGBS=[SUPPORTED_FILETYPE_GBS componentsSeparatedByString:@","];
    NSArray *filetype_extNSFPLAY=(no_aux_file?[SUPPORTED_FILETYPE_NSFPLAY componentsSeparatedByString:@","]:[SUPPORTED_FILETYPE_NSFPLAY_EXT componentsSeparatedByString:@","]);
    NSArray *filetype_extPIXEL=[SUPPORTED_FILETYPE_PIXEL componentsSeparatedByString:@","];
    NSArray *filetype_extUADE=(no_aux_file?[SUPPORTED_FILETYPE_UADE componentsSeparatedByString:@","]:[SUPPORTED_FILETYPE_UADE_EXT componentsSeparatedByString:@","]);
    NSArray *filetype_extMODPLUG=[SUPPORTED_FILETYPE_OMPT componentsSeparatedByString:@","];
    NSArray *filetype_extXMP=[SUPPORTED_FILETYPE_XMP componentsSeparatedByString:@","];
    NSArray *filetype_extGME=(no_aux_file?[SUPPORTED_FILETYPE_GME componentsSeparatedByString:@","]:[SUPPORTED_FILETYPE_GME_EXT componentsSeparatedByString:@","]);
    NSArray *filetype_extADPLUG=[SUPPORTED_FILETYPE_ADPLUG componentsSeparatedByString:@","];
    NSArray *filetype_ext2SF=(no_aux_file?[SUPPORTED_FILETYPE_2SF componentsSeparatedByString:@","]:[SUPPORTED_FILETYPE_2SF_EXT componentsSeparatedByString:@","]);
    NSArray *filetype_extV2M=[SUPPORTED_FILETYPE_V2M componentsSeparatedByString:@","];
    NSArray *filetype_extHC=(no_aux_file?[SUPPORTED_FILETYPE_HC componentsSeparatedByString:@","]:[SUPPORTED_FILETYPE_HC_EXT componentsSeparatedByString:@","]);
    NSArray *filetype_extEUP=(no_aux_file?[SUPPORTED_FILETYPE_EUP componentsSeparatedByString:@","]:[SUPPORTED_FILETYPE_EUP_EXT componentsSeparatedByString:@","]);
    NSArray *filetype_extHVL=[SUPPORTED_FILETYPE_HVL componentsSeparatedByString:@","];
    NSArray *filetype_extS98=[SUPPORTED_FILETYPE_S98 componentsSeparatedByString:@","];
    NSArray *filetype_extKSS=[SUPPORTED_FILETYPE_KSS componentsSeparatedByString:@","];
    NSArray *filetype_extGSF=(no_aux_file?[SUPPORTED_FILETYPE_GSF componentsSeparatedByString:@","]:[SUPPORTED_FILETYPE_GSF_EXT componentsSeparatedByString:@","]);
    NSArray *filetype_extASAP=[SUPPORTED_FILETYPE_ASAP componentsSeparatedByString:@","];
    NSArray *filetype_extVGMSTREAM=[SUPPORTED_FILETYPE_VGMSTREAM componentsSeparatedByString:@","];
    NSArray *filetype_extVGM=[SUPPORTED_FILETYPE_VGM componentsSeparatedByString:@","];
    NSArray *filetype_extWMIDI=(no_aux_file?[SUPPORTED_FILETYPE_WMIDI componentsSeparatedByString:@","]:[SUPPORTED_FILETYPE_WMIDI_EXT componentsSeparatedByString:@","]);
    NSArray *filetype_extCOVER=[SUPPORTED_FILETYPE_COVER componentsSeparatedByString:@","];
    
    NSString *extension;
    NSString *file_no_ext;
    int found=0;
    
    
    //extension = [_filePath pathExtension];
    //file_no_ext = [[_filePath lastPathComponent] stringByDeletingPathExtension];
    NSMutableArray *temparray_filepath=[NSMutableArray arrayWithArray:[[_filePath lastPathComponent] componentsSeparatedByString:@"."]];
    extension = (NSString *)[temparray_filepath lastObject];
    //[temparray_filepath removeLastObject];
    file_no_ext=[temparray_filepath firstObject];
        
    if (!found)
        for (int i=0;i<[filetype_extVGMSTREAM count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extVGMSTREAM objectAtIndex:i]]==NSOrderedSame) {found=MMP_VGMSTREAM;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extVGMSTREAM objectAtIndex:i]]==NSOrderedSame) {found=MMP_VGMSTREAM;break;}
        }
    if (!found)
        for (int i=0;i<[filetype_extVGM count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extVGM objectAtIndex:i]]==NSOrderedSame) {found=MMP_VGMPLAY;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extVGM objectAtIndex:i]]==NSOrderedSame) {found=MMP_VGMPLAY;break;}
        }
    if (!found)
        for (int i=0;i<[filetype_extASAP count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extASAP objectAtIndex:i]]==NSOrderedSame) {found=MMP_ASAP;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extASAP objectAtIndex:i]]==NSOrderedSame) {found=MMP_ASAP;break;}
        }
    if (!found)
        for (int i=0;i<[filetype_extGME count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extGME objectAtIndex:i]]==NSOrderedSame) {
                //check if .miniXXX or .XXX
                NSArray *singlefile=[SUPPORTED_FILETYPE_GME_WITHEXTFILE componentsSeparatedByString:@","];
                for (int j=0;j<[singlefile count];j++)
                    if ([extension caseInsensitiveCompare:[singlefile objectAtIndex:j]]==NSOrderedSame) {
                        break;
                    }
                found=MMP_GME;break;
            }
            if ([file_no_ext caseInsensitiveCompare:[filetype_extGME objectAtIndex:i]]==NSOrderedSame) {
                //check if .miniXXX or .XXX
                NSArray *singlefile=[SUPPORTED_FILETYPE_GME_WITHEXTFILE componentsSeparatedByString:@","];
                for (int j=0;j<[singlefile count];j++)
                    if ([file_no_ext caseInsensitiveCompare:[singlefile objectAtIndex:j]]==NSOrderedSame) {
                        break;
                    }
                found=MMP_GME;break;
            }
        }
    
    if (!found)
        for (int i=0;i<[filetype_extSID count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extSID objectAtIndex:i]]==NSOrderedSame) {found=MMP_SIDPLAY;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extSID objectAtIndex:i]]==NSOrderedSame) {found=MMP_SIDPLAY;break;}
        }
    if (!found)
        for (int i=0;i<[filetype_extPMD count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extPMD objectAtIndex:i]]==NSOrderedSame) {found=MMP_PMDMINI;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extPMD objectAtIndex:i]]==NSOrderedSame) {found=MMP_PMDMINI;break;}
        }
    if (!found)
        for (int i=0;i<[filetype_extADPLUG count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extADPLUG objectAtIndex:i]]==NSOrderedSame) {found=MMP_ADPLUG;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extADPLUG objectAtIndex:i]]==NSOrderedSame) {found=MMP_ADPLUG;break;}
        }
    
    if (!found)
        for (int i=0;i<[filetype_extSTSOUND count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extSTSOUND objectAtIndex:i]]==NSOrderedSame) {found=MMP_STSOUND;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extSTSOUND objectAtIndex:i]]==NSOrderedSame) {found=MMP_STSOUND;break;}
        }
    if (!found)
        for (int i=0;i<[filetype_extATARISOUND count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extATARISOUND objectAtIndex:i]]==NSOrderedSame) {found=MMP_ATARISOUND;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extATARISOUND objectAtIndex:i]]==NSOrderedSame) {found=MMP_ATARISOUND;break;}
        }
    if (!found)
        for (int i=0;i<[filetype_extSC68 count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extSC68 objectAtIndex:i]]==NSOrderedSame) {found=MMP_SC68;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extSC68 objectAtIndex:i]]==NSOrderedSame) {found=MMP_SC68;break;}
        }
    if (!found)
        for (int i=0;i<[filetype_extPT3 count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extPT3 objectAtIndex:i]]==NSOrderedSame) {found=MMP_PT3;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extPT3 objectAtIndex:i]]==NSOrderedSame) {found=MMP_PT3;break;}
        }
    if (!found)
        for (int i=0;i<[filetype_extGBS count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extGBS objectAtIndex:i]]==NSOrderedSame) {found=MMP_GBS;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extGBS objectAtIndex:i]]==NSOrderedSame) {found=MMP_GBS;break;}
        }
    if (!found) {
        //        for (int i=0;i<[filetype_extNSFPLAY count];i++) {
        //            if ([extension caseInsensitiveCompare:[filetype_extNSFPLAY objectAtIndex:i]]==NSOrderedSame) {found=MMP_NSFPLAY;break;}
        //            if ([file_no_ext caseInsensitiveCompare:[filetype_extNSFPLAY objectAtIndex:i]]==NSOrderedSame) {found=MMP_NSFPLAY;break;}
        //        }
        for (int i=0;i<[filetype_extNSFPLAY count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extNSFPLAY objectAtIndex:i]]==NSOrderedSame) {
                NSArray *singlefile=[SUPPORTED_FILETYPE_NSFPLAY_WITHEXTFILE componentsSeparatedByString:@","];
                for (int j=0;j<[singlefile count];j++)
                    if ([extension caseInsensitiveCompare:[singlefile objectAtIndex:j]]==NSOrderedSame) {
                        break;
                    }
                found=MMP_NSFPLAY;break;
            }
            if ([file_no_ext caseInsensitiveCompare:[filetype_extNSFPLAY objectAtIndex:i]]==NSOrderedSame) {
                NSArray *singlefile=[SUPPORTED_FILETYPE_NSFPLAY_WITHEXTFILE componentsSeparatedByString:@","];
                for (int j=0;j<[singlefile count];j++)
                    if ([file_no_ext caseInsensitiveCompare:[singlefile objectAtIndex:j]]==NSOrderedSame) {
                        break;
                    }
                found=MMP_NSFPLAY;break;
            }
        }
    }
    if (!found)
        for (int i=0;i<[filetype_extPIXEL count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extPIXEL objectAtIndex:i]]==NSOrderedSame) {found=MMP_PIXEL;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extPIXEL objectAtIndex:i]]==NSOrderedSame) {found=MMP_PIXEL;break;}
        }
    if (!found)
        for (int i=0;i<[filetype_ext2SF count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_ext2SF objectAtIndex:i]]==NSOrderedSame) {
                //check if .miniXXX or .XXX
                NSArray *singlefile=[SUPPORTED_FILETYPE_2SF_WITHEXTFILE componentsSeparatedByString:@","];
                for (int j=0;j<[singlefile count];j++)
                    if ([extension caseInsensitiveCompare:[singlefile objectAtIndex:j]]==NSOrderedSame) {
                        break;
                    }
                found=MMP_2SF;break;
            }
            if ([file_no_ext caseInsensitiveCompare:[filetype_ext2SF objectAtIndex:i]]==NSOrderedSame) {
                //check if .miniXXX or .XXX
                NSArray *singlefile=[SUPPORTED_FILETYPE_2SF_WITHEXTFILE componentsSeparatedByString:@","];
                for (int j=0;j<[singlefile count];j++)
                    if ([file_no_ext caseInsensitiveCompare:[singlefile objectAtIndex:j]]==NSOrderedSame) {
                        break;
                    }
                found=MMP_2SF;break;
            }
        }
    if (!found)
        for (int i=0;i<[filetype_extV2M count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extV2M objectAtIndex:i]]==NSOrderedSame) {found=MMP_V2M;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extV2M objectAtIndex:i]]==NSOrderedSame) {found=MMP_V2M;break;}
        }
    if (!found)
        for (int i=0;i<[filetype_extHC count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extHC objectAtIndex:i]]==NSOrderedSame) {
                //check if .miniXXX or .XXX
                NSArray *singlefile=[SUPPORTED_FILETYPE_HC_WITHEXTFILE componentsSeparatedByString:@","];
                for (int j=0;j<[singlefile count];j++)
                    if ([extension caseInsensitiveCompare:[singlefile objectAtIndex:j]]==NSOrderedSame) {
                        break;
                    }
                found=MMP_HC;break;
            }
            if ([file_no_ext caseInsensitiveCompare:[filetype_extHC objectAtIndex:i]]==NSOrderedSame) {
                //check if .miniXXX or .XXX
                NSArray *singlefile=[SUPPORTED_FILETYPE_HC_WITHEXTFILE componentsSeparatedByString:@","];
                for (int j=0;j<[singlefile count];j++)
                    if ([file_no_ext caseInsensitiveCompare:[singlefile objectAtIndex:j]]==NSOrderedSame) {
                        break;
                    }
                found=MMP_HC;break;
            }
        }
    /*if (!found)
        for (int i=0;i<[filetype_extMDX count];i++) { //PDX might be required
            if ([extension caseInsensitiveCompare:[filetype_extMDX objectAtIndex:i]]==NSOrderedSame) {
                found=MMP_MDXPDX;break;
            }
            if ([file_no_ext caseInsensitiveCompare:[filetype_extMDX objectAtIndex:i]]==NSOrderedSame) {
                found=MMP_MDXPDX;break;
            }
        }*/
    if (!found)
        for (int i=0;i<[filetype_extMDX count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extMDX objectAtIndex:i]]==NSOrderedSame) {
                //check if file or ressource
                NSArray *singlefile=[SUPPORTED_FILETYPE_MDX_WITHEXTFILE componentsSeparatedByString:@","];
                for (int j=0;j<[singlefile count];j++)
                    if ([extension caseInsensitiveCompare:[singlefile objectAtIndex:j]]==NSOrderedSame) {
                        break;
                    }
                found=MMP_MDXPDX;break;
            }
            if ([file_no_ext caseInsensitiveCompare:[filetype_extMDX objectAtIndex:i]]==NSOrderedSame) {
                //check if file or ressource
                NSArray *singlefile=[SUPPORTED_FILETYPE_MDX_WITHEXTFILE componentsSeparatedByString:@","];
                for (int j=0;j<[singlefile count];j++)
                    if ([file_no_ext caseInsensitiveCompare:[singlefile objectAtIndex:j]]==NSOrderedSame) {
                        break;
                    }
                found=MMP_MDXPDX;break;
            }
        }
    if (!found)
        for (int i=0;i<[filetype_extEUP count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extEUP objectAtIndex:i]]==NSOrderedSame) {
                //check if file or ressource
                NSArray *singlefile=[SUPPORTED_FILETYPE_EUP_WITHEXTFILE componentsSeparatedByString:@","];
                for (int j=0;j<[singlefile count];j++)
                    if ([extension caseInsensitiveCompare:[singlefile objectAtIndex:j]]==NSOrderedSame) {
                        break;
                    }
                found=MMP_EUP;break;
            }
            if ([file_no_ext caseInsensitiveCompare:[filetype_extEUP objectAtIndex:i]]==NSOrderedSame) {
                //check if file or ressource
                NSArray *singlefile=[SUPPORTED_FILETYPE_EUP_WITHEXTFILE componentsSeparatedByString:@","];
                for (int j=0;j<[singlefile count];j++)
                    if ([file_no_ext caseInsensitiveCompare:[singlefile objectAtIndex:j]]==NSOrderedSame) {
                        break;
                    }
                found=MMP_EUP;break;
            }
        }
    if (!found)
        for (int i=0;i<[filetype_extGSF count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extGSF objectAtIndex:i]]==NSOrderedSame) {
                //check if .miniXXX or .XXX
                NSArray *singlefile=[SUPPORTED_FILETYPE_GSF_WITHEXTFILE componentsSeparatedByString:@","];
                for (int j=0;j<[singlefile count];j++)
                    if ([extension caseInsensitiveCompare:[singlefile objectAtIndex:j]]==NSOrderedSame) {
                        break;
                    }
                found=MMP_GSF;break;
            }
            if ([file_no_ext caseInsensitiveCompare:[filetype_extGSF objectAtIndex:i]]==NSOrderedSame) {
                //check if .miniXXX or .XXX
                NSArray *singlefile=[SUPPORTED_FILETYPE_GSF_WITHEXTFILE componentsSeparatedByString:@","];
                for (int j=0;j<[singlefile count];j++)
                    if ([file_no_ext caseInsensitiveCompare:[singlefile objectAtIndex:j]]==NSOrderedSame) {
                        break;
                    }
                found=MMP_GSF;break;
            }
        }
    //tmp hack => redirect to timidity
    if (!found)
        for (int i=0;i<[filetype_extWMIDI count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extWMIDI objectAtIndex:i]]==NSOrderedSame) {found=MMP_TIMIDITY;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extWMIDI objectAtIndex:i]]==NSOrderedSame) {found=MMP_TIMIDITY;break;}
        }
    if (!found)
        for (int i=0;i<[filetype_extUADE count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extUADE objectAtIndex:i]]==NSOrderedSame) {
                //check if require second file
                NSArray *singlefile=[SUPPORTED_FILETYPE_UADE_WITHEXTFILE componentsSeparatedByString:@","];
                for (int j=0;j<[singlefile count];j++)
                    if ([extension caseInsensitiveCompare:[singlefile objectAtIndex:j]]==NSOrderedSame) {
                        break;
                    }
                
                found=MMP_UADE;break;
            }
            if ([file_no_ext caseInsensitiveCompare:[filetype_extUADE objectAtIndex:i]]==NSOrderedSame) {
                //check if require second file
                NSArray *singlefile=[SUPPORTED_FILETYPE_UADE_WITHEXTFILE componentsSeparatedByString:@","];
                for (int j=0;j<[singlefile count];j++)
                    if ([file_no_ext caseInsensitiveCompare:[singlefile objectAtIndex:j]]==NSOrderedSame) {
                        break;
                    }
                found=MMP_UADE;break;
            }
        }
    if ((!found))
        for (int i=0;i<[filetype_extMODPLUG count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extMODPLUG objectAtIndex:i]]==NSOrderedSame) {found=MMP_OPENMPT;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extMODPLUG objectAtIndex:i]]==NSOrderedSame) {found=MMP_OPENMPT;break;}
        }
    if ((!found))
        for (int i=0;i<[filetype_extXMP count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extXMP objectAtIndex:i]]==NSOrderedSame) {found=MMP_OPENMPT;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extXMP objectAtIndex:i]]==NSOrderedSame) {found=MMP_OPENMPT;break;}
        }
    if (!found) {
        for (int i=0;i<[filetype_extHVL count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extHVL objectAtIndex:i]]==NSOrderedSame) {found=MMP_HVL;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extHVL objectAtIndex:i]]==NSOrderedSame) {found=MMP_HVL;break;}
        }
    }
    if (!found) {
        for (int i=0;i<[filetype_extS98 count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extS98 objectAtIndex:i]]==NSOrderedSame) {found=MMP_S98;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extS98 objectAtIndex:i]]==NSOrderedSame) {found=MMP_S98;break;}
        }
    }
    if (!found) {
        for (int i=0;i<[filetype_extKSS count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extKSS objectAtIndex:i]]==NSOrderedSame) {found=MMP_KSS;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extKSS objectAtIndex:i]]==NSOrderedSame) {found=MMP_KSS;break;}
        }
    }
    
    if (!no_aux_file) {
        if (!found)
            for (int i=0;i<[filetype_extCOVER count];i++) {
                if ([extension caseInsensitiveCompare:[filetype_extCOVER objectAtIndex:i]]==NSOrderedSame)    {found=99;break;}
                if ([file_no_ext caseInsensitiveCompare:[filetype_extCOVER objectAtIndex:i]]==NSOrderedSame) {found=99;break;}
                
            }
    }
    
    return found;
}

+(NSString*) getFullPathForFilePath:(NSString*)filePath {
    NSString *fullFilePath;
    if (icloud_available && ([filePath containsString:[icloudURL path]])) fullFilePath=[NSString stringWithString:filePath];
    else fullFilePath=[NSHomeDirectory() stringByAppendingPathComponent:filePath];
    return fullFilePath;
}


+(NSString *)getFullCleanFilePath:(NSString*)filePath {
    NSRange r=[filePath rangeOfString:@"?"];
    if (r.location != NSNotFound) {
        //remove the subsong index
        filePath=[filePath substringToIndex:r.location];
    }
    r=[filePath rangeOfString:@"@"];
    if (r.location != NSNotFound) {
        //remove the subsong index
        filePath=[filePath substringToIndex:r.location];
    }
    return filePath;
}

+(NSString *)getFilePathNoSubSong:(NSString*)filePath {
    NSRange r=[filePath rangeOfString:@"?"];
    if (r.location != NSNotFound) {
        //remove the subsong index
        filePath=[filePath substringToIndex:r.location];
    }
    return filePath;
}

+(NSString *)getFilePathFromDocuments:(NSString*)filePath {
    NSMutableArray *tmp_path=[NSMutableArray arrayWithArray:[filePath componentsSeparatedByString:@"/"]];
    for (;;) {
        if ([(NSString *)[tmp_path firstObject] compare:@"Documents"]==NSOrderedSame) {
            break;
        }
        [tmp_path removeObjectAtIndex:0];
        if ([tmp_path count]==0) return NULL;
    }
    return [tmp_path componentsJoinedByString:@"/"];
}

+(NSString*) getCorrectFileName:(const char*)archiveFilename archive:(struct archive *)a entry:(struct archive_entry *)entry {
    NSString *file;
    if (archive_format(a) == ARCHIVE_FORMAT_RAW) {
        file=[[[NSString stringWithUTF8String:archiveFilename] lastPathComponent] stringByDeletingPathExtension];
    } else {
        file=[NSString stringWithUTF8String:archive_entry_pathname(entry)];        
        if (file==nil) {
            file=[NSString stringWithCString:archive_entry_pathname(entry) encoding:NSISOLatin1StringEncoding];
            if (file==nil) file=[NSString stringWithFormat:@"%s",archive_entry_pathname(entry)];
        }
    }
    return file;
}

+(int) scanarchive:(const char *)path filesList_ptr:(char***)filesList_ptr filesCount_ptr:(int*)filesCount_ptr {
    int found=0;
    struct archive *a = archive_read_new();
    struct archive_entry *entry;
    int r;
    
    //Specific case for RAR files as libarchive is buggy for them and UnrarKIT is used instead
    NSString *cpath=[NSString stringWithUTF8String:path];
    if ( ([[[cpath pathExtension] uppercaseString] isEqualToString:@"RAR"]) ||
        ([[[cpath pathExtension] uppercaseString] isEqualToString:@"RSN"]) ) {
        NSError *error = nil;
        URKArchive *archive = [[URKArchive alloc] initWithPath:cpath error:&error];
        if (!archive) {
            NSLog(@"Error: %ld %@",error.code,error.localizedDescription);
            return 0;
        }
        NSArray *filesInArchive = [archive listFilenames:&error];
        if (!filesInArchive) {
            NSLog(@"Error: %ld %@",error.code,error.localizedDescription);
            return 0;
        }
        NSString *file;
        NSMutableArray *filesList=[[NSMutableArray alloc] init];
        for (file in filesInArchive) {
            if ([ModizFileHelper isAcceptedFile:file no_aux_file:1]) {
                found++;
                [filesList addObject:file];
            }
        }
        if (filesCount_ptr) *filesCount_ptr=found;
        if (filesList_ptr) {
            *filesList_ptr=(char**)malloc(found*sizeof(char*));
            int i=0;
            for (file in filesList) {
                (*filesList_ptr)[i]=(char*)malloc(strlen([file fileSystemRepresentation])+1);
                strcpy((*filesList_ptr)[i],[file fileSystemRepresentation]);
                i++;
            }
        }
        
        return found;
    }
    
    archive_read_support_filter_all(a);
    archive_read_support_format_raw(a);
    archive_read_support_format_all(a);
    r = archive_read_open_filename(a, path, 16384);
        
    if (r==ARCHIVE_OK) {
        NSMutableArray *filesList=[[NSMutableArray alloc] init];
        NSString *file;
        for (;;) {
            r=archive_read_next_header(a, &entry);
            if ((r!=ARCHIVE_OK)&&(r!=ARCHIVE_WARN)) break;
            file=[ModizFileHelper getCorrectFileName:path archive:a entry:entry];
            if ([ModizFileHelper isAcceptedFile:file no_aux_file:1]) {
                found++;
                [filesList addObject:file];
            }
        }
        if (filesCount_ptr) *filesCount_ptr=found;
        
        if (filesList_ptr) {
            *filesList_ptr=(char**)malloc(found*sizeof(char*));
            int i=0;
            for (file in filesList) {
                (*filesList_ptr)[i]=(char*)malloc(strlen([file fileSystemRepresentation])+1);
                strcpy((*filesList_ptr)[i],[file fileSystemRepresentation]);
                i++;
            }
        }
        
    } else {
        NSLog( @"Skipping unsupported archive: %s\n", path );
    }
    r = archive_read_free(a);
    return found;
}

+(BOOL)addSkipBackupAttributeToItemAtPath:(NSString*)path {
    const char* filePath = [path fileSystemRepresentation];
    
    const char* attrName = "com.apple.MobileBackup";
    u_int8_t attrValue = 1;
    
    int result = setxattr(filePath, attrName, &attrValue, sizeof(attrValue), 0, 0);
    return result == 0;
}

+(void) updateFilesDoNotBackupAttributes {
    NSError *error;
    NSArray *dirContent;
    int result;
    //BOOL isDir;
    NSFileManager *fileManager = [[NSFileManager alloc] init];
    NSString *cpath=[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents/Samples"];
    NSString *file;
    const char* attrName = "com.apple.MobileBackup";
    u_int8_t attrValue = 1;
    
    dirContent=[fileManager subpathsOfDirectoryAtPath:cpath error:&error];
    for (file in dirContent) {
        //NSLog(@"%@",file);
        //        [mFileMngr fileExistsAtPath:[cpath stringByAppendingFormat:@"/%@",file] isDirectory:&isDir];
        result = setxattr([[cpath stringByAppendingFormat:@"/%@",file] fileSystemRepresentation], attrName, &attrValue, sizeof(attrValue), 0, 0);
        if (result) NSLog(@"Issue %d when settings nobackup flag on %@",result,[cpath stringByAppendingFormat:@"/%@",file]);
    }
    fileManager=nil;
}

+(void) extractToPath:(const char *)archivePath path:(const char *)extractPath caller:(NSObject*)caller progress:(NSProgress*)extractProgress context:(void*)context {
    //Specific case for RAR files as libarchive is buggy for them and UnrarKIT is used instead
    NSString *cpath=[NSString stringWithUTF8String:archivePath];
    if ( ([[[cpath pathExtension] uppercaseString] isEqualToString:@"RAR"]) ||
        ([[[cpath pathExtension] uppercaseString] isEqualToString:@"RSN"]) ) {
        NSError *error = nil;
        URKArchive *archive = [[URKArchive alloc] initWithPath:cpath error:&error];
        if (!archive) {
            NSLog(@"Error: %ld %@",error.code,error.localizedDescription);
            return;
        }
        NSArray *filesInArchive = [archive listFilenames:&error];
                
        [extractProgress setTotalUnitCount:[filesInArchive count]];
        NSString *observedSelector = NSStringFromSelector(@selector(fractionCompleted));
        [extractProgress addObserver:caller
                              forKeyPath:observedSelector
                                 options:NSKeyValueObservingOptionInitial
                                 context:context];
        
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
            NSError *error;
            
            [extractProgress becomeCurrentWithPendingUnitCount:[filesInArchive count]];
            
            [archive extractFilesTo:[NSString stringWithUTF8String:extractPath] overwrite:YES error:&error];
            if (error) {
                NSLog(@"Error: %ld %@",error.code,error.localizedDescription);
            }
            [extractProgress removeObserver:caller
                                     forKeyPath:NSStringFromSelector(@selector(fractionCompleted))
                                        context:context];
            [extractProgress resignCurrent];
            
//            dispatch_sync(dispatch_get_main_queue(), ^(void){
//                //Run UI Updates
//            });
            
        });
        
        return;
    }
    
    //1st get total files nb in archive
    struct archive *a = archive_read_new();
    struct archive_entry *entry;
    int r;
    int files_nb=0;
    archive_read_support_filter_all(a);
    archive_read_support_format_raw(a);
    archive_read_support_format_all(a);
    r = archive_read_open_filename(a, archivePath, 16384);
    if (r!=ARCHIVE_OK) return;
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        files_nb++;
    }
    archive_read_free(a);
    
    [extractProgress setTotalUnitCount:files_nb];
    
                
    NSString *observedSelector = NSStringFromSelector(@selector(fractionCompleted));
    [extractProgress addObserver:caller
                          forKeyPath:observedSelector
                             options:NSKeyValueObservingOptionInitial
                             context:context];
    
    NSString *archivePathNS=[NSString stringWithUTF8String:archivePath];
    NSString *extractPathNS=[NSString stringWithUTF8String:extractPath];
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
        struct archive *a = archive_read_new();
        struct archive_entry *entry;
        int r;
        NSError *err;
        
        archive_read_support_filter_all(a);
        archive_read_support_format_raw(a);
        archive_read_support_format_all(a);
        r = archive_read_open_filename(a, [archivePathNS UTF8String], 16384);
        
        
        if (r==ARCHIVE_OK) {
            FILE *f;
            int r;
            NSString *extractFilename,*extractPathFile;
            
            NSFileManager *mFileMngr=[[NSFileManager alloc] init];
                        
            [extractProgress becomeCurrentWithPendingUnitCount:files_nb];
            
            int idx=0;
            int cpt=0;
            for (;;) {
                r = archive_read_next_header(a, &entry);
                
                                
                if (r == ARCHIVE_EOF) break;
                if (r != ARCHIVE_OK) {
                    NSLog(@"archive_read_next_header() %s", archive_error_string(a));
                    break;
                }
                
                if ([ModizFileHelper isAcceptedFile:[ModizFileHelper getCorrectFileName:[archivePathNS UTF8String] archive:a entry:entry] no_aux_file:0]) {
                    
                    extractFilename=[NSString stringWithFormat:@"%@/%@",extractPathNS,[ModizFileHelper getCorrectFileName:archivePath archive:a entry:entry]];
                    extractPathFile=[extractFilename stringByDeletingLastPathComponent];
                    
                    //1st create path if not existing yet
                    [mFileMngr createDirectoryAtPath:extractPathFile withIntermediateDirectories:TRUE attributes:nil error:&err];
                    [ModizFileHelper addSkipBackupAttributeToItemAtPath:extractPathFile];
                    
                    //2nd extract file
                    f=fopen([extractFilename fileSystemRepresentation],"wb");
                    if (!f) {
                        NSLog(@"Cannot open %@ to extract %@",extractFilename,archivePathNS);
                    } else {
                        const void *buff;
                        size_t size;
                        la_int64_t offset;
                        
                        for (;;) {
                            if ([extractProgress isCancelled]) {
                                break;
                            }
                            
                            r = archive_read_data_block(a, &buff, &size, &offset);
                            if (r == ARCHIVE_EOF) break;
                            if (r < ARCHIVE_OK) break;
                            fwrite(buff,size,1,f);
                        }
                        
                        fclose(f);
                        
                        if ([ModizFileHelper isAcceptedFile:[ModizFileHelper getCorrectFileName:archivePath archive:a entry:entry] no_aux_file:1]) {
                            idx++;
                        }
                    }
                }
                //update NSProgress
                cpt++;
                [extractProgress setCompletedUnitCount:cpt];
                if ([extractProgress isCancelled]) {
                    break;
                }
            }
            
            [extractProgress removeObserver:caller
                                     forKeyPath:NSStringFromSelector(@selector(fractionCompleted))
                                        context:context];
            [extractProgress resignCurrent];
            
            NSLog(@"over");
        }
        archive_read_free(a);
        
        
    });
    
    return;
}

+(NSString *)getFullCleanFilePath:(NSString*)filePath arcidx_ptr:(int*)arcidx_ptr subsong_ptr:(int*)subsong_ptr {
    //check if arcidx (@%d) or subsong (?%d) are specified in filePath
    const char *tmp_str;
    NSString *ext=[[filePath componentsSeparatedByString:@"."] lastObject];
    if (ext) tmp_str=[ext UTF8String];
    else return nil;
    
    NSString *filePathTmp=[filePath stringByDeletingPathExtension];
    NSString *extClean=nil;
    
    char tmp_str_copy[1024];
    int found_arc=0;
    int arc_index=-1;
    int found_sub=0;
    int sub_index=-1;
    int i=0;
    
    while (tmp_str[i]) {
        if (found_arc) {
            arc_index=arc_index*10+tmp_str[i]-'0';
        }
        if (found_sub) {
            sub_index=sub_index*10+tmp_str[i]-'0';
        }
        if (tmp_str[i]=='@') {
            found_arc=1;
            arc_index=0;
            memcpy(tmp_str_copy,tmp_str,i);
            tmp_str_copy[i]=0;
            if (!extClean) extClean=[NSString stringWithUTF8String:tmp_str_copy];
        }
        if (tmp_str[i]=='?') {
            found_sub=1;
            sub_index=0;
            memcpy(tmp_str_copy,tmp_str,i);
            tmp_str_copy[i]=0;
            if (!extClean) extClean=[NSString stringWithUTF8String:tmp_str_copy];
        }
        i++;
    }
    if ((found_arc==0)&&(found_sub==0)) filePathTmp=[NSString stringWithString:filePath];
    else filePathTmp=[NSString stringWithFormat:@"%@.%@",filePathTmp,extClean];
    
    if (found_arc) *arcidx_ptr=arc_index;
    if (found_sub) *subsong_ptr=sub_index;
    return filePathTmp;
}


@end
