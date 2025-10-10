//
//  PlayerMenu.mm
//  modizer
//
//  Created by Yohann Magnien David on 10/10/2025.
//

#include "PlayerMenu.h"
#include "SettingsGenViewController.h"
#include "TextureUtils.h"

extern ImFont *font_body;
extern volatile t_settings settings[MAX_SETTINGS];

namespace PMenu {

static GLuint txtMenuHandle[16];
static GLuint txtSubMenuHandle[41];
const char *menuRootLabel[16]={
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,"Fullscreen",NULL,"Exit"
};

static bool pMenu_isInitialized=false;

enum PMenu_Menu_List {
    MENU_ROOT,
    MENU_OSCILLO,
    MENU_SPECTRUM,
    MENU_3DSPECTRUM,
    MENU_FX2,
    MENU_PIANO,
    MENU_MIDIPATTERN,
    MENU_MODPATTERN,
    MENU_MILKDROP
};

struct {
    int menu_idx;
} pMenu_state;

#define pMenu_getBundledResFilePath(name) [[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:name] UTF8String]

int playerGetActiveFX() {
    int active_idx=0;
    if (settings[GLOB_FXOscillo].detail.mdz_switch.switch_value) active_idx|=1<<0;
    if (settings[GLOB_FXSpectrum].detail.mdz_switch.switch_value) active_idx|=1<<1;
    if (settings[GLOB_FX3DSpectrum].detail.mdz_switch.switch_value) active_idx|=1<<2;
    if (settings[GLOB_FX2].detail.mdz_switch.switch_value) active_idx|=1<<3;
    
    if (settings[GLOB_FXPiano].detail.mdz_switch.switch_value||settings[GLOB_FXPianoRoll].detail.mdz_switch.switch_value) active_idx|=1<<4;
    if (settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_value) active_idx|=1<<5;
    if (settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value) active_idx|=1<<6;
    if (settings[GLOB_FX4].detail.mdz_boolswitch.switch_value) active_idx|=1<<7;
    
    if (settings[GLOB_FXMilkdrop].detail.mdz_switch.switch_value) active_idx|=1<<8;
    
    if (settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value) active_idx|=1<<13;
    return active_idx;
}


//------------------------------------------------------
// playerMenuInit
//   initialize system & load required data
//------------------------------------------------------
void playerMenuInit() {
    pMenu_state.menu_idx=MENU_ROOT;
    
    memset(txtMenuHandle,0,sizeof(txtMenuHandle));
    //Oscilloscope
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu5a_2x.png"), &(txtMenuHandle[0]), NULL, NULL)) {
        NSLog(@"Cannot load texture");
    }
    //Spectrum 2D
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu4a_2x.png"), &(txtMenuHandle[1]), NULL, NULL)) {
        NSLog(@"Cannot load texture");
    }
    //Spectrum 3D objects
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu12a_2x.png"), &(txtMenuHandle[2]), NULL, NULL)) {
        NSLog(@"Cannot load texture");
    }
    //Spectrum 3D landscape
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu2a_2x.png"), &(txtMenuHandle[3]), NULL, NULL)) {
        NSLog(@"Cannot load texture");
    }
    //Piano roll
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu11a_2x.png"), &(txtMenuHandle[4]), NULL, NULL)) {
        NSLog(@"Cannot load texture");
    }
    //Note scrollers
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu8a_2x.png"), &(txtMenuHandle[5]), NULL, NULL)) {
        NSLog(@"Cannot load texture");
    }
    //mod patterns
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu7a_2x.png"), &(txtMenuHandle[6]), NULL, NULL)) {
        NSLog(@"Cannot load texture");
    }
    //smoke FX
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu9_2x.png"), &(txtMenuHandle[7]), NULL, NULL)) {
        NSLog(@"Cannot load texture");
    }
    //milkdrop
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu13a_2x.png"), &(txtMenuHandle[8]), NULL, NULL)) {
        NSLog(@"Cannot load texture");
    }
    
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu0.png"), &(txtMenuHandle[12]), NULL, NULL)) {
        NSLog(@"Cannot load texture");
    }
    
    memset(txtSubMenuHandle,0,sizeof(txtSubMenuHandle));
    
#define SUBMENU0_START 0
#define SUBMENU0_SIZE 4
    //Oscilloscopes
    txtSubMenuHandle[SUBMENU0_START]=0;
    txtSubMenuHandle[SUBMENU0_START+1]=txtMenuHandle[0];
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu5b_2x.png"), &(txtSubMenuHandle[SUBMENU0_START+2]), NULL, NULL)) {
        NSLog(@"Cannot load texture");
    }
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu5c_2x.png"), &(txtSubMenuHandle[SUBMENU0_START+3]), NULL, NULL)) {
        NSLog(@"Cannot load texture");
    }
    
#define SUBMENU1_START SUBMENU0_START+SUBMENU0_SIZE
#define SUBMENU1_SIZE 3
    //Spectrum 2D
    txtSubMenuHandle[SUBMENU1_START]=0;
    txtSubMenuHandle[SUBMENU1_START+1]=txtMenuHandle[1];
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu4b_2x.png"), &(txtSubMenuHandle[SUBMENU1_START+2]), NULL, NULL)) {
        NSLog(@"Cannot load texture");
    }
    

#define SUBMENU2_START SUBMENU1_START+SUBMENU1_SIZE
#define SUBMENU2_SIZE 4
    //Spectrum 3D objects
    txtSubMenuHandle[SUBMENU2_START]=0;
    txtSubMenuHandle[SUBMENU2_START+1]=txtMenuHandle[2];
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu12b_2x.png"), &(txtSubMenuHandle[SUBMENU2_START+3]), NULL, NULL)) {
        NSLog(@"Cannot load texture");
    }
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu12c_2x.png"), &(txtSubMenuHandle[SUBMENU2_START+3]), NULL, NULL)) {
        NSLog(@"Cannot load texture");
    }
    
#define SUBMENU3_START SUBMENU2_START+SUBMENU2_SIZE
#define SUBMENU3_SIZE 9
    //Spectrum 3D landscape
    txtSubMenuHandle[SUBMENU3_START]=0;
    txtSubMenuHandle[SUBMENU3_START+1]=txtMenuHandle[3];
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu2b_2x.png"), &(txtSubMenuHandle[SUBMENU3_START+2]), NULL, NULL)) {
        NSLog(@"Cannot load texture");
    }
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu2c_2x.png"), &(txtSubMenuHandle[SUBMENU3_START+3]), NULL, NULL)) {
        NSLog(@"Cannot load texture");
    }
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu2d_2x.png"), &(txtSubMenuHandle[SUBMENU3_START+4]), NULL, NULL)) {
        NSLog(@"Cannot load texture");
    }
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu2e_2x.png"), &(txtSubMenuHandle[SUBMENU3_START+5]), NULL, NULL)) {
        NSLog(@"Cannot load texture");
    }
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu3a_2x.png"), &(txtSubMenuHandle[SUBMENU3_START+6]), NULL, NULL)) {
        NSLog(@"Cannot load texture");
    }
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu3b_2x.png"), &(txtSubMenuHandle[SUBMENU3_START+7]), NULL, NULL)) {
        NSLog(@"Cannot load texture");
    }
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu3c_2x.png"), &(txtSubMenuHandle[SUBMENU3_START+8]), NULL, NULL)) {
        NSLog(@"Cannot load texture");
    }
    
#define SUBMENU4_START SUBMENU3_START+SUBMENU3_SIZE
#define SUBMENU4_SIZE 7
    //Piano FX
    txtSubMenuHandle[SUBMENU4_START]=0;
    txtSubMenuHandle[SUBMENU4_START+1]=txtMenuHandle[4];
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu11b_2x.png"), &(txtSubMenuHandle[SUBMENU4_START+2]), NULL, NULL)) {
        NSLog(@"Cannot load texture");
    }
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu11c_2x.png"), &(txtSubMenuHandle[SUBMENU4_START+3]), NULL, NULL)) {
        NSLog(@"Cannot load texture");
    }
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu11d_2x.png"), &(txtSubMenuHandle[SUBMENU4_START+4]), NULL, NULL)) {
        NSLog(@"Cannot load texture");
    }
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu11e_2x.png"), &(txtSubMenuHandle[SUBMENU4_START+5]), NULL, NULL)) {
        NSLog(@"Cannot load texture");
    }
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu11f_2x.png"), &(txtSubMenuHandle[SUBMENU4_START+6]), NULL, NULL)) {
        NSLog(@"Cannot load texture");
    }
    
    
    
#define SUBMENU5_START SUBMENU4_START+SUBMENU4_SIZE
#define SUBMENU5_SIZE 3
    //Notes scrollers
    txtSubMenuHandle[SUBMENU5_START]=0;
    txtSubMenuHandle[SUBMENU5_START+1]=txtMenuHandle[5];
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu8b_2x.png"), &(txtSubMenuHandle[SUBMENU5_START+2]), NULL, NULL)) {
        NSLog(@"Cannot load texture");
    }
    
#define SUBMENU6_START SUBMENU5_START+SUBMENU5_SIZE
#define SUBMENU6_SIZE 7
    //Mod patterns
    txtSubMenuHandle[SUBMENU6_START]=0;
    txtSubMenuHandle[SUBMENU6_START+1]=txtMenuHandle[6];
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu7b_2x.png"), &(txtSubMenuHandle[SUBMENU6_START+2]), NULL, NULL)) {
        NSLog(@"Cannot load texture");
    }
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu7c_2x.png"), &(txtSubMenuHandle[SUBMENU6_START+3]), NULL, NULL)) {
        NSLog(@"Cannot load texture");
    }
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu7d_2x.png"), &(txtSubMenuHandle[SUBMENU6_START+4]), NULL, NULL)) {
        NSLog(@"Cannot load texture");
    }
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu7e_2x.png"), &(txtSubMenuHandle[SUBMENU6_START+5]), NULL, NULL)) {
        NSLog(@"Cannot load texture");
    }
    if (!LoadTextureFromFile(pMenu_getBundledResFilePath(@"txtMenu7f_2x.png"), &(txtSubMenuHandle[SUBMENU6_START+6]), NULL, NULL)) {
        NSLog(@"Cannot load texture");
    }
    
#define SUBMENU7_START SUBMENU6_START+SUBMENU6_SIZE
#define SUBMENU7_SIZE 2
    //Mod patterns
    txtSubMenuHandle[SUBMENU7_START]=0;
    txtSubMenuHandle[SUBMENU7_START+1]=txtMenuHandle[8];
    
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
    float menu_win_size=fmin(ww,hh)*glScaleFactor;
    float cell_size=fmin(ww,hh)*glScaleFactor/4.5f;
    
    cpt++;
    
    // Root window, full screen
    ImGui::SetNextWindowPos(ImVec2(0,0));
    ImGui::SetNextWindowSize(ImVec2(ww*glScaleFactor,hh*glScaleFactor));
    ImGui::GetStyle().Alpha=fadelev;
    ImGui::Begin("Modizer root menu",0,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoScrollbar);
    
    
    ImGui::BeginChild("Modizer menu",ImVec2(menu_win_size,menu_win_size));
    static ImGuiTableFlags flagTable = ImGuiTableFlags_Borders|ImGuiTableFlags_SizingFixedSame|ImGuiTableFlags_NoHostExtendX;
    
    //static ImGuiTableFlags sizing_policy_flags[4] = { ImGuiTableFlags_SizingFixedFit, ImGuiTableFlags_SizingFixedSame, ImGuiTableFlags_SizingStretchProp, ImGuiTableFlags_SizingStretchSame };

    if (font_body) ImGui::PushFont(font_body);
    
    int activeFx=playerGetActiveFX();

    ImGuiStyle& style = ImGui::GetStyle();
    style.FontSizeBase=18*menu_win_size/512;
    style._NextFrameFontSizeBase = style.FontSizeBase;
    
    //ImGui::PushStyleColor(ImGuiCol_TableBorderLight,ImVec4(0,0,0,1));
    //ImGui::PushStyleColor(ImGuiCol_TableBorderStrong,ImVec4(0,0,0,1));
    
    if (pMenu_state.menu_idx==MENU_ROOT) {
        if (ImGui::BeginTable("menu_root",4,flagTable)) {
            for (int r=0;r<4;r++) {
                ImGui::TableNextRow(0,cell_size);
                for (int c=0;c<4;c++) {
                    ImGui::TableSetColumnIndex(c);
                    ImGui::PushID(r*4+c);
                    bool ret=false;
                    ImVec2 uv0(0,0);
                    ImVec2 uv1(1,1);
                    ImVec4 bg_col(0,0,0,0.0f);
                    ImVec4 tint_col(0.7,0.7,0.7,1.0f);
                    float padding_val=0;
                    if (activeFx&(1<<(r*4+c))) {
                        //Active
                        tint_col.x=1.0;
                        tint_col.y=1.0;
                        tint_col.z=1.0;
                        tint_col.w=1.0f;
                        
                        padding_val=cell_size/40.0f;
                        if (txtMenuHandle[r*4+c]) ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(1,1,1,0.8f));
                        else ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0.5f,0.5f,1.0f,0.9f));
                        
                    } else {
                        //Inactive
                        if (txtMenuHandle[r*4+c]) ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(1.0,1.0,1.0,0.3f));
                        else ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0.7f,0.7f,1.0f,0.5f));
                    }
                    ImVec2 padding(padding_val,padding_val);
                    
                    
                    if (txtMenuHandle[r*4+c]) {
                        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,padding);
                        ret=ImGui::ImageButton("",(ImTextureID)(intptr_t)txtMenuHandle[r*4+c], ImVec2(cell_size-padding_val*2, cell_size-padding_val*2),uv0,uv1,bg_col,tint_col);
                        ImGui::PopStyleVar();
                    }
                    else if (menuRootLabel[r*4+c]) ret=ImGui::Button(menuRootLabel[r*4+c],ImVec2(cell_size, cell_size));
                    
                    ImGui::PopStyleColor();
                    if (ret) {
//                        NSLog(@"press: %dx%d",c,r);
                        switch (c*16+r) {
                            case 0x00:
                                pMenu_state.menu_idx=MENU_OSCILLO;
//                                    viewTapHelpShow_SubStart=SUBMENU0_START;
//                                    viewTapHelpShow_SubNb=SUBMENU0_SIZE;
                                break;
                            case 0x10:
                                pMenu_state.menu_idx=MENU_SPECTRUM;
//                                    viewTapHelpShow_SubStart=SUBMENU1_START;
//                                    viewTapHelpShow_SubNb=SUBMENU1_SIZE;
                                break;
                            case 0x20:
                                pMenu_state.menu_idx=MENU_3DSPECTRUM;
//                                    viewTapHelpShow_SubStart=SUBMENU2_START;
//                                    viewTapHelpShow_SubNb=SUBMENU2_SIZE;
                                break;
                            case 0x30:
                                pMenu_state.menu_idx=MENU_FX2;
//                                    viewTapHelpShow_SubStart=SUBMENU3_START;
//                                    viewTapHelpShow_SubNb=SUBMENU3_SIZE;
                                break;
                            case 0x01:
                                pMenu_state.menu_idx=MENU_PIANO;
//                                    viewTapHelpShow_SubStart=SUBMENU4_START;
//                                    viewTapHelpShow_SubNb=SUBMENU4_SIZE;
                                break;
                            case 0x11:
                                pMenu_state.menu_idx=MENU_MIDIPATTERN;
//                                    viewTapHelpShow_SubStart=SUBMENU5_START;
//                                    viewTapHelpShow_SubNb=SUBMENU5_SIZE;
                                break;
                            case 0x21:
                                pMenu_state.menu_idx=MENU_MODPATTERN;
//                                    viewTapHelpShow_SubStart=SUBMENU6_START;
//                                    viewTapHelpShow_SubNb=SUBMENU6_SIZE;
                                break;
                            case 0x31: {
                                int val=settings[GLOB_FX4].detail.mdz_boolswitch.switch_value;
                                val++;
                                if (val>=2) val=0;
                                settings[GLOB_FX4].detail.mdz_boolswitch.switch_value=val;
                                if (val) settings[GLOB_FX2].detail.mdz_boolswitch.switch_value=0;
                            }
                                break;
                            case 0x02:
                                pMenu_state.menu_idx=MENU_MILKDROP;
//                                    viewTapHelpShow_SubStart=SUBMENU7_START;
//                                    viewTapHelpShow_SubNb=SUBMENU7_SIZE;
                                break;
                            case 0x03:
                                //HIDE FX Screen
                                keepOpened=-1;
                                break;
                            case 0x13:
                                //Fullscreen switch
                                //oglViewFullscreen^=1;
                                settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value=!(settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value);
                                break;
                            case 0x23:
                                //ALL FX Off
                                settings[GLOB_FX2].detail.mdz_switch.switch_value=0;
                                settings[GLOB_FX4].detail.mdz_boolswitch.switch_value=0;
                                settings[GLOB_FXPianoRoll].detail.mdz_switch.switch_value=0;
                                settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_value=0;
                                settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value=0;
                                settings[GLOB_FXSpectrum].detail.mdz_switch.switch_value=0;
                                settings[GLOB_FXOscillo].detail.mdz_switch.switch_value=0;
                                settings[GLOB_FXPiano].detail.mdz_switch.switch_value=0;
                                settings[GLOB_FX3DSpectrum].detail.mdz_switch.switch_value=0;
                                settings[GLOB_FXMilkdrop].detail.mdz_switch.switch_value=0;
                                break;
                            case 0x33:
                                //Exit
                                keepOpened=0;
                                break;
                        }
                    }
                    ImGui::PopID();
                }
            }
            ImGui::EndTable();
        }
    }
//    ImGui::PopStyleColor();
//    ImGui::PopStyleColor();
    
    //ImGui::Text("Hello, world %d", 123);
    //ImGui::Image((ImTextureID)(intptr_t)txtMenuHandle[0], ImVec2(256, 256));
    if (font_body) ImGui::PopFont();
    
    ImGui::EndChild();
    
    ImGui::End();
    
    return keepOpened;
}


}
