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

#include "plugin.h"
#include "render/MDcontext.h"
#include "render/DrawBuffer.h"

using namespace render;

//#define CosineInterp(x) (0.5f - 0.5f*cosf((x) * M_PI))

inline float CosineInterp(float x)
{
    return (0.5f - 0.5f*cosf((x) * M_PI));
}

//#define COLOR_RGBA_01(r,g,b,a) COLOR_RGBA(((int)(r*255)),((int)(g*255)),((int)(b*255)),((int)(a*255)))



#define COLOR_RGBA_01(r,g,b,a) Color4F::ToU32(r,g,b,a)
#define FRAND frand()
//
//
//static CommonVars GetCommonVars(CPlugin *plugin)
//{
//    CommonVars common;
//    common.time        = (double)plugin->GetTime();
//    common.frame       = (double)plugin->GetFrame();
//    common.fps         = (double)plugin->GetFps();
//    common.progress    = (double)plugin->GetPresetProgress();
//    common.bass        = (double)plugin->GetImmRel(BAND_BASS);
//    common.mid         = (double)plugin->GetImmRel(BAND_MID);
//    common.treb        = (double)plugin->GetImmRel(BAND_TREBLE);
//    common.bass_att    = (double)plugin->GetAvgRel(BAND_BASS);
//    common.mid_att     = (double)plugin->GetAvgRel(BAND_MID);
//    common.treb_att    = (double)plugin->GetAvgRel(BAND_TREBLE);
//    return common;
//}





float CPlugin::GetPresetProgress()
{
    return  m_PresetDuration / m_NextPresetDuration;
}

void CState::LoadPerFrameEvallibVars()
{
    float time = m_plugin->GetTime();
    
	// load the 'var_pf_*' variables in this CState object with the correct values.
	// for vars that affect pixel motion, that means evaluating them at time==-1,
	//    (i.e. no blending w/blendto value); the blending of the file dx/dy 
	//    will be done *after* execution of the per-vertex code.
	// for vars that do NOT affect pixel motion, evaluate them at the current time,
	//    so that if they're blending, both states see the blended value.

	// 1. vars that affect pixel motion: (eval at time==-1)
	var_pf_zoom		= (double)m_fZoom.eval(-1);//GetTime());
	var_pf_zoomexp		= (double)m_fZoomExponent.eval(-1);//GetTime());
	var_pf_rot			= (double)m_fRot.eval(-1);//GetTime());
	var_pf_warp		= (double)m_fWarpAmount.eval(-1);//GetTime());
	var_pf_cxy.x			= (double)m_fRotCX.eval(-1);//GetTime());
	var_pf_cxy.y			= (double)m_fRotCY.eval(-1);//GetTime());
	var_pf_dxy.x			= (double)m_fXPush.eval(-1);//GetTime());
	var_pf_dxy.y			= (double)m_fYPush.eval(-1);//GetTime());
	var_pf_sxy.x			= (double)m_fStretchX.eval(-1);//GetTime());
	var_pf_sxy.y			= (double)m_fStretchY.eval(-1);//GetTime());
	// read-only:
    var_pf_common = m_plugin->GetCommonVars();
	//var_pf_monitor     = 0;   -leave this as it was set in the per-frame INIT code!
	for (int vi=0; vi<NUM_Q_VAR; vi++)
		var_pf_q[vi]	= q_values_after_init_code[vi];//0.0f;
	var_pf_monitor     = monitor_after_init_code;

	// 2. vars that do NOT affect pixel motion: (eval at time==now)
	var_pf_decay		= (double)m_fDecay.eval(time);
	var_pf_wave_rgba.a		= (double)m_fWaveAlpha.eval(time);
	var_pf_wave_rgba.r		= (double)m_fWaveR.eval(time);
	var_pf_wave_rgba.g		= (double)m_fWaveG.eval(time);
	var_pf_wave_rgba.b		= (double)m_fWaveB.eval(time);
	var_pf_wave_x		= (double)m_fWaveX.eval(time);
	var_pf_wave_y		= (double)m_fWaveY.eval(time);
	var_pf_wave_mystery= (double)m_fWaveParam.eval(time);
	var_pf_wave_mode   = (double)m_nWaveMode;	//?!?! -why won't it work if set to m_nWaveMode???
	var_pf_ob_size		= (double)m_fOuterBorderSize.eval(time);
	var_pf_ob_rgba.r		= (double)m_fOuterBorderR.eval(time);
	var_pf_ob_rgba.g		= (double)m_fOuterBorderG.eval(time);
	var_pf_ob_rgba.b		= (double)m_fOuterBorderB.eval(time);
	var_pf_ob_rgba.a		= (double)m_fOuterBorderA.eval(time);
	var_pf_ib_size		= (double)m_fInnerBorderSize.eval(time);
	var_pf_ib_rgba.r		= (double)m_fInnerBorderR.eval(time);
	var_pf_ib_rgba.g		= (double)m_fInnerBorderG.eval(time);
	var_pf_ib_rgba.b		= (double)m_fInnerBorderB.eval(time);
	var_pf_ib_rgba.a		= (double)m_fInnerBorderA.eval(time);
	var_pf_mv_xy.x        = (double)m_fMvX.eval(time);
	var_pf_mv_xy.y        = (double)m_fMvY.eval(time);
	var_pf_mv_dxy.x       = (double)m_fMvDX.eval(time);
	var_pf_mv_dxy.y       = (double)m_fMvDY.eval(time);
	var_pf_mv_l        = (double)m_fMvL.eval(time);
	var_pf_mv_rgba.r        = (double)m_fMvR.eval(time);
	var_pf_mv_rgba.g        = (double)m_fMvG.eval(time);
	var_pf_mv_rgba.b        = (double)m_fMvB.eval(time);
	var_pf_mv_rgba.a        = (double)m_fMvA.eval(time);
	var_pf_echo_zoom   = (double)m_fVideoEchoZoom.eval(time);
	var_pf_echo_alpha  = (double)m_fVideoEchoAlpha.eval(time);
	var_pf_echo_orient = (double)m_nVideoEchoOrientation;
	// new in v1.04:
	var_pf_wave_usedots  = (double)m_bWaveDots;
	var_pf_wave_thick    = (double)m_bWaveThick;
	var_pf_wave_additive = (double)m_bAdditiveWaves;
	var_pf_wave_brighten = (double)m_bMaximizeWaveColor;
	var_pf_darken_center = (double)m_bDarkenCenter;
	var_pf_gamma         = (double)m_fGammaAdj.eval(time);
	var_pf_wrap          = (double)m_bTexWrap;
	var_pf_invert        = (double)m_bInvert;
	var_pf_brighten      = (double)m_bBrighten;
	var_pf_darken        = (double)m_bDarken;
	var_pf_solarize      = (double)m_bSolarize;
	// new in v2.0:
	var_pf_blur1min      = (double)m_fBlur1Min.eval(time);
	var_pf_blur2min      = (double)m_fBlur2Min.eval(time);
	var_pf_blur3min      = (double)m_fBlur3Min.eval(time);
	var_pf_blur1max      = (double)m_fBlur1Max.eval(time);
	var_pf_blur2max      = (double)m_fBlur2Max.eval(time);
	var_pf_blur3max      = (double)m_fBlur3Max.eval(time);
	var_pf_blur1_edge_darken = (double)m_fBlur1EdgeDarken.eval(time);
}

static void BlendState(CStatePtr m_pState, CStatePtr m_pOldState, float m_fBlendProgress, float m_fSnapPoint)
{
    // For all variables that do NOT affect pixel motion, blend them NOW,
    // so later the user can just access m_pState->m_pf_whatever.
    double mix  = (double)CosineInterp(m_fBlendProgress);
    double mix2 = 1.0 - mix;
    m_pState->var_pf_decay        = mix*(m_pState->var_pf_decay       ) + mix2*(m_pOldState->var_pf_decay       );
    m_pState->var_pf_wave_rgba.a       = mix*(m_pState->var_pf_wave_rgba.a      ) + mix2*(m_pOldState->var_pf_wave_rgba.a      );
    m_pState->var_pf_wave_rgba.r       = mix*(m_pState->var_pf_wave_rgba.r      ) + mix2*(m_pOldState->var_pf_wave_rgba.r      );
    m_pState->var_pf_wave_rgba.g       = mix*(m_pState->var_pf_wave_rgba.g      ) + mix2*(m_pOldState->var_pf_wave_rgba.g      );
    m_pState->var_pf_wave_rgba.b       = mix*(m_pState->var_pf_wave_rgba.b      ) + mix2*(m_pOldState->var_pf_wave_rgba.b      );
    m_pState->var_pf_wave_x       = mix*(m_pState->var_pf_wave_x      ) + mix2*(m_pOldState->var_pf_wave_x      );
    m_pState->var_pf_wave_y       = mix*(m_pState->var_pf_wave_y      ) + mix2*(m_pOldState->var_pf_wave_y      );
    m_pState->var_pf_wave_mystery = mix*(m_pState->var_pf_wave_mystery) + mix2*(m_pOldState->var_pf_wave_mystery);
    // wave_mode: exempt (integer)
    m_pState->var_pf_ob_size      = mix*(m_pState->var_pf_ob_size     ) + mix2*(m_pOldState->var_pf_ob_size     );
    m_pState->var_pf_ob_rgba.r         = mix*(m_pState->var_pf_ob_rgba.r        ) + mix2*(m_pOldState->var_pf_ob_rgba.r        );
    m_pState->var_pf_ob_rgba.g         = mix*(m_pState->var_pf_ob_rgba.g        ) + mix2*(m_pOldState->var_pf_ob_rgba.g        );
    m_pState->var_pf_ob_rgba.b         = mix*(m_pState->var_pf_ob_rgba.b        ) + mix2*(m_pOldState->var_pf_ob_rgba.b        );
    m_pState->var_pf_ob_rgba.a         = mix*(m_pState->var_pf_ob_rgba.a        ) + mix2*(m_pOldState->var_pf_ob_rgba.a        );
    m_pState->var_pf_ib_size      = mix*(m_pState->var_pf_ib_size     ) + mix2*(m_pOldState->var_pf_ib_size     );
    m_pState->var_pf_ib_rgba.r         = mix*(m_pState->var_pf_ib_rgba.r        ) + mix2*(m_pOldState->var_pf_ib_rgba.r        );
    m_pState->var_pf_ib_rgba.g         = mix*(m_pState->var_pf_ib_rgba.g        ) + mix2*(m_pOldState->var_pf_ib_rgba.g        );
    m_pState->var_pf_ib_rgba.b         = mix*(m_pState->var_pf_ib_rgba.b        ) + mix2*(m_pOldState->var_pf_ib_rgba.b        );
    m_pState->var_pf_ib_rgba.a         = mix*(m_pState->var_pf_ib_rgba.a        ) + mix2*(m_pOldState->var_pf_ib_rgba.a        );
    m_pState->var_pf_mv_xy.x         = mix*(m_pState->var_pf_mv_xy.x        ) + mix2*(m_pOldState->var_pf_mv_xy.x        );
    m_pState->var_pf_mv_xy.y         = mix*(m_pState->var_pf_mv_xy.y        ) + mix2*(m_pOldState->var_pf_mv_xy.y       );
    m_pState->var_pf_mv_dxy.x        = mix*(m_pState->var_pf_mv_dxy.x       ) + mix2*(m_pOldState->var_pf_mv_dxy.x       );
    m_pState->var_pf_mv_dxy.y        = mix*(m_pState->var_pf_mv_dxy.y       ) + mix2*(m_pOldState->var_pf_mv_dxy.y       );
    m_pState->var_pf_mv_l         = mix*(m_pState->var_pf_mv_l        ) + mix2*(m_pOldState->var_pf_mv_l        );
    m_pState->var_pf_mv_rgba.r         = mix*(m_pState->var_pf_mv_rgba.r        ) + mix2*(m_pOldState->var_pf_mv_rgba.r        );
    m_pState->var_pf_mv_rgba.g         = mix*(m_pState->var_pf_mv_rgba.g        ) + mix2*(m_pOldState->var_pf_mv_rgba.g        );
    m_pState->var_pf_mv_rgba.b         = mix*(m_pState->var_pf_mv_rgba.b        ) + mix2*(m_pOldState->var_pf_mv_rgba.b        );
    m_pState->var_pf_mv_rgba.a         = mix*(m_pState->var_pf_mv_rgba.a        ) + mix2*(m_pOldState->var_pf_mv_rgba.a        );
    m_pState->var_pf_echo_zoom    = mix*(m_pState->var_pf_echo_zoom   ) + mix2*(m_pOldState->var_pf_echo_zoom   );
    m_pState->var_pf_echo_alpha   = mix*(m_pState->var_pf_echo_alpha  ) + mix2*(m_pOldState->var_pf_echo_alpha  );
    m_pState->var_pf_echo_orient  = (mix < m_fSnapPoint) ? m_pOldState->var_pf_echo_orient : m_pState->var_pf_echo_orient;
    // added in v1.04:
    m_pState->var_pf_wave_usedots = (mix < m_fSnapPoint) ? m_pOldState->var_pf_wave_usedots  : m_pState->var_pf_wave_usedots ;
    m_pState->var_pf_wave_thick   = (mix < m_fSnapPoint) ? m_pOldState->var_pf_wave_thick    : m_pState->var_pf_wave_thick   ;
    m_pState->var_pf_wave_additive= (mix < m_fSnapPoint) ? m_pOldState->var_pf_wave_additive : m_pState->var_pf_wave_additive;
    m_pState->var_pf_wave_brighten= (mix < m_fSnapPoint) ? m_pOldState->var_pf_wave_brighten : m_pState->var_pf_wave_brighten;
    m_pState->var_pf_darken_center= (mix < m_fSnapPoint) ? m_pOldState->var_pf_darken_center : m_pState->var_pf_darken_center;
    m_pState->var_pf_gamma        = mix*(m_pState->var_pf_gamma       ) + mix2*(m_pOldState->var_pf_gamma       );
    m_pState->var_pf_wrap         = (mix < m_fSnapPoint) ? m_pOldState->var_pf_wrap          : m_pState->var_pf_wrap         ;
    m_pState->var_pf_invert       = (mix < m_fSnapPoint) ? m_pOldState->var_pf_invert        : m_pState->var_pf_invert       ;
    m_pState->var_pf_brighten     = (mix < m_fSnapPoint) ? m_pOldState->var_pf_brighten      : m_pState->var_pf_brighten     ;
    m_pState->var_pf_darken       = (mix < m_fSnapPoint) ? m_pOldState->var_pf_darken        : m_pState->var_pf_darken       ;
    m_pState->var_pf_solarize     = (mix < m_fSnapPoint) ? m_pOldState->var_pf_solarize      : m_pState->var_pf_solarize     ;
    // added in v2.0:
    m_pState->var_pf_blur1min  = mix*(m_pState->var_pf_blur1min ) + mix2*(m_pOldState->var_pf_blur1min );
    m_pState->var_pf_blur2min  = mix*(m_pState->var_pf_blur2min ) + mix2*(m_pOldState->var_pf_blur2min );
    m_pState->var_pf_blur3min  = mix*(m_pState->var_pf_blur3min ) + mix2*(m_pOldState->var_pf_blur3min );
    m_pState->var_pf_blur1max  = mix*(m_pState->var_pf_blur1max ) + mix2*(m_pOldState->var_pf_blur1max );
    m_pState->var_pf_blur2max  = mix*(m_pState->var_pf_blur2max ) + mix2*(m_pOldState->var_pf_blur2max );
    m_pState->var_pf_blur3max  = mix*(m_pState->var_pf_blur3max ) + mix2*(m_pOldState->var_pf_blur3max );
    m_pState->var_pf_blur1_edge_darken = mix*(m_pState->var_pf_blur1_edge_darken) + mix2*(m_pOldState->var_pf_blur1_edge_darken);

}

void CState::RunPerFrameEquations()
{

    // values that will affect the pixel motion (and will be automatically blended
    //    LATER, when the results of 2 sets of these params creates 2 different U/V
    //  meshes that get blended together.)
    LoadPerFrameEvallibVars();
    
    // also do just a once-per-frame init for the *per-**VERTEX*** *READ-ONLY* variables
    // (the non-read-only ones will be reset/restored at the start of each vertex)
    var_pv_common        = var_pf_common;

    //var_pv_monitor     = var_pf_monitor;
    
    // execute once-per-frame expressions:
    if (m_perframe_expression)
    {
        m_perframe_expression->Execute();
    }
    
    // save some things for next frame:
    monitor_after_init_code = var_pf_monitor;
    
    // save some things for per-vertex code:
    for (int vi=0; vi<NUM_Q_VAR; vi++)
        var_pv_q[vi] = var_pf_q[vi];
    
    // (a few range checks:)
    var_pf_gamma     = std::max(0.0  , std::min(    8.0, var_pf_gamma    ));
    var_pf_echo_zoom = std::max(0.001, std::min( 1000.0, var_pf_echo_zoom));
}

void CPlugin::RunPerFrameEquations()
{
    PROFILE_FUNCTION()
    
	// run per-frame calculations

	/*
	  code is only valid when blending.
		  OLDcomp ~ blend-from preset has a composite shader;
		  NEWwarp ~ blend-to preset has a warp shader; etc.

	  code OLDcomp NEWcomp OLDwarp NEWwarp
		0    
		1            1
		2                            1
		3            1               1
		4     1
		5     1      1
		6     1                      1
		7     1      1               1
		8                    1
		9            1       1
		10                   1       1
		11           1       1       1
		12    1              1
		13    1      1       1
		14    1              1       1
		15    1      1       1       1
	*/

	// when blending booleans (like darken, invert, etc) for pre-shader presets,
	// if blending to/from a pixel-shader preset, we can tune the snap point
	// (when it changes during the blend) for a less jumpy transition:
	m_fSnapPoint = 0.5f;
	if (m_bBlending)
	{
        
        bool bOldPresetUsesWarpShader = (m_pOldState->m_nWarpPSVersion > 0);
        bool bNewPresetUsesWarpShader = (m_pState->m_nWarpPSVersion > 0);
        bool bOldPresetUsesCompShader = (m_pOldState->m_nCompPSVersion > 0);
        bool bNewPresetUsesCompShader = (m_pState->m_nCompPSVersion > 0);
        
        // note: 'code' is only meaningful if we are BLENDING.
        int code = (bOldPresetUsesWarpShader ? 8 : 0) |
                   (bOldPresetUsesCompShader ? 4 : 0) |
                   (bNewPresetUsesWarpShader ? 2 : 0) |
                   (bNewPresetUsesCompShader ? 1 : 0);
        
		switch(code)
		{
		case 4:
		case 6:
		case 12:
		case 14:
			// old preset (only) had a comp shader
			m_fSnapPoint = -0.01f;
			break;
		case 1:
		case 3:
		case 9:
		case 11:
			// new preset (only) has a comp shader
			m_fSnapPoint = 1.01f;
			break;
		case 0:
		case 2:
		case 8:
		case 10:
			// neither old or new preset had a comp shader
			m_fSnapPoint = 0.5f;
			break;
		case 5:
		case 7:
		case 13:
		case 15:
			// both old and new presets use a comp shader - so it won't matter
			m_fSnapPoint = 0.5f;
			break;
		}
	}
	
	if (!m_bBlending)
    {
        m_pState->RunPerFrameEquations();
    }
    else
	{
        m_pState->RunPerFrameEquations();
        m_pOldState->RunPerFrameEquations();
        BlendState(m_pState, m_pOldState, m_fBlendProgress, m_fSnapPoint);
	}
}

//static int RoundUp(int size, int align)
//{
//    return (size + (align -1)) & ~(align-1);
//}


void CPlugin::TestLineDraw()
{
        {
            Vertex v[2];
            v[0].Clear();
            v[1].Clear();
            
            v[0].x = -0.25;
            v[0].y = -0.25;
            v[1].x =  0.25;
            v[1].y =  0.25;
            
            v[0].Diffuse = v[1].Diffuse = Color4F::ToU32(1,1,1,1);
            
            SetFixedShader(nullptr);
            m_context->SetBlend(BLEND_SRCALPHA, BLEND_INVSRCALPHA);
            GetDrawBuffer()->DrawLineListAA(2, v, 128, 64);
            GetDrawBuffer()->Flush();
            
        }
}

void CPlugin::ClearTargets()
{
    PROFILE_FUNCTION()

    m_context->SetRenderTarget(m_lpVS[1], "lpVS[1]", LoadAction::Clear );
    m_context->SetRenderTarget(m_lpVS[0], "lpVS[0]", LoadAction::Clear );
    
    for (auto texture : m_blur_textures) {
        m_context->SetRenderTarget(texture, "blur_texture_clear", LoadAction::Clear );
    }
    for (auto texture : m_blurtemp_textures) {
        m_context->SetRenderTarget(texture, "blur_texture_clear", LoadAction::Clear );
    }

}

void CPlugin::RenderFrame()
{
    PROFILE_FUNCTION()
    
    // note that this transform only affects the fixed function shader, not any custom shaders
    Matrix44 ortho = MatrixOrthoLH(2.0f, -2.0f, 0.0f, 1.0f);
    m_context->SetTransform(ortho);
    m_context->SetDepthEnable(false);
    m_context->SetBlendDisable();

	{
		m_rand_frame = Vector4(FRAND, FRAND, FRAND, FRAND);

		// update m_fBlendProgress;
		if (m_bBlending)
		{
			m_fBlendProgress = (GetTime() - m_fBlendStartTime) / m_fBlendDuration;
			if (m_fBlendProgress > 1.0f)
			{
				m_bBlending = false;
			}
		}

	}
    
    m_fAspectX = (m_nTexSizeY > m_nTexSizeX) ? m_nTexSizeX/(float)m_nTexSizeY : 1.0f;
    m_fAspectY = (m_nTexSizeX > m_nTexSizeY) ? m_nTexSizeY/(float)m_nTexSizeX : 1.0f;
    m_fInvAspectX = 1.0f/m_fAspectX;
    m_fInvAspectY = 1.0f/m_fAspectY;
    
    // setup common vars that everthing else uses
    m_commonVars.time        = (double)GetTime();
    m_commonVars.frame       = (double)GetFrame();
    m_commonVars.fps         = (double)GetFps();
    m_commonVars.progress    = (double)GetPresetProgress();
    m_commonVars.bass        = (double)GetImmRel(BAND_BASS);
    m_commonVars.mid         = (double)GetImmRel(BAND_MID);
    m_commonVars.treb        = (double)GetImmRel(BAND_TREBLE);
    m_commonVars.bass_att    = (double)GetAvgRel(BAND_BASS);
    m_commonVars.mid_att     = (double)GetAvgRel(BAND_MID);
    m_commonVars.treb_att    = (double)GetAvgRel(BAND_TREBLE);
    m_commonVars.meshxy.x         = (double)m_nGridX;
    m_commonVars.meshxy.y         = (double)m_nGridY;
    m_commonVars.pixelsxy.x       = (double)GetWidth();
    m_commonVars.pixelsxy.y       = (double)GetHeight();
    m_commonVars.aspectxy.x       = (double)m_fInvAspectX;
    m_commonVars.aspectxy.y       = (double)m_fInvAspectY;
    
    
	RunPerFrameEquations();
    

    {
        // draw motion vectors to VS0
        m_context->SetRenderTarget(m_lpVS[0], "MotionVectors", LoadAction::Load);
        m_pState->DrawMotionVectors();
    }
    
    if (m_clearTargets)
    {
        ClearTargets();

        m_clearTargets = false;
    }

	

    {
        ComputeGridAlphaValues_ComputeBegin();
        DrawCustomShapes_ComputeBegin(); // draw these first; better for feedback if the waves draw *over* them.
        DrawCustomWaves_ComputeBegin();
    }
    
   
    
    {
        ComputeGridAlphaValues_ComputeEnd();
    }
    

    //  blend grid vertices together
    BlendGrid();
    

    // warp from vs0 + blur123 -> vs1
    WarpedBlit();
    
    // blur passes vs0 -> blur123
    BlurPasses(0);
       
        
    // draw audio data
    {
        m_context->SetRenderTarget(m_lpVS[1], "DrawCustomShapes", LoadAction::Load);
        DrawCustomShapes_ComputeEnd();
    }

    {
        m_context->SetRenderTarget(m_lpVS[1], "DrawCustomWaves", LoadAction::Load);
        DrawCustomWaves_ComputeEnd();
    }

    
    {
        m_context->SetRenderTarget(m_lpVS[1], "DrawWave", LoadAction::Load);
        DrawWave();
    }
    
    {
        m_context->SetRenderTarget(m_lpVS[1], "DrawSprites", LoadAction::Load);
        DrawSprites();
        
    }

    // vs0 + blur123 - > output texture
    CompositeBlit();

    
    // flip buffers
    std::swap(m_lpVS[0], m_lpVS[1]);

}

void CPlugin::CaptureScreenshot(render::TexturePtr texture, Vector2 pos, Size2D size)
{
    // update screenshot
    m_context->SetRenderTarget(texture, "Screenshot", LoadAction::Load);
    m_context->SetBlendDisable();
 
    
    // setup ortho projections
    float sw = texture->GetWidth();
    float sh = texture->GetHeight();
    
    Matrix44 m = MatrixOrthoLH(0, sw, sh, 0, 0, 1);
    m_context->SetTransform(m);
    
    
    DrawQuad(m_outputTexture, pos.x, pos.y, size.width, size.height, Color4F(1,1,1,1));
    m_context->SetRenderTarget(nullptr);
}


void CPlugin::BlendGrid()
{
    PROFILE_FUNCTION()

    int total = (int)m_vertinfo.size();
    
    const std::vector<WarpVertex> &va = m_pState->m_verts;
    
    if (!m_bBlending)
    {
        for (int i=0; i < total; i++)
        {
            Vertex &ov = m_verts[i];
            ov.x = m_vertinfo[i].vx;
            ov.y = m_vertinfo[i].vy;
            ov.z = 0.0;
            ov.rad = m_vertinfo[i].rad;
            ov.ang = m_vertinfo[i].ang;
            ov.tu = va[i].tu;
            ov.tv = va[i].tv;
            ov.Diffuse = 0xFFFFFFFF;
        }
        return;
    }

    float fBlend = m_fBlendProgress;
    const std::vector<WarpVertex> &vb = m_pOldState->m_verts;

    // blend verts between states
    for (int i=0; i < total; i++)
    {
        const td_vertinfo &vi = m_vertinfo[i];
        Vertex &ov = m_verts[i];
        ov.x = vi.vx;
        ov.y = vi.vy;
        ov.z = 0.0;
        ov.rad = m_vertinfo[i].rad;
        ov.ang = m_vertinfo[i].ang;

        
        // blend to UV's for m_pOldState
        float mix2 = vi.a*fBlend + vi.c;//fCosineBlend2;
        mix2 = std::max(0.0f,std::min(1.0f,mix2));
        //     if fBlend un-flipped, then mix2 is 0 at the beginning of a blend, 1 at the end...
        //                           and alphas are 0 at the beginning, 1 at the end.
        ov.tu = va[i].tu*(mix2) + vb[i].tu*(1-mix2);
        ov.tv = va[i].tv*(mix2) + vb[i].tv*(1-mix2);
        // this sets the alpha values for blending between two presets:
        ov.Diffuse = Color4F::ToU32(1,1,1, mix2);
    }
}


void CPlugin::WarpedBlit()
{
    PROFILE_FUNCTION()

    bool bOldPresetUsesWarpShader = (m_pOldState->UsesWarpShader());
    bool bNewPresetUsesWarpShader = (m_pState->UsesWarpShader());

    // set up to render [from VS0] to VS1.
    m_context->SetRenderTarget(m_lpVS[1], "WarpedBlit", LoadAction::Clear);
       
    // do the warping for this frame [warp shader]
    if (!m_bBlending)
    {
        // no blend
        if (bNewPresetUsesWarpShader)
            WarpedBlit_Shaders(1, false, false);
        else
            WarpedBlit_NoShaders(1, false, false);
    }
    else
    {
        // blending
        // WarpedBlit( nPass,  bAlphaBlend, bFlipAlpha, bCullTiles, bFlipCulling )
        // note: alpha values go from 0..1 during a blend.
        // note: bFlipCulling==false means tiles with alpha>0 will draw.
        //       bFlipCulling==true  means tiles with alpha<255 will draw.
        
        if (bOldPresetUsesWarpShader && bNewPresetUsesWarpShader)
        {
            WarpedBlit_Shaders  (0, false, false);
            WarpedBlit_Shaders  (1, true,  false);
        }
        else if (!bOldPresetUsesWarpShader && bNewPresetUsesWarpShader)
        {
            WarpedBlit_NoShaders(0, false, false);
            WarpedBlit_Shaders  (1, true,  false);
        }
        else if (bOldPresetUsesWarpShader && !bNewPresetUsesWarpShader)
        {
            WarpedBlit_Shaders  (0, false, false);
            WarpedBlit_NoShaders(1, true,  false);
        }
        else if (!bOldPresetUsesWarpShader && !bNewPresetUsesWarpShader)
        {
            //WarpedBlit_NoShaders(0, false, false,   true, true);
            //WarpedBlit_NoShaders(1, true,  false,   true, false);
            
            // special case - all the blending just happens in the vertex UV's, so just pretend there's no blend.
            WarpedBlit_NoShaders(1, false, false);
        }
    }

}


void CPlugin::CompositeBlit()
{
    PROFILE_FUNCTION()

    bool bOldPresetUsesCompShader = (m_pOldState->UsesCompShader());
    bool bNewPresetUsesCompShader = (m_pState->UsesCompShader());
    
    m_context->SetRenderTarget(m_outputTexture, "CompositeBlit", LoadAction::Clear);


    // show it to the user [composite shader]
    if (!m_bBlending)
    {
        // no blend
        if (bNewPresetUsesCompShader)
            ShowToUser_Shaders(1, false, false);
        else
            ShowToUser_NoShaders();//1, false, false, false, false);
    }
    else
    {
        // blending
        // ShowToUser( nPass,  bAlphaBlend, bFlipAlpha)
        // note: alpha values go from 0..1 during a blend.
        
        // NOTE: ShowToUser_NoShaders() must always come before ShowToUser_Shaders(),
        //        because it always draws the full quad (it can't do tile culling or alpha blending).
        //        [third case here]
        
        if (bOldPresetUsesCompShader && bNewPresetUsesCompShader)
        {
            ShowToUser_Shaders  (0, false, false);
            ShowToUser_Shaders  (1, true,  false);
        }
        else if (!bOldPresetUsesCompShader && bNewPresetUsesCompShader)
        {
            ShowToUser_NoShaders();
            ShowToUser_Shaders  (1, true,  false);
        }
        else if (bOldPresetUsesCompShader && !bNewPresetUsesCompShader)
        {
            // THA FUNKY REVERSAL
            //ShowToUser_Shaders  (0);
            //ShowToUser_NoShaders(1);
            ShowToUser_NoShaders();
            ShowToUser_Shaders  (0, true, true);
        }
        else if (!bOldPresetUsesCompShader && !bNewPresetUsesCompShader)
        {
            // special case - all the blending just happens in the blended state vars, so just pretend there's no blend.
            ShowToUser_NoShaders();//1, false, false, false, false);
        }
    }
}

void CState::DrawMotionVectors()
{
	// FLEXIBLE MOTION VECTOR FIELD
	if ((float)var_pf_mv_rgba.a >= 0.001f)
	{
        PROFILE_FUNCTION();

        ContextPtr context = m_plugin->m_context;
        IDrawBufferPtr draw = m_plugin->GetDrawBuffer();

        
        // set up to render [from NULL] to VS0 (for motion vectors).
        {
            context->SetDepthEnable(false);
            context->SetBlendDisable();
            m_plugin->SetFixedShader(nullptr);
        }
        
		//-------------------------------------------------------

		m_plugin->SetFixedShader(nullptr);
		//-------------------------------------------------------

		int x,y;

		int nX = (int)(var_pf_mv_xy.x);// + 0.999f);
		int nY = (int)(var_pf_mv_xy.y);// + 0.999f);
		float dx = (float)var_pf_mv_xy.x - nX;
		float dy = (float)var_pf_mv_xy.y - nY;
		if (nX > 64) { nX = 64; dx = 0; }
		if (nY > 48) { nY = 48; dy = 0; }

		if (nX > 0 && nY > 0)
		{
			/*
			float dx2 = m_fMotionVectorsTempDx;//(var_pf_mv_dx) * 0.05f*GetTime();		// 0..1 range
			float dy2 = m_fMotionVectorsTempDy;//(var_pf_mv_dy) * 0.05f*GetTime();		// 0..1 range
			if (GetFps() > 2.0f && GetFps() < 300.0f)
			{
				dx2 += (float)(var_pf_mv_dx) * 0.05f / GetFps();
				dy2 += (float)(var_pf_mv_dy) * 0.05f / GetFps();
			}
			if (dx2 > 1.0f) dx2 -= (int)dx2;
			if (dy2 > 1.0f) dy2 -= (int)dy2;
			if (dx2 < 0.0f) dx2 = 1.0f - (-dx2 - (int)(-dx2));
			if (dy2 < 0.0f) dy2 = 1.0f - (-dy2 - (int)(-dy2));
			// hack: when there is only 1 motion vector on the screem, to keep it in
			//       the center, we gradually migrate it toward 0.5.
			dx2 = dx2*0.995f + 0.5f*0.005f;	
			dy2 = dy2*0.995f + 0.5f*0.005f;
			// safety catch
			if (dx2 < 0 || dx2 > 1 || dy2 < 0 || dy2 > 1)
			{
				dx2 = 0.5f;
				dy2 = 0.5f;
			}
			m_fMotionVectorsTempDx = dx2;
			m_fMotionVectorsTempDy = dy2;*/
			float dx2 = (float)(var_pf_mv_dxy.x);
			float dy2 = (float)(var_pf_mv_dxy.y);

			float len_mult = (float)var_pf_mv_l;
			if (dx < 0) dx = 0;
			if (dy < 0) dy = 0;
			if (dx > 1) dx = 1;
			if (dy > 1) dy = 1;
			//dx = dx * 1.0f/(float)nX;
			//dy = dy * 1.0f/(float)nY;
			float inv_texsize = 1.0f/(float)m_plugin->m_nTexSizeX;
			float min_len = 1.0f*inv_texsize;

			Vertex v[(64+1)*2];
			memset(v, 0, sizeof(v));
			v[0].Diffuse = COLOR_RGBA_01((float)var_pf_mv_rgba.r,(float)var_pf_mv_rgba.g,(float)var_pf_mv_rgba.b,(float)var_pf_mv_rgba.a);
			for (x=1; x<(nX+1)*2; x++)
				v[x].Diffuse = v[0].Diffuse;

			context->SetBlend(BLEND_SRCALPHA, BLEND_INVSRCALPHA);

			for (y=0; y<nY; y++)
			{
				float fy = (y + 0.25f)/(float)(nY + dy + 0.25f - 1.0f);

				// now move by offset
				fy -= dy2;

				if (fy > 0.0001f && fy < 0.9999f)
				{
					int n = 0;
					for (x=0; x<nX; x++)
					{
						//float fx = (x + 0.25f)/(float)(nX + dx + 0.25f - 1.0f);
						float fx = (x + 0.25f)/(float)(nX + dx + 0.25f - 1.0f);

						// now move by offset
						fx += dx2;

						if (fx > 0.0001f && fx < 0.9999f)
						{
							float fx2 = 0.0f, fy2 = 0.0f;
							m_plugin->ReversePropagatePoint(fx, fy, &fx2, &fy2);	// NOTE: THIS IS REALLY A REVERSE-PROPAGATION
							//fx2 = fx*2 - fx2;
							//fy2 = fy*2 - fy2;
							//fx2 = fx + 1.0f/(float)m_nTexSize;
							//fy2 = 1-(fy + 1.0f/(float)m_nTexSize);

							// enforce minimum trail lengths:
							{	
								float dx = (fx2 - fx);
								float dy = (fy2 - fy);
								dx *= len_mult;
								dy *= len_mult;
								float len = sqrtf(dx*dx + dy*dy);

								if (len > min_len)
								{

								}
								else if (len > 0.00000001f)
								{
									len = min_len/len;
									dx *= len;
									dy *= len;
								}
								else
								{
									dx = min_len;
									dy = min_len;
								}
									
								fx2 = fx + dx;
								fy2 = fy + dy;
							}
							/**/

							v[n].x = fx * 2.0f - 1.0f;
							v[n].y = fy * 2.0f - 1.0f;
							v[n+1].x = fx2 * 2.0f - 1.0f;
							v[n+1].y = fy2 * 2.0f - 1.0f; 

							// actually, project it in the reverse direction
							//v[n+1].x = v[n].x*2.0f - v[n+1].x;// + dx*2; 
							//v[n+1].y = v[n].y*2.0f - v[n+1].y;// + dy*2; 
							//v[n].x += dx*2;
							//v[n].y += dy*2;

							n += 2;
						}
					}

					// draw it
                    draw->DrawLineList(n, v);
				}
			}
            
            draw->Flush();

			context->SetBlendDisable();
		}
	}
}

bool CPlugin::ReversePropagatePoint(float fx, float fy, float *fx2, float *fy2)
{
	//float fy = y/(float)nMotionVectorsY;
	int   y0 = (int)(fy*m_nGridY);
	float dy = fy*m_nGridY - y0;

	//float fx = x/(float)nMotionVectorsX;
	int   x0 = (int)(fx*m_nGridX);
	float dx = fx*m_nGridX - x0;

	int x1 = x0 + 1;
	int y1 = y0 + 1;

	if (x0 < 0) return false;
	if (y0 < 0) return false;
	//if (x1 < 0) return false;
	//if (y1 < 0) return false;
	//if (x0 > m_nGridX) return false;
	//if (y0 > m_nGridY) return false;
	if (x1 > m_nGridX) return false;
	if (y1 > m_nGridY) return false;

	float tu, tv;
	tu  = m_verts[y0*(m_nGridX+1)+x0].tu * (1-dx)*(1-dy);
	tv  = m_verts[y0*(m_nGridX+1)+x0].tv * (1-dx)*(1-dy);
	tu += m_verts[y0*(m_nGridX+1)+x1].tu * (dx)*(1-dy);
	tv += m_verts[y0*(m_nGridX+1)+x1].tv * (dx)*(1-dy);
	tu += m_verts[y1*(m_nGridX+1)+x0].tu * (1-dx)*(dy);
	tv += m_verts[y1*(m_nGridX+1)+x0].tv * (1-dx)*(dy);
	tu += m_verts[y1*(m_nGridX+1)+x1].tu * (dx)*(dy);
	tv += m_verts[y1*(m_nGridX+1)+x1].tv * (dx)*(dy);

	*fx2 = tu;
	*fy2 = 1.0f - tv;
	return true;
}

void CState::GetSafeBlurMinMax(float* blur_min, float* blur_max)
{
	blur_min[0] = (float)var_pf_blur1min;
	blur_min[1] = (float)var_pf_blur2min;
	blur_min[2] = (float)var_pf_blur3min;
	blur_max[0] = (float)var_pf_blur1max;
	blur_max[1] = (float)var_pf_blur2max;
	blur_max[2] = (float)var_pf_blur3max;

	// check that precision isn't wasted in later blur passes [...min-max gap can't grow!]
	// also, if min-max are close to each other, push them apart:
	const float fMinDist = 0.1f;
	if (blur_max[0] - blur_min[0] < fMinDist) {
		float avg = (blur_min[0] + blur_max[0])*0.5f;
		blur_min[0] = avg - fMinDist*0.5f;
		blur_max[0] = avg - fMinDist*0.5f;
	}
	blur_max[1] = std::min(blur_max[0], blur_max[1]);
	blur_min[1] = std::max(blur_min[0], blur_min[1]);
	if (blur_max[1] - blur_min[1] < fMinDist) {
		float avg = (blur_min[1] + blur_max[1])*0.5f;
		blur_min[1] = avg - fMinDist*0.5f;
		blur_max[1] = avg - fMinDist*0.5f;
	}
	blur_max[2] = std::min(blur_max[1], blur_max[2]);
	blur_min[2] = std::max(blur_min[1], blur_min[2]);
	if (blur_max[2] - blur_min[2] < fMinDist) {
		float avg = (blur_min[2] + blur_max[2])*0.5f;
		blur_min[2] = avg - fMinDist*0.5f;
		blur_max[2] = avg - fMinDist*0.5f;
	}
}

void CPlugin::DrawImageWithShader(ShaderPtr shader, TexturePtr source, TexturePtr dest)
{
    m_context->SetBlendDisable();
    m_context->SetShader(shader);
    m_context->SetRenderTarget(dest, dest->GetName().c_str(), LoadAction::Clear );
    
    auto sampler = shader->GetSampler(0);
    if (sampler)
    {
        sampler->SetTexture(source, SAMPLER_CLAMP, SAMPLER_LINEAR);
    }
    
    // set up fullscreen quad
    Vertex v[4];
    memset(v, 0, sizeof(v));
    
    v[0].x = -1;
    v[0].y = -1;
    v[1].x =  1;
    v[1].y = -1;
    v[2].x = -1;
    v[2].y =  1;
    v[3].x =  1;
    v[3].y =  1;
    
    v[0].tu = 0;
    v[0].tv = 0;
    v[1].tu = 1;
    v[1].tv = 0;
    v[2].tu = 0;
    v[2].tv = 1;
    v[3].tu = 1;
    v[3].tv = 1;
    
    for (int i=0; i < 4; i++)
    {
        v[i].Diffuse = 0xFFFFFFFF;
        v[i].ang = v[i].rad = 0;
    }
    
    m_context->DrawArrays(PRIMTYPE_TRIANGLESTRIP, 4, v);

}



void CPlugin::BlurPasses(int source_index)
{
		// Note: Blur is currently a little funky.  It blurs the *current* frame after warp;
		//         this way, it lines up well with the composite pass.  However, if you switch
		//         presets instantly, to one whose *warp* shader uses the blur texture,
		//         it will be outdated (just for one frame).  Oh well.  
		//       This also means that when sampling the blurred textures in the warp shader, 
		//         they are one frame old.  This isn't too big a deal.  Getting them to match
		//         up for the composite pass is probably more important.

        int m_nHighestBlurTexUsedThisFrame = m_pState->GetHighestBlurTexUsed();
        if (m_bBlending)
            m_nHighestBlurTexUsedThisFrame = std::max(m_nHighestBlurTexUsedThisFrame, m_pOldState->GetHighestBlurTexUsed());
    
		int passes = std::min( (int)m_blur_textures.size(), m_nHighestBlurTexUsedThisFrame);
        //passes = (int)m_blur_textures.size();
        if (passes==0)
            return;
    
        PROFILE_FUNCTION()

        m_context->PushLabel("BlurPasses");

		const float w[8] = { 4.0f, 3.8f, 3.5f, 2.9f, 1.9f, 1.2f, 0.7f, 0.3f };  //<- user can specify these
		float edge_darken = (float)m_pState->var_pf_blur1_edge_darken;
		float blur_min[3], blur_max[3];
		m_pState->GetSafeBlurMinMax(blur_min, blur_max);

		float fscale[3];
		float fbias[3];

		// figure out the progressive scale & bias needed, at each step, 
		// to go from one [min..max] range to the next.
		float temp_min, temp_max;
		fscale[0] = 1.0f / (blur_max[0] - blur_min[0]);
		fbias [0] = -blur_min[0] * fscale[0];
		temp_min  = (blur_min[1] - blur_min[0]) / (blur_max[0] - blur_min[0]);
		temp_max  = (blur_max[1] - blur_min[0]) / (blur_max[0] - blur_min[0]);
		fscale[1] = 1.0f / (temp_max - temp_min);
		fbias [1] = -temp_min * fscale[1];
		temp_min  = (blur_min[2] - blur_min[1]) / (blur_max[1] - blur_min[1]);
		temp_max  = (blur_max[2] - blur_min[1]) / (blur_max[1] - blur_min[1]);
		fscale[2] = 1.0f / (temp_max - temp_min);
		fbias [2] = -temp_min * fscale[2];

        TexturePtr source = m_lpVS[source_index];
    
		for (int pass=0; pass < passes; pass++)
		{
            TexturePtr dest = m_blurtemp_textures[pass];

			{
                // pass 1 (long horizontal pass)
                //-------------------------------------
                float fscale_now = fscale[pass];
                float fbias_now  = fbias[pass];


                int srcw = source->GetWidth();
                int srch = source->GetHeight();
                Vector4 srctexsize = Vector4( (float)srcw, (float)srch, 1.0f/(float)srcw, 1.0f/(float)srch );

        


				const float w1 = w[0] + w[1];
				const float w2 = w[2] + w[3];
				const float w3 = w[4] + w[5];
				const float w4 = w[6] + w[7];
				const float d1 = 0 + 2*w[1]/w1;
				const float d2 = 2 + 2*w[3]/w2;
				const float d3 = 4 + 2*w[5]/w3;
				const float d4 = 6 + 2*w[7]/w4;
				const float w_div = 0.5f/(w1+w2+w3+w4);
				//-------------------------------------
				//float4 _c0; // source texsize (.xy), and inverse (.zw)
				//float4 _c1; // w1..w4
				//float4 _c2; // d1..d4
				//float4 _c3; // scale, bias, w_div, 0
				//-------------------------------------
                
#if 1
                ShaderPtr shader =  m_shader_blur1;
                auto _srctexsize = shader->GetConstant("srctexsize");
                auto _wv = shader->GetConstant("wv");
                auto _dv = shader->GetConstant("dv");
                auto _fscale = shader->GetConstant("fscale");
                auto _fbias = shader->GetConstant("fbias");
                auto _w_div = shader->GetConstant("w_div");

                // set constants
                _srctexsize->SetVector( srctexsize );
                _wv->SetVector( Vector4( w1,w2,w3,w4 ));
                _dv->SetVector( Vector4( d1,d2,d3,d4 ));
                _fscale->SetFloat(fscale_now);
                _fbias->SetFloat(fbias_now);
                _w_div->SetFloat(w_div);

                
                DrawImageWithShader(shader, source, dest);
#else
                ShaderInfoPtr si = m_BlurShaders[0];
                ShaderConstantPtr* h = si->const_handles;
				if (h[0]) h[0]->SetVector( srctexsize );
				if (h[1]) h[1]->SetVector( Vector4( w1,w2,w3,w4 ));
				if (h[2]) h[2]->SetVector( Vector4( d1,d2,d3,d4 ));
				if (h[3]) h[3]->SetVector( Vector4( fscale_now,fbias_now,w_div,0));
                

                DrawImageWithShader(si->shader, source, dest);
#endif
			}
            
    
            {
                // pass 2 (short vertical pass)
                //-------------------------------------

                source = dest;
                dest   = m_blur_textures[pass];


                int srcw = source->GetWidth();
                int srch = source->GetHeight();
                Vector4 srctexsize = Vector4( (float)srcw, (float)srch, 1.0f/(float)srcw, 1.0f/(float)srch );


				const float w1 = w[0]+w[1] + w[2]+w[3];
				const float w2 = w[4]+w[5] + w[6]+w[7];
				const float d1 = 0 + 2*((w[2]+w[3])/w1);
				const float d2 = 2 + 2*((w[6]+w[7])/w2);
				const float w_div = 1.0f/((w1+w2)*2);
				//-------------------------------------
				//float4 _c0; // source texsize (.xy), and inverse (.zw)
				//float4 _c5; // w1,w2,d1,d2
				//float4 _c6; // w_div, edge_darken_c1, edge_darken_c2, edge_darken_c3
				//-------------------------------------
#if 1
                ShaderPtr shader =  m_shader_blur2;
                auto _srctexsize = shader->GetConstant("srctexsize");
                auto _wv = shader->GetConstant("wv");
                auto _dv = shader->GetConstant("dv");
                auto _edge_darken = shader->GetConstant("edge_darken");
                auto _w_div = shader->GetConstant("w_div");


                _srctexsize->SetVector( srctexsize );
                _wv->SetVector(Vector4(w1,w2,0,0) );
                _dv->SetVector(Vector4(d1,d2,0,0) );

                {
                    // note: only do this first time; if you do it many times,
                    // then the super-blurred levels will have big black lines along the top & left sides.
                    if (pass==0)
                        _edge_darken->SetVector(Vector4((1-edge_darken),edge_darken,5.0f,0.0f)); //darken edges
                    else
                        _edge_darken->SetVector(Vector4(1.0f,0.0f,5.0f,0.0f)); // don't darken
                }
                _w_div->SetFloat(w_div);
                DrawImageWithShader(shader, source, dest);
                
#else
                
                ShaderInfoPtr si = m_BlurShaders[1];
                ShaderConstantPtr* h = si->const_handles;

                if (h[0]) h[0]->SetVector( srctexsize );
				if (h[5]) h[5]->SetVector(Vector4(w1,w2,d1,d2) );
				if (h[6]) 
				{
					// note: only do this first time; if you do it many times, 
					// then the super-blurred levels will have big black lines along the top & left sides.
					if (pass==0)
						h[6]->SetVector(Vector4(w_div,(1-edge_darken),edge_darken,5.0f)); //darken edges
					else
						h[6]->SetVector(Vector4(w_div,1.0f,0.0f,5.0f )); // don't darken
				}
                DrawImageWithShader(si->shader, source, dest);
#endif
			}
            
            source = dest;
		}
	    
    m_context->PopLabel();

}


void CPlugin::ComputeGridAlphaValues_ComputeBegin()
{
    PROFILE_FUNCTION()

    m_pState->ComputeGridAlphaValues_ComputeBegin();
    if (m_bBlending)
        m_pOldState->ComputeGridAlphaValues_ComputeBegin();
    
}

void CPlugin::ComputeGridAlphaValues_ComputeEnd()
{
    PROFILE_FUNCTION()

    m_pState->ComputeGridAlphaValues_ComputeEnd();
    if (m_bBlending)
        m_pOldState->ComputeGridAlphaValues_ComputeEnd();
    
}


void CState::ComputeGridAlphaValues_ComputeBegin()
{
    PROFILE_FUNCTION();

    const std::vector<td_vertinfo>       &m_vertinfo = m_plugin->m_vertinfo;
    float fAspectX = m_plugin->m_fAspectX;
    float fAspectY = m_plugin->m_fAspectY;
    int gridX = m_plugin->m_nGridX;
    int gridY = m_plugin->m_nGridY;

    int total  = (gridY+1) * (gridX+1);
    
    m_verts.resize(total);
    
    m_pervertex_buffer->Resize(total);

    for (int n=0; n < total; n++)
    {
        float vx  = m_vertinfo[n].vx;
        float vy  = m_vertinfo[n].vy;
        float rad = m_vertinfo[n].rad;
        float ang = m_vertinfo[n].ang;
        // restore all the variables to their original states,
        //  run the user-defined equations,
        //  then move the results into local vars for computation as floats
        
        var_pv_xy.x        = (double)(vx* 0.5f*fAspectX + 0.5f);
        var_pv_xy.y        = (double)(vy*-0.5f*fAspectY + 0.5f);
        var_pv_rad        = (double)rad;
        var_pv_ang        = (double)ang;
        var_pv_zoom    = var_pf_zoom;
        var_pv_zoomexp    = var_pf_zoomexp;
        var_pv_rot        = var_pf_rot;
        var_pv_warp    = var_pf_warp;
        var_pv_cxy        = var_pf_cxy;
        var_pv_dxy        = var_pf_dxy;
        var_pv_sxy        = var_pf_sxy;

        m_pervertex_buffer->StoreJob(n);
    }

    // execute all kernels
    m_pervertex_buffer->ExecuteAsync();
}


void CState::ComputeGridAlphaValues_ComputeEnd()
{
    PROFILE_FUNCTION();

    float fAspectX = m_plugin->m_fAspectX;
    float fAspectY = m_plugin->m_fAspectY;
    float fInvAspectX = m_plugin->m_fInvAspectX;
    float fInvAspectY = m_plugin->m_fInvAspectY;
    const std::vector<td_vertinfo>       &m_vertinfo = m_plugin->m_vertinfo;

    // warp stuff
    float fWarpTime = m_plugin->GetTime() * m_fWarpAnimSpeed;
    float fWarpScaleInv = 1.0f / m_fWarpScale.eval(m_plugin->GetTime());
    float f[4];
    f[0] = 11.68f + 4.0f*cosf(fWarpTime*1.413f + 10);
    f[1] =  8.77f + 3.0f*cosf(fWarpTime*1.113f + 7);
    f[2] = 10.54f + 3.0f*cosf(fWarpTime*1.233f + 3);
    f[3] = 11.49f + 4.0f*cosf(fWarpTime*0.933f + 5);
    
    // texel alignment
//    float texel_offset_x = m_plugin->m_context->GetTexelOffset() / (float)m_plugin->m_nTexSizeX;
//    float texel_offset_y = m_plugin->m_context->GetTexelOffset() / (float)m_plugin->m_nTexSizeY;

    // wait for end
    m_pervertex_buffer->Sync();

    // complete
//    int n = 0;
//    for (int y=0; y<=m_plugin->m_nGridY; y++)
    int total = m_pervertex_buffer->GetSize();
    for (int n=0; n < total; n++)
    {
//        for (int x=0; x<=m_plugin->m_nGridX; x++)
        {
            float vx  = m_vertinfo[n].vx;
            float vy  = m_vertinfo[n].vy;
            float rad = m_vertinfo[n].rad;
//            float ang = m_vertinfo[n].ang;
//
//            var_pv_xy.x       = (double)(vx* 0.5f*fAspectX + 0.5f);
//            var_pv_xy.y       = (double)(vy*-0.5f*fAspectY + 0.5f);
//            var_pv_rad        = (double)rad;
//            var_pv_ang        = (double)ang;
//            var_pv_zoom       = var_pf_zoom;
//            var_pv_zoomexp    = var_pf_zoomexp;
//            var_pv_rot        = var_pf_rot;
//            var_pv_warp       = var_pf_warp;
//            var_pv_cxy        = var_pf_cxy;
//            var_pv_dxy        = var_pf_dxy;
//            var_pv_sxy        = var_pf_sxy;

            m_pervertex_buffer->ReadJob(n);

            
            float fZoom = (float)(var_pv_zoom);
            float fZoomExp = (float)(var_pv_zoomexp);
            float fRot  = (float)(var_pv_rot);
            float fWarp = (float)(var_pv_warp);
            float fCX   = (float)(var_pv_cxy.x);
            float fCY   = (float)(var_pv_cxy.y);
            float fDX   = (float)(var_pv_dxy.x);
            float fDY   = (float)(var_pv_dxy.y);
            float fSX   = (float)(var_pv_sxy.x);
            float fSY   = (float)(var_pv_sxy.y);
            
            
            float fZoom2 = powf(fZoom, powf(fZoomExp, rad*2.0f - 1.0f));
            
            // initial texcoords, w/built-in zoom factor
            float fZoom2Inv = 1.0f/fZoom2;
            float u =  vx*fAspectX*0.5f*fZoom2Inv + 0.5f;
            float v = -vy*fAspectY*0.5f*fZoom2Inv + 0.5f;
            //float u_orig = u;
            //float v_orig = v;
            //m_verts[n].tr = u_orig + texel_offset_x;
            //m_verts[n].ts = v_orig + texel_offset_y;
            
            // stretch on X, Y:
            u = (u - fCX)/fSX + fCX;
            v = (v - fCY)/fSY + fCY;
            
            // warping:
            //if (fWarp > 0.001f || fWarp < -0.001f)
            //{
            u += fWarp*0.0035f*sinf(fWarpTime*0.333f + fWarpScaleInv*(vx*f[0] - vy*f[3]));
            v += fWarp*0.0035f*cosf(fWarpTime*0.375f - fWarpScaleInv*(vx*f[2] + vy*f[1]));
            u += fWarp*0.0035f*cosf(fWarpTime*0.753f - fWarpScaleInv*(vx*f[1] - vy*f[2]));
            v += fWarp*0.0035f*sinf(fWarpTime*0.825f + fWarpScaleInv*(vx*f[0] + vy*f[3]));
            //}
            
            // rotation:
            float u2 = u - fCX;
            float v2 = v - fCY;
            
            float cos_rot = cosf(fRot);
            float sin_rot = sinf(fRot);
            u = u2*cos_rot - v2*sin_rot + fCX;
            v = u2*sin_rot + v2*cos_rot + fCY;
            
            // translation:
            u -= fDX;
            v -= fDY;
            
            // undo aspect ratio fix:
            u = (u-0.5f)*fInvAspectX + 0.5f;
            v = (v-0.5f)*fInvAspectY + 0.5f;
            
            // final half-texel-offset translation:
//            u += texel_offset_x;
//            v += texel_offset_y;

            // write texcoords
            m_verts[n].tu = u;
            m_verts[n].tv = v;
         //   n++;
        }
    }
    
}








void CPlugin::WarpedBlit_NoShaders(int nPass, bool bAlphaBlend, bool bFlipAlpha)
{
    PROFILE_FUNCTION()

	SamplerAddress texaddr = (m_pState->var_pf_wrap > m_fSnapPoint) ? SAMPLER_WRAP : SAMPLER_CLAMP;
    float fDecay = (float)(m_pState->var_pf_decay);

    // decay
    
    //if (m_pState->m_bBlending)
    //    fDecay = fDecay*(fCosineBlend) + (1.0f-fCosineBlend)*((float)(m_pOldState->var_pf_decay));


#if 0
    Color4F cDecay = COLOR_RGBA_01(fDecay,fDecay,fDecay,1);
    SetFixedShader(m_lpVS[0], texaddr, SAMPLER_LINEAR);

#else
    auto shader = m_shader_warp_fixed;
    auto sampler = shader->GetSampler(0);
    m_context->SetShader(shader);
    
    ShaderConstantPtr uniform_fdecay = shader->GetConstant("fDecay");
    if (uniform_fdecay)
        uniform_fdecay->SetFloat(fDecay);
    
	if (sampler)
    sampler->SetTexture(m_lpVS[0], texaddr, SAMPLER_LINEAR);
#endif
    



	if (bAlphaBlend)
	{
		if (bFlipAlpha)
		{
			m_context->SetBlend(BLEND_INVSRCALPHA, BLEND_SRCALPHA);
		}
		else
		{
			m_context->SetBlend(BLEND_SRCALPHA, BLEND_INVSRCALPHA);
		}
	}
	else 
		m_context->SetBlendDisable();

    
	// Hurl the triangles at the video card.
	// We're going to un-index it, so that we don't stress any crappy (AHEM intel g33)
	//  drivers out there.  
	// If we're blending, we'll skip any polygon that is all alpha-blended out.
	// This also respects the MaxPrimCount limit of the video card.

    int primCount = m_nGridX*m_nGridY*2;
#if 0
	{
        Vertex tempv[1024 * 16];
		int i=0;
        int src_idx = 0;
        for (int prim = 0; prim < primCount; prim++)
		{
			// copy 3 verts
			for (int j=0; j<3; j++) 
			{
				tempv[i++] = m_verts[ m_indices_list[src_idx++] ];
				// don't forget to flip sign on Y and factor in the decay color!:
				tempv[i-1].y *= -1;
//				tempv[i-1].Diffuse = (cDecay & 0x00FFFFFF) | (tempv[i-1].Diffuse & 0xFF000000);      
				tempv[i - 1].Diffuse.r = cDecay.r;
				tempv[i - 1].Diffuse.g = cDecay.g;
				tempv[i - 1].Diffuse.b = cDecay.b;
			}
        
		}
        m_context->Draw(PRIMTYPE_TRIANGLELIST, primCount, tempv );
	}
#elif 0
    {

        Vertex tempv[1024 * 16];
        for (size_t i=0; i < m_verts.size(); i++)
        {
            tempv[i] = m_verts[i];
            tempv[i].y *= -1;
            tempv[i].Diffuse.r = cDecay.r;
            tempv[i].Diffuse.g = cDecay.g;
            tempv[i].Diffuse.b = cDecay.b;
        }
        
        m_context->UploadIndexData(m_indices_list);
        m_context->UploadVertexData((int)m_verts.size(), tempv);
        m_context->DrawIndexed(PRIMTYPE_TRIANGLELIST, 0,  primCount * 3 );
    }
#else
    {
        m_context->UploadIndexData(m_indices_list);
        m_context->UploadVertexData(m_verts);
        m_context->DrawIndexed(PRIMTYPE_TRIANGLELIST, 0,  primCount * 3 );
    }

    
#endif

	m_context->SetBlendDisable();
}

void CPlugin::WarpedBlit_Shaders(int nPass, bool bAlphaBlend, bool bFlipAlpha)
{
    PROFILE_FUNCTION()

	// if nPass==0, it draws old preset (blending 1 of 2).
	// if nPass==1, it draws new preset (blending 2 of 2, OR done blending)
    
	if (bAlphaBlend)
	{
		if (bFlipAlpha)
		{
			m_context->SetBlend(BLEND_INVSRCALPHA, BLEND_SRCALPHA);
		}
		else
		{
			m_context->SetBlend(BLEND_SRCALPHA, BLEND_INVSRCALPHA);
		}
	}
	else 
		m_context->SetBlendDisable();

	int pass = nPass;
	{
		// PASS 0: draw using *blended per-vertex motion vectors*, but with the OLD warp shader.
		// PASS 1: draw using *blended per-vertex motion vectors*, but with the NEW warp shader.
		CStatePtr state = (pass==0) ? m_pOldState : m_pState;

		ApplyShaderParams(state->m_shader_warp, state );

		// Hurl the triangles at the video card.
		// We're going to un-index it, so that we don't stress any crappy (AHEM intel g33)
		//  drivers out there.  
		// We divide it into the two halves of the screen (top/bottom) so we can hack
		//  the 'ang' values along the angle-wrap seam, halfway through the draw.
		// If we're blending, we'll skip any polygon that is all alpha-blended out.
		// This also respects the MaxPrimCount limit of the video card.
        
#if 0
		Vertex tempv[1024 * 16];
		for (int half=0; half<2; half++)
		{
			// hack / restore the ang values along the angle-wrap [0 <-> 2pi] seam...
            float new_ang = half ? M_PI : -M_PI;
            int y_offset = (m_nGridY/2) * (m_nGridX + 1);
			for (int x=0; x<m_nGridX/2; x++)
				m_verts[y_offset + x].ang = new_ang;

			// send half of the polys
			int primCount = m_nGridX*m_nGridY*2 / 2;  // in this case, to draw HALF the polys
			
			int src_idx_offset = half * primCount*3;
			{
				int i=0;
                for (int prim = 0; prim < primCount; prim++)
				{
					// copy 3 verts
					for (int j=0; j<3; j++)
                    {
						tempv[i++] = m_verts[ m_indices_list[src_idx_offset++] ];
                    }
				}

                m_context->Draw(PRIMTYPE_TRIANGLELIST, primCount, tempv );
			}
		}
#elif 0
        
        for (int half=0; half<2; half++)
        {
            // hack / restore the ang values along the angle-wrap [0 <-> 2pi] seam...
            float new_ang = half ? M_PI : -M_PI;
            int y_offset = (m_nGridY/2) * (m_nGridX + 1);
            for (int x=0; x<m_nGridX/2; x++)
                m_verts[y_offset + x].ang = new_ang;
            
            // send half of the polys
            int primCount = m_nGridX*m_nGridY*2 / 2;  // in this case, to draw HALF the polys
            int src_idx_offset = half * primCount*3;
 

            m_context->UploadIndexData((int)m_indices_list.size(), m_indices_list.data());
            m_context->UploadVertexData((int)m_verts.size(), m_verts.data());
            m_context->DrawIndexed(PRIMTYPE_TRIANGLELIST, src_idx_offset,  primCount * 3 );
        }
#else
        
        {
            int y_offset_neg = (m_nGridY/2) * (m_nGridX + 1);
            int width = m_nGridX / 2;
            int y_offset_pos = (int)m_verts.size() - width;
            
            // mirror ang across seam
            for (int x=0; x < width; x++)
            {
                Vertex v = m_verts[y_offset_neg + x];
                v.ang = -M_PI;
                m_verts[y_offset_neg + x] = v;
                v.ang =  M_PI;
                m_verts[y_offset_pos + x] = v;
            }

            m_context->UploadIndexData(m_indices_list_wrapped);
            m_context->UploadVertexData(m_verts);
            m_context->DrawIndexed(PRIMTYPE_TRIANGLELIST, 0,  (int)m_indices_list_wrapped.size()  );
        }
        
    
        
        
        
#endif
        
        
	}

	m_context->SetBlendDisable();
    SetFixedShader(nullptr);
}

void CPlugin::DrawCustomShapes_ComputeBegin()
{
    if (!m_bBlending)
    {
        m_pState->DrawCustomShapes_ComputeBegin();
    }
    else
    {
        m_pState->DrawCustomShapes_ComputeBegin();
        m_pOldState->DrawCustomShapes_ComputeBegin();
    }

}

void CPlugin::DrawCustomShapes_ComputeEnd()
{
    if (!m_bBlending)
    {
        m_pState->DrawCustomShapes_ComputeEnd(1.0f);
    }
    else
    {
        m_pState->DrawCustomShapes_ComputeEnd(m_fBlendProgress);
        m_pOldState->DrawCustomShapes_ComputeEnd( (1-m_fBlendProgress));
    }
}

#define SHAPE_BUFFER 1

void CShape::Draw_ComputeBegin(CState *pState)
{
    PROFILE_FUNCTION();

#if SHAPE_BUFFER
    m_perframe_buffer->Resize(instances);
    
    
    for (int instance=0; instance<instances; instance++)
    {
        // 1. execute per-frame code
        LoadCustomShapePerFrameEvallibVars(pState);
        
        // set instance
        var_pf_instance = instance;
        
        m_perframe_buffer->StoreJob(instance);
        
    }
    
    m_perframe_buffer->ExecuteAsync();
    
#endif
}



void CShape::Draw_ComputeEnd(class CPlugin *plugin, CState * pState, float alpha_mult)
{
    PROFILE_FUNCTION();

    float fAspectY = plugin->m_fAspectY;

    m_perframe_buffer->Sync();

    // all shapes use the same shader...
    auto context = plugin->m_context;
    auto shader = plugin->m_shader_custom_shapes;
    auto sampler = shader->GetSampler(0);
    context->SetShader(shader);
    
    IDrawBufferPtr draw = plugin->GetDrawBuffer();
    
    bool is_textured = false;
	if (sampler)
		sampler->SetTexture(nullptr);
    
    for (int instance=0; instance<instances; instance++)
    {
#if !SHAPE_BUFFER
        // 1. execute per-frame code
        LoadCustomShapePerFrameEvallibVars(pState);
        
        // set instance
        var_pf_instance = instance;

        if (m_perframe_expression)
        {
            m_perframe_expression->Execute();
        }
#else
        m_perframe_buffer->ReadJob(instance);

#endif
        
        int sides = (int)(var_pf_sides);
        if (sides<3) sides=3;
        if (sides>100) sides=100;
        
        
        bool additive = ((int)(var_pf_additive) != 0);
        context->SetBlend(BLEND_SRCALPHA, additive ? BLEND_ONE : BLEND_INVSRCALPHA);
        
        
        float x0 = (float)(var_pf_x* 2-1);// * ASPECT;
        float y0 = (float)(var_pf_y*-2+1);
        
        auto rgba = COLOR_ARGB(
                               var_pf_rgba.a* alpha_mult,
                               var_pf_rgba.r,
                               var_pf_rgba.g,
                               var_pf_rgba.b);
        auto rgba2 =  COLOR_ARGB(
                                 var_pf_rgba2.a * alpha_mult,
                                 var_pf_rgba2.r,
                                 var_pf_rgba2.g,
                                 var_pf_rgba2.b
                                 );
        
        Vertex v[512];  // for textured shapes (has texcoords)
        
        // setup first vertex
        v[0].x = x0;
        v[0].y = y0;
        v[0].z = 0;
        v[0].tu = 0.5f;
        v[0].tv = 0.5f;
        v[0].rad = v[0].ang = 0.0f;
        v[0].Diffuse = rgba;

        
        float rad = (float)var_pf_rad;
        float ang = (float)var_pf_ang;
        float tex_ang  = (float)var_pf_tex_ang;
        float tex_zoom = (float)var_pf_tex_zoom;

        for (int j=1; j<sides+1; j++)
        {
            float t = (j-1)/(float)sides;
            v[j].x = x0 + rad*cosf(t*M_PI *2 + ang + M_PI *0.25f)* fAspectY;  // DON'T TOUCH!
            v[j].y = y0 + rad*sinf(t*M_PI *2 + ang + M_PI *0.25f);           // DON'T TOUCH!
            v[j].z = 0;
            v[j].tu = 0.5f + 0.5f*cosf(t*M_PI *2 + tex_ang + M_PI *0.25f)/(tex_zoom) * fAspectY; // DON'T TOUCH!
            v[j].tv = 0.5f + 0.5f*sinf(t*M_PI *2 + tex_ang + M_PI *0.25f)/(tex_zoom);     // DON'T TOUCH!
            v[j].rad = v[j].ang = 0.0f;
            v[j].Diffuse = rgba2;
        }
        v[sides+1] = v[1];
        
        // did textured state change?
        if ((int)(var_pf_textured) != 0)
        {
            if (!is_textured)
            {
                draw->Flush();
                // draw textured version
				if (sampler)
                sampler->SetTexture(plugin->m_lpVS[0]);
                is_textured = true;
            }
        }
        else
        {
            if (is_textured)
            {
                draw->Flush();
                // no texture
				if (sampler)
	                sampler->SetTexture(nullptr);
                is_textured = false;
            }
        }

        draw->DrawTriangleFan(v, sides + 2);
        
        // DRAW BORDER
        if (var_pf_border.a > 0)
        {
            if (is_textured)
            {
               draw->Flush();
               // no texture
			   if (sampler)
               sampler->SetTexture(nullptr);
               is_textured = false;
            }

            v[0].Diffuse =
            COLOR_ARGB(
                       var_pf_border.a * alpha_mult,
                       var_pf_border.r,
                       var_pf_border.g,
                       var_pf_border.b
                       );
            for (int j=1; j<sides+2; j++)
            {
                v[j].Diffuse = v[0].Diffuse;
            }
            
            bool thick = ((int)(var_pf_thick) != 0);
            draw->DrawLineStrip(sides + 1, &v[1], thick);
            
        }
        
    }
    
    draw->Flush();

    plugin->SetFixedShader(nullptr);
}

void CState::DrawCustomShapes_ComputeBegin()
{
    for (auto shape : m_shapes)
    {
        StopWatch sw = StopWatch::StartNew();
        if (shape->enabled)
        {
            shape->Draw_ComputeBegin(this);
        }
        shape->time_compute_begin = sw.GetElapsedMilliSeconds();
    }
}


void CState::DrawCustomShapes_ComputeEnd(float alpha_mult)
{
    for (auto shape : m_shapes)
    {
        StopWatch sw = StopWatch::StartNew();

        if (shape->enabled)
        {
            shape->Draw_ComputeEnd(m_plugin, this, alpha_mult);
        }

        shape->time_compute_end = sw.GetElapsedMilliSeconds();
    }

    m_plugin->SetFixedShader(nullptr);
	m_plugin->m_context->SetBlendDisable();
}


void CShape::LoadCustomShapePerFrameEvallibVars(CState *pState)
{
    var_pf_common  = pState->GetCommonVars();
	for (int vi=0; vi<NUM_Q_VAR; vi++)
		var_pf_q[vi] = pState->var_pf_q[vi];
	for (int vi=0; vi<NUM_T_VAR; vi++)
		var_pf_t[vi] = t_values_after_init_code[vi];
	var_pf_x     = x;
	var_pf_y     = y;
	var_pf_rad      = rad;
	var_pf_ang      = ang;
	var_pf_tex_zoom = tex_zoom;
	var_pf_tex_ang  = tex_ang;
	var_pf_sides    = sides;
	var_pf_additive = additive;
	var_pf_textured = textured;
	var_pf_instances = instances;
	var_pf_instance = 0;
	var_pf_thick    = thickOutline;
	var_pf_rgba.r     = rgba.r;
	var_pf_rgba.g        = rgba.g;
	var_pf_rgba.b        = rgba.b;
	var_pf_rgba.a        = rgba.a;
	var_pf_rgba2.r       = rgba2.r;
	var_pf_rgba2.g       = rgba2.g;
	var_pf_rgba2.b       = rgba2.b;
	var_pf_rgba2.a       = rgba2.a;
	var_pf_border.r = border.r;
	var_pf_border.g = border.g;
	var_pf_border.b = border.b;
	var_pf_border.a = border.a;
}



void CWave::LoadCustomWavePerFrameEvallibVars(CState * pState)
{
    var_pf_common  = pState->GetCommonVars();
	for (int vi=0; vi<NUM_Q_VAR; vi++)
		var_pf_q[vi] = pState->var_pf_q[vi];
	for (int vi=0; vi<NUM_T_VAR; vi++)
		var_pf_t[vi] = t_values_after_init_code[vi];
	var_pf_rgba.r         = rgba.r;
	var_pf_rgba.g         = rgba.g;
	var_pf_rgba.b         = rgba.b;
	var_pf_rgba.a         = rgba.a;
	var_pf_samples        = samples;
}

// does a better-than-linear smooth on a wave.  Roughly doubles the # of points.
#if 0
static int SmoothWave(const Vertex* vi, int nVertsIn, Vertex* vo)
{
	const float c1 = -0.15f;
	const float c2 = 1.15f;
	const float c3 = 1.15f;
	const float c4 = -0.15f;
	const float inv_sum = 1.0f/(c1+c2+c3+c4);

	int j = 0;

	int i_below = 0;
	int i_above;
	int i_above2 = 1;
	for (int i=0; i<nVertsIn-1; i++)
	{
		i_above = i_above2;
		i_above2 = std::min(nVertsIn-1,i+2);
		vo[j] = vi[i];
		vo[j+1].x = (c1*vi[i_below].x + c2*vi[i].x + c3*vi[i_above].x + c4*vi[i_above2].x)*inv_sum;
		vo[j+1].y = (c1*vi[i_below].y + c2*vi[i].y + c3*vi[i_above].y + c4*vi[i_above2].y)*inv_sum;
		vo[j+1].z = 0;
		vo[j+1].Diffuse = vi[i].Diffuse;//0xFFFF0080;
		i_below = i;
		j += 2;
	}
	vo[j++] = vi[nVertsIn-1];

	return j;
}
#endif

static void SmoothWave(const Vertex* vi, int nVertsIn, std::vector<Vertex> &vo)
{
    if (nVertsIn <= 0) return;
    const float c1 = -0.15f;
    const float c2 = 1.15f;
    const float c3 = 1.15f;
    const float c4 = -0.15f;
    const float inv_sum = 1.0f/(c1+c2+c3+c4);
    
    vo.reserve(nVertsIn * 2 + 8);

    int i_below = 0;
    int i_above;
    int i_above2 = 1;
    for (int i=0; i<nVertsIn-1; i++)
    {
        i_above = i_above2;
        i_above2 = std::min(nVertsIn-1,i+2);
        
        Vertex v1 = vi[i];
        
        vo.push_back(v1);
        
        Vertex v2 = v1;
        v2.x = (c1*vi[i_below].x + c2*vi[i].x + c3*vi[i_above].x + c4*vi[i_above2].x)*inv_sum;
        v2.y = (c1*vi[i_below].y + c2*vi[i].y + c3*vi[i_above].y + c4*vi[i_above2].y)*inv_sum;
        vo.push_back(v2);
        
        i_below = i;
    }
    vo.push_back(vi[nVertsIn-1]);
}

void CPlugin::DrawCustomWaves_ComputeBegin()
{
    PROFILE_FUNCTION();

    m_pState->DrawCustomWaves_ComputeBegin();

    if (m_bBlending)
    {
         m_pOldState->DrawCustomWaves_ComputeBegin();
    }
}

void CPlugin::DrawCustomWaves_ComputeEnd()
{
    PROFILE_FUNCTION();

    if (!m_bBlending)
    {
        m_pState->DrawCustomWaves_ComputeEnd(1.0f);
    }
    else
    {
        m_pState->DrawCustomWaves_ComputeEnd(m_fBlendProgress);
        m_pOldState->DrawCustomWaves_ComputeEnd((1-m_fBlendProgress));
    }

}

void CWave::Draw_ComputeBegin(class CPlugin *plugin, CState *pState)
{
    
    PROFILE_FUNCTION();
    
    // 1. execute per-frame code
    LoadCustomWavePerFrameEvallibVars(pState);
    
    // 2.a. do just a once-per-frame init for the *per-point* *READ-ONLY* variables
    //     (the non-read-only ones will be reset/restored at the start of each vertex)
    var_pp_common = var_pf_common;
    
    if (m_perframe_expression)
        m_perframe_expression->Execute();
    
    for (int vi=0; vi<NUM_Q_VAR; vi++)
        var_pp_q[vi] = var_pf_q[vi];
    for (int vi=0; vi<NUM_T_VAR; vi++)
        var_pp_t[vi] = var_pf_t[vi];
    
    int nSamples = (int)var_pf_samples;
    nSamples = std::min(512, nSamples);
    
    if ((nSamples >= 2) || (bUseDots && nSamples >= 1))
    {
    }
    else
    {
        return;
    }
    
    
    float tempdata[2][512];
    
    int max_samples = bSpectrum ? NUM_FREQUENCIES : NUM_WAVEFORM_SAMPLES;
    
    float mult = ((bSpectrum) ? 0.15f : 0.004f) * scaling * pState->m_fWaveScale.eval(-1);
    
    SampleBuffer<float> pdata1;
    SampleBuffer<float> pdata2;
    if (bSpectrum) {
        plugin->m_audio->RenderSpectrum(max_samples, pdata1, pdata2);
    } else {
        plugin->m_audio->RenderWaveForm(max_samples, pdata1, pdata2);
    }
    
    // initialize tempdata[2][512]
    {
    int j0 = (bSpectrum) ? 0 : (max_samples - nSamples)/2/**(1-bSpectrum)*/ - sep/2;
    int j1 = (bSpectrum) ? 0 : (max_samples - nSamples)/2/**(1-bSpectrum)*/ + sep/2;
    if (j0 < 0) j0 = 0;
    if (j1 < 0) j1 = 0;
    
    float t = (bSpectrum) ? (max_samples - sep)/(float)nSamples : 1;
    float mix1 = powf(smoothing*0.98f, 0.5f);  // lower exponent -> more default smoothing
    float mix2 = 1-mix1;
    // SMOOTHING:
    tempdata[0][0] = pdata1[j0];
    tempdata[1][0] = pdata2[j1];
    for (int j=1; j<nSamples; j++)
    {
        tempdata[0][j] = pdata1[(int)(j*t)+j0]*mix2 + tempdata[0][j-1]*mix1;
        tempdata[1][j] = pdata2[(int)(j*t)+j1]*mix2 + tempdata[1][j-1]*mix1;
    }
    // smooth again, backwards: [this fixes the asymmetry of the beginning & end..]
    for (int j=nSamples-2; j>=0; j--)
    {
        tempdata[0][j] = tempdata[0][j]*mix2 + tempdata[0][j+1]*mix1;
        tempdata[1][j] = tempdata[1][j]*mix2 + tempdata[1][j+1]*mix1;
    }
    // finally, scale to final size:
    for (int j=0; j<nSamples; j++)
    {
        tempdata[0][j] *= mult;
        tempdata[1][j] *= mult;
    }
    }
    // 2. for each point, execute per-point code
    
    
    m_perpoint_buffer->Resize(nSamples);
    
    // to do:
    //  -add any of the m_wave[i].xxx menu-accessible vars to the code?
    float j_mult = 1.0f/(float)(nSamples-1);
    for (int j=0; j<nSamples; j++)
    {
        float t = j*j_mult;
        float value1 = tempdata[0][j];
        float value2 = tempdata[1][j];
        var_pp_sample = t;
        var_pp_value1 = value1;
        var_pp_value2 = value2;
        var_pp_x      = 0.5f + value1;
        var_pp_y      = 0.5f + value2;
        var_pp_rgba      = var_pf_rgba;
        
        m_perpoint_buffer->StoreJob(j);
    }
    
    
    m_perpoint_buffer->ExecuteAsync();
}


void CWave::Draw_ComputeEnd(class CPlugin *plugin, CState * pState, float alpha_mult)
{
    
    PROFILE_FUNCTION();
    
    auto context = plugin->m_context;
    auto shader = plugin->m_shader_custom_waves;
    auto sampler = shader->GetSampler(0);
    
    float fInvAspectX = plugin->m_fInvAspectX;
    float fInvAspectY = plugin->m_fInvAspectY;
    float nTexSizeX = (float)plugin->m_nTexSizeX;
 //   float nTexSizeY = (float)plugin->m_nTexSizeY;

    // wait for end
    m_perpoint_buffer->Sync();

    int nSamples = m_perpoint_buffer->GetSize();
    
    std::vector<Vertex> verts;
    verts.reserve(nSamples);

    for (int j=0; j<nSamples; j++)
    {
        m_perpoint_buffer->ReadJob(j);
        
        Vertex v;
        v.Clear();
        
        v.x = (float)(var_pp_x* 2-1)*fInvAspectX;
        v.y = (float)(var_pp_y*-2+1)*fInvAspectY;
        v.z = 0;
        v.Diffuse = COLOR_ARGB(
                                  var_pp_rgba.a * alpha_mult,
                                  var_pp_rgba.r,
                                  var_pp_rgba.g,
                                  var_pp_rgba.b
                                  );
        
        verts.push_back(v);
    }
    
    // 3. smooth it
    if (!bUseDots)
    {
        std::vector<Vertex> v2;
        SmoothWave(verts.data(), (int)verts.size(), v2);
        verts.swap(v2);
    }
   
    
    
    context->SetShader(shader);
	if (sampler)
    sampler->SetTexture(nullptr);

    // 4. draw it
    context->SetBlend(BLEND_SRCALPHA, bAdditive ? BLEND_ONE : BLEND_INVSRCALPHA);
    
    IDrawBufferPtr draw = plugin->GetDrawBuffer();
    if (bUseDots)
    {
        float ptsize = (float)((nTexSizeX >= 1024) ? 2 : 1) + (bDrawThick ? 1 : 0);
        draw->DrawPointsWithSize((int)verts.size(), verts.data(), ptsize);
    }
    else
    {
        draw->DrawLineStrip((int)verts.size(), verts.data(), bDrawThick);
    }
    
    draw->Flush();
    
    
    
    context->SetBlendDisable();
}



void CState::DrawCustomWaves_ComputeBegin()
{
    for (auto wave : m_waves)
    {
        StopWatch wave_sw = StopWatch::StartNew();
        if (wave->enabled)
        {
            wave->Draw_ComputeBegin(m_plugin, this);
        }
        wave->time_compute_begin = wave_sw.GetElapsedMilliSeconds();
    }
}

void CState::DrawCustomWaves_ComputeEnd(float alpha_mult)
{
    for (auto wave : m_waves)
    {
        StopWatch wave_sw = StopWatch::StartNew();
        if (wave->enabled)
        {
            wave->Draw_ComputeEnd(m_plugin, this, alpha_mult);
        }
        wave->time_compute_end = wave_sw.GetElapsedMilliSeconds();
    }

}


void CPlugin::RenderWaveForm(
                             int sampleCount,
                             SampleBuffer<Sample> &samples
                             )

{
    m_audio->RenderWaveForm(
                            sampleCount,
                            samples
                            );
    
    float fWaveScale = m_pState->m_fWaveScale.eval(GetTime());
    samples.Modulate(fWaveScale);

    float fWaveSmoothing = m_pState->m_fWaveSmoothing.eval(GetTime());;
    samples.Smooth(fWaveSmoothing);
}


void CPlugin::RenderSpectrum(
                             int sampleCount,
                             SampleBuffer<float> &fSpectrum
                             )

{
    m_audio->RenderSpectrum(sampleCount, fSpectrum);
}


void CPlugin::DrawWave()
{
    
    PROFILE_FUNCTION();

	SetFixedShader(nullptr);

	Vertex v1[MAX_SAMPLES+1], v2[MAX_SAMPLES+1];

	m_context->SetBlend(BLEND_SRCALPHA, (m_pState->var_pf_wave_additive) ? BLEND_ONE : BLEND_INVSRCALPHA);

	//float cr = m_pState->m_waveR.eval(GetTime());
	//float cg = m_pState->m_waveG.eval(GetTime());
	//float cb = m_pState->m_waveB.eval(GetTime());
	float cr = (float)(m_pState->var_pf_wave_rgba.r);
	float cg = (float)(m_pState->var_pf_wave_rgba.g);
	float cb = (float)(m_pState->var_pf_wave_rgba.b);
	float cx = (float)(m_pState->var_pf_wave_x);
	float cy = (float)(m_pState->var_pf_wave_y); // note: it was backwards (top==1) in the original milkdrop, so we keep it that way!
	float fWaveParam = (float)(m_pState->var_pf_wave_mystery);

	/*if (m_pState->m_bBlending)
	{
		cr = cr*(m_pState->m_fBlendProgress) + (1.0f-m_pState->m_fBlendProgress)*((float)(m_pOldState->var_pf_wave_r));
		cg = cg*(m_pState->m_fBlendProgress) + (1.0f-m_pState->m_fBlendProgress)*((float)(m_pOldState->var_pf_wave_g));
		cb = cb*(m_pState->m_fBlendProgress) + (1.0f-m_pState->m_fBlendProgress)*((float)(m_pOldState->var_pf_wave_b));
		cx = cx*(m_pState->m_fBlendProgress) + (1.0f-m_pState->m_fBlendProgress)*((float)(m_pOldState->var_pf_wave_x));
		cy = cy*(m_pState->m_fBlendProgress) + (1.0f-m_pState->m_fBlendProgress)*((float)(m_pOldState->var_pf_wave_y));
		fWaveParam = fWaveParam*(m_pState->m_fBlendProgress) + (1.0f-m_pState->m_fBlendProgress)*((float)(m_pOldState->var_pf_wave_mystery));
	}*/

	if (cr < 0) cr = 0;
	if (cg < 0) cg = 0;
	if (cb < 0) cb = 0;
	if (cr > 1) cr = 1;
	if (cg > 1) cg = 1;
	if (cb > 1) cb = 1;

	// maximize color:
	if (m_pState->var_pf_wave_brighten)
	{
		float fMaximizeWaveColorAmount = 1.0f;
		float max = cr;
		if (max < cg) max = cg;
		if (max < cb) max = cb;
		if (max > 0.01f)
		{
			cr = cr/max*fMaximizeWaveColorAmount + cr*(1.0f - fMaximizeWaveColorAmount);
			cg = cg/max*fMaximizeWaveColorAmount + cg*(1.0f - fMaximizeWaveColorAmount);
			cb = cb/max*fMaximizeWaveColorAmount + cb*(1.0f - fMaximizeWaveColorAmount);
		}
	}

	float fWavePosX = cx*2.0f - 1.0f; // go from 0..1 user-range to -1..1 D3D range
	float fWavePosY = cy*2.0f - 1.0f;

//	float bass_rel   =  GetImmRel(0);
//	float mid_rel    =  GetImmRel(1);
	float treble_rel = GetImmRel(BAND_TREBLE);

//	int sample_offset = 0;
	int new_wavemode = (int)(m_pState->var_pf_wave_mode) % NUM_WAVES;  // since it can be changed from per-frame code!

	int its = (m_bBlending && (new_wavemode != m_pState->m_nOldWaveMode)) ? 2 : 1;
	int nVerts1 = 0;
	int nVerts2 = 0;
	int nBreak1 = -1;
	int nBreak2 = -1;
	float alpha1=0, alpha2=0;

    
	for (int it=0; it<its; it++)
	{
		int   wave   = (it==0) ? new_wavemode : m_pState->m_nOldWaveMode;
		int   nVerts = NUM_WAVEFORM_SAMPLES;		// allowed to peek ahead 64 (i.e. left is [i], right is [i+64])
		int   nBreak = -1;
        
//                nVerts = 16;

//        nVerts = 32;

		float fWaveParam2 = fWaveParam;
		//std::string fWaveParam; // kill its scope
		if ((wave==0 || wave==1 || wave==4) && (fWaveParam2 < -1 || fWaveParam2 > 1))
		{
			//fWaveParam2 = max(fWaveParam2, -1.0f);
			//fWaveParam2 = min(fWaveParam2,  1.0f);
			fWaveParam2 = fWaveParam2*0.5f + 0.5f;
			fWaveParam2 -= floorf(fWaveParam2);
			fWaveParam2 = fabsf(fWaveParam2);
			fWaveParam2 = fWaveParam2*2-1;
		}

		Vertex *v = (it==0) ? v1 : v2;
		memset(v, 0, sizeof(v[0])*nVerts);

		float alpha = (float)(m_pState->var_pf_wave_rgba.a);//m_pState->m_fWaveAlpha.eval(GetTime());
        
        if (m_pState->m_bModWaveAlphaByVolume)
            alpha *= (GetImmRelTotal() - m_pState->m_fModWaveAlphaStart.eval(GetTime()))/(m_pState->m_fModWaveAlphaEnd.eval(GetTime()) - m_pState->m_fModWaveAlphaStart.eval(GetTime()));
        
        
		//wave = 4;
		switch(wave)
		{
		case 0:
			// circular wave

			nVerts /= 2;
//			sample_offset = (NUM_WAVEFORM_SAMPLES-nVerts)/2;//mysound.GoGoAlignatron(nVerts * 12/10);	// only call this once nVerts is final!

			{
                SampleBuffer<Sample> samples;
                int pad = nVerts / 10;
                RenderWaveForm(nVerts + pad, samples);
                
				float inv_nverts_minus_one = 1.0f/(float)(nVerts-1);

				for (int i=0; i<nVerts; i++)
				{
					float rad = 0.5f + 0.4f*samples[i].volume() + fWaveParam2;
					float ang = (i)*inv_nverts_minus_one*6.28f + GetTime()*0.2f;
					if (i < pad)
					{
						float mix = (float)i / (float)(pad);
						mix = 0.5f - 0.5f*cosf(mix * M_PI);
						float rad_2 = 0.5f + 0.4f*samples[i + nVerts].volume() + fWaveParam2;
						rad = rad_2*(1.0f-mix) + rad*(mix);
					}
					v[i].x = rad*cosf(ang) *m_fAspectY + fWavePosX;		// 0.75 = adj. for aspect ratio
					v[i].y = rad*sinf(ang) *m_fAspectX + fWavePosY;
					//v[i].Diffuse = color;
				}
			}

			// dupe last vertex to connect the lines; skip if blending
			if (!m_bBlending)
			{
//				nVerts++;
                v[nVerts-1] = v[0];
			}

			break;

		case 1:
			// x-y osc. that goes around in a spiral, in time
            {

			alpha *= 1.25f;

			nVerts /= 2;	

            SampleBuffer<Sample> samples;
            RenderWaveForm(nVerts + 32, samples);

                
			for (int i=0; i<nVerts; i++)
			{
				float rad = 0.53f + 0.43f*samples[i].right() + fWaveParam2;
				float ang = samples[i+32].left() * 1.57f + GetTime()*2.3f;
				v[i].x = rad*cosf(ang) *m_fAspectY + fWavePosX;		// 0.75 = adj. for aspect ratio
				v[i].y = rad*sinf(ang) *m_fAspectX + fWavePosY;
				//v[i].Diffuse = color;//(COLOR_RGBA_01(cr, cg, cb, alpha*min(1, max(0, fL[i])));
			}
            }

			break;
			
		case 2:
			// centered spiro (alpha constant)
			//	 aimed at not being so sound-responsive, but being very "nebula-like"
			//   difference is that alpha is constant (and faint), and waves a scaled way up

			switch(m_nTexSizeX)
			{
			case 256:  alpha *= 0.07f; break;
			case 512:  alpha *= 0.09f; break;
			case 1024: alpha *= 0.11f; break;
			case 2048: alpha *= 0.13f; break;
			case 4096: alpha *= 0.15f; break;
			}

            {
                SampleBuffer<Sample> samples;
                RenderWaveForm(nVerts + 32, samples);

			for (int i=0; i<nVerts; i++)
			{
				v[i].x = samples[i   ].left()  *m_fAspectY + fWavePosX;//((pR[i] ^ 128) - 128)/90.0f * ASPECT; // 0.75 = adj. for aspect ratio
				v[i].y = samples[i+32].right() *m_fAspectX + fWavePosY;//((pL[i+32] ^ 128) - 128)/90.0f;
				//v[i].Diffuse = color;
			}
            }

			break;
		case 3:
			// centered spiro (alpha tied to volume)
			//	 aimed at having a strong audio-visual tie-in
			//   colors are always bright (no darks)

			switch(m_nTexSizeX)
			{
			case 256:  alpha = 0.075f; break;
			case 512:  alpha = 0.150f; break;
			case 1024: alpha = 0.220f; break;
			case 2048: alpha = 0.330f; break;
			case 4096: alpha = 0.400f; break;
			}

			alpha *= 1.3f;
			alpha *= powf(treble_rel, 2.0f);

            {
                SampleBuffer<Sample> samples;
                RenderWaveForm(nVerts + 32, samples);

			for (int i=0; i<nVerts; i++)
			{
				v[i].x = samples[i   ].left()  *m_fAspectY + fWavePosX;//((pR[i] ^ 128) - 128)/90.0f * ASPECT; // 0.75 = adj. for aspect ratio
				v[i].y = samples[i+32].right() *m_fAspectX + fWavePosY;//((pL[i+32] ^ 128) - 128)/90.0f;
				//v[i].Diffuse = color;
			}
            }
			break;
		case 4:
			// horizontal "script", left channel

			//sample_offset = (NUM_WAVEFORM_SAMPLES-nVerts)/2;//mysound.GoGoAlignatron(nVerts + 25);	// only call this once nVerts is final!


			{
                SampleBuffer<Sample> samples;
                RenderWaveForm(nVerts + 25, samples);

                
				float w1 = 0.45f + 0.5f*(fWaveParam2*0.5f + 0.5f);		// 0.1 - 0.9
				float w2 = 1.0f - w1;

				float inv_nverts = 1.0f/(float)(nVerts-1);

				for (int i=0; i<nVerts; i++)
				{
					v[i].x = -1.0f + 2.0f*(i*inv_nverts) + fWavePosX;
					v[i].y = samples[i].left() *0.47f + fWavePosY;//((pL[i] ^ 128) - 128)/270.0f;
					v[i].x += samples[i+25].right() *0.44f;//((pR[i+25] ^ 128) - 128)/290.0f;
					//v[i].Diffuse = color;

					// momentum
					if (i>1)
					{
						v[i].x = v[i].x*w2 + w1*(v[i-1].x*2.0f - v[i-2].x);
						v[i].y = v[i].y*w2 + w1*(v[i-1].y*2.0f - v[i-2].y);
					}
				}

				/*
				// center on Y
				float avg_y = 0;
				for (i=0; i<nVerts; i++) 
					avg_y += v[i].y;
				avg_y /= (float)nVerts;
				avg_y *= 0.5f;		// damp the movement
				for (i=0; i<nVerts; i++) 
					v[i].y -= avg_y;
				*/
			}

			break;

		case 5:
			// weird explosive complex # thingy

			switch(m_nTexSizeX)
			{
			case 256:  alpha *= 0.07f; break;
			case 512:  alpha *= 0.09f; break;
			case 1024: alpha *= 0.11f; break;
			case 2048: alpha *= 0.13f; break;
			case 4096: alpha *= 0.15f; break;
			}
			
			{
                SampleBuffer<Sample> samples;
                RenderWaveForm(nVerts + 32, samples);

				float cos_rot = cosf(GetTime()*0.3f);
				float sin_rot = sinf(GetTime()*0.3f);

				for (int i=0; i<nVerts; i++)
				{
                    float l0 = samples[i].left();
                    float r0 = samples[i].right();
                    float l1 = samples[i+32].left();
                    float r1 = samples[i+32].right();

					float x0 = (r0*l1 + l0*r1);
					float y0 = (r0*r0 - l1*l1);
					v[i].x = (x0*cos_rot - y0*sin_rot)*m_fAspectY + fWavePosX;
					v[i].y = (x0*sin_rot + y0*cos_rot)*m_fAspectX + fWavePosY;
					//v[i].Diffuse = color;
				}
			}

			break;

		case 6:
		case 7:
		case 8:
			// 6: angle-adjustable left channel, with temporal wave alignment;
			//   fWaveParam2 controls the angle at which it's drawn
			//	 fWavePosX slides the wave away from the center, transversely.
			//   fWavePosY does nothing
			//
			// 7: same, except there are two channels shown, and
			//   fWavePosY determines the separation distance.
			// 
			// 8: same as 6, except using the spectrum analyzer (UNFINISHED)
			// 
			nVerts /= 2;

			if (wave==8)
				nVerts = 256;
//			else
//				sample_offset = (NUM_WAVEFORM_SAMPLES-nVerts)/2;//mysound.GoGoAlignatron(nVerts);	// only call this once nVerts is final!

			{
				float ang = 1.57f*fWaveParam2;	// from -PI/2 to PI/2
				float dx  = cosf(ang);
				float dy  = sinf(ang);

				float edge_x[2], edge_y[2];

				//edge_x[0] = fWavePosX - dx*3.0f;
				//edge_y[0] = fWavePosY - dy*3.0f;
				//edge_x[1] = fWavePosX + dx*3.0f;
				//edge_y[1] = fWavePosY + dy*3.0f;
				edge_x[0] = fWavePosX*cosf(ang + 1.57f) - dx*3.0f;
				edge_y[0] = fWavePosX*sinf(ang + 1.57f) - dy*3.0f;
				edge_x[1] = fWavePosX*cosf(ang + 1.57f) + dx*3.0f;
				edge_y[1] = fWavePosX*sinf(ang + 1.57f) + dy*3.0f;

				for (int i=0; i<2; i++)	// for each point defining the line
				{
					// clip the point against 4 edges of screen
					// be a bit lenient (use +/-1.1 instead of +/-1.0) 
					//	 so the dual-wave doesn't end too soon, after the channels are moved apart
					for (int j=0; j<4; j++)
					{
						float t = 0;
						bool bClip = false;

						switch(j)
						{
						case 0:
							if (edge_x[i] > 1.1f)
							{
								t = (1.1f - edge_x[1-i]) / (edge_x[i] - edge_x[1-i]);
								bClip = true;
							}
							break;
						case 1:
							if (edge_x[i] < -1.1f)
							{
								t = (-1.1f - edge_x[1-i]) / (edge_x[i] - edge_x[1-i]);
								bClip = true;
								}
							break;
						case 2:
							if (edge_y[i] > 1.1f)
							{
								t = (1.1f - edge_y[1-i]) / (edge_y[i] - edge_y[1-i]);
								bClip = true;
							}
							break;
						case 3:
							if (edge_y[i] < -1.1f)
							{
								t = (-1.1f - edge_y[1-i]) / (edge_y[i] - edge_y[1-i]);
								bClip = true;
							}
							break;
						}

						if (bClip)
						{
							float dx = edge_x[i] - edge_x[1-i];
							float dy = edge_y[i] - edge_y[1-i];
							edge_x[i] = edge_x[1-i] + dx*t;
							edge_y[i] = edge_y[1-i] + dy*t;
						}
					}
				}

				dx = (edge_x[1] - edge_x[0]) / (float)(nVerts-1);
				dy = (edge_y[1] - edge_y[0]) / (float)(nVerts-1);
				float ang2 = atan2f(dy,dx);
				float perp_dx = cosf(ang2 + 1.57f);
				float perp_dy = sinf(ang2 + 1.57f);

				if (wave == 6)
                {
                    SampleBuffer<Sample> samples;
                    RenderWaveForm(nVerts, samples);

					for (int i=0; i<nVerts; i++)
					{
                        float s = samples[i].volume();
						v[i].x = edge_x[0] + dx*i + perp_dx*0.25f*s;
						v[i].y = edge_y[0] + dy*i + perp_dy*0.25f*s;
						//v[i].Diffuse = color;
					}
                }
				else if (wave == 8)
                {
                    SampleBuffer<float> fSpectrum;
                    RenderSpectrum(nVerts, fSpectrum);
                    
                    //256 verts
					for (int i=0; i<nVerts; i++)
					{
						float f = 0.1f*logf(fSpectrum[i]);
                        v[i].x = edge_x[0] + dx*i + perp_dx*f;
						v[i].y = edge_y[0] + dy*i + perp_dy*f;
						//v[i].Diffuse = color;
					}
                }
				else
				{
                    SampleBuffer<Sample> samples;
                    RenderWaveForm(nVerts, samples);

					float sep = powf(fWavePosY*0.5f + 0.5f, 2.0f);
					for (int i=0; i<nVerts; i++)
					{
                        float s = samples[i].left();
						v[i].x = edge_x[0] + dx*i + perp_dx*(0.25f*s + sep);
						v[i].y = edge_y[0] + dy*i + perp_dy*(0.25f*s + sep);
						//v[i].Diffuse = color;
					}

					//D3DPRIMITIVETYPE primtype = (m_pState->var_pf_wave_usedots) ? D3DPT_POINTLIST : D3DPT_LINESTRIP;
					//m_lpD3DDev->DrawPrimitive(primtype, D3DFVF_LVERTEX, (LPVOID)v, nVerts, NULL);

					for (int i=0; i<nVerts; i++)
					{
                        float s = samples[i].right();
						v[i+nVerts].x = edge_x[0] + dx*i + perp_dx*(0.25f*s - sep);
						v[i+nVerts].y = edge_y[0] + dy*i + perp_dy*(0.25f*s - sep);
						//v[i+nVerts].Diffuse = color;
					}

					nBreak = nVerts;
					nVerts *= 2;
				}
			}

			break;

            case 9:
                // horizontal "script", left channel
                {
                    SampleBuffer<Sample> samples;
                    RenderWaveForm(nVerts, samples);

                    float inv_nverts = 1.0f / (float)(nVerts-1);
                    for (int i = 0; i < nVerts; i++)
                    {
                        v[i].x = -1.0f + 2.0f*(i*inv_nverts) + fWavePosX;
                        v[i].y = samples[i].volume() * 1 + fWavePosY;
                    }
                }
                break;

		}

        // clamp alpha
        if (alpha < 0) alpha = 0;
        if (alpha > 1) alpha = 1;
        
		if (it==0)
		{
			nVerts1 = nVerts;
			nBreak1 = nBreak;
			alpha1  = alpha;
		}
		else
		{
			nVerts2 = nVerts;
			nBreak2 = nBreak;
			alpha2  = alpha;
		}
	}	

	// v1[] is for the current waveform
	// v2[] is for the old waveform (from prev. preset - only used if blending)
	// nVerts1 is the # of vertices in v1
	// nVerts2 is the # of vertices in v2
	// nBreak1 is the index of the point at which to break the solid line in v1[] (-1 if no break)
	// nBreak2 is the index of the point at which to break the solid line in v2[] (-1 if no break)

	float mix = CosineInterp(m_fBlendProgress);
	float mix2 = 1.0f - mix;

	// blend 2 waveforms
	if (nVerts2 > 0)
	{
		// note: this won't yet handle the case where (nBreak1 > 0 && nBreak2 > 0)
		//       in this case, code must break wave into THREE segments
		float m = (nVerts2-1)/(float)nVerts1;
		float x,y;
		for (int i=0; i<nVerts1; i++)
		{
			float fIdx = i*m;
			int   nIdx = (int)fIdx;
			float t = fIdx - nIdx;
			if (nIdx == nBreak2-1)
			{
				x = v2[nIdx].x;
				y = v2[nIdx].y;
				nBreak1 = i+1;
			}
			else
			{
				x = v2[nIdx].x*(1-t) + v2[nIdx+1].x*(t);
				y = v2[nIdx].y*(1-t) + v2[nIdx+1].y*(t);
			}
			v1[i].x = v1[i].x*(mix) + x*(mix2);
			v1[i].y = v1[i].y*(mix) + y*(mix2);
		}
	}

	// determine alpha
	if (nVerts2 > 0)
	{
		alpha1 = alpha1*(mix) + alpha2*(1.0f-mix);
	}

	// apply color & alpha
	// ALSO reverse all y values, to stay consistent with the pre-VMS milkdrop,
	//  which DIDN'T:
    v1[0].Diffuse = COLOR_RGBA_01(cr, cg, cb, alpha1);
	for (int i=0; i<nVerts1; i++)
	{
		v1[i].Diffuse = v1[0].Diffuse;
		v1[i].y = -v1[i].y;
	}
	
	// don't draw wave if (possibly blended) alpha is less than zero.
	if (alpha1 < 0.004f) {
		m_context->SetBlendDisable();
		return;
	}

	// TESSELLATE - smooth the wave, one time.
	std::vector<Vertex> vTess;
    std::vector<Vertex> vTess2;
	if (1)
	{
		if (nBreak1==-1)
		{
			SmoothWave(v1, nVerts1, vTess);
		}
		else 
		{
			SmoothWave(v1, nBreak1, vTess);
			SmoothWave(&v1[nBreak1], nVerts1-nBreak1, vTess2);
		}
	}

	// draw primitives
    IDrawBufferPtr draw = GetDrawBuffer();
	{
		//D3DPRIMITIVETYPE primtype = (m_pState->var_pf_wave_usedots) ? D3DPT_POINTLIST : D3DPT_LINESTRIP;
		bool thick = ((m_pState->var_pf_wave_thick || m_pState->var_pf_wave_usedots) && (m_nTexSizeX >= 512));
        if (m_pState->var_pf_wave_usedots)
        {
            draw->DrawPoints((int)vTess.size(),  vTess.data(),  thick);
            if (!vTess2.empty())
                draw->DrawPoints((int)vTess2.size(), vTess2.data(), thick);
        }
        else
        {
            draw->DrawLineStrip((int)vTess.size(),  vTess.data(), thick);
            if (!vTess2.empty())
                draw->DrawLineStrip((int)vTess2.size(), vTess2.data(), thick);
        }
		
	}
    draw->Flush();


	m_context->SetBlendDisable();
}

void CPlugin::DrawSprites()
{
    PROFILE_FUNCTION()

	SetFixedShader(nullptr);

	if (m_pState->var_pf_darken_center)
	{
		m_context->SetBlend(BLEND_SRCALPHA, BLEND_INVSRCALPHA);

		Vertex v3[6];
		memset(v3, 0, sizeof(v3));

		// colors:
		v3[0].Diffuse = COLOR_RGBA_01(0, 0, 0, 3.0f/32.0f);
		v3[1].Diffuse = COLOR_RGBA_01(0, 0, 0, 0.0f/32.0f);
		v3[2].Diffuse = v3[1].Diffuse;
		v3[3].Diffuse = v3[1].Diffuse;
		v3[4].Diffuse = v3[1].Diffuse;
		v3[5].Diffuse = v3[1].Diffuse;

		// positioning:
		float fHalfSize = 0.05f;
		v3[0].x = 0.0f; 	
		v3[1].x = 0.0f - fHalfSize*m_fAspectY; 	
		v3[2].x = 0.0f; 	
		v3[3].x = 0.0f + fHalfSize*m_fAspectY; 	
		v3[4].x = 0.0f; 
		v3[5].x = v3[1].x;
		v3[0].y = 0.0f; 	
		v3[1].y = 0.0f;
		v3[2].y = 0.0f - fHalfSize; 	
		v3[3].y = 0.0f;
		v3[4].y = 0.0f + fHalfSize; 	
		v3[5].y = v3[1].y;
		//v3[0].tu = 0;	v3[1].tu = 1;	v3[2].tu = 0;	v3[3].tu = 1;
		//v3[0].tv = 1;	v3[1].tv = 1;	v3[2].tv = 0;	v3[3].tv = 0;

		m_context->DrawArrays(PRIMTYPE_TRIANGLEFAN, 6, v3);

		m_context->SetBlendDisable();
	}

	// do borders
	{
		float fOuterBorderSize = (float)m_pState->var_pf_ob_size;
		float fInnerBorderSize = (float)m_pState->var_pf_ib_size;

		m_context->SetBlend(BLEND_SRCALPHA, BLEND_INVSRCALPHA);

		for (int it=0; it<2; it++)
		{
			Vertex v3[4];
			memset(v3, 0, sizeof(v3));

			// colors:
			float r = (it==0) ? (float)m_pState->var_pf_ob_rgba.r : (float)m_pState->var_pf_ib_rgba.r;
			float g = (it==0) ? (float)m_pState->var_pf_ob_rgba.g : (float)m_pState->var_pf_ib_rgba.g;
			float b = (it==0) ? (float)m_pState->var_pf_ob_rgba.b : (float)m_pState->var_pf_ib_rgba.b;
			float a = (it==0) ? (float)m_pState->var_pf_ob_rgba.a : (float)m_pState->var_pf_ib_rgba.a;
			if (a > 0.001f)
			{
				v3[0].Diffuse = COLOR_RGBA_01(r,g,b,a);
				v3[1].Diffuse = v3[0].Diffuse;
				v3[2].Diffuse = v3[0].Diffuse;
				v3[3].Diffuse = v3[0].Diffuse;

				// positioning:
				float fInnerRad = (it==0) ? 1.0f - fOuterBorderSize : 1.0f - fOuterBorderSize - fInnerBorderSize;
				float fOuterRad = (it==0) ? 1.0f                    : 1.0f - fOuterBorderSize;
				v3[0].x =  fInnerRad;
				v3[1].x =  fOuterRad; 	
				v3[2].x =  fOuterRad;
				v3[3].x =  fInnerRad;
				v3[0].y =  fInnerRad;
				v3[1].y =  fOuterRad;
				v3[2].y = -fOuterRad;
				v3[3].y = -fInnerRad;

				for (int rot=0; rot<4; rot++)
				{
					m_context->DrawArrays(PRIMTYPE_TRIANGLEFAN, 4, v3);

					// rotate by 90 degrees
					for (int v=0; v<4; v++)
					{
						float t = 1.570796327f;
						float x = v3[v].x;
						float y = v3[v].y;
						v3[v].x = x*cosf(t) - y*sinf(t);
						v3[v].y = x*sinf(t) + y*cosf(t);
					}
				}
			}
		}
		m_context->SetBlendDisable();
	}
}

void CPlugin::UvToMathSpace(float u, float v, float* rad, float* ang)
{
	// (screen space = -1..1 on both axes; corresponds to UV space)
	// uv space = [0..1] on both axes
	// "math" space = what the preset authors are used to:
	//      upper left = [0,0]
	//      bottom right = [1,1]
	//      rad == 1 at corners of screen
	//      ang == 0 at three o'clock, and increases counter-clockwise (to 6.28).
	float px = (u*2-1) * m_fAspectX;  // probably 1.0
	float py = (v*2-1) * m_fAspectY;  // probably <1
	
	*rad = sqrtf(px*px + py*py) / sqrtf(m_fAspectX*m_fAspectX + m_fAspectY*m_fAspectY);
	*ang = atan2f(py, px);
	if (*ang < 0)
		*ang += M_PI * 2;
}

void CPlugin::ApplyShaderParams(ShaderInfoPtr p, CStatePtr pState)
{
    
    PROFILE_FUNCTION();
    
    m_context->SetShader(p->shader);
    
	// bind textures
    for (const auto &tb : p->_texture_bindings)
	{
        if (!tb.shader_sampler)
            continue;
        tex_code texcode = tb.texcode;
        
        TexturePtr texture;
        switch (texcode)
        {
            case TEX_VS:
                texture = m_lpVS[0];
                break;
            case TEX_BLUR1:
                texture = m_blur_textures[0];
                break;
            case TEX_BLUR2:
                texture = m_blur_textures[1];
                break;
            case TEX_BLUR3:
                texture = m_blur_textures[2];
                break;
            case TEX_DISK:
                texture = tb.texptr;
                break;
            default:
                assert(0);
                texture= nullptr;
                break;
        }

   


		// also set up sampler stage, if anything is bound here...
		{
			bool bAniso = false;  
			SamplerFilter HQFilter = bAniso ? SAMPLER_ANISOTROPIC : SAMPLER_LINEAR;
			SamplerAddress wrap   = tb.bWrap ? SAMPLER_WRAP : SAMPLER_CLAMP;
			SamplerFilter filter = tb.bBilinear ? HQFilter : SAMPLER_POINT;
            if (tb.shader_sampler) tb.shader_sampler->SetTexture(texture, wrap, filter);
		}
	}

	// bind "texsize_XYZ" params
    for (const  auto &q : p->texsize_params)
	{
		q.texsize_param->SetVector(Vector4((float)q.w,(float)q.h,1.0f/q.w,1.0f/q.h));
	}

	float time_since_preset_start = m_PresetDuration;
	float time_since_preset_start_wrapped = time_since_preset_start - (int)(time_since_preset_start/10000)*10000;
	float time = GetTime();
	float progress = GetPresetProgress();
	float mip_x = logf((float)GetWidth())/logf(2.0f);
	float mip_y = logf((float)GetWidth())/logf(2.0f);
	float mip_avg = 0.5f*(mip_x + mip_y);
	float aspect_x = 1;
	float aspect_y = 1;
	if (GetWidth() > GetHeight())
		aspect_y = GetHeight()/(float)GetWidth();
	else
		aspect_x = GetWidth()/(float)GetHeight();
	
	float blur_min[3], blur_max[3];
	pState->GetSafeBlurMinMax(blur_min, blur_max);

	// bind float4's
	if (p->rand_frame ) p->rand_frame->SetVector( m_rand_frame );
	if (p->rand_preset) p->rand_preset->SetVector( pState->m_rand_preset );
	ShaderConstantPtr* h = p->const_handles; 
	if (h[0]) h[0]->SetVector( Vector4( aspect_x, aspect_y, 1.0f/aspect_x, 1.0f/aspect_y ));
	if (h[1]) h[1]->SetVector( Vector4(0, 0, 0, 0 ));
	if (h[2]) h[2]->SetVector( Vector4(time_since_preset_start_wrapped, GetFps(), (float)GetFrame(), progress));
	if (h[3]) h[3]->SetVector( Vector4(GetImmRel(BAND_BASS), GetImmRel(BAND_MID), GetImmRel(BAND_TREBLE), GetImmRelTotal() ));
	if (h[4]) h[4]->SetVector( Vector4(GetAvgRel(BAND_BASS), GetAvgRel(BAND_MID), GetAvgRel(BAND_TREBLE), GetAvgRelTotal() ));
	if (h[5]) h[5]->SetVector( Vector4( blur_max[0]-blur_min[0], blur_min[0], blur_max[1]-blur_min[1], blur_min[1] ));
	if (h[6]) h[6]->SetVector( Vector4( blur_max[2]-blur_min[2], blur_min[2], blur_min[0], blur_max[0] ));
	if (h[7]) h[7]->SetVector( Vector4((float)m_nTexSizeX, (float)m_nTexSizeY, 1.0f/(float)m_nTexSizeX, 1.0f/(float)m_nTexSizeY ));
	if (h[8]) h[8]->SetVector( Vector4( 0.5f+0.5f*cosf(time* 0.329f+1.2f),
															  0.5f+0.5f*cosf(time* 1.293f+3.9f), 
															  0.5f+0.5f*cosf(time* 5.070f+2.5f), 
															  0.5f+0.5f*cosf(time*20.051f+5.4f) 
		));
	if (h[9]) h[9]->SetVector( Vector4( 0.5f+0.5f*sinf(time* 0.329f+1.2f),
															  0.5f+0.5f*sinf(time* 1.293f+3.9f), 
															  0.5f+0.5f*sinf(time* 5.070f+2.5f), 
															  0.5f+0.5f*sinf(time*20.051f+5.4f) 
		));
	if (h[10]) h[10]->SetVector( Vector4( 0.5f+0.5f*cosf(time*0.0050f+2.7f),
																0.5f+0.5f*cosf(time*0.0085f+5.3f), 
																0.5f+0.5f*cosf(time*0.0133f+4.5f), 
																0.5f+0.5f*cosf(time*0.0217f+3.8f) 
		));
	if (h[11]) h[11]->SetVector( Vector4( 0.5f+0.5f*sinf(time*0.0050f+2.7f),
																0.5f+0.5f*sinf(time*0.0085f+5.3f), 
																0.5f+0.5f*sinf(time*0.0133f+4.5f), 
																0.5f+0.5f*sinf(time*0.0217f+3.8f) 
		));
	if (h[12]) h[12]->SetVector( Vector4( mip_x, mip_y, mip_avg, 0 ));
	if (h[13]) h[13]->SetVector( Vector4( blur_min[1], blur_max[1], blur_min[2], blur_max[2] ));

	// write q vars
	int num_q_float4s = sizeof(p->q_const_handles)/sizeof(p->q_const_handles[0]);
	for (int i=0; i<num_q_float4s; i++)
	{
		if (p->q_const_handles[i]) 
			p->q_const_handles[i]->SetVector( Vector4( 
				(float)pState->var_pf_q[i*4+0],
				(float)pState->var_pf_q[i*4+1],
				(float)pState->var_pf_q[i*4+2],
				(float)pState->var_pf_q[i*4+3] ));
	}

	// write matrices
	for (int i=0; i<20; i++)
	{
		if (p->rot_mat[i]) 
		{
			Matrix44 mx,my,mz,mxlate,temp;

			mx = MatrixRotationX(pState->m_rot_base[i].x + pState->m_rot_speed[i].x*time);
			my = MatrixRotationY(pState->m_rot_base[i].y + pState->m_rot_speed[i].y*time);
			mz = MatrixRotationZ(pState->m_rot_base[i].z + pState->m_rot_speed[i].z*time);
			mxlate = MatrixTranslation(pState->m_xlate[i]);

			temp = MatrixMultiply(mx,   mxlate);
			temp = MatrixMultiply(temp, mz);
			temp = MatrixMultiply(temp, my);

			p->rot_mat[i]->SetMatrix(temp);
		}
	}
	// the last 4 are totally random, each frame
	for (int i=20; i<24; i++)
	{
		if (p->rot_mat[i]) 
		{
			Matrix44 mx,my,mz,mxlate,temp;

			mx = MatrixRotationX(FRAND * 6.28f);
			my = MatrixRotationY(FRAND * 6.28f);
			mz = MatrixRotationZ(FRAND * 6.28f);
			mxlate = MatrixTranslation(FRAND, FRAND, FRAND);

			temp = MatrixMultiply(mx, mxlate);
			temp = MatrixMultiply(temp, mz);
			temp = MatrixMultiply(temp, my);

			p->rot_mat[i]->SetMatrix(temp);
		}
	}
}

void CPlugin::ShowToUser_NoShaders()//int bRedraw, int nPassOverride)
{
	// note: this one has to draw the whole screen!  (one big quad)

	SetFixedShader(m_lpVS[1]);

	float fZoom = 1.0f;
	Vertex v3[4];
	memset(v3, 0, sizeof(v3));

	// extend the poly we draw by 1 pixel around the viewable image area, 
	//  in case the video card wraps u/v coords with a +0.5-texel offset 
	//  (otherwise, a 1-pixel-wide line of the image would wrap at the top and left edges).
	v3[0].x = -1.0f;
	v3[1].x =  1.0f;
	v3[2].x = -1.0f;
	v3[3].x =  1.0f;
	v3[0].y =  1.0f;
	v3[1].y =  1.0f;
	v3[2].y = -1.0f;
	v3[3].y = -1.0f;

	//float aspect = GetWidth() / (float)(GetHeight()/(ASPECT)/**4.0f/3.0f*/);
	float aspect = GetWidth() / (float)(GetHeight()*m_fInvAspectY/**4.0f/3.0f*/);
	float x_aspect_mult = 1.0f;
	float y_aspect_mult = 1.0f;

	if (aspect>1)
		y_aspect_mult = aspect;
	else
		x_aspect_mult = 1.0f/aspect;

	for (int n=0; n<4; n++) 
	{
		v3[n].x *= x_aspect_mult;
		v3[n].y *= y_aspect_mult;
	}
	
	{
		float shade[4][3] = { 
			{ 1.0f, 1.0f, 1.0f },
			{ 1.0f, 1.0f, 1.0f },
			{ 1.0f, 1.0f, 1.0f },
			{ 1.0f, 1.0f, 1.0f } };  // for each vertex, then each comp.

		float fShaderAmount = m_pState->m_fShader.eval(GetTime());

		if (fShaderAmount > 0.001f)
		{
			for (int i=0; i<4; i++)
			{
				shade[i][0] = 0.6f + 0.3f*sinf(GetTime()*30.0f*0.0143f + 3 + i*21 + m_fRandStart[3]);
				shade[i][1] = 0.6f + 0.3f*sinf(GetTime()*30.0f*0.0107f + 1 + i*13 + m_fRandStart[1]);
				shade[i][2] = 0.6f + 0.3f*sinf(GetTime()*30.0f*0.0129f + 6 + i*9  + m_fRandStart[2]);
				float max = ((shade[i][0] > shade[i][1]) ? shade[i][0] : shade[i][1]);
				if (shade[i][2] > max) max = shade[i][2];
				for (int k=0; k<3; k++)
				{
					shade[i][k] /= max;
					shade[i][k] = 0.5f + 0.5f*shade[i][k];
				}
				for (int k=0; k<3; k++)
				{
					shade[i][k] = shade[i][k]*(fShaderAmount) + 1.0f*(1.0f - fShaderAmount);
				}
				v3[i].Diffuse = COLOR_RGBA_01(shade[i][0],shade[i][1],shade[i][2],1);
			}
		}

		float fVideoEchoZoom        = (float)(m_pState->var_pf_echo_zoom);//m_pState->m_fVideoEchoZoom.eval(GetTime());
		float fVideoEchoAlpha       = (float)(m_pState->var_pf_echo_alpha);//m_pState->m_fVideoEchoAlpha.eval(GetTime());
		int   nVideoEchoOrientation = (int)  (m_pState->var_pf_echo_orient) % 4;//m_pState->m_nVideoEchoOrientation;
		float fGammaAdj             = (float)(m_pState->var_pf_gamma);//m_pState->m_fGammaAdj.eval(GetTime());

		if (m_bBlending &&
			m_pState->m_fVideoEchoAlpha.eval(GetTime()) > 0.01f &&
			m_pState->m_fVideoEchoAlphaOld > 0.01f &&
			m_pState->m_nVideoEchoOrientation != m_pState->m_nVideoEchoOrientationOld)
		{
			if (m_fBlendProgress < m_fSnapPoint)
			{
				nVideoEchoOrientation = m_pState->m_nVideoEchoOrientationOld;
				fVideoEchoAlpha *= 1.0f - 2.0f*CosineInterp(m_fBlendProgress);
			}
			else
			{
				fVideoEchoAlpha *= 2.0f*CosineInterp(m_fBlendProgress) - 1.0f;
			}
		}

		if (fVideoEchoAlpha > 0.001f)
		{
			// video echo
			m_context->SetBlendDisable();

			for (int i=0; i<2; i++)
			{
				fZoom = (i==0) ? 1.0f : fVideoEchoZoom;

				float temp_lo = 0.5f - 0.5f/fZoom;
				float temp_hi = 0.5f + 0.5f/fZoom;
				v3[0].tu = temp_lo;
				v3[0].tv = temp_hi;
				v3[1].tu = temp_hi;
				v3[1].tv = temp_hi;
				v3[2].tu = temp_lo;
				v3[2].tv = temp_lo;
				v3[3].tu = temp_hi;
				v3[3].tv = temp_lo;

				// flipping
				if (i==1)
				{
					for (int j=0; j<4; j++)
					{
						if (nVideoEchoOrientation % 2)
							v3[j].tu = 1.0f - v3[j].tu;
						if (nVideoEchoOrientation >= 2)
							v3[j].tv = 1.0f - v3[j].tv;
					}
				}

				float mix = (i==1) ? fVideoEchoAlpha : 1.0f - fVideoEchoAlpha;
				for (int k=0; k<4; k++)	
					v3[k].Diffuse = COLOR_RGBA_01(mix*shade[k][0],mix*shade[k][1],mix*shade[k][2],1);

				m_context->DrawArrays(PRIMTYPE_TRIANGLESTRIP, 4, v3);

				if (i==0)
				{
					m_context->SetBlend(BLEND_ONE, BLEND_ONE);
				}

				if (fGammaAdj > 0.001f)
				{
					// draw layer 'i' a 2nd (or 3rd, or 4th...) time, additively
					int nRedraws = (int)(fGammaAdj - 0.0001f);
					float gamma;

					for (int nRedraw=0; nRedraw < nRedraws; nRedraw++)
					{
						if (nRedraw == nRedraws-1)
							gamma = fGammaAdj - (int)(fGammaAdj - 0.0001f);
						else
							gamma = 1.0f;

						for (int k=0; k<4; k++)
							v3[k].Diffuse = COLOR_RGBA_01(gamma*mix*shade[k][0],gamma*mix*shade[k][1],gamma*mix*shade[k][2],1);
						m_context->DrawArrays(PRIMTYPE_TRIANGLESTRIP, 4, v3);
					}
				}
			}
		}
		else
		{
			// no video echo
			v3[0].tu = 0;	v3[1].tu = 1;	v3[2].tu = 0;	v3[3].tu = 1;
			v3[0].tv = 1;	v3[1].tv = 1;	v3[2].tv = 0;	v3[3].tv = 0;

			m_context->SetBlendDisable();

			// draw it iteratively, solid the first time, and additively after that
			int nPasses = (int)(fGammaAdj - 0.001f) + 1;
			float gamma;

			for (int nPass=0; nPass < nPasses; nPass++)
			{
				if (nPass == nPasses - 1)
					gamma = fGammaAdj - (float)nPass;
				else
					gamma = 1.0f;

				for (int k=0; k<4; k++)
					v3[k].Diffuse = COLOR_RGBA_01(gamma*shade[k][0],gamma*shade[k][1],gamma*shade[k][2],1);
				m_context->DrawArrays(PRIMTYPE_TRIANGLESTRIP, 4, v3);

				if (nPass==0)
				{
					m_context->SetBlend(BLEND_ONE, BLEND_ONE);
				}
			}
		}

		Vertex v3[4];
		memset(v3, 0, sizeof(v3));
		v3[0].x = -1.0f;
		v3[1].x =  1.0f;
		v3[2].x = -1.0f;
		v3[3].x =  1.0f;
		v3[0].y =  1.0f;
		v3[1].y =  1.0f;
		v3[2].y = -1.0f;
		v3[3].y = -1.0f;
		for (int i=0; i<4; i++) v3[i].Diffuse = COLOR_RGBA_01(1,1,1,1);

		if (m_pState->var_pf_brighten)
		{
			// square root filter
            SetFixedShader(nullptr);

			// first, a perfect invert
			m_context->SetBlend(BLEND_INVDESTCOLOR, BLEND_ZERO);
			m_context->DrawArrays(PRIMTYPE_TRIANGLESTRIP, 4, v3);

			// then modulate by self (square it)
			m_context->SetBlend(BLEND_ZERO, BLEND_DESTCOLOR);
			m_context->DrawArrays(PRIMTYPE_TRIANGLESTRIP, 4, v3);

			// then another perfect invert
			m_context->SetBlend(BLEND_INVDESTCOLOR, BLEND_ZERO);
			m_context->DrawArrays(PRIMTYPE_TRIANGLESTRIP, 4, v3);
		}

		if (m_pState->var_pf_darken)
		{
			// squaring filter
            SetFixedShader(nullptr);
			m_context->SetBlend(BLEND_ZERO, BLEND_DESTCOLOR);
			m_context->DrawArrays(PRIMTYPE_TRIANGLESTRIP, 4, v3);

		}
		
		if (m_pState->var_pf_solarize)
		{
            SetFixedShader(nullptr);
			m_context->SetBlend(BLEND_ZERO, BLEND_INVDESTCOLOR);
			m_context->DrawArrays(PRIMTYPE_TRIANGLESTRIP, 4, v3);

			m_context->SetBlend(BLEND_DESTCOLOR, BLEND_ONE);
			m_context->DrawArrays(PRIMTYPE_TRIANGLESTRIP, 4, v3);
		}

		if (m_pState->var_pf_invert)
		{
            SetFixedShader(nullptr);
			m_context->SetBlend(BLEND_INVDESTCOLOR, BLEND_ZERO);
			
			m_context->DrawArrays(PRIMTYPE_TRIANGLESTRIP, 4, v3);
		}

		m_context->SetBlendDisable();
	}
}

void CPlugin::ShowToUser_Shaders(int nPass, bool bAlphaBlend, bool bFlipAlpha)//int bRedraw, int nPassOverride, bool bFlipAlpha)
{
	//SetFixedShader(nullptr);
	//m_context->SetBlendDisable();

//	float fZoom = 1.0f;


	float aspect = GetWidth() / (float)(GetHeight()*m_fInvAspectY/**4.0f/3.0f*/);
	float x_aspect_mult = 1.0f;
	float y_aspect_mult = 1.0f;

	if (aspect>1)
		y_aspect_mult = aspect;
	else
		x_aspect_mult = 1.0f/aspect;

	// hue shader
	float shade[4][3] = { 
		{ 1.0f, 1.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f } };  // for each vertex, then each comp.

	float fShaderAmount = 1;//since we don't know if shader uses it or not!  m_pState->m_fShader.eval(GetTime());

	if (fShaderAmount > 0.001f || m_bBlending)
	{
		// pick 4 colors for the 4 corners
		for (int i=0; i<4; i++)
		{
			shade[i][0] = 0.6f + 0.3f*sinf(GetTime()*30.0f*0.0143f + 3 + i*21 + m_fRandStart[3]);
			shade[i][1] = 0.6f + 0.3f*sinf(GetTime()*30.0f*0.0107f + 1 + i*13 + m_fRandStart[1]);
			shade[i][2] = 0.6f + 0.3f*sinf(GetTime()*30.0f*0.0129f + 6 + i*9  + m_fRandStart[2]);
			float max = ((shade[i][0] > shade[i][1]) ? shade[i][0] : shade[i][1]);
			if (shade[i][2] > max) max = shade[i][2];
			for (int k=0; k<3; k++)
			{
				shade[i][k] /= max;
				shade[i][k] = 0.5f + 0.5f*shade[i][k];
			}
			// note: we now pass the raw hue shader colors down; the shader can only use a certain % if it wants.
			//for (k=0; k<3; k++)
			//	shade[i][k] = shade[i][k]*(fShaderAmount) + 1.0f*(1.0f - fShaderAmount);
			//m_comp_verts[i].Diffuse = COLOR_RGBA_01(shade[i][0],shade[i][1],shade[i][2],1);
		}

		// interpolate the 4 colors & apply to all the verts
		for (int j=0; j<FCGSY; j++) 
		{
			for (int i=0; i<FCGSX; i++) 
			{
				Vertex* p = &m_comp_verts[i + j*FCGSX];
				float x = p->x*0.5f + 0.5f;
				float y = p->y*0.5f + 0.5f; 

				float col[3] = { 1, 1, 1 };
				if (fShaderAmount > 0.001f) 
				{
					for (int c=0; c<3; c++) 
						col[c] = shade[0][c]*(  x)*(  y) + 
								 shade[1][c]*(1-x)*(  y) + 
								 shade[2][c]*(  x)*(1-y) + 
								 shade[3][c]*(1-x)*(1-y);
				}

				// TO DO: improve interp here?
				// TO DO: during blend, only send the triangles needed

				// if blending, also set up the alpha values - pull them from the alphas used for the Warped Blit
				double alpha = 1;
				if (m_bBlending) 
				{
					x *= (m_nGridX + 1);
					y *= (m_nGridY + 1);
					x = std::max(std::min(x,(float)m_nGridX-1),0.0f);
					y = std::max(std::min(y,(float)m_nGridY-1),0.0f);
					int nx = (int)x;
					int ny = (int)y;
					double dx = x - nx;
					double dy = y - ny;
                    double alpha00 = (m_verts[(ny  )*(m_nGridX+1) + (nx  )].Diffuse >> 24);
                    double alpha01 = (m_verts[(ny  )*(m_nGridX+1) + (nx+1)].Diffuse >> 24);
                    double alpha10 = (m_verts[(ny+1)*(m_nGridX+1) + (nx  )].Diffuse >> 24);
                    double alpha11 = (m_verts[(ny+1)*(m_nGridX+1) + (nx+1)].Diffuse >> 24);
					alpha = alpha00*(1-dx)*(1-dy) +
							alpha01*(  dx)*(1-dy) + 
							alpha10*(1-dx)*(  dy) + 
							alpha11*(  dx)*(  dy);
					alpha /= 255.0f;
					//if (bFlipAlpha)
					//    alpha = 1-alpha;

					//alpha = (m_verts[y*(m_nGridX+1) + x].Diffuse >> 24) / 255.0f;
				}
				p->Diffuse = COLOR_RGBA_01(col[0],col[1],col[2],alpha);
			}
		}
	}

	if (bAlphaBlend)
	{
		if (bFlipAlpha) 
		{
			m_context->SetBlend(BLEND_INVSRCALPHA, BLEND_SRCALPHA);
		}
		else
		{
			m_context->SetBlend(BLEND_SRCALPHA, BLEND_INVSRCALPHA);
		}
	}
	else 
		m_context->SetBlendDisable();

	// Now do the final composite blit, fullscreen; 
	//  or do it twice, alpha-blending, if we're blending between two sets of shaders.

	int pass = nPass;
	{
		// PASS 0: draw using *blended per-vertex motion vectors*, but with the OLD comp shader.
		// PASS 1: draw using *blended per-vertex motion vectors*, but with the NEW comp shader.
        CStatePtr state = (pass==0) ? m_pOldState : m_pState;

		ApplyShaderParams(state->m_shader_comp, state );

		// Hurl the triangles at the video card.
		// We're going to un-index it, so that we don't stress any crappy (AHEM intel g33)
		//  drivers out there.  Not a big deal - only ~800 polys / 24kb of data.
		// If we're blending, we'll skip any polygon that is all alpha-blended out.
		// This also respects the MaxPrimCount limit of the video card.
        
        m_context->UploadIndexData(m_comp_indices);
        m_context->UploadVertexData(m_comp_verts);
        m_context->DrawIndexed(PRIMTYPE_TRIANGLELIST, 0, (int)m_comp_indices.size() );
	}

	m_context->SetBlendDisable();
    SetFixedShader(nullptr);
}
