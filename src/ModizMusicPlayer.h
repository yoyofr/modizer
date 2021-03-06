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

//#import "AppDelegate_Phone.h"


//libopenmpt
#import "../libs/libopenmpt/openmpt-trunk/libopenmpt/libopenmpt.h"
#import "../libs/libopenmpt/openmpt-trunk/libopenmpt/libopenmpt_ext.h"
#import "../libs/libopenmpt/openmpt-trunk/libopenmpt/libopenmpt_stream_callbacks_file.h"
#import "../libs/libopenmpt/openmpt-trunk/include/modplug/include/libmodplug/modplug.h"
//MODPLUG
//#import "modplug.h"

//2SF

#import "../libs/libxsf/src/in_xsf_framework/XSFPlayer.h"
#import "../libs/libxsf/src/in_ncsf/XSFPlayer_NCSF.h"
#import "XSFPlayer_2SF.h"
#import "XSFConfig.h"

//SNSF
#import "snsf_drvimpl.h"


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

#import "pmdmini.h"
//#import "pmdwin/pmdwinimport.h"

extern "C" {
    
#import <dirent.h>
// MDX
#import "mdx.h"
// PMD


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
    MMP_UADE,
    MMP_TIMIDITY,
    MMP_VGMSTREAM,
    MMP_VGMPLAY,
    MMP_GME,
    MMP_ASAP,
    MMP_GSF,
    MMP_MDXPDX,
    MMP_SC68,
    MMP_STSOUND,
    MMP_HVL,
    MMP_2SF,
    MMP_SNSF,
    MMP_HC,
    MMP_V2M,
    MMP_S98,
    MMP_KSS
};

@class DetailViewControllerIphone;

@interface ModizMusicPlayer : NSObject {
    DetailViewControllerIphone *detailViewControllerIphone;
	//General infos
	int mod_subsongs;
	int mod_currentsub,mod_minsub,mod_maxsub;
	unsigned int mPlayType;
	int mp_datasize,numChannels,numVoicesChannels;
    int mLoopMode; //0:off, 1:infinite
    int64_t mCurrentSamples,mTgtSamples,mFadeSamplesStart;
    NSString *mod_currentfile;
    NSString *mod_currentext;
    NSString *artist,*album;

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
    //KSS
    int kssOptLoopNb;
	//GME
	int optGMEFadeOut;
    int optGMEIgnoreSilence;
    float optGMERatio;
    bool optGMEEnableRatio;
    char optGMEFXon;
    char optGMEFXSurround;
    double optGMEFXEcho;
    double optGMEFXStereo;
    double optGMEEQTreble;
    double optGMEEQBass;
    //GSF
    char optGSFsoundLowPass;
    char optGSFsoundEcho;
    char optGSFsoundQuality;//1:44Khz, 2:22Khz, 4:11Khz
    char optGSFsoundInterpolation;
	//Openmpt
    int mPatternDataAvail;
    openmpt_module_ext *ompt_mod;
    openmpt_module_ext_interface_interactive *ompt_mod_interactive;
    ModPlugNote** ompt_patterns;
    int optOMPT_SamplingVal;
    int optOMPT_StereoSeparationVal;
    int optOMPT_MasterVol;
    
    //XMP
    int optXMP_InterpolationValue;
    int optXMP_StereoSeparationVal;
    int optXMP_AmpValue;
    int optXMP_MasterVol;
    int optXMP_DSP;
    int optXMP_Flags;

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
    //VGMPLAY stuff
    unsigned int optVGMPLAY_maxloop;
    unsigned char optVGMPLAY_ym2612emulator;
    unsigned char optVGMPLAY_ymf262emulator;
    bool optVGMPLAY_preferJapTag;
    //
	//Modplug stuff
	
	int *genRow,*genPattern,*playRow,*playPattern;//,*playOffset,*genOffset;
    unsigned char *genVolData,*playVolData;
	char *mp_data;
	int numPatterns,numSamples,numInstr;
	//
	AudioQueueRef mAudioQueue;
	AudioQueueBufferRef *mBuffers;
	int mQueueIsBeingStopped;
};
@property NSString *artist,*album;
@property int mod_subsongs,mod_currentsub,mod_minsub,mod_maxsub,mLoopMode;
@property int64_t mCurrentSamples,mTgtSamples,mFadeSamplesStart;
@property int optForceMono;
@property unsigned int mPlayType;
@property int mp_datasize,mPatternDataAvail;
@property NSString *mod_currentfile,*mod_currentext;
//Adplug stuff
@property CPlayer	*adPlugPlayer;
@property CAdPlugDatabase *adplugDB;
@property CEmuopl *opl;
@property int opl_towrite,mADPLUGopltype;
//GME stuff
@property Music_Emu* gme_emu;
//SID
//VGMPLAY
@property unsigned int optVGMPLAY_maxloop;
@property bool optVGMPLAY_preferJapTag;
//Modplug stuff
@property openmpt_module_ext *ompt_mod;
@property ModPlug_Settings mp_settings;
@property char *mp_data;
@property int *genRow,*genPattern,*playRow,*playPattern;//,*playOffset,*genOffset;
@property unsigned char *genVolData,*playVolData;
@property float mVolume;
@property int numChannels,numPatterns,numSamples,numInstr,numVoicesChannels;
@property char m_voicesDataAvail;
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
//SID
//
@property (nonatomic, retain) DetailViewControllerIphone *detailViewControllerIphone;

-(id) initMusicPlayer;

-(void) generateSoundThread;

-(NSString*) getModMessage;
-(NSString*) getModFileTitle;
-(NSString*) getModName;
-(NSString*) getModType;
-(NSString*) getPlayerName;

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

-(BOOL) isPlaying;

-(void) Seek:(int)seek_time;
-(int) isSeeking;
-(bool) isPaused;
-(BOOL) isEndReached;
-(void) Stop;
-(void) Pause:(BOOL)paused;
-(void) Play;
-(void) PlaySeek:(int)startPos subsong:(int)subsong;

-(void) playPrevSub;
-(void) playNextSub;
-(void) playGoToSub:(int)sub_index;

-(void) selectPrevArcEntry;
-(void) selectNextArcEntry;
-(void) selectArcEntry:(int)arc_index;

-(int) getSongLengthfromMD5:(int)track_nb;
-(void) setSongLengthfromMD5:(int)track_nb songlength:(int)slength;

-(int) isAcceptedFile:(NSString*)_filePath;
-(int) LoadModule:(NSString*)_filePath defaultMODPLAYER:(int)defaultMODPLAYER defaultSAPPLAYER:(int)defaultSAPPLAYER defaultVGMPLAYER:(int)defaultVGMPLAYER archiveMode:(int)archiveMode archiveIndex:(int)archiveIndex singleSubMode:(int)singleSubMode singleArcMode:(int)singleArcMode detailVC:(DetailViewControllerIphone*)detailVC isRestarting:(bool)isRestarting shuffle:(bool)shuffle;

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

-(void) optOMPT_MasterVol:(float)mstVol;
-(void) optOMPT_Sampling:(int) mode;
-(void) optOMPT_StereoSeparation:(float) val;



-(void) optVGMPLAY_MaxLoop:(unsigned int)val;
-(void) optVGMPLAY_YM2612emulator:(unsigned char)val;
-(void) optVGMPLAY_YMF262emulator:(unsigned char)val;
-(void) optVGMPLAY_PreferedJTag:(bool)val;

-(void) optGEN_DefaultLength:(float_t)val;

-(void) optTIM_Poly:(int)val;
-(void) optTIM_Reverb:(int)val;
-(void) optTIM_Chorus:(int)val;
-(void) optTIM_LPFilter:(int)val;
-(void) optTIM_Resample:(int)val;
-(void) optTIM_Amplification:(int)val;

-(void) optSIDSecondSIDAddress:(int)addr;
-(void) optSIDThirdSIDAddress:(int)addr;
-(void) optSIDForceLoop:(int)forceLoop;
-(void) optSIDFilter:(int)onoff;
-(void) optSIDClock:(int)clockMode;
-(void) optSIDModel:(int)modelMode;
-(void) optSIDEngine:(char)engine;
-(void) optSIDInterpolation:(char)mode;

-(void) optADPLUG:(int)opltype;

-(void) optGLOB_Panning:(int)onoff;
-(void) optGLOB_PanningValue:(float)value;


-(void) optVGMSTREAM_MaxLoop:(int)val;
-(void) optVGMSTREAM_Fadeouttime:(int)val;
-(void) optVGMSTREAM_ForceLoop:(unsigned int)val;
-(void) optVGMSTREAM_ResampleQuality:(unsigned int)val;
-(void) optVGMSTREAM_preferJapTag:(bool)val;

-(void) optHC_ResampleQuality:(unsigned int)val;

-(void) optGME_Fade:(int)fade;
-(void) optGME_IgnoreSilence:(int)ignoreSilence;
-(void) optGME_Ratio:(float)ratio isEnabled:(bool)enabled;
-(void) optGME_EQ:(double)treble bass:(double)bass;
-(void) optGME_FX:(int)enabled surround:(int)surround echo:(double)echo stereo:(double)stereo;

-(void) optGSF_UpdateParam;

-(void) optXMP_SetInterpolation:(int) mode;
-(void) optXMP_SetStereoSeparation:(int) value;
-(void) optXMP_SetMasterVol:(int) value;
-(void) optXMP_SetAmp:(int) value;
-(void) optXMP_SetDSP:(bool) value;
-(void) optXMP_SetFLAGS:(bool) value;

-(void) setLoopInf:(int)val;


//loaders
-(int) mmp_gsfLoad:(NSString*)filePath;
-(int) mmp_mdxpdxLoad:(NSString*)filePath;
-(int) mmp_sc68Load:(NSString*)filePath;
-(int) mmp_stsoundLoad:(NSString*)filePath;
-(int) mmp_sidplayLoad:(NSString*)filePath;
-(int) mmp_hvlLoad:(NSString*)filePath;
-(int) mmp_uadeLoad:(NSString*)filePath;
-(int) mmp_openmptLoad:(NSString*)filePath;
-(int) mmp_timidityLoad:(NSString*)filePath;
-(int) mmp_vgmstreamLoad:(NSString*)filePath extension:(NSString*)extension;
-(int) MMP_HCLoad:(NSString*)filePath;
-(void) MMP_HCClose;
-(int) mmp_2sfLoad:(NSString*)filePath;
-(int) mmp_snsfLoad:(NSString*)filePath;
-(int) mmp_vgmplayLoad:(NSString*)filePath;
-(int) mmp_pmdminiLoad:(NSString*)filePath;
-(int) mmp_asapLoad:(NSString*)filePath;
-(int) mmp_adplugLoad:(NSString*)filePath;
-(int) mmp_gmeLoad:(NSString*)filePath;
-(int) mmp_2sfLoad:(NSString*)filePath;

-(ModPlugNote*) ompt_getPattern:(int)pattern numrows:(unsigned int*)numrows;

-(NSString*) getFullFilePath:(NSString *)_filePath;

-(int) getSystemsNb;
-(NSString*) getSystemsName:(int)systemIdx;

-(int) getSystemForVoice:(int)voiceIdx;
-(void) setSystemm_voicesStatus:(int)systemIdx active:(bool)active;
-(int) getSystemm_voicesStatus:(int)systemIdx;
-(bool) isVoicesMutingSupported;
-(NSString*) getVoicesName:(unsigned int)channel;
-(bool) getm_voicesStatus:(unsigned int)channel;
-(void) setm_voicesStatus:(bool)active index:(unsigned int)channel;
-(void) vgmStream_ChangeToSub:(int) subsong;

-(NSString *) getCurrentFilenameMD5;

@end
