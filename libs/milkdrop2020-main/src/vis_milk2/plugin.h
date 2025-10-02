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

#ifndef __NULLSOFT_DX9_EXAMPLE_PLUGIN_H__
#define __NULLSOFT_DX9_EXAMPLE_PLUGIN_H__ 1

#include <vector>
#include "render/MDcontext.h"
#include "render/DrawBuffer.h"
#include "IVisualizer.h"
#include "IAudioAnalyzer.h"
#include "state.h"
#include <thread>
#include <future>


struct td_vertinfo
{
    float vx;
    float vy;
    float rad;
    float ang;
    float a;
    float c;
    
}; // blending: mix = max(0,min(1,a*t + c));

enum
{
	MD2_PS_NONE = 0,
	MD2_PS_2_0 = 2,
	MD2_PS_2_X = 3,
	MD2_PS_3_0 = 4,
	MD2_PS_4_0 = 5, // not supported by milkdrop
};


using StringVec = std::vector<std::string>;

class CPlugin : public IVisualizer
{
public:
        CPlugin(render::ContextPtr context, ITextureSetPtr texture_map, std::string assetDir);
		virtual ~CPlugin();

		// called by vis.cpp, on behalf of Winamp:
		virtual bool PluginInitialize();
        virtual void Render(float dt, Size2D outputSize, IAudioSourcePtr audioSource) override;
    
        virtual void ShowPresetEditor()
        {
            m_showPresetEditor = !m_showPresetEditor;
        }
    
    
    virtual void ShowPresetDebugger()
    {
        m_pluginDebugUI = !m_pluginDebugUI;
    }

  
    render::IDrawBufferPtr GetDrawBuffer()
    {
        return m_drawbuffer;
    }


			
	//====[ 1. members added to create this specific example plugin: ]================================================

		int          m_frame;           // current frame #, starting at zero
		float        m_time;            // current animation time in seconds; starts at zero.
		float        m_fps;             // current estimate of frames per second

    bool m_showPresetEditor = false;
    bool m_pluginDebugUI = false;
    
    render::ContextPtr   m_context;            // pointer to DXContext object
    render::IDrawBufferPtr m_drawbuffer;

		/// CONFIG PANEL SETTINGS THAT WE'VE ADDED (TAB #2)
		int			m_nTexSizeX;			// -1 = exact match to screen; -2 = nearest power of 2.
		int			m_nTexSizeY;
        render::PixelFormat m_FormatRGBA8 = render::PixelFormat::RGBA8Unorm;
        render::PixelFormat m_OutputFormat = render::PixelFormat::RGBA8Unorm;
        render::PixelFormat m_InternalFormat = render::PixelFormat::RGBA8Unorm;
		float       m_fAspectX;
		float       m_fAspectY;
		float       m_fInvAspectX;
		float       m_fInvAspectY;
		int			m_nGridX;
		int			m_nGridY;

		// PIXEL SHADERS
//        ShaderInfoPtr             m_BlurShaders[2];
    

		#define SHADER_WARP  0
		#define SHADER_COMP  1
		#define SHADER_BLUR  2
		#define SHADER_OTHER 3
		std::string ComposeShader(const std::string &szOrigShaderText, const char* szFn, const char* szProfile, int shaderType);
        ShaderInfoPtr       RecompileShader(const std::string &name, const std::string &vs_text, const std::string &ps_text, int shaderType, std::string &errors);

        void SetFixedShader(render::TexturePtr texture);
		void SetFixedShader(render::TexturePtr texture, render::SamplerAddress addr, render::SamplerFilter filter);

		render::ShaderPtr			    m_shader_fixed;		// rgb = vertex.rgb * texture.rgb, a = vertex.a
        ITextureSetPtr  m_texture_map;

        render::ShaderPtr                m_shader_custom_shapes;
        render::ShaderPtr                m_shader_custom_waves;
        render::ShaderPtr                m_shader_warp_fixed;
        render::ShaderPtr                m_shader_blur1;
        render::ShaderPtr                m_shader_blur2;
        render::ShaderPtr                m_shader_output;

    
		Vector4		m_rand_frame;  // 4 random floats (0..1); randomized once per frame; fed to pixel shaders.

		// RUNTIME SETTINGS THAT WE'VE ADDED
		float		m_fAnimTime;
        float		m_PresetDuration;       // time that this preset has been shown
		float		m_NextPresetDuration;   // time until next preset should be shown
		float       m_fSnapPoint;
private:
		CStatePtr	m_pState;				// points to current CState
		CStatePtr   m_pOldState;			// points to previous CState
        CommonVars  m_commonVars;   // common variables
public:
    
        const CommonVars &GetCommonVars() const  {return m_commonVars;}
    
        // blending parameters (moved here from CState)
        bool  m_clearTargets = false;
        bool  m_bBlending = false;
        float m_fBlendStartTime = 0;
        float m_fBlendDuration = 0;
        float m_fBlendProgress = 0;    // 0..1; updated every frame based on StartTime and Duration.

        IAudioAnalyzerPtr m_audio;
    
		std::string	m_assetDir;
		float		m_fRandStart[4];

        render::TexturePtr m_outputTexture;
		render::TexturePtr m_lpVS[2];
    
        std::vector<render::TexturePtr> m_blur_textures;
        std::vector<render::TexturePtr> m_blurtemp_textures;
        std::vector<render::Vertex>          m_verts;
        std::vector<td_vertinfo>       m_vertinfo;
		std::vector<render::IndexType>          m_indices_list;
        std::vector<render::IndexType>          m_indices_list_wrapped;

    
		// for final composite grid:
		#define FCGSX 32 // final composite gridsize - # verts - should be EVEN.  
//		#define FCGSY 24 // final composite gridsize - # verts - should be EVEN.
        #define FCGSY 32 // final composite gridsize - # verts - should be EVEN.
					 // # of grid *cells* is two less,
						 // since we have redundant verts along the center line in X and Y (...for clean 'ang' interp)
        std::vector<render::Vertex>    m_comp_verts; //[FCGSX*FCGSY];
        std::vector<render::IndexType>         m_comp_indices; //[(FCGSX-2)*(FCGSY-2)*2*3];

		StringVec texfiles;
    
        std::mt19937     m_random_generator;

		std::string        m_szShaderIncludeText;     // note: this still has char 13's and 10's in it - it's never edited on screen or loaded/saved with a preset.
		std::string        m_szDefaultWarpVShaderText; // THIS HAS CHAR 13/10 CONVERTED TO LINEFEED_CONTROL_CHAR
		std::string        m_szDefaultWarpPShaderText; // THIS HAS CHAR 13/10 CONVERTED TO LINEFEED_CONTROL_CHAR
		std::string        m_szDefaultCompVShaderText; // THIS HAS CHAR 13/10 CONVERTED TO LINEFEED_CONTROL_CHAR
		std::string        m_szDefaultCompPShaderText; // THIS HAS CHAR 13/10 CONVERTED TO LINEFEED_CONTROL_CHAR
		//const char* GetDefaultWarpShadersText() { return m_szDefaultWarpShaderText; }
		//const char* GetDefaultCompShadersText() { return m_szDefaultCompShaderText; }
		void        GenWarpPShaderText(std::string &szShaderText, float decay, bool bWrap);
		void        GenCompPShaderText(std::string &szShaderText, float brightness, float ve_alpha, float ve_zoom, int ve_orient, float hue_shader, bool bBrighten, bool bDarken, bool bSolarize, bool bInvert);

    
        SampleBuffer<Sample>         m_samples;

		// GET METHODS
		// ------------------------------------------------------------
		int       GetFrame()
		{
			return m_frame;
		};
		float     GetTime()
		{
			return m_time;
		};
		float     GetFps()
		{
			return m_fps;
		};

		int          GetWidth()
		{
			return m_nTexSizeX;
		}
		int          GetHeight()
		{
			return m_nTexSizeY;
		}

		render::ContextPtr  GetContext()
		{
			return m_context;
		}

		int warand(int modulo);
        float frand();
    
        virtual float GetPresetProgress() override;
    
        virtual const std::string &GetPresetName() override
        {
            return m_pState->GetName();
        }
    
    
    
        float GetImmRel(AudioBand band);
        float GetAvgRel(AudioBand band);
        float GetImmRelTotal();
        float GetAvgRelTotal();

        void RenderWaveForm(
                            int sampleCount, int samplePad,
                            SampleBuffer<float> &fL,
                            SampleBuffer<float> &fR
                         );

    void RenderWaveForm(
                        int sampleCount,
                        SampleBuffer<Sample> &fWaveForm
                        );

    void RenderWaveForm(
                        int sampleCount, int samplePad,
                        SampleBuffer<float> &fWaveForm
                        );

    
        void RenderSpectrum(
                            int sampleCount,
                            SampleBuffer<float> &fSpectrum
                            );


   //====[ 2. methods added: ]=====================================================================================

		void RenderFrame();
		void        RandomizeBlendPattern();
		void        GenPlasma(int x0, int x1, int y0, int y1, float dt);
		void		Randomize();
    

     
        virtual PresetPtr   LoadPreset(const std::string &text, std::string name, std::string &errors)  override;
        virtual void		SetPreset(PresetPtr preset, PresetLoadArgs args) override;
        virtual void        LoadEmptyPreset();
     
		bool		ReversePropagatePoint(float fx, float fy, float *fx2, float *fy2);
		virtual int 		HandleRegularKey(char key);

    
        void DrawImageWithShader(render::ShaderPtr shader, render::TexturePtr source, render::TexturePtr dest);
    
    

        virtual void Draw(ContentMode mode, float alpha = 1.0f) override;
        virtual void SetOutputSize(Size2D size);
        virtual render::TexturePtr GetInternalTexture() override;
        virtual render::TexturePtr GetOutputTexture() override;
        virtual void SetRandomSeed(uint32_t seed) override;
    
        virtual void CaptureScreenshot(render::TexturePtr texture, Vector2 pos, Size2D size) override;

    
        virtual render::TexturePtr LookupTexture(const std::string &name);
    
        void        DrawQuad(render::TexturePtr texture, float x, float y, float w, float h, Color4F color);

        virtual void        DrawDebugUI()  override;
        virtual void DrawAudioUI() override;
        virtual void DumpState();
    
		void		DrawWave();
        void        DrawCustomWaves_ComputeBegin();
        void        DrawCustomWaves_ComputeEnd();
		void        DrawCustomShapes_ComputeBegin();
        void        DrawCustomShapes_ComputeEnd();
		void		DrawSprites();
        void        ComputeGridAlphaValues_ComputeBegin();
        void        ComputeGridAlphaValues_ComputeEnd();
        void        BlendGrid();
        void        WarpedBlit();
        void        CompositeBlit();

    
    // note: 'bFlipAlpha' just flips the alpha blending in fixed-fn pipeline - not the values for culling tiles.
		void		 WarpedBlit_Shaders  (int nPass, bool bAlphaBlend, bool bFlipAlpha);
		void		 WarpedBlit_NoShaders(int nPass, bool bAlphaBlend, bool bFlipAlpha);
		void		 ShowToUser_Shaders  (int nPass, bool bAlphaBlend, bool bFlipAlpha);
		void		 ShowToUser_NoShaders();
		void        BlurPasses(int source);
		void		RunPerFrameEquations();
		void		DrawUserSprites();
		void        DoCustomSoundAnalysis();
        void        ClearTargets();

		void        UvToMathSpace(float u, float v, float* rad, float* ang);
		void        ApplyShaderParams(ShaderInfoPtr p, CStatePtr pState);
		void        RestoreShaderParams();
		void        AddNoiseTex(const char* szTexName, int size, int zoom_factor);
    void        AddNoiseVol(const char* szTexName, int size, int zoom_factor);

        render::ShaderPtr LoadShaderFromFile(const char* szBaseFilename);

		std::string LoadShaderCode(const char* szBaseFilename);
        bool PickRandomTexture(const std::string &prefix, std::string &szRetTextureFilename); //should be MAX_PATH chars
    
        render::TexturePtr LoadDiskTexture(const std::string &name);
        void TestLineDraw();

    
    

	//====[ 3. virtual functions: ]===========================================================================

    void AllocateOutputTexture();
    void AllocateVertexData();
    
		virtual bool AllocateResources();
		virtual void ReleaseResources();
};

#endif
