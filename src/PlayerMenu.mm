//
//  PlayerMenu.mm
//  modizer
//
//  Created by Yohann Magnien David on 10/10/2025.
//

extern float varCheck[4];

#define PL_MIN_FONT_SIZE 14
#define PL_MIN_BROWSE_FONT_SIZE 32
#define PL_IDEALFONTSIZE_RATIO 26

#define PMENU_PMEXPLORE_FAV_FLAG 1
#define PMENU_PMEXPLORE_SEL_FLAG 2

#define PMENU_EXPLORER_MIN_STRING_LENGTH 32


#include "PlayerMenu.h"
#include "SettingsGenViewController.h"
#include "TextureUtils.h"
#include <string.h>
#include <stdlib.h>
#import "DirParser.h"

#include "MDZFontAwesome.h"
extern FileNode *pmBundledPresetsFileNode;
extern FileNode *pmCustomPresetsFileNode;
extern MDZPlaylist *_mdzPM_playlist;
extern MDZFavorites *_mdzPM_Favorites;

FileNode *pmCurrentFileNode;
NSString *pMenu_currentPM_entry;
char pmFileNodeFilter[64];
static float idealFontSize;
extern int mouseMoveInProgress;
float browserFontSize,browserFontWidth;

extern void updatePresetCustomDirStructure();

#define PM_BUNDLED_PLAYLIST 1
#define PM_CUSTOM_PLAYLIST 2
int pmCurrentPlaylistMode;

extern float glScaleFactor;

int pMenu_TreeNodeLines;

#define faicon(a) [[NSString stringWithFormat:@"%C", static_cast<unichar>(a)] UTF8String]
#define faicon_with_pre_suf(pre,a,suf) [[NSString stringWithFormat:@"%s%C%s",pre, static_cast<unichar>(a),suf] UTF8String]

int pMenu_fullscreenStatus;

#define MENU_BACKGROUND_ALPHA 0.7f

#define pMenu_getBundledResFilePath(name) [[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:name] UTF8String]

#ifndef min
#define min(a,b) ((a)<(b)?a:b)
#endif
#ifndef max
#define max(a,b) ((a)>(b)?a:b)
#endif


extern float mdz_font_size[4];
extern ImFont *font_menu;
extern ImFont *font_menu_icon;
extern ImFont  *font_tracker[FONT_TRACKER_NB];
int font_idx;
extern volatile t_settings settings[MAX_SETTINGS];
extern bool _pmPresetUpdateDisplayInfo;

extern void pmSoftReinit(bool forceReloadPlaylist);

namespace PMenu {

ImVec4 pMenu_browser_selectedLine = ImVec4(0.4f,0.3f,0.7f,0.7f);
ImVec4 pMenu_browser_partiallySelectedLine = ImVec4(0.3f,0.3f,0.5f,0.6f);

ImVec4 pMenu_browser_notSelectedLineText = ImVec4(0.5,0.5,0.5,1.0);
ImVec4 pMenu_browser_selectedLineText = ImVec4(0.9,0.9,0.9,1.0);//242.0/255.0,165.0/255.0,95.0/255.0,1.0);
ImVec4 pMenu_browser_partiallySelectedLineText = ImVec4(214.0/255.0,202.0/255.0,134.0/255.0,1.0);

ImVec4 pMenu_browser_notSelectedLineTextPlaying = ImVec4(0.5,0.2,0.5,1.0);
ImVec4 pMenu_browser_selectedLineTextPlaying = ImVec4(0.9,0.5,0.9,1.0);//242.0/255.0,165.0/255.0,95.0/255.0,1.0);
ImVec4 pMenu_browser_partiallySelectedLineTextPlaying = ImVec4(214.0/255.0,102.0/255.0,134.0/255.0,1.0);

ImVec4 colorBtnTextInactive=ImVec4(0.25f,0.2f,0.5f,0.9f);
ImVec4 colorBtnTextInactiveH=ImVec4(0.8f,0.7f,0.9f,0.9f);
ImVec4 colorBtnTextActive=ImVec4(0.5f,0.4f,1.0f,0.9f);
ImVec4 colorBtnTextActiveH=ImVec4(1.0f,0.8f,1.0f,0.9f);

ImVec4 pMenu_browser_isFav=ImVec4(0.4,0.1,0.2,0.8f);
ImVec4 pMenu_browser_isFavH=ImVec4(0.8,0.2,0.4,0.8f);
ImVec4 pMenu_browser_quickAccessButton=ImVec4(0.2,0.2,0.3,0.8f);
ImVec4 pMenu_browser_quickAccessButtonH=ImVec4(0.4,0.4,0.6,0.8f);

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
    MENU_ROOT_MORE,
    MENU_PROJECTM_EXPLORE,
    MENU_INDEX_MAX
};

float menu_scrollX[MENU_INDEX_MAX];
float menu_scrollY[MENU_INDEX_MAX];

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

static float global_FXAlpha,global_MODPatOpacity;

static GLuint txtMenuHandle[16];
int menuRootColNb=4;
const char *menuRootLabel[16]={
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    "Close FX\nwindow","All FX off",NULL,NULL
};
void *menuRootVar[16]={
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
};
unsigned short menuRootLabelFAIcon[16]={
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,FA_ARROW_CIRCLE_RIGHT,FA_ARROWS_ALT,
    NULL,NULL,FA_COGS,FA_WINDOW_CLOSE,
};

int menuMoreColNb=4;
static GLuint txtMenuMoreHandle[16];
const char *menuRootMoreLabel[16]={
    NULL,"@sliderFX\nalpha|30|100",NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
};
void *menuRootMoreVar[16]={
    NULL,&global_FXAlpha,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
};
unsigned short menuRootMoreLabelFAIcon[16]={
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,FA_ARROWS_ALT,
    NULL,FA_COGS,FA_ARROW_CIRCLE_LEFT,FA_WINDOW_CLOSE,
};
char *menuMenuMoreDynLabel[16];

int menuProjectMColNb=4;
static GLuint txtMenuProjectMHandle[16];
const char *menuProjectMLabel[16]={
    NULL,NULL,"Show name\ntemp.","Show name",
    "Bundled\npresets","Custom\npresets",NULL,"Blend presets",
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL
};
void *menuProjectMVar[16]={
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
};
unsigned short menuProjectMLabelFAIcon[16]={
    FA_POWER_OFF,NULL,NULL,NULL,
    NULL,NULL,FA_RANDOM,NULL,
    NULL,NULL,FA_LOCK,FA_ARROWS_ALT,
    NULL,FA_COGS,FA_ARROW_CIRCLE_LEFT,FA_WINDOW_CLOSE,
};
char *menuProjectMDynLabel[16];

int menuProjectMExploreColNb=7;
static GLuint txtMenuProjectMExploreHandle[7*2];
const char *menuProjectMExploreLabel[7*2]={
    "Clear\nall",      "Select\nall",         "Favorites",   "Expand",NULL,NULL,NULL,
    "Select\nlisted",    "Remove\nlisted", "Selected",           "Collapse",NULL,NULL,NULL,
};
void *menuProjectMExploreVar[7*2]={
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,
    
};
unsigned short menuProjectMExploreLabelFAIcon[7*2]={
    NULL,NULL,NULL,NULL,NULL,FA_REFRESH,FA_CHECK_CIRCLE,
    NULL,NULL,NULL,NULL,NULL,FA_ARROW_CIRCLE_LEFT,FA_WINDOW_CLOSE,
};

int menuOscilloColNb=4;
static GLuint txtMenuOscilloHandle[16];
const char *menuOscilloLabel[16]={
    NULL,NULL,NULL,NULL,
    NULL,"Labels","Grid",NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
};
void *menuOscilloVar[16]={
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
};
unsigned short menuOscilloLabelFAIcon[16]={
    FA_POWER_OFF,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,FA_ARROWS_ALT,
    NULL,FA_COGS,FA_ARROW_CIRCLE_LEFT,FA_WINDOW_CLOSE,
};
char *menuOscilloDynLabel[16];

int menu2DSpectrumColNb=4;
static GLuint txtMenu2DSpectrumHandle[16];
const char *menu2DSpectrumLabel[16]={
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,"Fullscreen",
    NULL,"Go to\nsettings","Back","Exit"
};
void *menu2DSpectrumVar[16]={
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
};
unsigned short menu2DSpectrumLabelFAIcon[16]={
    FA_POWER_OFF,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,FA_ARROWS_ALT,
    NULL,FA_COGS,FA_ARROW_CIRCLE_LEFT,FA_WINDOW_CLOSE,
};

int menu3DSpectrumColNb=4;
static GLuint txtMenu3DSpectrumHandle[16];
const char *menu3DSpectrumLabel[16]={
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
};
void *menu3DSpectrumVar[16]={
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
};
unsigned short menu3DSpectrumLabelFAIcon[16]={
    FA_POWER_OFF,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,FA_ARROWS_ALT,
    NULL,FA_COGS,FA_ARROW_CIRCLE_LEFT,FA_WINDOW_CLOSE,
};
char *menu3DSpectrumDynLabel[16];

int menu3DLandscapeColNb=4;
static GLuint txtMenu3DLandscapeHandle[16];
const char *menu3DLandscapeLabel[16]={
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
};
void *menu3DLandscapeVar[16]={
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
};
unsigned short menu3DLandscapeLabelFAIcon[16]={
    FA_POWER_OFF,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,FA_ARROWS_ALT,
    NULL,FA_COGS,FA_ARROW_CIRCLE_LEFT,FA_WINDOW_CLOSE,
};
char *menu3DLandscapeDynLabel[16];

int menuPiano3DColNb=4;
static GLuint txtMenuPiano3DHandle[16];
const char *menuPiano3DLabel[16]={
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
};
void *menuPiano3DVar[16]={
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
};
unsigned short menuPiano3DLabelFAIcon[16]={
    FA_POWER_OFF,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,FA_ARROWS_ALT,
    NULL,FA_COGS,FA_ARROW_CIRCLE_LEFT,FA_WINDOW_CLOSE,
};

int menuPianoRollColNb=4;
static GLuint txtMenuPianoRollHandle[16];
const char *menuPianoRollLabel[16]={
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    "Voices\nlabels","Octaves\nlabels",NULL,NULL,
    NULL,NULL,NULL,NULL,
};
void *menuPianoRollVar[16]={
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
};
unsigned short menuPianoRollLabelFAIcon[16]={
    FA_POWER_OFF,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,FA_ARROWS_ALT,
    NULL,FA_COGS,FA_ARROW_CIRCLE_LEFT,FA_WINDOW_CLOSE,
};

int menuMidiColNb=4;
static GLuint txtMenuMidiHandle[16];
const char *menuMidiLabel[16]={
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
};
void *menuMidiVar[16]={
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
};
unsigned short menuMidiLabelFAIcon[16]={
    FA_POWER_OFF,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,FA_ARROWS_ALT,
    NULL,FA_COGS,FA_ARROW_CIRCLE_LEFT,FA_WINDOW_CLOSE,
};

int menuModPatternColNb=4;
static GLuint txtMenuModPatternHandle[16];
const char *menuModPatternLabel[16]={
    NULL,NULL,NULL,NULL,
    "Volume\nbars",NULL,NULL,"Fixed bar",
    NULL,"@sliderBG\nopacity|0|90",NULL,NULL,
    NULL,NULL,NULL,NULL,
};
void *menuModPatternVar[16]={
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,&global_MODPatOpacity,NULL,NULL,
    NULL,NULL,NULL,NULL,
};
unsigned short menuModPatternLabelFAIcon[16]={
    FA_POWER_OFF,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,FA_ARROWS_ALT,
    NULL,FA_COGS,FA_ARROW_CIRCLE_LEFT,FA_WINDOW_CLOSE,
};
char *menuModPatternDynLabel[16];



struct {
    int menu_idx;
} pMenu_state;

int pMenu_PMUpdateSelStatus(FileNode *fnode,bool propagateStatus,bool selStatus);
int pMenu_PMUpdateFavStatus(FileNode *fnode,bool propagateStatus,bool favStatus);
int pMenu_PMbuildDirTree(FileNode *fileNode, int idx,bool filter,int updExpandCollapse,int selectedMode,float drawMinY,float drawMaxY,float drawLineHeight);
int pMenu_PMPresetsSelAll(FileNode *fnode);
int pMenu_PMPresetsRemAll(FileNode *fnode);
int pMenu_PMPresetsSelFiltered(FileNode *fnode,int selectedMode,bool filterMode);
int pMenu_PMPresetsRemFiltered(FileNode *fnode,int selectedMode,bool filterMode);
void pMenu_PMInitTempData(FileNode *fnode);
void pMenu_PMCommitTempData(FileNode *fnode);


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
        if (settings[PROJECTM_FXONOFF].detail.mdz_boolswitch.switch_value) active_idx|=1<<FXPROJECTM_IDX;
        if (settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value) active_idx|=1<<11;
    } else if (menu_idx==MENU_ROOT_MORE) {
        if (settings[GLOB_FXSHOWINFO].detail.mdz_switch.switch_value) active_idx|=1<<0;
        if (settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value) active_idx|=1<<11;
        if (settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value) active_idx|=1<<11;
        
        menuMenuMoreDynLabel[0]=settings[GLOB_FXSHOWINFO].detail.mdz_switch.switch_labels[settings[GLOB_FXSHOWINFO].detail.mdz_switch.switch_value];
    } else if (menu_idx==MENU_OSCILLO) {
        if (settings[OSCILLO_FXMODE].detail.mdz_switch.switch_value==0) active_idx|=1<<0;
        if (settings[OSCILLO_FXMODE].detail.mdz_switch.switch_value==1) active_idx|=1<<1;
        if (settings[OSCILLO_FXMODE].detail.mdz_switch.switch_value==2) active_idx|=1<<2;
        if (settings[OSCILLO_FXMODE].detail.mdz_switch.switch_value==3) active_idx|=1<<3;
        if (settings[OSCILLO_FXMODE].detail.mdz_switch.switch_value==4) active_idx|=1<<4;
        if (settings[OSCILLO_ShowLabel].detail.mdz_boolswitch.switch_value==1) active_idx|=1<<5;
        if (settings[OSCILLO_ShowGrid].detail.mdz_boolswitch.switch_value==1) active_idx|=1<<6;
        if (settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value) active_idx|=1<<11;
        
        snprintf(menuOscilloDynLabel[7],64,"Thickness:\n%s",settings[OSCILLO_LINE_Width].detail.mdz_switch.switch_labels[settings[OSCILLO_LINE_Width].detail.mdz_switch.switch_value]);
        snprintf(menuOscilloDynLabel[8],64,"Font size\n%s",settings[OSCILLO_LabelFontSize].detail.mdz_switch.switch_labels[settings[OSCILLO_LabelFontSize].detail.mdz_switch.switch_value]);
    } else if (menu_idx==MENU_2DSPECTRUM) {
        if (settings[GLOB_FXSpectrum].detail.mdz_switch.switch_value==0) active_idx|=1<<0;
        if (settings[GLOB_FXSpectrum].detail.mdz_switch.switch_value==1) active_idx|=1<<1;
        if (settings[GLOB_FXSpectrum].detail.mdz_switch.switch_value==2) active_idx|=1<<2;
        if (settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value) active_idx|=1<<11;
    } else if (menu_idx==MENU_3DSPECTRUM) {
        if (settings[GLOB_FX3DSpectrum].detail.mdz_switch.switch_value==0) active_idx|=1<<0;
        if (settings[GLOB_FX3DSpectrum].detail.mdz_switch.switch_value==1) active_idx|=1<<1;
        if (settings[GLOB_FX3DSpectrum].detail.mdz_switch.switch_value==2) active_idx|=1<<2;
        if (settings[GLOB_FX3DSpectrum].detail.mdz_switch.switch_value==3) active_idx|=1<<3;
        if (settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value) active_idx|=1<<11;
        
        snprintf(menu3DSpectrumDynLabel[7],64,"Bloom:\n%s",settings[GLOB_FX3DSpectrumBloom].detail.mdz_switch.switch_labels[settings[GLOB_FX3DSpectrumBloom].detail.mdz_switch.switch_value]);
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
        if (settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value) active_idx|=1<<11;
        
        snprintf(menu3DLandscapeDynLabel[9],64,"Bloom:\n%s",settings[GLOB_FX3DLandscapeBloom].detail.mdz_switch.switch_labels[settings[GLOB_FX3DLandscapeBloom].detail.mdz_switch.switch_value]);
    } else if (menu_idx==MENU_PIANO3D) {
        if (settings[GLOB_FXPiano3D].detail.mdz_switch.switch_value==0) active_idx|=1<<0;
        if (settings[GLOB_FXPiano3D].detail.mdz_switch.switch_value==1) active_idx|=1<<1;
        if (settings[GLOB_FXPiano3D].detail.mdz_switch.switch_value==2) active_idx|=1<<2;
        if (settings[GLOB_FXPiano3D].detail.mdz_switch.switch_value==3) active_idx|=1<<3;
        if (settings[GLOB_FXPiano3D].detail.mdz_switch.switch_value==4) active_idx|=1<<4;
        if (settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value) active_idx|=1<<11;
    } else if (menu_idx==MENU_PIANOROLL) {
        if (settings[GLOB_FXPianoRoll].detail.mdz_switch.switch_value==0) active_idx|=1<<0;
        if (settings[GLOB_FXPianoRoll].detail.mdz_switch.switch_value==1) active_idx|=1<<1;
        if (settings[GLOB_FXPianoRoll].detail.mdz_switch.switch_value==2) active_idx|=1<<2;
        if (settings[GLOB_FXPianoRollVoicesLabels].detail.mdz_boolswitch.switch_value) active_idx|=1<<8;
        if (settings[GLOB_FXPianoRollOctavesLabels].detail.mdz_boolswitch.switch_value) active_idx|=1<<9;
        if (settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value) active_idx|=1<<11;
    } else if (menu_idx==MENU_MIDIPATTERN) {
        if (settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_value==0) active_idx|=1<<0;
        if (settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_value==1) active_idx|=1<<1;
        if (settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_value==2) active_idx|=1<<2;
        if (settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value) active_idx|=1<<11;
    } else if (menu_idx==MENU_MODPATTERN) {
        if (settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value==0) active_idx|=1<<0;
        if (settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value==1) active_idx|=1<<1;
        if (settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value==2) active_idx|=1<<2;
        if (settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value==3) active_idx|=1<<3;
        if (settings[GLOB_FXMODPattern_VolBar].detail.mdz_boolswitch.switch_value) active_idx|=1<<4;
        if (settings[GLOB_FXMODPattern_CurrentLineMode].detail.mdz_switch.switch_value==1) active_idx|=1<<7;
        if (settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value) active_idx|=1<<11;
        
        snprintf(menuModPatternDynLabel[5],64,"Themes:\n%s",settings[GLOB_FXMODPattern_Theme].detail.mdz_switch.switch_labels[settings[GLOB_FXMODPattern_Theme].detail.mdz_switch.switch_value]);
        snprintf(menuModPatternDynLabel[8],64,"Font size\n%s",settings[GLOB_FXMODPattern_FontSize].detail.mdz_switch.switch_labels[settings[GLOB_FXMODPattern_FontSize].detail.mdz_switch.switch_value]);
        snprintf(menuModPatternDynLabel[12],64,"Font:\n%s",settings[GLOB_FXMODPattern_Font].detail.mdz_switch.switch_labels[settings[GLOB_FXMODPattern_Font].detail.mdz_switch.switch_value]);
        
    } else if (menu_idx==MENU_PROJECTM) {
        if (settings[PROJECTM_FXONOFF].detail.mdz_boolswitch.switch_value) active_idx|=1<<1;
        else active_idx|=1<<0;
        if (settings[PROJECTM_ShowPresetLabel].detail.mdz_switch.switch_value==1) active_idx|=1<<2;
        else if (settings[PROJECTM_ShowPresetLabel].detail.mdz_switch.switch_value==2) active_idx|=1<<3;
        if (settings[PROJECTM_BundledPresets].detail.mdz_boolswitch.switch_value) active_idx|=1<<4;
        if (settings[PROJECTM_CustomPresets].detail.mdz_boolswitch.switch_value) active_idx|=1<<5;
        if (settings[PROJECTM_AutoSwitchPresetsMode].detail.mdz_switch.switch_value) active_idx|=1<<6;
        if (settings[PROJECTM_BlendPresets].detail.mdz_boolswitch.switch_value) active_idx|=1<<7;
        if (settings[PROJECTM_LockPreset].detail.mdz_boolswitch.switch_value) active_idx|=1<<10;
        if (settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value) active_idx|=1<<11;
        
        if (settings[PROJECTM_LockPreset].detail.mdz_boolswitch.switch_value) menuProjectMLabelFAIcon[10]=FA_LOCK;
        else menuProjectMLabelFAIcon[10]=FA_UNLOCK;
        
        
        if (settings[PROJECTM_BundledPresets].detail.mdz_boolswitch.switch_value) menuProjectMDynLabel[8]=(char*)"Select\nbundled\npresets";
        else menuProjectMDynLabel[8]=NULL;
        if (settings[PROJECTM_CustomPresets].detail.mdz_boolswitch.switch_value) menuProjectMDynLabel[9]=(char*)"Select\ncustom\npresets";
        else menuProjectMDynLabel[9]=NULL;
    }
    return active_idx;
}


//------------------------------------------------------
// playerMenuInit
//   initialize system & load required data
//------------------------------------------------------
void playerMenuInit() {
    pMenu_state.menu_idx=MENU_ROOT;
    
    memset(pmFileNodeFilter,0,sizeof(pmFileNodeFilter));
    
    memset(txtMenuHandle,0,sizeof(txtMenuHandle));
    memset(txtMenuMoreHandle,0,sizeof(txtMenuMoreHandle));
    memset(txtMenuProjectMHandle,0,sizeof(txtMenuProjectMHandle));
    memset(txtMenuProjectMExploreHandle,0,sizeof(txtMenuProjectMExploreHandle));
    memset(txtMenuOscilloHandle,0,sizeof(txtMenuOscilloHandle));
    memset(menuMenuMoreDynLabel,0,sizeof(menuMenuMoreDynLabel));
    memset(menuOscilloDynLabel,0,sizeof(menuOscilloDynLabel));
    memset(menuProjectMDynLabel,0,sizeof(menuProjectMDynLabel));
    memset(menu3DSpectrumDynLabel,0,sizeof(menu3DSpectrumDynLabel));
    memset(menu3DLandscapeDynLabel,0,sizeof(menu3DLandscapeDynLabel));
    memset(txtMenu2DSpectrumHandle,0,sizeof(txtMenu2DSpectrumHandle));
    memset(txtMenu3DSpectrumHandle,0,sizeof(txtMenu3DSpectrumHandle));
    memset(txtMenu3DLandscapeHandle,0,sizeof(txtMenu3DLandscapeHandle));
    memset(txtMenuPiano3DHandle,0,sizeof(txtMenuPiano3DHandle));
    memset(txtMenuPianoRollHandle,0,sizeof(txtMenuPianoRollHandle));
    memset(txtMenuMidiHandle,0,sizeof(txtMenuMidiHandle));
    memset(txtMenuModPatternHandle,0,sizeof(txtMenuModPatternHandle));
    memset(menuModPatternDynLabel,0,sizeof(menuModPatternDynLabel));
    
    memset(menu_scrollX,0,sizeof(menu_scrollX));
    memset(menu_scrollY,0,sizeof(menu_scrollY));
    
    menuOscilloDynLabel[7]=(char*)malloc(64);
    menuOscilloDynLabel[8]=(char*)malloc(64);
    menuProjectMDynLabel[8]=(char*)malloc(64);
    menuProjectMDynLabel[9]=(char*)malloc(64);
    menu3DSpectrumDynLabel[7]=(char*)malloc(64);
    menu3DLandscapeDynLabel[9]=(char*)malloc(64);
    menuModPatternDynLabel[5]=(char*)malloc(64);
    menuModPatternDynLabel[12]=(char*)malloc(64);
    
    menuModPatternDynLabel[8]=(char*)malloc(64);
    snprintf(menuModPatternDynLabel[8],64,"Font size\n%s",settings[GLOB_FXMODPattern_FontSize].detail.mdz_switch.switch_labels[settings[GLOB_FXMODPattern_FontSize].detail.mdz_switch.switch_value]);
    
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

int buildSubMenu(int r,
                 int c,
                 int num_col,
                 int isActive,
                 float cell_size,
                 float cell_sizeH,
                 
                 GLuint *current_txtMenuHandle,
                 const char **currentMenuLabel,
                 char **currentMenuDynLabel,
                 unsigned short *currentMenuLabelFAIcon,
                 void **currentMenuVar) {
    bool ret=false;
    int celIdx=r*num_col+c;
    ImVec2 uv0(0,0);ImVec2 uv1(1,1);ImVec4 bg_col(0,0,0,0.0f);ImVec4 tint_col(0.4,0.4,0.4,0.8f);
    if (isActive) {//Active
        tint_col.x=1.0;tint_col.y=1.0;tint_col.z=1.0;tint_col.w=1.0f;
        if (current_txtMenuHandle[celIdx]) {
            ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0.0,0.0,0.0,0.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ImVec4(1.0,1.0,1.0,0.5f));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button,colorBtnTextActive);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,colorBtnTextActiveH);
        }
    } else { //Inactive
        if (current_txtMenuHandle[celIdx]) {
            ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0.0,0.0,0.0,0.6f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ImVec4(1.0,1.0,1.0,0.3f));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button,colorBtnTextInactive);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,colorBtnTextInactiveH);
        }
    }
    ImVec2 cur_pos=ImGui::GetCursorPos();
    if (current_txtMenuHandle[celIdx]) { //Image Button
        float padding=6;
        if (isActive) {
            ImGui::SetNextItemAllowOverlap();
            tint_col.x=1.0;tint_col.y=1.0;tint_col.z=1.0;tint_col.w=1.0f;
            ImGui::PushID((celIdx)*4+0);
            ImGui::ImageButton("",(ImTextureID)(intptr_t)current_txtMenuHandle[celIdx], ImVec2(cell_size-padding, cell_sizeH-padding),uv0,uv1,bg_col,tint_col);
            ImGui::PopID();
            ImGui::SetCursorPos(cur_pos);
            ImGui::PushID((celIdx)*4+1);
            ret=ImGui::ImageButton("",(ImTextureID)(intptr_t)txtShineFx,ImVec2(cell_size-padding, cell_sizeH-padding),uv0,uv1,bg_col,tint_col);
            ImGui::PopID();
        } else {
            ImGui::PushID((celIdx)*4);
            ret=ImGui::ImageButton("",(ImTextureID)(intptr_t)current_txtMenuHandle[celIdx], ImVec2(cell_size-padding, cell_sizeH-padding),uv0,uv1,bg_col,tint_col);
            ImGui::PopID();
        }
    } else if (currentMenuLabel[celIdx]) { //Text Button
        ImGui::PushID((celIdx)*4+0);
        if (strstr(currentMenuLabel[celIdx],"@slider")) {
            char strName[64];
            const char *strStart=currentMenuLabel[celIdx]+strlen("@slider");
            int i=0;
            float minVal=0,maxVal=100;
            strName[0]=0;
            //Parse string to get label, minval, maxval
            //format: @sliderLABEL|minval|maxval
            while (strStart[i]) {
                if (strStart[i]=='|') {
                    snprintf(strName,i+1,"%s",strStart);
                    minVal=atof(strStart+i+1);
                    i++;
                    while (strStart[i]) {
                        if (strStart[i]=='|') {
                            maxVal=atof(strStart+i+1);
                            break;
                        }
                        i++;
                    }
                    break;
                }
                i++;
                if (i>=64) break;
            }
            
            
                cur_pos=ImGui::GetCursorPos();
                cur_pos.y+=(cell_sizeH/4);
                ImGui::SetCursorPos(cur_pos);
                ImGui::LabelText("", "%s",strName);
                cur_pos.x+=(cell_size-1.5*cell_size/3);
                cur_pos.y-=(cell_sizeH/4);
                ImGui::SetCursorPos(cur_pos);
                ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, cell_sizeH/5);
                ImGui::VSliderFloat("",ImVec2(cell_size/3,cell_sizeH*4/4),  (float*)(currentMenuVar[celIdx]), minVal, maxVal,"%.0f%%");
                ImGui::PopStyleVar();
        } else {
            if (isActive) {
                ret=ImGui::Button(currentMenuLabel[celIdx],ImVec2(cell_size, cell_sizeH));
            } else {
                ret=ImGui::Button(currentMenuLabel[celIdx],ImVec2(cell_size, cell_sizeH));
            }
        }
        ImGui::PopID();
    } else if (currentMenuDynLabel && currentMenuDynLabel[celIdx]) { //Dynamic text button
        ImGui::PushID((celIdx)*4+0);
        if (isActive) ret=ImGui::Button(currentMenuDynLabel[celIdx],ImVec2(cell_size, cell_sizeH));
        else ret=ImGui::Button(currentMenuDynLabel[celIdx],ImVec2(cell_size, cell_sizeH));
        ImGui::PopID();
    } else if (currentMenuLabelFAIcon[celIdx]) {
        ImGui::PushID((celIdx)*4+0);
        
        ImGui::PushFont(font_menu_icon,idealFontSize*2.0f*glScaleFactor*16.0/12.0);
        if (isActive) ret=ImGui::Button(faicon(currentMenuLabelFAIcon[celIdx]),ImVec2(cell_size, cell_sizeH));
        else ret=ImGui::Button(faicon(currentMenuLabelFAIcon[celIdx]),ImVec2(cell_size, cell_sizeH));
        ImGui::PopFont();
        
        ImGui::PopID();
    }
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    return ret;
}

//------------------------------------------------------
// playerShowMenu
//   draw current menu, keep record of state
//   return -1 if it has been closed and all FX view should be hidden
//   return 0 if it has been closed
//   return 1 if it is to be kept open
//------------------------------------------------------
int playerShowMenu(float ww,float hh,float glScaleFactor,float fadelev,float panX,float panY,int menushow) {
    static int cpt=0;
    if (!pMenu_isInitialized) return 0;
    int keepOpened=1;
    float menu_win_size;
    float menu_win_sizeH;;
    ImVec2 menu_win_pos;
    float cell_size;
    GLuint *current_txtMenuHandle;
    const char **currentMenuLabel;
    char **currentMenuDynLabel;
    unsigned short *currentMenuLabelFAIcon;
    void **currentMenuVar;
    static int selectedMode=0;
    
    cpt++;
    for (int i=0;i<16;i++) {
        menuCpt[i]++;
    }
    
    float menu_margin=1.0*glScaleFactor;
    float menu_cell_padding=1.0*glScaleFactor;
    
    menu_win_size=round(fmin(ww*glScaleFactor-2*menu_margin,hh*glScaleFactor-2*menu_margin));
    // Determine menu size, manage exception
    //------------------------------------------------
    if (pMenu_state.menu_idx==MENU_PROJECTM_EXPLORE) {
        menu_win_sizeH=(hh*0.9f)*glScaleFactor;
    } else {
        menu_win_sizeH=menu_win_size;
        
    }

    
    idealFontSize=menu_win_size/glScaleFactor/PL_IDEALFONTSIZE_RATIO;
    if (idealFontSize<PL_MIN_FONT_SIZE) idealFontSize=PL_MIN_FONT_SIZE;
    
    
    // Global var mirroring
    global_FXAlpha=settings[GLOB_FXAlpha].detail.mdz_slider.slider_value*100;
    
    // Mod pattern opacity
    global_MODPatOpacity=settings[GLOB_FXMODPattern_BGAlpha].detail.mdz_slider.slider_value*100;
    
    // Root window, full screen
    ImGui::SetNextWindowPos(ImVec2(menu_margin,menu_margin));
    ImGui::SetNextWindowSize(ImVec2(ww*glScaleFactor-2*menu_margin,hh*glScaleFactor-2*menu_margin));
    
    ImGui::GetStyle().FrameRounding = 10.0f;
    
    ImGui::PushStyleColor(ImGuiCol_WindowBg,ImVec4(0.0f,0.0f,0.0f,MENU_BACKGROUND_ALPHA));
    
    
    ImGui::Begin("Modizer root menu",0,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoScrollbar);
    ImGui::SetWindowFocus();
    ImGui::GetStyle().Alpha=fadelev;
    
    //------------------------------------------------
    
    menu_win_pos=ImVec2((ww*glScaleFactor-menu_win_size)/2,(hh*glScaleFactor-menu_win_sizeH)/2);
    
    ImGui::SetNextWindowPos(menu_win_pos);
    ImGui::BeginChild("Modizer menu",ImVec2(menu_win_size,menu_win_sizeH));
    static ImGuiTableFlags flagTable = /*ImGuiTableFlags_Borders|*/ImGuiTableFlags_NoBordersInBody|ImGuiTableFlags_SizingFixedSame|ImGuiTableFlags_NoHostExtendX|ImGuiTableFlags_PreciseWidths|ImGuiTableFlags_NoPadOuterX;
    
    ImVec2 cell_padding(menu_cell_padding,menu_cell_padding);
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, cell_padding);
    ImGui::PushStyleVar(ImGuiStyleVar_ImageBorderSize, 0);
    
    
    if (font_menu) ImGui::PushFont(font_menu,idealFontSize*glScaleFactor);
    else ImGui::PushFont(nullptr);//,18*menu_win_size/512);
    
    int activeFx=playerGetActivatedCells(pMenu_state.menu_idx);
    
    if (pMenu_state.menu_idx==MENU_ROOT) {
        //Select right current textures for root menu itemas, based on current settings
        playerRootMenuInitRightItemsTexture();
        int col_nb=menuRootColNb;
        if (ImGui::BeginTable("menu_root",col_nb,flagTable)) {
            current_txtMenuHandle=txtMenuHandle;
            currentMenuLabel=menuRootLabel;
            currentMenuLabelFAIcon=menuRootLabelFAIcon;
            currentMenuVar=menuRootVar;
            currentMenuDynLabel=NULL;
            cell_size=round((menu_win_size)/col_nb)-3*menu_cell_padding;
            for (int r=0;r<4;r++) {
                ImGui::TableNextRow(0,cell_size+2*menu_cell_padding);
                for (int c=0;c<col_nb;c++) {
                    ImGui::TableSetColumnIndex(c);
                    
                    bool isActive=activeFx&(1<<(r*4+c));
                    int ret=buildSubMenu(r,
                                         c,
                                         col_nb,
                                         isActive,
                                         cell_size,
                                         cell_size,
                                         current_txtMenuHandle,
                                         currentMenuLabel,
                                         currentMenuDynLabel,
                                         currentMenuLabelFAIcon,
                                         currentMenuVar);
                    if ((menushow==-1)&&((c*16+r)==0x33)) ret=1; //force exit
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
                            case 0x12:
                                break;
                            case 0x22: //Menu more
                                pMenu_state.menu_idx=MENU_ROOT_MORE;
                                break;
                            case 0x32: //Fullscreen switch
                                settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value=!(settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value);
                                break;
                            case 0x03://Close FX window
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
                                settings[PROJECTM_FXONOFF].detail.mdz_boolswitch.switch_value=0;
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
    } else if (pMenu_state.menu_idx==MENU_ROOT_MORE) {
        int col_nb=menuMoreColNb;
        if (ImGui::BeginTable("menu_root_more",col_nb,flagTable)) {
            current_txtMenuHandle=txtMenuMoreHandle;
            currentMenuLabel=menuRootMoreLabel;
            currentMenuLabelFAIcon=menuRootMoreLabelFAIcon;
            currentMenuVar=menuRootMoreVar;
            currentMenuDynLabel=menuMenuMoreDynLabel;
            cell_size=round((menu_win_size)/col_nb)-3*menu_cell_padding;
            for (int r=0;r<4;r++) {
                ImGui::TableNextRow(0,cell_size+2*menu_cell_padding);
                for (int c=0;c<col_nb;c++) {
                    ImGui::TableSetColumnIndex(c);
                    
                    bool isActive=activeFx&(1<<(r*4+c));
                    int ret=buildSubMenu(r,
                                         c,
                                         col_nb,
                                         isActive,
                                         cell_size,
                                         cell_size,
                                         current_txtMenuHandle,
                                         currentMenuLabel,
                                         currentMenuDynLabel,
                                         currentMenuLabelFAIcon,
                                         currentMenuVar);
                    if ((menushow==-1)&&((c*16+r)==0x33)) ret=1; //force exit
                    if (ret) {
                        switch (c*16+r) {
                            case 0x00://Show FPS
                                settings[GLOB_FXSHOWINFO].detail.mdz_switch.switch_value=(settings[GLOB_FXSHOWINFO].detail.mdz_switch.switch_value+1)%settings[GLOB_FXSHOWINFO].detail.mdz_switch.switch_value_nb;
                                break;
                            case 0x10: //FX Alpha
                                break;
                            case 0x20:
                                break;
                            case 0x30:
                                break;
                            case 0x01:
                                break;
                            case 0x11:
                                break;
                            case 0x21:
                                break;
                            case 0x31:
                                break;
                            case 0x02:
                                break;
                            case 0x12:
                                break;
                            case 0x22:
                                break;
                            case 0x32: //Fullscreen switch
                                settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value=!(settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value);
                                break;
                            case 0x03://Close FX window
                                keepOpened=-1;
                                break;
                            case 0x13: //Go to settings - visu
                                keepOpened=2;
                                break;
                            case 0x23: //Back to main menu
                                pMenu_state.menu_idx=MENU_ROOT;
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
        int col_nb=menuOscilloColNb;
        if (ImGui::BeginTable("menu_oscillo",col_nb,flagTable)) {
            current_txtMenuHandle=txtMenuOscilloHandle;
            currentMenuLabel=menuOscilloLabel;
            currentMenuLabelFAIcon=menuOscilloLabelFAIcon;
            currentMenuVar=menuOscilloVar;
            currentMenuDynLabel=menuOscilloDynLabel;
            cell_size=round((menu_win_size)/col_nb)-3*menu_cell_padding;
            for (int r=0;r<4;r++) {
                ImGui::TableNextRow(0,cell_size);
                for (int c=0;c<col_nb;c++) {
                    ImGui::TableSetColumnIndex(c);
                    
                    bool isActive=activeFx&(1<<(r*4+c));
                    int ret=buildSubMenu(r,
                                         c,
                                         col_nb,
                                         isActive,
                                         cell_size,
                                         cell_size,
                                         current_txtMenuHandle,
                                         currentMenuLabel,
                                         currentMenuDynLabel,
                                         currentMenuLabelFAIcon,
                                         currentMenuVar);
                    if ((menushow==-1)&&((c*16+r)==0x33)) ret=1; //force exit
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
                                settings[OSCILLO_LabelFontSize].detail.mdz_switch.switch_value=(settings[OSCILLO_LabelFontSize].detail.mdz_switch.switch_value+1)%settings[OSCILLO_LabelFontSize].detail.mdz_switch.switch_value_nb;
                                break;
                            case 0x12:
                                break;
                            case 0x22:
                                break;
                            case 0x32: //Fullscreen switch
                                settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value=!(settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value);
                                break;
                            case 0x03:break;
                            case 0x13: //Go to settings - oscillo
                                keepOpened=3;
                                break;
                            case 0x23: //Back to main menu
                                pMenu_state.menu_idx=MENU_ROOT;
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
    } else if (pMenu_state.menu_idx==MENU_2DSPECTRUM) {
        int col_nb=menu2DSpectrumColNb;
        if (ImGui::BeginTable("menu_2dspectrum",col_nb,flagTable)) {
            current_txtMenuHandle=txtMenu2DSpectrumHandle;
            currentMenuLabel=menu2DSpectrumLabel;
            currentMenuLabelFAIcon=menu2DSpectrumLabelFAIcon;
            currentMenuVar=menu2DSpectrumVar;
            currentMenuDynLabel=NULL;
            cell_size=round((menu_win_size)/col_nb)-3*menu_cell_padding;
            for (int r=0;r<4;r++) {
                ImGui::TableNextRow(0,cell_size);
                for (int c=0;c<col_nb;c++) {
                    ImGui::TableSetColumnIndex(c);
                    
                    bool isActive=activeFx&(1<<(r*4+c));
                    int ret=buildSubMenu(r,
                                         c,
                                         col_nb,
                                         isActive,
                                         cell_size,
                                         cell_size,
                                         current_txtMenuHandle,
                                         currentMenuLabel,
                                         currentMenuDynLabel,
                                         currentMenuLabelFAIcon,
                                         currentMenuVar);
                    if ((menushow==-1)&&((c*16+r)==0x33)) ret=1; //force exit
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
                            case 0x32: //Fullscreen switch
                                settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value=!(settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value);
                                break;
                            case 0x03:break;
                            case 0x13: //Go to settings - visu
                                keepOpened=2;
                                break;
                            case 0x23: //Back to main menu
                                pMenu_state.menu_idx=MENU_ROOT;
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
    } else if (pMenu_state.menu_idx==MENU_3DSPECTRUM) {
        int col_nb=menu3DSpectrumColNb;
        if (ImGui::BeginTable("menu_3dspectrum",col_nb,flagTable)) {
            current_txtMenuHandle=txtMenu3DSpectrumHandle;
            currentMenuLabel=menu3DSpectrumLabel;
            currentMenuDynLabel=menu3DSpectrumDynLabel;
            currentMenuLabelFAIcon=menu3DSpectrumLabelFAIcon;
            currentMenuVar=menu3DSpectrumVar;
            cell_size=round((menu_win_size)/col_nb)-3*menu_cell_padding;
            for (int r=0;r<4;r++) {
                ImGui::TableNextRow(0,cell_size);
                for (int c=0;c<col_nb;c++) {
                    ImGui::TableSetColumnIndex(c);
                    
                    bool isActive=activeFx&(1<<(r*4+c));
                    int ret=buildSubMenu(r,
                                         c,
                                         col_nb,
                                         isActive,
                                         cell_size,
                                         cell_size,
                                         current_txtMenuHandle,
                                         currentMenuLabel,
                                         currentMenuDynLabel,
                                         currentMenuLabelFAIcon,
                                         currentMenuVar);
                    if ((menushow==-1)&&((c*16+r)==0x33)) ret=1; //force exit
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
                            case 0x31: //Bloom
                                settings[GLOB_FX3DSpectrumBloom].detail.mdz_switch.switch_value=(settings[GLOB_FX3DSpectrumBloom].detail.mdz_switch.switch_value+1)%settings[GLOB_FX3DSpectrumBloom].detail.mdz_switch.switch_value_nb;
                                break;
                            case 0x02:break;
                            case 0x12:break;
                            case 0x22:break;
                            case 0x32: //Fullscreen switch
                                settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value=!(settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value);
                                break;
                            case 0x03:break;
                            case 0x13: //Go to settings - visu
                                keepOpened=2;
                                break;
                            case 0x23: //Back to main menu
                                pMenu_state.menu_idx=MENU_ROOT;
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
    } else if (pMenu_state.menu_idx==MENU_3DLANDSCAPE) {
        int col_nb=menu3DLandscapeColNb;
        if (ImGui::BeginTable("menu_3dlandscape",col_nb,flagTable)) {
            current_txtMenuHandle=txtMenu3DLandscapeHandle;
            currentMenuLabel=menu3DLandscapeLabel;
            currentMenuDynLabel=menu3DLandscapeDynLabel;
            currentMenuLabelFAIcon=menu3DLandscapeLabelFAIcon;
            currentMenuVar=menu3DLandscapeVar;
            cell_size=round((menu_win_size)/col_nb)-3*menu_cell_padding;
            for (int r=0;r<4;r++) {
                ImGui::TableNextRow(0,cell_size);
                for (int c=0;c<col_nb;c++) {
                    ImGui::TableSetColumnIndex(c);
                    
                    bool isActive=activeFx&(1<<(r*4+c));
                    int ret=buildSubMenu(r,
                                         c,
                                         col_nb,
                                         isActive,
                                         cell_size,
                                         cell_size,
                                         current_txtMenuHandle,
                                         currentMenuLabel,
                                         currentMenuDynLabel,
                                         currentMenuLabelFAIcon,
                                         currentMenuVar);
                    if ((menushow==-1)&&((c*16+r)==0x33)) ret=1; //force exit
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
                            case 0x12: //Bloom
                                settings[GLOB_FX3DLandscapeBloom].detail.mdz_switch.switch_value=(settings[GLOB_FX3DLandscapeBloom].detail.mdz_switch.switch_value+1)%settings[GLOB_FX3DLandscapeBloom].detail.mdz_switch.switch_value_nb;
                                break;
                            case 0x22:break;
                            case 0x32: //Fullscreen switch
                                settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value=!(settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value);
                                break;
                            case 0x03:break;
                            case 0x13: //Go to settings - visu
                                keepOpened=2;
                                break;
                            case 0x23: //Back to main menu
                                pMenu_state.menu_idx=MENU_ROOT;
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
    } else if (pMenu_state.menu_idx==MENU_PIANOROLL) {
        int col_nb=menuPianoRollColNb;
        if (ImGui::BeginTable("menu_pianoroll",col_nb,flagTable)) {
            current_txtMenuHandle=txtMenuPianoRollHandle;
            currentMenuLabel=menuPianoRollLabel;
            currentMenuLabelFAIcon=menuPianoRollLabelFAIcon;
            currentMenuVar=menuPianoRollVar;
            currentMenuDynLabel=NULL;
            cell_size=round((menu_win_size)/col_nb)-3*menu_cell_padding;
            for (int r=0;r<4;r++) {
                ImGui::TableNextRow(0,cell_size);
                for (int c=0;c<col_nb;c++) {
                    ImGui::TableSetColumnIndex(c);
                    
                    bool isActive=activeFx&(1<<(r*4+c));
                    int ret=buildSubMenu(r,
                                         c,
                                         col_nb,
                                         isActive,
                                         cell_size,
                                         cell_size,
                                         current_txtMenuHandle,
                                         currentMenuLabel,
                                         currentMenuDynLabel,
                                         currentMenuLabelFAIcon,
                                         currentMenuVar);
                    if ((menushow==-1)&&((c*16+r)==0x33)) ret=1; //force exit
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
                            case 0x32: //Fullscreen switch
                                settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value=!(settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value);
                                break;
                            case 0x03:break;
                            case 0x13: //Go to settings - visu
                                keepOpened=2;
                                break;
                            case 0x23: //Back to main menu
                                pMenu_state.menu_idx=MENU_ROOT;
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
    } else if (pMenu_state.menu_idx==MENU_PIANO3D) {
        int col_nb=menuPiano3DColNb;
        if (ImGui::BeginTable("menu_piano3d",col_nb,flagTable)) {
            current_txtMenuHandle=txtMenuPiano3DHandle;
            currentMenuLabel=menuPiano3DLabel;
            currentMenuLabelFAIcon=menuPiano3DLabelFAIcon;
            currentMenuVar=menuPiano3DVar;
            currentMenuDynLabel=NULL;
            cell_size=round((menu_win_size)/col_nb)-3*menu_cell_padding;
            for (int r=0;r<4;r++) {
                ImGui::TableNextRow(0,cell_size);
                for (int c=0;c<col_nb;c++) {
                    ImGui::TableSetColumnIndex(c);
                    
                    bool isActive=activeFx&(1<<(r*4+c));
                    int ret=buildSubMenu(r,
                                         c,
                                         col_nb,
                                         isActive,
                                         cell_size,
                                         cell_size,
                                         current_txtMenuHandle,
                                         currentMenuLabel,
                                         currentMenuDynLabel,
                                         currentMenuLabelFAIcon,
                                         currentMenuVar);
                    if ((menushow==-1)&&((c*16+r)==0x33)) ret=1; //force exit
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
                            case 0x32: //Fullscreen switch
                                settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value=!(settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value);
                                break;
                            case 0x03:break;
                            case 0x13: //Go to settings - visu
                                keepOpened=2;
                                break;
                            case 0x23: //Back to main menu
                                pMenu_state.menu_idx=MENU_ROOT;
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
    } else if (pMenu_state.menu_idx==MENU_MIDIPATTERN) {
        int col_nb=menuMidiColNb;
        if (ImGui::BeginTable("menu_midipattern",col_nb,flagTable)) {
            current_txtMenuHandle=txtMenuMidiHandle;
            currentMenuLabel=menuMidiLabel;
            currentMenuLabelFAIcon=menuMidiLabelFAIcon;
            currentMenuVar=menuMidiVar;
            currentMenuDynLabel=NULL;
            cell_size=round((menu_win_size)/col_nb)-3*menu_cell_padding;
            for (int r=0;r<4;r++) {
                ImGui::TableNextRow(0,cell_size);
                for (int c=0;c<col_nb;c++) {
                    ImGui::TableSetColumnIndex(c);
                    
                    bool isActive=activeFx&(1<<(r*4+c));
                    int ret=buildSubMenu(r,
                                         c,
                                         col_nb,
                                         isActive,
                                         cell_size,
                                         cell_size,
                                         current_txtMenuHandle,
                                         currentMenuLabel,
                                         currentMenuDynLabel,
                                         currentMenuLabelFAIcon,
                                         currentMenuVar);
                    if ((menushow==-1)&&((c*16+r)==0x33)) ret=1; //force exit
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
                            case 0x32: //Fullscreen switch
                                settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value=!(settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value);
                                break;
                            case 0x03:break;
                            case 0x13: //Go to settings - visu
                                keepOpened=2;
                                break;
                            case 0x23: //Back to main menu
                                pMenu_state.menu_idx=MENU_ROOT;
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
    } else if (pMenu_state.menu_idx==MENU_MODPATTERN) {
        int col_nb=menuModPatternColNb;
        if (ImGui::BeginTable("menu_modpattern",col_nb,flagTable)) {
            current_txtMenuHandle=txtMenuModPatternHandle;
            currentMenuLabel=menuModPatternLabel;
            currentMenuDynLabel=menuModPatternDynLabel;
            currentMenuLabelFAIcon=menuModPatternLabelFAIcon;
            currentMenuVar=menuModPatternVar;
            cell_size=round((menu_win_size)/col_nb)-3*menu_cell_padding;
            for (int r=0;r<4;r++) {
                ImGui::TableNextRow(0,cell_size);
                for (int c=0;c<col_nb;c++) {
                    ImGui::TableSetColumnIndex(c);
                    
                    bool isActive=activeFx&(1<<(r*4+c));
                    int ret=buildSubMenu(r,
                                         c,
                                         col_nb,
                                         isActive,
                                         cell_size,
                                         cell_size,
                                         current_txtMenuHandle,
                                         currentMenuLabel,
                                         currentMenuDynLabel,
                                         currentMenuLabelFAIcon,
                                         currentMenuVar);
                    if ((menushow==-1)&&((c*16+r)==0x33)) ret=1; //force exit
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
                            case 0x02: //Font size
                                settings[GLOB_FXMODPattern_FontSize].detail.mdz_switch.switch_value=(settings[GLOB_FXMODPattern_FontSize].detail.mdz_switch.switch_value+1)%settings[GLOB_FXMODPattern_FontSize].detail.mdz_switch.switch_value_nb;
                                break;
                            case 0x12:
                                break;
                            case 0x22:
                                break;
                            case 0x32: //Fullscreen switch
                                settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value=!(settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value);
                                break;
                                break;
                            case 0x03: //Current font
                                settings[GLOB_FXMODPattern_Font].detail.mdz_switch.switch_value=(settings[GLOB_FXMODPattern_Font].detail.mdz_switch.switch_value+1)%settings[GLOB_FXMODPattern_Font].detail.mdz_switch.switch_value_nb;
                                break;
                            case 0x13: //Go to settings - visu
                                keepOpened=2;
                                break;
                            case 0x23: //Back to main menu
                                pMenu_state.menu_idx=MENU_ROOT;
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
    } else if (pMenu_state.menu_idx==MENU_PROJECTM) {
        int col_nb=menuProjectMColNb;
        if (ImGui::BeginTable("menu_ProjectM",col_nb,flagTable)) {
            current_txtMenuHandle=txtMenuProjectMHandle;
            currentMenuLabel=menuProjectMLabel;
            currentMenuLabelFAIcon=menuProjectMLabelFAIcon;
            currentMenuVar=menuProjectMVar;
            currentMenuDynLabel=menuProjectMDynLabel;
            cell_size=round((menu_win_size)/col_nb)-3*menu_cell_padding;
                for (int r=0;r<4;r++) {
                    ImGui::TableNextRow(0,cell_size);
                    for (int c=0;c<col_nb;c++) {
                        ImGui::TableSetColumnIndex(c);
                        
                        bool isActive=activeFx&(1<<(r*4+c));
                        int ret=buildSubMenu(r,
                                             c,
                                             col_nb,
                                             isActive,
                                             cell_size,
                                             cell_size,
                                             current_txtMenuHandle,
                                             currentMenuLabel,
                                             currentMenuDynLabel,
                                             currentMenuLabelFAIcon,
                                             currentMenuVar);
                        if ((menushow==-1)&&((c*16+r)==0x33)) ret=1; //force exit
                    if (ret) {
                        switch (c*16+r) {
                            case 0x00: //PROJECTM OFF
                                settings[PROJECTM_FXONOFF].detail.mdz_boolswitch.switch_value=0;
                                break;
                            case 0x10: //PROJECTM ON
                                settings[PROJECTM_FXONOFF].detail.mdz_boolswitch.switch_value=1;
                                break;
                            case 0x20: //Show preset's name
                                if (settings[PROJECTM_ShowPresetLabel].detail.mdz_switch.switch_value==1) settings[PROJECTM_ShowPresetLabel].detail.mdz_switch.switch_value=0;
                                else settings[PROJECTM_ShowPresetLabel].detail.mdz_switch.switch_value=1;
                                pmSoftReinit(false);
                                break;
                            case 0x30:// Show temporarly preset's name
                                if (settings[PROJECTM_ShowPresetLabel].detail.mdz_switch.switch_value==2) settings[PROJECTM_ShowPresetLabel].detail.mdz_switch.switch_value=0;
                                else {
                                    settings[PROJECTM_ShowPresetLabel].detail.mdz_switch.switch_value=2;
                                    _pmPresetUpdateDisplayInfo=true; //Force a (re)display
                                }
                                pmSoftReinit(false);
                                break;
                            case 0x01://Bundled presets switch
                                if (settings[PROJECTM_BundledPresets].detail.mdz_boolswitch.switch_value) settings[PROJECTM_BundledPresets].detail.mdz_boolswitch.switch_value=0;
                                else settings[PROJECTM_BundledPresets].detail.mdz_boolswitch.switch_value=1;
                                pmSoftReinit(false);
                                break;
                            case 0x11://Custom presets switch
                                if (settings[PROJECTM_CustomPresets].detail.mdz_boolswitch.switch_value) settings[PROJECTM_CustomPresets].detail.mdz_boolswitch.switch_value=0;
                                else settings[PROJECTM_CustomPresets].detail.mdz_boolswitch.switch_value=1;
                                pmSoftReinit(false);
                                break;
                            case 0x21://Shuffle switch
                                settings[PROJECTM_AutoSwitchPresetsMode].detail.mdz_switch.switch_value=!settings[PROJECTM_AutoSwitchPresetsMode].detail.mdz_switch.switch_value;
                                pmSoftReinit(false);
                                break;
                            case 0x31:
                                if (settings[PROJECTM_BlendPresets].detail.mdz_boolswitch.switch_value) settings[PROJECTM_BlendPresets].detail.mdz_boolswitch.switch_value=0;
                                else settings[PROJECTM_BlendPresets].detail.mdz_boolswitch.switch_value=1;
                                pmSoftReinit(false);
                                break;
                            case 0x02://Bundled presets playlist editor
                                if (settings[PROJECTM_BundledPresets].detail.mdz_boolswitch.switch_value) {
                                    pMenu_fullscreenStatus=settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value;
                                    settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value=1;
                                    pmCurrentPlaylistMode=PM_BUNDLED_PLAYLIST;
                                    pmCurrentFileNode=pmBundledPresetsFileNode;
                                    pMenu_PMInitTempData(pmCurrentFileNode);
                                    selectedMode=0;
                                    pMenu_state.menu_idx=MENU_PROJECTM_EXPLORE;
                                }
                                break;
                            case 0x12://Custom presets playlist editor
                                if (settings[PROJECTM_CustomPresets].detail.mdz_boolswitch.switch_value) {
                                    pMenu_fullscreenStatus=settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value;
                                    settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value=1;
                                    pmCurrentPlaylistMode=PM_CUSTOM_PLAYLIST;
                                    pmCurrentFileNode=pmCustomPresetsFileNode;
                                    pMenu_PMInitTempData(pmCurrentFileNode);
                                    selectedMode=0;
                                    pMenu_state.menu_idx=MENU_PROJECTM_EXPLORE;
                                }
                                break;
                            case 0x22: //lock switch
                                if (settings[PROJECTM_LockPreset].detail.mdz_boolswitch.switch_value) settings[PROJECTM_LockPreset].detail.mdz_boolswitch.switch_value=0;
                                else settings[PROJECTM_LockPreset].detail.mdz_boolswitch.switch_value=1;
                                pmSoftReinit(false);
                                break;
                            case 0x32: //Fullscreen switch
                                settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value=!(settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value);
                                break;
                            case 0x03:
                                break;
                            case 0x13: //Go to settings - PROJECTM
                                keepOpened=4;
                                break;
                            case 0x23: //Back to main menu
                                pMenu_state.menu_idx=MENU_ROOT;
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
    } else if (pMenu_state.menu_idx==MENU_PROJECTM_EXPLORE) {
        int expandCollapseMode=0;
        if (selectedMode&PMENU_PMEXPLORE_FAV_FLAG) activeFx|=1<<2;
        if (selectedMode&PMENU_PMEXPLORE_SEL_FLAG) activeFx|=1<<9;
        ImGui::Text("Select active %s presets",(pmCurrentPlaylistMode==PM_BUNDLED_PLAYLIST?"bundled":"custom"));
        int col_nb=menuProjectMExploreColNb;
        if (ImGui::BeginTable("menu_ProjectM_Explore",col_nb,flagTable)) {
            settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value=1;
            current_txtMenuHandle=txtMenuProjectMExploreHandle;
            currentMenuLabel=menuProjectMExploreLabel;
            currentMenuLabelFAIcon=menuProjectMExploreLabelFAIcon;
            currentMenuVar=menuProjectMExploreVar;
            currentMenuDynLabel=NULL;
            cell_size=round((menu_win_size)/col_nb)-3*menu_cell_padding;
            float new_cell_h=ImGui::GetTextLineHeight()*2.2f;
            for (int r=0;r<2;r++) {
                ImGui::TableNextRow(0,new_cell_h);
                for (int c=0;c<col_nb;c++) {
                    ImGui::TableSetColumnIndex(c);
                    
                    bool isActive=activeFx&(1<<(r*col_nb+c));
                    int ret=buildSubMenu(r,
                                         c,
                                         col_nb,
                                         isActive,
                                         cell_size,
                                         new_cell_h,
                                         current_txtMenuHandle,
                                         currentMenuLabel,
                                         currentMenuDynLabel,
                                         currentMenuLabelFAIcon,
                                         currentMenuVar);
                    if ((menushow==-1)&&((c*16+r)==0x61)) ret=1; //force exit
                    if (ret) {
                        switch (c*16+r) {
                            case 0x00: //Clear all
                                pMenu_PMPresetsRemAll(pmCurrentFileNode);
                                break;
                            case 0x10: //Add all
                                pMenu_PMPresetsSelAll(pmCurrentFileNode);
                                break;
                            case 0x20: //Favorites
                                selectedMode^=1<<0;
                                break;
                            case 0x30: //Expand
                                expandCollapseMode=1;
                                break;
                            case 0x40: //TBD
                                break;
                            case 0x50: //Refresh
                                //if custom presets,rescan dir
                                if (pmCurrentFileNode==pmCustomPresetsFileNode) {
                                    updatePresetCustomDirStructure();
                                    pmCurrentFileNode=pmCustomPresetsFileNode;
                                }
                                [_mdzPM_playlist updateFileNodeStatus:pmCurrentFileNode];
                                
                                if (pmCurrentFileNode==pmCustomPresetsFileNode) [_mdzPM_Favorites updateFileNodeStatus:pmCurrentFileNode type:MDZ_PLAYLIST_FNODE_Custom];
                                else [_mdzPM_Favorites updateFileNodeStatus:pmCurrentFileNode type:MDZ_PLAYLIST_FNODE_Bundle];
                                
                                pMenu_PMInitTempData(pmCurrentFileNode);
                                break;
                            case 0x60: //Apply
                                pMenu_PMCommitTempData(pmCurrentFileNode);
                                pmSoftReinit(true);
                                [_mdzPM_playlist updateFileNodeStatus:pmCurrentFileNode];
                                if (pmCurrentFileNode==pmCustomPresetsFileNode) [_mdzPM_Favorites updateFileNodeStatus:pmCurrentFileNode type:MDZ_PLAYLIST_FNODE_Custom];
                                else [_mdzPM_Favorites updateFileNodeStatus:pmCurrentFileNode type:MDZ_PLAYLIST_FNODE_Bundle];
                                
                                pMenu_PMInitTempData(pmCurrentFileNode);
                                break;
                            case 0x01: //Add filtered
                                pMenu_PMPresetsSelFiltered(pmCurrentFileNode,selectedMode,pmFileNodeFilter[0]);
                                break;
                            case 0x11: //Remove filtered
                                pMenu_PMPresetsRemFiltered(pmCurrentFileNode,selectedMode,pmFileNodeFilter[0]);
                                break;
                            case 0x21: //Current selection
                                selectedMode^=1<<1;
                                break;
                            case 0x31: //Collapse
                                expandCollapseMode=2;
                                break;
                            case 0x41: //TBD
                                expandCollapseMode=2;
                                break;
                            case 0x51: //Back to main menu
                                settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value=pMenu_fullscreenStatus;
                                pMenu_state.menu_idx=MENU_PROJECTM;
                                break;
                            case 0x61: //Exit
                                settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value=pMenu_fullscreenStatus;
                                keepOpened=0;
                                break;
                        }
                    }
                }
            }
            ImGui::EndTable();
            
            browserFontSize=idealFontSize*0.8f;
            if (browserFontSize<PL_MIN_BROWSE_FONT_SIZE) browserFontSize=PL_MIN_BROWSE_FONT_SIZE;
            if (font_menu) ImGui::PushFont(font_menu,browserFontSize*glScaleFactor);
            else ImGui::PushFont(nullptr);
            
            browserFontWidth=ImGui::CalcTextSize("abcdefgh").x/8.0;
            
            if (ImGui::Button("X")) {
                pmFileNodeFilter[0]=0;
            }
            ImGui::SameLine();
            ImGui::InputText("Filter", pmFileNodeFilter, 64);
            
            
            
            ImVec2 pos=ImGui::GetCursorPos();
            float winTreeNodeHeight=menu_win_sizeH-pos.y;
            ImGui::BeginChild("Modizer menu pm explore subwin",ImVec2(menu_win_size,winTreeNodeHeight));
            
            int index=0;
            bool filter=false;
            if (pmFileNodeFilter[0]) {
                filter=true;
                NSString *strFilter=[NSString stringWithUTF8String:pmFileNodeFilter];
                [pmCurrentFileNode filterNodes:strFilter filterDir:true];
            }
            
            //update status consistency / tree
            pMenu_PMUpdateSelStatus(pmCurrentFileNode,FALSE,FALSE);
            //update fav status consistency / tree
            pMenu_PMUpdateFavStatus(pmCurrentFileNode,FALSE,FALSE);
            
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0,0,0,0));
            ImGui::PushStyleColor(ImGuiCol_Button,pMenu_browser_quickAccessButton);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,pMenu_browser_quickAccessButtonH);
            
            pMenu_currentPM_entry=[_mdzPM_playlist getCurFullpathNS];
            
            float drawLineHeight=ImGui::GetTextLineHeightWithSpacing();
            float drawMinY=ImGui::GetScrollY()-drawLineHeight;
            float drawMaxY=drawMinY+winTreeNodeHeight+drawLineHeight;
            
            pMenu_TreeNodeLines=0;
            index=pMenu_PMbuildDirTree(pmCurrentFileNode,index,filter,expandCollapseMode,selectedMode,drawMinY,drawMaxY,drawLineHeight);
            expandCollapseMode=0;  //Reset flag
            
            ImGui::PopStyleColor();
            ImGui::PopStyleColor();
            ImGui::PopStyleColor();
            
            if (mouseMoveInProgress) {
                menu_scrollX[pMenu_state.menu_idx]-=panX;
                if (menu_scrollX[pMenu_state.menu_idx]<0) menu_scrollX[pMenu_state.menu_idx]=0;
                if (menu_scrollX[pMenu_state.menu_idx]>ImGui::GetScrollMaxX()) menu_scrollX[pMenu_state.menu_idx]=ImGui::GetScrollMaxX();
                
                menu_scrollY[pMenu_state.menu_idx]-=panY;
                if (menu_scrollY[pMenu_state.menu_idx]>ImGui::GetScrollMaxY()) menu_scrollY[pMenu_state.menu_idx]=ImGui::GetScrollMaxY();
                if (menu_scrollY[pMenu_state.menu_idx]<0) menu_scrollY[pMenu_state.menu_idx]=0;
                
                ImGui::SetScrollX(menu_scrollX[pMenu_state.menu_idx]*glScaleFactor);
                ImGui::SetScrollY(menu_scrollY[pMenu_state.menu_idx]*glScaleFactor);
            } else {
                menu_scrollX[pMenu_state.menu_idx]=ImGui::GetScrollX()/glScaleFactor;
                menu_scrollY[pMenu_state.menu_idx]=ImGui::GetScrollY()/glScaleFactor;
            }
            
            ImGui::EndChild();
            ImGui::PopFont();
        }
    }
    
    ImGui::PopFont();
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
    ImGui::EndChild();
    ImGui::End();
    ImGui::PopStyleColor();
    
    //Global var mirroring
    settings[GLOB_FXAlpha].detail.mdz_slider.slider_value=global_FXAlpha/100.0;
    settings[GLOB_FXMODPattern_BGAlpha].detail.mdz_slider.slider_value=global_MODPatOpacity/100.0;
    
    return keepOpened;
}

std::string TruncateText(const std::string& p_text, float p_truncated_width) {
    std::string truncated_text = p_text;

    const float text_width =
            ImGui::CalcTextSize(p_text.c_str(), nullptr, true).x;

    if (text_width > p_truncated_width) {
        constexpr const char* ELLIPSIS = "...";
        const float ellipsis_size = ImGui::CalcTextSize(ELLIPSIS).x;

        int visible_chars = 0;
        for (size_t i = 0; i < p_text.size(); i++) {
            const float current_width = ImGui::CalcTextSize(
                    p_text.substr(0, i).c_str(), nullptr, true)
                                                .x;
            if (current_width + ellipsis_size > p_truncated_width) {
                break;
            }

            visible_chars = i;
        }

        //truncated_text = (p_text.substr(0, visible_chars) + ELLIPSIS).c_str();
        int visible_chars_left=floor(visible_chars/2);
        int visible_chars_right=ceil(visible_chars/2);
        truncated_text = (p_text.substr(0, visible_chars_left) + ELLIPSIS + p_text.substr(p_text.length()-visible_chars_right, visible_chars_right)).c_str();
    }

    return truncated_text;
}

NSString *limitStrSize(NSString *str,int width) {
    //std::string tmpStr=std::string([str UTF8String]);
    //std::string truncStr=TruncateText(tmpStr,width);
    //return [NSString stringWithUTF8String:truncStr.c_str()];
    if ([str length]<3) return str;
    if ([str length]>width) {
        int middle=width/2;
        int left=middle-1;
        int right=middle+1;
        NSString *strLeft=[str substringToIndex:middle-1];
        NSString *strRight=[str substringToIndex:middle+1];
        return [NSString stringWithFormat:@"%@...%@",strLeft,strRight];
    }
    return str;
}

int pMenu_PMbuildDirTree(FileNode *fileNode, int idx,bool filter,int updExpandCollapse,int selectedMode,float drawMinY,float drawMaxY,float drawLineHeight) {
    int flags_default=ImGuiTreeNodeFlags_SpanFullWidth;
    if (filter||selectedMode) {
        //open all nodes by default
        flags_default|=ImGuiTreeNodeFlags_DefaultOpen;
    }
    
    for (FileNode *child in fileNode.children) {
        if (child.isDirectory) {
            //Directory
            bool skipentry=false;
            if (filter) {
                if (!child.isMatchingFilter) skipentry=true;
            }
            if (selectedMode&PMENU_PMEXPLORE_FAV_FLAG) {
                if (!child.isFavorite_Temp) skipentry=true;
            }
            if (selectedMode&PMENU_PMEXPLORE_SEL_FLAG) {
                if (!child.isSelected_Temp) skipentry=true;
            }
            
            if (!skipentry) {
                //increase number of lines counter
                pMenu_TreeNodeLines++;
                
                float yPos=ImGui::GetCursorPosY();
                bool isVisible=(yPos>=drawMinY)&&(yPos<=drawMaxY);
                
                //Matching filter, if any
                int flags=flags_default|ImGuiTreeNodeFlags_OpenOnArrow;
                
                if (child.isSelected_Temp) flags|=ImGuiTreeNodeFlags_Selected;
                
                //if ( !child.isFullySelected ) ImGui::PushStyleColor(ImGuiCol_Header, pMenu_browser_partiallySelectedLine);
                
                switch (updExpandCollapse) {
                    default:
                    case 0:break;
                    case 1: //expand
                        ImGui::SetNextItemOpen(true);
                        break;
                    case 2: //collapse
                        ImGui::SetNextItemOpen(false);
                        break;
                }
                
                bool node_open=ImGui::TreeNodeEx((void*)(intptr_t)idx++, flags, "");
                if (isVisible) {
                    
                    NSString *strNode;
                    if (child.isFavorite_Temp) {
                        if (child.isFullyFavorite) strNode=[NSString stringWithFormat:@"%C%@",static_cast<unichar>(FA_STAR),[child name]];
                        else strNode=[NSString stringWithFormat:@"%C%@",static_cast<unichar>(FA_STAR_HALF_O),[child name]];
                    } else strNode=[NSString stringWithFormat:@"%C%@",static_cast<unichar>(FA_STAR_O),[child name]];
                    
                    if (!mouseMoveInProgress && ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
                        //Click detected
                        //if selected and not fully, force fully
                        //if not remove selected status
                        if (child.isSelected_Temp) {
                            if (child.isFullySelected) child.isSelected_Temp=FALSE;
                            else child.isSelected_Temp=TRUE;
                        } else child.isSelected_Temp=TRUE;
                        child.shouldPropagateStatus=TRUE;
                    }
                    
                    
                    
                    ImGui::SameLine();
                    
                    ImVec2 cpos=ImGui::GetCursorPos();
                    ImVec2 wsize=ImGui::GetWindowSize();
                    int max_char=(wsize.x-cpos.x-8*glScaleFactor)/browserFontWidth;
                    strNode=limitStrSize(strNode,max_char);
                    
                    if (child.isFullySelected) ImGui::TextColored(pMenu_browser_selectedLineText, "%s",[strNode UTF8String]);
                    else if (child.isSelected_Temp) ImGui::TextColored(pMenu_browser_partiallySelectedLineText, "%s",[strNode UTF8String]);
                    else ImGui::TextColored(pMenu_browser_notSelectedLineText,"%s",[strNode UTF8String]);
                    
                    if (node_open) {
                        idx=pMenu_PMbuildDirTree(child,idx,filter,updExpandCollapse,selectedMode,drawMinY,drawMaxY,drawLineHeight);
                        ImGui::TreePop();
                    }
                } else {
                    //ImGui::Text(" ");
                    //ImGui::Dummy(ImVec2(10.0f,drawLineHeight));
                    //Matching filter, if any
                    if (node_open) {
                        idx=pMenu_PMbuildDirTree(child,idx,filter,updExpandCollapse,selectedMode,drawMinY,drawMaxY,drawLineHeight);
                        ImGui::TreePop();
                    }
                }
                
            }
        } else {
            // File
            bool skipentry=false;
            if (filter) {
                if (!child.isMatchingFilter) skipentry=true;
            }
            if (selectedMode&PMENU_PMEXPLORE_FAV_FLAG) {
                if (!child.isFavorite_Temp) skipentry=true;
            }
            if (selectedMode&PMENU_PMEXPLORE_SEL_FLAG) {
                if (!child.isSelected_Temp) skipentry=true;
            }
            
            if (!skipentry) {
                pMenu_TreeNodeLines++;
                
                float yPos=ImGui::GetCursorPosY();
                bool isVisible=(yPos>=drawMinY)&&(yPos<=drawMaxY);
                
                //Matching filter, if any
                int flags=flags_default|ImGuiTreeNodeFlags_Leaf;
                
                if (child.isSelected_Temp) {
                    flags|=ImGuiTreeNodeFlags_Selected;
                }
                
                if (isVisible) {
                    NSString *strNode;
                    strNode=[NSString stringWithFormat:@"%@",[child name]];
                    
                    bool node_open=ImGui::TreeNodeEx((void*)(intptr_t)idx++, flags|ImGuiTreeNodeFlags_AllowOverlap, " ");
                    bool shouldUpdateSel=false;
                    if (!mouseMoveInProgress && ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
                        shouldUpdateSel=true;
                    }
                    if ( ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0) ) {
                        //                    [_mdzPM_playlist loadPreset:child cut:true];
                    }
                    
                    
                
                
                    ImGui::SameLine();
                    ImVec2 curPos=ImGui::GetCursorPos();
                    curPos.x=2;
                    ImGui::SetCursorPos(curPos);
                    //If clicking the button, do no register click for the node
                    if (ImGui::Button(faicon_with_pre_suf(" ",FA_EYE," "))) {
                        [_mdzPM_playlist loadPreset:child cut:true];
                        shouldUpdateSel=false;
                    }
                    //If still above button, do no register click for the node
                    if ( ImGui::IsItemHovered()  ) {
                        shouldUpdateSel=false;
                    }
                    
                    ImGui::SameLine();
                    if (child.isSelected) {
                        //If clicking the button, do no register click for the node
                        if (ImGui::Button(faicon_with_pre_suf(" ",FA_HAND_O_RIGHT," "))) {
                            [_mdzPM_playlist moveTo:child cut:true];
                            shouldUpdateSel=false;
                        }
                        //If still above button, do no register click for the node
                        if ( ImGui::IsItemHovered()  ) {
                            shouldUpdateSel=false;
                        }
                    }
                    
                    ImGui::SameLine();
                    if (child.isFavorite_Temp) {
                        //If clicking the button, do no register click for the node
                        ImGui::PushStyleColor(ImGuiCol_Button,pMenu_browser_isFav);
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,pMenu_browser_isFavH);
                        if (ImGui::Button(faicon_with_pre_suf(" ",FA_STAR," "))) {
                            shouldUpdateSel=false;
                            child.isFavorite_Temp=0;
                            NSString *title=[NSString stringWithFormat:@"(%c)%@",(child.presetType==MDZ_PLAYLIST_FNODE_Bundle?'B':'C'),child.localpath];
                            //MDZILog("removing %@",title);
                            [_mdzPM_Favorites remFavoritePreset:title];
                        }
                        ImGui::PopStyleColor();
                        ImGui::PopStyleColor();
                        //If still above button, do no register click for the node
                        if ( ImGui::IsItemHovered()  ) {
                            shouldUpdateSel=false;
                        }
                    } else {
                        //If clicking the button, do no register click for the node
                        if (ImGui::Button(faicon_with_pre_suf(" ",FA_STAR_O," "))) {
                            shouldUpdateSel=false;
                            child.isFavorite_Temp=1;
                            NSString *title=[NSString stringWithFormat:@"(%c)%@",(child.presetType==MDZ_PLAYLIST_FNODE_Bundle?'B':'C'),child.localpath];
                            //MDZILog("adding %@",title);
                            [_mdzPM_Favorites addFavoritePreset:title];
                        }
                        //If still above button, do no register click for the node
                        if ( ImGui::IsItemHovered()  ) {
                            shouldUpdateSel=false;
                        }
                    }
                    
                    if (shouldUpdateSel) child.isSelected_Temp=!child.isSelected_Temp;
                    ImGui::SameLine();
                    
                    ImVec2 cpos=ImGui::GetCursorPos();
                    ImVec2 wsize=ImGui::GetWindowSize();
                    int max_char=(wsize.x-cpos.x-8*glScaleFactor)/browserFontWidth;
                    strNode=limitStrSize(strNode,max_char);
                    
                    if ([pMenu_currentPM_entry isEqualToString:child.localpath]) {
                        if (child.isSelected_Temp) ImGui::TextColored(pMenu_browser_selectedLineTextPlaying, "%s",[strNode UTF8String]);
                        else ImGui::TextColored(pMenu_browser_notSelectedLineTextPlaying,"%s",[strNode UTF8String]);
                    } else {
                        if (child.isSelected_Temp) ImGui::TextColored(pMenu_browser_selectedLineText, "%s",[strNode UTF8String]);
                        else ImGui::TextColored(pMenu_browser_notSelectedLineText,"%s",[strNode UTF8String]);
                    }
                    
                    if (node_open) {
                        ImGui::TreePop();
                    }
                } else {
                    //ImGui::Text(" ");
                    ImGui::Dummy(ImVec2(10.0f,drawLineHeight));
                    idx++;
                }
            }
        }
    }
    return idx;
}

int pMenu_PMUpdateFavStatus(FileNode *fnode,bool propagateStatus,bool favStatus) {
    int ret=0;
    
    if ( !fnode.isDirectory ) {
        //Just a file, return favorite status
        
        if (propagateStatus) fnode.isFavorite_Temp=favStatus;
        
        if (fnode.isFavorite_Temp) {
            ret=1;
            fnode.isFullyFavorite=1;
        } else fnode.isFullyFavorite=0;
    } else {
        //directory
        bool partial=false;
        
        if (propagateStatus) {
            fnode.isFavorite_Temp=favStatus;
            fnode.shouldPropagateStatus=true;
        }
        
        for (FileNode *child in fnode.children) {
            if (pMenu_PMUpdateFavStatus(child,fnode.shouldPropagateStatus,fnode.isFavorite_Temp)) {
                //Child is favorite, increase counted
                ret++;
            }
            //check if child is partially favorite, if so propagate
            if (!child.isFullyFavorite) partial=true;
        }
        //Update how many children are favorites
        fnode.favoriteChildren = ret;
        
        //Remove propagate status
        fnode.shouldPropagateStatus=false;
        
        //Update dir favorite flag
        if (fnode.favoriteChildren==0) fnode.isFavorite_Temp = FALSE;
        else fnode.isFavorite_Temp = TRUE;
        
        //Update fullyFavorite flag accordingly, depend on status of children / partially favorite or not
        if (!partial && fnode.favoriteChildren==[fnode.children count]) {
            //all are favorites
            fnode.isFullyFavorite = TRUE;
        } else {
            fnode.isFullyFavorite = FALSE;
        }
    }
    return ret;
}


int pMenu_PMUpdateSelStatus(FileNode *fnode,bool propagateStatus,bool selStatus) {
    int ret=0;
    
    if ( !fnode.isDirectory ) {
        //Just a file, return selected status
        
        if (propagateStatus) fnode.isSelected_Temp=selStatus;
        
        if (fnode.isSelected_Temp) {
            ret=1;
            fnode.isFullySelected=1;
        } else fnode.isFullySelected=0;
    } else {
        //directory
        bool partial=false;
        
        if (propagateStatus) {
            fnode.isSelected_Temp=selStatus;
            fnode.shouldPropagateStatus=true;
        }
        
        for (FileNode *child in fnode.children) {
            if (pMenu_PMUpdateSelStatus(child,fnode.shouldPropagateStatus,fnode.isSelected_Temp)) {
                //Child is selected, increase counted
                ret++;
            }
            //check if child is partially selected, if so propagate
            if (!child.isFullySelected) partial=true;
        }
        //Update how many children are selected
        fnode.selectedChildren = ret;
        
        //Remove propagate status
        fnode.shouldPropagateStatus=false;
        
        //Update dir selected flag
        if (fnode.selectedChildren==0) fnode.isSelected_Temp = FALSE;
        else fnode.isSelected_Temp = TRUE;
        
        //Update fullyselected flag accordingly, depend on status of children / partially selected or not
        if (!partial && fnode.selectedChildren==[fnode.children count]) {
            //all are selected
            fnode.isFullySelected = TRUE;
        } else {
            fnode.isFullySelected = FALSE;
        }
    }
    return ret;
}

    
    
int pMenu_PMPresetsSelAll(FileNode *fnode) {
    int ret=0;
    fnode.isSelected_Temp=true;
    ret++;
    for (FileNode *child in fnode.children) {
        ret+=pMenu_PMPresetsSelAll(child);
    }
    return ret;
}
int pMenu_PMPresetsRemAll(FileNode *fnode) {
    int ret=0;
    fnode.isSelected_Temp=false;
    ret++;
    for (FileNode *child in fnode.children) {
        ret+=pMenu_PMPresetsRemAll(child);
    }
    return ret;
}
int pMenu_PMPresetsSelFiltered(FileNode *fnode,int selectedMode,bool filterMode){
    int ret=0;
    bool filterCheck;
    if (!filterMode) filterCheck=true;
    else filterCheck=fnode.isMatchingFilter;
    bool favCheck;
    if (!(selectedMode&PMENU_PMEXPLORE_FAV_FLAG)) favCheck=true;
    else favCheck=fnode.isFavorite;
    bool selCheck;
    if (!(selectedMode&PMENU_PMEXPLORE_SEL_FLAG)) selCheck=true;
    else selCheck=fnode.isSelected;
    if (filterCheck && favCheck && selCheck) {
        fnode.isSelected_Temp=true;
        ret++;
    }
    for (FileNode *child in fnode.children) {
        ret+=pMenu_PMPresetsSelFiltered(child,selectedMode,filterMode);
    }
    return ret;
}
int pMenu_PMPresetsRemFiltered(FileNode *fnode,int selectedMode,bool filterMode) {
    int ret=0;
    bool filterCheck;
    if (!filterMode) filterCheck=true;
    else filterCheck=fnode.isMatchingFilter;
    bool favCheck;
    if (!(selectedMode&PMENU_PMEXPLORE_FAV_FLAG)) favCheck=true;
    else favCheck=fnode.isFavorite;
    bool selCheck;
    if (!(selectedMode&PMENU_PMEXPLORE_SEL_FLAG)) selCheck=true;
    else selCheck=fnode.isSelected;
    if (filterCheck && favCheck && selCheck) {
        fnode.isSelected_Temp=false;
        ret++;
    }
    for (FileNode *child in fnode.children) {
        ret+=pMenu_PMPresetsRemFiltered(child,selectedMode,filterMode);
    }
    return ret;
}

void pMenu_PMInitTempData(FileNode *fnode) {
    fnode.isSelected_Temp=fnode.isSelected;
    fnode.isFavorite_Temp=fnode.isFavorite;
    for (FileNode *child in fnode.children) pMenu_PMInitTempData(child);
}

void pMenu_PMCommitTempData(FileNode *fnode) {
    fnode.isSelected=fnode.isSelected_Temp;
    fnode.isFavorite=fnode.isFavorite_Temp;
    for (FileNode *child in fnode.children) pMenu_PMCommitTempData(child);
}

}
