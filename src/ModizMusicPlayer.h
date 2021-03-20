//
//  ModizMusicPlayer.h
//  modizer4
//
//  Created by Yohann Magnien on 12/06/10.
//  Copyright 2010 __YoyoFR / Yohann Magnien__. All rights reserved.
//

#import "ModizerConstants.h"

#import <Foundation/Foundation.h>
//#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioToolbox.h>

#import "AppDelegate_Phone.h"

//libopenmpt
#import "../libs/libopenmpt/openmpt-trunk/libopenmpt/libopenmpt.h"
#import "../libs/libopenmpt/openmpt-trunk/libopenmpt/libopenmpt_stream_callbacks_file.h"
#import "../libs/libopenmpt/openmpt-trunk/include/modplug/include/libmodplug/modplug.h"
//MODPLUG
//#import "modplug.h"


//2SF

#import "XSFPlayer.h"
#import "XSFPlayer_NCSF.h"
#import "XSFPlayer_2SF.h"
#import "XSFConfig.h"

//SNSF
#import "SNESSystem.h"


//GME
#import "gme.h"

//ADPLUG
#import "adplug.h"
#import "opl.h"
#import "emuopl.h"
#import "kemuopl.h"
#import "temuopl.h"
//STSOUND
#import "YmMusic.h"
//HVL
#import "hvl_replay.h"

//SC68
#import "api68.h"

extern "C" {
    
	//AOSDK
#import "../libs/aosdk/ao.h"
#import "../libs/aosdk/eng_protos.h"
#import <dirent.h>
#import "driver.h"
// MDX
#import "mdx.h"
// PMD
#import "pmdmini.h"
#import "pmdwinimport.h"

//VGM
#import "../libs/libvgmplay/vgm/chips/mamedef.h"
#import "../libs/libvgmplay/vgm/VGMFile.h"
#import "../libs/libvgmplay/vgm/VGMPlay_Intf.h"
#import "../libs/libvgmplay/vgm/VGMPlay.h"
//CHIPS_OPTION ChipOpts[0x02];
    
}

enum MMP_PLAYER_TYPE {
    MMP_NONE=0,
    MMP_PMDMINI,
    MMP_ADPLUG,
    MMP_SIDPLAY,
    MMP_XMP,
    MMP_OPENMPT,
    MMP_DUMB,
    MMP_UADE,
    MMP_TIMIDITY,
    MMP_VGMSTREAM,
    MMP_MPG123,
    MMP_LAZYUSF,
    MMP_VGMPLAY,
    MMP_GME,
    MMP_ASAP,
    MMP_GSF,
    MMP_MDXPDX,
    MMP_SC68,
    MMP_STSOUND,
    MMP_HVL,
    MMP_SEXYPSF,
    MMP_AOSDK,
    MMP_2SF,
    MMP_SNSF
};



@interface ModizMusicPlayer : NSObject {
	//General infos
	int mod_subsongs;
	int mod_currentsub,mod_minsub,mod_maxsub;
	unsigned int mPlayType;
	int mp_datasize,numChannels;
	int mLoopMode; //0:off, 1:infinite
    long mCurrentSamples,mTgtSamples;

	//Player status
	int bGlobalAudioPause;
	float mVolume;
	//
	//for spectrum analyzer
	short int **buffer_ana_cpy;
	//
	//Option
	//Global
	int optForceMono;
	//HVL
	struct hvl_tune *hvl_song;
	//UADE
	int mUADE_OptChange;
	int mUADE_OptLED,mUADE_OptNORM,mUADE_OptPOSTFX,mUADE_OptPAN,mUADE_OptHEAD,mUADE_OptGAIN,mUADE_OptNTSC;
	float mUADE_OptGAINValue,mUADE_OptPANValue;
	//GME
	int optGMEFadeOut;
    int optGMEIgnoreSilence;
    float optGMERatio;
    bool optGMEEnableRatio;
    //GSF
    char optGSFsoundLowPass;
    char optGSFsoundEcho;
    char optGSFsoundQuality;//1:44Khz, 2:22Khz, 4:11Khz
    char optGSFsoundInterpolation;
	//Modplug
	ModPlug_Settings mp_settings;
    int mPatternDataAvail;
	//AO
	//SexyPSF
	//adplug
    int mADPLUGopltype;
	//SID
	
	
	//Adplug stuff
	CPlayer	*adPlugPlayer;
    CAdPlugDatabase *adplugDB;
	CEmuopl *opl;
	int opl_towrite;
	//
	//GME stuff
	Music_Emu* gme_emu;
	//
	//AO stuff
	unsigned char *ao_buffer;
	ao_display_info ao_info;
	//
    //VGMPLAY stuff
    unsigned int optVGMPLAY_maxloop;
    unsigned char optVGMPLAY_ym2612emulator;
    bool optVGMPLAY_preferJapTag;
    //
	//Modplug stuff
	
	ModPlugFile *mp_file;
	int *genRow,*genPattern,*playRow,*playPattern;//,*playOffset,*genOffset;
    unsigned char *genVolData,*playVolData;
	char *mp_data;
	int numPatterns,numSamples,numInstr;
	//
	AudioQueueRef mAudioQueue;
	AudioQueueBufferRef *mBuffers;
	int mQueueIsBeingStopped;
};
@property int mod_subsongs,mod_currentsub,mod_minsub,mod_maxsub,mLoopMode;
@property long mCurrentSamples,mTgtSamples;
@property int optForceMono;
@property unsigned int mPlayType;
@property int mp_datasize,mPatternDataAvail;
//Adplug stuff
@property CPlayer	*adPlugPlayer;
@property CAdPlugDatabase *adplugDB;
@property CEmuopl *opl;
@property int opl_towrite,mADPLUGopltype;
//GME stuff
@property Music_Emu* gme_emu;
//SID
//AO stuff
@property unsigned char *ao_buffer;
@property ao_display_info ao_info;
//VGMPLAY
@property unsigned int optVGMPLAY_maxloop;
@property unsigned char optVGMPLAY_ym2612emulator;
@property bool optVGMPLAY_preferJapTag;
//Modplug stuff
@property ModPlug_Settings mp_settings;
@property ModPlugFile *mp_file;
@property char *mp_data;
@property int *genRow,*genPattern,*playRow,*playPattern;//,*playOffset,*genOffset;
@property unsigned char *genVolData,*playVolData;
@property float mVolume;
@property int numChannels,numPatterns,numSamples,numInstr;
//GSF stuff
@property char optGSFsoundLowPass;
@property char optGSFsoundEcho;
@property char optGSFsoundQuality;//1:44Khz, 2:22Khz, 4:11Khz
@property char optGSFsoundInterpolation;
//Player status
@property int bGlobalAudioPause;
//for spectrum analyzer
@property short int **buffer_ana_cpy;
@property AudioQueueRef mAudioQueue;
@property AudioQueueBufferRef *mBuffers;
@property int mQueueIsBeingStopped;

-(id) initMusicPlayer;

-(void) generateSoundThread;

-(BOOL) isPlaying;
-(int) isSeeking;

-(int) isPlayingTrackedMusic;

-(void) playPrevSub;
-(void) playNextSub;
-(void) playGoToSub:(int)sub_index;

-(void) selectPrevArcEntry;
-(void) selectNextArcEntry;
-(void) selectArcEntry:(int)arc_index;


-(NSString*) getModMessage;
-(NSString*) getModName;
-(NSString*) getModType;
-(NSString*) getPlayerName;
-(void) setModPlugMasterVol:(float)mstVol;
-(void) Seek:(int)seek_time;

-(int) getSongLengthfromMD5:(int)track_nb;
-(void) setSongLengthfromMD5:(int)track_nb songlength:(int)slength;

-(ModPlug_Settings*) getMPSettings;
-(void) updateMPSettings;

-(BOOL) isEndReached;
-(void) Stop;
-(void) Pause:(BOOL)paused;
-(void) Play;
-(void) PlaySeek:(int)startPos subsong:(int)subsong;
-(int) isAcceptedFile:(NSString*)_filePath;
-(int) LoadModule:(NSString*)_filePath defaultMODPLAYER:(int)defaultMODPLAYER defaultSAPPLAYER:(int)defaultSAPPLAYER defaultVGMPLAYER:(int)defaultVGMPLAYER slowDevice:(int)slowDevice archiveMode:(int)archiveMode archiveIndex:(int)archiveIndex singleSubMode:(int)singleSubMode singleArcMode:(int)singleArcMode;

-(float) getIphoneVolume;
-(void) setIphoneVolume:(float) vol;

-(BOOL) iPhoneDrv_FillAudioBuffer:(AudioQueueBufferRef) mBuffer;
-(BOOL) iPhoneDrv_Init;
-(void) iPhoneDrv_Exit;
-(BOOL) iPhoneDrv_PlayStart;
-(void) iPhoneDrv_PlayStop;
-(void) iPhoneDrv_PlayWaitStop;
-(void) iPhoneDrv_Update:(AudioQueueBufferRef) mBuffer;

-(int) getCurrentPlayedBufferIdx;

-(void) optUADE_Led:(int)isOn;
-(void) optUADE_Norm:(int)isOn;
-(void) optUADE_PostFX:(int)isOn;
-(void) optUADE_Pan:(int)isOn;
-(void) optUADE_PanValue:(float_t)val;
-(void) optUADE_Head:(int)isOn;
-(void) optUADE_NTSC:(int)isOn;
-(void) optUADE_Gain:(int)isOn;
-(void) optUADE_PanValue:(float_t)val;
-(void) optUADE_GainValue:(float_t)val;



-(void) optVGMPLAY_MaxLoop:(unsigned int)val;
-(void) optVGMPLAY_YM2612emulator:(unsigned char)val;
-(void) optVGMPLAY_PreferedJTag:(bool)val;

-(void) optGEN_DefaultLength:(float_t)val;

-(void) optTIM_Poly:(int)val;
-(void) optTIM_Reverb:(int)val;
-(void) optTIM_Chorus:(int)val;
-(void) optTIM_LPFilter:(int)val;
-(void) optTIM_Resample:(int)val;
-(void) optTIM_Amplification:(int)val;

-(void) optSIDFilter:(int)onoff;
-(void) optSIDClock:(int)clockMode;
-(void) optSIDModel:(int)modelMode;

-(void) optSEXYPSF:(int)reverb interpol:(int)interpol;
-(void) optAOSDK:(int)reverb interpol:(int)interpol;
-(void) optADPLUG:(int)opltype;
-(void) optAOSDK_22KHZ:(int)value;
-(void) optAOSDK_DSFDSP:(int)value;
-(void) optAOSDK_DSFEmuRatio:(int)value;
-(void) optAOSDK_SSFDSP:(int)value;
-(void) optAOSDK_SSFEmuRatio:(int)value;
-(void) optDUMB_MastVol:(float)value;
-(void) optDUMB_Resampling:(int)value;

-(void) optGLOB_Panning:(int)onoff;
-(void) optGLOB_PanningValue:(float)value;


-(void) optVGMSTREAM_MaxLoop:(double)val;
-(void) optVGMSTREAM_ForceLoop:(unsigned int)val;
-(void) optVGMSTREAM_ResampleQuality:(unsigned int)val;
-(void) optVGMSTREAM_preferJapTag:(bool)val;

-(void) optLAZYUSF_ResampleQuality:(unsigned int)val;

-(void) optGME_Fade:(int)fade;
-(void) optGME_IgnoreSilence:(int)ignoreSilence;
-(void) optGME_Ratio:(float)ratio isEnabled:(bool)enabled;
-(void) optGME_EQ:(double)treble bass:(double)bass;
-(void) optGME_FX:(int)enabled surround:(int)surround echo:(double)echo stereo:(double)stereo;

-(void) optGSF_UpdateParam;

-(void) setLoopInf:(int)val;

-(BOOL) isMultiSongs;
-(BOOL) isArchive;
-(int) getArcEntriesCnt;
-(int) getArcIndex;

-(NSString*) getArcEntryTitle:(int)arc_index;

-(NSString*) getSubTitle:(int)subsong;

-(int) getSongLength;
-(int) getGlobalLength;
-(int) getCurrentTime;
-(int) shouldUpdateInfos;
-(void) setInfosUpdated;
-(int) getChannelVolume:(int)channel;

//loaders
-(int) mmp_gsfLoad:(NSString*)filePath;
-(int) mmp_mdxpdxLoad:(NSString*)filePath;
-(int) mmp_sc68Load:(NSString*)filePath;
-(int) mmp_stsoundLoad:(NSString*)filePath;
-(int) mmp_sidplayLoad:(NSString*)filePath;
-(int) mmp_hvlLoad:(NSString*)filePath;
-(int) mmp_uadeLoad:(NSString*)filePath;
-(int) mmp_sexypsfLoad:(NSString*)filePath;
-(int) mmp_aosdkLoad:(NSString*)filePath;
-(int) mmp_openmptLoad:(NSString*)filePath;
-(int) mmp_timidityLoad:(NSString*)filePath;
-(int) mmp_vgmstreamLoad:(NSString*)filePath extension:(NSString*)extension;
-(int) mmp_mpg123Load:(NSString*)filePath extension:(NSString*)extension;
-(int) mmp_lazyusfLoad:(NSString*)filePath;
-(int) mmp_2sfLoad:(NSString*)filePath;
-(int) mmp_snsfLoad:(NSString*)filePath;
-(int) mmp_vgmplayLoad:(NSString*)filePath;
-(int) mmp_pmdminiLoad:(NSString*)filePath;
-(int) mmp_dumbLoad:(NSString*)filePath;
-(int) mmp_asapLoad:(NSString*)filePath;
-(int) mmp_adplugLoad:(NSString*)filePath;
-(int) mmp_gmeLoad:(NSString*)filePath;
-(int) mmp_2sfLoad:(NSString*)filePath;

-(NSString*) getFullFilePath:(NSString *)_filePath;

@end
