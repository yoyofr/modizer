/*
 *  RenderUtils.mm
 *  modizer
 *
 *  Created by Yohann Magnien on 23/08/10.
 *  Copyright 2010 __YoyoFR / Yohann Magnien__. All rights reserved.
 *
 */
#define BLOOM_BLUR_ITERATIONS 7 //

#define MODPATTERN_FRAME_COLHIGH1 140,160,255,255
#define MODPATTERN_FRAME_COLHIGH2 60,100,255,255

#define MODPATTERN_FRAME_COLMED1 140/3,160/3,255/3,255
#define MODPATTERN_FRAME_COLMED2 60/3,100/3,255/3,255

#define MODPATTERN_FRAME_COLLOW1 140/5,160/5,255/5,255
#define MODPATTERN_FRAME_COLLOW2 60/5,100/5,255/5,255

extern int NOTES_DISPLAY_TOPMARGIN;

#include "RenderUtils.h"
#include "TextureUtils.h"

#import "SettingsGenViewController.h"
extern volatile t_settings settings[MAX_SETTINGS];

//#import "Font.h"
//#import "GLString.h"

//--------------------------------------------------
// ImGui
//--------------------------------------------------
#include "../utils/imgui/imgui.h"
#include "../utils/imgui/backends/imgui_impl_ios.h"
#include "../utils/imgui/backends/imgui_impl_opengl3.h"



#define glPushMatrix(...)
#define glTranslatef(...)
#define glPushMatrix(...)
#define glPopMatrix(...)
#define glEnableClientState(...)
#define glVertexPointer(...)
#define glColorPointer(...)
#define glDisableClientState(...)

#define glMatrixMode(...)
#define glLoadIdentity(...)

#define glFrustumf(...)



unsigned int data_midifx_pal1[32];/*={
    0x0000fe, 0xfd00fe, 0xfe0000, 0x0aff05, 0xff78ff, 0x7900ff, 0x0077fe, 0x9701ff, 0xfeff05, 0x0700ba, 0x77fe77, 0x4187ba, 0xb98744, 0xf034ab, 0xaa31ec, 0xaa0001, 0x00ab05, 0x0003ac, 0xedb1ff, 0x154e56, 0x8d476f, 0x6c8c60, 0xf87574, 0xf6e38b, 0x5b430b, 0xa2f0eb, 0xe3e0f5, 0x115205, 0x39eec0, 0x1f3e9e, 0x89aa0d, 0xfb7810
};*/
unsigned int data_midifx_pal2[32];
unsigned int data_midifx_pal3[32];

unsigned int data_midifx_pal_custom[32]={
    0x0000fe, 0xfd00fe, 0xfe0000, 0x0aff05, 0xff78ff, 0x7900ff, 0x0077fe, 0x9701ff, 0xfeff05, 0x0700ba, 0x77fe77, 0x4187ba, 0xb98744, 0xf034ab, 0xaa31ec, 0xaa0001, 0x00ab05, 0x0003ac, 0xedb1ff, 0x154e56, 0x8d476f, 0x6c8c60, 0xf87574, 0xf6e38b, 0x5b430b, 0xa2f0eb, 0xe3e0f5, 0x115205, 0x39eec0, 0x1f3e9e, 0x89aa0d, 0xfb7810
};

unsigned int *data_midifx_col=data_midifx_pal1;


#define MAX_VISIBLE_CHAN 64

#define SPECTRUM_DEPTH 32
#define SPECTRUM_ZSIZE 12
#define SPECTRUM_Y 12.0f
#define SPECTR_XSIZE_FACTOR 0.95f
#define SPECTRUM_DECREASE_FACTOR 0.96f
#define SPECTR_XSIZE 38.0f
static float oldSpectrumDataL[SPECTRUM_DEPTH*4][SPECTRUM_BANDS];
static float oldSpectrumDataR[SPECTRUM_DEPTH*4][SPECTRUM_BANDS];
static GLfloat vertices[4][3];  /* Holds Float Info For 4 Sets Of Vertices */
static GLfloat normals[4][3];  /* Holds Float Info For 4 Sets Of Vertices */
static GLfloat vertColor[4][4];  /* Holds Float Info For 4 Sets Of Vertices */

extern int MIDIFX_OFS;

static int max_indices;

int txt_pianoRoll[3];

#define INDICES_SIZE_KEYW 60
#define INDICES_SIZE_KEYB 36
#define INDICES_SIZE_BOX 30

#define MAX_BARS 4096
typedef struct {
    unsigned int frameidx;
    signed short int startidx;
    unsigned char note;
    unsigned char subnote;
    unsigned char instr;
    unsigned char played;
    signed short int size;
    
} t_data_bar2draw;
static t_data_bar2draw data_bar2draw[MAX_BARS];

static int pianoroll_cpt;

#define PR_KEY_PRESSED (1<<0)
#define PR_WHITE_KEY (1<<1)
static uint8_t pianoroll_key_status[SOUND_MAXMOD_CHANNELS][256];

static int piano_note_type[128];
static float piano_note_posx[128];
static float piano_note_posy[128];
static float piano_note_posz[128];


static GLfloat verticesBAR[MAX_BARS*24][3];  /* Holds Float Info For 4 Sets Of Vertices */
static GLfloat vertColorBAR[MAX_BARS*24][4];  /* Holds Float Info For 4 Sets Of Vertices */
static GLushort vertBARindices[MAX_BARS*6*6]; //6 faces, 6 indices/face
//1,2,3,4,4,5,5,6,7,8,8,9,9,10,11,12,

float ambientLight[3][4] = {
    {0.1f, 0.1f, 0.2f, 1.0f},
    {0.2f, 0.1f, 0.1f, 1.0f},
    {0.1f, 0.1f, 0.1f, 1.0f }
};	// �wiat�o otoczenia
float diffuseLight[3][4] = {
    {0.5f, 0.5f, 0.9f, 1.0f },
    {0.9f, 0.5f, 0.5f, 1.0f },
    {1.0f, 1.0f, 1.0f, 1.0f }
};	// �wiat�o rozproszone
float specularLight[3][4] = {
    {1.0f, 1.0f, 1.0f, 1.0f },
    {1.0f, 1.0f, 1.0f, 1.0f },
    {1.0f, 1.0f, 1.0f, 1.0f }
};	// �wiat�o odbicia
float position[] = { 0, 0, 8, 1 };


GLUserData *userData_simpleRender2D;
GLUserData *userData_Render2DLines;
GLUserData *userData_Render2DTextures;
GLUserData *userData_Render2DTexturesBlur;
GLUserData *userData_Render2DTexturesBlend;
bool renderIsInit;

/********************************************************************************/


///
// Create a shader object, load the shader source, and
// compile the shader.
//
GLuint RenderUtils::LoadShader ( GLenum type, const GLchar *shaderSrc )
{
   GLuint shader;
   GLint compiled;
   
   // Create the shader object
   shader = glCreateShader ( type );

   if ( shader == 0 )
       return 0;

   // Load the shader source
   glShaderSource ( shader, 1, &shaderSrc, NULL );
   
   // Compile the shader
   glCompileShader ( shader );

   // Check the compile status
   glGetShaderiv ( shader, GL_COMPILE_STATUS, &compiled );

   if ( !compiled )
   {
      GLint infoLen = 0;

      glGetShaderiv ( shader, GL_INFO_LOG_LENGTH, &infoLen );
      
      if ( infoLen > 1 )
      {
         char* infoLog = (char*)malloc (sizeof(char) * infoLen );

         glGetShaderInfoLog ( shader, infoLen, NULL, infoLog );
         NSLog(@"Error compiling shader:\n%s\n", infoLog );
         
         free ( infoLog );
      }

      glDeleteShader ( shader );
      return 0;
   }

   return shader;

}


GLuint RenderUtils::LoadShaderFromFile ( GLenum type, const char *shaderFile )
{
   GLuint shader;
   GLint compiled;
    
    //allocate shader source code buffer & read file
    FILE *f;
    f=fopen(shaderFile,"rb");
    if (!f) return 0;
    
    fseek(f,0,SEEK_END);
    size_t fsize=ftell(f);
    fseek(f,0,SEEK_SET);
    if (!fsize) {
        fclose(f);
        return 0;
    }
    char *shaderData=(char*)malloc(fsize+1);
    shaderData[fsize]=0;
    fread(shaderData, 1, fsize, f);
    fclose(f);
    
   // Create the shader object
   shader = glCreateShader ( type );

    if ( shader == 0 ) {
        free(shaderData);
        return 0;
    }
       

   // Load the shader source
   glShaderSource ( shader, 1, (const GLchar**) &shaderData, NULL );
   
   // Compile the shader
   glCompileShader ( shader );
    
   
   // Check the compile status
   glGetShaderiv ( shader, GL_COMPILE_STATUS, &compiled );

   if ( !compiled )
   {
      GLint infoLen = 0;

      glGetShaderiv ( shader, GL_INFO_LOG_LENGTH, &infoLen );
      
      if ( infoLen > 1 )
      {
         char* infoLog = (char*)malloc (sizeof(char) * infoLen );

         glGetShaderInfoLog ( shader, infoLen, NULL, infoLog );
         NSLog(@"Error compiling shader:\n%s\n", infoLog );
         
         free ( infoLog );
      }

      glDeleteShader ( shader );
       
       //release shader source code buffer
       free(shaderData);

       
      return 0;
   }
    
    //release shader source code buffer
    free(shaderData);

   return shader;
}

///
// Initialize the shader and program object
//

int RenderUtils::RenderInit() {
    renderIsInit=false;
    
    GLUserData *userData = (GLUserData*)malloc(sizeof(GLUserData));
    GLuint vertexShader;
    GLuint fragmentShader;
    GLuint programObject;
    GLint linked;
    
    vertexShader = LoadShaderFromFile(GL_VERTEX_SHADER,[[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:@"/MDZShaders/Vertex2DSimple.glsl"] UTF8String]);
    fragmentShader = LoadShaderFromFile(GL_FRAGMENT_SHADER,[[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:@"/MDZShaders/Fragment2DSimple.glsl"] UTF8String]);
    
    // Create the program object
    programObject = glCreateProgram ( );
    
    if ( programObject == 0 )
        return 0;
    
    glAttachShader ( programObject, vertexShader );
    glAttachShader ( programObject, fragmentShader );
    
    // Link the program
    glLinkProgram ( programObject );
    
    // Check the link status
    glGetProgramiv ( programObject, GL_LINK_STATUS, &linked );
    
    if ( !linked )
    {
        GLint infoLen = 0;
        
        glGetProgramiv ( programObject, GL_INFO_LOG_LENGTH, &infoLen );
        
        if ( infoLen > 1 )
        {
            char* infoLog = (char*)malloc (sizeof(char) * infoLen );
            
            glGetProgramInfoLog ( programObject, infoLen, NULL, infoLog );
            NSLog ( @"Error linking program:\n%s\n", infoLog );
            
            free ( infoLog );
        }
        
        glDeleteProgram ( programObject );
        return 0;
    }
    
    userData_simpleRender2D=InitProgram((char*)[[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:@"/MDZShaders/Vertex2DSimple.glsl"]  UTF8String],
                                        (char*)[[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:@"/MDZShaders/Fragment2DSimple.glsl"] UTF8String]);
    
    userData_Render2DLines=InitProgram((char*)[[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:@"/MDZShaders/Vertex2DLines.glsl"]  UTF8String],
                                        (char*)[[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:@"/MDZShaders/Fragment2DLines.glsl"] UTF8String]);
    
    userData_Render2DTextures=InitProgram((char*)[[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:@"/MDZShaders/Vertex2DTextures.glsl"]  UTF8String],
                                        (char*)[[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:@"/MDZShaders/Fragment2DTextures.glsl"] UTF8String]);
    
    userData_Render2DTexturesBlur=InitProgram((char*)[[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:@"/MDZShaders/Vertex2DTextures.glsl"]  UTF8String],
                                        (char*)[[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:@"/MDZShaders/Fragment2DTexturesBlur.glsl"] UTF8String]);
    userData_Render2DTexturesBlend=InitProgram((char*)[[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:@"/MDZShaders/Vertex2DTextures.glsl"]  UTF8String],
                                        (char*)[[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:@"/MDZShaders/Fragment2DTexturesBlend.glsl"] UTF8String]);
    
    if (!userData_simpleRender2D ||
        !userData_Render2DLines ||
        !userData_Render2DTextures ||
        !userData_Render2DTexturesBlur ||
        !userData_Render2DTexturesBlend) {
        
        return 0;
    }
    
    renderIsInit=true;
    return 1;
}

GLUserData* RenderUtils::InitProgram(char *vsfile,char *fsfile) {
    GLUserData *userData = (GLUserData*)malloc(sizeof(GLUserData));
    GLuint vertexShader;
    GLuint fragmentShader;
    GLuint programObject;
    GLint linked;
    
    vertexShader = LoadShaderFromFile(GL_VERTEX_SHADER,vsfile);
    fragmentShader = LoadShaderFromFile(GL_FRAGMENT_SHADER,fsfile);
    
    // Create the program object
    programObject = glCreateProgram ( );
    
    if ( programObject == 0 ) {
        if (userData) free(userData);
        return 0;
    }
    
    glAttachShader ( programObject, vertexShader );
    glAttachShader ( programObject, fragmentShader );
    
    // Link the program
    glLinkProgram ( programObject );
    
    // Check the link status
    glGetProgramiv ( programObject, GL_LINK_STATUS, &linked );
    
    if ( !linked )
    {
        GLint infoLen = 0;
        
        glGetProgramiv ( programObject, GL_INFO_LOG_LENGTH, &infoLen );
        
        if ( infoLen > 1 )
        {
            char* infoLog = (char*)malloc (sizeof(char) * infoLen );
            
            glGetProgramInfoLog ( programObject, infoLen, NULL, infoLog );
            NSLog ( @"Error linking program:\n%s\n", infoLog );
            
            free ( infoLog );
        }
        
        glDeleteProgram ( programObject );
        
        if (userData) free(userData);
        return 0;
    }
    
    userData->programObject=programObject;
    
    // Get the uniform locations
    userData->mvpLoc = glGetUniformLocation ( userData->programObject, "u_mvpMatrix" );
    
    return userData;
}

void RenderUtils::ShutdownProgram(GLUserData *userData) {
    if (userData) free(userData);
}

/********************************************************************************/

static GLint blendSrc,blendDst;
static GLboolean isBlendOn,isCuffFaceOn,isDepthTestOn,isStencilTestOn;
void glDumpState() {
    glGetBooleanv(GL_BLEND,&isBlendOn);
    glGetIntegerv(GL_BLEND_SRC_ALPHA,&blendSrc);
    glGetIntegerv(GL_BLEND_DST_ALPHA,&blendDst);
    glGetBooleanv(GL_CULL_FACE,&isCuffFaceOn);
    glGetBooleanv(GL_DEPTH_TEST,&isDepthTestOn);
    glGetBooleanv(GL_STENCIL_TEST,&isStencilTestOn);
}
void glRestoreState() {
    if (isBlendOn) glEnable(GL_BLEND);
    else glDisable(GL_BLEND);
    
    if (isCuffFaceOn) glEnable(GL_CULL_FACE);
    else glDisable(GL_CULL_FACE);
    
    if (isDepthTestOn) glEnable(GL_DEPTH_TEST);
    else glDisable(GL_DEPTH_TEST);
    
    if (isStencilTestOn) glEnable(GL_STENCIL_TEST);
    else glDisable(GL_STENCIL_TEST);
    
    glBlendFunc(blendSrc,blendDst);
}

void RenderUtils::DrawTexture(uint ww,uint hh,GLuint textureIdx,float alpha,bool reversed) {
    // Use the program object
    if (!renderIsInit) return;
    
    glUseProgram ( userData_Render2DTextures->programObject );
    
    GLuint positionAttribHandle = glGetAttribLocation(userData_Render2DTextures->programObject, "a_position");
    GLuint textCoordAttribHandle    = glGetAttribLocation(userData_Render2DTextures->programObject, "a_textCoord");
    GLuint textureUnifHandle    = glGetUniformLocation(userData_Render2DTextures->programObject, "u_curTexture");
    GLuint alphaUnifHandle    = glGetUniformLocation(userData_Render2DTextures->programObject, "u_alpha");

    //Save opengl state
    glDumpState();
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureIdx);
    
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    
    LineVertexF ptsTriangles[6];
    coordData ptsTextCoord[6];
    
    ptsTriangles[0].x=-1; ptsTriangles[0].y=-1;
    ptsTriangles[1].x=1; ptsTriangles[1].y=-1;
    ptsTriangles[2].x=1; ptsTriangles[2].y=1;
    
    ptsTriangles[3].x=-1; ptsTriangles[3].y=-1;
    ptsTriangles[4].x=1; ptsTriangles[4].y=1;
    ptsTriangles[5].x=-1; ptsTriangles[5].y=1;
    
    ptsTextCoord[0].u=0; ptsTextCoord[0].v=0;
    ptsTextCoord[1].u=1; ptsTextCoord[1].v=0;
    ptsTextCoord[2].u=1; ptsTextCoord[2].v=1;
    
    ptsTextCoord[3].u=0; ptsTextCoord[3].v=0;
    ptsTextCoord[4].u=1; ptsTextCoord[4].v=1;
    ptsTextCoord[5].u=0; ptsTextCoord[5].v=1;
    
    if (reversed) {
        for (int i=0;i<6;i++) {
            ptsTextCoord[i].v=1.0f-ptsTextCoord[i].v;
        }
    }
    
    // Load the vertex data
    glVertexAttribPointer ( positionAttribHandle, 2, GL_FLOAT, GL_FALSE, sizeof(LineVertexF), &(ptsTriangles[0].x) );
    glVertexAttribPointer ( textCoordAttribHandle, 2, GL_FLOAT, GL_FALSE, sizeof(coordData), &(ptsTextCoord[0].u) );
    
    // Load the vertex data
    glEnableVertexAttribArray ( positionAttribHandle );
    glEnableVertexAttribArray ( textCoordAttribHandle );
    
    glVertexAttribDivisor ( positionAttribHandle, 0);
    glVertexAttribDivisor ( textCoordAttribHandle, 0);
    
    // Load the uniforms
    // Load the texture idx
    glUniform1ui(textureUnifHandle, 0);
    glUniform1fv(alphaUnifHandle,1,&alpha);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureIdx);
    
    
//    glBindTexture(GL_TEXTURE_2D, textureUnifHandle);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//
//    glActiveTexture(GL_TEXTURE1);
//    glBindTexture(GL_TEXTURE_2D, textureUnifHandle);

    
    glDrawArrays(GL_TRIANGLES,0,6);
    
    
    glRestoreState();
}

void RenderUtils::DrawTextureBlur(uint ww,uint hh,GLuint textureIdx,int hori,float min_brightness) {
    // Use the program object
    if (!renderIsInit) return;
    
    GLuint positionAttribHandle = glGetAttribLocation(userData_Render2DTexturesBlur->programObject, "a_position");
    GLuint textCoordAttribHandle    = glGetAttribLocation(userData_Render2DTexturesBlur->programObject, "a_textCoord");
    GLuint textureUnifHandle    = glGetUniformLocation(userData_Render2DTexturesBlur->programObject, "u_curTexture");
    GLuint horizontalUnifHandle    = glGetUniformLocation(userData_Render2DTexturesBlur->programObject, "u_horizontal");
    GLuint minBrightnessUnifHandle    = glGetUniformLocation(userData_Render2DTexturesBlur->programObject, "u_min_brightness");
    
    glDumpState();
    
    glDisable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    
    LineVertexF ptsTriangles[6];
    coordData ptsTextCoord[6];
    
    ptsTriangles[0].x=-1; ptsTriangles[0].y=-1;
    ptsTriangles[1].x=1; ptsTriangles[1].y=-1;
    ptsTriangles[2].x=1; ptsTriangles[2].y=1;
    
    ptsTriangles[3].x=-1; ptsTriangles[3].y=-1;
    ptsTriangles[4].x=1; ptsTriangles[4].y=1;
    ptsTriangles[5].x=-1; ptsTriangles[5].y=1;
    
    ptsTextCoord[0].u=0; ptsTextCoord[0].v=0;
    ptsTextCoord[1].u=1; ptsTextCoord[1].v=0;
    ptsTextCoord[2].u=1; ptsTextCoord[2].v=1;
    
    ptsTextCoord[3].u=0; ptsTextCoord[3].v=0;
    ptsTextCoord[4].u=1; ptsTextCoord[4].v=1;
    ptsTextCoord[5].u=0; ptsTextCoord[5].v=1;
    
    // Load the vertex data
    glVertexAttribPointer ( positionAttribHandle, 2, GL_FLOAT, GL_FALSE, sizeof(LineVertexF), &(ptsTriangles[0].x) );
    glVertexAttribPointer ( textCoordAttribHandle, 2, GL_FLOAT, GL_FALSE, sizeof(coordData), &(ptsTextCoord[0].u) );
    
    // Load the vertex data
    glEnableVertexAttribArray ( positionAttribHandle );
    glEnableVertexAttribArray ( textCoordAttribHandle );
    
    glVertexAttribDivisor ( positionAttribHandle, 0);
    glVertexAttribDivisor ( textCoordAttribHandle, 0);
    
    // Load the uniforms
    // Load the texture idx
    glUniform1ui(textureUnifHandle, 0);
    glUniform1i(horizontalUnifHandle, hori);
    glUniform1f(minBrightnessUnifHandle, min_brightness);
    
    glActiveTexture(GL_TEXTURE0+0);
    glBindTexture(GL_TEXTURE_2D, textureIdx);
    
    
    glDrawArrays(GL_TRIANGLES,0,6);
    glRestoreState();
}

void RenderUtils::DrawTextureBlend(uint ww,uint hh,GLuint textOrigIdx,GLuint textBlurIdx) {
    // Use the program object
    if (!renderIsInit) return;
    
    glUseProgram ( userData_Render2DTexturesBlend->programObject );
    
    GLint positionAttribHandle = glGetAttribLocation(userData_Render2DTexturesBlend->programObject, "a_position");
    GLint textCoordAttribHandle    = glGetAttribLocation(userData_Render2DTexturesBlend->programObject, "a_textCoord");
    GLint textOrigUnifHandle    = glGetUniformLocation(userData_Render2DTexturesBlend->programObject, "u_textOriginal");
    GLint textBlurUnifHandle    = glGetUniformLocation(userData_Render2DTexturesBlend->programObject, "u_textBlurred");
    
    // Save state
    glDumpState();
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    
    LineVertexF ptsTriangles[6];
    coordData ptsTextCoord[6];
    
    ptsTriangles[0].x=-1; ptsTriangles[0].y=-1;
    ptsTriangles[1].x=1; ptsTriangles[1].y=-1;
    ptsTriangles[2].x=1; ptsTriangles[2].y=1;
    
    ptsTriangles[3].x=-1; ptsTriangles[3].y=-1;
    ptsTriangles[4].x=1; ptsTriangles[4].y=1;
    ptsTriangles[5].x=-1; ptsTriangles[5].y=1;
    
    ptsTextCoord[0].u=0; ptsTextCoord[0].v=0;
    ptsTextCoord[1].u=1; ptsTextCoord[1].v=0;
    ptsTextCoord[2].u=1; ptsTextCoord[2].v=1;
    
    ptsTextCoord[3].u=0; ptsTextCoord[3].v=0;
    ptsTextCoord[4].u=1; ptsTextCoord[4].v=1;
    ptsTextCoord[5].u=0; ptsTextCoord[5].v=1;
    
    // Load the vertex data
    glVertexAttribPointer ( positionAttribHandle, 2, GL_FLOAT, GL_FALSE, sizeof(LineVertexF), &(ptsTriangles[0].x) );
    glVertexAttribPointer ( textCoordAttribHandle, 2, GL_FLOAT, GL_FALSE, sizeof(coordData), &(ptsTextCoord[0].u) );
    
    // Load the vertex data
    glEnableVertexAttribArray ( positionAttribHandle );
    glEnableVertexAttribArray ( textCoordAttribHandle );
    
    glVertexAttribDivisor ( positionAttribHandle, 0);
    glVertexAttribDivisor ( textCoordAttribHandle, 0);
    
    // Load the uniforms
    glUniform1i(textOrigUnifHandle, 0);
    glUniform1i(textBlurUnifHandle, 1);
    
    glActiveTexture(GL_TEXTURE0+0);
    glBindTexture(GL_TEXTURE_2D, textOrigIdx);
    glActiveTexture(GL_TEXTURE0+1);
    glBindTexture(GL_TEXTURE_2D, textBlurIdx);
    
    // Render
    glDrawArrays(GL_TRIANGLES,0,6);
    // Restore state
    glRestoreState();
}


static GLuint framebuffer = 0;
static GLuint renderedTexture = 0;
static GLuint colorRenderbuffer,depthRenderbuffer;
static int renderedTextWidth,renderedTextHeight;
static GLint curFramebuffer,curRenderbuffer;
static unsigned int pingpongFBO[2];
static unsigned int pingpongBuffer[2];

void RenderUtils::startRenderToTexture(int width,int height) {
    static bool firstCall=true;
    // The framebuffer, which regroups 0, 1, or more textures, and 0 or 1 depth buffer.
    
    glGetIntegerv(GL_FRAMEBUFFER_BINDING,&curFramebuffer);
    //glGetIntegerv(GL_RENDERBUFFER_BINDING,&curRenderbuffer);
    
    if (firstCall||(renderedTextWidth!=width)||(renderedTextHeight!=height)) {
        renderedTextWidth=width;
        renderedTextHeight=height;
        RenderUtils::initRenderToTexture(width,height);
        firstCall=false;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glViewport(0,0,width,height);
    glClearColor(0.0f,0.0f,0.0f,0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Bind our texture in Texture Unit 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderedTexture);
}


void RenderUtils::endRenderToTexture(int width,int height) {
    //apply BLUR
    if (!renderIsInit) return;
    GLuint curTexture;
    bool horizontal = true, first_iteration = true;
    int amount = BLOOM_BLUR_ITERATIONS;
    glUseProgram ( userData_Render2DTexturesBlur->programObject );
    
//    for (unsigned int i = 0; i < amount; i++)
//    {
//        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
//        curTexture=first_iteration ? renderedTexture : pingpongBuffer[!horizontal];
//        RenderUtils::DrawTextureBlur(width, height, curTexture, (horizontal?1:0),(first_iteration?0.0f:0.0f));
//        horizontal = !horizontal;
//        if (first_iteration)
//            first_iteration = false;
//    }
    for (unsigned int i = 0; i < amount; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
        curTexture=first_iteration ? renderedTexture : pingpongBuffer[!horizontal];
        RenderUtils::DrawTextureBlur(width, height, curTexture, i,(first_iteration?0.0f:0.0f));
        horizontal = !horizontal;
        if (first_iteration)
            first_iteration = false;
    }
    // Bind rendering buffer
    glBindFramebuffer(GL_FRAMEBUFFER, curFramebuffer);
    glViewport(0,0,width,height);
    
    // Render by blending the original & blurred textures
    RenderUtils::DrawTextureBlend(width, height, renderedTexture,curTexture);
    //RenderUtils::DrawTexture(width, height, curTexture,1.0,0);
    //RenderUtils::DrawTexture(width, height, renderedTexture,1.0,0);
}

void RenderUtils::shutdownRenderToTexture() {
    if (framebuffer) glDeleteFramebuffers(1, &framebuffer);
    framebuffer=0;
    if (pingpongFBO[0]) glDeleteFramebuffers(2, pingpongFBO);
    pingpongFBO[0]=pingpongFBO[1]=0;
    
    if (renderedTexture) glDeleteTextures(1, &renderedTexture);
    renderedTexture=0;
    if (pingpongBuffer[0]) glDeleteTextures(2, pingpongBuffer);
    pingpongBuffer[0]=pingpongBuffer[1]=0;
}

bool RenderUtils::initRenderToTexture(int width,int height) {
    if (!renderIsInit) return false;
    
    if (!framebuffer) NSLog(@"init render to texture %d x %d",width,height);
    else {
        NSLog(@"reinit render to texture %d x %d",width,height);
        RenderUtils::shutdownRenderToTexture();
    }
    // Save current framebuffer & renderbuffer
    glGetIntegerv(GL_FRAMEBUFFER_BINDING,&curFramebuffer);
    //glGetIntegerv(GL_RENDERBUFFER_BINDING,&curRenderbuffer);
    
    //----------------------------
    // Initial rendering setup
    //----------------------------
    // Create the framebuffer and bind it
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    
    // create the texture
    glGenTextures(1, &renderedTexture);
//    NSLog(@"new texture %s %d","renderedTexture",renderedTexture);
    
    glBindTexture(GL_TEXTURE_2D, renderedTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,  width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    // Set "renderedTexture" as our colour attachement #0
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderedTexture, 0);
    
    // Set the list of draw buffers.
    GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers
    
    // Create a color renderbuffer, allocate storage for it, and attach it to the framebuffer’s color attachment point.
//    glGenRenderbuffers(1, &colorRenderbuffer);
//    glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer);
//    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, width, height);
//    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorRenderbuffer);
    
    // Create a depth or depth/stencil renderbuffer, allocate storage for it, and attach it to the framebuffer’s depth attachment point.
//    glGenRenderbuffers(1, &depthRenderbuffer);
//    glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);
//    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);
//    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);
    
    
    //----------------------------
    // Blur rendering setup
    //----------------------------
    
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongBuffer);
//    NSLog(@"new texture %s %d","pingpongBuffer0",pingpongBuffer[0]);
//    NSLog(@"new texture %s %d","pingpongBuffer1",pingpongBuffer[1]);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongBuffer[i]);
//        glTexImage2D(
//            GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL
//        );
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F,  width, height, 0, GL_RGBA, GL_FLOAT, NULL);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongBuffer[i], 0
        );
        
        // Set the list of draw buffers.
        GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
        glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers
    }
    
    // Always check that our framebuffer is ok
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER) ;
    if(status != GL_FRAMEBUFFER_COMPLETE) {
        NSLog(@"failed to make complete framebuffer object %x", status);
        return false;
    }
    return true;
}


#define OSCILLO_BUFFER_NB 4
#define OSCILLO_BUFFER_SIZE SOUND_BUFFER_SIZE_SAMPLE*OSCILLO_BUFFER_NB
static signed char *prev_snd_data;
static signed char *prev_snd_dataStereo;
static int snd_data_ofs[SOUND_MAXVOICES_BUFFER_FX];
static signed char cur_snd_data[OSCILLO_BUFFER_SIZE*SOUND_MAXVOICES_BUFFER_FX];

//static int mVoicesName_FontSize;

#define FX_OSCILLO_MAXROWS 16
#include "ModizerVoicesData.h"

#define absint(a) (a>=0?a:-a)

#define FIXED_POINT_PRECISION 16
void RenderUtils::DrawOscilloMultiple(signed char **snd_data,int snd_data_idx,int num_voices,uint ww,uint hh,uint color_mode,float mScaleFactor,bool isfullscreen,bool bloom,char *voices_label,bool draw_frame,bool flag_direct_stereo) {
    SimpleLineVertexF *ptsLines;
    int mulfactor;
    int val[SOUND_MAXVOICES_BUFFER_FX];
    int oval[SOUND_MAXVOICES_BUFFER_FX];
    int sp[SOUND_MAXVOICES_BUFFER_FX];
    int osp[SOUND_MAXVOICES_BUFFER_FX];
    
    int colR,colG,colB,tmpR,tmpG,tmpB,colA;
    int count,countLines;
    int64_t max_gap,tmp_gap,ofs1,ofs2,old_ofs;
    
    static char first_call=1;
    
    if (!renderIsInit) return;
    
//    if (bloom) RenderUtils::startRenderToTexture(ww*mScaleFactor,hh*mScaleFactor);
//    if (bloom) RenderUtils::startRenderToTexture(ww,hh);
    
    while (snd_data_idx<0) snd_data_idx+=SOUND_BUFFER_NB;
    while (snd_data_idx>=SOUND_BUFFER_NB) snd_data_idx-=SOUND_BUFFER_NB;
    
    int max_len_oscillo_buffer=735;// 1frame at 60fps & 44100Hz, assume OSCILLO_BUFFER_SIZE>735  OSCILLO_BUFFER_SIZE*2/6;
    int max_ofs=OSCILLO_BUFFER_SIZE-max_len_oscillo_buffer;
    int min_ofs=0;
    
    colA=255;//128;
    
    while (snd_data_idx<0) snd_data_idx+=SOUND_BUFFER_NB;
    while (snd_data_idx>=SOUND_BUFFER_NB) snd_data_idx-=SOUND_BUFFER_NB;
    
    //-----------------------------------------------------------------
    if (flag_direct_stereo) {
        for (int i=0;i<num_voices;i++)
            for (int k=0;k<OSCILLO_BUFFER_NB;k++) {
                for (int j=0;j<SOUND_BUFFER_SIZE_SAMPLE;j++) {
                    cur_snd_data[(j+k*SOUND_BUFFER_SIZE_SAMPLE)*SOUND_MAXVOICES_BUFFER_FX+i]=((short int **)snd_data)[(snd_data_idx+k)%SOUND_BUFFER_NB][j*2+i]>>8;
                }
            }
        
    } else {
        for (int i=0;i<num_voices;i++)
            for (int k=0;k<OSCILLO_BUFFER_NB;k++) {
                for (int j=0;j<SOUND_BUFFER_SIZE_SAMPLE;j++) {
                    cur_snd_data[(j+k*SOUND_BUFFER_SIZE_SAMPLE)*SOUND_MAXVOICES_BUFFER_FX+i]=snd_data[(snd_data_idx+k)%SOUND_BUFFER_NB][j*SOUND_MAXVOICES_BUFFER_FX+i];
                }
            }
    }
    
    if (first_call) {
        prev_snd_data=(signed char*)malloc(OSCILLO_BUFFER_SIZE*SOUND_MAXVOICES_BUFFER_FX);
        if (!prev_snd_data) {
            printf("%s: cannot allocate prev_snd_data\n",__func__);
            return;
        }
        memcpy(prev_snd_data,cur_snd_data,OSCILLO_BUFFER_SIZE*SOUND_MAXVOICES_BUFFER_FX);
        
        for (int i=0;i<SOUND_MAXVOICES_BUFFER_FX;i++) {
            snd_data_ofs[i]=max_ofs/2;
            //mVoicesName[i]=NULL;
        }
        //mVoicesName_FontSize=-1;
        
        first_call=0;
    }
    
    int columns_nb=((num_voices-1)/FX_OSCILLO_MAXROWS)+1;
    int columns_width=ww/columns_nb;
    
    int max_voices_by_row=(num_voices+columns_nb-1)/columns_nb;
    float ratio;
    
    //check best config, maximize 16/9 ratio
    if (num_voices>=1)
        for (;;) {
            columns_width=ww/columns_nb;
            max_voices_by_row=(num_voices+columns_nb-1)/columns_nb;
            mulfactor=(hh-8)/(max_voices_by_row)/2;
            ratio=columns_width/(2*mulfactor);
            
            if (ratio<=2) break;
            if (columns_nb>=num_voices) break;
            
            columns_nb++;
            
        }
    // NSLog(@"%d %d / %f",columns_width,mulfactor,ratio);

    float thickness;
    switch (settings[OSCILLO_LINE_Width].detail.mdz_switch.switch_value) {
        default:
        case 0:
            thickness=1.0;//(1.0f*mScaleFactor);
            break;
        case 1:
            thickness=2.0;//(2.0f*mScaleFactor);
            break;
        case 2:
            thickness=3.0;//(3.0f*mScaleFactor);
            break;
        case 3:
            thickness=4.0;//(4.0f*mScaleFactor);
            break;
    }

    
    int xofs=(ww-columns_width*columns_nb)/2;
    int smpl_ofs_incr=(max_len_oscillo_buffer)*(1<<FIXED_POINT_PRECISION)/columns_width;
    int cur_voices=0;
    
    int max_count=columns_width*num_voices;
    ptsLines=(SimpleLineVertexF*)malloc(sizeof(SimpleLineVertexF)*columns_width*num_voices);
    if (!ptsLines) {
        printf("%s: cannot allocate LineVertex buffer\n",__func__);
        return;
    }
    count=0;
    countLines=0;
    
    //determine min smplincr / width of oscillo on screen, help reduce processing time
    int smplincr=OSCILLO_BUFFER_SIZE/columns_width;
    if (smplincr<1) smplincr=1;
    int bufflen=max_len_oscillo_buffer/smplincr;
    
    // min gap to match/allow
    int min_gap_threshold=0;//bufflen;
    for (int j=0;j<num_voices;j++) {
        // for each voices
        max_gap=0;
        //reset start offset / previous frame
        old_ofs=0;
        
        ofs1=snd_data_ofs[j];
        ofs2=snd_data_ofs[j]-smplincr;
        int right_done=0;
        int left_done=0;
        for (;;) {
            // start analyzing
            
            //check on right side, ofs1
            if ((ofs1<max_ofs)&& !right_done) {
                tmp_gap=0;
                signed char *snd_data_ptr=cur_snd_data+ofs1*SOUND_MAXVOICES_BUFFER_FX+j;
                signed char *prev_snd_data_ptr=prev_snd_data+j;
                int64_t val;
                int incr=smplincr*SOUND_MAXVOICES_BUFFER_FX;
                for (int i=0;i<bufflen;i++) {
                    //compute diff between 2 samples with respective offset
                    val=(int)(*snd_data_ptr)*(int)(*prev_snd_data_ptr);
                    tmp_gap+=val;
                    snd_data_ptr+=incr;
                    prev_snd_data_ptr+=incr;
                }
                
                //if (tmp_gap<min_gap) { //if more aligned, use ofs as new ref
                if (max_gap<tmp_gap) {
                    max_gap=tmp_gap;
                    snd_data_ofs[j]=ofs1;
                }
                
                ofs1+=smplincr;
            } else right_done=1;
            //check on left side, ofs2
            if ((ofs2>0)&& !left_done) {
                tmp_gap=0;
                signed char *snd_data_ptr=cur_snd_data+ofs2*SOUND_MAXVOICES_BUFFER_FX+j;
                signed char *prev_snd_data_ptr=prev_snd_data+j;
                int64_t val;
                int incr=smplincr*SOUND_MAXVOICES_BUFFER_FX;
                for (int i=0;i<bufflen;i++) {
                    //compute diff between 2 samples with respective offset
                    val=(int)(*snd_data_ptr)*(int)(*prev_snd_data_ptr);
                    tmp_gap+=val;
                    snd_data_ptr+=incr;
                    prev_snd_data_ptr+=incr;
                }
                
                if (tmp_gap>max_gap) { //if more aligned, use ofs as new ref
                    max_gap=tmp_gap;
                    snd_data_ofs[j]=ofs2;
                }
                ofs2-=smplincr;
            } else left_done=1;
            
            if ( left_done && right_done ) break;
        }
        //snd_data_ofs[j]=0;
    }
    
    for (int i=0;i<max_len_oscillo_buffer;i++){
        for (int j=0;j<num_voices;j++) {
            prev_snd_data[i*SOUND_MAXVOICES_BUFFER_FX+j]=cur_snd_data[(i+(snd_data_ofs[j]))*SOUND_MAXVOICES_BUFFER_FX+j];
        }
    }
    
    for (int i=0;i<num_voices;i++) {
        val[i]=(signed int)(cur_snd_data[((snd_data_ofs[i]))*SOUND_MAXVOICES_BUFFER_FX+i])*(mulfactor-1)>>7;
        sp[i]=(val[i]); if(sp[i]>=mulfactor) sp[i]=mulfactor-1; if (sp[i]<=-mulfactor) sp[i]=-mulfactor+1;
    }
    
    ImGui::SetNextWindowPos(ImVec2(0,0));
    ImGui::SetNextWindowSize(ImVec2(ww*mScaleFactor,hh*mScaleFactor));
    ImGui::GetStyle().Alpha=1.0f;
    ImGui::PushStyleColor(ImGuiCol_WindowBg,ImVec4(0,0,0,0));
    
    float fontSize=16;
    switch (settings[OSCILLO_LabelFontSize].detail.mdz_switch.switch_value) {
        case 0: //10
            fontSize=10;
            break;
        case 1: //16
            fontSize=16;
            break;
        case 2: //24
            fontSize=24;
            break;
    }
    
    ImGui::PushFont(nullptr,fontSize*mScaleFactor);
    ImGui::Begin("OscilloFX",0,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoFocusOnAppearing);
    //ImGui::SetCursorPos(ImVec2(0,0));
    //ImGui::Text("%d %d",ww,hh);
    
    for (int r=0;r<columns_nb;r++) {
        int xpos=xofs+r*columns_width;
        int max_voices=num_voices*(r+1)/columns_nb;
        int ypos=hh-mulfactor;
        
        for (;cur_voices<max_voices;cur_voices++,ypos-=2*mulfactor) {
            int smpl_ofs=snd_data_ofs[cur_voices]<<FIXED_POINT_PRECISION;
            
            if (color_mode==1) {
                colR=(settings[OSCILLO_MONO_COLOR].detail.mdz_color.rgb>>16)&0xFF;
                colG=(settings[OSCILLO_MONO_COLOR].detail.mdz_color.rgb>>8)&0xFF;
                colB=(settings[OSCILLO_MONO_COLOR].detail.mdz_color.rgb>>0)&0xFF;
                
            } else {
                colR=((m_voice_voiceColor[cur_voices]>>16)&0xFF);
                colG=((m_voice_voiceColor[cur_voices]>>8)&0xFF);
                colB=((m_voice_voiceColor[cur_voices]>>0)&0xFF);
            }
            
            //draw label if specified
            if (voices_label) {
                ImVec2 cursorPos=ImVec2((xpos+4)*mScaleFactor,
                                        (hh-(ypos+mulfactor-4))*mScaleFactor);
                
                ImGui::SetCursorPos(cursorPos);
                ImGui::Text("%s",voices_label+cur_voices*32);
            }
            
            for (int i=0; i<columns_width-2; i++) {
                oval[cur_voices]=val[cur_voices];
                val[cur_voices]=cur_snd_data[((smpl_ofs>>FIXED_POINT_PRECISION))*SOUND_MAXVOICES_BUFFER_FX+cur_voices];
                osp[cur_voices]=sp[cur_voices];
                sp[cur_voices]=(val[cur_voices])*(mulfactor-1)>>7; if(sp[cur_voices]>=mulfactor) sp[cur_voices]=mulfactor-1; if (sp[cur_voices]<=-mulfactor) sp[cur_voices]=-mulfactor+1;
                
                tmpR=colR;//+((val[cur_voices]-oval[cur_voices])<<1);
                tmpG=colG;//+((val[cur_voices]-oval[cur_voices])<<1);
                tmpB=colB;//+((val[cur_voices]-oval[cur_voices])<<1);
                if (tmpR>255) tmpR=255;if (tmpG>255) tmpG=255;if (tmpB>255) tmpB=255;
                if (tmpR<0) tmpR=0;if (tmpG<0) tmpG=0;if (tmpB<0) tmpB=0;
                
                if (countLines>=max_count-1) break;
                
                ptsLines[countLines++] = SimpleLineVertexF(xpos+i,osp[cur_voices]+ypos,
                                                         xpos+i+1,sp[cur_voices]+ypos,ww,hh);
                
                smpl_ofs+=smpl_ofs_incr;//*3/4;
            }
        }
    }
    
    ImGui::End();
    ImGui::PopFont();
    ImGui::PopStyleColor();
    
    GLfloat line_width=mScaleFactor*thickness/(float)hh;
    
    LineVertexF *ptsTriangles;
    
    ptsTriangles=(LineVertexF*)malloc(sizeof(LineVertexF)*6);
    
    ptsTriangles[0].x=0; ptsTriangles[0].y=-0.5;
    ptsTriangles[1].x=1; ptsTriangles[1].y=-0.5;
    ptsTriangles[2].x=1; ptsTriangles[2].y=0.5;
    
    ptsTriangles[3].x=0; ptsTriangles[3].y=-0.5;
    ptsTriangles[4].x=1; ptsTriangles[4].y=0.5;
    ptsTriangles[5].x=0; ptsTriangles[5].y=0.5;
    
    //  5---24
    //  | / |
    //  03--1
    //
    
    for (int i=0;i<6;i++) {
        ptsTriangles[i].r=(float)colR/255.0;
        ptsTriangles[i].g=(float)colG/255.0;
        ptsTriangles[i].b=(float)colB/255.0;
        ptsTriangles[i].a=1;
    }
    
    // Use the program object
    glUseProgram ( userData_Render2DLines->programObject );
    
    GLuint positionAttribHandle = glGetAttribLocation(userData_Render2DLines->programObject, "a_position");
    GLuint pointABAttribHandle = glGetAttribLocation(userData_Render2DLines->programObject, "a_pointAB");
    GLuint colorAttribHandle    = glGetAttribLocation(userData_Render2DLines->programObject, "a_color");
    GLuint widthHandle = glGetUniformLocation(userData_Render2DLines->programObject, "u_width");
    
    //Save opengl state
    glDumpState();
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    
    // Load the vertex data
    glVertexAttribPointer ( positionAttribHandle, 2, GL_FLOAT, GL_FALSE, sizeof(LineVertexF), &(ptsTriangles[0].x) );
    glVertexAttribPointer ( pointABAttribHandle, 4, GL_FLOAT, GL_FALSE, sizeof(SimpleLineVertexF), &(ptsLines[0].Ax) );
    glVertexAttribPointer ( colorAttribHandle, 4, GL_FLOAT, GL_FALSE, sizeof(LineVertexF), &(ptsTriangles[0].r) );
    
    // Load the vertex data
    glEnableVertexAttribArray ( positionAttribHandle );
    glEnableVertexAttribArray ( pointABAttribHandle );
    glEnableVertexAttribArray ( colorAttribHandle );
    
    glVertexAttribDivisor ( pointABAttribHandle, 1);
    
    // Generate a model view matrix to rotate/translate the cube
    esMatrixLoadIdentity ( &(userData_Render2DLines->mvpMatrix) );
    
    // Load the uniforms
    // Load the MVP matrix
    glUniformMatrix4fv ( userData_Render2DLines->mvpLoc, 1, GL_FALSE, ( GLfloat * ) &userData_Render2DLines->mvpMatrix.m[0][0] );
    
    // Load the line width
    glUniform1fv(widthHandle,1, &line_width);
    
    //ImGui::Text("%.3f x %.3f, %.3f x %.3f",ptsLines[0].Ax,ptsLines[0].Ay,ptsLines[0].Bx,ptsLines[0].By);
    
    glDrawArraysInstanced(GL_TRIANGLES,0,6, countLines);
    
    if (draw_frame) {
        count=0;
        countLines=0;
        
        line_width=thickness*(2.0f/(float)ww);
        
        colR=(settings[OSCILLO_GRID_COLOR].detail.mdz_color.rgb>>16)&0xFF;
        colG=(settings[OSCILLO_GRID_COLOR].detail.mdz_color.rgb>>8)&0xFF;
        colB=(settings[OSCILLO_GRID_COLOR].detail.mdz_color.rgb>>0)&0xFF;
        
        for (int i=0;i<6;i++) {
            ptsTriangles[i].r=(float)colR/255.0;
            ptsTriangles[i].g=(float)colG/255.0;
            ptsTriangles[i].b=(float)colB/255.0;
            ptsTriangles[i].a=1;
        }
        //top
        ptsLines[countLines++] = SimpleLineVertexF(0, hh-1,
                                                 ww-1,hh-1,ww,hh);
        //right
        ptsLines[countLines++] = SimpleLineVertexF(ww-1, hh-1,
                                                 ww-1,hh-mulfactor*max_voices_by_row*2,ww,hh);
        //bottom
        ptsLines[countLines++] = SimpleLineVertexF(ww-1,hh-mulfactor*max_voices_by_row*2,
                                                   0,hh-mulfactor*max_voices_by_row*2,ww,hh);
        //left
        ptsLines[countLines++] = SimpleLineVertexF(0,hh-mulfactor*max_voices_by_row*2,
                                                   0,hh-1,ww,hh);
        for (int r=0;r<columns_nb;r++) {
            int xpos=xofs+r*columns_width;
            int max_voices=num_voices*(r+1)/columns_nb;
            int ypos=hh-mulfactor;
            
            ptsLines[countLines++] = SimpleLineVertexF(xpos,hh-1,
                                                       xpos,hh-mulfactor*max_voices_by_row*2,ww,hh);
        }
        for (int r=0;r<max_voices_by_row;r++) {
            ptsLines[countLines++] = SimpleLineVertexF(0,hh-mulfactor*r*2,
                                                       ww-1,hh-mulfactor*r*2,ww,hh);
        }
        glDrawArraysInstanced(GL_TRIANGLES,0,6, countLines);
    }
    
    free(ptsTriangles);
    free(ptsLines);
    
    glVertexAttribDivisor ( pointABAttribHandle, 0);
    glRestoreState();
    
//    if (bloom) RenderUtils::endRenderToTexture(ww*mScaleFactor,hh*mScaleFactor,bloom_size);
//    if (bloom) RenderUtils::endRenderToTexture(ww,hh,bloom_size);
}

static int DrawSpectrum_first_call=1;
static int spectrumPeakValueL[SPECTRUM_BANDS];
static int spectrumPeakValueR[SPECTRUM_BANDS];
static int spectrumPeakValueL_index[SPECTRUM_BANDS];
static int spectrumPeakValueR_index[SPECTRUM_BANDS];


static int beatValueL_index[SPECTRUM_BANDS];
static int beatValueR_index[SPECTRUM_BANDS];

int RenderUtils::buildQuad(LineVertexF *pts,
                       int x1,int y1,
                           int x2,int y2,
                           int x3,int y3,
                           int x4,int y4,
                           int r1,int g1,int b1,int a1,
                           int r2,int g2,int b2,int a2,
                           int r3,int g3,int b3,int a3,
                           int r4,int g4,int b4,int a4,int ww,int hh) {
    int count=0;
    
    if (!pts) return 0;
    
    //1st triangle
    pts[count++]=LineVertexF(x1,y1,r1,g1,b1,a1,ww,hh);
    pts[count++]=LineVertexF(x2,y2,r2,g2,b2,a2,ww,hh);
    pts[count++]=LineVertexF(x3,y3,r3,g3,b3,a3,ww,hh);
    
    //2nd triangle
    pts[count++]=LineVertexF(x1,y1,r1,g1,b1,a1,ww,hh);
    pts[count++]=LineVertexF(x3,y3,r3,g3,b3,a3,ww,hh);
    pts[count++]=LineVertexF(x4,y4,r4,g4,b4,a4,ww,hh);
    
    return count;
}

void RenderUtils::DrawChanLayout(uint _ww,uint _hh,int display_note_mode,int chanNb,float pixOfs,float char_width,float char_height,float mScaleFactor) {
    int count=0;
    float col_size,col_ofs;
    LineVertexF *pts;
    
    switch (display_note_mode){
        case 0:col_size=11*char_width;col_ofs=(char_width)*2.5f-1;break;
        case 1:col_size=6*char_width;col_ofs=(char_width)*2.5f-1;break;
        case 2:col_size=4*char_width;col_ofs=(char_width)*2.5f-1;break;
    }
    
    pts=(LineVertexF*)malloc(sizeof(LineVertexF)*6*((chanNb+1)*3+4+4));
    if (!pts) {
        NSLog(@"%s - cannot allocate memory",__func__);
        return;
    }
    float min_w=col_size*chanNb+col_ofs;
    min_w=fmin(min_w,_ww);
    
    //Header line
    count+=RenderUtils::buildQuad(&(pts[count]),
                                  col_ofs,     _hh,
                                  min_w, _hh,
                                  min_w, _hh-2,
                                  col_ofs,     _hh-2,
                                  MODPATTERN_FRAME_COLHIGH1,
                                  MODPATTERN_FRAME_COLHIGH2,
                                  MODPATTERN_FRAME_COLHIGH2,
                                  MODPATTERN_FRAME_COLHIGH1,
                                  _ww,_hh);
    
    count+=RenderUtils::buildQuad(&(pts[count]),
                                  col_ofs,     _hh-2,
                                  min_w, _hh-2,
                                  min_w, _hh-(char_height+2-0)-2,
                                  col_ofs,     _hh-(char_height+2-0)-2,
                                  MODPATTERN_FRAME_COLMED1,
                                  MODPATTERN_FRAME_COLMED2,
                                  MODPATTERN_FRAME_COLMED2,
                                  MODPATTERN_FRAME_COLMED1,
                                  _ww,_hh);

    count+=RenderUtils::buildQuad(&(pts[count]),
                                  col_ofs,     _hh-(char_height+2-0)-2,
                                  min_w, _hh-(char_height+2-0)-2,
                                  min_w, _hh-(char_height+2-0),
                                  col_ofs,     _hh-(char_height+2-0),
                                  MODPATTERN_FRAME_COLLOW1,
                                  MODPATTERN_FRAME_COLLOW2,
                                  MODPATTERN_FRAME_COLLOW2,
                                  MODPATTERN_FRAME_COLLOW1,
                                  _ww,_hh);
    count+=RenderUtils::buildQuad(&(pts[count]),
                                  col_ofs-2,     _hh,
                                  col_ofs, _hh,
                                  col_ofs,  _hh-(char_height+2-0),
                                  col_ofs-2,      _hh-(char_height+2-0),
                                  MODPATTERN_FRAME_COLHIGH1,
                                  MODPATTERN_FRAME_COLHIGH2,
                                  MODPATTERN_FRAME_COLHIGH2,
                                  MODPATTERN_FRAME_COLHIGH1,
                                  _ww,_hh);
    
    //border / lines nb

    count+=RenderUtils::buildQuad(&(pts[count]),
                                  0,     0,
                                  2, 0,
                                  2,  _hh-(char_height+2-0)-2,
                                  0,      _hh-(char_height+2-0)-2,
                                  MODPATTERN_FRAME_COLHIGH1,
                                  MODPATTERN_FRAME_COLHIGH2,
                                  MODPATTERN_FRAME_COLHIGH2,
                                  MODPATTERN_FRAME_COLHIGH1,
                                  _ww,_hh);


    count+=RenderUtils::buildQuad(&(pts[count]),
                                  2,     0,
                                  col_ofs-2, 0,
                                  col_ofs-2,  _hh-(char_height+2-0)-2,
                                  2,      _hh-(char_height+2-0)-2,
                                  MODPATTERN_FRAME_COLMED1,
                                  MODPATTERN_FRAME_COLMED2,
                                  MODPATTERN_FRAME_COLMED2,
                                  MODPATTERN_FRAME_COLMED1,
                                  _ww,_hh);
        
    count+=RenderUtils::buildQuad(&(pts[count]),
                                  col_ofs-2,     0,
                                  col_ofs, 0,
                                  col_ofs,  _hh-(char_height+2-0)-2,
                                  col_ofs-2,      _hh-(char_height+2-0)-2,
                                  MODPATTERN_FRAME_COLLOW1,
                                  MODPATTERN_FRAME_COLLOW2,
                                  MODPATTERN_FRAME_COLLOW2,
                                  MODPATTERN_FRAME_COLLOW1,
                                  _ww,_hh);
        
    count+=RenderUtils::buildQuad(&(pts[count]),
                                  0,     _hh-(char_height+2-0),
                                  col_ofs, _hh-(char_height+2-0),
                                  col_ofs,  _hh-(char_height+2-0)-2,
                                  0,      _hh-(char_height+2-0)-2,
                                  MODPATTERN_FRAME_COLHIGH1,
                                  MODPATTERN_FRAME_COLHIGH1,
                                  MODPATTERN_FRAME_COLHIGH2,
                                  MODPATTERN_FRAME_COLHIGH2,
                                  _ww,_hh);


    //then draw channels frame
    for (int i=1; i<=chanNb; i++) {
        if (pixOfs+col_size*i+col_ofs-2.0f>_ww) break;
        if ((pixOfs+col_size*i+col_ofs-2.0)<col_ofs) continue;
        count+=RenderUtils::buildQuad(&(pts[count]),
                               pixOfs+col_size*i+col_ofs-2.0f, _hh-2,
                               pixOfs+col_size*i+col_ofs-2.0f+1.0, _hh-2,
                               pixOfs+col_size*i+col_ofs-2.0f+1.0,    0,
                               pixOfs+col_size*i+col_ofs-2.0f,    0,
                                      MODPATTERN_FRAME_COLHIGH1,
                                      MODPATTERN_FRAME_COLHIGH1,
                                      MODPATTERN_FRAME_COLHIGH2,
                                      MODPATTERN_FRAME_COLHIGH2,
                                      _ww,_hh);
        
        count+=RenderUtils::buildQuad(&(pts[count]),
                                      pixOfs+col_size*i+col_ofs-1, _hh-2,
                                      pixOfs+col_size*i+col_ofs+2.0, _hh-2,
                                      pixOfs+col_size*i+col_ofs+2.0,    0,
                                      pixOfs+col_size*i+col_ofs-1,    0,
                                      MODPATTERN_FRAME_COLMED1,
                                      MODPATTERN_FRAME_COLMED1,
                                      MODPATTERN_FRAME_COLMED2,
                                      MODPATTERN_FRAME_COLMED2,
                                      _ww,_hh);
            
        count+=RenderUtils::buildQuad(&(pts[count]),
                                      pixOfs+col_size*i+col_ofs+2, _hh-2,
                                      pixOfs+col_size*i+col_ofs+2+1.0, _hh-2,
                                      pixOfs+col_size*i+col_ofs+2+1.0,    0,
                                      pixOfs+col_size*i+col_ofs,    0,
                                      MODPATTERN_FRAME_COLLOW1,
                                      MODPATTERN_FRAME_COLLOW1,
                                      MODPATTERN_FRAME_COLLOW2,
                                      MODPATTERN_FRAME_COLLOW2,
                                      _ww,_hh);
    }
    
        

    // Use the program object
    glUseProgram ( userData_simpleRender2D->programObject );
    
    GLuint positionAttribHandle = glGetAttribLocation(userData_simpleRender2D->programObject, "a_position");
    GLuint colorAttribHandle    = glGetAttribLocation(userData_simpleRender2D->programObject, "a_color");
    
    //Save opengl state
    glDumpState();
    
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    
    // Load the vertex data
    glVertexAttribPointer ( positionAttribHandle, 3, GL_FLOAT, GL_FALSE, sizeof(LineVertexF), &(pts[0].x) );
    glVertexAttribPointer ( colorAttribHandle, 4, GL_FLOAT, GL_FALSE, sizeof(LineVertexF), &(pts[0].r) );
    
    // Load the vertex data
    glEnableVertexAttribArray ( positionAttribHandle );
    glEnableVertexAttribArray ( colorAttribHandle );
    
    // Load the uniforms
    glDrawArrays(GL_TRIANGLES,0,count);
    
    glRestoreState();
    
    free(pts);
    
}

void RenderUtils::DrawChanLayoutAfter(uint _ww,uint _hh,int display_note_mode,int *volumeData,int chanNb,float pixOfs,float char_width,float char_height,float char_yOfs,int rowToHighlight,float mScaleFactor) {
    int ii;
    int colr,colg,colb,cola;
    int count=0;
    float col_size,col_ofs;
    LineVertexF *pts;
    
    switch (display_note_mode){
        case 0:col_size=11*char_width;col_ofs=(char_width)*2.5f-1;break;
        case 1:col_size=6*char_width;col_ofs=(char_width)*2.5f-1;break;
        case 2:col_size=4*char_width;col_ofs=(char_width)*2.5f-1;break;
    }
    
    pts=(LineVertexF*)malloc(sizeof(LineVertexF)*6*(3+0));
    if (!pts) {
        NSLog(@"%s - cannot allocate memory",__func__);
        return;
    }
    float min_w=col_size*chanNb+col_ofs;
    min_w=fmin(min_w,_ww);
    
    switch (display_note_mode){
        case 0:col_size=11*char_width;col_ofs=(char_width)*2+8+6-2;break;
        case 1:col_size=6*char_width;col_ofs=(char_width)*2+8+6-2;break;
        case 2:col_size=4*char_width;col_ofs=(char_width)*2+8+6-2;break;
    }
    
    //Draw current playing line
    ii=_hh-rowToHighlight*char_height-2*char_height-2-char_yOfs;
    
    colr=230;colg=76;colb=153;cola=150;
    count+=RenderUtils::buildQuad(&(pts[count]),
                                  0,     ii-1,
                                  min_w, ii-1,
                                  min_w, ii+char_height,
                                  0,     ii+char_height,
                                  colr,colg,colb,cola,
                                  colr,colg,colb,cola,
                                  colr,colg,colb,cola,
                                  colr,colg,colb,cola,
                                  _ww,_hh);
    
    count+=RenderUtils::buildQuad(&(pts[count]),
                                  0,     ii-1,
                                  min_w, ii-1,
                                  min_w, ii-3,
                                  0, ii-3,
                                  colr/2,colg/2,colb/2,cola,
                                  colr/2,colg/2,colb/2,cola,
                                  colr/2,colg/2,colb/2,cola,
                                  colr/2,colg/2,colb/2,cola,
                                  _ww,_hh);

    colr*=1.4f;if (colr>255) colr=255;
    colg*=1.4f;if (colg>255) colg=255;
    colb*=1.4f;if (colb>255) colb=255;
    cola*=1.4f;if (cola>255) cola=255;
    count+=RenderUtils::buildQuad(&(pts[count]),
                                  0,    ii+char_height-2,
                                  min_w, ii+char_height-2,
                                  min_w, ii+char_height,
                                  0, ii+char_height,
                                  colr,colg,colb,cola,
                                  colr,colg,colb,cola,
                                  colr,colg,colb,cola,
                                  colr,colg,colb,cola,
                                  _ww,_hh);
    //Volumes bar
//    if (volumeData) {
//        for (int i=0; i<chanNb; i++) {
//            if (col_size*i+col_ofs-2.0f>_ww) break;
//            
//            int cr,cg,cb,crb,cgb,cbb;
//            crb=100;
//            cgb=50;
//            cbb=150;
//            cr=crb+volumeData[i]*2; if (cr<0) cr=0; if (cr>255) cr=255;
//            cg=cgb+volumeData[i]/4; if (cg<0) cg=0; if (cg>255) cg=255;
//            cb=cbb+volumeData[i]; if (cb<0) cb=0; if (cb>255) cb=255;
//            
//            
//            
//            pts[0] = LineVertex(pixOfs+col_size*i+col_ofs+col_size*1/5, 0,    crb,cgb,cbb,125);
//            pts[1] = LineVertex(pixOfs+col_size*i+col_ofs+col_size*4/5,    0,    crb,cgb,cbb,125);
//            pts[2] = LineVertex(pixOfs+col_size*i+col_ofs+col_size*1/5,    volumeData[i]*_hh/256/5,cr,cg,cb,125);
//            pts[3] = LineVertex(pixOfs+col_size*i+col_ofs+col_size*4/5,    volumeData[i]*_hh/256/5,cr,cg,cb,125);
//        }
//    }
    
    // Use the program object
    glUseProgram ( userData_simpleRender2D->programObject );
    
    GLuint positionAttribHandle = glGetAttribLocation(userData_simpleRender2D->programObject, "a_position");
    GLuint colorAttribHandle    = glGetAttribLocation(userData_simpleRender2D->programObject, "a_color");
    
    //Save opengl state
    glDumpState();
    
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    
    // Load the vertex data
    glVertexAttribPointer ( positionAttribHandle, 3, GL_FLOAT, GL_FALSE, sizeof(LineVertexF), &(pts[0].x) );
    glVertexAttribPointer ( colorAttribHandle, 4, GL_FLOAT, GL_FALSE, sizeof(LineVertexF), &(pts[0].r) );
    
    // Load the vertex data
    glEnableVertexAttribArray ( positionAttribHandle );
    glEnableVertexAttribArray ( colorAttribHandle );
    
    // Load the uniforms
    glDrawArrays(GL_TRIANGLES,0,count);
    
    glRestoreState();
    
    free(pts);
    
}

/* Reduces A Normal Vector (3 Coordinates)       */
/* To A Unit Normal Vector With A Length Of One. */
void RenderUtils::ReduceToUnit(GLfloat vector[3]) {
    /* Holds Unit Length */
    GLfloat length;
    
    /* Calculates The Length Of The Vector */
    length=(GLfloat)sqrt((vector[0]*vector[0])+(vector[1]*vector[1])+(vector[2]*vector[2]));
    
    /* Prevents Divide By 0 Error By Providing */
    if (length==0.0f)
    {
        /* An Acceptable Value For Vectors To Close To 0. */
        length=1.0f;
    }
    
    vector[0]/=length;  /* Dividing Each Element By */
    vector[1]/=length;  /* The Length Results In A  */
    vector[2]/=length;  /* Unit Normal Vector.      */
}

/* Calculates Normal For A Quad Using 3 Points */
void RenderUtils::calcNormal(GLfloat v[3][3], GLfloat out[3]) {
    /* Vector 1 (x,y,z) & Vector 2 (x,y,z) */
    GLfloat v1[3], v2[3];
    /* Define X Coord */
    static const int x=0;
    /* Define Y Coord */
    static const int y=1;
    /* Define Z Coord */
    static const int z=2;
    
    /* Finds The Vector Between 2 Points By Subtracting */
    /* The x,y,z Coordinates From One Point To Another. */
    
    /* Calculate The Vector From Point 1 To Point 0 */
    v1[x]=v[0][x]-v[1][x];      /* Vector 1.x=Vertex[0].x-Vertex[1].x */
    v1[y]=v[0][y]-v[1][y];      /* Vector 1.y=Vertex[0].y-Vertex[1].y */
    v1[z]=v[0][z]-v[1][z];      /* Vector 1.z=Vertex[0].y-Vertex[1].z */
    
    /* Calculate The Vector From Point 2 To Point 1 */
    v2[x]=v[1][x]-v[2][x];      /* Vector 2.x=Vertex[0].x-Vertex[1].x */
    v2[y]=v[1][y]-v[2][y];      /* Vector 2.y=Vertex[0].y-Vertex[1].y */
    v2[z]=v[1][z]-v[2][z];      /* Vector 2.z=Vertex[0].z-Vertex[1].z */
    
    /* Compute The Cross Product To Give Us A Surface Normal */
    out[x]=v1[y]*v2[z]-v1[z]*v2[y];     /* Cross Product For Y - Z */
    out[y]=v1[z]*v2[x]-v1[x]*v2[z];     /* Cross Product For X - Z */
    out[z]=v1[x]*v2[y]-v1[y]*v2[x];     /* Cross Product For X - Y */
    
    ReduceToUnit(out);          /* Normalize The Vectors */
}

void RenderUtils::drawbar(float x,float y,float z,float sx,float sy,float sz,float crt,float cgt,float cbt) {
    float cr,cg,cb;
    //top
    cr=crt;cg=cgt;cb=cbt;
    vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
    vertices[0][0]=x;
    vertices[0][1]=y;
    vertices[0][2]=z+sz;
    vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
    vertices[1][0]=x+sx;
    vertices[1][1]=y;
    vertices[1][2]=z+sz;
    vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
    vertices[2][0]=x;
    vertices[2][1]=y+sy;
    vertices[2][2]=z+sz;
    vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
    vertices[3][0]=x+sx;
    vertices[3][1]=y+sy;
    vertices[3][2]=z+sz;
    
    normals[0][0]=0;
    normals[0][1]=0;
    normals[0][2]=1;
    normals[1][0]=0;
    normals[1][1]=0;
    normals[1][2]=1;
    normals[2][0]=0;
    normals[2][1]=0;
    normals[2][2]=1;
    normals[3][0]=0;
    normals[3][1]=0;
    normals[3][2]=1;
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    //left
    cr=crt/2;cg=cgt/2;cb=cbt/2;
    vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
    vertices[0][0]=x;
    vertices[0][1]=y;
    vertices[0][2]=z;
    vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
    vertices[1][0]=x;
    vertices[1][1]=y+sy;
    vertices[1][2]=z;
    vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
    vertices[2][0]=x;
    vertices[2][1]=y;
    vertices[2][2]=z+sz;
    vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
    vertices[3][0]=x;
    vertices[3][1]=y+sy;
    vertices[3][2]=z+sz;
    
    normals[0][0]=-1;
    normals[0][1]=0;
    normals[0][2]=0;
    normals[1][0]=-1;
    normals[1][1]=0;
    normals[1][2]=0;
    normals[2][0]=-1;
    normals[2][1]=0;
    normals[2][2]=0;
    normals[3][0]=-1;
    normals[3][1]=0;
    normals[3][2]=0;
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    //right
    vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
    vertices[0][0]=x+sx;
    vertices[0][1]=y;
    vertices[0][2]=z;
    vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
    vertices[1][0]=x+sx;
    vertices[1][1]=y+sy;
    vertices[1][2]=z;
    vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
    vertices[2][0]=x+sx;
    vertices[2][1]=y;
    vertices[2][2]=z+sz;
    vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
    vertices[3][0]=x+sx;
    vertices[3][1]=y+sy;
    vertices[3][2]=z+sz;
    
    normals[0][0]=1;
    normals[0][1]=0;
    normals[0][2]=0;
    normals[1][0]=1;
    normals[1][1]=0;
    normals[1][2]=0;
    normals[2][0]=1;
    normals[2][1]=0;
    normals[2][2]=0;
    normals[3][0]=1;
    normals[3][1]=0;
    normals[3][2]=0;
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    //up
    vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
    vertices[0][0]=x;
    vertices[0][1]=y+sy;
    vertices[0][2]=z;
    vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
    vertices[1][0]=x+sx;
    vertices[1][1]=y+sy;
    vertices[1][2]=z;
    vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
    vertices[2][0]=x;
    vertices[2][1]=y+sy;
    vertices[2][2]=z+sz;
    vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
    vertices[3][0]=x+sx;
    vertices[3][1]=y+sy;
    vertices[3][2]=z+sz;
    
    normals[0][0]=0;
    normals[0][1]=1;
    normals[0][2]=0;
    normals[1][0]=0;
    normals[1][1]=1;
    normals[1][2]=0;
    normals[2][0]=0;
    normals[2][1]=1;
    normals[2][2]=0;
    normals[3][0]=0;
    normals[3][1]=1;
    normals[3][2]=0;
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    //down
    vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
    vertices[0][0]=x;
    vertices[0][1]=y;
    vertices[0][2]=z;
    vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
    vertices[1][0]=x+sx;
    vertices[1][1]=y;
    vertices[1][2]=z;
    vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
    vertices[2][0]=x;
    vertices[2][1]=y;
    vertices[2][2]=z+sz;
    vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
    vertices[3][0]=x+sx;
    vertices[3][1]=y;
    vertices[3][2]=z+sz;
    
    normals[0][0]=0;
    normals[0][1]=-1;
    normals[0][2]=0;
    normals[1][0]=0;
    normals[1][1]=-1;
    normals[1][2]=0;
    normals[2][0]=0;
    normals[2][1]=-1;
    normals[2][2]=0;
    normals[3][0]=0;
    normals[3][1]=-1;
    normals[3][2]=0;
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    //back
    cr=crt/4;cg=cgt/4;cb=cbt/4;
    vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
    vertices[0][0]=x;
    vertices[0][1]=y;
    vertices[0][2]=z;
    vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
    vertices[1][0]=x+sx;
    vertices[1][1]=y;
    vertices[1][2]=z;
    vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
    vertices[2][0]=x;
    vertices[2][1]=y+sy;
    vertices[2][2]=z;
    vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
    vertices[3][0]=x+sx;
    vertices[3][1]=y+sy;
    vertices[3][2]=z;
    
    normals[0][0]=0;
    normals[0][1]=0;
    normals[0][2]=-1;
    normals[1][0]=0;
    normals[1][1]=0;
    normals[1][2]=-1;
    normals[2][0]=0;
    normals[2][1]=0;
    normals[2][2]=-1;
    normals[3][0]=0;
    normals[3][1]=0;
    normals[3][2]=-1;
    
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    
}

void RenderUtils::drawbar3(float x,float y,float z,float sx,float sy,float sz,float crt,float cgt,float cbt) {
    float cr,cg,cb;
    cr=crt;cg=cgt;cb=cbt;
    vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
    vertices[0][0]=x+sx;
    vertices[0][1]=y;
    vertices[0][2]=z;
    vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
    vertices[1][0]=x+sx;
    vertices[1][1]=y+sy;
    vertices[1][2]=z;
    vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
    vertices[2][0]=x+sx;
    vertices[2][1]=y;
    vertices[2][2]=z+sz;
    vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
    vertices[3][0]=x+sx;
    vertices[3][1]=y+sy;
    vertices[3][2]=z+sz;
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void RenderUtils::drawbar2(float x,float y,float z,float sx,float sy,float sz,float crt,float cgt,float cbt) {
    float cr,cg,cb;
    //top
    cr=crt;cg=cgt;cb=cbt;
    vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
    vertices[0][0]=x;
    vertices[0][1]=y;
    vertices[0][2]=z+sz;
    vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
    vertices[1][0]=x+sx;
    vertices[1][1]=y;
    vertices[1][2]=z+sz;
    vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
    vertices[2][0]=x;
    vertices[2][1]=y+sy;
    vertices[2][2]=z+sz;
    vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
    vertices[3][0]=x+sx;
    vertices[3][1]=y+sy;
    vertices[3][2]=z+sz;
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    //left
    cr=crt/2;cg=cgt/2;cb=cbt/2;
    vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
    vertices[0][0]=x;
    vertices[0][1]=y;
    vertices[0][2]=z;
    vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
    vertices[1][0]=x;
    vertices[1][1]=y+sy;
    vertices[1][2]=z;
    vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
    vertices[2][0]=x;
    vertices[2][1]=y;
    vertices[2][2]=z+sz;
    vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
    vertices[3][0]=x;
    vertices[3][1]=y+sy;
    vertices[3][2]=z+sz;
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    //right
    vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
    vertices[0][0]=x+sx;
    vertices[0][1]=y;
    vertices[0][2]=z;
    vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
    vertices[1][0]=x+sx;
    vertices[1][1]=y+sy;
    vertices[1][2]=z;
    vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
    vertices[2][0]=x+sx;
    vertices[2][1]=y;
    vertices[2][2]=z+sz;
    vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
    vertices[3][0]=x+sx;
    vertices[3][1]=y+sy;
    vertices[3][2]=z+sz;
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    //up
    vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
    vertices[0][0]=x;
    vertices[0][1]=y+sy;
    vertices[0][2]=z;
    vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
    vertices[1][0]=x+sx;
    vertices[1][1]=y+sy;
    vertices[1][2]=z;
    vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
    vertices[2][0]=x;
    vertices[2][1]=y+sy;
    vertices[2][2]=z+sz;
    vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
    vertices[3][0]=x+sx;
    vertices[3][1]=y+sy;
    vertices[3][2]=z+sz;
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    //down
    vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
    vertices[0][0]=x;
    vertices[0][1]=y;
    vertices[0][2]=z;
    vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
    vertices[1][0]=x+sx;
    vertices[1][1]=y;
    vertices[1][2]=z;
    vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
    vertices[2][0]=x;
    vertices[2][1]=y;
    vertices[2][2]=z+sz;
    vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
    vertices[3][0]=x+sx;
    vertices[3][1]=y;
    vertices[3][2]=z+sz;
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    //back
    cr=crt/4;cg=cgt/4;cb=cbt/4;
    vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
    vertices[0][0]=x;
    vertices[0][1]=y;
    vertices[0][2]=z;
    vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
    vertices[1][0]=x+sx;
    vertices[1][1]=y;
    vertices[1][2]=z;
    vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
    vertices[2][0]=x;
    vertices[2][1]=y+sy;
    vertices[2][2]=z;
    vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
    vertices[3][0]=x+sx;
    vertices[3][1]=y+sy;
    vertices[3][2]=z;
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    
}

float barSpectrumDataL[SPECTRUM_BANDS];
float barSpectrumDataR[SPECTRUM_BANDS];

void RenderUtils::DrawSpectrum2D(short int *spectrumDataL,short int *spectrumDataR,uint ww,uint hh,int mode,int nb_spectrum_bands,float mScaleFactor,bool bloom) {
    if (!renderIsInit) return;
    
    LineVertexF *pts;
    int index=0;
    float spL,spR;
    float crt,cgt,cbt;
    float x,y,sx,sy;
    
//    if (bloom) RenderUtils::startRenderToTexture(ww*mScaleFactor,hh*mScaleFactor);
//    if (bloom) RenderUtils::startRenderToTexture(ww,hh);
#if 1
    for (int i=0;i<nb_spectrum_bands;i++) {
        barSpectrumDataL[i]=1.0f*(float)spectrumDataL[i]/512.0f;
        barSpectrumDataR[i]=1.0f*(float)spectrumDataR[i]/512.0f;
    }
    
    pts=(LineVertexF*)malloc(sizeof(LineVertexF)*6*nb_spectrum_bands*2);
    
    glDumpState();
    
    // Use the program object
    glUseProgram ( userData_simpleRender2D->programObject );
    
    GLuint positionAttribHandle = glGetAttribLocation(userData_simpleRender2D->programObject, "a_position");
    GLuint colorAttribHandle    = glGetAttribLocation(userData_simpleRender2D->programObject, "a_color");
    
    glDisable(GL_BLEND);
//    glEnable(GL_BLEND);
//    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    
    crt=0;
    cgt=0;
    cbt=0;
    
    for (int i=0; i<nb_spectrum_bands; i++) {
        /////////////////
        //LEFT
        spL=barSpectrumDataL[i];
        
        if (i<nb_spectrum_bands*2/3) {
            cbt=(float)(nb_spectrum_bands*2/3-i)/(nb_spectrum_bands*2/3);
        } else {
            cbt=0;
        }
        if (i>nb_spectrum_bands/3) {
            cgt=(float)(i-nb_spectrum_bands/3)/(nb_spectrum_bands*2/3);
        } else {
            cgt=0;
        }
        crt=1-fabs(i-nb_spectrum_bands/2)/(nb_spectrum_bands/2);
        
        if (spL>2) crt+=(spL-2)*0.05f;
        if (spL>2) cgt+=(spL-2)*0.05f;
        if (spL>2) cbt+=(spL-2)*0.05f;
        
        crt*=0.5f+(1*spL);
        if (crt>1) crt=1;
        cgt*=0.5f+(1*spL);
        if (cgt>1) cgt=1;
        cbt*=0.5f+(1*spL);
        if (cbt>1) cbt=1;
        
        if (mode==1) {
            x=(float)ww*(i+4)/((float)nb_spectrum_bands+8);
            sx=(float)ww/(nb_spectrum_bands+8)-1;
            y=(float)hh/2+(float)hh/8;
            sy=spL*(float)hh/32;
            
            pts[index++] = LineVertexF(x, y,crt,cgt,cbt,1.0f);
            pts[index++] = LineVertexF(x, y+sy,crt,cgt,cbt,1.0f);
            pts[index++] = LineVertexF(x+sx, y+sy,crt,cgt,cbt,1.0f);
            
            pts[index++] = LineVertexF(x+sx, y+sy,crt,cgt,cbt,1.0f);
            pts[index++] = LineVertexF(x+sx, y,crt,cgt,cbt,1.0f);
            pts[index++] = LineVertexF(x, y,crt,cgt,cbt,1.0f);
        } else if (mode==2) {
            x=ww*(i+4)/(nb_spectrum_bands+8);
            sx=ww/(nb_spectrum_bands+8)-1;
            y=hh/2;
            sy=spL*hh/32;
            
            pts[index++] = LineVertexF(x, y,crt,cgt,cbt,1.0f);
            pts[index++] = LineVertexF(x, y+sy,crt,cgt,cbt,1.0f);
            pts[index++] = LineVertexF(x+sx, y+sy,crt,cgt,cbt,1.0f);
            
            pts[index++] = LineVertexF(x+sx, y+sy,crt,cgt,cbt,1.0f);
            pts[index++] = LineVertexF(x+sx, y,crt,cgt,cbt,1.0f);
            pts[index++] = LineVertexF(x, y,crt,cgt,cbt,1.0f);
        }
        
        /////////////////
        //RIGHT
        spR=barSpectrumDataR[i];
        
        if (i<nb_spectrum_bands*2/3) {
            cbt=(float)(nb_spectrum_bands*2/3-i)/(nb_spectrum_bands*2/3);
        } else {
            cbt=0.1;
        }
        if (i>nb_spectrum_bands/3) {
            cgt=(float)(i-nb_spectrum_bands/3)/(nb_spectrum_bands*2/3)+0.1;
        } else {
            cgt=0.1;
        }
        crt=1-fabs(i-nb_spectrum_bands/2)/(nb_spectrum_bands/2);
        
        if (spR>2) crt+=(spR-2)*0.05f;
        if (spR>2) cgt+=(spR-2)*0.05f;
        if (spR>2) cbt+=(spR-2)*0.05f;
        
        crt*=0.5+(spR);
        if (crt>1) crt=1;
        cgt*=0.5+(spR);
        if (cgt>1) cgt=1;
        cbt*=0.5+(spR);
        if (cbt>1) cbt=1;
        
        
        if (mode==1) {
            x=(float)ww*(i+4)/((float)nb_spectrum_bands+8);
            sx=(float)ww/((float)nb_spectrum_bands+8)-1;
            y=(float)hh/2+(float)hh/4;
            sy=spR*(float)hh/32;
            
            y=(float)hh/2-(float)hh/8;
            
            pts[index++] = LineVertexF(x, y,crt,cgt,cbt,1.0f);
            pts[index++] = LineVertexF(x, y+sy,crt,cgt,cbt,1.0f);
            pts[index++] = LineVertexF(x+sx, y+sy,crt,cgt,cbt,1.0f);
            
            pts[index++] = LineVertexF(x+sx, y+sy,crt,cgt,cbt,1.0f);
            pts[index++] = LineVertexF(x+sx, y,crt,cgt,cbt,1.0f);
            pts[index++] = LineVertexF(x, y,crt,cgt,cbt,1.0f);
        } else if (mode==2) {
            x=ww*(i+4)/(nb_spectrum_bands+8);
            sx=ww/(nb_spectrum_bands+8)-1;
            y=hh/2;
            sy=-spR*hh/32;
            
            pts[index++] = LineVertexF(x, y,crt,cgt,cbt,1.0f);
            pts[index++] = LineVertexF(x, y+sy,crt,cgt,cbt,1.0f);
            pts[index++] = LineVertexF(x+sx, y+sy,crt,cgt,cbt,1.0f);
            
            pts[index++] = LineVertexF(x+sx, y+sy,crt,cgt,cbt,1.0f);
            pts[index++] = LineVertexF(x+sx, y,crt,cgt,cbt,1.0f);
            pts[index++] = LineVertexF(x, y,crt,cgt,cbt,1.0f);
        }
    }
    
    for (int i=0;i<index;i++) {
        pts[i].x=2*pts[i].x/ww-1;
        pts[i].y=2*pts[i].y/hh-1;
    }
    
    // Load the vertex data
    glVertexAttribPointer ( positionAttribHandle, 2, GL_FLOAT, GL_FALSE, sizeof(LineVertexF), &(pts[0].x) );
    glVertexAttribPointer ( colorAttribHandle, 4, GL_FLOAT, GL_FALSE, sizeof(LineVertexF), &(pts[0].r) );
    
    // Load the vertex data
    glEnableVertexAttribArray ( positionAttribHandle );
    glEnableVertexAttribArray ( colorAttribHandle );
    
    // Load the uniforms
    
    glDrawArrays(GL_TRIANGLES, 0, index);

    glRestoreState();
    
    free(pts);
    
//    if (bloom) RenderUtils::endRenderToTexture(ww*mScaleFactor,hh*mScaleFactor,bloom_size);
//    if (bloom) RenderUtils::endRenderToTexture(ww,hh,bloom_size);
#endif
}


void RenderUtils::DrawSpectrum3DBar(short int *spectrumDataL,short int *spectrumDataR,uint ww,uint hh,float angle,int mode,int nb_spectrum_bands,int mirror) {
#if 0
    GLfloat spL,spR;
    GLfloat crt,cgt,cbt;
    GLfloat x,y,z,sx,sy,sz;
    static int frameCpt=0;
    
    if (frameCpt==0) {
        frameCpt=arc4random()&32767;
        memset(barSpectrumDataL,0,sizeof(float)*SPECTRUM_BANDS);
        memset(barSpectrumDataR,0,sizeof(float)*SPECTRUM_BANDS);
    }
    for (int i=0;i<nb_spectrum_bands;i++) {
        /*barSpectrumDataL[i]=barSpectrumDataL[i]*0.8;
         barSpectrumDataR[i]=barSpectrumDataR[i]*0.8;
         if (barSpectrumDataL[i]<(float)spectrumDataL[i]/512.0f)
         */
        barSpectrumDataL[i]=(float)spectrumDataL[i]/512.0f;
        /*if (barSpectrumDataR[i]<(float)spectrumDataR[i]/512.0f) */
        barSpectrumDataR[i]=(float)spectrumDataR[i]/512.0f;
    }
    
    //////////////////////////////
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float aspectRatio = (float)ww/(float)hh;
    float _hw;// = 16*1.0/2;//0.2f;
    float _hh;// = _hw/aspectRatio;
    //glFrustumf(-_hw, _hw, -_hh, _hh, 100.0f, 10000.0f);
    
    
    switch (mode) {
        case 1:
            _hw = (float)nb_spectrum_bands*0.99/2;
            _hh = _hw/aspectRatio;
            break;
        case 2:
            _hw = 16*1.0/2;
            _hh = _hw/aspectRatio;
            break;
        case 3:
            _hw = (float)nb_spectrum_bands*0.99/2;
            _hh = _hw/aspectRatio;
            break;
        case 4:
            _hw = (float)nb_spectrum_bands*0.99/2;
            _hh = _hw/aspectRatio;
            break;
    }
    
    glFrustumf(-_hw, _hw, -_hh, _hh, 100.0f, 10000.0f);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glPushMatrix();                     /* Push The Modelview Matrix */
    glEnable(GL_COLOR_MATERIAL);
    glEnable( GL_LIGHTING );
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, position );
    glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 90);
    
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight[2]);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight[2]);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight[2] );
    
    
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    
    glVertexPointer(3, GL_FLOAT, 0, vertices);
    glColorPointer(4, GL_FLOAT, 0, vertColor);
    glNormalPointer(GL_FLOAT, 0, normals);
    
    frameCpt++;
    
    switch (mode) {
        case 1:
            glTranslatef(0.0, 0.0, -150.0+
                         15*(0.8f*sin((float)frameCpt*0.1f*3.14159f/991)+
                             1.7f*sin((float)frameCpt*0.1f*3.14159f/3065)-
                             0.3f*sin((float)frameCpt*0.1f*3.14159f/5009)));
            
            glRotatef(-90+5.0f*(0.8f*sin((float)frameCpt*0.1f*3.14159f/2691)+
                                0.7f*sin((float)frameCpt*0.1f*3.14159f/3113)-
                                0.3f*sin((float)frameCpt*0.1f*3.14159f/7409)),0,0,1);
            
            
            glRotatef(3*360.0f*(0.5f*sin((float)frameCpt*0.1f*3.14159f/761)-
                                0.7f*sin((float)frameCpt*0.1f*3.14159f/1211)-
                                0.9f*sin((float)frameCpt*0.1f*3.14159f/2213)), 0, 1, 0);
            
            
            glRotatef(5.0f*(0.8f*sin((float)frameCpt*0.1f*3.14159f/891)-
                            0.2f*sin((float)frameCpt*0.1f*3.14159f/211)-
                            0.4f*sin((float)frameCpt*0.1f*3.14159f/5213)),0,0,1);
            
            break;
        case 2:
            glTranslatef(0.0, 0.0, -190.0+
                         15*(0.8f*sin((float)frameCpt*0.1f*3.14159f/991)+
                             1.7f*sin((float)frameCpt*0.1f*3.14159f/3065)-
                             0.3f*sin((float)frameCpt*0.1f*3.14159f/5009)));
            
            
            
            glRotatef(20+10.0f*(0.8f*sin((float)frameCpt*0.1f*3.14159f/2691)+
                                0.7f*sin((float)frameCpt*0.1f*3.14159f/3113)-
                                0.3f*sin((float)frameCpt*0.1f*3.14159f/7409)),1,0,0);
            
            glRotatef(5.0f*(0.8f*sin((float)frameCpt*0.1f*3.14159f/891)-
                            0.2f*sin((float)frameCpt*0.1f*3.14159f/211)-
                            0.4f*sin((float)frameCpt*0.1f*3.14159f/5213)),0,0,1);
            
            
            glRotatef(360.0f*(0.5f*sin((float)frameCpt*0.1f*3.14159f/761)-
                              0.7f*sin((float)frameCpt*0.1f*3.14159f/1211)-
                              0.9f*sin((float)frameCpt*0.1f*3.14159f/2213)), 0, 1, 0);
            
            break;
        case 3:
            glTranslatef(0.0, 0.0, -150.0+
                         15*(0.8f*sin((float)frameCpt*0.1f*3.14159f/991)+
                             1.7f*sin((float)frameCpt*0.1f*3.14159f/3065)-
                             0.3f*sin((float)frameCpt*0.1f*3.14159f/5009)));
            
            glRotatef(-90+5.0f*(0.8f*sin((float)frameCpt*0.1f*3.14159f/2691)+
                                0.7f*sin((float)frameCpt*0.1f*3.14159f/3113)-
                                0.3f*sin((float)frameCpt*0.1f*3.14159f/7409)),0,0,1);
            
            
            glRotatef(3*360.0f*(0.5f*sin((float)frameCpt*0.1f*3.14159f/761)-
                                0.7f*sin((float)frameCpt*0.1f*3.14159f/1211)-
                                0.9f*sin((float)frameCpt*0.1f*3.14159f/2213)), 0, 1, 0);
            
            
            glRotatef(5.0f*(0.8f*sin((float)frameCpt*0.1f*3.14159f/891)-
                            0.2f*sin((float)frameCpt*0.1f*3.14159f/211)-
                            0.4f*sin((float)frameCpt*0.1f*3.14159f/5213)),0,0,1);
            
            break;
        case 4:
            glTranslatef(0.0, 0.0, -150.0+
                         0*(0.8f*sin((float)frameCpt*0.1f*3.14159f/991)+
                            1.7f*sin((float)frameCpt*0.1f*3.14159f/3065)-
                            0.3f*sin((float)frameCpt*0.1f*3.14159f/5009)));
            
            glRotatef(-90+5.0f*(0.8f*sin((float)frameCpt*0.1f*3.14159f/2691)+
                                0.7f*sin((float)frameCpt*0.1f*3.14159f/3113)-
                                0.3f*sin((float)frameCpt*0.1f*3.14159f/7409)),0,0,1);
            
            
            glRotatef(90+0*360.0f*(0.5f*sin((float)frameCpt*0.1f*3.14159f/761)-
                                   0.7f*sin((float)frameCpt*0.1f*3.14159f/1211)-
                                   0.9f*sin((float)frameCpt*0.1f*3.14159f/2213)), 0, 1, 0);
            
            
            glRotatef(5.0f*(0.8f*sin((float)frameCpt*0.1f*3.14159f/891)-
                            0.2f*sin((float)frameCpt*0.1f*3.14159f/211)-
                            0.4f*sin((float)frameCpt*0.1f*3.14159f/5213)),0,0,1);
            
            break;
    }
    
    vertColor[0][3]=vertColor[1][3]=vertColor[2][3]=vertColor[3][3]=1;
    crt=0;
    cgt=0;
    cbt=0;
    
    float ang=0;
    x=-0.5;y=0;z=0;
    sx=sy=24.0/(float)nb_spectrum_bands;
    float trans=14+sx;
    
    if (mode==2) {
        for (int i=0; i<nb_spectrum_bands; i++) {
            /////////////////
            //LEFT
            spL=barSpectrumDataL[i];
            
            if (i<nb_spectrum_bands*2/3) {
                cbt=(float)(nb_spectrum_bands*2/3-i)/(nb_spectrum_bands*2/3);
            } else {
                cbt=0;
            }
            if (i>nb_spectrum_bands/3) {
                cgt=(float)(i-nb_spectrum_bands/3)/(nb_spectrum_bands*2/3);
            } else {
                cgt=0;
            }
            crt=1-fabs(i-nb_spectrum_bands/2)/(nb_spectrum_bands/2);
            crt*=0.5+(spL/1);
            if (crt>1) crt=1;
            cgt*=0.5+(spL/1);
            if (cgt>1) cgt=1;
            cbt*=0.5+(spL/1);
            if (cbt>1) cbt=1;
            
            sz=(spL+0.1f);
            x=0-sx/2;
            y=4+ang/10;
            z=-4-ang/10-spL/4;
            
            //y=(i-nb_spectrum_bands/2)*sy*1.2;
            //z=1+spL/4;
            
            glTranslatef(0,-2,trans);
            glRotatef(ang+270,1,0,0);
            drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
            glRotatef(-(ang+270),1,0,0);
            glTranslatef(0,2,-trans);
            
            glRotatef(180,0,1,0);
            
            glTranslatef(0,-2,trans);
            glRotatef(ang+270,1,0,0);
            drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
            glRotatef(-(ang+270),1,0,0);
            glTranslatef(0,2,-trans);
            
            
            glRotatef(180,0,1,0);
            
            
            
            /////////////////
            //RIGHT
            spR=barSpectrumDataR[i];
            /////////////////
            if (i<nb_spectrum_bands*2/3) {
                cbt=(float)(nb_spectrum_bands*2/3-i)/(nb_spectrum_bands*2/3);
            } else {
                cbt=0;
            }
            if (i>nb_spectrum_bands/3) {
                cgt=(float)(i-nb_spectrum_bands/3)/(nb_spectrum_bands*2/3);
            } else {
                cgt=0;
            }
            crt=1-fabs(i-nb_spectrum_bands/2)/(nb_spectrum_bands/2);
            crt*=0.5+(spR/1);
            if (crt>1) crt=1;
            cgt*=0.5+(spR/1);
            if (cgt>1) cgt=1;
            cbt*=0.5+(spR/1);
            if (cbt>1) cbt=1;
            
            sz=(spR+0.1f);
            x=0-sx/2;
            y=4+ang/10;
            z=-4-ang/10-spR/4;
            
            //y=(i-nb_spectrum_bands/2)*sy*1.2;
            //z=1+spL/4;
            
            glRotatef(90,0,1,0);
            
            glTranslatef(0,-2,trans);
            glRotatef(ang+270,1,0,0);
            drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
            glRotatef(-(ang+270),1,0,0);
            glTranslatef(0,2,-trans);
            
            glRotatef(180,0,1,0);
            
            glTranslatef(0,-2,trans);
            glRotatef(ang+270,1,0,0);
            drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
            glRotatef(-(ang+270),1,0,0);
            glTranslatef(0,2,-trans);
            
            
            glRotatef(180+90,0,1,0);
            
            if (ang<90) ang+=(90.0/(float)nb_spectrum_bands)*1.1;
            if (ang>90) ang=90;
            
            
        }
        
        glRotatef(180,0,0,1);
        glTranslatef(0,14,0);
        ang=0;
        x=-0.5;y=0;z=0;
        sx=sy=24.0/(float)nb_spectrum_bands;
        trans=14+sx;
        if (mirror)
            for (int i=0; i<nb_spectrum_bands; i++) {
                /////////////////
                //LEFT
                spL=barSpectrumDataL[i];
                
                if (i<nb_spectrum_bands*2/3) {
                    cbt=(float)(nb_spectrum_bands*2/3-i)/(nb_spectrum_bands*2/3);
                } else {
                    cbt=0;
                }
                if (i>nb_spectrum_bands/3) {
                    cgt=(float)(i-nb_spectrum_bands/3)/(nb_spectrum_bands*2/3);
                } else {
                    cgt=0;
                }
                crt=1-fabs(i-nb_spectrum_bands/2)/(nb_spectrum_bands/2);
                crt*=0.5+(spL/1);
                if (crt>1) crt=1;
                cgt*=0.5+(spL/1);
                if (cgt>1) cgt=1;
                cbt*=0.5+(spL/1);
                if (cbt>1) cbt=1;
                
                crt*=0.5;cgt*=0.5;cbt*=0.5;
                
                sz=(spL+0.1f);
                x=0-sx/2;
                y=4+ang/10;
                z=-4-ang/10-spL/4;
                
                //y=(i-nb_spectrum_bands/2)*sy*1.2;
                //z=1+spL/4;
                
                glTranslatef(0,-2,trans);
                glRotatef(ang+270,1,0,0);
                drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
                glRotatef(-(ang+270),1,0,0);
                glTranslatef(0,2,-trans);
                
                glRotatef(180,0,1,0);
                
                glTranslatef(0,-2,trans);
                glRotatef(ang+270,1,0,0);
                drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
                glRotatef(-(ang+270),1,0,0);
                glTranslatef(0,2,-trans);
                
                
                glRotatef(180,0,1,0);
                
                
                
                /////////////////
                //RIGHT
                spR=barSpectrumDataR[i];
                /////////////////
                if (i<nb_spectrum_bands*2/3) {
                    cbt=(float)(nb_spectrum_bands*2/3-i)/(nb_spectrum_bands*2/3);
                } else {
                    cbt=0;
                }
                if (i>nb_spectrum_bands/3) {
                    cgt=(float)(i-nb_spectrum_bands/3)/(nb_spectrum_bands*2/3);
                } else {
                    cgt=0;
                }
                crt=1-fabs(i-nb_spectrum_bands/2)/(nb_spectrum_bands/2);
                crt*=0.5+(spR/1);
                if (crt>1) crt=1;
                cgt*=0.5+(spR/1);
                if (cgt>1) cgt=1;
                cbt*=0.5+(spR/1);
                if (cbt>1) cbt=1;
                
                crt*=0.5;cgt*=0.5;cbt*=0.5;
                
                sz=(spR+0.1f);
                x=0-sx/2;
                y=4+ang/10;
                z=-4-ang/10-spR/4;
                
                //y=(i-nb_spectrum_bands/2)*sy*1.2;
                //z=1+spL/4;
                
                glRotatef(90,0,1,0);
                
                glTranslatef(0,-2,trans);
                glRotatef(ang+270,1,0,0);
                drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
                glRotatef(-(ang+270),1,0,0);
                glTranslatef(0,2,-trans);
                
                glRotatef(180,0,1,0);
                
                glTranslatef(0,-2,trans);
                glRotatef(ang+270,1,0,0);
                drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
                glRotatef(-(ang+270),1,0,0);
                glTranslatef(0,2,-trans);
                
                
                glRotatef(180+90,0,1,0);
                
                if (ang<90) ang+=(90.0/(float)nb_spectrum_bands)*1.1;
                if (ang>90) ang=90;
                
                
            }
    }
    if (mode==1) {
        for (int i=0; i<nb_spectrum_bands; i++) {
            /////////////////
            //LEFT
            spL=barSpectrumDataL[i];
            
            if (i<nb_spectrum_bands*2/3) {
                cbt=(float)(nb_spectrum_bands*2/3-i)/(nb_spectrum_bands*2/3);
            } else {
                cbt=0;
            }
            if (i>nb_spectrum_bands/3) {
                cgt=(float)(i-nb_spectrum_bands/3)/(nb_spectrum_bands*2/3);
            } else {
                cgt=0;
            }
            crt=1-fabs(i-nb_spectrum_bands/2)/(nb_spectrum_bands/2);
            crt*=0.5+(spL/1);
            if (crt>1) crt=1;
            cgt*=0.5+(spL/1);
            if (cgt>1) cgt=1;
            cbt*=0.5+(spL/1);
            if (cbt>1) cbt=1;
            
            
            sx=1;
            sy=1;
            sz=spL+0.1f;
            x=0-sx/2;
            y=(i-nb_spectrum_bands/2)*sy*1.2;
            z=1+spL/4;
            
            drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
            glRotatef(180, 0, 1, 0);
            drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
            
            /////////////////
            //RIGHT
            spR=barSpectrumDataR[i];
            
            if (i<nb_spectrum_bands*2/3) {
                cbt=(float)(nb_spectrum_bands*2/3-i)/(nb_spectrum_bands*2/3);
            } else {
                cbt=0.1;
            }
            if (i>nb_spectrum_bands/3) {
                cgt=(float)(i-nb_spectrum_bands/3)/(nb_spectrum_bands*2/3)+0.1;
            } else {
                cgt=0.1;
            }
            crt=1-fabs(i-nb_spectrum_bands/2)/(nb_spectrum_bands/2);
            crt*=0.5+(spR/1);
            if (crt>1) crt=1;
            cgt*=0.5+(spR/1);
            if (cgt>1) cgt=1;
            cbt*=0.5+(spR/1);
            if (cbt>1) cbt=1;
            
            
            sx=1;
            sy=1;
            sz=spR+0.1f;
            x=0-sx/2;
            y=(i-nb_spectrum_bands/2)*sy*1.2;
            z=1+spR/4;
            
            glRotatef(90, 0, 1, 0);
            drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
            glRotatef(180, 0, 1, 0);
            drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
            
            glRotatef(180-90, 0, 1, 0);
        }
        
        glRotatef(-3*360.0f*(0.5f*sin((float)frameCpt*0.1f*3.14159f/761)-
                             0.7f*sin((float)frameCpt*0.1f*3.14159f/1211)-
                             0.9f*sin((float)frameCpt*0.1f*3.14159f/2213)), 0, 1, 0);
        
        //glRotatef(180,0,0,1);
        glTranslatef(12,0,0);
        
        glRotatef(-3*360.0f*(0.5f*sin((float)frameCpt*0.1f*3.14159f/761)-
                             0.7f*sin((float)frameCpt*0.1f*3.14159f/1211)-
                             0.9f*sin((float)frameCpt*0.1f*3.14159f/2213)), 0, 1, 0);
        
        ang=0;
        if (mirror)
            for (int i=0; i<nb_spectrum_bands; i++) {
                /////////////////
                //LEFT
                spL=barSpectrumDataL[i];
                
                if (i<nb_spectrum_bands*2/3) {
                    cbt=(float)(nb_spectrum_bands*2/3-i)/(nb_spectrum_bands*2/3);
                } else {
                    cbt=0;
                }
                if (i>nb_spectrum_bands/3) {
                    cgt=(float)(i-nb_spectrum_bands/3)/(nb_spectrum_bands*2/3);
                } else {
                    cgt=0;
                }
                crt=1-fabs(i-nb_spectrum_bands/2)/(nb_spectrum_bands/2);
                crt*=0.5+(spL/1);
                if (crt>1) crt=1;
                cgt*=0.5+(spL/1);
                if (cgt>1) cgt=1;
                cbt*=0.5+(spL/1);
                if (cbt>1) cbt=1;
                
                crt*=0.5;cgt*=0.5;cbt*=0.5;
                
                sx=1;
                sy=1;
                sz=spL+0.1f;
                x=0-sx/2;
                y=(i-nb_spectrum_bands/2)*sy*1.2;
                z=1+spL/4;
                
                drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
                glRotatef(180, 0, 1, 0);
                drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
                
                /////////////////
                //RIGHT
                spR=barSpectrumDataR[i];
                
                if (i<nb_spectrum_bands*2/3) {
                    cbt=(float)(nb_spectrum_bands*2/3-i)/(nb_spectrum_bands*2/3);
                } else {
                    cbt=0.1;
                }
                if (i>nb_spectrum_bands/3) {
                    cgt=(float)(i-nb_spectrum_bands/3)/(nb_spectrum_bands*2/3)+0.1;
                } else {
                    cgt=0.1;
                }
                crt=1-fabs(i-nb_spectrum_bands/2)/(nb_spectrum_bands/2);
                crt*=0.5+(spR/1);
                if (crt>1) crt=1;
                cgt*=0.5+(spR/1);
                if (cgt>1) cgt=1;
                cbt*=0.5+(spR/1);
                if (cbt>1) cbt=1;
                
                crt*=0.5;cgt*=0.5;cbt*=0.5;
                
                sx=1;
                sy=1;
                sz=spR+0.1f;
                x=0-sx/2;
                y=(i-nb_spectrum_bands/2)*sy*1.2;
                z=1+spR/4;
                
                glRotatef(90, 0, 1, 0);
                drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
                glRotatef(180, 0, 1, 0);
                drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
                
                glRotatef(180-90, 0, 1, 0);
            }
    }
    
    if (mode==4) {
        for (int i=0; i<nb_spectrum_bands; i++) {
            /////////////////
            //LEFT
            spL=barSpectrumDataL[i];
            
            if (i<nb_spectrum_bands*2/3) {
                cbt=(float)(nb_spectrum_bands*2/3-i)/(nb_spectrum_bands*2/3);
            } else {
                cbt=0;
            }
            if (i>nb_spectrum_bands/3) {
                cgt=(float)(i-nb_spectrum_bands/3)/(nb_spectrum_bands*2/3);
            } else {
                cgt=0;
            }
            crt=1-fabs(i-nb_spectrum_bands/2)/(nb_spectrum_bands/2);
            crt*=0.5+(spL/1);
            if (crt>1) crt=1;
            cgt*=0.5+(spL/1);
            if (cgt>1) cgt=1;
            cbt*=0.5+(spL/1);
            if (cbt>1) cbt=1;
            
            
            sx=1;
            sy=1;
            sz=spL+0.1f;
            x=0-sx/2;
            y=(i-nb_spectrum_bands/2)*sy*1.2;
            z=0.1f;//+spL/2;
            
            drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
            
            /////////////////
            //RIGHT
            spR=barSpectrumDataR[i];
            
            if (i<nb_spectrum_bands*2/3) {
                cbt=(float)(nb_spectrum_bands*2/3-i)/(nb_spectrum_bands*2/3);
            } else {
                cbt=0.1;
            }
            if (i>nb_spectrum_bands/3) {
                cgt=(float)(i-nb_spectrum_bands/3)/(nb_spectrum_bands*2/3)+0.1;
            } else {
                cgt=0.1;
            }
            crt=1-fabs(i-nb_spectrum_bands/2)/(nb_spectrum_bands/2);
            crt*=0.5+(spR/1);
            if (crt>1) crt=1;
            cgt*=0.5+(spR/1);
            if (cgt>1) cgt=1;
            cbt*=0.5+(spR/1);
            if (cbt>1) cbt=1;
            
            
            sx=1;
            sy=1;
            sz=spR+0.1f;
            x=0-sx/2;
            y=(i-nb_spectrum_bands/2)*sy*1.2;
            z=0.1f;//+spR/2;
            
            //glRotatef(90, 0, 1, 0);
            //drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
            glRotatef(180, 0, 1, 0);
            drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
            
            glRotatef(180, 0, 1, 0);
        }
        
        /*glRotatef(-3*360.0f*(0.5f*sin((float)frameCpt*0.1f*3.14159f/761)-
         0.7f*sin((float)frameCpt*0.1f*3.14159f/1211)-
         0.9f*sin((float)frameCpt*0.1f*3.14159f/2213)), 0, 1, 0);
         */
        //glRotatef(180,0,0,1);
        glTranslatef(0,0,12);
        
        glRotatef(-45,0,1,0);
        
        /*glRotatef(-3*360.0f*(0.5f*sin((float)frameCpt*0.1f*3.14159f/761)-
         0.7f*sin((float)frameCpt*0.1f*3.14159f/1211)-
         0.9f*sin((float)frameCpt*0.1f*3.14159f/2213)), 0, 1, 0);
         */
        ang=0;
        if (mirror*0)
            for (int i=0; i<nb_spectrum_bands; i++) {
                /////////////////
                //LEFT
                spL=barSpectrumDataL[i];
                
                if (i<nb_spectrum_bands*2/3) {
                    cbt=(float)(nb_spectrum_bands*2/3-i)/(nb_spectrum_bands*2/3);
                } else {
                    cbt=0;
                }
                if (i>nb_spectrum_bands/3) {
                    cgt=(float)(i-nb_spectrum_bands/3)/(nb_spectrum_bands*2/3);
                } else {
                    cgt=0;
                }
                crt=1-fabs(i-nb_spectrum_bands/2)/(nb_spectrum_bands/2);
                crt*=0.5+(spL/1);
                if (crt>1) crt=1;
                cgt*=0.5+(spL/1);
                if (cgt>1) cgt=1;
                cbt*=0.5+(spL/1);
                if (cbt>1) cbt=1;
                
                crt*=0.5;cgt*=0.5;cbt*=0.5;
                
                sx=1;
                sy=1;
                sz=spL+0.1f;
                x=0-sx/2;
                y=(i-nb_spectrum_bands/2)*sy*1.2;
                z=0.1f;//+spL/4;
                
                drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
                
                /////////////////
                //RIGHT
                spR=barSpectrumDataR[i];
                
                if (i<nb_spectrum_bands*2/3) {
                    cbt=(float)(nb_spectrum_bands*2/3-i)/(nb_spectrum_bands*2/3);
                } else {
                    cbt=0.1;
                }
                if (i>nb_spectrum_bands/3) {
                    cgt=(float)(i-nb_spectrum_bands/3)/(nb_spectrum_bands*2/3)+0.1;
                } else {
                    cgt=0.1;
                }
                crt=1-fabs(i-nb_spectrum_bands/2)/(nb_spectrum_bands/2);
                crt*=0.5+(spR/1);
                if (crt>1) crt=1;
                cgt*=0.5+(spR/1);
                if (cgt>1) cgt=1;
                cbt*=0.5+(spR/1);
                if (cbt>1) cbt=1;
                
                crt*=0.5;cgt*=0.5;cbt*=0.5;
                
                sx=1;
                sy=1;
                sz=spR+0.1f;
                x=0-sx/2;
                y=(i-nb_spectrum_bands/2)*sy*1.2;
                z=0.1f;//+spR/4;
                
                glRotatef(180, 0, 1, 0);
                drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
                
                glRotatef(180, 0, 1, 0);
            }
    }
    
    if (mode==3) {
        float dsz,curve_rate;
#define absf(x) (x<0?x:-x)
        for (int i=0; i<nb_spectrum_bands; i++) {
            /////////////////
            //LEFT
            spL=barSpectrumDataL[i];
            
            if (i<nb_spectrum_bands*2/3) {
                cbt=(float)(nb_spectrum_bands*2/3-i)/(nb_spectrum_bands*2/3);
            } else {
                cbt=0;
            }
            if (i>nb_spectrum_bands/3) {
                cgt=(float)(i-nb_spectrum_bands/3)/(nb_spectrum_bands*2/3);
            } else {
                cgt=0;
            }
            crt=1-fabs(i-nb_spectrum_bands/2)/(nb_spectrum_bands/2);
            crt*=0.5+(spL/1);
            if (crt>1) crt=1;
            cgt*=0.5+(spL/1);
            if (cgt>1) cgt=1;
            cbt*=0.5+(spL/1);
            if (cbt>1) cbt=1;
            
            dsz=2+3*(0.2f*sin(frameCpt*0.05f+i*0.1f)+0.5f*sin(frameCpt*0.07f-i*0.3f+0.01f)*sin(frameCpt*0.07f-i*0.3f+0.01f)+0.5f*sin(frameCpt*0.01f+i*0.03f+0.2f)*sin(frameCpt*0.01f+i*0.03f+0.2f)*sin(frameCpt*0.01f+i*0.03f+0.2f));
            curve_rate=i*2*360/nb_spectrum_bands+360*(0.2f*sin(frameCpt*0.02f+i*0.01f)+0.5f*sin(frameCpt*0.01f-i*0.03f+0.01f)*sin(frameCpt*0.01f-i*0.03f+0.01f)+0.5f*sin(frameCpt*0.008f+i*0.003f+0.2f)*sin(frameCpt*0.008f+i*0.003f+0.2f)*sin(frameCpt*0.008f+i*0.003f+0.2f));
            
            sx=1;
            sy=1;
            sz=spL*2+0.1f;
            x=-0.5f;
            z=dsz+spL/4;
            y=(i-nb_spectrum_bands/2)*sy*1.05f;
            
            glRotatef( curve_rate ,0,1,0);
            drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
            glRotatef(90, 0, 1, 0);
            drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
            glRotatef(90, 0, -1, 0);
            glRotatef(curve_rate,0,-1,0);
            
            /////////////////
            //RIGHT
            spR=barSpectrumDataR[i];
            
            if (i<nb_spectrum_bands*2/3) {
                cbt=(float)(nb_spectrum_bands*2/3-i)/(nb_spectrum_bands*2/3);
            } else {
                cbt=0.1;
            }
            if (i>nb_spectrum_bands/3) {
                cgt=(float)(i-nb_spectrum_bands/3)/(nb_spectrum_bands*2/3)+0.1;
            } else {
                cgt=0.1;
            }
            crt=1-fabs(i-nb_spectrum_bands/2)/(nb_spectrum_bands/2);
            crt*=0.5+(spR/1);
            if (crt>1) crt=1;
            cgt*=0.5+(spR/1);
            if (cgt>1) cgt=1;
            cbt*=0.5+(spR/1);
            if (cbt>1) cbt=1;
            
            
            sx=1;
            sy=1;
            sz=spR*2+0.1f;
            x=-0.5f;
            z=dsz+spR/4;
            y=(i-nb_spectrum_bands/2)*sy*1.05f;
            
            glRotatef(180+curve_rate,0,1,0);
            drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
            glRotatef(90, 0, 1, 0);
            drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
            glRotatef(90, 0, -1, 0);
            glRotatef(180+curve_rate,0,-1,0);
            
        }
        
        glRotatef(-3*360.0f*(0.5f*sin((float)frameCpt*0.1f*3.14159f/761)-
                             0.7f*sin((float)frameCpt*0.1f*3.14159f/1211)-
                             0.9f*sin((float)frameCpt*0.1f*3.14159f/2213)), 0, 1, 0);
        
        //glRotatef(180,0,0,1);
        glTranslatef(15,0,0);
        
        glRotatef(-3*360.0f*(0.5f*sin((float)frameCpt*0.1f*3.14159f/761)-
                             0.7f*sin((float)frameCpt*0.1f*3.14159f/1211)-
                             0.9f*sin((float)frameCpt*0.1f*3.14159f/2213)), 0, 1, 0);
        
        ang=0;
        if (mirror)
            for (int i=0; i<nb_spectrum_bands; i++) {
                /////////////////
                //LEFT
                spL=barSpectrumDataL[i];
                
                if (i<nb_spectrum_bands*2/3) {
                    cbt=(float)(nb_spectrum_bands*2/3-i)/(nb_spectrum_bands*2/3);
                } else {
                    cbt=0;
                }
                if (i>nb_spectrum_bands/3) {
                    cgt=(float)(i-nb_spectrum_bands/3)/(nb_spectrum_bands*2/3);
                } else {
                    cgt=0;
                }
                crt=1-fabs(i-nb_spectrum_bands/2)/(nb_spectrum_bands/2);
                crt*=0.5+(spL/1);
                if (crt>1) crt=1;
                cgt*=0.5+(spL/1);
                if (cgt>1) cgt=1;
                cbt*=0.5+(spL/1);
                if (cbt>1) cbt=1;
                
                crt*=0.5;cgt*=0.5;cbt*=0.5;
                
                dsz=2+3*(0.2f*sin(frameCpt*0.05f+i*0.1f)+0.5f*sin(frameCpt*0.07f-i*0.3f+0.01f)*sin(frameCpt*0.07f-i*0.3f+0.01f)+0.5f*sin(frameCpt*0.01f+i*0.03f+0.2f)*sin(frameCpt*0.01f+i*0.03f+0.2f)*sin(frameCpt*0.01f+i*0.03f+0.2f));
                curve_rate=-(i*2*360/nb_spectrum_bands+360*(0.2f*sin(frameCpt*0.02f+i*0.01f)+0.5f*sin(frameCpt*0.01f-i*0.03f+0.01f)*sin(frameCpt*0.01f-i*0.03f+0.01f)+0.5f*sin(frameCpt*0.008f+i*0.003f+0.2f)*sin(frameCpt*0.008f+i*0.003f+0.2f)*sin(frameCpt*0.008f+i*0.003f+0.2f)));
                
                
                
                sx=1;
                sy=1;
                sz=spL*2+0.1f;
                x=-0.5f;
                z=dsz+spL/4;
                y=(i-nb_spectrum_bands/2)*sy*1.05f;
                
                glRotatef( curve_rate ,0,1,0);
                drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
                glRotatef(90, 0, 1, 0);
                drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
                glRotatef(90, 0, -1, 0);
                glRotatef(curve_rate,0,-1,0);
                
                
                /////////////////
                //RIGHT
                spR=barSpectrumDataR[i];
                
                if (i<nb_spectrum_bands*2/3) {
                    cbt=(float)(nb_spectrum_bands*2/3-i)/(nb_spectrum_bands*2/3);
                } else {
                    cbt=0.1;
                }
                if (i>nb_spectrum_bands/3) {
                    cgt=(float)(i-nb_spectrum_bands/3)/(nb_spectrum_bands*2/3)+0.1;
                } else {
                    cgt=0.1;
                }
                crt=1-fabs(i-nb_spectrum_bands/2)/(nb_spectrum_bands/2);
                crt*=0.5+(spR/1);
                if (crt>1) crt=1;
                cgt*=0.5+(spR/1);
                if (cgt>1) cgt=1;
                cbt*=0.5+(spR/1);
                if (cbt>1) cbt=1;
                
                crt*=0.5;cgt*=0.5;cbt*=0.5;
                
                sx=1;
                sy=1;
                sz=spR*2+0.1f;
                x=-0.5f;
                z=dsz+spR/4;
                y=(i-nb_spectrum_bands/2)*sy*1.05f;
                
                glRotatef(180+curve_rate,0,1,0);
                drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
                glRotatef(90, 0, 1, 0);
                drawbar(x,y,z,sx,sy,sz,crt,cgt,cbt);
                glRotatef(90, 0, -1, 0);
                glRotatef(180+curve_rate,0,-1,0);
            }
    }
    
    
    /* Disable Vertex Pointer */
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    
    glDisable(GL_LIGHT0);
    glDisable( GL_LIGHTING );
    glDisable(GL_COLOR_MATERIAL);
    
    
    //    glDisable(GL_BLEND);
    
    /* Pop The Matrix */
    glPopMatrix();
#endif
}


void RenderUtils::DrawSpectrum3D(short int *spectrumDataL,short int *spectrumDataR,uint ww,uint hh,float angle,int mode,int nb_spectrum_bands) {
#if 0
    GLfloat y,z,z2,spL,spR;
    GLfloat cr,cg,cb,tr,tb,tg;
    
    //////////////////////////////
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    const float aspectRatio = (float)ww/(float)hh;
    const float _hw = 0.1f;
    const float _hh = _hw/aspectRatio;
    glFrustumf(-_hw, _hw, -_hh, _hh, 1.0f, (SPECTRUM_DEPTH-1)*SPECTRUM_ZSIZE*2+120.0f);
    
    glPushMatrix();                     /* Push The Modelview Matrix */
    
    glTranslatef(0.0, 0.0, -120.0);      /* Translate 50 Units Into The Screen */
    if ((mode==3)||(mode==6)) glRotatef(angle/30.0f, 0, 0, 1);
    if ((mode==2)||(mode==5)) glRotatef(90.0f, 0, 0, 1);
    
    
    //	glEnable(GL_BLEND);
    //	glBlendFunc(GL_ONE, GL_ONE);
    
    /* Begin Drawing Quads, setup vertex array pointer */
    glVertexPointer(3, GL_FLOAT, 0, vertices);
    glColorPointer(4, GL_FLOAT, 0, vertColor);
    /* Enable Vertex Pointer */
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    
    for (int i=0;i<nb_spectrum_bands;i++) {
        oldSpectrumDataL[SPECTRUM_DEPTH-1][i]=((float)spectrumDataL[i]/128.0f<24?(float)spectrumDataL[i]/128.0f:24);
        oldSpectrumDataR[SPECTRUM_DEPTH-1][i]=((float)spectrumDataR[i]/128.0f<24?(float)spectrumDataR[i]/128.0f:24);
    }
    vertColor[0][3]=vertColor[1][3]=vertColor[2][3]=vertColor[3][3]=1;
    for (int j=1;j<SPECTRUM_DEPTH;j++) {
        for (int i=0; i<nb_spectrum_bands; i++) {
            oldSpectrumDataL[j-1][i]=oldSpectrumDataL[j][i]*SPECTRUM_DECREASE_FACTOR;
            oldSpectrumDataR[j-1][i]=oldSpectrumDataR[j][i]*SPECTRUM_DECREASE_FACTOR;
            
            z=-(j-1)*(SPECTRUM_ZSIZE);
            
            if (mode<=3) z2=z-(SPECTRUM_ZSIZE+j)*0.9f;
            else z2=z*0.9f;
            
            
            if (z>0) z=0;
            if (z2>0) z2=0;
            
            y=SPECTRUM_Y;
            spL=oldSpectrumDataL[j][i];
            spR=oldSpectrumDataR[j][i];
            
            tg=spL*2/8;
            tb=spL*1/8;
            tr=spL*3/8;
            tr=tr-(tg+tb)/2;
            cr=tb/3;
            cg=tg/3;
            cb=tr;
            
            
            
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=(GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE/(GLfloat)nb_spectrum_bands;
            vertices[0][1]=y+0;
            vertices[0][2]=z+0.0f;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[1][1]=y+0;
            vertices[1][2]=z+0.0f;
            
            
            spL*=0.5f;
            cr=tb;
            cg=tr/3;
            cb=tg;
            
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=(GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE/(GLfloat)nb_spectrum_bands;
            vertices[2][1]=y-spL;
            vertices[2][2]=z+0.0f;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[3][1]=y-spL;
            vertices[3][2]=z+0.0f;
            /* Render The Quad */
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            
            cr*=0.5f;
            cg*=0.5f;
            cb*=0.5f;
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=(GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE/(GLfloat)nb_spectrum_bands;
            vertices[0][1]=y-spL;
            vertices[0][2]=z+0.0f;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[1][1]=y-spL;
            vertices[1][2]=z+0.0f;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=(GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE/(GLfloat)nb_spectrum_bands;
            vertices[2][1]=y-spL;
            vertices[2][2]=z2;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[3][1]=y-spL;
            vertices[3][2]=z2;
            /* Render The Quad */
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            
            cr*=0.5f;
            cg*=0.5f;
            cb*=0.5f;
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[0][1]=y+0;
            vertices[0][2]=z+0.0f;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[1][1]=y-spL;
            vertices[1][2]=z+0.0f;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[2][1]=y+0;
            vertices[2][2]=z2;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[3][1]=y-spL;
            vertices[3][2]=z2;
            /* Render The Quad */
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE)/(GLfloat)nb_spectrum_bands;
            vertices[0][1]=y+0;
            vertices[0][2]=z+0.0f;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE)/(GLfloat)nb_spectrum_bands;
            vertices[1][1]=y-spL;
            vertices[1][2]=z+0.0f;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE)/(GLfloat)nb_spectrum_bands;
            vertices[2][1]=y+0;
            vertices[2][2]=z2;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE)/(GLfloat)nb_spectrum_bands;
            vertices[3][1]=y-spL;
            vertices[3][2]=z2;
            /* Render The Quad */
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            
            tg=spR*2/8;
            tb=spR*1/8;
            tr=spR*3/8;
            tr=tr-(tg+tb)/2;
            cr=tg/3;
            cg=tr/3;
            cb=tb;
            
            y=-SPECTRUM_Y;
            
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=(GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE/(GLfloat)nb_spectrum_bands;
            vertices[0][1]=y+0;
            vertices[0][2]=z+0.0f;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[1][1]=y+0;
            vertices[1][2]=z+0.0f;
            
            spR*=0.5f;
            cr=tg;
            cg=tb;
            cb=tb/3;
            
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=(GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE/(GLfloat)nb_spectrum_bands;
            vertices[2][1]=y+spR;
            vertices[2][2]=z+0.0f;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[3][1]=y+spR;
            vertices[3][2]=z+0.0f;
            /* Render The Quad */
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            
            cr*=0.5f;
            cg*=0.5f;
            cb*=0.5f;
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=(GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE/(GLfloat)nb_spectrum_bands;
            vertices[0][1]=y+spR;
            vertices[0][2]=z+0.0f;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[1][1]=y+spR;
            vertices[1][2]=z+0.0f;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=(GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE/(GLfloat)nb_spectrum_bands;
            vertices[2][1]=y+spR;
            vertices[2][2]=z2;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[3][1]=y+spR;
            vertices[3][2]=z2;
            /* Render The Quad */
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            
            cr*=0.5f;
            cg*=0.5f;
            cb*=0.5f;
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[0][1]=y+0;
            vertices[0][2]=z+0.0f;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[1][1]=y+spR;
            vertices[1][2]=z+0.0f;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[2][1]=y+0;
            vertices[2][2]=z2;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[3][1]=y+spR;
            vertices[3][2]=z2;
            /* Render The Quad */
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE)/(GLfloat)nb_spectrum_bands;
            vertices[0][1]=y+0;
            vertices[0][2]=z+0.0f;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE)/(GLfloat)nb_spectrum_bands;
            vertices[1][1]=y+spR;
            vertices[1][2]=z+0.0f;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE)/(GLfloat)nb_spectrum_bands;
            vertices[2][1]=y+0;
            vertices[2][2]=z2;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE)/(GLfloat)nb_spectrum_bands;
            vertices[3][1]=y+spR;
            vertices[3][2]=z2;
            /* Render The Quad */
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            
        }
    }
    /* Disable Vertex Pointer */
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    
    //    glDisable(GL_BLEND);
    
    /* Pop The Matrix */
    glPopMatrix();
#endif
}

void RenderUtils::DrawSpectrumLandscape3D(short int *spectrumDataL,short int *spectrumDataR,uint ww,uint hh,float angle,int mode,int nb_spectrum_bands) {
#if 0
    GLfloat y,z,z2,spL,spR;
    GLfloat cr,cg,cb,tr,tb,tg;
    
    //////////////////////////////
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    const float aspectRatio = (float)ww/(float)hh;
    const float _hw = 0.1f;
    const float _hh = _hw/aspectRatio;
    glFrustumf(-_hw, _hw, -_hh, _hh, 1.0f, (SPECTRUM_DEPTH-1)*SPECTRUM_ZSIZE*2+120.0f);
    
    glPushMatrix();                     /* Push The Modelview Matrix */
    
    glTranslatef(0.0, 0.0, -80.0);      /* Translate Into The Screen */
    if ((mode==3)||(mode==6)) glRotatef(angle/30.0f, 0, 0, 1);
    if ((mode==2)||(mode==5)) glRotatef(90.0f, 0, 0, 1);
    
    
    //	glEnable(GL_BLEND);
    //	glBlendFunc(GL_ONE, GL_ONE);
    
    /* Begin Drawing Quads, setup vertex array pointer */
    glVertexPointer(3, GL_FLOAT, 0, vertices);
    glColorPointer(4, GL_FLOAT, 0, vertColor);
    /* Enable Vertex Pointer */
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    
    for (int i=0;i<nb_spectrum_bands;i++) {
        oldSpectrumDataL[SPECTRUM_DEPTH-1][i]=((float)spectrumDataL[i]/128.0f<24?(float)spectrumDataL[i]/128.0f:24);
        oldSpectrumDataR[SPECTRUM_DEPTH-1][i]=((float)spectrumDataR[i]/128.0f<24?(float)spectrumDataR[i]/128.0f:24);
    }
    vertColor[0][3]=vertColor[1][3]=vertColor[2][3]=vertColor[3][3]=1;
    for (int j=1;j<SPECTRUM_DEPTH;j++) {
        for (int i=0; i<nb_spectrum_bands; i++) {
            oldSpectrumDataL[j-1][i]=oldSpectrumDataL[j][i];//*SPECTRUM_DECREASE_FACTOR;
            oldSpectrumDataR[j-1][i]=oldSpectrumDataR[j][i];//*SPECTRUM_DECREASE_FACTOR;
            
            z=-(j-1)*(SPECTRUM_ZSIZE);
            
            if (mode<=3) z2=z-(SPECTRUM_ZSIZE+j)*0.9f;
            else z2=z*0.9f;
            
            
            if (z>0) z=0;
            if (z2>0) z2=0;
            
            y=SPECTRUM_Y;
            spL=oldSpectrumDataL[j][i];
            spR=oldSpectrumDataR[j][i];
            
            //***********************************************************************
            //***********************************************************************
            //LEFT Channel
            //***********************************************************************
            //***********************************************************************
            
            
            tg=spL*2/8;
            tb=spL*1/8;
            tr=spL*3/8;
            tr=tr-(tg+tb)/2;
            cr=tb/3;
            cg=tg/3;
            cb=tr;
            
            
            
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=(GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE/(GLfloat)nb_spectrum_bands;
            vertices[0][1]=y+0;
            vertices[0][2]=z+0.0f;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[1][1]=y+0;
            vertices[1][2]=z+0.0f;
            
            
            spL*=0.5f;
            cr=tb;
            cg=tr/3;
            cb=tg;
            
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=(GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE/(GLfloat)nb_spectrum_bands;
            vertices[2][1]=y-spL;
            vertices[2][2]=z+0.0f;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[3][1]=y-spL;
            vertices[3][2]=z+0.0f;
            /* Render The Quad */
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            
            cr*=0.5f;
            cg*=0.5f;
            cb*=0.5f;
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=(GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE/(GLfloat)nb_spectrum_bands;
            vertices[0][1]=y-spL;
            vertices[0][2]=z+0.0f;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[1][1]=y-spL;
            vertices[1][2]=z+0.0f;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=(GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE/(GLfloat)nb_spectrum_bands;
            vertices[2][1]=y-spL;
            vertices[2][2]=z2;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[3][1]=y-spL;
            vertices[3][2]=z2;
            /* Render The Quad */
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            
            cr*=0.5f;
            cg*=0.5f;
            cb*=0.5f;
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[0][1]=y+0;
            vertices[0][2]=z+0.0f;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[1][1]=y-spL;
            vertices[1][2]=z+0.0f;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[2][1]=y+0;
            vertices[2][2]=z2;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[3][1]=y-spL;
            vertices[3][2]=z2;
            /* Render The Quad */
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            
            
            
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE)/(GLfloat)nb_spectrum_bands;
            vertices[0][1]=y+0;
            vertices[0][2]=z+0.0f;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE)/(GLfloat)nb_spectrum_bands;
            vertices[1][1]=y-spL;
            vertices[1][2]=z+0.0f;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE)/(GLfloat)nb_spectrum_bands;
            vertices[2][1]=y+0;
            vertices[2][2]=z2;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE)/(GLfloat)nb_spectrum_bands;
            vertices[3][1]=y-spL;
            vertices[3][2]=z2;
            /* Render The Quad */
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            
            //***********************************************************************
            //***********************************************************************
            //RIGHT Channel
            //***********************************************************************
            //***********************************************************************
            
            
            tg=spR*2/8;
            tb=spR*1/8;
            tr=spR*3/8;
            tr=tr-(tg+tb)/2;
            cr=tg/3;
            cg=tr/3;
            cb=tb;
            
            y=-SPECTRUM_Y;
            
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=(GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE/(GLfloat)nb_spectrum_bands;
            vertices[0][1]=y+0;
            vertices[0][2]=z+0.0f;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[1][1]=y+0;
            vertices[1][2]=z+0.0f;
            
            spR*=0.5f;
            cr=tg;
            cg=tb;
            cb=tb/3;
            
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=(GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE/(GLfloat)nb_spectrum_bands;
            vertices[2][1]=y+spR;
            vertices[2][2]=z+0.0f;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[3][1]=y+spR;
            vertices[3][2]=z+0.0f;
            /* Render The Quad */
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            
            cr*=0.5f;
            cg*=0.5f;
            cb*=0.5f;
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=(GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE/(GLfloat)nb_spectrum_bands;
            vertices[0][1]=y+spR;
            vertices[0][2]=z+0.0f;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[1][1]=y+spR;
            vertices[1][2]=z+0.0f;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=(GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE/(GLfloat)nb_spectrum_bands;
            vertices[2][1]=y+spR;
            vertices[2][2]=z2;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[3][1]=y+spR;
            vertices[3][2]=z2;
            /* Render The Quad */
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            
            cr*=0.5f;
            cg*=0.5f;
            cb*=0.5f;
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[0][1]=y+0;
            vertices[0][2]=z+0.0f;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[1][1]=y+spR;
            vertices[1][2]=z+0.0f;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[2][1]=y+0;
            vertices[2][2]=z2;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE+SPECTR_XSIZE*SPECTR_XSIZE_FACTOR)/(GLfloat)nb_spectrum_bands;
            vertices[3][1]=y+spR;
            vertices[3][2]=z2;
            /* Render The Quad */
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE)/(GLfloat)nb_spectrum_bands;
            vertices[0][1]=y+0;
            vertices[0][2]=z+0.0f;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE)/(GLfloat)nb_spectrum_bands;
            vertices[1][1]=y+spR;
            vertices[1][2]=z+0.0f;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE)/(GLfloat)nb_spectrum_bands;
            vertices[2][1]=y+0;
            vertices[2][2]=z2;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=((GLfloat)(i-nb_spectrum_bands/2)*SPECTR_XSIZE)/(GLfloat)nb_spectrum_bands;
            vertices[3][1]=y+spR;
            vertices[3][2]=z2;
            /* Render The Quad */
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            
        }
    }
    /* Disable Vertex Pointer */
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    
    //    glDisable(GL_BLEND);
    
    /* Pop The Matrix */
    glPopMatrix();
#endif
}


static int sphSize=0;
static int sphMode=0;
static GLfloat sphVert[(SPECTRUM_BANDS/2)*(SPECTRUM_BANDS/2)*4*5][3];  /* Holds Float Info For 4 Sets Of Vertices */
static GLfloat sphNorm[(SPECTRUM_BANDS/2)*(SPECTRUM_BANDS/2)*5][3];  /* Holds Float Info For 4 Sets Of Vertices */


void RenderUtils::DrawSpectrum3DMorph(short int *spectrumDataL,short int *spectrumDataR,uint ww,uint hh,float angle,int mode,int nb_spectrum_bands) {
#if 0
    GLfloat x1,x2,x3,x4,y1,y2,y3,y4,z1,z2,spL,spR;
    GLfloat cr,cg,cb,tr,tg,tb;
    //////////////////////////////
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    //	glEnable(GL_BLEND);
    //	glBlendFunc(GL_ONE, GL_ONE);
    
    
    const float aspectRatio = (float)ww/(float)hh;
    const float _hw = 0.1f;
    const float _hh = _hw/aspectRatio;
    glFrustumf(-_hw, _hw, -_hh, _hh, 1.0f, (SPECTRUM_DEPTH-1)*SPECTRUM_ZSIZE+220.0f);
    glPushMatrix();                     /* Push The Modelview Matrix */
    glTranslatef(0.0, 0.0, -180.0);      /* Translate 50 Units Into The Screen */
    if ((mode==3)||(mode==6)) glRotatef(angle/30.0f, 0, 0, 1);
    if ((mode==2)||(mode==5)) glRotatef(90.0f, 0, 0, 1);
    /* Begin Drawing Quads, setup vertex array pointer */
    glVertexPointer(3, GL_FLOAT, 0, vertices);
    glColorPointer(4, GL_FLOAT, 0, vertColor);
    /* Enable Vertex Pointer */
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    for (int i=0;i<nb_spectrum_bands;i++) {
        oldSpectrumDataL[SPECTRUM_DEPTH-1][i]=((float)spectrumDataL[i]/128.0f<24?(float)spectrumDataL[i]/128.0f:24);
        oldSpectrumDataR[SPECTRUM_DEPTH-1][i]=((float)spectrumDataR[i]/128.0f<24?(float)spectrumDataR[i]/128.0f:24);
    }
    
    vertColor[0][3]=vertColor[1][3]=vertColor[2][3]=vertColor[3][3]=1;
    
    for (int j=1;j<SPECTRUM_DEPTH;j++) {
        for (int i=0; i<nb_spectrum_bands; i++) {
            oldSpectrumDataL[j-1][i]=oldSpectrumDataL[j][i]*SPECTRUM_DECREASE_FACTOR;
            oldSpectrumDataR[j-1][i]=oldSpectrumDataR[j][i]*SPECTRUM_DECREASE_FACTOR;
            z1=-(j-1)*(SPECTRUM_ZSIZE);
            if (mode<=3) z2=z1-(SPECTRUM_ZSIZE)*0.9f;
            else z2=z1*0.9f;
            if (z1>0) z1=0;
            if (z2>0) z2=0;
            spL=oldSpectrumDataL[j][i];
            spR=oldSpectrumDataR[j][i];
            tg=spR*2/8; if (tg<0) tg=0; if (tg>255) tg=255;
            tb=spR*1/8; if (tb<0) tb=0; if (tb>255) tb=255;
            tr=spR*3/8; if (tr<0) tr=0; if (tr>255) tr=255;
            tr=tr-(tg+tb)/2;if (tr<0) tr=0;
            cr=tg/3;
            cg=tr/3;
            cb=tb;
            
            x1=(25)*cos( (((float)i+0.0f)/(nb_spectrum_bands))*3.146);
            x3=(25)*cos( (((float)i+1.0f)/(nb_spectrum_bands))*3.146);
            
            x2=(25-spL)*cos( (((float)i+0.5f)/(nb_spectrum_bands))*3.146)+(x1-x3)/2;//(25-spL)*cos( (((float)i+0.0f)/(nb_spectrum_bands))*3.146);
            x4=(25-spL)*cos( (((float)i+0.5f)/(nb_spectrum_bands))*3.146)-(x1-x3)/2;//(25-spL)*cos( (((float)i+1.0f)/(nb_spectrum_bands))*3.146);
            
            y1=(25)*sin( (((float)i+0.0f)/(nb_spectrum_bands))*3.146 );
            y3=(25)*sin( (((float)i+1.0f)/(nb_spectrum_bands))*3.146 );
            
            y2=(25-spL)*sin( (((float)i+0.5f)/(nb_spectrum_bands))*3.146 )+(y1-y3)/2;//(25-spL)*sin( (((float)i+0.0f)/(nb_spectrum_bands))*3.146 );
            y4=(25-spL)*sin( (((float)i+0.5f)/(nb_spectrum_bands))*3.146 )-(y1-y3)/2;//(25-spL)*sin( (((float)i+1.0f)/(nb_spectrum_bands))*3.146 );
            
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=x1;
            vertices[0][1]=y1;
            vertices[0][2]=z1;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=x3;
            vertices[1][1]=y3;
            vertices[1][2]=z1;
            cr=tb;
            cg=tr/3;
            cb=tg;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=x2;
            vertices[2][1]=y2;
            vertices[2][2]=z1;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=x4;
            vertices[3][1]=y4;
            vertices[3][2]=z1;
            /* Render The Quad */
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            cr*=0.25f;
            cg*=0.25f;
            cb*=0.25f;
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=x2;
            vertices[0][1]=y2;
            vertices[0][2]=z1;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=x2;
            vertices[1][1]=y2;
            vertices[1][2]=z2;
            
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=x4;
            vertices[2][1]=y4;
            vertices[2][2]=z1;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=x4;
            vertices[3][1]=y4;
            vertices[3][2]=z2;
            /* Render The Quad */
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            
            
            tg=spR*2/8; if (tg<0) tg=0; if (tg>255) tg=255;
            tb=spR*1/8; if (tb<0) tb=0; if (tb>255) tb=255;
            tr=spR*3/8; if (tr<0) tr=0; if (tr>255) tr=255;
            tr=tr-(tg+tb)/2;if (tr<0) tr=0;
            cr=tg/3;
            cg=tr/3;
            cb=tb;
            
            
            /*			x1=(25)*cos( (((float)i+0.0f)/(nb_spectrum_bands))*3.146);
             x2=(25-spR)*cos( (((float)i+0.0f)/(nb_spectrum_bands))*3.146);
             x3=(25)*cos( (((float)i+1.0f)/(nb_spectrum_bands))*3.146);
             x4=(25-spR)*cos( (((float)i+1.0f)/(nb_spectrum_bands))*3.146);
             y1=-(25)*sin( (((float)i+0.0f)/(nb_spectrum_bands))*3.146 );
             y2=-(25-spR)*sin( (((float)i+0.0f)/(nb_spectrum_bands))*3.146 );
             y3=-(25)*sin( (((float)i+1.0f)/(nb_spectrum_bands))*3.146 );
             y4=-(25-spR)*sin( (((float)i+1.0f)/(nb_spectrum_bands))*3.146 );*/
            
            x1=(25)*cos( (((float)i+0.0f)/(nb_spectrum_bands))*3.146);
            x3=(25)*cos( (((float)i+1.0f)/(nb_spectrum_bands))*3.146);
            
            x2=(25-spR)*cos( (((float)i+0.5f)/(nb_spectrum_bands))*3.146)+(x1-x3)/2;//(25-spL)*cos( (((float)i+0.0f)/(nb_spectrum_bands))*3.146);
            x4=(25-spR)*cos( (((float)i+0.5f)/(nb_spectrum_bands))*3.146)-(x1-x3)/2;//(25-spL)*cos( (((float)i+1.0f)/(nb_spectrum_bands))*3.146);
            
            y1=-(25)*sin( (((float)i+0.0f)/(nb_spectrum_bands))*3.146 );
            y3=-(25)*sin( (((float)i+1.0f)/(nb_spectrum_bands))*3.146 );
            
            y2=-(25-spR)*sin( (((float)i+0.5f)/(nb_spectrum_bands))*3.146 )+(y1-y3)/2;//(25-spL)*sin( (((float)i+0.0f)/(nb_spectrum_bands))*3.146 );
            y4=-(25-spR)*sin( (((float)i+0.5f)/(nb_spectrum_bands))*3.146 )-(y1-y3)/2;//(25-spL)*sin( (((float)i+1.0f)/(nb_spectrum_bands))*3.146 );
            
            
            
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=x1;
            vertices[0][1]=y1;
            vertices[0][2]=z1;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=x3;
            vertices[1][1]=y3;
            vertices[1][2]=z1;
            cr=tg;
            cg=tb;
            cb=tb/3;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=x2;
            vertices[2][1]=y2;
            vertices[2][2]=z1;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=x4;
            vertices[3][1]=y4;
            vertices[3][2]=z1;
            /* Render The Quad */
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            cr*=0.25f;
            cg*=0.25f;
            cb*=0.25f;
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=x2;
            vertices[0][1]=y2;
            vertices[0][2]=z1;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=x2;
            vertices[1][1]=y2;
            vertices[1][2]=z2;
            
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=x4;
            vertices[2][1]=y4;
            vertices[2][2]=z1;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=x4;
            vertices[3][1]=y4;
            vertices[3][2]=z2;
            /* Render The Quad */
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            
        }
    }
    /* Disable Vertex Pointer */
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    
    //	glDisable(GL_BLEND);
    
    
    /* Pop The Matrix */
    glPopMatrix();
#endif
}

#define MIDIFX_LEN 128*2
unsigned int data_midifx_framecpt=0;
int data_midifx_len=MIDIFX_LEN;
unsigned char data_midifx_note[MIDIFX_LEN][256];
unsigned char data_midifx_subnote[MIDIFX_LEN][256];
unsigned char data_midifx_instr[MIDIFX_LEN][256];
unsigned char data_midifx_vol[MIDIFX_LEN][256];
unsigned int data_midifx_st[MIDIFX_LEN][256];
static int data_midifx_first=1;

int data_pianofx_len=MIDIFX_LEN;
unsigned int data_pianofx_framecpt=0;
unsigned char data_pianofx_note[MIDIFX_LEN][256];
unsigned char data_pianofx_subnote[MIDIFX_LEN][256];
unsigned char data_pianofx_instr[MIDIFX_LEN][256];
unsigned char data_pianofx_vol[MIDIFX_LEN][256];
unsigned int data_pianofx_st[MIDIFX_LEN][256];
int data_pianofx_first=1;



#define VOICE_FREE	(1<<0)
#define VOICE_ON	(1<<1)
#define VOICE_SUSTAINED	(1<<2)
#define VOICE_OFF	(1<<3)
#define VOICE_DIE	(1<<4)


//unsigned int data_midifx_col[16]={
////    0x8010E7,0x5D3E79,0x29004D,0xBF7BFD,0xE7CFFD,
////     0xFF4500,0x865340,0x551700,0xFF9872,0xFFDBCE,
////     0x00E87F,0x3A7A5D,0x004E2A,0x71FDBD,0xCCFDE6,
////     0xFFF200
//    //0x868240,0x555100,0xFFF872,0xFFFDCE
//
//    0xFF5512,0x761AFF,0x21ff94,0xffb129,
//    0xcb30ff,0x38ffe4,0xfffc40,0xff47ed,
//    0x4fd9ff,0xc7ff57,0xff5eb7,0x66a8ff,
//    0x9cff6e,0xff7591,0x7d88ff,0x85ff89
//};

unsigned char piano_key[12]={0,1,0,1,0,0,1,0,1,0,1,0};
unsigned char piano_key_state[128];
unsigned char piano_key_instr[128];

//extern int texturePiano;

void RenderUtils::DrawPiano3D(uint ww,uint hh,int automove,float posx,float posy,float posz,float rotx,float roty,int color_mode) {
#if 0
    int index;
    float key_length,key_lengthBL,key_height,key_heightBL;
    float key_leftpos;
    static int piano_fxcpt;
    static int first_call=1;
    
    if (first_call) {
        memset(piano_key_state,0,128);
        memset(piano_key_instr,0,128);
        first_call=0;
        piano_fxcpt=arc4random()&0xFFF;
    }
    
    piano_fxcpt++;
    
    GLfloat yf,yn,ynBL,z,yadj;
    GLfloat cr,cg,cb,crt,cgt,cbt;
    
    //////////////////////////////
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    const float aspectRatio = (float)ww/(float)hh;
    const float _hw = 52.0/2/16;//0.2f;
    const float _hh = _hw/aspectRatio;
    glFrustumf(-_hw, _hw, -_hh, _hh, 100.0f, 10000.0f);
    
    glPushMatrix();                     /* Push The Modelview Matrix */
    
    if (automove) {
        glTranslatef(0.0, 0.0, -100.0*11);      /* Translate 50 Units Into The Screen */
        
        glRotatef(5.0f*(0.8f*sin((float)piano_fxcpt*3.14159f/769)+
                        0.5f*sin((float)piano_fxcpt*3.14159f/229)+
                        0.3f*sin((float)piano_fxcpt*3.14159f/311)), 0, 1, 0);
        
        glRotatef(30+15.0f*(0.4f*sin((float)piano_fxcpt*3.14159f/191)+
                            0.7f*sin((float)piano_fxcpt*3.14159f/911)+
                            0.3f*sin((float)piano_fxcpt*3.14159f/409)), 1, 0, 0);
    } else {
        glTranslatef(posx,posy,posz-100*12);
        glRotatef(30+rotx, 1, 0, 0);
        glRotatef(roty, 0, 1, 0);
    }
    
    
    
    int j=MIDIFX_LEN-MIDIFX_OFS-1;//-(data_pianofx_len/2);//MIDIFX_OFS;
    //glLineWidth(line_width+2);
    index=0;
    for (int i=0; i<256; i++) {
        if (data_pianofx_note[j][i]) {
            int instr=data_pianofx_instr[j][i];
            int vol=data_pianofx_vol[j][i];
            unsigned int st=data_pianofx_st[j][i];
            
            if (vol&&(st&VOICE_ON)) {
                //note pressed
                piano_key_state[(data_pianofx_note[j][i])&127]=8;
                piano_key_instr[(data_pianofx_note[j][i])&127]=instr;
            }
        }
    }
    
    yadj=0.01;
    
#define PIANO3D_DRAWKEY \
if (piano_key_state[i+k]) { \
yn=yf-key_height*4/5*piano_key_state[i+k]/8; \
ynBL=yf-key_heightBL*3/5*piano_key_state[i+k]/8; \
piano_key_state[i+k]--; \
} else { \
yn=ynBL=yf; \
} \
if (piano_ofs==12) piano_ofs=0; \
if (piano_key[piano_ofs]==0) { /*white key*/ \
if (piano_key_state[i+k]) { \
crt=(0.3f*piano_key_state[i+k]+1.0f*(8-piano_key_state[i+k]))/8; \
cgt=(0.3f*piano_key_state[i+k]+1.0f*(8-piano_key_state[i+k]))/8; \
cbt=(0.9f*piano_key_state[i+k]+1.0f*(8-piano_key_state[i+k]))/8; \
} else crt=cgt=cbt=1.0f; \
/*Key / Up Face*/ \
cr=crt;cg=cgt;cb=cbt;\
vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb; \
vertices[0][0]=(float)(white_idx-key_leftpos+0.05f); \
vertices[0][1]=yn+yadj; \
vertices[0][2]=z+0.1f;  \
vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb; \
vertices[1][0]=(float)(white_idx-key_leftpos+0.05f); \
vertices[1][1]=yf+yadj; \
vertices[1][2]=z-key_length;  \
vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb; \
vertices[2][0]=(float)(white_idx-key_leftpos+0.95f); \
vertices[2][1]=yn+yadj; \
vertices[2][2]=z+0.1f;  \
vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb; \
vertices[3][0]=(float)(white_idx-key_leftpos+0.95f); \
vertices[3][1]=yf+yadj; \
vertices[3][2]=z-key_length;  \
glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); \
/*Key / Down Face*/ \
cr=crt*0.4;cg=cgt*0.4;cb=cbt*0.4; \
vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb; \
vertices[0][0]=(float)(white_idx-key_leftpos+0.05f); \
vertices[0][1]=yn-key_height; \
vertices[0][2]=z; \
vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb; \
vertices[1][0]=(float)(white_idx-key_leftpos+0.05f); \
vertices[1][1]=yf-key_height; \
vertices[1][2]=z-key_length; \
vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb; \
vertices[2][0]=(float)(white_idx-key_leftpos+0.95f); \
vertices[2][1]=yn-key_height; \
vertices[2][2]=z; \
vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb; \
vertices[3][0]=(float)(white_idx-key_leftpos+0.95f); \
vertices[3][1]=yf-key_height; \
vertices[3][2]=z-key_length; \
glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); \
/*Key / Front Face*/ \
cr=crt*0.6f;cg=cgt*0.6f;cb=cbt*0.6f; \
vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb; \
vertices[0][0]=(float)(white_idx-key_leftpos+0.10f); \
vertices[0][1]=yn-key_height; \
vertices[0][2]=z;  \
vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb; \
vertices[1][0]=(float)(white_idx-key_leftpos+0.10f); \
vertices[1][1]=yn+0; \
vertices[1][2]=z;   \
vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb; \
vertices[2][0]=(float)(white_idx-key_leftpos+0.90f); \
vertices[2][1]=yn-key_height; \
vertices[2][2]=z;  \
vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb; \
vertices[3][0]=(float)(white_idx-key_leftpos+0.90f); \
vertices[3][1]=yn; \
vertices[3][2]=z;   \
glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); \
/*Key / Back Face*/ \
vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb; \
vertices[0][0]=(float)(white_idx-key_leftpos+0.10f); \
vertices[0][1]=yf-key_height; \
vertices[0][2]=z-key_length; \
vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb; \
vertices[1][0]=(float)(white_idx-key_leftpos+0.10f); \
vertices[1][1]=yf+0; \
vertices[1][2]=z-key_length; \
vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb; \
vertices[2][0]=(float)(white_idx-key_leftpos+0.90f); \
vertices[2][1]=yf-key_height; \
vertices[2][2]=z-key_length; \
vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb; \
vertices[3][0]=(float)(white_idx-key_leftpos+0.90f); \
vertices[3][1]=yf; \
vertices[3][2]=z-key_length; \
glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); \
/*Key / Right Face*/ \
vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb; \
vertices[0][0]=(float)(white_idx-key_leftpos+0.90f); \
vertices[0][1]=yn-key_height; \
vertices[0][2]=z;  \
vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb; \
vertices[1][0]=(float)(white_idx-key_leftpos+0.90f); \
vertices[1][1]=yn+0; \
vertices[1][2]=z;  \
vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb; \
vertices[2][0]=(float)(white_idx-key_leftpos+0.90f); \
vertices[2][1]=yf-key_height; \
vertices[2][2]=z-key_length;   \
vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb; \
vertices[3][0]=(float)(white_idx-key_leftpos+0.90f); \
vertices[3][1]=yf; \
vertices[3][2]=z-key_length;  \
glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); \
/*Key / Left Face*/ \
vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb; \
vertices[0][0]=(float)(white_idx-key_leftpos+0.10f); \
vertices[0][1]=yf-key_height; \
vertices[0][2]=z-key_length;  \
vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb; \
vertices[1][0]=(float)(white_idx-key_leftpos+0.10f); \
vertices[1][1]=yf+0; \
vertices[1][2]=z-key_length;  \
vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb; \
vertices[2][0]=(float)(white_idx-key_leftpos+0.10f); \
vertices[2][1]=yn-key_height; \
vertices[2][2]=z;  \
vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb; \
vertices[3][0]=(float)(white_idx-key_leftpos+0.10f); \
vertices[3][1]=yn; \
vertices[3][2]=z;  \
glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); \
white_idx++; \
} else { /*black key*/ \
if (piano_key_state[i+k]) { \
crt=(0.9f*piano_key_state[i+k]+0.4f*(8-piano_key_state[i+k]))/8; \
cgt=(0.3f*piano_key_state[i+k]+0.4f*(8-piano_key_state[i+k]))/8; \
cbt=(0.3f*piano_key_state[i+k]+0.4f*(8-piano_key_state[i+k]))/8; \
} else crt=cgt=cbt=0.2f; \
/*TOP*/ \
cr=crt;cg=cgt;cb=cbt;\
vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb; \
vertices[0][0]=(float)(white_idx-key_leftpos-0.15f); \
vertices[0][1]=ynBL+key_heightBL; \
vertices[0][2]=z-key_lengthBL*6/5;  \
vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb; \
vertices[1][0]=(float)(white_idx-key_leftpos-0.15f); \
vertices[1][1]=yf+key_heightBL; \
vertices[1][2]=z-key_length;  \
vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb; \
vertices[2][0]=(float)(white_idx-key_leftpos+0.15f); \
vertices[2][1]=ynBL+key_heightBL; \
vertices[2][2]=z-key_lengthBL*6/5;  \
vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb; \
vertices[3][0]=(float)(white_idx-key_leftpos+0.15f); \
vertices[3][1]=yf+key_heightBL; \
vertices[3][2]=z-key_length; \
glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); \
cr=crt*0.6f;cg=cgt*0.6f;cb=cbt*0.6f; \
/*FRONT*/ \
vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb; \
vertices[0][0]=(float)(white_idx-key_leftpos-0.3f); \
vertices[0][1]=ynBL; \
vertices[0][2]=z-key_lengthBL;   \
vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb; \
vertices[1][0]=(float)(white_idx-key_leftpos-0.15f); \
vertices[1][1]=ynBL+key_heightBL; \
vertices[1][2]=z-key_lengthBL*6/5; \
vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb; \
vertices[2][0]=(float)(white_idx-key_leftpos+0.3f); \
vertices[2][1]=ynBL; \
vertices[2][2]=z-key_lengthBL; \
vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb; \
vertices[3][0]=(float)(white_idx-key_leftpos+0.15f); \
vertices[3][1]=ynBL+key_heightBL; \
vertices[3][2]=z-key_lengthBL*6/5; \
glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); \
/*RIGHT*/ \
vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb; \
vertices[0][0]=(float)(white_idx-key_leftpos+0.3f); \
vertices[0][1]=ynBL; \
vertices[0][2]=z-key_lengthBL; \
vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb; \
vertices[1][0]=(float)(white_idx-key_leftpos+0.15f); \
vertices[1][1]=ynBL+key_heightBL; \
vertices[1][2]=z-key_lengthBL*6/5; \
vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb; \
vertices[2][0]=(float)(white_idx-key_leftpos+0.3f); \
vertices[2][1]=yf; \
vertices[2][2]=z-key_length;  \
vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb; \
vertices[3][0]=(float)(white_idx-key_leftpos+0.15f); \
vertices[3][1]=yf+key_heightBL; \
vertices[3][2]=z-key_length; \
glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); \
/*BACK*/ \
vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb; \
vertices[0][0]=(float)(white_idx-key_leftpos-0.3f); \
vertices[0][1]=yf; \
vertices[0][2]=z-key_length; \
vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb; \
vertices[1][0]=(float)(white_idx-key_leftpos-0.15f); \
vertices[1][1]=yf+key_heightBL; \
vertices[1][2]=z-key_length; \
vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb; \
vertices[2][0]=(float)(white_idx-key_leftpos+0.3f); \
vertices[2][1]=yf; \
vertices[2][2]=z-key_length; \
vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb; \
vertices[3][0]=(float)(white_idx-key_leftpos+0.15f); \
vertices[3][1]=yf+key_heightBL; \
vertices[3][2]=z-key_length; \
glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); \
/*LEFT*/ \
vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb; \
vertices[0][0]=(float)(white_idx-key_leftpos-0.3f); \
vertices[0][1]=yf; \
vertices[0][2]=z-key_length; \
vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb; \
vertices[1][0]=(float)(white_idx-key_leftpos-0.15f); \
vertices[1][1]=yf+key_heightBL; \
vertices[1][2]=z-key_length; \
vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb; \
vertices[2][0]=(float)(white_idx-key_leftpos-0.3f); \
vertices[2][1]=ynBL; \
vertices[2][2]=z-key_lengthBL; \
vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb; \
vertices[3][0]=(float)(white_idx-key_leftpos-0.15f); \
vertices[3][1]=ynBL+key_heightBL; \
vertices[3][2]=z-key_lengthBL*6/5; \
glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);  \
}
    
    //    glEnable(GL_BLEND);
    //    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    //glDisable(GL_DEPTH_TEST);
    
    /* Begin Drawing Quads, setup vertex array pointer */
    glVertexPointer(3, GL_FLOAT, 0, vertices);
    glColorPointer(4, GL_FLOAT, 0, vertColor);
    
    /* Enable Vertex Pointer */
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    
    
    
    //draw piano
    vertColor[0][3]=vertColor[1][3]=vertColor[2][3]=vertColor[3][3]=1.0f;
    int white_idx=0;
    key_length=6;
    key_lengthBL=6*4/9;
    key_height=0.8f;
    key_heightBL=0.6f;
    
    yf=-5;
    yn=-5;
    z=-0-key_length*2;
    
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    
    key_leftpos=28.0f/2;
    
    int piano_ofs=0;
    int k=0;
    z=0;
    vertColor[0][3]=vertColor[1][3]=vertColor[2][3]=vertColor[3][3]=1.0f;
    
    for (int i=0;i<48;i++,piano_ofs++) {
        PIANO3D_DRAWKEY
    }
    
    z=z-key_length;
    yf=yf+key_height*3;
    key_leftpos+=28.0f;
    
    k=48;
    for (int i=0;i<48;i++,piano_ofs++) {
        PIANO3D_DRAWKEY
    }
    
    z=z-key_length;
    yf=yf+key_height*3;
    key_leftpos+=28.0f-(28-19)/2;
    k=96;
    for (int i=0;i<32;i++,piano_ofs++) {
        PIANO3D_DRAWKEY
    }
    
    
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    
    /* Pop The Matrix */
    glPopMatrix();
    
    //    glDisable(GL_BLEND);
#endif
}


int qsort_CompareBar(const void *entryA, const void *entryB) {
    //have playing bar drawn first
    int valA;
    int valB;
    
    
    valA=((t_data_bar2draw*)entryA)->frameidx;
    valB=((t_data_bar2draw*)entryB)->frameidx;
    
    if (valA==valB) {
        //try to have min start idx first
        //        valA=((t_data_bar2draw*)entryA)->startidx;
        //        valB=((t_data_bar2draw*)entryB)->startidx;
        valA=-((t_data_bar2draw*)entryA)->played;
        valB=-((t_data_bar2draw*)entryB)->played;
        
        if (valA==valB) {
            //if same startidx, start with longer bar
            valA=-((t_data_bar2draw*)entryA)->size;
            valB=-((t_data_bar2draw*)entryB)->size;
            
            if (valA==valB) {
                //if same size, use note
                valA=(((t_data_bar2draw*)entryA)->note);
                valB=(((t_data_bar2draw*)entryB)->note);
                
                if (valA==valB) {
                    //if same note, use instr
                    valA=((t_data_bar2draw*)entryA)->instr;
                    valB=((t_data_bar2draw*)entryB)->instr;
                }
            }
        }
    }
    return valA-valB;
}

void RenderUtils::UpdateDataPiano(unsigned int *data,bool clearbuffer,bool paused) {
    //if first launch, clear buffers
    if (data_pianofx_first|clearbuffer) {
        data_pianofx_first=0;
        for (int i=0;i<data_pianofx_len;i++) {
            memset(data_pianofx_note[i],0,256);
            memset(data_pianofx_subnote[i],0,256);
            memset(data_pianofx_instr[i],0,256);
            memset(data_pianofx_vol[i],0,256);
            memset(data_pianofx_st[i],0,256*sizeof(unsigned int));
        }
    }
    //update old ones
    for (int j=0;j<data_pianofx_len-1;j++) {
        memcpy(data_pianofx_note[j],data_pianofx_note[j+1],256);
        memcpy(data_pianofx_subnote[j],data_pianofx_note[j+1],256);
        memcpy(data_pianofx_instr[j],data_pianofx_instr[j+1],256);
        memcpy(data_pianofx_vol[j],data_pianofx_vol[j+1],256);
        memcpy(data_pianofx_st[j],data_pianofx_st[j+1],256*sizeof(unsigned int));
    }
    //add new one
    for (int i=0;i<256;i++) {
        unsigned int note=data[i];
        data_pianofx_note[data_pianofx_len-1][i]=note&0xFF;
        data_pianofx_subnote[data_pianofx_len-1][i]=(note>>28)&0xF;
        data_pianofx_instr[data_pianofx_len-1][i]=(note>>8)&0xFF;
        data_pianofx_vol[data_pianofx_len-1][i]=(note>>16)&0xFF;
        data_pianofx_st[data_pianofx_len-1][i]=((note>>24)&0xF)|(data_pianofx_framecpt<<8);
    }
    
    if (settings[GLOB_FXPianoCutLine].detail.mdz_switch.switch_value==0) { //cut note bars after piano
        for (int j=0;j<data_pianofx_len-1-MIDIFX_OFS;j++) {
            memset(data_pianofx_note[j],0,256);
            memset(data_pianofx_subnote[j],0,256);
            memset(data_pianofx_instr[j],0,256);
            memset(data_pianofx_vol[j],0,256);
            memset(data_pianofx_st[j],0,256*sizeof(unsigned int));
        }
    }
    
    if (!paused) {
        data_pianofx_framecpt++;
        pianoroll_cpt++;
    }
}


void RenderUtils::DrawPiano3DWithNotesWall(uint ww,uint hh,int automove,float posx,float posy,float posz,float rotx,float roty,int color_mode,int fxquality) {
#if 0
    int index;
    float key_length,key_lengthBL,key_height,key_heightBL;
    float key_leftpos;
    static int piano_fxcpt;
    static int first_call=1;
    static int note_min=0;
    static int note_max=127;
    static float ztrans=-100*16-30;
    static float ztrans_tgt=-100*16-30;
    static int ztrans_wait=0;
    static float xtrans=0;
    static float xtrans_tgt=0;
    static float xtransSpeed_tgt=0;
    static float ztransSpeed_tgt=0;
    static int camera_pos=0;
    static int camera_pos_countdown=0;
    
    
    if (first_call) {
        
        memset(piano_key_state,0,128);
        memset(piano_key_instr,0,128);
        first_call=0;
        piano_fxcpt=arc4random()&0xFFF;
        
        for (int i=0;i<MAX_BARS*6*6;i++) vertColorBAR[i][3]=1;
    }
    
    if (camera_pos_countdown==0) {
        camera_pos=arc4random()%8;
        camera_pos_countdown=30*15+(arc4random()&511);//15s min before switching
    } else camera_pos_countdown--;
    
    //    camera_pos=5;
    
    piano_fxcpt++;
    
    GLfloat x,y,z,yf,yn,ynBL,yadj;
    GLfloat cr,cg,cb,crt,cgt,cbt;
    
    //////////////////////////////
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    const float aspectRatio = (float)ww/(float)hh;
    const float _hw = 75.0/2/16;//0.2f;
    const float _hh = _hw/aspectRatio;
    glFrustumf(-_hw, _hw, -_hh, _hh, 100.0f, 10000.0f);
    
    glPushMatrix();                     /* Push The Modelview Matrix */
    
    
    //interval to draw
    if (automove) {
        int nb_white_key=(note_max-note_min+5)*7/12+4;
        z=nb_white_key/75.0f*100*16;
        if (z<100*2) z=100*2;
        ztrans_tgt=-z-50;
        
        switch (camera_pos) {
            case 1:
            case 2:
                ztrans_tgt=-z-30;
                break;
                /*            case 3:
                 ztrans_tgt=-z-60;
                 break;*/
            case 5:
            case 7:
                ztrans_tgt=-z-30;
                break;
        }
        
        if (ztrans>ztrans_tgt) {
            ztrans=ztrans+(ztrans_tgt-ztrans)*ztransSpeed_tgt*0.5f;
            if (ztransSpeed_tgt<0.1) ztransSpeed_tgt=ztransSpeed_tgt+0.001;
            if (ztrans-ztrans_tgt<0.1) {
                ztransSpeed_tgt=0;
            }
            
            ztrans_wait=60*5+arc4random()&511;
        } else {
            if (ztrans_wait==0) {
                ztrans=ztrans+(ztrans_tgt-ztrans)*ztransSpeed_tgt*0.5f;
                if (ztransSpeed_tgt<0.1) ztransSpeed_tgt=ztransSpeed_tgt+0.001;
                if (ztrans_tgt-ztrans<0.1) {
                    ztrans_wait=60*5+arc4random()&511;
                    ztransSpeed_tgt=0;
                }
            } else ztrans_wait--;
        }
        
        xtrans_tgt=((note_max+note_min)/2-64)*7.0/12;
        xtrans=xtrans+(xtrans_tgt-xtrans)*xtransSpeed_tgt*0.5f;
        if (xtransSpeed_tgt<0.1) xtransSpeed_tgt=xtransSpeed_tgt+0.001;
        if (abs(xtrans-xtrans_tgt)<0.1) {
            xtransSpeed_tgt=0;
        }
        
        
        float roty_adj,rotx_adj,xrandfact,rotx_randfact,roty_randfact;
        xrandfact=1.0;
        rotx_randfact=8.0f;
        roty_randfact=5.0f;
        switch (camera_pos) {
            case 0:  //front
                rotx_adj=30;
                roty_adj=0;
                break;
            case 1:  //left
                rotx_adj=30;
                roty_adj=45;
                break;
            case 2:  //front
                rotx_adj=25;
                roty_adj=0;
                break;
            case 3:  //right
                rotx_adj=30;
                roty_adj=-45;
                break;
            case 4: //front
                rotx_adj=35;
                roty_adj=0;
                break;
            case 5: //left
                rotx_adj=20;
                roty_adj=75;
                xtrans=0;
                xrandfact=0.2f;
                rotx_randfact=3.0f;
                roty_randfact=2.0f;
                break;
            case 6: //front
                rotx_adj=32;
                roty_adj=0;
                break;
            case 7: //right
                rotx_adj=20;
                roty_adj=-75;
                xtrans=0;
                xrandfact=0.2f;
                rotx_randfact=3.0f;
                roty_randfact=2.0f;
                break;
        }
        
        glTranslatef(-xtrans+xrandfact*(0.9f*sin((float)piano_fxcpt*0.5*3.14159f/319)+
                                        0.5f*sin((float)piano_fxcpt*0.5*3.14159f/789)-
                                        0.7f*sin((float)piano_fxcpt*0.5*3.14159f/1061)),
                     2.0,
                     ztrans-5*(1.2f*cos((float)piano_fxcpt*0.5*3.14159f/719)+
                               0.5f*sin((float)piano_fxcpt*0.5*3.14159f/289)-
                               0.7f*sin((float)piano_fxcpt*0.5*3.14159f/361)));
        
        
        glRotatef(rotx_adj+rotx_randfact*(0.4f*sin((float)piano_fxcpt*0.5*3.14159f/91)+
                                          0.7f*sin((float)piano_fxcpt*0.5*3.14159f/911)+
                                          0.3f*sin((float)piano_fxcpt*0.5*3.14159f/409)), 1, 0, 0);
        glRotatef(roty_adj+roty_randfact*(0.8f*sin((float)piano_fxcpt*0.5*3.14159f/173)+
                                          0.5f*sin((float)piano_fxcpt*0.5*3.14159f/1029)+
                                          0.3f*sin((float)piano_fxcpt*0.5*3.14159f/511)), 0, 1, 0);
        
    } else {
        glTranslatef(posx,posy,posz-100*15);
        glRotatef(30+rotx, 1, 0, 0);
        glRotatef(roty, 0, 1, 0);
    }
    
    
    int j=data_pianofx_len-1-MIDIFX_OFS;
    //glLineWidth(line_width+2);
    index=0;
    for (int i=0; i<256; i++) {
        if (data_pianofx_note[j][i]) {
            int instr=data_pianofx_instr[j][i];
            int vol=data_pianofx_vol[j][i];
            unsigned int st=data_pianofx_st[j][i];
            
            if (vol&&(st&VOICE_ON)) {
                //note pressed
                piano_key_state[(data_pianofx_note[j][i])&127]=8;
                piano_key_instr[(data_pianofx_note[j][i])&127]=instr;
            }
        }
    }
    
    
    //    glEnable(GL_BLEND);
    //    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    //glDisable(GL_DEPTH_TEST);
    
    /* Begin Drawing Quads, setup vertex array pointer */
    glVertexPointer(3, GL_FLOAT, 0, vertices);
    glColorPointer(4, GL_FLOAT, 0, vertColor);
    
    /* Enable Vertex Pointer */
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    
    
    
    //draw piano
    int white_idx=0;
    key_length=6;
    key_lengthBL=6*4/9;
    key_height=0.8f;
    key_heightBL=0.6f;
    
    yf=-5;
    yn=-5;
    z=-0-key_length*2;
    
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    
    
    key_leftpos=75.0f/2;
    
    int  vertices_count=0;
    
    
    int piano_ofs=0;
    z=0;
    vertColor[0][3]=vertColor[1][3]=vertColor[2][3]=vertColor[3][3]=1.0f;
    yadj=0.01f;
    for (int i=0;i<128;i++) {
        if (piano_key_state[i]) {
            yn=yf-key_height*4/5*piano_key_state[i]/8;
            ynBL=yf-key_heightBL*3/5*piano_key_state[i]/8;
            piano_key_state[i]--;
            
            int colidx;//=i%12;
            if (color_mode==0) {
                colidx=(i%12);
            } else if (color_mode==1) {
                colidx=(piano_key_instr[i])&63;
            }
            
            crt=((data_midifx_col[colidx&31]>>16)&0xFF)/255.0f;
            cgt=((data_midifx_col[colidx&31]>>8)&0xFF)/255.0f;
            cbt=(data_midifx_col[colidx&31]&0xFF)/255.0f;
            
            if (colidx&0x20) {
                crt=(crt+1)/2;
                cgt=(cgt+1)/2;
                cbt=(cbt+1)/2;
            }
            
        } else {
            yn=ynBL=yf;
        }
        if (piano_ofs==12) piano_ofs=0;
        if (piano_key[piano_ofs]==0) { /*white key*/
            piano_note_type[i]=0;
            piano_note_posx[i]=(float)(white_idx-key_leftpos+0.5f);
            piano_note_posy[i]=yf+yadj;
            piano_note_posz[i]=z-key_length;
            if (piano_key_state[i]) {
                crt=(crt*piano_key_state[i]+1.0f*(8-piano_key_state[i]))/8;
                cgt=(cgt*piano_key_state[i]+1.0f*(8-piano_key_state[i]))/8;
                cbt=(cbt*piano_key_state[i]+1.0f*(8-piano_key_state[i]))/8;
            } else crt=cgt=cbt=1.0f;
            /*Key / Up Face*/
            cr=crt;cg=cgt;cb=cbt;
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=(float)(white_idx-key_leftpos+0.05f);
            vertices[0][1]=yn+yadj;
            vertices[0][2]=z+0.1f;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=(float)(white_idx-key_leftpos+0.05f);
            vertices[1][1]=yf+yadj;
            vertices[1][2]=z-key_length;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=(float)(white_idx-key_leftpos+0.95f);
            vertices[2][1]=yn+yadj;
            vertices[2][2]=z+0.1f;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=(float)(white_idx-key_leftpos+0.95f);
            vertices[3][1]=yf+yadj;
            vertices[3][2]=z-key_length;
            
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            vertices_count+=4;
            
            
            /*Key / Down Face*/
            cr=crt*0.4;cg=cgt*0.4;cb=cbt*0.4;
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=(float)(white_idx-key_leftpos+0.05f);
            vertices[0][1]=yn-key_height;
            vertices[0][2]=z;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=(float)(white_idx-key_leftpos+0.05f);
            vertices[1][1]=yf-key_height;
            vertices[1][2]=z-key_length;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=(float)(white_idx-key_leftpos+0.95f);
            vertices[2][1]=yn-key_height;
            vertices[2][2]=z;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=(float)(white_idx-key_leftpos+0.95f);
            vertices[3][1]=yf-key_height;
            vertices[3][2]=z-key_length;
            
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            vertices_count+=4;
            
            /*Key / Front Face*/
            cr=crt*0.6f;cg=cgt*0.6f;cb=cbt*0.6f;
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=(float)(white_idx-key_leftpos+0.10f);
            vertices[0][1]=yn-key_height;
            vertices[0][2]=z;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=(float)(white_idx-key_leftpos+0.10f);
            vertices[1][1]=yn+0;
            vertices[1][2]=z;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=(float)(white_idx-key_leftpos+0.90f);
            vertices[2][1]=yn-key_height;
            vertices[2][2]=z;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=(float)(white_idx-key_leftpos+0.90f);
            vertices[3][1]=yn;
            vertices[3][2]=z;
            
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            vertices_count+=4;
            
            /*Key / Back Face*/
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=(float)(white_idx-key_leftpos+0.10f);
            vertices[0][1]=yf-key_height;
            vertices[0][2]=z-key_length;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=(float)(white_idx-key_leftpos+0.10f);
            vertices[1][1]=yf+0;
            vertices[1][2]=z-key_length;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=(float)(white_idx-key_leftpos+0.90f);
            vertices[2][1]=yf-key_height;
            vertices[2][2]=z-key_length;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=(float)(white_idx-key_leftpos+0.90f);
            vertices[3][1]=yf;
            vertices[3][2]=z-key_length;
            
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            vertices_count+=4;
            
            /*Key / Right Face*/
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=(float)(white_idx-key_leftpos+0.90f);
            vertices[0][1]=yn-key_height;
            vertices[0][2]=z;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=(float)(white_idx-key_leftpos+0.90f);
            vertices[1][1]=yn+0;
            vertices[1][2]=z;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=(float)(white_idx-key_leftpos+0.90f);
            vertices[2][1]=yf-key_height;
            vertices[2][2]=z-key_length;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=(float)(white_idx-key_leftpos+0.90f);
            vertices[3][1]=yf;
            vertices[3][2]=z-key_length;
            
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            vertices_count+=4;
            
            /*Key / Left Face*/
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=(float)(white_idx-key_leftpos+0.10f);
            vertices[0][1]=yf-key_height;
            vertices[0][2]=z-key_length;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=(float)(white_idx-key_leftpos+0.10f);
            vertices[1][1]=yf+0;
            vertices[1][2]=z-key_length;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=(float)(white_idx-key_leftpos+0.10f);
            vertices[2][1]=yn-key_height;
            vertices[2][2]=z;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=(float)(white_idx-key_leftpos+0.10f);
            vertices[3][1]=yn;
            vertices[3][2]=z;
            
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            vertices_count+=4;
            
            white_idx++;
        } else { /*black key*/
            piano_note_type[i]=1;
            piano_note_posx[i]=(float)(white_idx-key_leftpos);
            piano_note_posy[i]=yf+key_heightBL;
            piano_note_posz[i]=z-key_length;
            if (piano_key_state[i]) {
                crt=(crt*piano_key_state[i]+0.4f*(8-piano_key_state[i]))/8;
                cgt=(cgt*piano_key_state[i]+0.4f*(8-piano_key_state[i]))/8;
                cbt=(cbt*piano_key_state[i]+0.4f*(8-piano_key_state[i]))/8;
            } else crt=cgt=cbt=0.2f;
            /*TOP*/
            cr=crt;cg=cgt;cb=cbt;
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=(float)(white_idx-key_leftpos-0.15f);
            vertices[0][1]=ynBL+key_heightBL;
            vertices[0][2]=z-key_lengthBL*6/5;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=(float)(white_idx-key_leftpos-0.15f);
            vertices[1][1]=yf+key_heightBL;
            vertices[1][2]=z-key_length;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=(float)(white_idx-key_leftpos+0.15f);
            vertices[2][1]=ynBL+key_heightBL;
            vertices[2][2]=z-key_lengthBL*6/5;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=(float)(white_idx-key_leftpos+0.15f);
            vertices[3][1]=yf+key_heightBL;
            vertices[3][2]=z-key_length;
            
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            vertices_count+=4;
            
            cr=crt*0.6f;cg=cgt*0.6f;cb=cbt*0.6f;
            /*FRONT*/
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=(float)(white_idx-key_leftpos-0.3f);
            vertices[0][1]=ynBL;
            vertices[0][2]=z-key_lengthBL;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=(float)(white_idx-key_leftpos-0.15f);
            vertices[1][1]=ynBL+key_heightBL;
            vertices[1][2]=z-key_lengthBL*6/5;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=(float)(white_idx-key_leftpos+0.3f);
            vertices[2][1]=ynBL;
            vertices[2][2]=z-key_lengthBL;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=(float)(white_idx-key_leftpos+0.15f);
            vertices[3][1]=ynBL+key_heightBL;
            vertices[3][2]=z-key_lengthBL*6/5;
            
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            vertices_count+=4;
            
            /*BACK*/
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=(float)(white_idx-key_leftpos-0.3f);
            vertices[0][1]=yf;
            vertices[0][2]=z-key_length;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=(float)(white_idx-key_leftpos-0.15f);
            vertices[1][1]=yf+key_heightBL;
            vertices[1][2]=z-key_length;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=(float)(white_idx-key_leftpos+0.3f);
            vertices[2][1]=yf;
            vertices[2][2]=z-key_length;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=(float)(white_idx-key_leftpos+0.15f);
            vertices[3][1]=yf+key_heightBL;
            vertices[3][2]=z-key_length;
            
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            vertices_count+=4;
            
            /*RIGHT*/
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=(float)(white_idx-key_leftpos+0.3f);
            vertices[0][1]=ynBL;
            vertices[0][2]=z-key_lengthBL;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=(float)(white_idx-key_leftpos+0.15f);
            vertices[1][1]=ynBL+key_heightBL;
            vertices[1][2]=z-key_lengthBL*6/5;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=(float)(white_idx-key_leftpos+0.3f);
            vertices[2][1]=yf;
            vertices[2][2]=z-key_length;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=(float)(white_idx-key_leftpos+0.15f);
            vertices[3][1]=yf+key_heightBL;
            vertices[3][2]=z-key_length;
            
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            vertices_count+=4;
            
            /*LEFT*/
            vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
            vertices[0][0]=(float)(white_idx-key_leftpos-0.3f);
            vertices[0][1]=yf;
            vertices[0][2]=z-key_length;
            vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
            vertices[1][0]=(float)(white_idx-key_leftpos-0.15f);
            vertices[1][1]=yf+key_heightBL;
            vertices[1][2]=z-key_length;
            vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
            vertices[2][0]=(float)(white_idx-key_leftpos-0.3f);
            vertices[2][1]=ynBL;
            vertices[2][2]=z-key_lengthBL;
            vertColor[3][0]=cr;vertColor[3][1]=cg;vertColor[3][2]=cb;
            vertices[3][0]=(float)(white_idx-key_leftpos-0.15f);
            vertices[3][1]=ynBL+key_heightBL;
            vertices[3][2]=z-key_lengthBL*6/5;
            
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            vertices_count+=4;
        }
        piano_ofs++;
    }
    
    
    //    glDisable(GL_DEPTH_TEST);
    //    glEnable(GL_BLEND);
    //    glBlendFunc(GL_ONE,GL_ONE);
    
    vertColor[0][3]=vertColor[1][3]=vertColor[2][3]=vertColor[3][3]=1.0f;
    
    int tgt_note_min=127;
    int tgt_note_max=0;
    
    int data_bar2draw_count=0;
    
    
    
    for (int i=0; i<256; i++) { //for each channels
        int j=0;
        while (j<data_pianofx_len) {  //while not having reach roof
            if (data_pianofx_note[j][i]) {  //do we have a note ?
                int instr=data_pianofx_instr[j][i];
                int vol=data_pianofx_vol[j][i];
                unsigned int st=data_pianofx_st[j][i];
                int note=data_pianofx_note[j][i];
                
                if (vol&&(st&VOICE_ON)) {  //check volume & status => we have something
                    data_bar2draw[data_bar2draw_count].startidx=j;
                    data_bar2draw[data_bar2draw_count].note=note;
                    data_bar2draw[data_bar2draw_count].instr=instr;
                    data_bar2draw[data_bar2draw_count].size=0;
                    data_bar2draw[data_bar2draw_count].played=0;
                    data_bar2draw[data_bar2draw_count].frameidx=st>>8;
                    while ( (data_pianofx_instr[j][i]==instr)&&
                           (data_pianofx_note[j][i]==note)&&
                           (data_pianofx_vol[j][i]&&
                            (data_pianofx_st[j][i]&VOICE_ON))) {  //while same bar (instru & notes), increase size
                        data_bar2draw[data_bar2draw_count].size++;
                        //propagate lowest frame nb
                        data_pianofx_st[j][i]=st;
                        if (j==(data_pianofx_len-MIDIFX_OFS-1)) data_bar2draw[data_bar2draw_count].played=1;
                        j++;
                        if (j==data_pianofx_len) break;
                        if (settings[GLOB_FXPianoCutLine].detail.mdz_switch.switch_value==1) {
                            if (j==(data_pianofx_len-MIDIFX_OFS-1)) break;
                        }
                    }
                    data_bar2draw_count++;
                } else j++;
            } else j++;
            if (data_bar2draw_count==MAX_BARS) break;
        }
        if (data_bar2draw_count==MAX_BARS) break;
    }
    qsort(data_bar2draw,data_bar2draw_count,sizeof(t_data_bar2draw),qsort_CompareBar);
    
    /*
     if (data_bar2draw_count>=2) { //propagate played flag
     for (int i=1;i<data_bar2draw_count;i++) {
     int note=data_bar2draw[i-1].note;
     int played=data_bar2draw[i-1].played;
     int instr=data_bar2draw[i-1].instr;
     
     if (played) {
     if ((data_bar2draw[i].instr==instr)&&((data_bar2draw[i].note)==note)&&
     (data_bar2draw[i].startidx<=(data_bar2draw[i-1].startidx+data_bar2draw[i-1].size)))
     data_bar2draw[i].played=1;
     }
     }
     
     for (int i=data_bar2draw_count-2;i>=0;i--) {
     int note=data_bar2draw[i+1].note;
     int played=data_bar2draw[i+1].played;
     int instr=data_bar2draw[i+1].instr;
     
     if (played) {
     if ((data_bar2draw[i].instr==instr)&&((data_bar2draw[i].note)==note)&&
     (data_bar2draw[i+1].startidx<=(data_bar2draw[i].startidx+data_bar2draw[i].size))) data_bar2draw[i].played=1;
     }
     }
     }
     
     for (int i=1;i<data_bar2draw_count;i++) {
     if ((data_bar2draw[i].instr==data_bar2draw[i-1].instr)&&
     (data_bar2draw[i].note==data_bar2draw[i-1].note)&&
     (data_bar2draw[i].startidx>=data_bar2draw[i-1].startidx)&&
     (data_bar2draw[i].startidx+data_bar2draw[i].size<=data_bar2draw[i-1].startidx+data_bar2draw[i-1].size)) {
     data_bar2draw[i].size=0;
     }
     }
     */
    int vertices_index=0;
    int indices_index=0;
    glVertexPointer(3, GL_FLOAT, 0, verticesBAR);
    glColorPointer(4, GL_FLOAT, 0, vertColorBAR);
    
    
    //TO OPTIMIZE
    unsigned int data_bar_2dmap[128*MIDIFX_LEN];
    memset(data_bar_2dmap,0,128*MIDIFX_LEN*sizeof(unsigned int));
    
    for (int i=0;i<data_bar2draw_count;i++) {
        int note=data_bar2draw[i].note&127;
        int played=data_bar2draw[i].played;
        int instr=data_bar2draw[i].instr;
        int colidx;
        if (color_mode==0) { //note
            colidx=(note%12);
        } else if (color_mode==1) { //instru
            colidx=(instr)&63;
        }
        
        if (data_bar2draw[i].size==0) continue;
        
        //printf("i:%d start:%d end:%d instr:%d note:%d played:%d\n",i,data_bar2draw[i].startidx,data_bar2draw[i].startidx+data_bar2draw[i].size,instr,note,played);
        
        
        float adj_size=0;
        int max_draw_count=0;
        for (int j=data_bar2draw[i].startidx;j<data_bar2draw[i].startidx+data_bar2draw[i].size;j++) {
            //            int _instr=(data_bar_2dmap[(note&127)*MIDIFX_LEN+j]>>16);
            int draw_count=data_bar_2dmap[(note&127)*MIDIFX_LEN+j]&255;
            if (draw_count) {
                if (adj_size<0.1f*(float)(draw_count)) adj_size=0.1f*(float)(draw_count);
                //                if (_instr!=(instr+1)) {
                draw_count++;
                if (max_draw_count<draw_count) max_draw_count=draw_count;
                //                    data_bar_2dmap[(note&127)*MIDIFX_LEN+j]=(((int)(data_bar2draw[i].instr)+1)<<16)|draw_count;
                data_bar_2dmap[(note&127)*MIDIFX_LEN+j]=draw_count;
                //                }
            } else {
                //                data_bar_2dmap[(note&127)*MIDIFX_LEN+j]=(((int)(data_bar2draw[i].instr)+1)<<16)|1;
                data_bar_2dmap[(note&127)*MIDIFX_LEN+j]=1;
                if (max_draw_count<1) max_draw_count=1;
            }
        }
        //        printf("adj: %f\n",adj_size);
        for (int j=data_bar2draw[i].startidx;j<data_bar2draw[i].startidx+data_bar2draw[i].size;j++) {
            data_bar_2dmap[(note&127)*MIDIFX_LEN+j]=max_draw_count;
        }
        
        
        crt=((data_midifx_col[colidx&31]>>16)&0xFF)/255.0f/1.0f;
        cgt=((data_midifx_col[colidx&31]>>8)&0xFF)/255.0f/1.0f;
        cbt=(data_midifx_col[colidx&31]&0xFF)/255.0f/1.0f;
        
        if (colidx&0x20) {
            crt=(crt+1)/2;
            cgt=(cgt+1)/2;
            cbt=(cbt+1)/2;
        }
        
        if (played) {
            crt=(crt*2+1*3)/5;
            cgt=(cgt*2+1*3)/5;
            cbt=(cbt*2+1*3)/5;
            if (crt>1) crt=1;
            if (cgt>1) cgt=1;
            if (cbt>1) cbt=1;
        }
        
        if ( ((data_bar2draw[i].startidx+data_bar2draw[i].size)<data_pianofx_len-MIDIFX_OFS) && !played && (settings[GLOB_FXPianoCutLine].detail.mdz_switch.switch_value==1)) { //shade note bars after piano
            crt=(crt)/2;
            cgt=(cgt)/2;
            cbt=(cbt)/2;
        }
        
        if (note>tgt_note_max) tgt_note_max=note;
        if (note<tgt_note_min) tgt_note_min=note;
        
        x=piano_note_posx[note&127];
        y=piano_note_posy[note&127]+((float)(data_bar2draw[i].startidx)-(data_pianofx_len-MIDIFX_OFS)+MIDIFX_OFS*3*0)*0.5f;
        z=piano_note_posz[note&127];
        
        float x1;
        float y1=y;
        float z1=z;
        float sx;
        float sy=0.5f*(float)data_bar2draw[i].size;
        float sz=0.1f;
        
        if (piano_note_type[note&127]) {
            //black key
            sx=0.2;
            z1+=key_length*0.55;
        } else {
            //white key
            sx=0.4;
            z1+=key_length*0.9;
        }
        
        if (played) {
            double adj=sx*0.5f;
            sx=sx+adj;
            
            //sz+=0.1f;
            //z1-=0.05f;
        }
        x1=x-sx/2;
        
        /*sx=sx-adj_size;x1=x1+adj_size/2;
         sy=sy-adj_size;y1=y1+adj_size/2;
         sz=sz-adj_size;z1=z1-adj_size/2;*/
        z1=z1-adj_size;
        
        //front
        cr=crt;cg=cgt;cb=cbt;
        vertColorBAR[vertices_index+0][0]=cr;vertColorBAR[vertices_index+0][1]=cg;vertColorBAR[vertices_index+0][2]=cb;
        verticesBAR[vertices_index+0][0]=x1;
        verticesBAR[vertices_index+0][1]=y1;
        verticesBAR[vertices_index+0][2]=z1;
        vertColorBAR[vertices_index+1][0]=cr;vertColorBAR[vertices_index+1][1]=cg;vertColorBAR[vertices_index+1][2]=cb;
        verticesBAR[vertices_index+1][0]=x1+sx;
        verticesBAR[vertices_index+1][1]=y1;
        verticesBAR[vertices_index+1][2]=z1;
        vertColorBAR[vertices_index+2][0]=cr;vertColorBAR[vertices_index+2][1]=cg;vertColorBAR[vertices_index+2][2]=cb;
        verticesBAR[vertices_index+2][0]=x1;
        verticesBAR[vertices_index+2][1]=y1+sy;
        verticesBAR[vertices_index+2][2]=z1;
        vertColorBAR[vertices_index+3][0]=cr;vertColorBAR[vertices_index+3][1]=cg;vertColorBAR[vertices_index+3][2]=cb;
        verticesBAR[vertices_index+3][0]=x1+sx;
        verticesBAR[vertices_index+3][1]=y1+sy;
        verticesBAR[vertices_index+3][2]=z1;
        //glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        if (indices_index==0) {
            vertBARindices[indices_index++]=0;
            vertBARindices[indices_index++]=1;
            vertBARindices[indices_index++]=2;
            vertBARindices[indices_index++]=3;
            vertBARindices[indices_index++]=3;
        } else {
            vertBARindices[indices_index++]=vertices_index+0;
            vertBARindices[indices_index++]=vertices_index+0;
            vertBARindices[indices_index++]=vertices_index+1;
            vertBARindices[indices_index++]=vertices_index+2;
            vertBARindices[indices_index++]=vertices_index+3;
            vertBARindices[indices_index++]=vertices_index+3;
        }
        vertices_index+=4;
        
        if (fxquality==2) {
            //back
            cr=crt;cg=cgt;cb=cbt;
            vertColorBAR[vertices_index+0][0]=cr;vertColorBAR[vertices_index+0][1]=cg;vertColorBAR[vertices_index+0][2]=cb;
            verticesBAR[vertices_index+0][0]=x1;
            verticesBAR[vertices_index+0][1]=y1;
            verticesBAR[vertices_index+0][2]=z1-sz;
            vertColorBAR[vertices_index+1][0]=cr;vertColorBAR[vertices_index+1][1]=cg;vertColorBAR[vertices_index+1][2]=cb;
            verticesBAR[vertices_index+1][0]=x1+sx;
            verticesBAR[vertices_index+1][1]=y1;
            verticesBAR[vertices_index+1][2]=z1-sz;
            vertColorBAR[vertices_index+2][0]=cr;vertColorBAR[vertices_index+2][1]=cg;vertColorBAR[vertices_index+2][2]=cb;
            verticesBAR[vertices_index+2][0]=x1;
            verticesBAR[vertices_index+2][1]=y1+sy;
            verticesBAR[vertices_index+2][2]=z1-sz;
            vertColorBAR[vertices_index+3][0]=cr;vertColorBAR[vertices_index+3][1]=cg;vertColorBAR[vertices_index+3][2]=cb;
            verticesBAR[vertices_index+3][0]=x1+sx;
            verticesBAR[vertices_index+3][1]=y1+sy;
            verticesBAR[vertices_index+3][2]=z1-sz;
            //glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            vertBARindices[indices_index++]=vertices_index+0;
            vertBARindices[indices_index++]=vertices_index+0;
            vertBARindices[indices_index++]=vertices_index+1;
            vertBARindices[indices_index++]=vertices_index+2;
            vertBARindices[indices_index++]=vertices_index+3;
            vertBARindices[indices_index++]=vertices_index+3;
            vertices_index+=4;
            
            
            cr=crt/2;cg=cgt/2;cb=cbt/2;
            //left
            vertColorBAR[vertices_index+0][0]=cr;vertColorBAR[vertices_index+0][1]=cg;vertColorBAR[vertices_index+0][2]=cb;
            verticesBAR[vertices_index+0][0]=x1;
            verticesBAR[vertices_index+0][1]=y1;
            verticesBAR[vertices_index+0][2]=z1;
            vertColorBAR[vertices_index+1][0]=cr;vertColorBAR[vertices_index+1][1]=cg;vertColorBAR[vertices_index+1][2]=cb;
            verticesBAR[vertices_index+1][0]=x1;
            verticesBAR[vertices_index+1][1]=y1;
            verticesBAR[vertices_index+1][2]=z1-sz;
            vertColorBAR[vertices_index+2][0]=cr;vertColorBAR[vertices_index+2][1]=cg;vertColorBAR[vertices_index+2][2]=cb;
            verticesBAR[vertices_index+2][0]=x1;
            verticesBAR[vertices_index+2][1]=y1+sy;
            verticesBAR[vertices_index+2][2]=z1;
            vertColorBAR[vertices_index+3][0]=cr;vertColorBAR[vertices_index+3][1]=cg;vertColorBAR[vertices_index+3][2]=cb;
            verticesBAR[vertices_index+3][0]=x1;
            verticesBAR[vertices_index+3][1]=y1+sy;
            verticesBAR[vertices_index+3][2]=z1-sz;
            //glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            vertBARindices[indices_index++]=vertices_index+0;
            vertBARindices[indices_index++]=vertices_index+0;
            vertBARindices[indices_index++]=vertices_index+1;
            vertBARindices[indices_index++]=vertices_index+2;
            vertBARindices[indices_index++]=vertices_index+3;
            vertBARindices[indices_index++]=vertices_index+3;
            vertices_index+=4;
            
            //right
            vertColorBAR[vertices_index+0][0]=cr;vertColorBAR[vertices_index+0][1]=cg;vertColorBAR[vertices_index+0][2]=cb;
            verticesBAR[vertices_index+0][0]=x1+sx;
            verticesBAR[vertices_index+0][1]=y1;
            verticesBAR[vertices_index+0][2]=z1;
            vertColorBAR[vertices_index+1][0]=cr;vertColorBAR[vertices_index+1][1]=cg;vertColorBAR[vertices_index+1][2]=cb;
            verticesBAR[vertices_index+1][0]=x1+sx;
            verticesBAR[vertices_index+1][1]=y1;
            verticesBAR[vertices_index+1][2]=z1-sz;
            vertColorBAR[vertices_index+2][0]=cr;vertColorBAR[vertices_index+2][1]=cg;vertColorBAR[vertices_index+2][2]=cb;
            verticesBAR[vertices_index+2][0]=x1+sx;
            verticesBAR[vertices_index+2][1]=y1+sy;
            verticesBAR[vertices_index+2][2]=z1;
            vertColorBAR[vertices_index+3][0]=cr;vertColorBAR[vertices_index+3][1]=cg;vertColorBAR[vertices_index+3][2]=cb;
            verticesBAR[vertices_index+3][0]=x1+sx;
            verticesBAR[vertices_index+3][1]=y1+sy;
            verticesBAR[vertices_index+3][2]=z1-sz;
            //glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            vertBARindices[indices_index++]=vertices_index+0;
            vertBARindices[indices_index++]=vertices_index+0;
            vertBARindices[indices_index++]=vertices_index+1;
            vertBARindices[indices_index++]=vertices_index+2;
            vertBARindices[indices_index++]=vertices_index+3;
            vertBARindices[indices_index++]=vertices_index+3;
            vertices_index+=4;
            
            cr=crt*1.5f;cg=cgt*1.5f;cb=cbt*1.5f;
            if (cr>1) cr=1;if (cg>1) cg=1;if (cb>1) cb=1;
            //top
            vertColorBAR[vertices_index+0][0]=cr;vertColorBAR[vertices_index+0][1]=cg;vertColorBAR[vertices_index+0][2]=cb;
            verticesBAR[vertices_index+0][0]=x1;
            verticesBAR[vertices_index+0][1]=y1+sy;
            verticesBAR[vertices_index+0][2]=z1;
            vertColorBAR[vertices_index+1][0]=cr;vertColorBAR[vertices_index+1][1]=cg;vertColorBAR[vertices_index+1][2]=cb;
            verticesBAR[vertices_index+1][0]=x1+sx;
            verticesBAR[vertices_index+1][1]=y1+sy;
            verticesBAR[vertices_index+1][2]=z1;
            vertColorBAR[vertices_index+2][0]=cr;vertColorBAR[vertices_index+2][1]=cg;vertColorBAR[vertices_index+2][2]=cb;
            verticesBAR[vertices_index+2][0]=x1;
            verticesBAR[vertices_index+2][1]=y1+sy;
            verticesBAR[vertices_index+2][2]=z1-sz;
            vertColorBAR[vertices_index+3][0]=cr;vertColorBAR[vertices_index+3][1]=cg;vertColorBAR[vertices_index+3][2]=cb;
            verticesBAR[vertices_index+3][0]=x1+sx;
            verticesBAR[vertices_index+3][1]=y1+sy;
            verticesBAR[vertices_index+3][2]=z1-sz;
            //glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            vertBARindices[indices_index++]=vertices_index+0;
            vertBARindices[indices_index++]=vertices_index+0;
            vertBARindices[indices_index++]=vertices_index+1;
            vertBARindices[indices_index++]=vertices_index+2;
            vertBARindices[indices_index++]=vertices_index+3;
            vertBARindices[indices_index++]=vertices_index+3;
            vertices_index+=4;
            
            cr=crt/3;cg=cgt/3;cb=cbt/3;
            //bottom
            vertColorBAR[vertices_index+0][0]=cr;vertColorBAR[vertices_index+0][1]=cg;vertColorBAR[vertices_index+0][2]=cb;
            verticesBAR[vertices_index+0][0]=x1;
            verticesBAR[vertices_index+0][1]=y1;
            verticesBAR[vertices_index+0][2]=z1;
            vertColorBAR[vertices_index+1][0]=cr;vertColorBAR[vertices_index+1][1]=cg;vertColorBAR[vertices_index+1][2]=cb;
            verticesBAR[vertices_index+1][0]=x1+sx;
            verticesBAR[vertices_index+1][1]=y1;
            verticesBAR[vertices_index+1][2]=z1;
            vertColorBAR[vertices_index+2][0]=cr;vertColorBAR[vertices_index+2][1]=cg;vertColorBAR[vertices_index+2][2]=cb;
            verticesBAR[vertices_index+2][0]=x1;
            verticesBAR[vertices_index+2][1]=y1;
            verticesBAR[vertices_index+2][2]=z1-sz;
            vertColorBAR[vertices_index+3][0]=cr;vertColorBAR[vertices_index+3][1]=cg;vertColorBAR[vertices_index+3][2]=cb;
            verticesBAR[vertices_index+3][0]=x1+sx;
            verticesBAR[vertices_index+3][1]=y1;
            verticesBAR[vertices_index+3][2]=z1-sz;
            //glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            vertBARindices[indices_index++]=vertices_index+0;
            vertBARindices[indices_index++]=vertices_index+0;
            vertBARindices[indices_index++]=vertices_index+1;
            vertBARindices[indices_index++]=vertices_index+2;
            vertBARindices[indices_index++]=vertices_index+3;
            vertBARindices[indices_index++]=vertices_index+3;
            vertices_index+=4;
        }
    }
    
    
    glDrawElements(GL_TRIANGLE_STRIP,indices_index,GL_UNSIGNED_SHORT,vertBARindices);
    
    //printf("vertices_count: %d\n",vertices_count+24*data_bar2draw_count);
    
    
    if (tgt_note_max>0) note_max=tgt_note_max;
    if (tgt_note_min<127) note_min=tgt_note_min;
    
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    
    //glDisable(GL_BLEND);
    //    glEnable(GL_DEPTH_TEST);
    
    /* Pop The Matrix */
    glPopMatrix();
#endif
}

void RenderUtils::UpdateDataMidiFX(unsigned int *data,bool clearBuffer,bool paused) {
    //if first launch, clear buffers
    if (data_midifx_first||clearBuffer) {
        data_midifx_first=0;
        for (int i=0;i<MIDIFX_LEN;i++) {
            memset(data_midifx_note[i],0,256);
            memset(data_midifx_subnote[i],0,256);
            memset(data_midifx_instr[i],0,256);
            memset(data_midifx_vol[i],0,256);
            memset(data_midifx_st[i],0,256*sizeof(unsigned int));
        }
    }
    if (!paused) {
        //update old ones
        for (int j=0;j<MIDIFX_LEN-1;j++) {
            memcpy(data_midifx_note[j],data_midifx_note[j+1],256);
            memcpy(data_midifx_subnote[j],data_midifx_subnote[j+1],256);
            memcpy(data_midifx_instr[j],data_midifx_instr[j+1],256);
            memcpy(data_midifx_vol[j],data_midifx_vol[j+1],256);
            memcpy(data_midifx_st[j],data_midifx_st[j+1],256*sizeof(unsigned int));
        }
        //add new one
        for (int i=0;i<256;i++) {
            unsigned int note=data[i];
            data_midifx_note[MIDIFX_LEN-1][i]=note&0xFF;
            data_midifx_subnote[MIDIFX_LEN-1][i]=(note>>28)&0xF;
            data_midifx_instr[MIDIFX_LEN-1][i]=(note>>8)&0xFF;
            data_midifx_vol[MIDIFX_LEN-1][i]=(note>>16)&0xFF;
            data_midifx_st[MIDIFX_LEN-1][i]=((note>>24)&0xF)|(data_midifx_framecpt<<8);
        }
    }
    
    if (settings[GLOB_FXMIDICutLine].detail.mdz_switch.switch_value==0) { //cut note bars after piano
        for (int j=0;j<MIDIFX_LEN-1-MIDIFX_OFS;j++) {
            memset(data_midifx_note[j],0,256);
            memset(data_midifx_subnote[j],0,256);
            memset(data_midifx_instr[j],0,256);
            memset(data_midifx_vol[j],0,256);
            memset(data_midifx_st[j],0,256*sizeof(unsigned int));
        }
    }
    
    if (settings[PIANOMIDI_MULTI_COLORSET].detail.mdz_switch.switch_value==0) data_midifx_col=data_midifx_pal1;
    else if (settings[PIANOMIDI_MULTI_COLORSET].detail.mdz_switch.switch_value==1) data_midifx_col=data_midifx_pal2;
    else if (settings[PIANOMIDI_MULTI_COLORSET].detail.mdz_switch.switch_value==2) data_midifx_col=data_midifx_pal3;
    else data_midifx_col=data_midifx_pal_custom;
    
    if (!paused) data_midifx_framecpt++;
}

void RenderUtils::DrawMidiFX(uint ww,uint hh,int horiz_vert,float note_display_range, float note_display_offset,int fx_len,int color_mode,float mScaleFactor) {
#if 0
    LineVertexF *ptsB;
    coordData *texcoords; /* Holds Float Info For 4 Sets Of Texture coordinates. */
    int crt,cgt,cbt,ca;
    int crtp[4],cgtp[4],cbtp[4],cap[4];
    int index;
    //int band_width,ofs_band;
    float band_width;
    float line_width;
    float line_width_extra;
    uint8_t sparkPresent[256];
    static uint8_t sparkIntensity[256];
    static bool first_call=true;
    
    if (first_call) {
        first_call=false;
        memset(sparkIntensity,0,sizeof(sparkIntensity));
    }
    
    
    if (fx_len>MIDIFX_LEN) fx_len=MIDIFX_LEN;
    if (fx_len<=MIDIFX_OFS) fx_len=MIDIFX_OFS+1;
    
    if (fx_len!=data_midifx_len) {
        data_midifx_len=fx_len;
        //data_midifx_first=1;
    }
    
    ptsB=(LineVertexF*)malloc(sizeof(LineVertexF)*30*MAX_BARS);
    max_indices=30*MAX_BARS;
    texcoords=(coordData*)malloc(sizeof(coordData)*256*6*8); //max 256 notes, 6pts/spark and max 8 sparks/notes
    
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    
    if (horiz_vert==0) {//Horiz
        band_width=(float)(ww+0*ww/4)/data_midifx_len;
        //        ofs_band=(ww-band_width*data_midifx_len)>>1;
        line_width=1.0f*hh/note_display_range;
    } else { //vert
        band_width=(float)(hh+0*hh/4)/data_midifx_len;
        //        ofs_band=(hh-band_width*data_midifx_len)>>1;
        line_width=1.0f*ww/note_display_range;
    }
    //line_width_extra=line_width*0.2f;
    //    if (line_width_extra<2) line_width_extra=2;
    line_width_extra=3;
    
    
    
    //glDisable(GL_BLEND);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    
    
    glVertexPointer(2, GL_FLOAT, sizeof(LineVertexF), &ptsB[0].x);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(LineVertexF), &ptsB[0].r);
    
    
    //////////////////////////////////////////////
    
    int data_bar2draw_count=0;
    
    for (int i=0; i<256; i++) { //for each channels
        int j=MIDIFX_LEN-data_midifx_len;
        while (j<MIDIFX_LEN) {  //while not having reach roof
            if (data_midifx_note[j][i]) {  //do we have a note ?
                unsigned int instr=data_midifx_instr[j][i];
                unsigned int vol=data_midifx_vol[j][i];
                unsigned int st=data_midifx_st[j][i];
                unsigned int note=data_midifx_note[j][i];
                int subnote=data_midifx_subnote[j][i];
                
                if (vol&&(st&VOICE_ON)) {  //check volume & status => we have something
                    data_bar2draw[data_bar2draw_count].startidx=j-(MIDIFX_LEN-data_midifx_len);
                    data_bar2draw[data_bar2draw_count].note=note;
                    data_bar2draw[data_bar2draw_count].subnote=subnote;
                    data_bar2draw[data_bar2draw_count].instr=instr;
                    data_bar2draw[data_bar2draw_count].size=0;
                    data_bar2draw[data_bar2draw_count].played=0;
                    data_bar2draw[data_bar2draw_count].frameidx=st>>8;
                    while ( (data_midifx_instr[j][i]==instr)&&
                           (data_midifx_note[j][i]==note)&&
                           (data_midifx_vol[j][i]<=vol)&&
                           (data_midifx_st[j][i]&VOICE_ON) ) {  //while same bar (instru & notes), increase size
                        data_bar2draw[data_bar2draw_count].size++;
                        //propagate lowest frame nb
                        data_midifx_st[j][i]=st;
                        if ((settings[GLOB_FXMIDIBarVibrato].detail.mdz_switch.switch_value==1)&&(data_midifx_subnote[j][i]!=subnote)) break;
                        //take most recent subnote if before playing bar
                        if (j<MIDIFX_LEN-MIDIFX_OFS) data_bar2draw[data_bar2draw_count].subnote=data_midifx_subnote[j][i];
                        if (j==(MIDIFX_LEN-MIDIFX_OFS-1)) data_bar2draw[data_bar2draw_count].played=1;
                        //update vol to latest encountered one, allow to retrigger if volume increases from last bar to new one and manage special case with vol = 2111121111
                        vol=data_midifx_vol[j][i];
                        j++;
                        if (settings[GLOB_FXMIDICutLine].detail.mdz_switch.switch_value==1) {
                            if (j==(MIDIFX_LEN-MIDIFX_OFS-1)) break;
                        }
                        /*if (data_midifx_vol[j][i]>vol) {
                         break;
                         }*/
                        if (j==MIDIFX_LEN) break;
                    }
                    data_bar2draw_count++;
                    //j++;
                } else j++;
            } else j++;
            if (data_bar2draw_count==MAX_BARS) break;
        }
        if (data_bar2draw_count==MAX_BARS) break;
    }
    qsort(data_bar2draw,data_bar2draw_count,sizeof(t_data_bar2draw),qsort_CompareBar);
    
    index=0;
    
    for (int i=0;i<data_bar2draw_count;i++) {
        int played=data_bar2draw[i].played;
        int note=data_bar2draw[i].note;
        int subnote=(data_bar2draw[i].subnote<8?data_bar2draw[i].subnote:data_bar2draw[i].subnote-8-7);
        
        int instr=data_bar2draw[i].instr;
        int colidx;
        if (color_mode==0) { //note
            colidx=(note%12);
        } else if (color_mode==1) { //instru
            colidx=(instr)&63;
        }
        
        if (data_bar2draw[i].size==0) continue;
        
        //printf("i:%d start:%d end:%d instr:%d note:%d played:%d\n",i,data_bar2draw[i].startidx,data_bar2draw[i].startidx+data_bar2draw[i].size,instr,note,played);
        
        
        crt=((data_midifx_col[colidx&31]>>16)&0xFF);
        cgt=((data_midifx_col[colidx&31]>>8)&0xFF);
        cbt=(data_midifx_col[colidx&31]&0xFF);
        
        if (colidx&0x20) {
            crt=(crt+255)/2;
            cgt=(cgt+255)/2;
            cbt=(cbt+255)/2;
        }
        
        if (played) {
            crt=(crt*3+255*3)/6;
            cgt=(cgt*3+255*3)/6;
            cbt=(cbt*3+255*3)/6;
            if (crt>255) crt=255;
            if (cgt>255) cgt=255;
            if (cbt>255) cbt=255;
            line_width_extra=3;
            ca=192;
        } else {
            line_width_extra=0;
            ca=192;
        }
        
        if ( ((data_bar2draw[i].startidx+data_bar2draw[i].size)<data_midifx_len-MIDIFX_OFS) && !played && (settings[GLOB_FXMIDICutLine].detail.mdz_switch.switch_value==1)) { //shade note bars after piano
            crt=(crt)/2;
            cgt=(cgt)/2;
            cbt=(cbt)/2;
        }
        
        switch (settings[GLOB_FXMIDIBarStyle].detail.mdz_switch.switch_value) {
            case 0:
                for (int ii=0;ii<4;ii++) {
                    double fact=1;
                    double ofs=0;
                    
                    crtp[ii]=crt*fact+ofs;cgtp[ii]=cgt*fact+ofs;cbtp[ii]=cbt*fact+ofs;cap[ii]=ca;
                    if (crtp[ii]>255) crtp[ii]=255;
                    if (cgtp[ii]>255) cgtp[ii]=255;
                    if (cbtp[ii]>255) cbtp[ii]=255;
                    if (cap[ii]>255) cap[ii]=255;
                }
                break;
            case 1:
                for (int ii=0;ii<4;ii++) {
                    double fact=1;
                    double ofs=0;
                    if ((ii==3)||(ii==2)) {
                        fact=1.5f;
                        ofs=64;
                    }
                    if ((ii==0)||(ii==1)) {
                        fact=0.5f;
                        ofs=0;
                    }
                    crtp[ii]=crt*fact+ofs;
                    cgtp[ii]=cgt*fact+ofs;
                    cbtp[ii]=cbt*fact+ofs;
                    cap[ii]=ca;
                    if (crtp[ii]>255) crtp[ii]=255;
                    if (cgtp[ii]>255) cgtp[ii]=255;
                    if (cbtp[ii]>255) cbtp[ii]=255;
                    if (cap[ii]>255) cap[ii]=255;
                }
                break;
            case 2:
                for (int ii=0;ii<4;ii++) {
                    double fact=1;
                    double ofs=0;
                    if ((ii==3)||(ii==2)) {
                        fact=1.5f;
                        ofs=96;
                    }
                    if ((ii==0)||(ii==1)) {
                        fact=0.5f;
                        ofs=0;
                    }
                    crtp[ii]=crt*fact+ofs;cgtp[ii]=cgt*fact+ofs;cbtp[ii]=cbt*fact+ofs;cap[ii]=ca;
                    if (crtp[ii]>255) crtp[ii]=255;
                    if (cgtp[ii]>255) cgtp[ii]=255;
                    if (cbtp[ii]>255) cbtp[ii]=255;
                    if (cap[ii]>255) cap[ii]=255;
                }
                crt=crtp[3];cgt=cgtp[3];cbt=cbtp[3];
                for (int ii=2;ii<4;ii++) {
                    crtp[ii]=crtp[0];
                    cgtp[ii]=cgtp[0];
                    cbtp[ii]=cbtp[0];
                    cap[ii]=cap[0];
                }
                break;
        }
        
        if (horiz_vert==0) { //horiz
            float posNote=note*line_width-note_display_offset+subnote;
            float posStart=(int)(data_bar2draw[i].startidx)*ww/data_midifx_len;
            float posEnd=((int)(data_bar2draw[i].startidx)+(int)(data_bar2draw[i].size))*ww/data_midifx_len;
            
            if ( ((posNote+(line_width)+line_width_extra)>=0) && ((posNote-line_width_extra)<(int)hh)) {
                
                if ((settings[GLOB_FXMIDIBarStyle].detail.mdz_switch.switch_value==0)||(settings[GLOB_FXMIDIBarStyle].detail.mdz_switch.switch_value==2)) {
                    
                    if (index+12>=max_indices) {
                        glDrawArrays(GL_TRIANGLES, 0, index);
                        index=0;
                    }
                    ptsB[index++] = LineVertexF(posStart-line_width_extra, posNote-line_width_extra,crtp[0],cgtp[0],cbtp[0],cap[0]);
                    ptsB[index++] = LineVertexF(posEnd+line_width_extra, posNote-line_width_extra,crtp[1],cgtp[1],cbtp[1],cap[1]);
                    ptsB[index++] = LineVertexF(posStart-line_width_extra, posNote+(line_width)/2,crt,cgt,cbt,ca);
                    
                    ptsB[index++] = LineVertexF(posEnd+line_width_extra, posNote-line_width_extra,crtp[1],cgtp[1],cbtp[1],cap[1]);
                    ptsB[index++] = LineVertexF(posStart-line_width_extra, posNote+(line_width)/2,crt,cgt,cbt,ca);
                    ptsB[index++] = LineVertexF(posEnd+line_width_extra, posNote+(line_width)/2,crt,cgt,cbt,ca);
                    
                    ptsB[index++] = LineVertexF(posStart-line_width_extra, posNote+(line_width)/2,crt,cgt,cbt,ca);
                    ptsB[index++] = LineVertexF(posEnd+line_width_extra, posNote+(line_width)/2,crt,cgt,cbt,ca);
                    ptsB[index++] = LineVertexF(posStart-line_width_extra, posNote+line_width+line_width_extra,crtp[2],cgtp[2],cbtp[2],cap[2]);
                    
                    ptsB[index++] = LineVertexF(posEnd+line_width_extra, posNote+(line_width)/2,crt,cgt,cbt,ca);
                    ptsB[index++] = LineVertexF(posStart-line_width_extra, posNote+line_width+line_width_extra,crtp[2],cgtp[2],cbtp[2],cap[2]);
                    ptsB[index++] = LineVertexF(posEnd+line_width_extra, posNote+line_width+line_width_extra,crtp[3],cgtp[3],cbtp[3],cap[3]);
                } else {
                    int border_size=(line_width>=8?2:1);
                    
                    if (index+30>=max_indices) {
                        glDrawArrays(GL_TRIANGLES, 0, index);
                        index=0;
                    }
                    //top
                    ptsB[index++] = LineVertexF(posStart-line_width_extra, posNote-line_width_extra,crtp[0],cgtp[0],cbtp[0],cap[0]);
                    ptsB[index++] = LineVertexF(posEnd+line_width_extra, posNote-line_width_extra,crtp[1],cgtp[1],cbtp[1],cap[1]);
                    ptsB[index++] = LineVertexF(posEnd+line_width_extra, posNote-line_width_extra+border_size,crtp[1],cgtp[1],cbtp[1],cap[1]);
                    
                    ptsB[index++] = LineVertexF(posStart-line_width_extra, posNote-line_width_extra,crtp[0],cgtp[0],cbtp[0],cap[0]);
                    ptsB[index++] = LineVertexF(posEnd+line_width_extra, posNote-line_width_extra+border_size,crtp[1],cgtp[1],cbtp[1],cap[1]);
                    ptsB[index++] = LineVertexF(posStart-line_width_extra, posNote-line_width_extra+border_size,crtp[0],cgtp[0],cbtp[0],cap[0]);
                    
                    //left
                    ptsB[index++] = LineVertexF(posStart-line_width_extra, posNote-line_width_extra,crtp[0],cgtp[0],cbtp[0],cap[0]);
                    ptsB[index++] = LineVertexF(posStart-line_width_extra+border_size, posNote-line_width_extra,crtp[1],cgtp[1],cbtp[1],cap[1]);
                    ptsB[index++] = LineVertexF(posStart-line_width_extra+border_size, posNote+(line_width)+line_width_extra,crtp[1],cgtp[1],cbtp[1],cap[1]);
                    
                    ptsB[index++] = LineVertexF(posStart-line_width_extra, posNote-line_width_extra,crtp[0],cgtp[0],cbtp[0],cap[0]);
                    ptsB[index++] = LineVertexF(posStart-line_width_extra+border_size, posNote+(line_width)+line_width_extra,crtp[1],cgtp[1],cbtp[1],cap[1]);
                    ptsB[index++] = LineVertexF(posStart-line_width_extra, posNote+(line_width)+line_width_extra,crtp[1],cgtp[1],cbtp[1],cap[1]);
                    
                    //bottom
                    ptsB[index++] = LineVertexF(posStart-line_width_extra, posNote+(line_width)+line_width_extra,crtp[2],cgtp[2],cbtp[2],cap[2]);
                    ptsB[index++] = LineVertexF(posEnd+line_width_extra, posNote+(line_width)+line_width_extra,crtp[3],cgtp[3],cbtp[3],cap[3]);
                    ptsB[index++] = LineVertexF(posEnd+line_width_extra, posNote+(line_width)+line_width_extra-border_size,crtp[3],cgtp[3],cbtp[3],cap[3]);
                    
                    ptsB[index++] = LineVertexF(posStart-line_width_extra, posNote+(line_width)+line_width_extra,crtp[2],cgtp[2],cbtp[2],cap[2]);
                    ptsB[index++] = LineVertexF(posEnd+line_width_extra, posNote+(line_width)+line_width_extra-border_size,crtp[3],cgtp[3],cbtp[3],cap[3]);
                    ptsB[index++] = LineVertexF(posStart-line_width_extra, posNote+(line_width)+line_width_extra-border_size,crtp[2],cgtp[2],cbtp[2],cap[2]);
                    
                    //right
                    ptsB[index++] = LineVertexF(posEnd+line_width_extra, posNote-line_width_extra,crtp[2],cgtp[2],cbtp[2],cap[2]);
                    ptsB[index++] = LineVertexF(posEnd+line_width_extra-border_size, posNote-line_width_extra,crtp[3],cgtp[3],cbtp[3],cap[3]);
                    ptsB[index++] = LineVertexF(posEnd+line_width_extra-border_size, posNote+(line_width)+line_width_extra,crtp[3],cgtp[3],cbtp[3],cap[3]);
                    
                    ptsB[index++] = LineVertexF(posEnd+line_width_extra, posNote-line_width_extra,crtp[2],cgtp[2],cbtp[2],cap[2]);
                    ptsB[index++] = LineVertexF(posEnd+line_width_extra-border_size, posNote+(line_width)+line_width_extra,crtp[3],cgtp[3],cbtp[3],cap[3]);
                    ptsB[index++] = LineVertexF(posEnd+line_width_extra, posNote+(line_width)+line_width_extra,crtp[3],cgtp[3],cbtp[3],cap[3]);
                    
                    //inner part
                    ptsB[index++] = LineVertexF(posStart-line_width_extra+border_size, posNote-line_width_extra+border_size,crt,cgt,cbt,ca);
                    ptsB[index++] = LineVertexF(posEnd+line_width_extra-border_size, posNote-line_width_extra+border_size,crt,cgt,cbt,ca);
                    ptsB[index++] = LineVertexF(posStart-line_width_extra+border_size, posNote+(line_width)+line_width_extra-border_size,crt,cgt,cbt,ca);
                    
                    ptsB[index++] = LineVertexF(posEnd+line_width_extra-border_size, posNote-line_width_extra+border_size,crt,cgt,cbt,ca);
                    ptsB[index++] = LineVertexF(posStart-line_width_extra+border_size, posNote+(line_width)+line_width_extra-border_size,crt,cgt,cbt,ca);
                    ptsB[index++] = LineVertexF(posEnd+line_width_extra-border_size, posNote+(line_width)+line_width_extra-border_size,crt,cgt,cbt,ca);
                    
                    
                }
            }
        } else {  //vert
            float posNote=note*line_width-note_display_offset+subnote;
            float posStart=(int)(data_bar2draw[i].startidx)*hh/data_midifx_len;
            float posEnd=((int)(data_bar2draw[i].startidx)+(int)(data_bar2draw[i].size))*hh/data_midifx_len;
            if ( ((posNote+(line_width)+line_width_extra)>=0) && ((posNote-line_width_extra)<(int)ww)) {
                
                if ((settings[GLOB_FXMIDIBarStyle].detail.mdz_switch.switch_value==0)||(settings[GLOB_FXMIDIBarStyle].detail.mdz_switch.switch_value==2)) {
                    
                    if (index+12>=max_indices) {
                        glDrawArrays(GL_TRIANGLES, 0, index);
                        index=0;
                    }
                    
                    ptsB[index++] = LineVertexF(posNote-line_width_extra, posStart-line_width_extra,crtp[0],cgtp[0],cbtp[0],cap[0]);
                    ptsB[index++] = LineVertexF(posNote-line_width_extra, posEnd+line_width_extra,crtp[1],cgtp[1],cbtp[1],cap[1]);
                    ptsB[index++] = LineVertexF(posNote+(line_width)/2, posStart-line_width_extra, crt,cgt,cbt,ca);
                    
                    ptsB[index++] = LineVertexF(posNote-line_width_extra, posEnd+line_width_extra,crtp[1],cgtp[1],cbtp[1],cap[1]);
                    ptsB[index++] = LineVertexF(posNote+(line_width)/2, posStart-line_width_extra, crt,cgt,cbt,ca);
                    ptsB[index++] = LineVertexF(posNote+(line_width)/2, posEnd+line_width_extra, crt,cgt,cbt,ca);
                    
                    ptsB[index++] = LineVertexF(posNote+(line_width)/2, posStart-line_width_extra, crt,cgt,cbt,ca);
                    ptsB[index++] = LineVertexF(posNote+(line_width)/2, posEnd+line_width_extra, crt,cgt,cbt,ca);
                    ptsB[index++] = LineVertexF(posNote+(line_width)+line_width_extra, posStart-line_width_extra,crtp[2],cgtp[2],cbtp[2],cap[2]);
                    
                    ptsB[index++] = LineVertexF(posNote+(line_width)/2, posEnd+line_width_extra, crt,cgt,cbt,ca);
                    ptsB[index++] = LineVertexF(posNote+(line_width)+line_width_extra, posStart-line_width_extra,crtp[2],cgtp[2],cbtp[2],cap[2]);
                    ptsB[index++] = LineVertexF(posNote+(line_width)+line_width_extra, posEnd+line_width_extra,crtp[3],cgtp[3],cbtp[3],cap[3]);
                } else {
                    int border_size=(line_width>=8?2:1);
                    
                    if (index+30>=max_indices) {
                        glDrawArrays(GL_TRIANGLES, 0, index);
                        index=0;
                    }
                    
                    //top
                    ptsB[index++] = LineVertexF(posNote-line_width_extra, posStart-line_width_extra, crtp[0],cgtp[0],cbtp[0],cap[0]);
                    ptsB[index++] = LineVertexF(posNote-line_width_extra, posStart-line_width_extra+border_size, crtp[1],cgtp[1],cbtp[1],cap[1]);
                    ptsB[index++] = LineVertexF(posNote+(line_width)+line_width_extra, posStart-line_width_extra+border_size, crtp[1],cgtp[1],cbtp[1],cap[1]);
                    
                    ptsB[index++] = LineVertexF(posNote-line_width_extra, posStart-line_width_extra, crtp[0],cgtp[0],cbtp[0],cap[0]);
                    ptsB[index++] = LineVertexF(posNote+(line_width)+line_width_extra, posStart-line_width_extra+border_size, crtp[1],cgtp[1],cbtp[1],cap[1]);
                    ptsB[index++] = LineVertexF(posNote+(line_width)+line_width_extra, posStart-line_width_extra, crtp[1],cgtp[1],cbtp[1],cap[1]);
                    
                    //left
                    ptsB[index++] = LineVertexF(posNote-line_width_extra, posStart-line_width_extra, crtp[0],cgtp[0],cbtp[0],cap[0]);
                    ptsB[index++] = LineVertexF(posNote-line_width_extra, posEnd+line_width_extra, crtp[1],cgtp[1],cbtp[1],cap[1]);
                    ptsB[index++] = LineVertexF(posNote-line_width_extra+border_size, posEnd+line_width_extra, crtp[1],cgtp[1],cbtp[1],cap[1]);
                    
                    ptsB[index++] = LineVertexF(posNote-line_width_extra, posStart-line_width_extra, crtp[0],cgtp[0],cbtp[0],cap[0]);
                    ptsB[index++] = LineVertexF(posNote-line_width_extra+border_size, posEnd+line_width_extra, crtp[1],cgtp[1],cbtp[1],cap[1]);
                    ptsB[index++] = LineVertexF(posNote-line_width_extra+border_size, posStart-line_width_extra, crtp[0],cgtp[0],cbtp[0],cap[0]);
                    
                    //right
                    ptsB[index++] = LineVertexF(posNote+(line_width)+line_width_extra,posStart-line_width_extra, crtp[2],cgtp[2],cbtp[2],cap[2]);
                    ptsB[index++] = LineVertexF(posNote+(line_width)+line_width_extra,posEnd+line_width_extra, crtp[3],cgtp[3],cbtp[3],cap[3]);
                    ptsB[index++] = LineVertexF(posNote+(line_width)+line_width_extra-border_size,posEnd+line_width_extra, crtp[3],cgtp[3],cbtp[3],cap[3]);
                    
                    ptsB[index++] = LineVertexF(posNote+(line_width)+line_width_extra,posStart-line_width_extra, crtp[2],cgtp[2],cbtp[2],cap[2]);
                    ptsB[index++] = LineVertexF(posNote+(line_width)+line_width_extra-border_size,posEnd+line_width_extra, crtp[3],cgtp[3],cbtp[3],cap[3]);
                    ptsB[index++] = LineVertexF(posNote+(line_width)+line_width_extra-border_size,posStart-line_width_extra, crtp[2],cgtp[2],cbtp[2],cap[2]);
                    
                    //bottom
                    ptsB[index++] = LineVertexF(posNote-line_width_extra, posEnd+line_width_extra, crtp[2],cgtp[2],cbtp[2],cap[2]);
                    ptsB[index++] = LineVertexF(posNote-line_width_extra, posEnd+line_width_extra-border_size, crtp[3],cgtp[3],cbtp[3],cap[3]);
                    ptsB[index++] = LineVertexF(posNote+(line_width)+line_width_extra,posEnd+line_width_extra-border_size, crtp[3],cgtp[3],cbtp[3],cap[3]);
                    
                    ptsB[index++] = LineVertexF(posNote-line_width_extra, posEnd+line_width_extra, crtp[2],cgtp[2],cbtp[2],cap[2]);
                    ptsB[index++] = LineVertexF(posNote+(line_width)+line_width_extra, posEnd+line_width_extra-border_size, crtp[3],cgtp[3],cbtp[3],cap[3]);
                    ptsB[index++] = LineVertexF(posNote+(line_width)+line_width_extra, posEnd+line_width_extra, crtp[3],cgtp[3],cbtp[3],cap[3]);
                    
                    //inner part
                    ptsB[index++] = LineVertexF(posNote-line_width_extra+border_size, posStart-line_width_extra+border_size, crt,cgt,cbt,ca);
                    ptsB[index++] = LineVertexF(posNote-line_width_extra+border_size, posEnd+line_width_extra-border_size, crt,cgt,cbt,ca);
                    ptsB[index++] = LineVertexF(posNote+(line_width)+line_width_extra-border_size, posStart-line_width_extra+border_size, crt,cgt,cbt,ca);
                    
                    ptsB[index++] = LineVertexF(posNote-line_width_extra+border_size, posEnd+line_width_extra-border_size, crt,cgt,cbt,ca);
                    ptsB[index++] = LineVertexF(posNote+(line_width)+line_width_extra-border_size, posStart-line_width_extra+border_size, crt,cgt,cbt,ca);
                    ptsB[index++] = LineVertexF(posNote+(line_width)+line_width_extra-border_size, posEnd+line_width_extra-border_size, crt,cgt,cbt,ca);
                }
            }
        }
    }
    
    //    printf("total: %d\n",index);
    
    glDrawArrays(GL_TRIANGLES, 0, index);
    
    //////////////////////////////////////////////
    
    //    glEnable(GL_BLEND);
    //    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    
    //current playing line
    //    230,76,153
    index=0;
    if (horiz_vert==0) {
        index=DrawBox(ptsB, index,
                      (data_midifx_len-MIDIFX_OFS-1)*band_width-band_width*2,
                      0,
                      band_width*2,hh,1,
                      235,210,255,200,0);
        
    } else {
        index=DrawBox(ptsB, index,
                      0,
                      (data_midifx_len-MIDIFX_OFS-1)*band_width-band_width*2,
                      ww,
                      band_width*2,1,
                      235,210,255,200,0);
    }
    glDrawArrays(GL_TRIANGLES, 0, index);
    //    glLineWidth(band_width*mScaleFactor);
    //    glDrawArrays(GL_LINES, 0, 2);
    
    
#if 0
    //Draw spark fx
    glDisable(GL_DEPTH_TEST);           /* Disable Depth Testing     */
    glEnable(GL_TEXTURE_2D);            /* Enable 2D Texture Mapping */
    glBindTexture(GL_TEXTURE_2D, txt_pianoRoll[TXT_PIANOROLL_SPARK]);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE);
    
    
    glTexCoordPointer(2, GL_FLOAT, 0, texcoords);
    /* Enable Texture Coordinations Pointer */
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    
    memset(sparkPresent,0,sizeof(sparkPresent));
    index=0;
    int midi_data_ofs=data_midifx_len-MIDIFX_OFS-1;
    if (settings[GLOB_FXPianoRollSpark].detail.mdz_switch.switch_value)
        for (int i=0; i<256; i++) { //for each channels
            if ((data_midifx_note[MIDIFX_LEN-MIDIFX_OFS-1][i])&&
                data_midifx_vol[MIDIFX_LEN-MIDIFX_OFS-1][i] &&
                ( data_midifx_vol[MIDIFX_LEN-MIDIFX_OFS-1][i]>=data_midifx_vol[MIDIFX_LEN-MIDIFX_OFS-1+1][i]) ) {  //do we have a note ?
                unsigned int note=data_midifx_note[MIDIFX_LEN-MIDIFX_OFS-1][i];
                if (!note) note=data_midifx_note[MIDIFX_LEN-MIDIFX_OFS-1+1][i];
                
                //avoid rendering twice for same note
                if (sparkPresent[note]) continue;
                sparkPresent[note]=1;
                if (sparkIntensity[note]<128) sparkIntensity[note]+=8;
                
                unsigned int instr=data_midifx_instr[MIDIFX_LEN-MIDIFX_OFS-1][i];
                int colidx=instr&63;
                int crt=((data_midifx_col[colidx&31]>>16)&0xFF);
                int cgt=((data_midifx_col[colidx&31]>>8)&0xFF);
                int cbt=(data_midifx_col[colidx&31]&0xFF);
                
                if (colidx&0x20) {
                    crt=(crt+255)/2;
                    cgt=(cgt+255)/2;
                    cbt=(cbt+255)/2;
                }
                
                crt=(crt*3+255*3)/6;
                cgt=(cgt*3+255*3)/6;
                cbt=(cbt*3+255*3)/6;
                
                if (crt>255) crt=255;
                if (cgt>255) cgt=255;
                if (cbt>255) cbt=255;
                
                
                line_width_extra=2;
                
                float posNote,posStart;
                float wd;
                float width=line_width;
                
                if (horiz_vert==0) { //horiz
                    posNote=note*line_width-note_display_offset;
                    posStart=(data_midifx_len-MIDIFX_OFS-1)*band_width;//+band_width;
                    
                    if ( ((posNote+(line_width)+line_width_extra)>=0) && ((posNote-line_width_extra)<(int)hh)) {
                        
                        wd=width;
                        posNote-=wd/2;
                        wd=wd*2;
                        
                        for (int sp=0;sp<4;sp++) {
                            
                            texcoords[index+0].u=0.0f; texcoords[index+0].v=80.0/128;
                            texcoords[index+1].u=0.0f; texcoords[index+1].v=18.0/128;
                            texcoords[index+2].u=1.0f; texcoords[index+2].v=80.0/128;
                            
                            texcoords[index+3].u=0.0f; texcoords[index+3].v=18.0/128;
                            texcoords[index+4].u=1.0f; texcoords[index+4].v=80.0/128;
                            texcoords[index+5].u=1.0f; texcoords[index+5].v=18.0/128;
                            
                            
                            ptsB[index+0].x=posStart;ptsB[index+0].y=posNote+wd;
                            ptsB[index+1].x=posStart+wd/2;ptsB[index+1].y=posNote+wd;
                            ptsB[index+2].x=posStart;ptsB[index+2].y=posNote;
                            
                            ptsB[index+3].x=posStart+wd/2;ptsB[index+3].y=posNote+wd;
                            ptsB[index+4].x=posStart;ptsB[index+4].y=posNote;
                            ptsB[index+5].x=posStart+wd/2;ptsB[index+5].y=posNote;
                            
                            //apply some distortion
                            float wd_distX=wd/9.0;
                            float wd_distY=wd/3.0;
                            
                            float distorFactors[4][4][6]={
                                {   {+0.7,  3 ,+0.2,  5, -0.3, 11},
                                    {+0.2,  3 ,+0.5,  5, -0.4, 5},
                                    
                                    {+0.5,  5 ,-0.1,  7, +0.4, 13},
                                    {+0.3,  2 ,-0.5,  3, +0.2, 3}},
                                
                                {   {+0.5,  1 ,+0.2,  7, -0.3, 7},
                                    {+0.3,  3 ,+0.5,  2, -0.4, 7},
                                    
                                    {-0.3,  7 ,-0.1,  9, +0.4, 5},
                                    {-0.2,  5 ,-0.5,  5, +0.2, 11}},
                                
                                {   {-0.7,  2 ,+0.2,  11, -0.3, 9},
                                    {+0.2,  5 ,+0.5,  13, +0.7, 5},
                                    
                                    {+0.6,  9 ,-0.1,  3, +0.4, 3},
                                    {+0.4,  1 ,-0.5,  3, +0.2, 8}},
                                
                                {   {+0.8,  9 ,+0.2,  7, -0.3, 9},
                                    {+0.4,  11 ,+0.5, 7, +0.4, 5},
                                    
                                    {-0.5,  3 ,+0.1,  4, -0.4, 11},
                                    {-0.3,  5 ,+0.5,  5, +0.2, 3}}};
                            
                            ptsB[index+1].x+=wd_distX*(distorFactors[sp][0][0]*sin(pianoroll_cpt*distorFactors[sp][0][1]*3.14159/32)
                                                       +distorFactors[sp][0][2]*sin(pianoroll_cpt*distorFactors[sp][0][3]*3.14159/32)
                                                       +distorFactors[sp][0][4]*sin(pianoroll_cpt*distorFactors[sp][0][5]*3.14159/32));
                            
                            ptsB[index+3].x+=wd_distX*(distorFactors[sp][0][0]*sin(pianoroll_cpt*distorFactors[sp][0][1]*3.14159/32)
                                                       +distorFactors[sp][0][2]*sin(pianoroll_cpt*distorFactors[sp][0][3]*3.14159/32)
                                                       +distorFactors[sp][0][4]*sin(pianoroll_cpt*distorFactors[sp][0][5]*3.14159/32));
                            
                            ptsB[index+1].y+=wd_distY*(distorFactors[sp][1][0]*sin(pianoroll_cpt*distorFactors[sp][1][1]*3.14159/32)
                                                       +distorFactors[sp][1][2]*sin(pianoroll_cpt*distorFactors[sp][1][3]*3.14159/32)
                                                       +distorFactors[sp][1][4]*sin(pianoroll_cpt*distorFactors[sp][1][5]*3.14159/32));
                            
                            ptsB[index+3].y+=wd_distY*(distorFactors[sp][1][0]*sin(pianoroll_cpt*distorFactors[sp][1][1]*3.14159/32)
                                                       +distorFactors[sp][1][2]*sin(pianoroll_cpt*distorFactors[sp][1][3]*3.14159/32)
                                                       +distorFactors[sp][1][4]*sin(pianoroll_cpt*distorFactors[sp][1][5]*3.14159/32));
                            
                            ptsB[index+5].x+=wd_distX*(distorFactors[sp][2][0]*sin(pianoroll_cpt*distorFactors[sp][2][1]*3.14159/32)
                                                       +distorFactors[sp][2][2]*sin(pianoroll_cpt*distorFactors[sp][2][3]*3.14159/32)
                                                       +distorFactors[sp][2][4]*sin(pianoroll_cpt*distorFactors[sp][2][5]*3.14159/32));
                            
                            ptsB[index+5].y+=wd_distY*(distorFactors[sp][3][0]*sin(pianoroll_cpt*distorFactors[sp][3][1]*3.14159/32)
                                                       +distorFactors[sp][3][2]*sin(pianoroll_cpt*distorFactors[sp][3][3]*3.14159/32)
                                                       +distorFactors[sp][3][4]*sin(pianoroll_cpt*distorFactors[sp][3][5]*3.14159/32));
                            
                            if (sp&1) {
                                for (int ii=0;ii<6;ii++) {
                                    texcoords[index+ii].u=1-texcoords[index+ii].u;
                                }
                                
                            }
                            for (int ii=0;ii<6;ii++) {
                                if (settings[GLOB_FXPianoRollSpark].detail.mdz_switch.switch_value==2) {
                                    ptsB[index+ii].r=255;
                                    ptsB[index+ii].g=255;
                                    ptsB[index+ii].b=255;
                                } else {
                                    ptsB[index+ii].r=crt;
                                    ptsB[index+ii].g=cgt;
                                    ptsB[index+ii].b=cbt;
                                }
                                ptsB[index+ii].a=sparkIntensity[note]/4;///4;
                            }
                            index+=6;
                            
                        }
                    }
                }else { //vert
                    posNote=note*line_width-note_display_offset;
                    posStart=(int)midi_data_ofs*hh/data_midifx_len;
                }
            }
        }
    glDrawArrays(GL_TRIANGLES, 0, index);
    
    glBindTexture(GL_TEXTURE_2D, txt_pianoRoll[TXT_PIANOROLL_LIGHT]);
    memset(sparkPresent,0,sizeof(sparkPresent));
    index=0;
    if (settings[GLOB_FXPianoRollSpark].detail.mdz_switch.switch_value)
        for (int i=0; i<256; i++) { //for each channels
            if ((data_midifx_note[MIDIFX_LEN-MIDIFX_OFS-1][i])&&
                data_midifx_vol[MIDIFX_LEN-MIDIFX_OFS-1][i] &&
                ( data_midifx_vol[MIDIFX_LEN-MIDIFX_OFS-1][i]>=data_midifx_vol[MIDIFX_LEN-MIDIFX_OFS-1+1][i]) ) {  //do we have a note ?
                unsigned int note=data_midifx_note[MIDIFX_LEN-MIDIFX_OFS-1][i];
                if (!note) note=data_midifx_note[MIDIFX_LEN-MIDIFX_OFS-1+1][i];
                
                //avoid rendering twice for same note
                if (sparkPresent[note]) continue;
                sparkPresent[note]=1;
                if (sparkIntensity[note]<128) sparkIntensity[note]+=8;
                
                unsigned int instr=data_midifx_instr[MIDIFX_LEN-MIDIFX_OFS-1][i];
                int colidx=instr&63;
                int crt=((data_midifx_col[colidx&31]>>16)&0xFF);
                int cgt=((data_midifx_col[colidx&31]>>8)&0xFF);
                int cbt=(data_midifx_col[colidx&31]&0xFF);
                
                if (colidx&0x20) {
                    crt=(crt+255)/2;
                    cgt=(cgt+255)/2;
                    cbt=(cbt+255)/2;
                }
                
                crt=(crt*3+255*3)/6;
                cgt=(cgt*3+255*3)/6;
                cbt=(cbt*3+255*3)/6;
                
                if (crt>255) crt=255;
                if (cgt>255) cgt=255;
                if (cbt>255) cbt=255;
                
                
                line_width_extra=2;
                
                float posNote,posStart;
                float wd;
                float width=line_width;
                
                if (horiz_vert==0) { //horiz
                    posNote=note*line_width-note_display_offset;
                    posStart=(data_midifx_len-MIDIFX_OFS-1)*band_width;//+band_width;
                    
                    if ( ((posNote+(line_width)+line_width_extra)>=0) && ((posNote-line_width_extra)<(int)hh)) {
                        
                        wd=width*2;
                        posStart-=wd;
                        posNote-=wd/2+width/2;
                        wd=wd*2;
                        
                        for (int sp=0;sp<4;sp++) {
                            
                            texcoords[index+0].u=0.0f; texcoords[index+0].v=128.0/128;
                            texcoords[index+1].u=0.0f; texcoords[index+1].v=0.0/128;
                            texcoords[index+2].u=1.0f; texcoords[index+2].v=128.0/128;
                            
                            texcoords[index+3].u=0.0f; texcoords[index+3].v=0.0/128;
                            texcoords[index+4].u=1.0f; texcoords[index+4].v=128.0/128;
                            texcoords[index+5].u=1.0f; texcoords[index+5].v=0.0/128;
                            
                            
                            ptsB[index+0].x=posStart;ptsB[index+0].y=posNote+wd;
                            ptsB[index+1].x=posStart+wd*1.2;ptsB[index+1].y=posNote+wd;
                            ptsB[index+2].x=posStart;ptsB[index+2].y=posNote;
                            
                            ptsB[index+3].x=posStart+wd*1.2;ptsB[index+3].y=posNote+wd;
                            ptsB[index+4].x=posStart;ptsB[index+4].y=posNote;
                            ptsB[index+5].x=posStart+wd*1.2;ptsB[index+5].y=posNote;
                            
                            //apply some distortion
                            float wd_distX=wd/18.0;
                            float wd_distY=wd/6.0;
                            
                            float distorFactors[4][4][6]={
                                {   {+0.7,  3 ,+0.2,  5, -0.3, 11},
                                    {+0.2,  3 ,+0.5,  5, -0.4, 5},
                                    
                                    {+0.5,  5 ,-0.1,  7, +0.4, 13},
                                    {+0.3,  2 ,-0.5,  3, +0.2, 3}},
                                
                                {   {+0.5,  1 ,+0.2,  7, -0.3, 7},
                                    {+0.3,  3 ,+0.5,  2, -0.4, 7},
                                    
                                    {-0.3,  7 ,-0.1,  9, +0.4, 5},
                                    {-0.2,  5 ,-0.5,  5, +0.2, 11}},
                                
                                {   {-0.7,  2 ,+0.2,  11, -0.3, 9},
                                    {+0.2,  5 ,+0.5,  13, +0.7, 5},
                                    
                                    {+0.6,  9 ,-0.1,  3, +0.4, 3},
                                    {+0.4,  1 ,-0.5,  3, +0.2, 8}},
                                
                                {   {+0.8,  9 ,+0.2,  7, -0.3, 9},
                                    {+0.4,  11 ,+0.5, 7, +0.4, 5},
                                    
                                    {-0.5,  3 ,+0.1,  4, -0.4, 11},
                                    {-0.3,  5 ,+0.5,  5, +0.2, 3}}};
                            
                            ptsB[index+1].x+=wd_distX*(distorFactors[sp][0][0]*sin(pianoroll_cpt*distorFactors[sp][0][1]*3.14159/32)
                                                       +distorFactors[sp][0][2]*sin(pianoroll_cpt*distorFactors[sp][0][3]*3.14159/32)
                                                       +distorFactors[sp][0][4]*sin(pianoroll_cpt*distorFactors[sp][0][5]*3.14159/32));
                            
                            ptsB[index+3].x+=wd_distX*(distorFactors[sp][0][0]*sin(pianoroll_cpt*distorFactors[sp][0][1]*3.14159/32)
                                                       +distorFactors[sp][0][2]*sin(pianoroll_cpt*distorFactors[sp][0][3]*3.14159/32)
                                                       +distorFactors[sp][0][4]*sin(pianoroll_cpt*distorFactors[sp][0][5]*3.14159/32));
                            
                            ptsB[index+1].y+=wd_distY*(distorFactors[sp][1][0]*sin(pianoroll_cpt*distorFactors[sp][1][1]*3.14159/32)
                                                       +distorFactors[sp][1][2]*sin(pianoroll_cpt*distorFactors[sp][1][3]*3.14159/32)
                                                       +distorFactors[sp][1][4]*sin(pianoroll_cpt*distorFactors[sp][1][5]*3.14159/32));
                            
                            ptsB[index+3].y+=wd_distY*(distorFactors[sp][1][0]*sin(pianoroll_cpt*distorFactors[sp][1][1]*3.14159/32)
                                                       +distorFactors[sp][1][2]*sin(pianoroll_cpt*distorFactors[sp][1][3]*3.14159/32)
                                                       +distorFactors[sp][1][4]*sin(pianoroll_cpt*distorFactors[sp][1][5]*3.14159/32));
                            
                            ptsB[index+5].x+=wd_distX*(distorFactors[sp][2][0]*sin(pianoroll_cpt*distorFactors[sp][2][1]*3.14159/32)
                                                       +distorFactors[sp][2][2]*sin(pianoroll_cpt*distorFactors[sp][2][3]*3.14159/32)
                                                       +distorFactors[sp][2][4]*sin(pianoroll_cpt*distorFactors[sp][2][5]*3.14159/32));
                            
                            ptsB[index+5].y+=wd_distY*(distorFactors[sp][3][0]*sin(pianoroll_cpt*distorFactors[sp][3][1]*3.14159/32)
                                                       +distorFactors[sp][3][2]*sin(pianoroll_cpt*distorFactors[sp][3][3]*3.14159/32)
                                                       +distorFactors[sp][3][4]*sin(pianoroll_cpt*distorFactors[sp][3][5]*3.14159/32));
                            
                            if (sp&1) {
                                for (int ii=0;ii<6;ii++) {
                                    texcoords[index+ii].u=1-texcoords[index+ii].u;
                                }
                                
                            }
                            for (int ii=0;ii<6;ii++) {
                                if (settings[GLOB_FXPianoRollSpark].detail.mdz_switch.switch_value==2) {
                                    ptsB[index+ii].r=255;
                                    ptsB[index+ii].g=255;
                                    ptsB[index+ii].b=255;
                                } else {
                                    ptsB[index+ii].r=crt;
                                    ptsB[index+ii].g=cgt;
                                    ptsB[index+ii].b=cbt;
                                }
                                ptsB[index+ii].a=sparkIntensity[note]/4;
                            }
                            index+=6;
                            
                        }
                    }
                }else { //vert
                    posNote=note*line_width-note_display_offset;
                    posStart=(int)midi_data_ofs*hh/data_midifx_len;
                }
            }
        }
    glDrawArrays(GL_TRIANGLES, 0, index);
    
    //reset spark intensity if no note played
    for (int i=0;i<256;i++) {
        if (sparkPresent[i]==0) {
            if (sparkIntensity[i]>8) sparkIntensity[i]-=8;
            else sparkIntensity[i]=0;
        }
    }
    
    
    glBindTexture(GL_TEXTURE_2D,0);
    glDisable(GL_TEXTURE_2D);            /* Enable 2D Texture Mapping */
    glEnableClientState(GL_COLOR_ARRAY);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    index=0;
#endif
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisable(GL_BLEND);
    
    free(ptsB);
    free(texcoords);
#endif
}


int RenderUtils::DrawBox(LineVertexF *ptsB,int index,float x,float y,float width,float height,float border_size,int crt,int cgt,int cbt,int ca,int subnote) {
    int crtp[6],cgtp[6],cbtp[6],cap[6];
    
    for (int ii=0;ii<4;ii++) {
        double fact=1;
        double ofs=0;
        if ((ii==3)||(ii==2)) {
            fact=1.5f;
            ofs=100;
        }
        if ((ii==0)||(ii==1)) {
            fact=0.5f;
            ofs=0;
        }
        crtp[ii]=crt*fact+ofs;
        cgtp[ii]=cgt*fact+ofs;
        cbtp[ii]=cbt*fact+ofs;
        cap[ii]=ca;
        if (crtp[ii]>255) crtp[ii]=255;
        if (cgtp[ii]>255) cgtp[ii]=255;
        if (cbtp[ii]>255) cbtp[ii]=255;
        if (cap[ii]>255) cap[ii]=255;
    }
    //top
    ptsB[index++] = LineVertexF(x, y,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+width, y,crtp[1],cgtp[1],cbtp[1],cap[1]);
    ptsB[index++] = LineVertexF(x+width, y+border_size,crtp[1],cgtp[1],cbtp[1],cap[1]);
    
    ptsB[index++] = LineVertexF(x, y,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+width, y+border_size,crtp[1],cgtp[1],cbtp[1],cap[1]);
    ptsB[index++] = LineVertexF(x, y+border_size,crtp[0],cgtp[0],cbtp[0],cap[0]);
    
    //left
    ptsB[index++] = LineVertexF(x, y,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+border_size, y,crtp[1],cgtp[1],cbtp[1],cap[1]);
    ptsB[index++] = LineVertexF(x+border_size, y+height,crtp[1],cgtp[1],cbtp[1],cap[1]);
    
    ptsB[index++] = LineVertexF(x, y,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+border_size, y+height,crtp[1],cgtp[1],cbtp[1],cap[1]);
    ptsB[index++] = LineVertexF(x, y+height,crtp[1],cgtp[1],cbtp[1],cap[1]);
    
    //bottom
    ptsB[index++] = LineVertexF(x, y+height,crtp[2],cgtp[2],cbtp[2],cap[2]);
    ptsB[index++] = LineVertexF(x+width, y+height,crtp[3],cgtp[3],cbtp[3],cap[3]);
    ptsB[index++] = LineVertexF(x+width, y+height-border_size,crtp[3],cgtp[3],cbtp[3],cap[3]);
    
    ptsB[index++] = LineVertexF(x, y+height,crtp[2],cgtp[2],cbtp[2],cap[2]);
    ptsB[index++] = LineVertexF(x+width, y+height-border_size,crtp[3],cgtp[3],cbtp[3],cap[3]);
    ptsB[index++] = LineVertexF(x, y+height-border_size,crtp[2],cgtp[2],cbtp[2],cap[2]);
    
    //right
    ptsB[index++] = LineVertexF(x+width, y,crtp[2],cgtp[2],cbtp[2],cap[2]);
    ptsB[index++] = LineVertexF(x+width-border_size, y,crtp[3],cgtp[3],cbtp[3],cap[3]);
    ptsB[index++] = LineVertexF(x+width-border_size, y+height,crtp[3],cgtp[3],cbtp[3],cap[3]);
    
    ptsB[index++] = LineVertexF(x+width, y,crtp[2],cgtp[2],cbtp[2],cap[2]);
    ptsB[index++] = LineVertexF(x+width-border_size, y+height,crtp[3],cgtp[3],cbtp[3],cap[3]);
    ptsB[index++] = LineVertexF(x+width, y+height,crtp[3],cgtp[3],cbtp[3],cap[3]);
    
    //inner part
    if (subnote) {
        if (subnote>0) {
            float fact=1+(float)subnote*0.1/8.0;
            float ofs=0+subnote*8;
            crtp[4]=crt/fact-ofs;cgtp[4]=cgt/fact-ofs;cbtp[4]=cbt/fact-ofs;cap[4]=ca;
            crtp[5]=crt*fact+ofs;cgtp[5]=cgt*fact+ofs;cbtp[5]=cbt*fact+ofs;cap[5]=ca;
        } else {
            float fact=1;
            float ofs=0;
            fact=1-(float)subnote*0.1/8.0;
            ofs=0-subnote*8;
            crtp[4]=crt*fact+ofs;cgtp[4]=cgt*fact+ofs;cbtp[4]=cbt*fact+ofs;cap[4]=ca;
            crtp[5]=crt/fact-ofs;cgtp[5]=cgt/fact-ofs;cbtp[5]=cbt/fact-ofs;cap[5]=ca;
        }
        for (int ii=4;ii<6;ii++) {
            if (crtp[ii]>255) crtp[ii]=255;
            if (cgtp[ii]>255) cgtp[ii]=255;
            if (cbtp[ii]>255) cbtp[ii]=255;
            if (cap[ii]>255) cap[ii]=255;
            if (crtp[ii]<0) crtp[ii]=0;
            if (cgtp[ii]<0) cgtp[ii]=0;
            if (cbtp[ii]<0) cbtp[ii]=0;
            if (cap[ii]<0) cap[ii]=0;
        }
    } else {
        crtp[4]=crt;cgtp[4]=cgt;cbtp[4]=cbt;cap[4]=ca;
        crtp[5]=crt;cgtp[5]=cgt;cbtp[5]=cbt;cap[5]=ca;
    }
    ptsB[index++] = LineVertexF(x+border_size, y+border_size,crtp[4],cgtp[4],cbtp[4],cap[4]);
    ptsB[index++] = LineVertexF(x+width-border_size, y+border_size,crtp[5],cgtp[5],cbtp[5],cap[5]);
    ptsB[index++] = LineVertexF(x+border_size, y+height-border_size,crtp[4],cgtp[4],cbtp[4],cap[4]);
    
    ptsB[index++] = LineVertexF(x+width-border_size, y+border_size,crtp[5],cgtp[5],cbtp[5],cap[5]);
    ptsB[index++] = LineVertexF(x+border_size, y+height-border_size,crtp[4],cgtp[4],cbtp[4],cap[4]);
    ptsB[index++] = LineVertexF(x+width-border_size, y+height-border_size,crtp[5],cgtp[5],cbtp[5],cap[5]);
    
    return index;
}

int lastkey_type;

#define PR_SHADOW_WHITE (1<<0)
#define PR_SHADOW_SMALL_BLACK (1<<1)
#define PR_SHADOW_LARGE_BLACK (1<<2)

int RenderUtils::DrawKeyW(LineVertexF *ptsB,int index,float x,float y,float width,float height,float border_size,int crt,int cgt,int cbt,int ca,int subnote,int
                          note_idx,int channel) {
#if 0
    int crtp[6],cgtp[6],cbtp[6],cap[6];
    float height2;
    uint8_t shadow_type=0;
    float shadow_left_black_ofs=0;
    
    bool pressed=pianoroll_key_status[channel][note_idx]&PR_KEY_PRESSED;
    
    if (note_idx) { //not 1st key
        if (pianoroll_key_status[channel][note_idx-1]&PR_WHITE_KEY) {
            //no black key on the left
            if (!(pianoroll_key_status[channel][note_idx-1]&PR_KEY_PRESSED)) {
                //left white key not pressed
                //shadow if current key is pressed
                if (pressed) shadow_type=PR_SHADOW_WHITE;
            }
        } else {
            //black key on the left
            if (!(pianoroll_key_status[channel][note_idx-1]&PR_KEY_PRESSED)) {
                //left black key not pressed, shadow on
                //small shadow if key not pressed or else large shadow
                if (pressed) shadow_type=PR_SHADOW_LARGE_BLACK;
                else shadow_type=PR_SHADOW_SMALL_BLACK;
                
                //check black key type
                uint8_t note_type=(pianoroll_key_status[channel][note_idx-1]>>4)%12;
                switch (note_type) {
                    case 1://C#
                    case 6://F#
                        shadow_left_black_ofs=(width*2.0f/12);
                        break;
                    case 3://D#
                    case 10://A#
                        shadow_left_black_ofs=(width*4.0f/12);
                        break;
                    case 8://G#
                        shadow_left_black_ofs=(width*3.0f/12);
                        break;
                    default:
                        break;
                }
            }
            //check white key on the left
            if (note_idx>=2) {
                if (!(pianoroll_key_status[channel][note_idx-2]&PR_KEY_PRESSED)) {
                    //left white key not pressed
                    //shadow if current key is pressed
                    if (pressed) shadow_type|=PR_SHADOW_WHITE;
                }
            }
        }
    }
    
    
    if (!pressed) {
        //y+=height/16;
        //height=height*15/16;
        height2=height/8;
    } else {
        height2=height/24;
        crt=crt*0.8f;
        cgt=cgt*0.8f;
        cbt=cbt*0.8f;
    }
    
    for (int ii=0;ii<4;ii++) {
        double fact=1;
        double ofs=0;
        switch (ii) {
            case 3:fact=1.5f; ofs=100; break;
            case 2:fact=0.75f; ofs=0; break;
            case 1:fact=0.5f; ofs=0; break;
            case 0:fact=0.25f; ofs=0; break;
        }
        crtp[ii]=crt*fact+ofs;cgtp[ii]=cgt*fact+ofs;cbtp[ii]=cbt*fact+ofs;cap[ii]=ca;
        if (crtp[ii]>255) crtp[ii]=255;if (cgtp[ii]>255) cgtp[ii]=255;if (cbtp[ii]>255) cbtp[ii]=255;
        if (cap[ii]>255) cap[ii]=255;
    }
    if (subnote) {
        if (subnote>0) {
            float fact=1.1+(float)subnote*0.1/8.0;
            float ofs=16+subnote*2;
            crtp[4]=crt*fact+ofs;cgtp[4]=cgt*fact+ofs;cbtp[4]=cbt*fact+ofs;cap[4]=ca;
            crtp[5]=crt*fact+ofs;cgtp[5]=cgt*fact+ofs;cbtp[5]=cbt*fact+ofs;cap[5]=ca;
        } else {
            float fact=1;
            float ofs=0;
            fact=1.1-(float)subnote*0.1/8.0; ofs=16-subnote*2;
            crtp[4]=crt*fact+ofs;cgtp[4]=cgt*fact+ofs;cbtp[4]=cbt*fact+ofs;cap[4]=ca;
            crtp[5]=crt*fact+ofs;cgtp[5]=cgt*fact+ofs;cbtp[5]=cbt*fact+ofs;cap[5]=ca;
        }
        for (int ii=4;ii<6;ii++) {
            if (crtp[ii]>255) crtp[ii]=255;
            if (cgtp[ii]>255) cgtp[ii]=255;
            if (cbtp[ii]>255) cbtp[ii]=255;
            if (cap[ii]>255) cap[ii]=255;
        }
    } else {
        crtp[4]=crt;cgtp[4]=cgt;cbtp[4]=cbt;cap[4]=ca;
        crtp[5]=crt;cgtp[5]=cgt;cbtp[5]=cbt;cap[5]=ca;
    }
    
    //left high
    ptsB[index++] = LineVertexF(x               , y+height2         ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+1             , y+height2         ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+1             , y+height  ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    
    ptsB[index++] = LineVertexF(x               , y+height2         ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+1             , y+height  ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x               , y+height  ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    
    //left low
    ptsB[index++] = LineVertexF(x               , y         ,0,0,0,cap[0]);
    ptsB[index++] = LineVertexF(x+width/12      , y         ,0,0,0,cap[0]);
    ptsB[index++] = LineVertexF(x+width/12      , y+height2 ,0,0,0,cap[0]);
    
    ptsB[index++] = LineVertexF(x               , y          ,0,0,0,cap[0]);
    ptsB[index++] = LineVertexF(x+width/12      , y+height2  ,0,0,0,cap[0]);
    ptsB[index++] = LineVertexF(x               , y+height2  ,0,0,0,cap[0]);
    
    //right high
    ptsB[index++] = LineVertexF(x+width             , y+height2     ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+width-1           , y+height2     ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+width-1           , y+height      ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    
    ptsB[index++] = LineVertexF(x+width             , y+height2 ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+width-1           , y+height  ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+width             , y+height  ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    
    //right low
    ptsB[index++] = LineVertexF(x+width             , y         ,0,0,0,cap[0]);
    ptsB[index++] = LineVertexF(x+width-width/12    , y         ,0,0,0,cap[0]);
    ptsB[index++] = LineVertexF(x+width-width/12    , y+height2 ,0,0,0,cap[0]);
    
    ptsB[index++] = LineVertexF(x+width             , y          ,0,0,0,cap[0]);
    ptsB[index++] = LineVertexF(x+width-width/12    , y+height2  ,0,0,0,cap[0]);
    ptsB[index++] = LineVertexF(x+width             , y+height2  ,0,0,0,cap[0]);
    
    //inner part high
    
    ptsB[index++] = LineVertexF(x+1           , y+height2+2   ,crtp[4],cgtp[4],cbtp[4],cap[4]);
    ptsB[index++] = LineVertexF(x+width-1     , y+height2+2   ,crtp[5],cgtp[5],cbtp[5],cap[5]);
    ptsB[index++] = LineVertexF(x+1           , y+height      ,crtp[4],cgtp[4],cbtp[4],cap[4]);
    
    ptsB[index++] = LineVertexF(x+width-1     , y+height2+2   ,crtp[5],cgtp[5],cbtp[5],cap[5]);
    ptsB[index++] = LineVertexF(x+1           , y+height      ,crtp[4],cgtp[4],cbtp[4],cap[4]);
    ptsB[index++] = LineVertexF(x+width-1     , y+height      ,crtp[5],cgtp[5],cbtp[5],cap[5]);
    
    //top
    //    ptsB[index++] = LineVertexF(x+1               , y+height     ,crtp[3],cgtp[3],cbtp[3],cap[3]);
    //    ptsB[index++] = LineVertexF(x+width-1         , y+height     ,crtp[3],cgtp[3],cbtp[3],cap[3]);
    //    ptsB[index++] = LineVertexF(x+width-1         , y+height-2   ,crtp[3],cgtp[3],cbtp[3],cap[3]);
    //
    //    ptsB[index++] = LineVertexF(x+1               , y+height     ,crtp[3],cgtp[3],cbtp[3],cap[3]);
    //    ptsB[index++] = LineVertexF(x+width-1         , y+height-2   ,crtp[3],cgtp[3],cbtp[3],cap[3]);
    //    ptsB[index++] = LineVertexF(x+1               , y+height-2   ,crtp[3],cgtp[3],cbtp[3],cap[3]);
    
    
    //inner part low
    ptsB[index++] = LineVertexF(x+width/16           , y+2   ,crtp[1],cgtp[1],cbtp[1],cap[1]);
    ptsB[index++] = LineVertexF(x+width-width/16     , y+2   ,crtp[1],cgtp[1],cbtp[1],cap[1]);
    ptsB[index++] = LineVertexF(x+width/16           , y+height2      ,crtp[1],cgtp[1],cbtp[1],cap[1]);
    
    ptsB[index++] = LineVertexF(x+width-width/16     , y+2   ,crtp[1],cgtp[1],cbtp[1],cap[1]);
    ptsB[index++] = LineVertexF(x+width/16           , y+height2      ,crtp[1],cgtp[1],cbtp[1],cap[1]);
    ptsB[index++] = LineVertexF(x+width-width/16     , y+height2      ,crtp[1],cgtp[1],cbtp[1],cap[1]);
    
    //bottom high
    ptsB[index++] = LineVertexF(x+1       , y+height2             ,crtp[3],cgtp[3],cbtp[3],cap[3]);
    ptsB[index++] = LineVertexF(x+width-1 , y+height2             ,crtp[3],cgtp[3],cbtp[3],cap[3]);
    ptsB[index++] = LineVertexF(x+width-1 , y+height2+2           ,crtp[3],cgtp[3],cbtp[3],cap[3]);
    
    ptsB[index++] = LineVertexF(x+1       , y+height2             ,crtp[3],cgtp[3],cbtp[3],cap[3]);
    ptsB[index++] = LineVertexF(x+width-1 , y+height2+2           ,crtp[3],cgtp[3],cbtp[3],cap[3]);
    ptsB[index++] = LineVertexF(x+1       , y+height2+2           ,crtp[3],cgtp[3],cbtp[3],cap[3]);
    
    //bottom low
    ptsB[index++] = LineVertexF(x+width/16       , y             ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+width-width/16 , y             ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+width-width/16 , y+2 ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    
    ptsB[index++] = LineVertexF(x+width/16       , y             ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+width-width/16 , y+2 ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+width/16       , y+2 ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    
    //shadow
    if (shadow_type) {
        if (shadow_type&PR_SHADOW_WHITE) {
            ptsB[index++] = LineVertexF(x+1       , y+height*7/8    ,0,0,0,128);
            ptsB[index++] = LineVertexF(x+width/3 , y+height2+height2       ,0,0,0,0);
            ptsB[index++] = LineVertexF(x+1       , y+height2           ,0,0,0,128);
        }
        
        if (shadow_type&PR_SHADOW_SMALL_BLACK) {
            ptsB[index++] = LineVertexF(x+shadow_left_black_ofs         , y+height    ,0,0,0,128);
            ptsB[index++] = LineVertexF(x+shadow_left_black_ofs+width/4 , y+height*2/5+height2       ,0,0,0,0);
            ptsB[index++] = LineVertexF(x+shadow_left_black_ofs         , y+height*2/5           ,0,0,0,128);
        }
        
        if (shadow_type&PR_SHADOW_LARGE_BLACK) {
            ptsB[index++] = LineVertexF(x+shadow_left_black_ofs         , y+height    ,0,0,0,128);
            ptsB[index++] = LineVertexF(x+shadow_left_black_ofs+width/5 , y+height      ,0,0,0,128);
            ptsB[index++] = LineVertexF(x+shadow_left_black_ofs         , y+height*2/5           ,0,0,0,128);
            
            ptsB[index++] = LineVertexF(x+shadow_left_black_ofs+width/5 , y+height    ,0,0,0,128);
            ptsB[index++] = LineVertexF(x+shadow_left_black_ofs+width/2 , y+height*2/5+height2*3       ,0,0,0,0);
            ptsB[index++] = LineVertexF(x+shadow_left_black_ofs         , y+height*2/5           ,0,0,0,128);
        }
    }
    return index;
#endif
    return 0;
}


int RenderUtils::DrawKeyB(LineVertexF *ptsB,int index,float x,float y,float width,float height,float border_size,int crt,int cgt,int cbt,int ca,int subnote,int note_idx,int channel) {
#if 0
    int crtp[6],cgtp[6],cbtp[6],cap[6];
    float height2;
    
    bool pressed=pianoroll_key_status[channel][note_idx]&PR_KEY_PRESSED;
    
    if (!pressed) {
        //y+=height/16;
        //height=height*15/16;
        height2=height/4;
    } else {
        height2=height/12;
        crt=crt*0.8f;
        cgt=cgt*0.8f;
        cbt=cbt*0.8f;
    }
    
    for (int ii=0;ii<4;ii++) {
        double fact=1;
        double ofs=0;
        switch (ii) {
            case 3:fact=1.5f; ofs=128; break;
            case 2:fact=1.2f; ofs=128; break;
            case 1:fact=1.25f; ofs=100; break;
            case 0:fact=0.5f; ofs=0; break;
        }
        crtp[ii]=crt*fact+ofs;cgtp[ii]=cgt*fact+ofs;cbtp[ii]=cbt*fact+ofs;cap[ii]=ca;
        if (crtp[ii]>255) crtp[ii]=255;if (cgtp[ii]>255) cgtp[ii]=255;if (cbtp[ii]>255) cbtp[ii]=255;
        if (cap[ii]>255) cap[ii]=255;
    }
    if (subnote) {
        if (subnote>0) {
            float fact=1.1+(float)subnote*0.25/8.0;
            float ofs=32+subnote*4;
            crtp[4]=crt*fact+ofs;cgtp[4]=cgt*fact+ofs;cbtp[4]=cbt*fact+ofs;cap[4]=ca;
            crtp[5]=crt*fact+ofs;cgtp[5]=cgt*fact+ofs;cbtp[5]=cbt*fact+ofs;cap[5]=ca;
        } else {
            float fact=1;
            float ofs=0;
            fact=1.1-(float)subnote*0.25/8.0; ofs=32-subnote*4;
            crtp[4]=crt*fact+ofs;cgtp[4]=cgt*fact+ofs;cbtp[4]=cbt*fact+ofs;cap[4]=ca;
            crtp[5]=crt*fact+ofs;cgtp[5]=cgt*fact+ofs;cbtp[5]=cbt*fact+ofs;cap[5]=ca;
        }
        for (int ii=4;ii<6;ii++) {
            if (crtp[ii]>255) crtp[ii]=255;
            if (cgtp[ii]>255) cgtp[ii]=255;
            if (cbtp[ii]>255) cbtp[ii]=255;
            if (cap[ii]>255) cap[ii]=255;
        }
    } else {
        crtp[4]=crt;cgtp[4]=cgt;cbtp[4]=cbt;cap[4]=ca;
        crtp[5]=crt;cgtp[5]=cgt;cbtp[5]=cbt;cap[5]=ca;
    }
    
    //to have the black key slightly overlapping the felt
    height*=1.02;
    
    //left
    ptsB[index++] = LineVertexF(x               , y         ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+width/8             , y+height2         ,crtp[1],cgtp[1],cbtp[1],cap[1]);
    ptsB[index++] = LineVertexF(x+width/8             , y+height  ,crtp[1],cgtp[1],cbtp[1],cap[1]);
    
    ptsB[index++] = LineVertexF(x               , y         ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+width/8             , y+height  ,crtp[1],cgtp[1],cbtp[1],cap[1]);
    ptsB[index++] = LineVertexF(x               , y+height  ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    
    //right
    ptsB[index++] = LineVertexF(x+width             , y         ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+width-width/8           , y+height2         ,crtp[1],cgtp[1],cbtp[1],cap[1]);
    ptsB[index++] = LineVertexF(x+width-width/8           , y+height  ,crtp[1],cgtp[1],cbtp[1],cap[1]);
    
    ptsB[index++] = LineVertexF(x+width             , y         ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+width-width/8           , y+height  ,crtp[1],cgtp[1],cbtp[1],cap[1]);
    ptsB[index++] = LineVertexF(x+width             , y+height  ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    
    //inner part high
    
    ptsB[index++] = LineVertexF(x+width/8           , y+height2+2   ,crtp[4],cgtp[4],cbtp[4],cap[4]);
    ptsB[index++] = LineVertexF(x+width-width/8     , y+height2+2   ,crtp[5],cgtp[5],cbtp[5],cap[5]);
    ptsB[index++] = LineVertexF(x+width/8           , y+height      ,crtp[4],cgtp[4],cbtp[4],cap[4]);
    
    ptsB[index++] = LineVertexF(x+width-width/8     , y+height2+2   ,crtp[5],cgtp[5],cbtp[5],cap[5]);
    ptsB[index++] = LineVertexF(x+width/8           , y+height     ,crtp[4],cgtp[4],cbtp[4],cap[4]);
    ptsB[index++] = LineVertexF(x+width-width/8     , y+height      ,crtp[5],cgtp[5],cbtp[5],cap[5]);
    
    //top
    //    ptsB[index++] = LineVertexF(x+1               , y+height     ,crtp[3],cgtp[3],cbtp[3],cap[3]);
    //    ptsB[index++] = LineVertexF(x+width-1         , y+height     ,crtp[3],cgtp[3],cbtp[3],cap[3]);
    //    ptsB[index++] = LineVertexF(x+width-1         , y+height-2   ,crtp[3],cgtp[3],cbtp[3],cap[3]);
    //
    //    ptsB[index++] = LineVertexF(x+1               , y+height     ,crtp[3],cgtp[3],cbtp[3],cap[3]);
    //    ptsB[index++] = LineVertexF(x+width-1         , y+height-2   ,crtp[3],cgtp[3],cbtp[3],cap[3]);
    //    ptsB[index++] = LineVertexF(x+1               , y+height-2   ,crtp[3],cgtp[3],cbtp[3],cap[3]);
    
    
    //inner part low
    ptsB[index++] = LineVertexF(x           , y+2         ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+width     , y+2         ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+width/8           , y+height2   ,crtp[1],cgtp[1],cbtp[1],cap[1]);
    
    ptsB[index++] = LineVertexF(x+width     , y+2         ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+width/8           , y+height2   ,crtp[1],cgtp[1],cbtp[1],cap[1]);
    ptsB[index++] = LineVertexF(x+width-width/8     , y+height2   ,crtp[1],cgtp[1],cbtp[1],cap[1]);
    
    //bottom high
    ptsB[index++] = LineVertexF(x+width/8       , y+height2             ,crtp[3],cgtp[3],cbtp[3],cap[3]);
    ptsB[index++] = LineVertexF(x+width-width/8 , y+height2             ,crtp[3],cgtp[3],cbtp[3],cap[3]);
    ptsB[index++] = LineVertexF(x+width-width/8 , y+height2+2           ,crtp[3],cgtp[3],cbtp[3],cap[3]);
    
    ptsB[index++] = LineVertexF(x+width/8       , y+height2             ,crtp[3],cgtp[3],cbtp[3],cap[3]);
    ptsB[index++] = LineVertexF(x+width-width/8 , y+height2+2           ,crtp[3],cgtp[3],cbtp[3],cap[3]);
    ptsB[index++] = LineVertexF(x+width/8       , y+height2+2           ,crtp[3],cgtp[3],cbtp[3],cap[3]);
    
    //bottom low
    ptsB[index++] = LineVertexF(x      , y             ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+width , y             ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+width , y+2 ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    
    ptsB[index++] = LineVertexF(x       , y             ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x+width , y+2 ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    ptsB[index++] = LineVertexF(x       , y+2 ,crtp[0],cgtp[0],cbtp[0],cap[0]);
    
    return index;
#endif
    return 0;
}


void RenderUtils::DrawPianoRollFX(uint ww,uint hh,int horiz_vert,float note_display_range, float note_display_offset,int fx_len,int color_mode,float mScaleFactor,char *voices_label) {
#if 0
    LineVertexF *ptsB;
    int crt,cgt,cbt,ca;
    int index;
    int voices_posX[SOUND_MAXVOICES_BUFFER_FX];
    //int band_width,ofs_band;
    static bool first_call=true;
    
    
    if (first_call) {
        for (int i=0;i<SOUND_MAXVOICES_BUFFER_FX;i++) {
            mVoicesNamePiano[i]=NULL;
        }
        first_call=false;
        memset(mOctavesIndex,0,sizeof(mOctavesIndex));
    }
    
    if (fx_len>MIDIFX_LEN) fx_len=MIDIFX_LEN;
    if (fx_len<=MIDIFX_OFS) fx_len=MIDIFX_OFS+1;
    
    if (fx_len!=data_midifx_len) {
        data_midifx_len=fx_len;
        //data_midifx_first=1;
    }
    
    if (!mOscilloFont[0]) {
        NSString *fontPath;
        //if (mScaleFactor<2) fontPath = [[NSBundle mainBundle] pathForResource:@"tracking10" ofType: @"fnt"];
        //else fontPath = [[NSBundle mainBundle] pathForResource:@"tracking14" ofType: @"fnt"];
        fontPath = [[NSBundle mainBundle] pathForResource:@"tracking10" ofType: @"fnt"];
        mOscilloFont[0] = new CFont([fontPath UTF8String]);
        fontPath = [[NSBundle mainBundle] pathForResource:@"tracking16" ofType: @"fnt"];
        mOscilloFont[1] = new CFont([fontPath UTF8String]);
        fontPath = [[NSBundle mainBundle] pathForResource:@"tracking24" ofType: @"fnt"];
        mOscilloFont[2] = new CFont([fontPath UTF8String]);
    }
    
    if (mOscilloFont[1] && voices_label)
        for (int i=0;i<m_genNumMidiVoicesChannels;i++) {
            if (mVoicesNamePiano[i]) {
                if (strcmp(mVoicesNamePiano[i]->mText,voices_label+i*32)) {
                    //not the same, reset string
                    delete mVoicesNamePiano[i];
                    mVoicesNamePiano[i]=NULL;
                }
            }
            if (!mVoicesNamePiano[i]) {
                mVoicesNamePiano[i]=new CGLString(voices_label+i*32, mOscilloFont[1],mScaleFactor);
            }
        }
    
    if (mOscilloFont[1] && (mOctavesIndex[0]==NULL)) {
        char str_tmp[3];
        for (int i=0;i<256/12;i++) {
            snprintf(str_tmp,3,"%d",i);
            mOctavesIndex[i]=new CGLString(str_tmp, mOscilloFont[1],mScaleFactor);
        }
    }
    
    ptsB=(LineVertexF*)malloc(sizeof(LineVertexF)*30*MAX_BARS);
    max_indices=30*MAX_BARS;
    
    
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    
    
    glVertexPointer(2, GL_FLOAT, sizeof(LineVertexF), &ptsB[0].x);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(LineVertexF), &ptsB[0].r);
    
    
    //////////////////////////////////////////////
    
    float note_posX[256];
    uint8_t note_posType[256];
    int midi_data_ofs=MIDIFX_LEN-MIDIFX_OFS-1;
    
    //white keys
    float visible_wkeys_range=(note_display_range*7.0/12.0);
    float width=(float)(ww)/visible_wkeys_range;
    float height=width*4;
    float x;
    float y;
    //black keys
    float widthB=width/2.0;
    float heightB=height*3/5;
    float xB;
    float yB;
    
    int border_size=2;
    int num_rows=hh/(height+16);
    if (m_genNumMidiVoicesChannels<num_rows) num_rows=m_genNumMidiVoicesChannels;
    
    //draw piano felt
    index=0;
    int crtp[2],cgtp[2],cbtp[2],cap[2];
    crtp[0]=60;crtp[1]=120;
    cgtp[0]=60;cgtp[1]=0;
    cbtp[0]=60;cbtp[1]=0;
    cap[0]=255;cap[1]=255;
    float wd=height/32.0;
    for (int j=0;j<num_rows;j++) {
        y=hh-(height+16)*(j+1)+height;
        ptsB[index++] = LineVertexF(0,y     ,crtp[0],cgtp[0],cbtp[0],cap[0]);
        ptsB[index++] = LineVertexF(ww,y    ,crtp[0],cgtp[0],cbtp[0],cap[0]);
        ptsB[index++] = LineVertexF(ww,y+wd ,crtp[1],cgtp[1],cbtp[1],cap[1]);
        
        ptsB[index++] = LineVertexF(0,y      ,crtp[0],cgtp[0],cbtp[0],cap[0]);
        ptsB[index++] = LineVertexF(ww,y+wd  ,crtp[1],cgtp[1],cbtp[1],cap[1]);
        ptsB[index++] = LineVertexF(0,y+wd   ,crtp[1],cgtp[1],cbtp[1],cap[1]);
    }
    glDrawArrays(GL_TRIANGLES, 0, index);
    
    index=0;
    
    //prepapre key data & check all key status
    int wk_idx=0;//white key counter
    int bk_idx=0;//black key counter
    for (int note=0;note<256;note++) {
        bool whitekey=false;
        int note_idx=(note%12);
        if ( (note_idx==0)||(note_idx==2)||(note_idx==4)||(note_idx==5)||(note_idx==7)||(note_idx==9)||(note_idx==11)) whitekey=true;
        
        if (whitekey) {
            x=(float)(ww)*wk_idx/visible_wkeys_range-note_display_offset;
            note_posX[note]=x;
            note_posType[note]=0;
            wk_idx++;
        } else {
            x=(float)(ww)*(wk_idx-1)/visible_wkeys_range-note_display_offset;
            switch (bk_idx%5) {
                case 0: //C#
                    xB=(x+width*8.0f/12);
                    break;
                case 1://D#
                    xB=(x+width*10.0f/12);
                    break;
                case 2://F#
                    xB=(x+width*8.0f/12);
                    break;
                case 3://G#
                    xB=(x+width*9.0f/12);
                    break;
                case 4://A#
                    xB=(x+width*10.0f/12);
                    break;
            }
            note_posX[note]=xB;
            note_posType[note]=1;
            bk_idx++;
        }
        for (int i=0;i<SOUND_MAXMOD_CHANNELS;i++) {
            pianoroll_key_status[i][note]=(whitekey?PR_WHITE_KEY:0)|(note_idx<<4);
        }
    }
    for (int i=0; i<256; i++) { //for each channels
        if ((data_midifx_note[midi_data_ofs][i])&&
            (data_midifx_vol[midi_data_ofs][i]>=data_midifx_vol[midi_data_ofs+1][i]) ) {  //do we have a note ?
            unsigned int note=data_midifx_note[midi_data_ofs][i];
            unsigned int instr=data_midifx_instr[midi_data_ofs][i]&63;
            
            pianoroll_key_status[instr%num_rows][note]|=PR_KEY_PRESSED;
        }
    }
    
    //draw white keys
    for (int j=0;j<num_rows;j++) {
        y=hh-(height+16)*(j+1);
        
        for (int note=0;note<256;note++) {
            if (note_posType[note]==0) { //white key
                x=note_posX[note];
                if ((x+width>0)||(x<ww) ) {
                    if (index+INDICES_SIZE_KEYW>=max_indices) {
                        glDrawArrays(GL_TRIANGLES, 0, index);
                        index=0;
                    }
                    index=DrawKeyW(ptsB,index,x,y,width,height,2,220,220,220,255,0,note,j);
                }
            }
        }
    }
    
    //1st pass draw notes - white keys
    for (int i=0; i<256; i++) { //for each channels
        if ((data_midifx_note[midi_data_ofs][i])&&
            (data_midifx_vol[midi_data_ofs][i]>=data_midifx_vol[midi_data_ofs+1][i]) ) {  //do we have a note ?
            unsigned int note=data_midifx_note[midi_data_ofs][i];
            unsigned int instr=data_midifx_instr[midi_data_ofs][i]&63;
            unsigned int vol=data_midifx_vol[midi_data_ofs][i];
            unsigned int st=data_midifx_st[midi_data_ofs][i];
            
            if ( note_posType[note]==0 ) { //white key
                int subnote=data_midifx_subnote[midi_data_ofs][i];
                subnote=(subnote<8?subnote:subnote-8-7);
                
                int colidx=instr&63;
                int crt=((data_midifx_col[colidx&31]>>16)&0xFF);
                int cgt=((data_midifx_col[colidx&31]>>8)&0xFF);
                int cbt=(data_midifx_col[colidx&31]&0xFF);
                
                if (colidx&0x20) {
                    crt=(crt+255)/2;
                    cgt=(cgt+255)/2;
                    cbt=(cbt+255)/2;
                }
                
                if (vol&&(st&VOICE_ON)) {  //check volume & status => we have something
                    x=note_posX[note];
                    
                    if (note_posType[note]==0) { //white key
                        y=hh-(height+16)*((instr%num_rows)+1);
                        if (index+INDICES_SIZE_KEYW>=max_indices) {
                            glDrawArrays(GL_TRIANGLES, 0, index);
                            index=0;
                        }
                        if ( (x+width>0)||(x<ww)) {
                            index=DrawKeyW(ptsB,index,x,y,width,height,border_size,crt,cgt,cbt,255,subnote,note,instr%num_rows);
                        }
                    }
                }
            }
        }
    }
    
    //draw black keys
    for (int j=0;j<num_rows;j++) {
        y=hh-(height+16)*(j+1);
        for (int note=0;note<256;note++) {
            if (note_posType[note]) {
                yB=y+height-heightB;
                xB=note_posX[note];
                if (index+INDICES_SIZE_KEYB>=max_indices) {
                    glDrawArrays(GL_TRIANGLES, 0, index);
                    index=0;
                }
                if ( (xB+widthB>0)||(xB<ww)) index=DrawKeyB(ptsB,index,xB,yB,widthB,heightB,border_size,40,40,40,255,0,note,j);
            }
        }
    }
    
    //2nd pass draw notes - black keys
    for (int i=0; i<256; i++) { //for each channels
        if ((data_midifx_note[midi_data_ofs][i])&&
            (data_midifx_vol[midi_data_ofs][i]>=data_midifx_vol[midi_data_ofs+1][i]) ) {  //do we have a note ?
            unsigned int note=data_midifx_note[midi_data_ofs][i];
            
            uint8_t note_idx=note%12;
            if ((note_idx==1)||(note_idx==3)||(note_idx==6)||(note_idx==8)||(note_idx==10)) { //black key
                
                unsigned int instr=data_midifx_instr[midi_data_ofs][i]&63;
                unsigned int vol=data_midifx_vol[midi_data_ofs][i];
                unsigned int st=data_midifx_st[midi_data_ofs][i];
                
                int subnote=data_midifx_subnote[midi_data_ofs][i];
                subnote=(subnote<8?subnote:subnote-8-7);
                
                int colidx=instr&63;
                int crt=((data_midifx_col[colidx&31]>>16)&0xFF);
                int cgt=((data_midifx_col[colidx&31]>>8)&0xFF);
                int cbt=(data_midifx_col[colidx&31]&0xFF);
                
                if (colidx&0x20) {
                    crt=(crt+255)/2;
                    cgt=(cgt+255)/2;
                    cbt=(cbt+255)/2;
                }
                
                if (vol&&(st&VOICE_ON)) {  //check volume & status => we have something
                    //                    printf("B instr %d note %d note%%12 %d type %d x %f\n",instr,note,note%12,note_posType[note],x);
                    
                    x=note_posX[note];
                    
                    if (note_posType[note]==1) { //back key
                        y=hh-(height+16)*((instr%num_rows)+1)+height-heightB;
                        if (index+INDICES_SIZE_KEYB>=max_indices) {
                            glDrawArrays(GL_TRIANGLES, 0, index);
                            index=0;
                        }
                        if ( (x+widthB>0)||(x<ww))  index=DrawKeyB(ptsB,index,x,y,widthB,heightB,border_size,crt,cgt,cbt,255,subnote,note,instr%num_rows);
                    }
                }
            }
        }
    }
    
    
    
    memset(voices_posX,0,sizeof(voices_posX));
    //draw label small colored boxes
    if (voices_label&&settings[GLOB_FXPianoRollVoicesLabels].detail.mdz_switch.switch_value)
        for (int i=0;i<m_genNumMidiVoicesChannels;i++) {
            int j=i%num_rows;
            y=hh-(height+16)*(j+1)+height+1;
            
            if (mVoicesNamePiano[i]) {
                x=voices_posX[j]+16;
                
                voices_posX[j]+=16+mOscilloFont[1]->maxCharWidth*strlen(mVoicesNamePiano[i]->mText)/mScaleFactor;
                
                int colidx=i&63;
                int crt=((data_midifx_col[colidx&31]>>16)&0xFF);
                int cgt=((data_midifx_col[colidx&31]>>8)&0xFF);
                int cbt=(data_midifx_col[colidx&31]&0xFF);
                
                if (colidx&0x20) {
                    crt=(crt+255)/2;
                    cgt=(cgt+255)/2;
                    cbt=(cbt+255)/2;
                }
                
                y=hh-(height+16)*(j+1)+height+4;
                if (index+INDICES_SIZE_BOX>=max_indices) {
                    glDrawArrays(GL_TRIANGLES, 0, index);
                    index=0;
                }
                index=DrawBox(ptsB,index,x-10,y,8,8,1/*border_size*/,crt,cgt,cbt,255,0);
            }
        }
    
    glDrawArrays(GL_TRIANGLES, 0, index);
    
    
    
    //////////////////////////////////////////////
    
    //    glEnable(GL_BLEND);
    //    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisable(GL_BLEND);
    
    memset(voices_posX,0,sizeof(voices_posX));
    
    //draw label
    if (voices_label&&settings[GLOB_FXPianoRollVoicesLabels].detail.mdz_switch.switch_value) {
        for (int i=0;i<m_genNumMidiVoicesChannels;i++) {
            int j=i%num_rows;
            y=hh-(height+16)*(j+1)+height+4+1;
            
            if (mVoicesNamePiano[i]) {
                x=voices_posX[j]+16;
                glPushMatrix();
                glTranslatef(x,y, 0.0f);
                mVoicesNamePiano[i]->Render(255);
                voices_posX[j]+=16+mOscilloFont[1]->maxCharWidth*strlen(mVoicesNamePiano[i]->mText)/mScaleFactor;
                glPopMatrix();
            }
        }
    }
    
    if (mOctavesIndex[0]&&settings[GLOB_FXPianoRollOctavesLabels].detail.mdz_switch.switch_value) {
        for (int i=0;i<m_genNumMidiVoicesChannels;i++) {
            int j=i%num_rows;
            for (int o=0;o<256/12;o++) {
                x=o*width*7.0-note_display_offset;
                
                if (mOctavesIndex[o]) {
                    glPushMatrix();
                    
                    if (pianoroll_key_status[j][o*12]&PR_KEY_PRESSED) y=hh-(height+16)*(j+1)+16-12+height/24;
                    else y=hh-(height+16)*(j+1)+16-12+height/8;
                    
                    float lblwidth=strlen(mOctavesIndex[o]->mText)*mOscilloFont[1]->maxCharWidth/mScaleFactor;
                    x+=(width-lblwidth)/2;
                    
                    glTranslatef(x,y, 0.0f);
                    mOctavesIndex[o]->Render(32);
                    
                    glPopMatrix();
                }
            }
        }
    }
    free(ptsB);
#endif
}

void RenderUtils::DrawPianoRollSynthesiaFX(uint ww,uint hh,int horiz_vert,float note_display_range, float note_display_offset,int fx_len,int color_mode,float mScaleFactor,char *voices_label) {
#if 0
    LineVertexF *ptsB;
    coordData *texcoords; /* Holds Float Info For 4 Sets Of Texture coordinates. */
    int crt,cgt,cbt,ca;
    int index;
    static bool first_call=true;
    float band_width;
    float line_width;
    float line_width_extra;
    uint8_t sparkPresent[256];
    static uint8_t sparkIntensity[256];
    float ofsy;
    
    
    if (first_call) {
        for (int i=0;i<SOUND_MAXVOICES_BUFFER_FX;i++) {
            mVoicesNamePiano[i]=NULL;
        }
        first_call=false;
        memset(mOctavesIndex,0,sizeof(mOctavesIndex));
        memset(sparkIntensity,0,sizeof(sparkIntensity));
        pianoroll_cpt=0;
    }
    
    if (!mOscilloFont[0]) {
        NSString *fontPath;
        //if (mScaleFactor<2) fontPath = [[NSBundle mainBundle] pathForResource:@"tracking10" ofType: @"fnt"];
        //else fontPath = [[NSBundle mainBundle] pathForResource:@"tracking14" ofType: @"fnt"];
        fontPath = [[NSBundle mainBundle] pathForResource:@"tracking10" ofType: @"fnt"];
        mOscilloFont[0] = new CFont([fontPath UTF8String]);
        fontPath = [[NSBundle mainBundle] pathForResource:@"tracking16" ofType: @"fnt"];
        mOscilloFont[1] = new CFont([fontPath UTF8String]);
        fontPath = [[NSBundle mainBundle] pathForResource:@"tracking24" ofType: @"fnt"];
        mOscilloFont[2] = new CFont([fontPath UTF8String]);
    }
    
    float note_posX[256];
    uint8_t note_posType[256];
    int midi_data_ofs=MIDIFX_LEN-MIDIFX_OFS-1;
    
    //white keys
    float visible_wkeys_range=(note_display_range*7.0/12.0);
    float width=(float)(ww)/visible_wkeys_range;
    float height=width*4;
    float x;
    float y;
    //black keys
    float widthB=round(width/2.0);
    float heightB=height*3/5;
    float xB;
    float yB;
    int voices_posX[SOUND_MAXVOICES_BUFFER_FX];
    
    ofsy=0;
    
    if (mOscilloFont[1] && voices_label)
        for (int i=0;i<m_genNumMidiVoicesChannels;i++) {
            if (mVoicesNamePiano[i]) {
                if (strcmp(mVoicesNamePiano[i]->mText,voices_label+i*32)) {
                    //not the same, reset string
                    delete mVoicesNamePiano[i];
                    mVoicesNamePiano[i]=NULL;
                }
            }
            if (!mVoicesNamePiano[i]) {
                mVoicesNamePiano[i]=new CGLString(voices_label+i*32, mOscilloFont[1],mScaleFactor);
            }
        }
    
    if (mOscilloFont[1] && (mOctavesIndex[0]==NULL)) {
        char str_tmp[3];
        for (int i=0;i<256/12;i++) {
            snprintf(str_tmp,3,"%d",i-1);
            mOctavesIndex[i]=new CGLString(str_tmp, mOscilloFont[1],mScaleFactor);
        }
    }
    
    if (voices_label&&settings[GLOB_FXPianoRollVoicesLabels].detail.mdz_switch.switch_value) {
        int labels_lines_needed=1;
        x=16;
        for (int i=0;i<m_genNumMidiVoicesChannels;i++) {
            int j=i;
            
            if (mVoicesNamePiano[i]) {
                
                x+=16+mOscilloFont[1]->maxCharWidth*strlen(mVoicesNamePiano[i]->mText)/mScaleFactor;
                if (x>ww) {
                    x=16+16+mOscilloFont[1]->maxCharWidth*strlen(mVoicesNamePiano[i]->mText)/mScaleFactor;
                    labels_lines_needed+=1;
                }
            }
        }
        if (labels_lines_needed>3) labels_lines_needed=3;
        ofsy=16*labels_lines_needed;
    }
    
    ptsB=(LineVertexF*)malloc(sizeof(LineVertexF)*30*MAX_BARS);
    max_indices=30*MAX_BARS;
    texcoords=(coordData*)malloc(sizeof(coordData)*256*6*8); //max 256 notes, 6pts/spark and max 8 sparks/notes
    
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    
    glVertexPointer(2, GL_FLOAT, sizeof(LineVertexF), &ptsB[0].x);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(LineVertexF), &ptsB[0].r);
    
    data_midifx_len=MIDIFX_OFS+1; //yoyofr: to review
    band_width=(float)(hh+0*hh/4)/data_midifx_len;
    line_width=1.0f*ww/note_display_range;
    line_width_extra=3;
    
    //////////////////////////////////////////////
    
    int data_bar2draw_count=0;
    
    for (int i=0; i<256; i++) { //for each channels
        int j=MIDIFX_LEN-data_midifx_len;
        while (j<MIDIFX_LEN) {  //while not having reach roof
            if (data_midifx_note[j][i]) {  //do we have a note ?
                unsigned int instr=data_midifx_instr[j][i];
                unsigned int vol=data_midifx_vol[j][i];
                unsigned int st=data_midifx_st[j][i];
                unsigned int note=data_midifx_note[j][i];
                int subnote=data_midifx_subnote[j][i];
                
                if (vol&&(st&VOICE_ON)) {  //check volume & status => we have something
                    data_bar2draw[data_bar2draw_count].startidx=j-(MIDIFX_LEN-data_midifx_len);
                    data_bar2draw[data_bar2draw_count].note=note;
                    data_bar2draw[data_bar2draw_count].subnote=subnote;
                    data_bar2draw[data_bar2draw_count].instr=instr;
                    data_bar2draw[data_bar2draw_count].size=0;
                    data_bar2draw[data_bar2draw_count].played=0;
                    data_bar2draw[data_bar2draw_count].frameidx=st>>8;
                    while ( (data_midifx_instr[j][i]==instr)&&
                           (data_midifx_note[j][i]==note)&&
                           (data_midifx_vol[j][i]<=vol)&&
                           (data_midifx_st[j][i]&VOICE_ON) ) {  //while same bar (instru & notes), increase size
                        data_bar2draw[data_bar2draw_count].size++;
                        //propagate lowest frame nb
                        data_midifx_st[j][i]=st;
                        if ((settings[GLOB_FXMIDIBarVibrato].detail.mdz_switch.switch_value==1)&&(data_midifx_subnote[j][i]!=subnote)) break;
                        //take most recent subnote if before playing bar
                        if (j<MIDIFX_LEN-MIDIFX_OFS) data_bar2draw[data_bar2draw_count].subnote=data_midifx_subnote[j][i];
                        if (j==(MIDIFX_LEN-MIDIFX_OFS-1)) data_bar2draw[data_bar2draw_count].played=1;
                        //update vol to latest encountered one, allow to retrigger if volume increases from last bar to new one and manage special case with vol = 2111121111
                        vol=data_midifx_vol[j][i];
                        j++;
                        
                        if (j==(MIDIFX_LEN-MIDIFX_OFS-1)) break;
                        
                        if (j==MIDIFX_LEN) break;
                    }
                    data_bar2draw_count++;
                    //j++;
                } else j++;
            } else j++;
            if (data_bar2draw_count==MAX_BARS) break;
        }
        if (data_bar2draw_count==MAX_BARS) break;
    }
    qsort(data_bar2draw,data_bar2draw_count,sizeof(t_data_bar2draw),qsort_CompareBar);
    
    int border_size=2;
    
    //prepapre key data & check all key status
    int wk_idx=0;//white key counter
    int bk_idx=0;//black key counter
    for (int note=0;note<256;note++) {
        bool whitekey=false;
        int note_idx=(note%12);
        if ( (note_idx==0)||(note_idx==2)||(note_idx==4)||(note_idx==5)||(note_idx==7)||(note_idx==9)||(note_idx==11)) whitekey=true;
        
        if (whitekey) {
            x=(float)(ww)*wk_idx/visible_wkeys_range-note_display_offset;
            note_posX[note]=x;
            note_posType[note]=0;
            wk_idx++;
        } else {
            x=(float)(ww)*(wk_idx-1)/visible_wkeys_range-note_display_offset;
            switch (bk_idx%5) {
                case 0: //C#
                    xB=(x+width*8.0f/12);
                    break;
                case 1://D#
                    xB=(x+width*10.0f/12);
                    break;
                case 2://F#
                    xB=(x+width*8.0f/12);
                    break;
                case 3://G#
                    xB=(x+width*9.0f/12);
                    break;
                case 4://A#
                    xB=(x+width*10.0f/12);
                    break;
            }
            note_posX[note]=xB;
            note_posType[note]=1;
            bk_idx++;
        }
        pianoroll_key_status[0][note]=(whitekey?PR_WHITE_KEY:0)|(note_idx<<4);
    }
    for (int i=0; i<256; i++) { //for each channels
        if ((data_midifx_note[midi_data_ofs][i])&&
            (data_midifx_vol[midi_data_ofs][i]>=data_midifx_vol[midi_data_ofs+1][i]) ) {  //do we have a note ?
            unsigned int note=data_midifx_note[midi_data_ofs][i];
            unsigned int instr=data_midifx_instr[midi_data_ofs][i]&63;
            
            pianoroll_key_status[0][note]|=PR_KEY_PRESSED;
        }
    }
    
    
    index=0;
    for (int i=0;i<data_bar2draw_count;i++) {
        int played=data_bar2draw[i].played;
        int note=data_bar2draw[i].note;
        
        int subnote=(data_bar2draw[i].subnote<8?data_bar2draw[i].subnote:data_bar2draw[i].subnote-8-7);
        
        int instr=data_bar2draw[i].instr;
        int colidx;
        if (color_mode==0) { //note
            colidx=(note%12);
        } else if (color_mode==1) { //instru
            colidx=(instr)&63;
        }
        
        if (data_bar2draw[i].size==0) continue;
        
        //printf("i:%d start:%d end:%d instr:%d note:%d played:%d\n",i,data_bar2draw[i].startidx,data_bar2draw[i].startidx+data_bar2draw[i].size,instr,note,played);
        
        
        crt=((data_midifx_col[colidx&31]>>16)&0xFF);
        cgt=((data_midifx_col[colidx&31]>>8)&0xFF);
        cbt=(data_midifx_col[colidx&31]&0xFF);
        
        if (colidx&0x20) {
            crt=(crt+255)/2;
            cgt=(cgt+255)/2;
            cbt=(cbt+255)/2;
        }
        
        if (played) {
            crt=(crt*3+255*3)/6;
            cgt=(cgt*3+255*3)/6;
            cbt=(cbt*3+255*3)/6;
            if (crt>255) crt=255;
            if (cgt>255) cgt=255;
            if (cbt>255) cbt=255;
            line_width_extra=2;
            ca=192;
        } else {
            line_width_extra=0;
            ca=192;
        }
        
        float posNote;
        float wd;
        posNote=note_posX[note];
        if (note_posType[note]==0) wd=width+line_width_extra*2;
        else wd=widthB+line_width_extra*2;
        
        float wd_ofs=wd*subnote/10;
        wd+=wd_ofs;
        posNote-=wd_ofs/2;
        
        float posStart=(int)(data_bar2draw[i].startidx)*(hh-height-16)/data_midifx_len+height+0+ofsy+height/32;
        float posEnd=((int)(data_bar2draw[i].startidx)+(int)(data_bar2draw[i].size))*(hh-height-8)/data_midifx_len+height+0+ofsy+height/32;
        if ( (posNote-line_width_extra+wd>=0) && (posNote-line_width_extra<(int)ww)) {
            int border_size=(line_width>=8?2:1);
            
            
            index=DrawBox(ptsB,index,
                          posNote-line_width_extra, //x
                          posStart-line_width_extra, //y
                          wd, //w
                          posEnd-posStart+line_width_extra*2, //h
                          border_size,crt,cgt,cbt,255,0/*subnote*/);
            
        }
    }
    //    printf("total: %d\n",index);
    glDrawArrays(GL_TRIANGLES, 0, index);
    
    
    //Draw spark fx
    //glDisable(GL_DEPTH_TEST);           /* Disable Depth Testing     */
    glEnable(GL_TEXTURE_2D);            /* Enable 2D Texture Mapping */
    glBindTexture(GL_TEXTURE_2D, txt_pianoRoll[TXT_PIANOROLL_SPARK]);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE);
    
    
    glTexCoordPointer(2, GL_FLOAT, 0, texcoords);
    /* Enable Texture Coordinations Pointer */
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    
    memset(sparkPresent,0,sizeof(sparkPresent));
    index=0;
    if (settings[GLOB_FXPianoRollSpark].detail.mdz_switch.switch_value)
        for (int i=0; i<256; i++) { //for each channels
            if ((data_midifx_note[midi_data_ofs][i]/*||data_midifx_note[midi_data_ofs+1][i]*/)&&
                data_midifx_vol[midi_data_ofs][i] &&
                ( data_midifx_vol[midi_data_ofs][i]>=data_midifx_vol[midi_data_ofs+1][i]) ) {  //do we have a note ?
                unsigned int note=data_midifx_note[midi_data_ofs][i];
                if (!note) note=data_midifx_note[midi_data_ofs+1][i];
                
                //avoid rendering twice for same note
                if (sparkPresent[note]) continue;
                sparkPresent[note]=1;
                if (sparkIntensity[note]<128) sparkIntensity[note]+=8;
                
                unsigned int instr=data_midifx_instr[midi_data_ofs][i];
                int colidx=instr&63;
                int crt=((data_midifx_col[colidx&31]>>16)&0xFF);
                int cgt=((data_midifx_col[colidx&31]>>8)&0xFF);
                int cbt=(data_midifx_col[colidx&31]&0xFF);
                
                if (colidx&0x20) {
                    crt=(crt+255)/2;
                    cgt=(cgt+255)/2;
                    cbt=(cbt+255)/2;
                }
                
                crt=(crt*3+255*3)/6;
                cgt=(cgt*3+255*3)/6;
                cbt=(cbt*3+255*3)/6;
                
                //                crt=crt*1.5f+64;
                //                cgt=cgt*1.5f+64;
                //                cbt=cbt*1.5f+64;
                //
                if (crt>255) crt=255;
                if (cgt>255) cgt=255;
                if (cbt>255) cbt=255;
                
                line_width_extra=2;
                
                float posNote;
                float wd;
                
                posNote=note_posX[note];
                if (note_posType[note]==0) wd=width+line_width_extra*2;
                else wd=widthB+line_width_extra*2;
                
                posNote-=wd/2;
                wd=wd*2;
                
                for (int sp=0;sp<4;sp++) {
                    
                    texcoords[index+0].u=0.0f; texcoords[index+0].v=80.0/128;
                    texcoords[index+1].u=0.0f; texcoords[index+1].v=18.0/128;
                    texcoords[index+2].u=1.0f; texcoords[index+2].v=80.0/128;
                    
                    texcoords[index+3].u=0.0f; texcoords[index+3].v=18.0/128;
                    texcoords[index+4].u=1.0f; texcoords[index+4].v=80.0/128;
                    texcoords[index+5].u=1.0f; texcoords[index+5].v=18.0/128;
                    
                    
                    ptsB[index+0].x=posNote;ptsB[index+0].y=ofsy+0+height+height/32;
                    ptsB[index+1].x=posNote;ptsB[index+1].y=ofsy+0+height+height/32+wd/3;
                    ptsB[index+2].x=posNote+wd;ptsB[index+2].y=ofsy+0+height+height/32;
                    
                    ptsB[index+3].x=posNote;ptsB[index+3].y=ofsy+0+height+height/32+wd/3;
                    ptsB[index+4].x=posNote+wd;ptsB[index+4].y=ofsy+0+height+height/32;
                    ptsB[index+5].x=posNote+wd;ptsB[index+5].y=ofsy+0+height+height/32+wd/3;
                    
                    //apply some distortion
                    float wd_distX=wd/3.0;
                    float wd_distY=wd/9.0;
                    
                    float distorFactors[4][4][6]={
                        {   {+0.7,  3 ,+0.2,  5, -0.3, 11},
                            {+0.2,  3 ,+0.5,  5, -0.4, 5},
                            
                            {+0.5,  5 ,-0.1,  7, +0.4, 13},
                            {+0.3,  2 ,-0.5,  3, +0.2, 3}},
                        
                        {   {+0.5,  1 ,+0.2,  7, -0.3, 7},
                            {+0.3,  3 ,+0.5,  2, -0.4, 7},
                            
                            {-0.3,  7 ,-0.1,  9, +0.4, 5},
                            {-0.2,  5 ,-0.5,  5, +0.2, 11}},
                        
                        {   {-0.7,  2 ,+0.2,  11, -0.3, 9},
                            {+0.2,  5 ,+0.5,  13, +0.7, 5},
                            
                            {+0.6,  9 ,-0.1,  3, +0.4, 3},
                            {+0.4,  1 ,-0.5,  3, +0.2, 8}},
                        
                        {   {+0.8,  9 ,+0.2,  7, -0.3, 9},
                            {+0.4,  11 ,+0.5, 7, +0.4, 5},
                            
                            {-0.5,  3 ,+0.1,  4, -0.4, 11},
                            {-0.3,  5 ,+0.5,  5, +0.2, 3}}};
                    
                    ptsB[index+1].x+=wd_distX*(distorFactors[sp][0][0]*sin(pianoroll_cpt*distorFactors[sp][0][1]*3.14159/32)
                                               +distorFactors[sp][0][2]*sin(pianoroll_cpt*distorFactors[sp][0][3]*3.14159/32)
                                               +distorFactors[sp][0][4]*sin(pianoroll_cpt*distorFactors[sp][0][5]*3.14159/32));
                    
                    ptsB[index+3].x+=wd_distX*(distorFactors[sp][0][0]*sin(pianoroll_cpt*distorFactors[sp][0][1]*3.14159/32)
                                               +distorFactors[sp][0][2]*sin(pianoroll_cpt*distorFactors[sp][0][3]*3.14159/32)
                                               +distorFactors[sp][0][4]*sin(pianoroll_cpt*distorFactors[sp][0][5]*3.14159/32));
                    
                    ptsB[index+1].y+=wd_distY*(distorFactors[sp][1][0]*sin(pianoroll_cpt*distorFactors[sp][1][1]*3.14159/32)
                                               +distorFactors[sp][1][2]*sin(pianoroll_cpt*distorFactors[sp][1][3]*3.14159/32)
                                               +distorFactors[sp][1][4]*sin(pianoroll_cpt*distorFactors[sp][1][5]*3.14159/32));
                    
                    ptsB[index+3].y+=wd_distY*(distorFactors[sp][1][0]*sin(pianoroll_cpt*distorFactors[sp][1][1]*3.14159/32)
                                               +distorFactors[sp][1][2]*sin(pianoroll_cpt*distorFactors[sp][1][3]*3.14159/32)
                                               +distorFactors[sp][1][4]*sin(pianoroll_cpt*distorFactors[sp][1][5]*3.14159/32));
                    
                    ptsB[index+5].x+=wd_distX*(distorFactors[sp][2][0]*sin(pianoroll_cpt*distorFactors[sp][2][1]*3.14159/32)
                                               +distorFactors[sp][2][2]*sin(pianoroll_cpt*distorFactors[sp][2][3]*3.14159/32)
                                               +distorFactors[sp][2][4]*sin(pianoroll_cpt*distorFactors[sp][2][5]*3.14159/32));
                    
                    ptsB[index+5].y+=wd_distY*(distorFactors[sp][3][0]*sin(pianoroll_cpt*distorFactors[sp][3][1]*3.14159/32)
                                               +distorFactors[sp][3][2]*sin(pianoroll_cpt*distorFactors[sp][3][3]*3.14159/32)
                                               +distorFactors[sp][3][4]*sin(pianoroll_cpt*distorFactors[sp][3][5]*3.14159/32));
                    
                    if (sp&1) {
                        for (int ii=0;ii<6;ii++) {
                            texcoords[index+ii].u=1-texcoords[index+ii].u;
                        }
                        
                    }
                    for (int ii=0;ii<6;ii++) {
                        if (settings[GLOB_FXPianoRollSpark].detail.mdz_switch.switch_value==2) {
                            ptsB[index+ii].r=255;
                            ptsB[index+ii].g=255;
                            ptsB[index+ii].b=255;
                        } else {
                            ptsB[index+ii].r=crt;
                            ptsB[index+ii].g=cgt;
                            ptsB[index+ii].b=cbt;
                        }
                        ptsB[index+ii].a=sparkIntensity[note]/4;
                    }
                    index+=6;
                }
                
            }
        }
    glDrawArrays(GL_TRIANGLES, 0, index);
    
    //reset spark intensity if no note played
    for (int i=0;i<256;i++) {
        if (sparkPresent[i]==0) {
            if (sparkIntensity[i]>8) sparkIntensity[i]-=8;
            else sparkIntensity[i]=0;
        }
    }
    
    
    glBindTexture(GL_TEXTURE_2D,0);
    glDisable(GL_TEXTURE_2D);            /* Enable 2D Texture Mapping */
    glEnableClientState(GL_COLOR_ARRAY);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    index=0;
    //draw piano felt
    int crtp[2],cgtp[2],cbtp[2],cap[2];
    crtp[0]=60;crtp[1]=120;
    cgtp[0]=60;cgtp[1]=0;
    cbtp[0]=60;cbtp[1]=0;
    cap[0]=255;cap[1]=255;
    float wd=height/32.0;
        y=ofsy+height;
        ptsB[index++] = LineVertexF(0,y     ,crtp[0],cgtp[0],cbtp[0],cap[0]);
        ptsB[index++] = LineVertexF(ww,y    ,crtp[0],cgtp[0],cbtp[0],cap[0]);
        ptsB[index++] = LineVertexF(ww,y+wd ,crtp[1],cgtp[1],cbtp[1],cap[1]);
        
        ptsB[index++] = LineVertexF(0,y      ,crtp[0],cgtp[0],cbtp[0],cap[0]);
        ptsB[index++] = LineVertexF(ww,y+wd  ,crtp[1],cgtp[1],cbtp[1],cap[1]);
        ptsB[index++] = LineVertexF(0,y+wd   ,crtp[1],cgtp[1],cbtp[1],cap[1]);
    glDrawArrays(GL_TRIANGLES, 0, index);
    
    index=0;
    
    
    //draw white keys
    y=ofsy+0;
    for (int note=0;note<256;note++) {
        if ( note_posType[note]==0) { //white key
            x=note_posX[note];
            if (index+INDICES_SIZE_KEYW>=max_indices) {
                glDrawArrays(GL_TRIANGLES, 0, index);
                index=0;
            }
            if ((x+width>0)||(x<ww)) index=DrawKeyW(ptsB,index,x,y,width,height,border_size,220,220,220,255,0,note,0);
        }
    }
    
    //1st pass draw notes - white keys
    for (int i=0; i<256; i++) { //for each channels
        if ((data_midifx_note[midi_data_ofs][i]/*||data_midifx_note[midi_data_ofs+1][i]*/)&&
            data_midifx_vol[midi_data_ofs][i] &&
            (data_midifx_vol[midi_data_ofs][i]>=data_midifx_vol[midi_data_ofs+1][i]) ) {  //do we have a note ?
            unsigned int note=data_midifx_note[midi_data_ofs][i];
            if (!note) note=data_midifx_note[midi_data_ofs+1][i];
            
            if (note_posType[note]==0) { //white key
                
                unsigned int instr=data_midifx_instr[midi_data_ofs][i];
                unsigned int vol=data_midifx_vol[midi_data_ofs][i];
                unsigned int st=data_midifx_st[midi_data_ofs][i];
                
                int subnote=data_midifx_subnote[midi_data_ofs][i];
                subnote=(subnote<8?subnote:subnote-8-7);
                
                int colidx=instr&63;
                int crt=((data_midifx_col[colidx&31]>>16)&0xFF);
                int cgt=((data_midifx_col[colidx&31]>>8)&0xFF);
                int cbt=(data_midifx_col[colidx&31]&0xFF);
                
                if (colidx&0x20) {
                    crt=(crt+255)/2;
                    cgt=(cgt+255)/2;
                    cbt=(cbt+255)/2;
                }
                
                if (vol&&(st&VOICE_ON)) {  //check volume & status => we have something
                    x=note_posX[note];
                    
                    y=ofsy+0;
                    if (index+INDICES_SIZE_KEYW>=max_indices) {
                        glDrawArrays(GL_TRIANGLES, 0, index);
                        index=0;
                    }
                    if ( (x+width>0)||(x<ww) ) index=DrawKeyW(ptsB,index,x,y,width,height,border_size,crt,cgt,cbt,255,subnote,note,0);
                }
            }
        }
    }
    
    //draw black keys
    y=ofsy+0;
    for (int note=0;note<256;note++) {
        if (note_posType[note] ) { //black key
            yB=y+height-heightB;
            xB=note_posX[note];
            if (index+INDICES_SIZE_KEYB>=max_indices) {
                glDrawArrays(GL_TRIANGLES, 0, index);
                index=0;
            }
            if ( (xB+widthB>0)||(xB<ww) ) index=DrawKeyB(ptsB,index,xB,yB,widthB,heightB,border_size,40,40,40,255,0,note,0);
        }
    }
    
    //2nd pass draw notes - black keys
    for (int i=0; i<256; i++) { //for each channels
        if ((data_midifx_note[midi_data_ofs][i]/*||data_midifx_note[midi_data_ofs+1][i]*/)&&
            (data_midifx_vol[midi_data_ofs][i]>=data_midifx_vol[midi_data_ofs+1][i]) ) {  //do we have a note ?
            unsigned int note=data_midifx_note[midi_data_ofs][i];
            if (!note) note=data_midifx_note[midi_data_ofs+1][i];
            
            if (note_posType[note]) { //black key
                
                unsigned int instr=data_midifx_instr[midi_data_ofs][i];
                unsigned int vol=data_midifx_vol[midi_data_ofs][i];
                unsigned int st=data_midifx_st[midi_data_ofs][i];
                
                int subnote=data_midifx_subnote[midi_data_ofs][i];
                subnote=(subnote<8?subnote:subnote-8-7);
                
                int colidx=instr&63;
                int crt=((data_midifx_col[colidx&31]>>16)&0xFF);
                int cgt=((data_midifx_col[colidx&31]>>8)&0xFF);
                int cbt=(data_midifx_col[colidx&31]&0xFF);
                
                if (colidx&0x20) {
                    crt=(crt+255)/2;
                    cgt=(cgt+255)/2;
                    cbt=(cbt+255)/2;
                }
                
                if (vol&&(st&VOICE_ON)) {  //check volume & status => we have something
                    //                    printf("B instr %d note %d note%%12 %d type %d x %f\n",instr,note,note%12,note_posType[note],x);
                    x=note_posX[note];
                    y=ofsy+height-heightB+0;
                    if (index+INDICES_SIZE_KEYB>=max_indices) {
                        glDrawArrays(GL_TRIANGLES, 0, index);
                        index=0;
                    }
                    if ( (x+widthB>0)||(x<ww) )  index=DrawKeyB(ptsB,index,x,y,widthB,heightB,border_size,crt,cgt,cbt,255,subnote,note,0);
                }
            }
        }
    }
    
    
    
    memset(voices_posX,0,sizeof(voices_posX));
    
    //draw label small colored boxes
    if (voices_label&&settings[GLOB_FXPianoRollVoicesLabels].detail.mdz_switch.switch_value)
        x=16;
    y=ofsy-16+4;
    for (int i=0;i<m_genNumMidiVoicesChannels;i++) {
        int j=i;
        
        if (mVoicesNamePiano[i]) {
            float widthx=16+mOscilloFont[1]->maxCharWidth*strlen(mVoicesNamePiano[i]->mText)/mScaleFactor;
            
            int colidx=i&63;
            int crt=((data_midifx_col[colidx&31]>>16)&0xFF);
            int cgt=((data_midifx_col[colidx&31]>>8)&0xFF);
            int cbt=(data_midifx_col[colidx&31]&0xFF);
            
            if (colidx&0x20) {
                crt=(crt+255)/2;
                cgt=(cgt+255)/2;
                cbt=(cbt+255)/2;
            }
            
            if (x+widthx>ww) {
                x=16;
                y-=16;
            }
            if (index+INDICES_SIZE_BOX>=max_indices) {
                glDrawArrays(GL_TRIANGLES, 0, index);
                index=0;
            }
            index=DrawBox(ptsB,index,x-10,y,8,8,1/*border_size*/,crt,cgt,cbt,255,0);
            
            x+=widthx;
        }
    }
    
    glDrawArrays(GL_TRIANGLES, 0, index);
    
    
    //Draw light fx
    //glDisable(GL_DEPTH_TEST);           /* Disable Depth Testing     */
    glEnable(GL_TEXTURE_2D);            /* Enable 2D Texture Mapping */
    glBindTexture(GL_TEXTURE_2D, txt_pianoRoll[TXT_PIANOROLL_LIGHT]);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE);
    
    
    glTexCoordPointer(2, GL_FLOAT, 0, texcoords);
    /* Enable Texture Coordinations Pointer */
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    
    memset(sparkPresent,0,sizeof(sparkPresent));
    index=0;
    if (settings[GLOB_FXPianoRollSpark].detail.mdz_switch.switch_value)
        for (int i=0; i<256; i++) { //for each channels
            if ((data_midifx_note[midi_data_ofs][i]/*||data_midifx_note[midi_data_ofs+1][i]*/)&&
                (data_midifx_vol[midi_data_ofs][i]>=data_midifx_vol[midi_data_ofs+1][i]) ) {  //do we have a note ?
                unsigned int note=data_midifx_note[midi_data_ofs][i];
                if (!note) note=data_midifx_note[midi_data_ofs+1][i];
                
                //avoid rendering twice for same note
                if (sparkPresent[note]) continue;
                sparkPresent[note]=1;
                
                unsigned int instr=data_midifx_instr[midi_data_ofs][i];
                int colidx=instr&63;
                int crt=((data_midifx_col[colidx&31]>>16)&0xFF);
                int cgt=((data_midifx_col[colidx&31]>>8)&0xFF);
                int cbt=(data_midifx_col[colidx&31]&0xFF);
                
                if (colidx&0x20) {
                    crt=(crt+255)/2;
                    cgt=(cgt+255)/2;
                    cbt=(cbt+255)/2;
                }
                
                crt=(crt*3+255*3)/6;
                cgt=(cgt*3+255*3)/6;
                cbt=(cbt*3+255*3)/6;
                
                if (crt>255) crt=255;
                if (cgt>255) cgt=255;
                if (cbt>255) cbt=255;
                
                line_width_extra=2;
                
                float posNote;
                float wd;
                posNote=note_posX[note];
                if (note_posType[note]==0) wd=width+line_width_extra*2;
                else wd=widthB+line_width_extra*2;
                
                //if (wd>=3) wd-=2;
                //posNote+=2;
                posNote-=wd*1.0;
                wd=wd*3;
                
                for (int sp=0;sp<4;sp++) {
                    
                    texcoords[index+0].u=0.0f; texcoords[index+0].v=128.0/128;
                    texcoords[index+1].u=0.0f; texcoords[index+1].v=0.0/128;
                    texcoords[index+2].u=1.0f; texcoords[index+2].v=128.0/128;
                    
                    texcoords[index+3].u=0.0f; texcoords[index+3].v=0.0/128;
                    texcoords[index+4].u=1.0f; texcoords[index+4].v=128.0/128;
                    texcoords[index+5].u=1.0f; texcoords[index+5].v=0.0/128;
                    
                    
                    ptsB[index+0].x=posNote;ptsB[index+0].y=ofsy+0+height-wd*3/4+height/32+height/16;
                    ptsB[index+1].x=posNote;ptsB[index+1].y=ofsy+0+height+wd/2+height/32+height/16;
                    ptsB[index+2].x=posNote+wd;ptsB[index+2].y=ofsy+0+height-wd*3/4+height/32+height/16;
                    
                    ptsB[index+3].x=posNote;ptsB[index+3].y=ofsy+0+height+wd/2+height/32+height/16;
                    ptsB[index+4].x=posNote+wd;ptsB[index+4].y=ofsy+0+height-wd*3/4+height/32+height/16;
                    ptsB[index+5].x=posNote+wd;ptsB[index+5].y=ofsy+0+height+wd/2+height/32+height/16;
                    
                    //apply some distortion
                    float wd_distX=wd/3.0;
                    float wd_distY=wd/3.0;
                    
                    float distorFactors[4][4][6]={
                        {   {+0.7,  3 ,+0.2,  5, -0.3, 11},
                            {+0.2,  3 ,+0.5,  5, -0.4, 5},
                            
                            {+0.5,  5 ,-0.1,  7, +0.4, 13},
                            {+0.3,  2 ,-0.5,  3, +0.2, 3}},
                        
                        {   {+0.5,  1 ,+0.2,  7, -0.3, 7},
                            {+0.3,  3 ,+0.5,  2, -0.4, 7},
                            
                            {-0.3,  7 ,-0.1,  9, +0.4, 5},
                            {-0.2,  5 ,-0.5,  5, +0.2, 11}},
                        
                        {   {-0.7,  2 ,+0.2,  11, -0.3, 9},
                            {+0.2,  5 ,+0.5,  13, +0.7, 5},
                            
                            {+0.6,  9 ,-0.1,  3, +0.4, 3},
                            {+0.4,  1 ,-0.5,  3, +0.2, 8}},
                        
                        {   {+0.8,  9 ,+0.2,  7, -0.3, 9},
                            {+0.4,  11 ,+0.5, 7, +0.4, 5},
                            
                            {-0.5,  3 ,+0.1,  4, -0.4, 11},
                            {-0.3,  5 ,+0.5,  5, +0.2, 3}}};
                    
                    ptsB[index+1].x+=wd_distX*(distorFactors[sp][0][0]*sin(pianoroll_cpt*distorFactors[sp][0][1]*3.14159/32)
                                               +distorFactors[sp][0][2]*sin(pianoroll_cpt*distorFactors[sp][0][3]*3.14159/32)
                                               +distorFactors[sp][0][4]*sin(pianoroll_cpt*distorFactors[sp][0][5]*3.14159/32));
                    
                    ptsB[index+3].x+=wd_distX*(distorFactors[sp][0][0]*sin(pianoroll_cpt*distorFactors[sp][0][1]*3.14159/32)
                                               +distorFactors[sp][0][2]*sin(pianoroll_cpt*distorFactors[sp][0][3]*3.14159/32)
                                               +distorFactors[sp][0][4]*sin(pianoroll_cpt*distorFactors[sp][0][5]*3.14159/32));
                    
                    ptsB[index+1].y+=wd_distY*(distorFactors[sp][1][0]*sin(pianoroll_cpt*distorFactors[sp][1][1]*3.14159/32)
                                               +distorFactors[sp][1][2]*sin(pianoroll_cpt*distorFactors[sp][1][3]*3.14159/32)
                                               +distorFactors[sp][1][4]*sin(pianoroll_cpt*distorFactors[sp][1][5]*3.14159/32));
                    
                    ptsB[index+3].y+=wd_distY*(distorFactors[sp][1][0]*sin(pianoroll_cpt*distorFactors[sp][1][1]*3.14159/32)
                                               +distorFactors[sp][1][2]*sin(pianoroll_cpt*distorFactors[sp][1][3]*3.14159/32)
                                               +distorFactors[sp][1][4]*sin(pianoroll_cpt*distorFactors[sp][1][5]*3.14159/32));
                    
                    ptsB[index+5].x+=wd_distX*(distorFactors[sp][2][0]*sin(pianoroll_cpt*distorFactors[sp][2][1]*3.14159/32)
                                               +distorFactors[sp][2][2]*sin(pianoroll_cpt*distorFactors[sp][2][3]*3.14159/32)
                                               +distorFactors[sp][2][4]*sin(pianoroll_cpt*distorFactors[sp][2][5]*3.14159/32));
                    
                    ptsB[index+5].y+=wd_distY*(distorFactors[sp][3][0]*sin(pianoroll_cpt*distorFactors[sp][3][1]*3.14159/32)
                                               +distorFactors[sp][3][2]*sin(pianoroll_cpt*distorFactors[sp][3][3]*3.14159/32)
                                               +distorFactors[sp][3][4]*sin(pianoroll_cpt*distorFactors[sp][3][5]*3.14159/32));
                    
                    for (int ii=0;ii<6;ii++) {
                        if (settings[GLOB_FXPianoRollSpark].detail.mdz_switch.switch_value==2) {
                            ptsB[index+ii].r=255;
                            ptsB[index+ii].g=255;
                            ptsB[index+ii].b=255;
                        } else {
                            ptsB[index+ii].r=crt;
                            ptsB[index+ii].g=cgt;
                            ptsB[index+ii].b=cbt;
                        }
                        ptsB[index+ii].a=sparkIntensity[note]/4;
                    }
                    index+=6;
                }
                
            }
        }
    glDrawArrays(GL_TRIANGLES, 0, index);
    
    glBindTexture(GL_TEXTURE_2D,0);
    glDisable(GL_TEXTURE_2D);            /* Enable 2D Texture Mapping */
    glEnableClientState(GL_COLOR_ARRAY);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    index=0;
    
    
    //////////////////////////////////////////////
    
    //    glEnable(GL_BLEND);
    //    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisable(GL_BLEND);
    
    memset(voices_posX,0,sizeof(voices_posX));
    
    //draw label
    if (voices_label&&settings[GLOB_FXPianoRollVoicesLabels].detail.mdz_switch.switch_value) {
        y=ofsy-16;//height+16;
        x=16;
        for (int i=0;i<m_genNumMidiVoicesChannels;i++) {
            int j=i;
            
            if (mVoicesNamePiano[i]) {
                float widthx=16+mOscilloFont[1]->maxCharWidth*strlen(mVoicesNamePiano[i]->mText)/mScaleFactor;
                if (x+widthx>ww) {
                    x=16;
                    y-=16;
                }
                
                glPushMatrix();
                glTranslatef(x,y+3, 0.0f);
                mVoicesNamePiano[i]->Render(255);
                x+=widthx;
                glPopMatrix();
            }
        }
    }
    
    if (mOctavesIndex[0]&&settings[GLOB_FXPianoRollOctavesLabels].detail.mdz_switch.switch_value) {
            for (int o=0;o<256/12;o++) {
                x=o*width*7.0-note_display_offset;
                
                if (mOctavesIndex[o]) {
                    
                    if (pianoroll_key_status[0][o*12]&PR_KEY_PRESSED) y=ofsy+0+3+height/24;
                    else y=ofsy+0+3+height/8;
                    
                    glPushMatrix();
                    
                    float lblwidth=strlen(mOctavesIndex[o]->mText)*mOscilloFont[1]->maxCharWidth/mScaleFactor;
                    x+=(width-lblwidth)/2;
                    
                    glTranslatef(x,y, 0.0f);
                    mOctavesIndex[o]->Render(32);
                    
                    glPopMatrix();
                }
            }
    }
    free(ptsB);
    free(texcoords);
#endif
}
