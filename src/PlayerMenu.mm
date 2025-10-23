//
//  PlayerMenu.mm
//  modizer
//
//  Created by Yohann Magnien David on 10/10/2025.
//

#include "PlayerMenu.h"
#include "SettingsGenViewController.h"
#include "TextureUtils.h"

#define MENU_BACKGROUND_ALPHA 0.7f

#define pMenu_getBundledResFilePath(name) [[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:name] UTF8String]

#ifndef min
#define min(a,b) ((a)<(b)?a:b)
#endif
#ifndef max
#define max(a,b) ((a)>(b)?a:b)
#endif


extern float font_size[4];
extern ImFont *font_menu[4];
int font_idx;
extern volatile t_settings settings[MAX_SETTINGS];
extern bool _pmPresetHasChanged;

extern void pmSoftReinit();

namespace PMenu {

ImVec4 colorBtnTextInactive=ImVec4(0.25f,0.2f,0.5f,0.9f);
ImVec4 colorBtnTextActive=ImVec4(0.5f,0.4f,1.0f,0.9f);

static bool pMenu_isInitialized=false;

enum PMenu_Menu_List {
    MENU_ROOT=0,
    MENU_PROJECTM,
    MENU_OSCILLO,
    MENU_PIANOROLL,
    MENU_PIANO3D,
    MENU_MIDIPATTERN,
    MENU_MODPATTERN,
    MENU_2DSPECTRUM,
    MENU_3DSPECTRUM,
    MENU_3DLANDSCAPE,
    
    MENU_INDEX_MAX
};
#define FXPROJECTM_IDX (MENU_PROJECTM-1)
#define FXOSCILLO_IDX (MENU_OSCILLO-1)
#define FXPIANOROLL_IDX (MENU_PIANOROLL-1)
#define FXPIANO3D_IDX (MENU_PIANO3D-1)
#define FXMIDI_IDX (MENU_MIDIPATTERN-1)
#define FXMODPATTERN_IDX (MENU_MODPATTERN-1)
#define FX2DSPECTRUM_IDX (MENU_2DSPECTRUM-1)
#define FX3DSPECTRUM_IDX (MENU_3DSPECTRUM-1)
#define FX3DLANDSCAPE_IDX (MENU_3DLANDSCAPE-1)


static GLuint txtShineFx;
static int menuCpt[16];

static float global_FXAlpha;

static GLuint txtMenuHandle[16];
const char *menuRootLabel[16]={
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,"Show FPS","@slider_alpha","Fullscreen",
    "Close FX\nwindow","All FX off","Go to\nsettings","Exit Menu"
};
static GLuint txtMenuProjectMHandle[16];
const char *menuProjectMLabel[16]={
    "Off",NULL,"Show name\nand\ndisappear","Show name",
    "Blend presets","Lock preset","Random order","Sequential\norder",
    "Default\npresets","Custom\npresets",NULL,NULL,
    NULL,"Go to\nsettings","Back","Exit Menu"
};
static GLuint txtMenuOscilloHandle[16];
const char *menuOscilloLabel[16]={
    "Off",NULL,NULL,NULL,
    NULL,"Labels","Grid",NULL,
    "Size 10","Size 16","Size 24",NULL,
    NULL,"Go to\nsettings","Back","Exit Menu"
};
char *menuOscilloDynLabel[16];
static GLuint txtMenu2DSpectrumHandle[16];
const char *menu2DSpectrumLabel[16]={
    "Off",NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,"Go to\nsettings","Back","Exit Menu"
};
static GLuint txtMenu3DSpectrumHandle[16];
const char *menu3DSpectrumLabel[16]={
    "Off",NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,"Go to\nsettings","Back","Exit Menu"
};
static GLuint txtMenu3DLandscapeHandle[16];
const char *menu3DLandscapeLabel[16]={
    "Off",NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,"Go to\nsettings","Back","Exit Menu"
};
static GLuint txtMenuPiano3DHandle[16];
const char *menuPiano3DLabel[16]={
    "Off",NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,"Go to\nsettings","Back","Exit Menu"
};
static GLuint txtMenuPianoRollHandle[16];
const char *menuPianoRollLabel[16]={
    "Off",NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    "Voices\nlabels","Octaves\nlabels",NULL,NULL,
    NULL,"Go to\nsettings","Back","Exit Menu"
};
static GLuint txtMenuMidiHandle[16];
const char *menuMidiLabel[16]={
    "Off",NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,"Go to\nsettings","Back","Exit Menu"
};
static GLuint txtMenuModPatternHandle[16];
const char *menuModPatternLabel[16]={
    "Off",NULL,NULL,NULL,
    "Volume\nbars",NULL,NULL,"Fixed bar",
    "Size 10","Size 16","Size 24","Size 32",
    NULL,"Go to\nsettings","Back","Exit Menu"
};
char *menuModPatternDynLabel[16];


struct {
    int menu_idx;
} pMenu_state;


void playerRootMenuInitRightItemsTexture() {
    txtMenuHandle[FXOSCILLO_IDX]=txtMenuOscilloHandle[max(settings[OSCILLO_FXMODE].detail.mdz_switch.switch_value&15,1)];
    txtMenuHandle[FX2DSPECTRUM_IDX]=txtMenu2DSpectrumHandle[max(settings[GLOB_FXSpectrum].detail.mdz_switch.switch_value&15,1)];
    txtMenuHandle[FX3DSPECTRUM_IDX]=txtMenu3DSpectrumHandle[max(settings[GLOB_FX3DSpectrum].detail.mdz_switch.switch_value&15,1)];
    txtMenuHandle[FX3DLANDSCAPE_IDX]=txtMenu3DLandscapeHandle[max(settings[GLOB_FX3DLandscape].detail.mdz_switch.switch_value&15,1)];
    
    txtMenuHandle[FXPIANOROLL_IDX]=txtMenuPianoRollHandle[max(settings[GLOB_FXPianoRoll].detail.mdz_switch.switch_value&15,1)];
    
    txtMenuHandle[FXPIANO3D_IDX]=txtMenuPiano3DHandle[max((settings[GLOB_FXPiano3D].detail.mdz_switch.switch_value)&15,1)];
    
    txtMenuHandle[FXMIDI_IDX]=txtMenuMidiHandle[max(settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_value&15,1)];
    txtMenuHandle[FXMODPATTERN_IDX]=txtMenuModPatternHandle[max(settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value&15,1)];
}

int playerGetActivatedCells(int menu_idx) {
    int active_idx=0;
    
    if (menu_idx==MENU_ROOT) {
        if (settings[OSCILLO_FXMODE].detail.mdz_switch.switch_value) active_idx|=1<<FXOSCILLO_IDX;
        if (settings[GLOB_FXSpectrum].detail.mdz_switch.switch_value) active_idx|=1<<FX2DSPECTRUM_IDX;
        if (settings[GLOB_FX3DSpectrum].detail.mdz_switch.switch_value) active_idx|=1<<FX3DSPECTRUM_IDX;
        if (settings[GLOB_FX3DLandscape].detail.mdz_switch.switch_value) active_idx|=1<<FX3DLANDSCAPE_IDX;
        if (settings[GLOB_FXPiano3D].detail.mdz_switch.switch_value) active_idx|=1<<FXPIANO3D_IDX;
        if (settings[GLOB_FXPianoRoll].detail.mdz_switch.switch_value) active_idx|=1<<FXPIANOROLL_IDX;
        if (settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_value) active_idx|=1<<FXMIDI_IDX;
        if (settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value) active_idx|=1<<FXMODPATTERN_IDX;
        if (settings[PROJECTM_FXONOFF].detail.mdz_switch.switch_value) active_idx|=1<<FXPROJECTM_IDX;
        if (settings[GLOB_FXSHOWFPS].detail.mdz_boolswitch.switch_value) active_idx|=1<<9;
        if (settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value) active_idx|=1<<11;
    } else if (menu_idx==MENU_OSCILLO) {
        if (settings[OSCILLO_FXMODE].detail.mdz_switch.switch_value==0) active_idx|=1<<0;
        if (settings[OSCILLO_FXMODE].detail.mdz_switch.switch_value==1) active_idx|=1<<1;
        if (settings[OSCILLO_FXMODE].detail.mdz_switch.switch_value==2) active_idx|=1<<2;
        if (settings[OSCILLO_FXMODE].detail.mdz_switch.switch_value==3) active_idx|=1<<3;
        if (settings[OSCILLO_FXMODE].detail.mdz_switch.switch_value==4) active_idx|=1<<4;
        if (settings[OSCILLO_ShowLabel].detail.mdz_boolswitch.switch_value==1) active_idx|=1<<5;
        if (settings[OSCILLO_ShowGrid].detail.mdz_boolswitch.switch_value==1) active_idx|=1<<6;
        if (settings[OSCILLO_LabelFontSize].detail.mdz_switch.switch_value==0) active_idx|=1<<8;
        if (settings[OSCILLO_LabelFontSize].detail.mdz_switch.switch_value==1) active_idx|=1<<9;
        if (settings[OSCILLO_LabelFontSize].detail.mdz_switch.switch_value==2) active_idx|=1<<10;
        
        snprintf(menuOscilloDynLabel[7],64,"Thickness:\n%s",settings[OSCILLO_LINE_Width].detail.mdz_switch.switch_labels[settings[OSCILLO_LINE_Width].detail.mdz_switch.switch_value]);
    } else if (menu_idx==MENU_2DSPECTRUM) {
        if (settings[GLOB_FXSpectrum].detail.mdz_switch.switch_value==0) active_idx|=1<<0;
        if (settings[GLOB_FXSpectrum].detail.mdz_switch.switch_value==1) active_idx|=1<<1;
        if (settings[GLOB_FXSpectrum].detail.mdz_switch.switch_value==2) active_idx|=1<<2;
    } else if (menu_idx==MENU_3DSPECTRUM) {
        if (settings[GLOB_FX3DSpectrum].detail.mdz_switch.switch_value==0) active_idx|=1<<0;
        if (settings[GLOB_FX3DSpectrum].detail.mdz_switch.switch_value==1) active_idx|=1<<1;
        if (settings[GLOB_FX3DSpectrum].detail.mdz_switch.switch_value==2) active_idx|=1<<2;
        if (settings[GLOB_FX3DSpectrum].detail.mdz_switch.switch_value==3) active_idx|=1<<3;
    } else if (menu_idx==MENU_3DLANDSCAPE) {
        if (settings[GLOB_FX3DLandscape].detail.mdz_switch.switch_value==0) active_idx|=1<<0;
        if (settings[GLOB_FX3DLandscape].detail.mdz_switch.switch_value==1) active_idx|=1<<1;
        if (settings[GLOB_FX3DLandscape].detail.mdz_switch.switch_value==2) active_idx|=1<<2;
        if (settings[GLOB_FX3DLandscape].detail.mdz_switch.switch_value==3) active_idx|=1<<3;
        if (settings[GLOB_FX3DLandscape].detail.mdz_switch.switch_value==4) active_idx|=1<<4;
        if (settings[GLOB_FX3DLandscape].detail.mdz_switch.switch_value==5) active_idx|=1<<5;
        if (settings[GLOB_FX3DLandscape].detail.mdz_switch.switch_value==6) active_idx|=1<<6;
        if (settings[GLOB_FX3DLandscape].detail.mdz_switch.switch_value==7) active_idx|=1<<7;
        if (settings[GLOB_FX3DLandscape].detail.mdz_switch.switch_value==8) active_idx|=1<<8;
    } else if (menu_idx==MENU_PIANO3D) {
        if (settings[GLOB_FXPiano3D].detail.mdz_switch.switch_value==0) active_idx|=1<<0;
        if (settings[GLOB_FXPiano3D].detail.mdz_switch.switch_value==1) active_idx|=1<<1;
        if (settings[GLOB_FXPiano3D].detail.mdz_switch.switch_value==2) active_idx|=1<<2;
        if (settings[GLOB_FXPiano3D].detail.mdz_switch.switch_value==3) active_idx|=1<<3;
        if (settings[GLOB_FXPiano3D].detail.mdz_switch.switch_value==4) active_idx|=1<<4;
    } else if (menu_idx==MENU_PIANOROLL) {
        if (settings[GLOB_FXPianoRoll].detail.mdz_switch.switch_value==0) active_idx|=1<<0;
        if (settings[GLOB_FXPianoRoll].detail.mdz_switch.switch_value==1) active_idx|=1<<1;
        if (settings[GLOB_FXPianoRoll].detail.mdz_switch.switch_value==2) active_idx|=1<<2;
        if (settings[GLOB_FXPianoRollVoicesLabels].detail.mdz_boolswitch.switch_value) active_idx|=1<<8;
        if (settings[GLOB_FXPianoRollOctavesLabels].detail.mdz_boolswitch.switch_value) active_idx|=1<<9;
    } else if (menu_idx==MENU_MIDIPATTERN) {
        if (settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_value==0) active_idx|=1<<0;
        if (settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_value==1) active_idx|=1<<1;
        if (settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_value==2) active_idx|=1<<2;
    } else if (menu_idx==MENU_MODPATTERN) {
        if (settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value==0) active_idx|=1<<0;
        if (settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value==1) active_idx|=1<<1;
        if (settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value==2) active_idx|=1<<2;
        if (settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value==3) active_idx|=1<<3;
        if (settings[GLOB_FXMODPattern_VolBar].detail.mdz_boolswitch.switch_value) active_idx|=1<<4;
        if (settings[GLOB_FXMODPattern_CurrentLineMode].detail.mdz_switch.switch_value==1) active_idx|=1<<7;
        if (settings[GLOB_FXMODPattern_FontSize].detail.mdz_switch.switch_value==0) active_idx|=1<<8;
        if (settings[GLOB_FXMODPattern_FontSize].detail.mdz_switch.switch_value==1) active_idx|=1<<9;
        if (settings[GLOB_FXMODPattern_FontSize].detail.mdz_switch.switch_value==2) active_idx|=1<<10;
        if (settings[GLOB_FXMODPattern_FontSize].detail.mdz_switch.switch_value==3) active_idx|=1<<11;
        
        snprintf(menuModPatternDynLabel[5],64,"Themes:\n%s",settings[GLOB_FXMODPattern_Theme].detail.mdz_switch.switch_labels[settings[GLOB_FXMODPattern_Theme].detail.mdz_switch.switch_value]);
        snprintf(menuModPatternDynLabel[12],64,"Font:\n%s",settings[GLOB_FXMODPattern_Font].detail.mdz_switch.switch_labels[settings[GLOB_FXMODPattern_Font].detail.mdz_switch.switch_value]);
    } else if (menu_idx==MENU_PROJECTM) {
        if (settings[PROJECTM_FXONOFF].detail.mdz_switch.switch_value) active_idx|=1<<1;
        else active_idx|=1<<0;
        if (settings[PROJECTM_ShowPresetLabel].detail.mdz_switch.switch_value==1) active_idx|=1<<2;
        else if (settings[PROJECTM_ShowPresetLabel].detail.mdz_switch.switch_value==2) active_idx|=1<<3;
        if (settings[PROJECTM_BlendPresets].detail.mdz_boolswitch.switch_value) active_idx|=1<<4;
        if (settings[PROJECTM_LockPreset].detail.mdz_boolswitch.switch_value) active_idx|=1<<5;
        if (settings[PROJECTM_AutoSwitchPresetsMode].detail.mdz_switch.switch_value) active_idx|=1<<7;
        else active_idx|=1<<6;
        if (settings[PROJECTM_BundledPresets].detail.mdz_boolswitch.switch_value) active_idx|=1<<8;
        if (settings[PROJECTM_CustomPresets].detail.mdz_boolswitch.switch_value) active_idx|=1<<9;
    }
    return active_idx;
}


//------------------------------------------------------
// playerMenuInit
//   initialize system & load required data
//------------------------------------------------------
void playerMenuInit() {
    pMenu_state.menu_idx=MENU_ROOT;
    
    memset(txtMenuHandle,0,sizeof(txtMenuHandle));
    memset(txtMenuProjectMHandle,0,sizeof(txtMenuProjectMHandle));
    memset(txtMenuOscilloHandle,0,sizeof(txtMenuOscilloHandle));
    memset(menuOscilloDynLabel,0,sizeof(menuOscilloDynLabel));
    memset(txtMenu2DSpectrumHandle,0,sizeof(txtMenu2DSpectrumHandle));
    memset(txtMenu3DSpectrumHandle,0,sizeof(txtMenu3DSpectrumHandle));
    memset(txtMenu3DLandscapeHandle,0,sizeof(txtMenu3DLandscapeHandle));
    memset(txtMenuPiano3DHandle,0,sizeof(txtMenuPiano3DHandle));
    memset(txtMenuPianoRollHandle,0,sizeof(txtMenuPianoRollHandle));
    memset(txtMenuMidiHandle,0,sizeof(txtMenuMidiHandle));
    memset(txtMenuModPatternHandle,0,sizeof(txtMenuModPatternHandle));
    memset(menuModPatternDynLabel,0,sizeof(menuModPatternDynLabel));
    
    menuOscilloDynLabel[7]=(char*)malloc(64);
    menuModPatternDynLabel[5]=(char*)malloc(64);
    menuModPatternDynLabel[12]=(char*)malloc(64);
    
    for (int i=0;i<16;i++) menuCpt[i]=rand();
    
    //ProjectM
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu13a_2x.png"), &(txtMenuHandle[FXPROJECTM_IDX]), NULL, NULL)) {
        MDZELog("Cannot load texture");
    }
    //Oscillo
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu5a_2x.png"), &(txtMenuHandle[FXOSCILLO_IDX]), NULL, NULL)) {
        MDZELog("Cannot load texture");
    }
    //Piano roll
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu11e_2x.png"), &(txtMenuHandle[FXPIANOROLL_IDX]), NULL, NULL)) {
        MDZELog("Cannot load texture");
    }
    //Piano 3D
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu11a_2x.png"), &(txtMenuHandle[FXPIANO3D_IDX]), NULL, NULL)) {
        MDZELog("Cannot load texture");
    }
    //Note scrollers
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu8a_2x.png"), &(txtMenuHandle[FXMIDI_IDX]), NULL, NULL)) {
        MDZELog("Cannot load texture");
    }
    //mod patterns
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu7a_2x.png"), &(txtMenuHandle[FXMODPATTERN_IDX]), NULL, NULL)) {
        MDZELog("Cannot load texture");
    }
    //Spectrum 2D
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu4a_2x.png"), &(txtMenuHandle[FX2DSPECTRUM_IDX]), NULL, NULL)) {
        MDZELog("Cannot load texture");
    }
    //Spectrum 3D objects
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu12a_2x.png"), &(txtMenuHandle[FX3DSPECTRUM_IDX]), NULL, NULL)) {
        MDZELog("Cannot load texture");
    }
    //Spectrum 3D landscape
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu2a_2x.png"), &(txtMenuHandle[FX3DLANDSCAPE_IDX]), NULL, NULL)) {
        MDZELog("Cannot load texture");
    }
    
    //Oscilloscopes
    txtMenuOscilloHandle[1]=txtMenuHandle[FXOSCILLO_IDX];
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu5b_2x.png"), &(txtMenuOscilloHandle[2]), NULL, NULL)) {
        MDZELog("Cannot load texture");
    }
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu5c_2x.png"), &(txtMenuOscilloHandle[3]), NULL, NULL)) {
        MDZELog("Cannot load texture");
    }
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu5d_2x.png"), &(txtMenuOscilloHandle[4]), NULL, NULL)) {
        MDZELog("Cannot load texture");
    }
    
    //Spectrum 2D
    txtMenu2DSpectrumHandle[1]=txtMenuHandle[FX2DSPECTRUM_IDX];
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu4b_2x.png"), &(txtMenu2DSpectrumHandle[2]), NULL, NULL)) {
        MDZELog("Cannot load texture");
    }
    
    
    //Spectrum 3D objects
    txtMenu3DSpectrumHandle[1]=txtMenuHandle[FX3DSPECTRUM_IDX];
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu12b_2x.png"), &(txtMenu3DSpectrumHandle[2]), NULL, NULL)) {
        MDZELog("Cannot load texture");
    }
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu12c_2x.png"), &(txtMenu3DSpectrumHandle[3]), NULL, NULL)) {
        MDZELog("Cannot load texture");
    }
    
    //Spectrum 3D landscape
    txtMenu3DLandscapeHandle[1]=txtMenuHandle[FX3DLANDSCAPE_IDX];
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu2b_2x.png"), &(txtMenu3DLandscapeHandle[2]), NULL, NULL)) {
        MDZELog("Cannot load texture");
    }
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu2c_2x.png"), &(txtMenu3DLandscapeHandle[3]), NULL, NULL)) {
        MDZELog("Cannot load texture");
    }
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu2d_2x.png"), &(txtMenu3DLandscapeHandle[4]), NULL, NULL)) {
        MDZELog("Cannot load texture");
    }
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu2e_2x.png"), &(txtMenu3DLandscapeHandle[5]), NULL, NULL)) {
        MDZELog("Cannot load texture");
    }
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu3a_2x.png"), &(txtMenu3DLandscapeHandle[6]), NULL, NULL)) {
        MDZELog("Cannot load texture");
    }
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu3b_2x.png"), &(txtMenu3DLandscapeHandle[7]), NULL, NULL)) {
        MDZELog("Cannot load texture");
    }
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu3c_2x.png"), &(txtMenu3DLandscapeHandle[8]), NULL, NULL)) {
        MDZELog("Cannot load texture");
    }
    
    //Piano Roll
    txtMenuPianoRollHandle[1]=txtMenuHandle[FXPIANOROLL_IDX];
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu11f_2x.png"), &(txtMenuPianoRollHandle[2]), NULL, NULL)) {
        MDZELog("Cannot load texture");
    }
    
    //Piano 3D
    txtMenuPiano3DHandle[1]=txtMenuHandle[FXPIANO3D_IDX];
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu11b_2x.png"), &(txtMenuPiano3DHandle[2]), NULL, NULL)) {
        MDZELog("Cannot load texture");
    }
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu11c_2x.png"), &(txtMenuPiano3DHandle[3]), NULL, NULL)) {
        MDZELog("Cannot load texture");
    }
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu11d_2x.png"), &(txtMenuPiano3DHandle[4]), NULL, NULL)) {
        MDZELog("Cannot load texture");
    }
    
    //Notes scrollers
    txtMenuMidiHandle[1]=txtMenuHandle[FXMIDI_IDX];
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu8b_2x.png"), &(txtMenuMidiHandle[2]), NULL, NULL)) {
        MDZELog("Cannot load texture");
    }
    
    //Mod patterns
    txtMenuModPatternHandle[1]=txtMenuHandle[FXMODPATTERN_IDX];
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu7b_2x.png"), &(txtMenuModPatternHandle[2]), NULL, NULL)) {
        MDZELog("Cannot load texture");
    }
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu7c_2x.png"), &(txtMenuModPatternHandle[3]), NULL, NULL)) {
        MDZELog("Cannot load texture");
    }
    
    //ProjectM
    txtMenuProjectMHandle[1]=txtMenuHandle[FXPROJECTM_IDX];
    
    
    //shine texture
    txtShineFx=0;
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"gloss.png"), &(txtShineFx), NULL, NULL)) {
        MDZELog("Cannot load texture");
    }
    
    pMenu_isInitialized=true;
}
//------------------------------------------------------
// playerMenuShutdown
//   deinit & release data
//------------------------------------------------------
void playerMenuShutdown() {
    pMenu_isInitialized=false;
}

//------------------------------------------------------
// playerMenuBack
//   go back one level, if possible
//------------------------------------------------------
void playerMenuBack() {
    if (pMenu_state.menu_idx>MENU_ROOT) {
        pMenu_state.menu_idx=MENU_ROOT;
    }
}

//------------------------------------------------------
// playerShowMenu
//   draw current menu, keep record of state
//   return -1 if it has been closed and all FX view should be hidden
//   return 0 if it has been closed
//   return 1 if it is to be kept open
//------------------------------------------------------
int playerShowMenu(float ww,float hh,float glScaleFactor,float fadelev) {
    static int cpt=0;
    if (!pMenu_isInitialized) return 0;
    int keepOpened=1;
    float menu_win_size=round(fmin(ww,hh)*glScaleFactor);
    ImVec2 menu_win_pos=ImVec2((ww*glScaleFactor-menu_win_size)/2,(hh*glScaleFactor-menu_win_size)/2);
    float cell_size=round(fmin(ww,hh)*glScaleFactor/4.4f);
    GLuint *current_txtMenuHandle;
    const char **currentMenuLabel;
    char **currentMenuDynLabel;
    
    cpt++;
    for (int i=0;i<16;i++) {
        menuCpt[i]++;
    }
    
    int font_idx=3;
    float idealFontSize=menu_win_size/64;
    for (int i=0;i<4;i++) {
        if ( (((idealFontSize-font_size[i])/idealFontSize)<0.2) || (font_size[i]>idealFontSize) ) {
            font_idx=i;
            break;
        }
    }
    
    
    // Global var mirroring
    global_FXAlpha=settings[GLOB_FXAlpha].detail.mdz_slider.slider_value*100;
    
    // Root window, full screen
    ImGui::SetNextWindowPos(ImVec2(0,0));
    ImGui::SetNextWindowSize(ImVec2(ww*glScaleFactor,hh*glScaleFactor));
    
    ImGui::PushStyleColor(ImGuiCol_WindowBg,ImVec4(0.0f,0.0f,0.0f,MENU_BACKGROUND_ALPHA));
    
    ImGui::Begin("Modizer root menu",0,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoScrollbar);
    ImGui::SetWindowFocus();
    ImGui::GetStyle().Alpha=fadelev;
    
    ImGui::SetNextWindowPos(menu_win_pos);
    ImGui::BeginChild("Modizer menu",ImVec2(menu_win_size,menu_win_size));
    static ImGuiTableFlags flagTable = /*ImGuiTableFlags_Borders|*/ImGuiTableFlags_NoBordersInBody|ImGuiTableFlags_SizingFixedSame|ImGuiTableFlags_NoHostExtendX|ImGuiTableFlags_PreciseWidths;
    
    if (font_menu[font_idx]) ImGui::PushFont(font_menu[font_idx]);
    else ImGui::PushFont(nullptr);//,18*menu_win_size/512);
    
    int activeFx=playerGetActivatedCells(pMenu_state.menu_idx);
    
    ImGuiStyle& style = ImGui::GetStyle();
    
    
    if (pMenu_state.menu_idx==MENU_ROOT) {
        //Select right current textures for root menu itemas, based on current settings
        playerRootMenuInitRightItemsTexture();
        if (ImGui::BeginTable("menu_root",4,flagTable)) {
            current_txtMenuHandle=txtMenuHandle;
            currentMenuLabel=menuRootLabel;
            for (int r=0;r<4;r++) {
                ImGui::TableNextRow(0,cell_size);
                for (int c=0;c<4;c++) {
                    bool isActive=activeFx&(1<<(r*4+c));
                    ImGui::TableSetColumnIndex(c);
                    bool ret=false;
                    ImVec2 uv0(0,0);ImVec2 uv1(1,1);ImVec4 bg_col(0,0,0,0.0f);ImVec4 tint_col(0.4,0.4,0.4,0.8f);
                    if (isActive) {//Active
                        tint_col.x=1.0;tint_col.y=1.0;tint_col.z=1.0;tint_col.w=1.0f;
                        if (current_txtMenuHandle[r*4+c]) ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0.0,0.0,0.0,0.0f));
                        else ImGui::PushStyleColor(ImGuiCol_Button,colorBtnTextActive);
                    } else { //Inactive
                        if (current_txtMenuHandle[r*4+c]) ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0.0,0.0,0.0,0.6f));
                        else ImGui::PushStyleColor(ImGuiCol_Button,colorBtnTextInactive);
                    }
                    ImVec2 cur_pos=ImGui::GetCursorPos();
                    if (current_txtMenuHandle[r*4+c]) { //Image Button
                        if (isActive) {
                            ImGui::SetNextItemAllowOverlap();
                            tint_col.x=1.0;tint_col.y=1.0;tint_col.z=1.0;tint_col.w=1.0f;
                            ImGui::PushID((r*4+c)*4+0);
                            ImGui::ImageButton("",(ImTextureID)(intptr_t)current_txtMenuHandle[r*4+c], ImVec2(cell_size, cell_size),uv0,uv1,bg_col,tint_col);
                            ImGui::PopID();
                            ImGui::SetCursorPos(cur_pos);
                            ImGui::PushID((r*4+c)*4+1);
                            ret=ImGui::ImageButton("",(ImTextureID)(intptr_t)txtShineFx,ImVec2(cell_size, cell_size),uv0,uv1,bg_col,tint_col);
                            ImGui::PopID();
                        } else {
                            ImGui::PushID((r*4+c)*4);
                            ret=ImGui::ImageButton("",(ImTextureID)(intptr_t)current_txtMenuHandle[r*4+c], ImVec2(cell_size, cell_size),uv0,uv1,bg_col,tint_col);
                            ImGui::PopID();
                        }
                    } else if (currentMenuLabel[r*4+c]) { //Text Button
                        ImGui::PushID((r*4+c)*4+0);
                        if (strcmp(currentMenuLabel[r*4+c],"@slider_alpha")==0) {
                                cur_pos=ImGui::GetCursorPos();
                                cur_pos.y+=(cell_size/4);
                                ImGui::SetCursorPos(cur_pos);
                                ImGui::LabelText("", "FX\nalpha");
                                cur_pos.x+=(cell_size-1.5*cell_size/3);
                                cur_pos.y-=(cell_size/4);
                                ImGui::SetCursorPos(cur_pos);
                                ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, cell_size/5);
                                ImGui::VSliderFloat("",ImVec2(cell_size/3,cell_size*4/4),  &global_FXAlpha, 30.0f, 100.0f,"%.0f%%");
                                ImGui::PopStyleVar();
                        } else {
                            if (isActive) {
                                ret=ImGui::Button(currentMenuLabel[r*4+c],ImVec2(cell_size, cell_size));
                            } else {
                                ret=ImGui::Button(currentMenuLabel[r*4+c],ImVec2(cell_size, cell_size));
                            }
                        }
                        ImGui::PopID();
                    }
                    ImGui::PopStyleColor();
                    if (ret) {
                        switch (c*16+r) {
                            case 0x00:
                                if (MENU_INDEX_MAX>=1) pMenu_state.menu_idx=1;
                                break;
                            case 0x10:
                                if (MENU_INDEX_MAX>=2) pMenu_state.menu_idx=2;
                                break;
                            case 0x20:
                                if (MENU_INDEX_MAX>=3) pMenu_state.menu_idx=3;
                                break;
                            case 0x30:
                                if (MENU_INDEX_MAX>=4) pMenu_state.menu_idx=4;
                                break;
                            case 0x01:
                                if (MENU_INDEX_MAX>=5) pMenu_state.menu_idx=5;
                                break;
                            case 0x11:
                                if (MENU_INDEX_MAX>=6) pMenu_state.menu_idx=6;
                                break;
                            case 0x21:
                                if (MENU_INDEX_MAX>=7) pMenu_state.menu_idx=7;
                                break;
                            case 0x31:
                                if (MENU_INDEX_MAX>=8) pMenu_state.menu_idx=8;
                                break;
                            case 0x02:
                                if (MENU_INDEX_MAX>=9) pMenu_state.menu_idx=9;
                                break;
                            case 0x12: //Show FPS
                                settings[GLOB_FXSHOWFPS].detail.mdz_boolswitch.switch_value=!settings[GLOB_FXSHOWFPS].detail.mdz_boolswitch.switch_value;
                                break;
                            case 0x22: //FX Alpha
                                break;
                            case 0x32: //Fullscreen switch
                                settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value=!(settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value);
                                break;
                            case 0x03://HIDE FX Screen
                                keepOpened=-1;
                                break;
                            case 0x13: //ALL FX Off
                                settings[GLOB_FX3DLandscape].detail.mdz_switch.switch_value=0;
                                settings[GLOB_FXPianoRoll].detail.mdz_switch.switch_value=0;
                                settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_value=0;
                                settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value=0;
                                settings[GLOB_FXSpectrum].detail.mdz_switch.switch_value=0;
                                settings[OSCILLO_FXMODE].detail.mdz_switch.switch_value=0;
                                settings[GLOB_FXPiano3D].detail.mdz_switch.switch_value=0;
                                settings[GLOB_FX3DSpectrum].detail.mdz_switch.switch_value=0;
                                settings[PROJECTM_FXONOFF].detail.mdz_switch.switch_value=0;
                                break;
                            case 0x23: //Go to settings - visu
                                keepOpened=2;
                                break;
                            case 0x33: //Exit
                                keepOpened=0;
                                break;
                        }
                    }
                }
            }
            ImGui::EndTable();
        }
    } else if (pMenu_state.menu_idx==MENU_OSCILLO) {
        if (ImGui::BeginTable("menu_oscillo",4,flagTable)) {
            current_txtMenuHandle=txtMenuOscilloHandle;
            currentMenuLabel=menuOscilloLabel;
            currentMenuDynLabel=menuOscilloDynLabel;
            for (int r=0;r<4;r++) {
                ImGui::TableNextRow(0,cell_size);
                for (int c=0;c<4;c++) {
                    bool isActive=activeFx&(1<<(r*4+c));
                    ImGui::TableSetColumnIndex(c);
                    
                    bool ret=false;
                    ImVec2 uv0(0,0);ImVec2 uv1(1,1);ImVec4 bg_col(0,0,0,0.0f);ImVec4 tint_col(0.4,0.4,0.4,0.8f);
                    //float padding_val=0;
                    if (isActive) {//Active
                        tint_col.x=1.0;tint_col.y=1.0;tint_col.z=1.0;tint_col.w=1.0f;
                        if (current_txtMenuHandle[r*4+c]) ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0.0,0.0,0.0,0.0f));
                        else ImGui::PushStyleColor(ImGuiCol_Button,colorBtnTextActive);
                    } else { //Inactive
                        if (current_txtMenuHandle[r*4+c]) ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0.0,0.0,0.0,0.4f));
                        else ImGui::PushStyleColor(ImGuiCol_Button,colorBtnTextInactive);
                    }
                    ImVec2 cur_pos=ImGui::GetCursorPos();
                    if (current_txtMenuHandle[r*4+c]) { //Image Button
                        if (isActive) {
                            ImGui::SetNextItemAllowOverlap();
                            tint_col.x=1.0;tint_col.y=1.0;tint_col.z=1.0;tint_col.w=1.0f;
                            ImGui::PushID((r*4+c)*4+0);
                            ImGui::ImageButton("",(ImTextureID)(intptr_t)current_txtMenuHandle[r*4+c], ImVec2(cell_size, cell_size),uv0,uv1,bg_col,tint_col);
                            ImGui::PopID();
                            tint_col.x=1.0;tint_col.y=1.0;tint_col.z=1.0;tint_col.w=1.0f;
                            ImGui::SetCursorPos(cur_pos);
                            ImGui::PushID((r*4+c)*4+1);
                            ret=ImGui::ImageButton("",(ImTextureID)(intptr_t)txtShineFx,ImVec2(cell_size, cell_size),uv0,uv1,bg_col,tint_col);
                            ImGui::PopID();
                        } else {
                            ImGui::PushID((r*4+c)*4);
                            ret=ImGui::ImageButton("",(ImTextureID)(intptr_t)current_txtMenuHandle[r*4+c], ImVec2(cell_size, cell_size),uv0,uv1,bg_col,tint_col);
                            ImGui::PopID();
                        }
                    } else if (currentMenuLabel[r*4+c]) { //Text Button
                        ImGui::PushID((r*4+c)*4+0);
                        if (isActive) ret=ImGui::Button(currentMenuLabel[r*4+c],ImVec2(cell_size, cell_size));
                        else ret=ImGui::Button(currentMenuLabel[r*4+c],ImVec2(cell_size, cell_size));
                        ImGui::PopID();
                    } else if (currentMenuDynLabel[r*4+c]) { //Text button
                        ImGui::PushID((r*4+c)*4+0);
                        if (isActive) ret=ImGui::Button(currentMenuDynLabel[r*4+c],ImVec2(cell_size, cell_size));
                        else ret=ImGui::Button(currentMenuDynLabel[r*4+c],ImVec2(cell_size, cell_size));
                        ImGui::PopID();
                    }
                    ImGui::PopStyleColor();
                    if (ret) {
                        switch (c*16+r) {
                            case 0x00: //Oscillo OFF
                                settings[OSCILLO_FXMODE].detail.mdz_switch.switch_value=0;
                                txtMenuHandle[FXOSCILLO_IDX]=current_txtMenuHandle[1];
                                break;
                            case 0x10: //Oscillo Green
                                settings[OSCILLO_FXMODE].detail.mdz_switch.switch_value=1;
                                txtMenuHandle[FXOSCILLO_IDX]=current_txtMenuHandle[1];
                                break;
                            case 0x20: //Oscillo Custom col
                                settings[OSCILLO_FXMODE].detail.mdz_switch.switch_value=2;
                                txtMenuHandle[FXOSCILLO_IDX]=current_txtMenuHandle[2];
                                break;
                            case 0x30: //Oscillo stereo green
                                settings[OSCILLO_FXMODE].detail.mdz_switch.switch_value=3;
                                txtMenuHandle[FXOSCILLO_IDX]=current_txtMenuHandle[3];
                                break;
                            case 0x01: //Oscillo stereo custom col
                                settings[OSCILLO_FXMODE].detail.mdz_switch.switch_value=4;
                                txtMenuHandle[FXOSCILLO_IDX]=current_txtMenuHandle[4];
                                break;
                            case 0x11: //Oscillo show labels
                                if (settings[OSCILLO_ShowLabel].detail.mdz_boolswitch.switch_value) settings[OSCILLO_ShowLabel].detail.mdz_boolswitch.switch_value=0;
                                else settings[OSCILLO_ShowLabel].detail.mdz_boolswitch.switch_value=1;
                                break;
                            case 0x21: //Oscillo show grid
                                if (settings[OSCILLO_ShowGrid].detail.mdz_boolswitch.switch_value) settings[OSCILLO_ShowGrid].detail.mdz_boolswitch.switch_value=0;
                                else settings[OSCILLO_ShowGrid].detail.mdz_boolswitch.switch_value=1;
                                
                                break;
                            case 0x31: //Oscillo line width
                                settings[OSCILLO_LINE_Width].detail.mdz_switch.switch_value=(settings[OSCILLO_LINE_Width].detail.mdz_switch.switch_value+1)%settings[OSCILLO_LINE_Width].detail.mdz_switch.switch_value_nb;
                                break;
                            case 0x02:
                                settings[OSCILLO_LabelFontSize].detail.mdz_switch.switch_value=0;
                                break;
                            case 0x12:
                                settings[OSCILLO_LabelFontSize].detail.mdz_switch.switch_value=1;
                                break;
                            case 0x22:
                                settings[OSCILLO_LabelFontSize].detail.mdz_switch.switch_value=2;
                                break;
                            case 0x32:break;
                            case 0x03:break;
                            case 0x13: //Go to settings - oscillo
                                keepOpened=3;
                                break;
                            case 0x23: //Back to main menu
                                pMenu_state.menu_idx=MENU_ROOT;
                                break;
                            case 0x33: //Exit menu
                                keepOpened=0;
                                break;
                        }
                    }
                }
            }
            ImGui::EndTable();
        }
    } else if (pMenu_state.menu_idx==MENU_2DSPECTRUM) {
        if (ImGui::BeginTable("menu_2dspectrum",4,flagTable)) {
            current_txtMenuHandle=txtMenu2DSpectrumHandle;
            currentMenuLabel=menu2DSpectrumLabel;
            
            for (int r=0;r<4;r++) {
                ImGui::TableNextRow(0,cell_size);
                for (int c=0;c<4;c++) {
                    bool isActive=activeFx&(1<<(r*4+c));
                    ImGui::TableSetColumnIndex(c);
                    
                    bool ret=false;
                    ImVec2 uv0(0,0);ImVec2 uv1(1,1);ImVec4 bg_col(0,0,0,0.0f);ImVec4 tint_col(0.4,0.4,0.4,0.8f);
                    if (isActive) {//Active
                        tint_col.x=1.0;tint_col.y=1.0;tint_col.z=1.0;tint_col.w=1.0f;
                        if (current_txtMenuHandle[r*4+c]) ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0.0,0.0,0.0,0.0f));
                        else ImGui::PushStyleColor(ImGuiCol_Button,colorBtnTextActive);
                    } else { //Inactive
                        if (current_txtMenuHandle[r*4+c]) ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0.0,0.0,0.0,0.4f));
                        else ImGui::PushStyleColor(ImGuiCol_Button,colorBtnTextInactive);
                    }
                    ImVec2 cur_pos=ImGui::GetCursorPos();
                    if (current_txtMenuHandle[r*4+c]) { //Image Button
                        if (isActive) {
                            ImGui::SetNextItemAllowOverlap();
                            tint_col.x=1.0;tint_col.y=1.0;tint_col.z=1.0;tint_col.w=1.0f;
                            ImGui::PushID((r*4+c)*4+0);
                            ImGui::ImageButton("",(ImTextureID)(intptr_t)current_txtMenuHandle[r*4+c], ImVec2(cell_size, cell_size),uv0,uv1,bg_col,tint_col);
                            ImGui::PopID();
                            tint_col.x=1.0;tint_col.y=1.0;tint_col.z=1.0;tint_col.w=1.0f;
                            ImGui::SetCursorPos(cur_pos);
                            ImGui::PushID((r*4+c)*4+1);
                            ret=ImGui::ImageButton("",(ImTextureID)(intptr_t)txtShineFx,ImVec2(cell_size, cell_size),uv0,uv1,bg_col,tint_col);
                            ImGui::PopID();
                        } else {
                            ImGui::PushID((r*4+c)*4);
                            ret=ImGui::ImageButton("",(ImTextureID)(intptr_t)current_txtMenuHandle[r*4+c], ImVec2(cell_size, cell_size),uv0,uv1,bg_col,tint_col);
                            ImGui::PopID();
                        }
                    } else if (currentMenuLabel[r*4+c]) { //Text Button
                        ImGui::PushID((r*4+c)*4+0);
                        if (isActive) {
                            ret=ImGui::Button(currentMenuLabel[r*4+c],ImVec2(cell_size, cell_size));
                        } else {
                            ret=ImGui::Button(currentMenuLabel[r*4+c],ImVec2(cell_size, cell_size));
                        }
                        ImGui::PopID();
                    }
                    ImGui::PopStyleColor();
                    if (ret) {
                        switch (c*16+r) {
                            case 0x00: //2dSpectrum OFF
                                settings[GLOB_FXSpectrum].detail.mdz_switch.switch_value=0;
                                txtMenuHandle[FX2DSPECTRUM_IDX]=current_txtMenuHandle[1];
                                break;
                            case 0x10: //2DSpectrum ON
                                settings[GLOB_FXSpectrum].detail.mdz_switch.switch_value=1;
                                txtMenuHandle[FX2DSPECTRUM_IDX]=current_txtMenuHandle[1];
                                break;
                            case 0x20: //Show preset's name
                                settings[GLOB_FXSpectrum].detail.mdz_switch.switch_value=2;
                                txtMenuHandle[FX2DSPECTRUM_IDX]=current_txtMenuHandle[2];
                                break;
                            case 0x30:break;
                            case 0x01:break;
                            case 0x11:break;
                            case 0x21:break;
                            case 0x31:break;
                            case 0x02:break;
                            case 0x12:break;
                            case 0x22:break;
                            case 0x32:break;
                            case 0x03:break;
                            case 0x13: //Go to settings - visu
                                keepOpened=2;
                                break;
                            case 0x23: //Back to main menu
                                pMenu_state.menu_idx=MENU_ROOT;
                                break;
                            case 0x33: //Exit menu
                                keepOpened=0;
                                break;
                        }
                    }
                }
            }
            ImGui::EndTable();
        }
    } else if (pMenu_state.menu_idx==MENU_3DSPECTRUM) {
        if (ImGui::BeginTable("menu_3dspectrum",4,flagTable)) {
            current_txtMenuHandle=txtMenu3DSpectrumHandle;
            currentMenuLabel=menu3DSpectrumLabel;
            
            for (int r=0;r<4;r++) {
                ImGui::TableNextRow(0,cell_size);
                for (int c=0;c<4;c++) {
                    bool isActive=activeFx&(1<<(r*4+c));
                    ImGui::TableSetColumnIndex(c);
                    
                    bool ret=false;
                    ImVec2 uv0(0,0);ImVec2 uv1(1,1);ImVec4 bg_col(0,0,0,0.0f);ImVec4 tint_col(0.4,0.4,0.4,0.8f);
                    //float padding_val=0;
                    if (isActive) {//Active
                        tint_col.x=1.0;tint_col.y=1.0;tint_col.z=1.0;tint_col.w=1.0f;
                        if (current_txtMenuHandle[r*4+c]) ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0.0,0.0,0.0,0.0f));
                        else ImGui::PushStyleColor(ImGuiCol_Button,colorBtnTextActive);
                    } else { //Inactive
                        if (current_txtMenuHandle[r*4+c]) ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0.0,0.0,0.0,0.4f));
                        else ImGui::PushStyleColor(ImGuiCol_Button,colorBtnTextInactive);
                    }
                    ImVec2 cur_pos=ImGui::GetCursorPos();
                    if (current_txtMenuHandle[r*4+c]) { //Image Button
                        if (isActive) {
                            ImGui::SetNextItemAllowOverlap();
                            tint_col.x=1.0;tint_col.y=1.0;tint_col.z=1.0;tint_col.w=1.0f;
                            ImGui::PushID((r*4+c)*4+0);
                            ImGui::ImageButton("",(ImTextureID)(intptr_t)current_txtMenuHandle[r*4+c], ImVec2(cell_size, cell_size),uv0,uv1,bg_col,tint_col);
                            ImGui::PopID();
                            tint_col.x=1.0;tint_col.y=1.0;tint_col.z=1.0;tint_col.w=1.0f;
                            ImGui::SetCursorPos(cur_pos);
                            ImGui::PushID((r*4+c)*4+1);
                            ret=ImGui::ImageButton("",(ImTextureID)(intptr_t)txtShineFx,ImVec2(cell_size, cell_size),uv0,uv1,bg_col,tint_col);
                            ImGui::PopID();
                        } else {
                            ImGui::PushID((r*4+c)*4);
                            ret=ImGui::ImageButton("",(ImTextureID)(intptr_t)current_txtMenuHandle[r*4+c], ImVec2(cell_size, cell_size),uv0,uv1,bg_col,tint_col);
                            ImGui::PopID();
                        }
                    } else if (currentMenuLabel[r*4+c]) { //Text Button
                        ImGui::PushID((r*4+c)*4+0);
                        if (isActive) {
                            ret=ImGui::Button(currentMenuLabel[r*4+c],ImVec2(cell_size, cell_size));
                        } else {
                            ret=ImGui::Button(currentMenuLabel[r*4+c],ImVec2(cell_size, cell_size));
                        }
                        ImGui::PopID();
                    }
                    ImGui::PopStyleColor();
                    if (ret) {
                        switch (c*16+r) {
                            case 0x00: //3dSpectrum OFF
                                settings[GLOB_FX3DSpectrum].detail.mdz_switch.switch_value=0;
                                txtMenuHandle[FX3DSPECTRUM_IDX]=current_txtMenuHandle[1];
                                break;
                            case 0x10: //3DSpectrum ON-mode 1
                                settings[GLOB_FX3DSpectrum].detail.mdz_switch.switch_value=1;
                                txtMenuHandle[FX3DSPECTRUM_IDX]=current_txtMenuHandle[1];
                                break;
                            case 0x20: //3DSpectrum ON-mode 2
                                settings[GLOB_FX3DSpectrum].detail.mdz_switch.switch_value=2;
                                txtMenuHandle[FX3DSPECTRUM_IDX]=current_txtMenuHandle[2];
                                break;
                            case 0x30: //3DSpectrum ON-mode 3
                                settings[GLOB_FX3DSpectrum].detail.mdz_switch.switch_value=3;
                                txtMenuHandle[FX3DSPECTRUM_IDX]=current_txtMenuHandle[3];
                                break;
                            case 0x01:break;
                            case 0x11:break;
                            case 0x21:break;
                            case 0x31:break;
                            case 0x02:break;
                            case 0x12:break;
                            case 0x22:break;
                            case 0x32:break;
                            case 0x03:break;
                            case 0x13: //Go to settings - visu
                                keepOpened=2;
                                break;
                            case 0x23: //Back to main menu
                                pMenu_state.menu_idx=MENU_ROOT;
                                break;
                            case 0x33: //Exit menu
                                keepOpened=0;
                                break;
                        }
                    }
                }
            }
            ImGui::EndTable();
        }
    } else if (pMenu_state.menu_idx==MENU_3DLANDSCAPE) {
        if (ImGui::BeginTable("menu_3dlandscape",4,flagTable)) {
            current_txtMenuHandle=txtMenu3DLandscapeHandle;
            currentMenuLabel=menu3DLandscapeLabel;
            
            for (int r=0;r<4;r++) {
                ImGui::TableNextRow(0,cell_size);
                for (int c=0;c<4;c++) {
                    bool isActive=activeFx&(1<<(r*4+c));
                    ImGui::TableSetColumnIndex(c);
                    
                    bool ret=false;
                    ImVec2 uv0(0,0);ImVec2 uv1(1,1);ImVec4 bg_col(0,0,0,0.0f);ImVec4 tint_col(0.4,0.4,0.4,0.8f);
                    //float padding_val=0;
                    if (isActive) {//Active
                        tint_col.x=1.0;tint_col.y=1.0;tint_col.z=1.0;tint_col.w=1.0f;
                        if (current_txtMenuHandle[r*4+c]) ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0.0,0.0,0.0,0.0f));
                        else ImGui::PushStyleColor(ImGuiCol_Button,colorBtnTextActive);
                    } else { //Inactive
                        if (current_txtMenuHandle[r*4+c]) ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0.0,0.0,0.0,0.4f));
                        else ImGui::PushStyleColor(ImGuiCol_Button,colorBtnTextInactive);
                    }
                    ImVec2 cur_pos=ImGui::GetCursorPos();
                    if (current_txtMenuHandle[r*4+c]) { //Image Button
                        if (isActive) {
                            ImGui::SetNextItemAllowOverlap();
                            tint_col.x=1.0;tint_col.y=1.0;tint_col.z=1.0;tint_col.w=1.0f;
                            ImGui::PushID((r*4+c)*4+0);
                            ImGui::ImageButton("",(ImTextureID)(intptr_t)current_txtMenuHandle[r*4+c], ImVec2(cell_size, cell_size),uv0,uv1,bg_col,tint_col);
                            ImGui::PopID();
                            tint_col.x=1.0;tint_col.y=1.0;tint_col.z=1.0;tint_col.w=1.0f;
                            //bg_col.w=0.0f;
                            ImGui::SetCursorPos(cur_pos);
                            ImGui::PushID((r*4+c)*4+1);
                            ret=ImGui::ImageButton("",(ImTextureID)(intptr_t)txtShineFx,ImVec2(cell_size, cell_size),uv0,uv1,bg_col,tint_col);
                            ImGui::PopID();
                        } else {
                            ImGui::PushID((r*4+c)*4);
                            ret=ImGui::ImageButton("",(ImTextureID)(intptr_t)current_txtMenuHandle[r*4+c], ImVec2(cell_size, cell_size),uv0,uv1,bg_col,tint_col);
                            ImGui::PopID();
                        }
                    } else if (currentMenuLabel[r*4+c]) { //Text Button
                        ImGui::PushID((r*4+c)*4+0);
                        if (isActive) {
                            ret=ImGui::Button(currentMenuLabel[r*4+c],ImVec2(cell_size, cell_size));
                        } else {
                            ret=ImGui::Button(currentMenuLabel[r*4+c],ImVec2(cell_size, cell_size));
                        }
                        ImGui::PopID();
                    }
                    ImGui::PopStyleColor();
                    if (ret) {
                        switch (c*16+r) {
                            case 0x00: //3DLandscape off
                                settings[GLOB_FX3DLandscape].detail.mdz_switch.switch_value=0;
                                txtMenuHandle[FX3DLANDSCAPE_IDX]=current_txtMenuHandle[1];
                                break;
                            case 0x10: //3DLandscape 1
                                settings[GLOB_FX3DLandscape].detail.mdz_switch.switch_value=1;
                                txtMenuHandle[FX3DLANDSCAPE_IDX]=current_txtMenuHandle[1];
                                break;
                            case 0x20: //3DLandscape 2
                                settings[GLOB_FX3DLandscape].detail.mdz_switch.switch_value=2;
                                txtMenuHandle[FX3DLANDSCAPE_IDX]=current_txtMenuHandle[2];
                                break;
                            case 0x30: //3DLandscape 3
                                settings[GLOB_FX3DLandscape].detail.mdz_switch.switch_value=3;
                                txtMenuHandle[FX3DLANDSCAPE_IDX]=current_txtMenuHandle[3];
                                break;
                            case 0x01: //3DLandscape 4
                                settings[GLOB_FX3DLandscape].detail.mdz_switch.switch_value=4;
                                txtMenuHandle[FX3DLANDSCAPE_IDX]=current_txtMenuHandle[4];
                                break;
                            case 0x11: //3DLandscape 5
                                settings[GLOB_FX3DLandscape].detail.mdz_switch.switch_value=5;
                                txtMenuHandle[FX3DLANDSCAPE_IDX]=current_txtMenuHandle[5];
                                break;
                            case 0x21: //3DLandscape 6
                                settings[GLOB_FX3DLandscape].detail.mdz_switch.switch_value=6;
                                txtMenuHandle[FX3DLANDSCAPE_IDX]=current_txtMenuHandle[6];
                                break;
                            case 0x31: //3DLandscape 7
                                settings[GLOB_FX3DLandscape].detail.mdz_switch.switch_value=7;
                                txtMenuHandle[FX3DLANDSCAPE_IDX]=current_txtMenuHandle[7];
                                break;
                            case 0x02: //3DLandscape 8
                                settings[GLOB_FX3DLandscape].detail.mdz_switch.switch_value=8;
                                txtMenuHandle[FX3DLANDSCAPE_IDX]=current_txtMenuHandle[8];
                                break;
                            case 0x12:break;
                            case 0x22:break;
                            case 0x32:break;
                            case 0x03:break;
                            case 0x13: //Go to settings - visu
                                keepOpened=2;
                                break;
                            case 0x23: //Back to main menu
                                pMenu_state.menu_idx=MENU_ROOT;
                                break;
                            case 0x33: //Exit menu
                                keepOpened=0;
                                break;
                        }
                    }
                }
            }
            ImGui::EndTable();
        }
    } else if (pMenu_state.menu_idx==MENU_PIANOROLL) {
        if (ImGui::BeginTable("menu_pianoroll",4,flagTable)) {
            current_txtMenuHandle=txtMenuPianoRollHandle;
            currentMenuLabel=menuPianoRollLabel;
            for (int r=0;r<4;r++) {
                ImGui::TableNextRow(0,cell_size);
                for (int c=0;c<4;c++) {
                    bool isActive=activeFx&(1<<(r*4+c));
                    ImGui::TableSetColumnIndex(c);
                    bool ret=false;
                    ImVec2 uv0(0,0);ImVec2 uv1(1,1);ImVec4 bg_col(0,0,0,0.0f);ImVec4 tint_col(0.4,0.4,0.4,0.8f);
                    if (isActive) {//Active
                        tint_col.x=1.0;tint_col.y=1.0;tint_col.z=1.0;tint_col.w=1.0f;
                        if (current_txtMenuHandle[r*4+c]) ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0.0,0.0,0.0,0.0f));
                        else ImGui::PushStyleColor(ImGuiCol_Button,colorBtnTextActive);
                    } else { //Inactive
                        if (current_txtMenuHandle[r*4+c]) ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0.0,0.0,0.0,0.4f));
                        else ImGui::PushStyleColor(ImGuiCol_Button,colorBtnTextInactive);
                    }
                    ImVec2 cur_pos=ImGui::GetCursorPos();
                    if (current_txtMenuHandle[r*4+c]) { //Image Button
                        if (isActive) {
                            ImGui::SetNextItemAllowOverlap();
                            tint_col.x=1.0;tint_col.y=1.0;tint_col.z=1.0;tint_col.w=1.0f;
                            ImGui::PushID((r*4+c)*4+0);
                            ImGui::ImageButton("",(ImTextureID)(intptr_t)current_txtMenuHandle[r*4+c], ImVec2(cell_size, cell_size),uv0,uv1,bg_col,tint_col);
                            ImGui::PopID();
                            tint_col.x=1.0;tint_col.y=1.0;tint_col.z=1.0;tint_col.w=1.0f;
                            ImGui::SetCursorPos(cur_pos);
                            ImGui::PushID((r*4+c)*4+1);
                            ret=ImGui::ImageButton("",(ImTextureID)(intptr_t)txtShineFx,ImVec2(cell_size, cell_size),uv0,uv1,bg_col,tint_col);
                            ImGui::PopID();
                        } else {
                            ImGui::PushID((r*4+c)*4);
                            ret=ImGui::ImageButton("",(ImTextureID)(intptr_t)current_txtMenuHandle[r*4+c], ImVec2(cell_size, cell_size),uv0,uv1,bg_col,tint_col);
                            ImGui::PopID();
                        }
                    } else if (currentMenuLabel[r*4+c]) { //Text Button
                        ImGui::PushID((r*4+c)*4+0);
                        if (isActive) ret=ImGui::Button(currentMenuLabel[r*4+c],ImVec2(cell_size, cell_size));
                        else ret=ImGui::Button(currentMenuLabel[r*4+c],ImVec2(cell_size, cell_size));
                        ImGui::PopID();
                    }
                    ImGui::PopStyleColor();
                    if (ret) {
                        switch (c*16+r) {
                            case 0x00:
                                settings[GLOB_FXPianoRoll].detail.mdz_switch.switch_value=0;
                                txtMenuHandle[FXPIANOROLL_IDX]=current_txtMenuHandle[1];
                                break;
                            case 0x10:
                                settings[GLOB_FXPianoRoll].detail.mdz_switch.switch_value=1;
                                txtMenuHandle[FXPIANOROLL_IDX]=current_txtMenuHandle[1];
                                break;
                            case 0x20:
                                settings[GLOB_FXPianoRoll].detail.mdz_switch.switch_value=2;
                                txtMenuHandle[FXPIANOROLL_IDX]=current_txtMenuHandle[2];
                                break;
                            case 0x30:break;
                            case 0x01:break;
                            case 0x11:break;
                            case 0x21:break;
                            case 0x31:break;
                            case 0x02: // Voices labels
                                settings[GLOB_FXPianoRollVoicesLabels].detail.mdz_boolswitch.switch_value=!settings[GLOB_FXPianoRollVoicesLabels].detail.mdz_boolswitch.switch_value;
                                break;
                            case 0x12: // Octaves labels
                                settings[GLOB_FXPianoRollOctavesLabels].detail.mdz_boolswitch.switch_value=!settings[GLOB_FXPianoRollOctavesLabels].detail.mdz_boolswitch.switch_value;
                                break;
                            case 0x22:break;
                            case 0x32:break;
                            case 0x03:break;
                            case 0x13: //Go to settings - visu
                                keepOpened=2;
                                break;
                            case 0x23: //Back to main menu
                                pMenu_state.menu_idx=MENU_ROOT;
                                break;
                            case 0x33: //Exit menu
                                keepOpened=0;
                                break;
                        }
                    }
                }
            }
            ImGui::EndTable();
        }
    } else if (pMenu_state.menu_idx==MENU_PIANO3D) {
        if (ImGui::BeginTable("menu_piano3d",4,flagTable)) {
            current_txtMenuHandle=txtMenuPiano3DHandle;
            currentMenuLabel=menuPiano3DLabel;
            for (int r=0;r<4;r++) {
                ImGui::TableNextRow(0,cell_size);
                for (int c=0;c<4;c++) {
                    bool isActive=activeFx&(1<<(r*4+c));
                    ImGui::TableSetColumnIndex(c);
                    bool ret=false;
                    ImVec2 uv0(0,0);ImVec2 uv1(1,1);ImVec4 bg_col(0,0,0,0.0f);ImVec4 tint_col(0.4,0.4,0.4,0.8f);
                    if (isActive) {//Active
                        tint_col.x=1.0;tint_col.y=1.0;tint_col.z=1.0;tint_col.w=1.0f;
                        if (current_txtMenuHandle[r*4+c]) ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0.0,0.0,0.0,0.0f));
                        else ImGui::PushStyleColor(ImGuiCol_Button,colorBtnTextActive);
                    } else { //Inactive
                        if (current_txtMenuHandle[r*4+c]) ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0.0,0.0,0.0,0.4f));
                        else ImGui::PushStyleColor(ImGuiCol_Button,colorBtnTextInactive);
                    }
                    ImVec2 cur_pos=ImGui::GetCursorPos();
                    if (current_txtMenuHandle[r*4+c]) { //Image Button
                        if (isActive) {
                            ImGui::SetNextItemAllowOverlap();
                            tint_col.x=1.0;tint_col.y=1.0;tint_col.z=1.0;tint_col.w=1.0f;
                            ImGui::PushID((r*4+c)*4+0);
                            ImGui::ImageButton("",(ImTextureID)(intptr_t)current_txtMenuHandle[r*4+c], ImVec2(cell_size, cell_size),uv0,uv1,bg_col,tint_col);
                            ImGui::PopID();
                            tint_col.x=1.0;tint_col.y=1.0;tint_col.z=1.0;tint_col.w=1.0f;
                            ImGui::SetCursorPos(cur_pos);
                            ImGui::PushID((r*4+c)*4+1);
                            ret=ImGui::ImageButton("",(ImTextureID)(intptr_t)txtShineFx,ImVec2(cell_size, cell_size),uv0,uv1,bg_col,tint_col);
                            ImGui::PopID();
                        } else {
                            ImGui::PushID((r*4+c)*4);
                            ret=ImGui::ImageButton("",(ImTextureID)(intptr_t)current_txtMenuHandle[r*4+c], ImVec2(cell_size, cell_size),uv0,uv1,bg_col,tint_col);
                            ImGui::PopID();
                        }
                    } else if (currentMenuLabel[r*4+c]) { //Text Button
                        ImGui::PushID((r*4+c)*4+0);
                        if (isActive) ret=ImGui::Button(currentMenuLabel[r*4+c],ImVec2(cell_size, cell_size));
                        else ret=ImGui::Button(currentMenuLabel[r*4+c],ImVec2(cell_size, cell_size));
                        ImGui::PopID();
                    }
                    ImGui::PopStyleColor();
                    if (ret) {
                        switch (c*16+r) {
                            case 0x00:
                                settings[GLOB_FXPiano3D].detail.mdz_switch.switch_value=0;
                                txtMenuHandle[FXPIANO3D_IDX]=current_txtMenuHandle[1];
                                break;
                            case 0x10:
                                settings[GLOB_FXPiano3D].detail.mdz_switch.switch_value=1;
                                txtMenuHandle[FXPIANO3D_IDX]=current_txtMenuHandle[1];
                                break;
                            case 0x20:
                                settings[GLOB_FXPiano3D].detail.mdz_switch.switch_value=2;
                                txtMenuHandle[FXPIANO3D_IDX]=current_txtMenuHandle[2];
                                break;
                            case 0x30:
                                settings[GLOB_FXPiano3D].detail.mdz_switch.switch_value=3;
                                txtMenuHandle[FXPIANO3D_IDX]=current_txtMenuHandle[3];
                                break;
                            case 0x01:
                                settings[GLOB_FXPiano3D].detail.mdz_switch.switch_value=4;
                                txtMenuHandle[FXPIANO3D_IDX]=current_txtMenuHandle[4];
                                break;
                            case 0x11:break;
                            case 0x21:break;
                            case 0x31:break;
                            case 0x02:break;
                            case 0x12:break;
                            case 0x22:break;
                            case 0x32:break;
                            case 0x03:break;
                            case 0x13: //Go to settings - visu
                                keepOpened=2;
                                break;
                            case 0x23: //Back to main menu
                                pMenu_state.menu_idx=MENU_ROOT;
                                break;
                            case 0x33: //Exit menu
                                keepOpened=0;
                                break;
                        }
                    }
                }
            }
            ImGui::EndTable();
        }
    } else if (pMenu_state.menu_idx==MENU_MIDIPATTERN) {
        if (ImGui::BeginTable("menu_midipattern",4,flagTable)) {
            current_txtMenuHandle=txtMenuMidiHandle;
            currentMenuLabel=menuMidiLabel;
            for (int r=0;r<4;r++) {
                ImGui::TableNextRow(0,cell_size);
                for (int c=0;c<4;c++) {
                    bool isActive=activeFx&(1<<(r*4+c));
                    ImGui::TableSetColumnIndex(c);
                    bool ret=false;
                    ImVec2 uv0(0,0);ImVec2 uv1(1,1);ImVec4 bg_col(0,0,0,0.0f);ImVec4 tint_col(0.4,0.4,0.4,0.8f);
                    if (isActive) {//Active
                        tint_col.x=1.0;tint_col.y=1.0;tint_col.z=1.0;tint_col.w=1.0f;
                        if (current_txtMenuHandle[r*4+c]) ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0.0,0.0,0.0,0.0f));
                        else ImGui::PushStyleColor(ImGuiCol_Button,colorBtnTextActive);
                    } else { //Inactive
                        if (current_txtMenuHandle[r*4+c]) ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0.0,0.0,0.0,0.4f));
                        else ImGui::PushStyleColor(ImGuiCol_Button,colorBtnTextInactive);
                    }
                    ImVec2 cur_pos=ImGui::GetCursorPos();
                    if (current_txtMenuHandle[r*4+c]) { //Image Button
                        if (isActive) {
                            ImGui::SetNextItemAllowOverlap();
                            tint_col.x=1.0;tint_col.y=1.0;tint_col.z=1.0;tint_col.w=1.0f;
                            ImGui::PushID((r*4+c)*4+0);
                            ImGui::ImageButton("",(ImTextureID)(intptr_t)current_txtMenuHandle[r*4+c], ImVec2(cell_size, cell_size),uv0,uv1,bg_col,tint_col);
                            ImGui::PopID();
                            tint_col.x=1.0;tint_col.y=1.0;tint_col.z=1.0;tint_col.w=1.0f;
                            ImGui::SetCursorPos(cur_pos);
                            ImGui::PushID((r*4+c)*4+1);
                            ret=ImGui::ImageButton("",(ImTextureID)(intptr_t)txtShineFx,ImVec2(cell_size, cell_size),uv0,uv1,bg_col,tint_col);
                            ImGui::PopID();
                        } else {
                            ImGui::PushID((r*4+c)*4);
                            ret=ImGui::ImageButton("",(ImTextureID)(intptr_t)current_txtMenuHandle[r*4+c], ImVec2(cell_size, cell_size),uv0,uv1,bg_col,tint_col);
                            ImGui::PopID();
                        }
                    } else if (currentMenuLabel[r*4+c]) { //Text Button
                        ImGui::PushID((r*4+c)*4+0);
                        if (isActive) ret=ImGui::Button(currentMenuLabel[r*4+c],ImVec2(cell_size, cell_size));
                        else ret=ImGui::Button(currentMenuLabel[r*4+c],ImVec2(cell_size, cell_size));
                        ImGui::PopID();
                    }
                    ImGui::PopStyleColor();
                    if (ret) {
                        switch (c*16+r) {
                            case 0x00: //Off
                                settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_value=0;
                                txtMenuHandle[FXMIDI_IDX]=current_txtMenuHandle[1];
                                break;
                            case 0x10: //
                                settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_value=1;
                                txtMenuHandle[FXMIDI_IDX]=current_txtMenuHandle[1];
                                break;
                            case 0x20: //
                                settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_value=2;
                                txtMenuHandle[FXMIDI_IDX]=current_txtMenuHandle[2];
                                break;
                            case 0x30:break;
                            case 0x01:break;
                            case 0x11:break;
                            case 0x21:break;
                            case 0x31:break;
                            case 0x02:break;
                            case 0x12:break;
                            case 0x22:break;
                            case 0x32:break;
                            case 0x03:break;
                            case 0x13: //Go to settings - visu
                                keepOpened=2;
                                break;
                            case 0x23: //Back to main menu
                                pMenu_state.menu_idx=MENU_ROOT;
                                break;
                            case 0x33: //Exit menu
                                keepOpened=0;
                                break;
                        }
                    }
                }
            }
            ImGui::EndTable();
        }
    } else if (pMenu_state.menu_idx==MENU_MODPATTERN) {
        if (ImGui::BeginTable("menu_modpattern",4,flagTable)) {
            current_txtMenuHandle=txtMenuModPatternHandle;
            currentMenuLabel=menuModPatternLabel;
            currentMenuDynLabel=menuModPatternDynLabel;
            for (int r=0;r<4;r++) {
                ImGui::TableNextRow(0,cell_size);
                for (int c=0;c<4;c++) {
                    bool isActive=activeFx&(1<<(r*4+c));
                    ImGui::TableSetColumnIndex(c);
                    bool ret=false;
                    ImVec2 uv0(0,0);ImVec2 uv1(1,1);ImVec4 bg_col(0,0,0,0.0f);ImVec4 tint_col(0.4,0.4,0.4,0.8f);
                    if (isActive) {//Active
                        tint_col.x=1.0;tint_col.y=1.0;tint_col.z=1.0;tint_col.w=1.0f;
                        if (current_txtMenuHandle[r*4+c]) ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0.0,0.0,0.0,0.0f));
                        else ImGui::PushStyleColor(ImGuiCol_Button,colorBtnTextActive);
                    } else { //Inactive
                        if (current_txtMenuHandle[r*4+c]) ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0.0,0.0,0.0,0.4f));
                        else ImGui::PushStyleColor(ImGuiCol_Button,colorBtnTextInactive);
                    }
                    ImVec2 cur_pos=ImGui::GetCursorPos();
                    if (current_txtMenuHandle[r*4+c]) { //Image Button
                        if (isActive) {
                            ImGui::SetNextItemAllowOverlap();
                            tint_col.x=1.0;tint_col.y=1.0;tint_col.z=1.0;tint_col.w=1.0f;
                            ImGui::PushID((r*4+c)*4+0);
                            ImGui::ImageButton("",(ImTextureID)(intptr_t)current_txtMenuHandle[r*4+c], ImVec2(cell_size, cell_size),uv0,uv1,bg_col,tint_col);
                            ImGui::PopID();
                            tint_col.x=1.0;tint_col.y=1.0;tint_col.z=1.0;tint_col.w=1.0f;
                            ImGui::SetCursorPos(cur_pos);
                            ImGui::PushID((r*4+c)*4+1);
                            ret=ImGui::ImageButton("",(ImTextureID)(intptr_t)txtShineFx,ImVec2(cell_size, cell_size),uv0,uv1,bg_col,tint_col);
                            ImGui::PopID();
                        } else {
                            ImGui::PushID((r*4+c)*4);
                            ret=ImGui::ImageButton("",(ImTextureID)(intptr_t)current_txtMenuHandle[r*4+c], ImVec2(cell_size, cell_size),uv0,uv1,bg_col,tint_col);
                            ImGui::PopID();
                        }
                    } else if (currentMenuLabel[r*4+c]) { //Text Button
                        ImGui::PushID((r*4+c)*4+0);
                        if (isActive) ret=ImGui::Button(currentMenuLabel[r*4+c],ImVec2(cell_size, cell_size));
                        else ret=ImGui::Button(currentMenuLabel[r*4+c],ImVec2(cell_size, cell_size));
                        ImGui::PopID();
                    } else if (currentMenuDynLabel[r*4+c]) { //Text button
                        ImGui::PushID((r*4+c)*4+0);
                        if (isActive) ret=ImGui::Button(menuModPatternDynLabel[r*4+c],ImVec2(cell_size, cell_size));
                        else ret=ImGui::Button(menuModPatternDynLabel[r*4+c],ImVec2(cell_size, cell_size));
                        ImGui::PopID();
                    }
                    ImGui::PopStyleColor();
                    if (ret) {
                        switch (c*16+r) {
                            case 0x00: //off
                                settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value=0;
                                txtMenuHandle[FXMODPATTERN_IDX]=current_txtMenuHandle[1];
                                break;
                            case 0x10: //All info, no volume bar
                                settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value=1;
                                txtMenuHandle[FXMODPATTERN_IDX]=current_txtMenuHandle[1];
                                break;
                            case 0x20: //Medium info, no volume bar
                                settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value=2;
                                txtMenuHandle[FXMODPATTERN_IDX]=current_txtMenuHandle[2];
                                break;
                            case 0x30: //Min info, no volume bar
                                settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value=3;
                                txtMenuHandle[FXMODPATTERN_IDX]=current_txtMenuHandle[3];
                                break;
                            case 0x01: //Volume bars
                                settings[GLOB_FXMODPattern_VolBar].detail.mdz_boolswitch.switch_value=!settings[GLOB_FXMODPattern_VolBar].detail.mdz_boolswitch.switch_value;
                                break;
                            case 0x11: //Theme
                                settings[GLOB_FXMODPattern_Theme].detail.mdz_switch.switch_value++;
                                if (settings[GLOB_FXMODPattern_Theme].detail.mdz_switch.switch_value>=settings[GLOB_FXMODPattern_Theme].detail.mdz_switch.switch_value_nb) settings[GLOB_FXMODPattern_Theme].detail.mdz_switch.switch_value=0;
                                break;
                            case 0x21: //Min info, volume bars
                                break;
                            case 0x31: //Fixed bar for mod current line
                                if (settings[GLOB_FXMODPattern_CurrentLineMode].detail.mdz_switch.switch_value) settings[GLOB_FXMODPattern_CurrentLineMode].detail.mdz_switch.switch_value=0;
                                else settings[GLOB_FXMODPattern_CurrentLineMode].detail.mdz_switch.switch_value=1;
                                break;
                            case 0x02: //Font size 10
                                settings[GLOB_FXMODPattern_FontSize].detail.mdz_switch.switch_value=0;
                                break;
                            case 0x12: //Font size 16
                                settings[GLOB_FXMODPattern_FontSize].detail.mdz_switch.switch_value=1;
                                break;
                            case 0x22: //Font size 24
                                settings[GLOB_FXMODPattern_FontSize].detail.mdz_switch.switch_value=2;
                                break;
                            case 0x32: //Font size 32
                                settings[GLOB_FXMODPattern_FontSize].detail.mdz_switch.switch_value=3;
                                break;
                            case 0x03: //Current font
                                settings[GLOB_FXMODPattern_Font].detail.mdz_switch.switch_value=(settings[GLOB_FXMODPattern_Font].detail.mdz_switch.switch_value+1)%5;
                                break;
                            case 0x13: //Go to settings - visu
                                keepOpened=2;
                                break;
                            case 0x23: //Back to main menu
                                pMenu_state.menu_idx=MENU_ROOT;
                                break;
                            case 0x33: //Exit menu
                                keepOpened=0;
                                break;
                        }
                    }
                }
            }
            ImGui::EndTable();
        }
    } else if (pMenu_state.menu_idx==MENU_PROJECTM) {
        
        if (ImGui::BeginTable("menu_ProjectM",4,flagTable)) {
            current_txtMenuHandle=txtMenuProjectMHandle;
            currentMenuLabel=menuProjectMLabel;
                
                for (int r=0;r<4;r++) {
                    ImGui::TableNextRow(0,cell_size);
                    for (int c=0;c<4;c++) {
                        bool isActive=activeFx&(1<<(r*4+c));
                        ImGui::TableSetColumnIndex(c);
                        
                        bool ret=false;
                        ImVec2 uv0(0,0);ImVec2 uv1(1,1);ImVec4 bg_col(0,0,0,0.0f);ImVec4 tint_col(0.4,0.4,0.4,0.8f);
                        //float padding_val=0;
                        if (isActive) {//Active
                            tint_col.x=1.0;tint_col.y=1.0;tint_col.z=1.0;tint_col.w=1.0f;
                            if (current_txtMenuHandle[r*4+c]) ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0.0,0.0,0.0,0.0f));
                            else ImGui::PushStyleColor(ImGuiCol_Button,colorBtnTextActive);
                        } else { //Inactive
                            if (current_txtMenuHandle[r*4+c]) ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0.0,0.0,0.0,0.4f));
                            else ImGui::PushStyleColor(ImGuiCol_Button,colorBtnTextInactive);
                        }
                        ImVec2 cur_pos=ImGui::GetCursorPos();
                        if (current_txtMenuHandle[r*4+c]) { //Image Button
                            if (isActive) {
                                ImGui::SetNextItemAllowOverlap();
                                tint_col.x=1.0;tint_col.y=1.0;tint_col.z=1.0;tint_col.w=1.0f;
                                ImGui::PushID((r*4+c)*4+0);
                                ImGui::ImageButton("",(ImTextureID)(intptr_t)current_txtMenuHandle[r*4+c], ImVec2(cell_size, cell_size),uv0,uv1,bg_col,tint_col);
                                ImGui::PopID();
                                tint_col.x=1.0;tint_col.y=1.0;tint_col.z=1.0;tint_col.w=1.0f;
                                ImGui::SetCursorPos(cur_pos);
                                ImGui::PushID((r*4+c)*4+1);
                                ret=ImGui::ImageButton("",(ImTextureID)(intptr_t)txtShineFx,ImVec2(cell_size, cell_size),uv0,uv1,bg_col,tint_col);
                                ImGui::PopID();
                            } else {
                                ImGui::PushID((r*4+c)*4);
                                ret=ImGui::ImageButton("",(ImTextureID)(intptr_t)current_txtMenuHandle[r*4+c], ImVec2(cell_size, cell_size),uv0,uv1,bg_col,tint_col);
                                ImGui::PopID();
                            }
                        } else if (currentMenuLabel[r*4+c]) { //Text Button
                            ImGui::PushID((r*4+c)*4+0);
                            if (isActive) {
                                ret=ImGui::Button(currentMenuLabel[r*4+c],ImVec2(cell_size, cell_size));
                            } else {
                                ret=ImGui::Button(currentMenuLabel[r*4+c],ImVec2(cell_size, cell_size));
                            }
                            ImGui::PopID();
                        }
                        ImGui::PopStyleColor();
                    if (ret) {
                        switch (c*16+r) {
                            case 0x00: //MIKDROP OFF
                                settings[PROJECTM_FXONOFF].detail.mdz_switch.switch_value=0;
                                break;
                            case 0x10: //PROJECTM ON
                                settings[PROJECTM_FXONOFF].detail.mdz_switch.switch_value=1;
                                break;
                            case 0x20: //Show preset's name
                                if (settings[PROJECTM_ShowPresetLabel].detail.mdz_switch.switch_value==1) settings[PROJECTM_ShowPresetLabel].detail.mdz_switch.switch_value=0;
                                else settings[PROJECTM_ShowPresetLabel].detail.mdz_switch.switch_value=1;
                                pmSoftReinit();
                                break;
                            case 0x30:// Show temporarly preset's name
                                if (settings[PROJECTM_ShowPresetLabel].detail.mdz_switch.switch_value==2) settings[PROJECTM_ShowPresetLabel].detail.mdz_switch.switch_value=0;
                                else {
                                    settings[PROJECTM_ShowPresetLabel].detail.mdz_switch.switch_value=2;
                                    _pmPresetHasChanged=true; //Force a (re)display
                                }
                                pmSoftReinit();
                                break;
                            case 0x01:
                                if (settings[PROJECTM_BlendPresets].detail.mdz_boolswitch.switch_value) settings[PROJECTM_BlendPresets].detail.mdz_boolswitch.switch_value=0;
                                else settings[PROJECTM_BlendPresets].detail.mdz_boolswitch.switch_value=1;
                                pmSoftReinit();
                                break;
                            case 0x11:
                                if (settings[PROJECTM_LockPreset].detail.mdz_boolswitch.switch_value) settings[PROJECTM_LockPreset].detail.mdz_boolswitch.switch_value=0;
                                else settings[PROJECTM_LockPreset].detail.mdz_boolswitch.switch_value=1;
                                pmSoftReinit();
                                break;
                            case 0x21:
                                settings[PROJECTM_AutoSwitchPresetsMode].detail.mdz_switch.switch_value=0;
                                pmSoftReinit();
                                break;
                            case 0x31:
                                settings[PROJECTM_AutoSwitchPresetsMode].detail.mdz_switch.switch_value=1;
                                pmSoftReinit();
                                break;
                            case 0x02:
                                if (settings[PROJECTM_BundledPresets].detail.mdz_boolswitch.switch_value) settings[PROJECTM_BundledPresets].detail.mdz_boolswitch.switch_value=0;
                                else settings[PROJECTM_BundledPresets].detail.mdz_boolswitch.switch_value=1;
                                pmSoftReinit();
                                break;
                            case 0x12:
                                if (settings[PROJECTM_CustomPresets].detail.mdz_boolswitch.switch_value) settings[PROJECTM_CustomPresets].detail.mdz_boolswitch.switch_value=0;
                                else settings[PROJECTM_CustomPresets].detail.mdz_boolswitch.switch_value=1;
                                pmSoftReinit();
                                break;
                            case 0x22:break;
                            case 0x32:break;
                            case 0x03:break;
                            case 0x13: //Go to settings - mikdrop
                                keepOpened=4;
                                break;
                            case 0x23: //Back to main menu
                                pMenu_state.menu_idx=MENU_ROOT;
                                break;
                            case 0x33: //Exit menu
                                keepOpened=0;
                                break;
                        }
                    }
                }
            }
            ImGui::EndTable();
        }
    }
    ImGui::PopFont();
    ImGui::EndChild();
    ImGui::End();
    ImGui::PopStyleColor();
    
    //Global var mirroring
    settings[GLOB_FXAlpha].detail.mdz_slider.slider_value=global_FXAlpha/100;
    
    return keepOpened;
}

}
