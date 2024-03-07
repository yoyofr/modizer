//
//  ModizFileHelper.mm
//  modizer
//
//  Created by Yohann Magnien David on 07/03/2024.
//

#import "ModizFileHelper.h"

//FEX
#include "fex.h"
//GME
#include "gme.h"
//SID2
#include "sidplayfp/SidTune.h"
#include "sidplayfp/SidTuneInfo.h"


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
    
    return true;
    
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
    fex_type_t type;
    fex_t* fex;
    
    NSArray *filetypes_ext=[SUPPORTED_FILETYPE_ARCFILE componentsSeparatedByString:@","];
    if ([filetypes_ext indexOfObject:[[cpath pathExtension] uppercaseString]]==NSNotFound) return ret;
    
    /* Determine file's type */
    if (fex_identify_file( &type, [cpath UTF8String] )) {
        NSLog(@"fex cannot determine type of %@",cpath);
    }
    /* Only open files that fex can handle */
    if ( type != NULL ) {
        if (strcmp(fex_type_name(type),"file")!=0) {
            if (fex_open_type( &fex, [cpath UTF8String], type )) {
                NSLog(@"cannot fex open : %s / type : %d",[cpath UTF8String],type);
            } else {
                ret=true;
                fex_close( fex );
            }
        }
    }
    fex=NULL;
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
            NSArray *filetype_extNSFPLAY=[SUPPORTED_FILETYPE_NSFPLAY componentsSeparatedByString:@","];
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
            NSArray *filetype_extArchive=[SUPPORTED_FILETYPE_ARCFILE componentsSeparatedByString:@","];
            filetype_ext=[NSMutableArray arrayWithCapacity:[filetype_extArchive count]];
            [filetype_ext addObjectsFromArray:filetype_extArchive];
            break;
        }
    }
    return filetype_ext;
}

+(int) isAcceptedFile:(NSString*)_filePath no_aux_file:(int)no_aux_file {
    NSArray *filetype_extMDX=[SUPPORTED_FILETYPE_MDX componentsSeparatedByString:@","];
    NSArray *filetype_extPMD=[SUPPORTED_FILETYPE_PMD componentsSeparatedByString:@"."];
    NSArray *filetype_extSID=[SUPPORTED_FILETYPE_SID componentsSeparatedByString:@","];
    NSArray *filetype_extSTSOUND=[SUPPORTED_FILETYPE_STSOUND componentsSeparatedByString:@","];
    NSArray *filetype_extATARISOUND=[SUPPORTED_FILETYPE_ATARISOUND componentsSeparatedByString:@","];
    NSArray *filetype_extSC68=[SUPPORTED_FILETYPE_SC68 componentsSeparatedByString:@","];
    NSArray *filetype_extPT3=[SUPPORTED_FILETYPE_PT3 componentsSeparatedByString:@","];
    NSArray *filetype_extNSFPLAY=[SUPPORTED_FILETYPE_NSFPLAY componentsSeparatedByString:@","];
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
        for (int i=0;i<[filetype_extNSFPLAY count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extNSFPLAY objectAtIndex:i]]==NSOrderedSame) {found=MMP_NSFPLAY;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extNSFPLAY objectAtIndex:i]]==NSOrderedSame) {found=MMP_NSFPLAY;break;}
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

+(int) isAllowedFile:(NSString*)file {
    NSArray *filetype_extSID=[SUPPORTED_FILETYPE_SID componentsSeparatedByString:@","];
    NSArray *filetype_extMDX=[SUPPORTED_FILETYPE_MDX componentsSeparatedByString:@","];
    NSArray *filetype_extSTSOUND=[SUPPORTED_FILETYPE_STSOUND componentsSeparatedByString:@","];
    NSArray *filetype_extATARISOUND=[SUPPORTED_FILETYPE_ATARISOUND componentsSeparatedByString:@","];
    NSArray *filetype_extSC68=[SUPPORTED_FILETYPE_SC68 componentsSeparatedByString:@","];
    NSArray *filetype_extPT3=[SUPPORTED_FILETYPE_PT3 componentsSeparatedByString:@","];
    NSArray *filetype_extNSFPLAY=[SUPPORTED_FILETYPE_NSFPLAY componentsSeparatedByString:@","];
    NSArray *filetype_extPIXEL=[SUPPORTED_FILETYPE_PIXEL componentsSeparatedByString:@","];
    NSArray *filetype_extUADE=[SUPPORTED_FILETYPE_UADE componentsSeparatedByString:@","];
    NSArray *filetype_extMODPLUG=[SUPPORTED_FILETYPE_OMPT componentsSeparatedByString:@","];
    NSArray *filetype_extXMP=[SUPPORTED_FILETYPE_XMP componentsSeparatedByString:@","];
    NSArray *filetype_extGME=[SUPPORTED_FILETYPE_GME componentsSeparatedByString:@","];
    NSArray *filetype_extADPLUG=[SUPPORTED_FILETYPE_ADPLUG componentsSeparatedByString:@","];
    NSArray *filetype_extHC=[SUPPORTED_FILETYPE_HC componentsSeparatedByString:@","];
    NSArray *filetype_extEUP=[SUPPORTED_FILETYPE_EUP componentsSeparatedByString:@","];
    NSArray *filetype_extHVL=[SUPPORTED_FILETYPE_HVL componentsSeparatedByString:@","];
    NSArray *filetype_extS98=[SUPPORTED_FILETYPE_S98 componentsSeparatedByString:@","];
    NSArray *filetype_extKSS=[SUPPORTED_FILETYPE_KSS componentsSeparatedByString:@","];
    NSArray *filetype_extGSF=[SUPPORTED_FILETYPE_GSF componentsSeparatedByString:@","];
    NSArray *filetype_extASAP=[SUPPORTED_FILETYPE_ASAP componentsSeparatedByString:@","];
    NSArray *filetype_extVGM=[SUPPORTED_FILETYPE_VGM componentsSeparatedByString:@","];
    NSArray *filetype_extWMIDI=[SUPPORTED_FILETYPE_WMIDI componentsSeparatedByString:@","];
    NSArray *filetype_extARCHIVE=[SUPPORTED_FILETYPE_ARCHIVE componentsSeparatedByString:@","];
    NSArray *filetype_extPMD=[SUPPORTED_FILETYPE_PMD componentsSeparatedByString:@","];
    NSArray *filetype_ext2SF=[SUPPORTED_FILETYPE_2SF componentsSeparatedByString:@","];
    NSArray *filetype_extV2M=[SUPPORTED_FILETYPE_V2M componentsSeparatedByString:@","];
    NSArray *filetype_extVGMSTREAM=[SUPPORTED_FILETYPE_VGMSTREAM componentsSeparatedByString:@","];
    NSString *extension;// = [file pathExtension];
    NSString *file_no_ext;// = [[file lastPathComponent] stringByDeletingPathExtension];
    
    NSMutableArray *temparray_filepath=[NSMutableArray arrayWithArray:[[file lastPathComponent] componentsSeparatedByString:@"."]];
    extension = (NSString *)[temparray_filepath lastObject];
    //[temparray_filepath removeLastObject];
    file_no_ext=[temparray_filepath firstObject];
    
    int found=0;
    for (int i=0;i<[filetype_extUADE count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_extUADE objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
        if ([file_no_ext caseInsensitiveCompare:[filetype_extUADE objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
    }
    if (!found)
        for (int i=0;i<[filetype_extMDX count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extMDX objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extMDX objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
        }
    if (!found)
        for (int i=0;i<[filetype_extSID count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extSID objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extSID objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
        }
    if (!found)
        for (int i=0;i<[filetype_extSTSOUND count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extSTSOUND objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extSTSOUND objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
        }
    if (!found)
        for (int i=0;i<[filetype_extATARISOUND count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extATARISOUND objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extATARISOUND objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
        }
    if (!found)
        for (int i=0;i<[filetype_extATARISOUND count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extATARISOUND objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extATARISOUND objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
        }
    if (!found)
        for (int i=0;i<[filetype_extSC68 count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extSC68 objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extSC68 objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
        }
    if (!found)
        for (int i=0;i<[filetype_extPT3 count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extPT3 objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extPT3 objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
        }
    if (!found)
        for (int i=0;i<[filetype_extNSFPLAY count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extNSFPLAY objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extNSFPLAY objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
        }
    if (!found)
        for (int i=0;i<[filetype_extPIXEL count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extPIXEL objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extPIXEL objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
        }
    if (!found)
        for (int i=0;i<[filetype_extMODPLUG count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extMODPLUG objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extMODPLUG objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
        }
    if (!found)
        for (int i=0;i<[filetype_extXMP count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extXMP objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extXMP objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
        }
    if (!found)
        for (int i=0;i<[filetype_extGME count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extGME objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extGME objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
        }
    if (!found)
        for (int i=0;i<[filetype_extADPLUG count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extADPLUG objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extADPLUG objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
        }
    if (!found)
        for (int i=0;i<[filetype_ext2SF count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_ext2SF objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_ext2SF objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
        }
    if (!found)
        for (int i=0;i<[filetype_extV2M count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extV2M objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extV2M objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
        }
    if (!found)
        for (int i=0;i<[filetype_extVGMSTREAM count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extVGMSTREAM objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extVGMSTREAM objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
        }
    if (!found)
        for (int i=0;i<[filetype_extHC count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extHC objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extHC objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
        }
    if (!found)
        for (int i=0;i<[filetype_extEUP count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extEUP objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extEUP objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
        }
    if (!found)
        for (int i=0;i<[filetype_extHVL count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extHVL objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extHVL objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
        }
    if (!found)
        for (int i=0;i<[filetype_extS98 count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extS98 objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extS98 objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
        }
    if (!found)
        for (int i=0;i<[filetype_extKSS count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extKSS objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extKSS objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
        }
    if (!found)
        for (int i=0;i<[filetype_extGSF count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extGSF objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extGSF objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
        }
    if (!found)
        for (int i=0;i<[filetype_extASAP count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extASAP objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extASAP objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
        }
    if (!found)
        for (int i=0;i<[filetype_extVGM count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extVGM objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extVGM objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
        }
    if (!found)
        for (int i=0;i<[filetype_extWMIDI count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extWMIDI objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extWMIDI objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
        }
    if (!found)
        for (int i=0;i<[filetype_extARCHIVE count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extARCHIVE objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extARCHIVE objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
        }
    if (!found)
        for (int i=0;i<[filetype_extPMD count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extPMD objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extPMD objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
        }
    
    if (found) return 1;
    return 0;
}


@end
