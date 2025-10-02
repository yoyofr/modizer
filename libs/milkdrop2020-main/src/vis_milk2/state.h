/*
  LICENSE
  -------
Copyright 2005-2013 Nullsoft, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice,
	this list of conditions and the following disclaimer. 

  * Redistributions in binary form must reproduce the above copyright notice,
	this list of conditions and the following disclaimer in the documentation
	and/or other materials provided with the distribution. 

  * Neither the name of Nullsoft nor the names of its contributors may be used to 
	endorse or promote products derived from this software without specific prior written permission. 
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR 
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND 
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR 
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT 
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _MILKDROP_STATE_
#define _MILKDROP_STATE_ 1

#include <string>
#include "platform.h"
#include "render/MDcontext.h"
#include "script/script.h"

#define MAX_CUSTOM_WAVES  4
#define MAX_CUSTOM_SHAPES 4
#define NUM_WAVES         10

#define NUM_Q_VAR 32
#define NUM_T_VAR 8

class PresetReader;
class PresetWriter;

class CState;
using CStatePtr = std::shared_ptr<CState>;

enum class CStateDebugPanel
{
    None,
    State,
    General,
    Wave,
    Motion,
    ShaderWarp,
    ShaderComp,
    Wave0,
    Wave1,
    Wave2,
    Wave3,
    Shape0,
    Shape1,
    Shape2,
    Shape3,
};


class CBlendableFloat
{
public:
	CBlendableFloat();
	~CBlendableFloat();

	float operator = (float f) { 
		val = f; 
		m_bBlending = false; 
		return val;
	};
	float operator *= (float f) { 
		val *= f; 
		m_bBlending = false; 
		return val;
	};
	float operator /= (float f) { 
		val /= f; 
		m_bBlending = false; 
		return val;
	};
	float operator -= (float f) { 
		val -= f; 
		m_bBlending = false; 
		return val;
	};
	float operator += (float f) { 
		val += f; 
		m_bBlending = false; 
		return val;
	};

	float eval(float fTime);	// call this from animation code.  if fTime < 0, it will return unblended 'val'.
	void  StartBlendFrom(CBlendableFloat *f_from, float fAnimTime, float fDuration);

    float *GetPtr() {return &val;}
    
protected:
	float val;
	bool  m_bBlending;
	float m_fBlendStartTime;
	float m_fBlendDuration;
	float m_fBlendFrom;
};

struct CommonVars
{
    Script::Var time;
    Script::Var frame;
    Script::Var fps;
    Script::Var progress;
    Script::Var bass;
    Script::Var mid;
    Script::Var treb;
    Script::Var bass_att;
    Script::Var mid_att;
    Script::Var treb_att;
    Script::Vector2 meshxy;
    Script::Vector2 pixelsxy;
    Script::Vector2 aspectxy;
    

    void RegisterStruct(Script::IContextPtr context)
    {
        context->RegVar("time", time);        // i
        context->RegVar("fps", fps);        // i
        context->RegVar("frame", frame);     // i
        context->RegVar("progress", progress);  // i
        context->RegVar("bass", bass);        // i
        context->RegVar("mid", mid);        // i
        context->RegVar("treb", treb);        // i
        context->RegVar("bass_att", bass_att);    // i
        context->RegVar("mid_att", mid_att);    // i
        context->RegVar("treb_att", treb_att);    // i
        context->RegVar("mesh", meshxy);  // i
        context->RegVar("pixels", pixelsxy);  // i
        context->RegVar("aspect", aspectxy);  // i

    }
};

class Component
{
public:
    inline Component() = default;
    virtual ~Component() = default;

    virtual const std::string &GetName() const = 0;
//    virtual void ShowDebugUI() = 0;

    double time_compute_begin = 0.0f;
    double time_compute_end = 0.0f;

};

class CShape : public Component
{
public:
    CShape(int index)
    : index(index)
    {
        name = "shape" + std::to_string(index);
    }
    
    virtual ~CShape() = default;

	int  Import(PresetReader &pr);
    void Export(PresetWriter &pw);
	int  Export(FILE* f, const char* szFile);
    void LoadCustomShapePerFrameEvallibVars(class CState *pState);
    void RegisterVars();
    void InitExpressions(CState * pState);

    virtual const std::string &GetName() const override {return name;}
    void ShowDebugUI();
    virtual void Dump()  {}

    void Draw_ComputeBegin(CState *pState);
    void Draw_ComputeEnd(class CPlugin *plugin, CState * pState, float alpha_mult);

    std::string name;
    int index = 0;

    bool enabled = false;
    int sides   = 4;
    int additive = 0;
    int thickOutline = 0;
    int textured = 0;
    int instances = 1;
    float tex_zoom = 1.0f;
    float tex_ang  = 0.0f;
    float x = 0.5f;
    float y = 0.5f;
    float rad = 0.1f;
    float ang = 0.0f;
    Color4F rgba = Color4F(1.0f, 0.0f, 0.0f, 1.0f);
    Color4F rgba2 = Color4F(0.0f, 1.0f, 0.0f, 0.0f);
    Color4F border = Color4F(1.0f, 1.0f, 1.0f, 0.1f);


	std::string  m_szInit; // note: only executed once -> don't need to save codehandle
	std::string  m_szPerFrame;
		
	// for per-frame expression evaluation:
    Script::IContextPtr	m_perframe_context;
    Script::IKernelPtr m_perframe_init;
    Script::IKernelPtr m_perframe_expression;
    Script::IKernelBufferPtr m_perframe_buffer;

    CommonVars var_pf_common;
	Script::Var var_pf_q[NUM_Q_VAR];
	Script::Var var_pf_t[NUM_T_VAR];
	Script::Color var_pf_rgba;
    Script::Color var_pf_rgba2;
    Script::Color var_pf_border;
    Script::Var var_pf_x;
    Script::Var var_pf_y;
	Script::Var var_pf_rad, var_pf_ang;
	Script::Var var_pf_sides, var_pf_textured, var_pf_additive, var_pf_thick, var_pf_instances, var_pf_instance;
	Script::Var var_pf_tex_zoom, var_pf_tex_ang;

    Script::ValueType t_values_after_init_code[NUM_T_VAR] = {0};
};
using CShapePtr = CShape *;

class CWave: public Component
{
public:
    CWave(int index)
    : index(index)
    {
        name = "wave" + std::to_string(index);
    }

    virtual ~CWave() = default;
	int  Import(PresetReader &pr);
    void Export(PresetWriter &pw);
	int  Export(FILE* f, const char* szFile);
    void LoadCustomWavePerFrameEvallibVars(CState * pState);
    void RegisterVars();
    void InitExpressions(CState * pState);

    virtual const std::string &GetName() const override {return name;}
    void ShowDebugUI();
    void Dump();

    //void Draw(class CPlugin *plugin, CState * pState, float alpha_mult);
    void Draw_ComputeBegin(class CPlugin *plugin, CState * pState);
    void Draw_ComputeEnd(class CPlugin *plugin, CState * pState, float alpha_mult);

    std::string name;
    int index = 0;
    bool enabled = false;
    int samples = 512;
    int sep = 0;
    float scaling = 1.0f;
    float smoothing = 0.5f;
    Color4F rgba = Color4F(1.0f, 1.0f, 1.0f, 1.0f);
    int bSpectrum = 0;
    int bUseDots = 0;
    int bDrawThick = 0;
    int bAdditive = 0;

	std::string  m_szInit; // note: only executed once -> don't need to save codehandle
	std::string  m_szPerFrame;
	std::string  m_szPerPoint;

	// for per-frame expression evaluation:
    Script::IContextPtr	m_perframe_context;
    Script::IKernelPtr m_perframe_init;
    Script::IKernelPtr m_perframe_expression;
    CommonVars var_pf_common;

	Script::Var var_pf_q[NUM_Q_VAR];
	Script::Var var_pf_t[NUM_T_VAR];
	Script::Color var_pf_rgba;
	Script::Var var_pf_samples;

	// for per-point expression evaluation:
    Script::IContextPtr	m_perpoint_context;
    Script::IKernelPtr m_perpoint_expression;
    Script::IKernelBufferPtr m_perpoint_buffer;
    CommonVars var_pp_common;
	Script::Var var_pp_q[NUM_Q_VAR];
	Script::Var var_pp_t[NUM_T_VAR];
    
   
	Script::ValueType t_values_after_init_code[NUM_T_VAR]  = {0};

    Script::Var var_pp_sample;
    Script::Var var_pp_value1;
    Script::Var var_pp_value2;
    Script::Var var_pp_x;
    Script::Var var_pp_y;
    Script::Color var_pp_rgba;
    
    
};
using CWavePtr = CWave *;

struct TexSizeParamInfo
{
	std::string    texname;  // just for ref
	render::ShaderConstantPtr texsize_param;
	int        w, h;
};

typedef enum { TEX_DISK, TEX_VS, TEX_BLUR1, TEX_BLUR2, TEX_BLUR3, TEX_UNKNOWN } tex_code;

struct SamplerInfo
{
	render::TexturePtr          texptr;
    render::ShaderSamplerPtr    shader_sampler;
    std::string         name;
    tex_code            texcode = TEX_UNKNOWN;  // if ==TEX_VS, forget the pointer - texture bound @ that stage is the double-buffered VS.

	bool               bBilinear = true;
	bool               bWrap = true;
};


class ShaderInfo // : public RefCounted
{
public:
	render::ShaderPtr	shader;
    double      compileTime = 0.0f;

	// float4 handles:
	render::ShaderConstantPtr rand_frame;
	render::ShaderConstantPtr rand_preset;
	render::ShaderConstantPtr const_handles[24];
	render::ShaderConstantPtr q_const_handles[(NUM_Q_VAR + 3) / 4];
	render::ShaderConstantPtr rot_mat[24];

	typedef std::vector<TexSizeParamInfo> TexSizeParamInfoList;
	TexSizeParamInfoList texsize_params;
    
    int             highest_blur_used = 0;

	// sampler stages for various PS texture bindings:
	//int texbind_vs;
	//int texbind_disk[32];
	//int texbind_voronoi;
	//...
    std::vector<SamplerInfo>   _texture_bindings;  // an entry for each sampler slot.  These are ALIASES - DO NOT DELETE.

    void ApplyShaderParams(class CPlugin *plugin, CStatePtr pState);
    
	void CacheParams(class CPlugin *plugin);
};

using ShaderInfoPtr = std::shared_ptr<ShaderInfo>;


struct WarpVertex
{
    float tu, tv;
};

#define CUR_MILKDROP_PRESET_VERSION 201
// 200: milkdrop 2
// 201: instead of just 1 variable for shader version, it tracks 2 (comp and warp) separately.
class CPlugin;
class CState
{
public:
	CState(CPlugin *plugin);
	virtual ~CState();

	void Default();
	void Randomize(int nMode);
	void StartBlendFrom(CState *s_from, float fAnimTime, float fTimespan);
    bool ImportFromFile(std::string path, std::string name, std::string &errors);
    bool ImportFromText(std::string text, std::string name, std::string &errors);
	bool Export(std::string szIniFile);
    
    void Export(PresetWriter &pw);
	void RecompileExpressions();
    bool RecompileShaders(std::string &errors);
	void GenDefaultWarpShader();
	void GenDefaultCompShader();    
    void LoadPerFrameEvallibVars();
    void RunPerFrameEquations();
    
    void DrawMotionVectors();
    void DrawCustomShapes_ComputeBegin();
    void DrawCustomShapes_ComputeEnd(float alpha_mult);
    
    void DrawCustomWaves_ComputeBegin();
    void DrawCustomWaves_ComputeEnd(float alpha_mult);

    void ComputeGridAlphaValues_ComputeBegin();
    void ComputeGridAlphaValues_ComputeEnd();

    const std::string &GetName() const {return m_name;}
    void ShowDebugUI( CStateDebugPanel mode);

    void DebugUI(bool *open);
    void Dump();

    int GetHighestBlurTexUsed();
    void        GetSafeBlurMinMax(float* blur_min, float* blur_max);

    bool UsesCompShader() const { return m_nCompPSVersion > 0;}
    bool UsesWarpShader() const { return m_nWarpPSVersion > 0;}


    float frand();
    
    float time_import = 0.0f;
    
	CPlugin * m_plugin;

	std::string m_name;		// this is just the filename, without a path or extension.
    
	ShaderInfoPtr m_shader_warp;
	ShaderInfoPtr m_shader_comp;
    
    
    // vertex data written by ComputeGridAlphaValues
    std::vector<WarpVertex>     m_verts;
    

	int                 m_nMinPSVersion;  // the min of the warp & comp values...
	int                 m_nMaxPSVersion;  // the max of the warp & comp values...
	int                 m_nWarpPSVersion;  // 0 = milkdrop 1 era (no PS), 2 = ps_2_0, 3 = ps_3_0
	int                 m_nCompPSVersion;  // 0 = milkdrop 1 era (no PS), 2 = ps_2_0, 3 = ps_3_0
	float				m_fRating;		// 0..5
	// post-processing:
	CBlendableFloat		m_fGammaAdj;	// +0 -> +1.0 (double), +2.0 (triple)...
	CBlendableFloat		m_fVideoEchoZoom;
	CBlendableFloat 	m_fVideoEchoAlpha;
	float				m_fVideoEchoAlphaOld;
	int					m_nVideoEchoOrientation;
	int					m_nVideoEchoOrientationOld;

	// fps-dependant:
	CBlendableFloat		m_fDecay;			// 1.0 = none, 0.95 = heavy decay

	// other:
	int					m_nWaveMode;
	int					m_nOldWaveMode;
	bool				m_bAdditiveWaves;
	CBlendableFloat		m_fWaveAlpha;
	CBlendableFloat		m_fWaveScale;
	CBlendableFloat		m_fWaveSmoothing;	
	bool				m_bWaveDots;
	bool                m_bWaveThick;
	CBlendableFloat		m_fWaveParam;		// -1..1; 0 is normal
	bool				m_bModWaveAlphaByVolume;
	CBlendableFloat		m_fModWaveAlphaStart;	// when relative volume hits this level, alpha -> 0
	CBlendableFloat		m_fModWaveAlphaEnd;		// when relative volume hits this level, alpha -> 1
	float				m_fWarpAnimSpeed;	// 1.0 = normal, 2.0 = double, 0.5 = half, etc.
	CBlendableFloat		m_fWarpScale;
	CBlendableFloat		m_fZoomExponent;
	CBlendableFloat		m_fShader;			// 0 = no color shader, 1 = full color shader
	bool				m_bMaximizeWaveColor;
	bool				m_bTexWrap;
	bool				m_bDarkenCenter;
	bool				m_bRedBlueStereo;
	bool				m_bBrighten;
	bool				m_bDarken;
	bool				m_bSolarize;
	bool				m_bInvert;


	// map controls:
	CBlendableFloat		m_fZoom;
	CBlendableFloat		m_fRot;	
	CBlendableFloat		m_fRotCX;	
	CBlendableFloat		m_fRotCY;	
	CBlendableFloat		m_fXPush;
	CBlendableFloat		m_fYPush;
	CBlendableFloat		m_fWarpAmount;
	CBlendableFloat		m_fStretchX;
	CBlendableFloat		m_fStretchY;
	CBlendableFloat		m_fWaveR;
	CBlendableFloat		m_fWaveG;
	CBlendableFloat		m_fWaveB;
	CBlendableFloat		m_fWaveX;
	CBlendableFloat		m_fWaveY;
	CBlendableFloat		m_fOuterBorderSize;
	CBlendableFloat		m_fOuterBorderR;
	CBlendableFloat		m_fOuterBorderG;
	CBlendableFloat		m_fOuterBorderB;
	CBlendableFloat		m_fOuterBorderA;
	CBlendableFloat		m_fInnerBorderSize;
	CBlendableFloat		m_fInnerBorderR;
	CBlendableFloat		m_fInnerBorderG;
	CBlendableFloat		m_fInnerBorderB;
	CBlendableFloat		m_fInnerBorderA;
	CBlendableFloat		m_fMvX;
	CBlendableFloat		m_fMvY;
	CBlendableFloat		m_fMvDX;
	CBlendableFloat		m_fMvDY;
	CBlendableFloat		m_fMvL;
	CBlendableFloat		m_fMvR;
	CBlendableFloat		m_fMvG;
	CBlendableFloat		m_fMvB;
	CBlendableFloat		m_fMvA;
	CBlendableFloat		m_fBlur1Min;
	CBlendableFloat		m_fBlur2Min;
	CBlendableFloat		m_fBlur3Min;
	CBlendableFloat		m_fBlur1Max;
	CBlendableFloat		m_fBlur2Max;
	CBlendableFloat		m_fBlur3Max;
	CBlendableFloat		m_fBlur1EdgeDarken;

    std::vector<CShape *>              m_shapes;
    std::vector<CWave *>               m_waves;
	
	// some random stuff for driving shaders:
	void         RandomizePresetVars();
	Vector4      m_rand_preset; // 4 random floats (0..1); randomized @ preset load; fed to pixel shaders.  --FIXME (blending)
	Vector3  m_xlate[20];
	Vector3  m_rot_base[20];
	Vector3  m_rot_speed[20];
    

	// for arbitrary function evaluation:
	std::string		m_szPerFrameInit;
	std::string		m_szPerFrameExpr;
	std::string		m_szPerPixelExpr;
	std::string     m_szWarpShadersText; // pixel shader code
	std::string     m_szCompShadersText; // pixel shader code
	void			FreeScriptObjects();
	void			RegisterBuiltInVariables();
    void GenerateCode(std::string outputDir);


	// for once-per-frame expression evaluation: [although, these vars are also shared w/preset init expr eval]
    Script::IContextPtr m_perframe_context;
    Script::IKernelPtr   m_perframe_init;
    Script::IKernelPtr	m_perframe_expression;
    
    
    CommonVars var_pf_common;
    const CommonVars &GetCommonVars() const  {return var_pf_common;}

    Script::Var var_pf_zoom, var_pf_zoomexp, var_pf_rot, var_pf_warp;
    Script::Vector2 var_pf_cxy;
    Script::Vector2 var_pf_dxy;
    Script::Vector2 var_pf_sxy;
    Script::Color var_pf_wave_rgba;
    Script::Var var_pf_wave_x, var_pf_wave_y, var_pf_wave_mystery, var_pf_wave_mode;
	Script::Var var_pf_decay;
	Script::Var var_pf_q[NUM_Q_VAR];
    Script::Var var_pf_ob_size;
    Script::Color var_pf_ob_rgba;
    Script::Var var_pf_ib_size;
    Script::Color var_pf_ib_rgba;
	Script::Vector2 var_pf_mv_xy;
	Script::Vector2 var_pf_mv_dxy;
	Script::Var var_pf_mv_l;
    Script::Color var_pf_mv_rgba;
	Script::Var var_pf_monitor;
	Script::Var var_pf_echo_zoom, var_pf_echo_alpha, var_pf_echo_orient;
	// new in v1.04:
	Script::Var var_pf_wave_usedots, var_pf_wave_thick, var_pf_wave_additive, var_pf_wave_brighten;
	Script::Var var_pf_darken_center, var_pf_gamma, var_pf_wrap;
	Script::Var var_pf_invert, var_pf_brighten, var_pf_darken, var_pf_solarize;
	Script::Var var_pf_blur1min;
	Script::Var var_pf_blur2min;
	Script::Var var_pf_blur3min;
	Script::Var var_pf_blur1max;
	Script::Var var_pf_blur2max;
	Script::Var var_pf_blur3max;
	Script::Var var_pf_blur1_edge_darken;
	
	// for per-vertex expression evaluation:
    Script::IContextPtr	m_pervertex_context;
    Script::IKernelPtr	m_pervertex_expression;
    Script::IKernelBufferPtr    m_pervertex_buffer;

    CommonVars var_pv_common;
    Script::Var var_pv_zoom, var_pv_zoomexp, var_pv_rot, var_pv_warp;
    
    Script::Vector2 var_pv_cxy;
    Script::Vector2 var_pv_dxy;
    Script::Vector2 var_pv_sxy;
    Script::Vector2 var_pv_xy;
    Script::Var var_pv_rad, var_pv_ang;
	Script::Var var_pv_q[NUM_Q_VAR];


    
	Script::ValueType q_values_after_init_code[NUM_Q_VAR];
	Script::ValueType monitor_after_init_code;
};


using CStatePtr = std::shared_ptr<CState>;

#endif
