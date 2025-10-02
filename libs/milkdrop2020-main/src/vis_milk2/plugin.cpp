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
/*
  ##########################################################################################

  NOTE: 
  
  The DX9 SDK include & lib files for building MilkDrop 2 are right here, in the subdirectory:
	.\dx9sdk_summer04\

  The summer 2004 SDK statically links you to d3dx9.lib (5,820 kb).  It bloats vis_milk2.dll 
  a bit, but we (Ben Allison / Ryan Geiss) decided it's worth the stability.  We tinkered 
  with using the February 2005 sdk, which lets you dynamically link to d3dx9_31.dll (?), 
  but decided to skip it because it would cause too much hassle/confusion among users.

  Summer 2004 sdk: http://www.microsoft.com/downloads/details.aspx?FamilyID=fd044a42-9912-42a3-9a9e-d857199f888e&DisplayLang=en
  Feb 2005 sdk:    http://www.microsoft.com/downloads/details.aspx?FamilyId=77960733-06E9-47BA-914A-844575031B81&displaylang=en

  ##########################################################################################
*/

/*
Order of Function Calls
-----------------------
	The only code that will be called by the plugin framework are the
	12 virtual functions in plugin.h.  But in what order are they called?  
	A breakdown follows.  A function name in { } means that it is only 
	called under certain conditions.

	Order of function calls...
	
	When the PLUGIN launches
	------------------------
		INITIALIZATION
			OverrideDefaults
			MyPreInitialize
			MyReadConfig
			<< DirectX gets initialized at this point >>
			AllocateMyNonDx9Stuff
			AllocateMyDX9Stuff
		RUNNING
			+--> { CleanUpMyDX9Stuff + AllocateMyDX9Stuff }  // called together when user resizes window or toggles fullscreen<->windowed.
			|    MyRenderFn
			|    MyRenderUI
			|    { MyWindowProc }                            // called, between frames, on mouse/keyboard/system events.  100% threadsafe.
			+----<< repeat >>
		CLEANUP        
			CleanUpMyDX9Stuff
			CleanUpMyNonDx9Stuff
			<< DirectX gets uninitialized at this point >>

	When the CONFIG PANEL launches
	------------------------------
		INITIALIZATION
			OverrideDefaults
			MyPreInitialize
			MyReadConfig
			<< DirectX gets initialized at this point >>
		RUNNING
			{ MyConfigTabProc }                  // called on startup & on keyboard events
		CLEANUP
			[ MyWriteConfig ]                    // only called if user clicked 'OK' to exit
			<< DirectX gets uninitialized at this point >>
*/

/*
  NOTES
  -----
  


  To do
  -----
	-VMS VERSION:
		-based on vms 1.05, but the 'fix slow text' option has been added.
			that includes m_lpDDSText, CTextManager (m_text), changes to 
			DrawDarkTranslucentBox, the replacement of all DrawText calls
			(now routed through m_text), and adding the 'fix slow text' cb
			to the config panel.

	-KILLED FEATURES:
		-vj mode
	
	-NEW FEATURES FOR 1.04:
			-added the following variables for per-frame scripting: (all booleans, except 'gamma')
				wave_usedots, wave_thick, wave_additive, wave_brighten
				gamma, darken_center, wrap, invert, brighten, darken, solarize
				(also, note that echo_zoom, echo_alpha, and echo_orient were already in there,
				 but weren't covered in the documentation!)
		d   -fixed: spectrum w/512 samples + 256 separation -> infinite spike
		d   -reverted dumb changes to aspect ratio stuff
		d   -reverted wave_y fix; now it's backwards, just like it's always been
				(i.e. for wave's y position, 0=bottom and 1=top, which is opposite
				to the convention in the rest of milkdrop.  decided to keep the
				'bug' so presets don't need modified.)
		d   -fixed: Krash: Inconsistency bug - pressing Escape while in the code windows 
				for custom waves completely takes you out of the editing menus, 
				rather than back to the custom wave menu 
		d   -when editing code: fix display of '&' character 
		d   -internal texture size now has a little more bias toward a finer texture, 
				based on the window size, when set to 'Auto'.  (Before, for example,
				to reach 1024x1024, the window had to be 768x768 or greater; now, it
				only has to be 640x640 (25% of the way there).  I adjusted it because
				before, at in-between resolutions like 767x767, it looked very grainy;
				now it will always look nice and crisp, at any window size, but still
				won't cause too much aliasing (due to downsampling for display).
		d   -fixed: rova:
				When creating presets have commented code // in the per_pixel section when cause error in preset.
				Example nothing in per_frame and just comments in the per_pixel. EXamples on repuest I have a few.
		d   -added kill keys:
				-CTRL+K kills all running sprites
				-CTRL+T kills current song title anim
				-CTRL+Y kills current custom message
		d   -notice to sprite users:
				-in milk_img.ini, color key can't be a range anymore; it's
					now limited to just a single color.  'colorkey_lo' and 
					'colorkey_hi' have been replaced with just one setting, 
					'colorkey'.
		d   -song titles + custom messages are working again
		?   -fixed?: crashes on window resize [out of mem]
				-Rova: BTW the same bug as krash with the window resizing.
				-NOT due to the 'integrate w/winamp' option.
				-> might be fixed now (had forgotten to release m_lpDDSText)
		<AFTER BETA 3..>
		d   -added checkbox to config screen to automatically turn SCROLL LOCK on @ startup
		d   -added alphanumeric seeking to the playlist; while playlist is up,
				you can now press A-Z and 0-9 to seek to the next song in the playlist
				that starts with that character.
		d   -<fixed major bug w/illegal mem access on song title launches;
				could have been causing crashing madness @ startup on many systems>
		d   -<fixed bug w/saving 64x48 mesh size>
		d   -<fixed squashed shapes>
		d   -<fixed 'invert' variable>
		d   -<fixed squashed song titles + custom msgs>
		?   -<might have fixed scroll lock stuff>  
		?   -<might have fixed crashing; could have been due to null ptr for failed creation of song title texture.>
		?   -<also might have solved any remaining resize or exit bugs by callign SetTexture(NULL)
				in DX8 cleanup.>
		d   -<fixed sizing issues with songtitle font.>
		d   -<fixed a potentially bogus call to deallocate memory on exit, when it was cleaning up the menus.>
		d   -<fixed more scroll lock issues>
		d   -<fixed broken Noughts & Crosses presets; max # of per-frame vars was one too few, after the additions of the new built-in variables.>
		d   -<hopefully fixed waveforms>
		<AFTER BETA 4>
			-now when playlist is up, SHIFT+A-Z seeks upward (while lowercase/regular a-z seeks downward).
			-custom shapes:
				-OH MY GOD
				-increased max. # of custom shapes (and waves) from 3 to 4
				-added 'texture' option, which allows you to use the last frame as a texture on the shape
					-added "tex_ang" and "tex_zoom" params to control the texture coords
				-each frame, custom shapes now draw BEFORE regular waveform + custom waves
				-added init + per-frame vars: "texture", "additive", "thick", "tex_ang", "tex_zoom"
			-fixed valid characters for filenames when importing/exporting custom shapes/waves;
				also, it now gives error messages on error in import/export.
			-cranked max. meshsize up to 96x72
			-Krash, Rova: now the 'q' variables, as modified by the preset per-frame equations, are again 
				readable by the custom waves + custom shapes.  Sorry about that.  Should be the end of the 
				'q' confusion.
			-added 'meshx' and 'meshy' [read-only] variables to the preset init, per-frame, and per-pixel 
				equations (...and inc'd the size of the global variable pool by 2).
			-removed t1-t8 vars for Custom Shapes; they were unnecessary (since there's no per-point code there).
			-protected custom waves from trying to draw when # of sample minus the separation is < 2
				(or 1 if drawing with dots)
			-fixed some [minor] preset-blending bugs in the custom wave code 
			-created a visual map for the flow of values for the q1-q8 and t1-t8 variables:
				q_and_t_vars.gif (or something).
			-fixed clipping of onscreen text in low-video-memory situations.  Now, if there isn't enough
				video memory to create an offscreen texture that is at least 75% of the size of the 
				screen (or to create at least a 256x256 one), it won't bother using one, and will instead draw text directly to the screen.
				Otherwise, if the texture is from 75%-99% of the screen size, text will now at least
				appear in the correct position on the screen so that it will be visible; this will mean
				that the right- and bottom-aligned text will no longer be fully right/bottom-aligned 
				to the edge of the screen.                
			-fixed blurry text 
			-VJ mode is partially restored; the rest will come with beta 7 or the final release.  At the time of beta 6, VJ mode still has some glitches in it, but I'm working on them.  Most notably, it doesn't resize the text image when you resize the window; that's next on my list.
		<AFTER BETA 6:>            
			-now sprites can burn-in on any frame, not just on the last frame.
				set 'burn' to one (in the sprite code) on any frame to make it burn in.
				this will break some older sprites, but it's super easy to fix, and 
				I think it's worth it. =)  thanks to papaw00dy for the suggestion!
			-fixed the asymptotic-value bug with custom waves using spectral data & having < 512 samples (thanks to telek's example!)
			-fixed motion vectors' reversed Y positioning.
			-fixed truncation ("...") of long custom messages
			-fixed that pesky bug w/the last line of code on a page
			-fixed the x-positioning of custom waves & shapes.  Before, if you were 
				saving some coordinates from the preset's per-frame equations (say in q1 and q2)
				and then you set "x = q1; y = q2;" in a custom shape's per-frame code
				(or in a custom wave's per-point code), the x position wouldn't really be
				in the right place, because of aspect ratio multiplications.  Before, you had
				to actually write "x = (q1-0.5)*0.75 + 0.5; y = q2;" to get it to line up; 
				now it's fixed, though, and you can just write "x = q1; y = q2;".
			-fixed some bugs where the plugin start up, in windowed mode, on the wrong window
				(and hence run super slow).
			-fixed some bugs w/a munged window frame when the "integrate with winamp" option
				was checked.
		<AFTER BETA 7:>
			-preset ratings are no longer read in all at once; instead, they are scanned in
				1 per frame until they're all in.  This fixes the long pauses when you switch
				to a directory that has many hundreds of presets.  If you want to switch
				back to the old way (read them all in at once), there is an option for it
				in the config panel.
			-cranked max. mesh size up to 128x96
			-fixed bug in custom shape per-frame code, where t1-t8 vars were not 
				resetting, at the beginning of each frame, to the values that they had 
				@ the end of the custom shape init code's execution.
			-
			-
			-


		beta 2 thread: http://forums.winamp.com/showthread.php?threadid=142635
		beta 3 thread: http://forums.winamp.com/showthread.php?threadid=142760
		beta 4 thread: http://forums.winamp.com/showthread.php?threadid=143500
		beta 6 thread: http://forums.winamp.com/showthread.php?threadid=143974
		(+read about beat det: http://forums.winamp.com/showthread.php?threadid=102205)

@       -code editing: when cursor is on 1st posn. in line, wrong line is highlighted!?
		-requests:
			-random sprites (...they can just write a prog for this, tho)
			-Text-entry mode.
				-Like your favorite online game, hit T or something to enter 'text entry' mode. Type a message, then either hit ESC to clear and cancel text-entry mode, or ENTER to display the text on top of the vis. Easier for custom messages than editing the INI file (and probably stopping or minimizing milkdrop to do it) and reloading.
				-OR SKIP IT; EASY TO JUST EDIT, RELOAD, AND HIT 00.
			-add 'AA' parameter to custom message text file?
		-when mem is low, fonts get kicked out -> white boxes
			-probably happening b/c ID3DXFont can't create a 
			 temp surface to use to draw text, since all the
			 video memory is gobbled up.
*       -add to installer: q_and_t_vars.gif
*       -presets:
			1. pick final set
					a. OK-do a pass weeding out slow presets (crank mesh size up)
					b. OK-do 2nd pass; rate them & delete crappies
					c. OK-merge w/set from 1.03; check for dupes; delete more suckies
			2. OK-check for cpu-guzzlers
			3. OK-check for big ones (>= 8kb)
			4. check for ultra-spastic-when-audio-quiet ones
			5. update all ratings
			6. zip 'em up for safekeeping
*       -docs: 
				-link to milkdrop.co.uk
				-preset authoring:
					-there are 11 variable pools:
						preset:
							a) preset init & per-frame code
							b) preset per-pixel code
						custom wave 1:
							c) init & per-frame code
							d) per-point code
						custom wave 2:
							e) init & per-frame code
							f) per-point code
						custom wave 3:
							g) init & per-frame code
							h) per-point code
						i) custom shape 1: init & per-frame code
						j) custom shape 2: init & per-frame code
						k) custom shape 3: init & per-frame code

					-all of these have predefined variables, the values of many of which 
						trickle down from init code, to per-frame code, to per-pixel code, 
						when the same variable is defined for each of these.
					-however, variables that you define ("my_var = 5;") do NOT trickle down.
						To allow you to pass custom values from, say, your per-frame code
						to your per-pixel code, the variables q1 through q8 were created.
						For custom waves and custom shapes, t1 through t8 work similarly.
					-q1-q8:
						-purpose: to allow [custom] values to carry from {the preset init
							and/or per-frame equations}, TO: {the per-pixel equations},
							{custom waves}, and {custom shapes}.
						-are first set in preset init code.
						-are reset, at the beginning of each frame, to the values that 
							they had at the end of the preset init code. 
						-can be modified in per-frame code...
							-changes WILL be passed on to the per-pixel code 
							-changes WILL pass on to the q1-q8 vars in the custom waves
								& custom shapes code
							-changes will NOT pass on to the next frame, though;
								use your own (custom) variables for that.
						-can be modified in per-pixel code...
							-changes will pass on to the next *pixel*, but no further
							-changes will NOT pass on to the q1-q8 vars in the custom
								waves or custom shapes code.
							-changes will NOT pass on to the next frame, after the
								last pixel, though.
						-CUSTOM SHAPES: q1-q8...
							-are readable in both the custom shape init & per-frame code
							-start with the same values as q1-q8 had at the end of the *preset*
								per-frame code, this frame
							-can be modified in the init code, but only for a one-time
								pass-on to the per-frame code.  For all subsequent frames
								(after the first), the per-frame code will get the q1-q8
								values as described above.
							-can be modified in the custom shape per-frame code, but only 
								as temporary variables; the changes will not pass on anywhere.
						-CUSTOM WAVES: q1-q8...
							-are readable in the custom wave init, per-frame, and per-point code
							-start with the same values as q1-q8 had at the end of the *preset*
								per-frame code, this frame
							-can be modified in the init code, but only for a one-time
								pass-on to the per-frame code.  For all subsequent frames
								(after the first), the per-frame code will get the q1-q8
								values as described above.
							-can be modified in the custom wave per-frame code; changes will
								pass on to the per-point code, but that's it.
							-can be modified in the per-point code, and the modified values
								will pass on from point to point, but won't carry beyond that.
						-CUSTOM WAVES: t1-t8...
							-allow you to generate & save values in the custom wave init code,
								that can pass on to the per-frame and, more sigificantly,
								per-point code (since it's in a different variable pool).
							-...                                
						


						!-whatever the values of q1-q8 were at the end of the per-frame and per-pixel
							code, these are copied to the q1-q8 variables in the custom wave & custom 
							shape code, for that frame.  However, those values are separate.
							For example, if you modify q1-q8 in the custom wave #1 code, those changes 
							will not be visible anywhere else; if you modify q1-q8 in the custom shape
							#2 code, same thing.  However, if you modify q1-q8 in the per-frame custom
							wave code, those modified values WILL be visible to the per-point custom
							wave code, and can be modified within it; but after the last point,
							the values q1-q8 will be discarded; on the next frame, in custom wave #1
							per-frame code, the values will be freshly copied from the values of the 
							main q1-q8 after the preset's per-frame and per-point code have both been
							executed.                          
					-monitor: 
						-can be read/written in preset init code & preset per-frame code.
						-not accessible from per-pixel code.
						-if you write it on one frame, then that value persists to the next frame.
					-t1-t8:
						-
						-
						-
				-regular docs:
					-put in the stuff recommended by VMS (vidcap, etc.)
					-add to troubleshooting:
						1) desktop mode icons not appearing?  or
						   onscreen text looking like colored boxes?
							 -> try freeing up some video memory.  lower your res; drop to 16 bit;
								etc.  TURN OFF AUTO SONGTITLES.
						1) slow desktop/fullscr mode?  -> try disabling auto songtitles + desktop icons.
							also try reducing texsize to 256x256, since that eats memory that the text surface could claim.
						2) 
						3) 
		*   -presets:
				-add new 
				-fix 3d presets (bring gammas back down to ~1.0)
				-check old ones, make sure they're ok
					-"Rovastar - Bytes"
					-check wave_y
		*   -document custom waves & shapes
		*   -slow text is mostly fixed... =(
				-desktop icons + playlist both have begin/end around them now, but in desktop mode,
				 if you bring up playlist or Load menu, fps drops in half; press Esc, and fps doesn't go back up.
			-
			-
			-
		-DONE / v1.04:
			-updated to VMS 1.05
				-[list benefits...]
				-
				-
			-3d mode: 
				a) SWAPPED DEFAULT L/R LENS COLORS!  All images on the web are left=red, right=blue!                    
				b) fixed image display when viewing a 3D preset in a non-4:3 aspect ratio window
				c) gamma now works for 3d presets!  (note: you might have to update your old presets.
						if they were 3D presets, the gamma was ignored and 1.0 was used; now,
						if gamma was >1.0 in the old preset, it will probably appear extremely bright.)
				d) added SHIFT+F9 and CTRL+C9 to inc/dec stereo separation
				e) added default stereo separation to config panel
			-cranked up the max. mesh size (was 48x36, now 64x48) and the default mesh size
				(was 24x18, now 32x24)
			-fixed aspect ratio for final display
			-auto-texsize is now computed slightly differently; for vertically or horizontally-stretched
				windows, the texsize is now biased more toward the larger dimension (vs. just the
				average).
			-added anisotropic filtering (for machines that support it)
			-fixed bug where the values of many variables in the preset init code were not set prior 
				to execution of the init code (e.g. time, bass, etc. were all broken!)
			-added various preset blend effects.  In addition to the old uniform fade, there is
				now a directional wipe, radial wipe, and plasma fade.
			-FIXED SLOW TEXT for DX8 (at least, on the geforce 4).  
				Not using DT_RIGHT or DT_BOTTOM was the key.

		
		-why does text kill it in desktop mode?
		-text is SLOOW
		-to do: add (and use) song title font + tooltip font
		-re-test: menus, text, boxes, etc.
		-re-test: TIME        
		-testing:
			-make sure sound works perfectly.  (had to repro old pre-vms sound analysis!)
			-autogamma: currently assumes/requires that GetFrame() resets to 0 on a mode change
				(i.e. windowed -> fullscreen)... is that the case?
			-restore motion vectors
			-
			-
		-restore lost surfaces
		-test bRedraw flag (desktop mode/paused)
		-search for //? in milkdropfs.cpp and fix things
			
		problem: no good soln for VJ mode
		problem: D3DX won't give you solid background for your text.
			soln: (for later-) create wrapper fn that draws w/solid bkg.

		SOLN?: use D3DX to draw all text (plugin.cpp stuff AND playlist); 
		then, for VJ mode, create a 2nd DxContext 
		w/its own window + windowproc + fonts.  (YUCK)
	1) config panel: test, and add WM_HELP's (copy from tooltips)
	2) keyboard input: test; and...
		-need to reset UI_MODE when playlist is turned on, and
		-need to reset m_show_playlist when UI_MODE is changed.  (?)
		-(otherwise they can both show @ same time and will fight 
			for keys and draw over each other)
	3) comment out most of D3D stuff in milkdropfs.cpp, and then 
		get it running w/o any milkdrop, but with text, etc.
	4) sound

  Issues / To Do Later
  --------------------
	1) sprites: color keying stuff probably won't work any more...
	2) scroll lock: pull code from Monkey
	3) m_nGridY should not always be m_nGridX*3/4!
	4) get solid backgrounds for menus, waitstring code, etc.
		(make a wrapper function!)

	99) at end: update help screen

  Things that will be different
  -----------------------------
	1) font sizes are no longer relative to size of window; they are absolute.
	2) 'don't clear screen at startup' option is gone
	3) 'always on top' option is gone
	4) text will not be black-on-white when an inverted-color preset is showing

				-VJ mode:
					-notes
						1. (remember window size/pos, and save it from session to session?  nah.)
						2. (kiv: scroll lock)
						3. (VJ window + desktop mode:)
								-ok w/o VJ mode
								-w/VJ mode, regardless of 'fix slow text' option, probs w/focus;
									click on vj window, and plugin window flashes to top of Z order!
								-goes away if you comment out 1st call to PushWindowToJustBeforeDesktop()...
								-when you disable PushWindowToJustBeforeDesktop:
									-..and click on EITHER window, milkdrop jumps in front of the taskbar.
									-..and click on a non-MD window, nothing happens.
								d-FIXED somehow, magically, while fixing bugs w/true fullscreen mode!
						4. (VJ window + true fullscreen mode:)
								d-make sure VJ window gets placed on the right monitor, at startup,
									and respects taskbar posn.
								d-bug - start in windowed mode, then dbl-clk to go [true] fullscreen 
									on 2nd monitor, all with VJ mode on, and it excepts somewhere 
									in m_text.DrawNow() in a call to DrawPrimitive()!
									FIXED - had to check m_vjd3d8_device->TestCooperativeLevel
									each frame, and destroy/reinit if device needed reset.
								d-can't resize VJ window when grfx window is running true fullscreen!
									-FIXED, by dropping the Sleep(30)/return when m_lost_focus
										was true, and by not consuming WM_NCACTIVATE in true fullscreen
										mode when m_hTextWnd was present, since DX8 doesn't do its
										auto-minimize thing in that case.



*/

#include "platform.h"
#include "plugin.h"
#include <chrono>

using namespace render;

#define FRAND frand()


extern float AdjustRateToFPS(float per_frame_decay_rate_at_fps1, float fps1, float actual_fps);


// glue factory
IVisualizerPtr CreateVisualizer(ContextPtr context, ITextureSetPtr texture_map, std::string pluginDir)
{
    std::shared_ptr<CPlugin> plugin = std::make_shared<CPlugin>(context, texture_map, pluginDir );
	if (!plugin->PluginInitialize())
	{
		return nullptr;
	}
	return plugin;
}

#define IsNumericChar(x) (x >= '0' && x <= '9')

std::string CPlugin::LoadShaderCode(const char* szBaseFilename)
{
	std::string szFile = m_assetDir + "/shaders/" + szBaseFilename;
	std::replace(szFile.begin(), szFile.end(), '\\', '/');
    
    std::string text;
    if (!FileReadAllText(szFile, text))
    {
        LogError("Can't read file %s", szFile.c_str());
        assert(0);
        return text;
    }
    return text;
}



//----------------------------------------------------------------------

CPlugin::CPlugin(ContextPtr context, ITextureSetPtr texture_map, std::string assetDir)
	:m_context(context), m_texture_map(texture_map), m_drawbuffer(render::CreateDrawBuffer(context))
{
	m_frame = 0;
	m_time = 0;
	m_fps = 60;
    
    m_audio = CreateAudioAnalyzer();

	// Initialize EVERY data member you've added to CPlugin here;
	//   these will be the default values.
	// If you want to initialize any of your variables with random values
	//   (using rand()), be sure to seed the random number generator first!
	// (If you want to change the default values for settings that are part of
	//   the plugin shell (framework), do so from OverrideDefaults() above.)

	// seed the system's random number generator w/the current system time:
	//srand((unsigned)time(NULL));  -don't - let winamp do it

	// CONFIG PANEL SETTINGS THAT WE'VE ADDED (TAB #2)
	m_nTexSizeX			= -1;	// -1 means "auto"
	m_nTexSizeY			= -1;	// -1 means "auto"
    
    m_FormatRGBA8 = context->IsSupported(PixelFormat::BGRA8Unorm) ? PixelFormat::BGRA8Unorm : PixelFormat::RGBA8Unorm;
    m_InternalFormat   = context->IsSupported(PixelFormat::RGBA16Unorm) ? PixelFormat::RGBA16Unorm : m_FormatRGBA8;

    if (PixelFormatIsHDR(context->GetDisplayFormat()))
    {
        m_OutputFormat = PixelFormat::RGBA16Float;
    }
    else
    {
        m_OutputFormat = m_FormatRGBA8;
    }
    
	m_nGridX			= 48;//32;
	m_nGridY			= 36;//24;

	// RUNTIME SETTINGS THAT WE'VE ADDED
	m_PresetDuration     = 0.0f;
	m_NextPresetDuration = 0.0f;
	m_fSnapPoint = 0.5f;

    // load empty presets
    LoadEmptyPreset();
    LoadEmptyPreset();

	m_verts.clear();
	m_vertinfo.clear();
    m_indices_list.clear();

	// --------------------other init--------------------

	m_assetDir = assetDir;
}

CPlugin::~CPlugin()
{
	ReleaseResources();
}

//----------------------------------------------------------------------

std::string StripComments(const std::string &str)
{
	std::string dest;
	dest.reserve(str.size());
	for (size_t i = 0; i < str.size(); )
	{
		char c = str[i];
		if (c == '/')
		{
			if (str[i + 1] == '/')
			{
				i = str.find('\n', i);
				if (i == std::string::npos)
					break;
				continue;
			}

			if (str[i + 1] == '*')
			{
				i = str.find("*/", i);
				if (i == std::string::npos)
					break;
				i += 2;
				continue;
			}

		}

		// keep char
		dest.push_back(c);
		i++;
	}
	return dest;
}

//----------------------------------------------------------------------

static float SquishToCenter(float x, float fExp)
{
	if (x > 0.5f)
		return powf(x*2-1, fExp)*0.5f + 0.5f;

	return (1-powf(1-x*2, fExp))*0.5f;
}



void CPlugin::SetFixedShader(TexturePtr texture)
{
    SetFixedShader(texture, SAMPLER_WRAP, SAMPLER_LINEAR);
}

void CPlugin::SetFixedShader(TexturePtr texture, SamplerAddress addr, SamplerFilter filter)
{
    m_context->SetShader(m_shader_fixed);
    
    auto main_sampler = m_shader_fixed->GetSampler(0);
    if (main_sampler)
    {
        main_sampler->SetTexture(texture, addr, filter);
    }    
}


ShaderPtr CPlugin::LoadShaderFromFile(const char *name)
{
    std::string rootDir = m_assetDir + "/shaders/";
    std::string path = PathCombine(rootDir, name);
    

    std::string code = LoadShaderCode(name);
    
    auto shader = m_context->CreateShader(name);
    if (!shader->CompileAndLink(
                    {
                        ShaderSource{ShaderType::Vertex,   path, code, "VS", "vs_1_1", "hlsl"},
                        ShaderSource{ShaderType::Fragment, path, code, "PS", "ps_3_0", "hlsl"}
                    }
                ))
    {
        // error
        LogError("%s\n", shader->GetErrors().c_str());
    }
                        
    
    return shader;
}

void CPlugin::SetOutputSize(Size2D size)
{
    // size hasn't changed
    if (m_nTexSizeX == size.width && m_nTexSizeY == size.height)
        return;
    
    m_nTexSizeX = size.width;
    m_nTexSizeY = size.height;
    AllocateOutputTexture();
    AllocateVertexData();
}


void CPlugin::AllocateOutputTexture()
{
    m_outputTexture = m_context->CreateRenderTarget("output", m_nTexSizeX, m_nTexSizeY, m_OutputFormat );
    
    m_lpVS[0] = m_context->CreateRenderTarget("VS1", m_nTexSizeX, m_nTexSizeY, m_InternalFormat);
    assert(m_lpVS[0]);
    m_lpVS[1] = m_context->CreateRenderTarget("VS2", m_nTexSizeX, m_nTexSizeY, m_InternalFormat);
	assert(m_lpVS[1]);

    LogPrint("Created render target texture(%dx%dx%s)\n",
             m_lpVS[0]->GetWidth(),
             m_lpVS[0]->GetHeight(),
             PixelFormatToString(m_lpVS[0]->GetFormat())
             );


    // create blur textures w/same format.  A complete mip chain costs 33% more video mem then 1 full-sized VS.
    int w = m_nTexSizeX;
    int h = m_nTexSizeY;
    int i =0;
    m_blur_textures.clear();
    m_blurtemp_textures.clear();
    while (m_blur_textures.size() < 3)
    {
        char texname[256];
        
        bool temp_texture =  (i % 2) == 0 ;
        sprintf(texname, "blur%d%s", i / 2 + 1, (!temp_texture) ? "" : "-horizontal");

        // main VS = 1024
        // blur0 = 512
        // blur1 = 256  <-  user sees this as "blur1"
        // blur2 = 128
        // blur3 = 128  <-  user sees this as "blur2"
        // blur4 =  64
        // blur5 =  64  <-  user sees this as "blur3"
        if (!temp_texture || (i<2))
        {
            w = std::max(16, w/2);
            h = std::max(16, h/2);
        }
        int w2 = ((w+3)/16)*16;
        int h2 = ((h+3)/4)*4;
        
        auto texture  = m_context->CreateRenderTarget(texname, w2, h2, m_InternalFormat);
        if (!temp_texture) {
            m_blur_textures.push_back(texture);
        } else {
            m_blurtemp_textures.push_back(texture);
        }

//        LogPrint("Created render target blur texture %s (%dx%d)\n", texname, w2, h2 );


        // add it to m_textures[].
        i++;
    }

    m_fAspectX = (m_nTexSizeY > m_nTexSizeX) ? m_nTexSizeX/(float)m_nTexSizeY : 1.0f;
    m_fAspectY = (m_nTexSizeX > m_nTexSizeY) ? m_nTexSizeY/(float)m_nTexSizeX : 1.0f;
    m_fInvAspectX = 1.0f/m_fAspectX;
    m_fInvAspectY = 1.0f/m_fAspectY;
    
    m_clearTargets = true;
    
}

void CPlugin::AllocateVertexData()
{
        // BUILD VERTEX LIST for final composite blit
        //   note the +0.5-texel offset!
        //   (otherwise, a 1-pixel-wide line of the image would wrap at the top and left edges).
        m_comp_verts.clear();
        m_comp_verts.reserve(FCGSX*FCGSY);
        //float fOnePlusInvWidth  = 1.0f + 1.0f/(float)GetWidth();
        //float fOnePlusInvHeight = 1.0f + 1.0f/(float)GetHeight();
        float fHalfTexelW =  m_context->GetTexelOffset() / (float)GetWidth();   // 2.5: 2 pixels bad @ bottom right
        float fHalfTexelH =  m_context->GetTexelOffset() / (float)GetHeight();
        float fDivX = 1.0f / (float)(FCGSX-2);
        float fDivY = 1.0f / (float)(FCGSY-2);
        for (int j=0; j<FCGSY; j++)
        {
            int j2 = j - j/(FCGSY/2);
            float v = j2*fDivY;
            v = SquishToCenter(v, 3.0f);
            float sy = -((v-fHalfTexelH)*2-1);//fOnePlusInvHeight*v*2-1;
            for (int i=0; i<FCGSX; i++)
            {
                int i2 = i - i/(FCGSX/2);
                float u = i2*fDivX;
                u = SquishToCenter(u, 3.0f);
                float sx = (u-fHalfTexelW)*2-1;//fOnePlusInvWidth*u*2-1;
                float rad, ang;
                UvToMathSpace( u, v, &rad, &ang );
                    // fix-ups:
                   if (i==FCGSX/2-1) {
                       if (j < FCGSY/2-1)
                           ang = M_PI*1.5f;
                       else if (j == FCGSY/2-1)
                           ang = M_PI*1.25f;
                       else if (j == FCGSY/2)
                           ang = M_PI*0.75f;
                       else
                           ang = M_PI*0.5f;
                   }
                   else if (i==FCGSX/2) {
                       if (j < FCGSY/2-1)
                           ang = M_PI*1.5f;
                       else if (j == FCGSY/2-1)
                           ang = M_PI*1.75f;
                       else if (j == FCGSY/2)
                           ang = M_PI*0.25f;
                       else
                           ang = M_PI*0.5f;
                   }
                   else if (j==FCGSY/2-1) {
                       if (i < FCGSX/2-1)
                           ang = M_PI*1.0f;
                       else if (i == FCGSX/2-1)
                           ang = M_PI*1.25f;
                       else if (i == FCGSX/2)
                           ang = M_PI*1.75f;
                       else
                           ang = M_PI*2.0f;
                   }
                   else if (j==FCGSY/2) {
                       if (i < FCGSX/2-1)
                           ang = M_PI*1.0f;
                       else if (i == FCGSX/2-1)
                           ang = M_PI*0.75f;
                       else if (i == FCGSX/2)
                           ang = M_PI*0.25f;
                       else
                           ang = M_PI*0.0f;
                   }
                
                // store vertex
                Vertex p;
                p.x = sx;
                p.y = sy;
                p.z = 0;
                p.tu = u;
                p.tv = v;
                p.rad = rad;
                p.ang = ang;
                p.Diffuse = 0xFFFFFFFF;
                m_comp_verts.push_back(p);
            }
        }

        // build index list for final composite blit -
        // order should be friendly for interpolation of 'ang' value!
        m_comp_indices.clear();
        m_comp_indices.reserve( (FCGSX-2)*(FCGSY-2)*2*3 );
        for (int y=0; y<FCGSY-1; y++)
        {
            if (y==FCGSY/2-1)
                continue;
            for (int x=0; x<FCGSX-1; x++)
            {
                if (x==FCGSX/2-1)
                    continue;
                bool left_half = (x < FCGSX/2);
                bool top_half  = (y < FCGSY/2);
                bool center_4 = ((x==FCGSX/2 || x==FCGSX/2-1) && (y==FCGSY/2 || y==FCGSY/2-1));

                if ( ((int)left_half + (int)top_half + (int)center_4) % 2 )
                {
                    m_comp_indices.push_back( (y  )*FCGSX + (x  ) );
                    m_comp_indices.push_back( (y  )*FCGSX + (x+1) );
                    m_comp_indices.push_back( (y+1)*FCGSX + (x+1) );
                    m_comp_indices.push_back( (y+1)*FCGSX + (x+1) );
                    m_comp_indices.push_back( (y+1)*FCGSX + (x  ) );
                    m_comp_indices.push_back( (y  )*FCGSX + (x  ) );
                }
                else
                {
                    m_comp_indices.push_back( (y+1)*FCGSX + (x  ) );
                    m_comp_indices.push_back( (y  )*FCGSX + (x  ) );
                    m_comp_indices.push_back( (y  )*FCGSX + (x+1) );
                    m_comp_indices.push_back( (y  )*FCGSX + (x+1) );
                    m_comp_indices.push_back( (y+1)*FCGSX + (x+1) );
                    m_comp_indices.push_back( (y+1)*FCGSX + (x  ) );
                }
            }
        }
     
        // -----------------
        // -----------------
        
        
        //dumpmsg("Init: mesh allocation");
        m_verts.clear();
        m_verts.reserve( (m_nGridX+1)*(m_nGridY+1) + (m_nGridX / 2) );
        m_vertinfo.clear();
        m_vertinfo.reserve((m_nGridX+1)*(m_nGridY+1));
        m_indices_list.clear();
        m_indices_list.reserve( m_nGridX*m_nGridY*6 );

    //    float texel_offset_x = m_context->GetTexelOffset() / (float)m_nTexSizeX;
    //    float texel_offset_y = m_context->GetTexelOffset() / (float)m_nTexSizeY;
        for (int y=0; y<=m_nGridY; y++)
        {
            for (int x=0; x<=m_nGridX; x++)
            {
                // precompute x,y,z
                float vx = (float)x/(float)m_nGridX*2.0f - 1.0f;
                float vy = (float)y/(float)m_nGridY*2.0f - 1.0f;
                
                
                // setup vertex info
                td_vertinfo vi;
                vi.vx = vx;
                vi.vy = vy;

                // precompute rad, ang, being conscious of aspect ratio
                vi.rad = sqrtf(vx*vx*m_fAspectX*m_fAspectX + vy*vy*m_fAspectY*m_fAspectY);
                if (y==m_nGridY/2 && x==m_nGridX/2)
                    vi.ang = 0.0f;
                else
                    vi.ang = atan2f(vy*m_fAspectY, vx*m_fAspectX);
                vi.a = 1;
                vi.c = 0;
                m_vertinfo.push_back(vi);


                // setup vertex
                Vertex v;
                v.x = vx;
                v.y = vy;
                v.z = 0.0f;
                v.rad = vi.rad;
                v.ang = vi.ang;
                v.tu =  vx*0.5f + 0.5f;
                v.tv = -vy*0.5f + 0.5f;
                v.Diffuse = 0xFFFFFFFF;
                m_verts.push_back(v);
    //
    //            printf("%04d (%02d,%02d) %f %f %f %f\n", (int)m_verts.size() - 1,
    //                   x, y, vx, vy, vi.rad, vi.ang);
            }
        }
        
        
       

        
        // generate triangle strips for the 4 quadrants.
        // each quadrant has m_nGridY/2 strips.
        // each strip has m_nGridX+2 *points* in it, or m_nGridX/2 polygons.
        int xref, yref;

        // also generate triangle lists for drawing the main warp mesh.
        for (int quadrant=0; quadrant<4; quadrant++)
        {
            for (int slice=0; slice < m_nGridY/2; slice++)
            {
                for (int i=0; i < m_nGridX/2; i++)
                {
                    // quadrants:    2 3
                    //                0 1
                    xref = i;
                    yref = slice;

                    if (quadrant & 1)
                        xref = m_nGridX-1 - xref;
                    if (quadrant & 2)
                        yref = m_nGridY-1 - yref;

                    int v = xref + (yref)*(m_nGridX+1);

                    m_indices_list.push_back(  v );
                    m_indices_list.push_back(  v           +1);
                    m_indices_list.push_back(  v+m_nGridX+1  );
                    m_indices_list.push_back(  v           +1);
                    m_indices_list.push_back(  v+m_nGridX+1  );
                    m_indices_list.push_back(  v+m_nGridX+1+1);
                }
            }
        }
        
        
    #if 1
        {
            int y_offset_neg = (m_nGridY/2) * (m_nGridX + 1);
            int y_offset_pos = (int)m_verts.size();
            int width = m_nGridX / 2;
            
            // add duplicate row for inverted angle
            for (int x=0; x < width; x++)
            {
                m_verts.push_back(m_verts[y_offset_neg + x]);
            }
            
            for (int x=0; x < width; x++)
            {
                m_verts[y_offset_neg + x].ang = -M_PI;
                m_verts[y_offset_pos + x].ang =  M_PI;
            }
            
            // fix second half of indice to use new vertices instead
            m_indices_list_wrapped = m_indices_list;
            for (size_t i=m_indices_list_wrapped.size() / 2; i < m_indices_list_wrapped.size(); i++)
            {
                IndexType index = m_indices_list_wrapped[i];
                if ((int)index >= y_offset_neg && (int)index < (y_offset_neg+width))
                {
                    // remap index
                    IndexType new_index = index - y_offset_neg + y_offset_pos;
                  //  printf("remap %d: %d -> %d\n", (int)i, index, new_index);
                    m_indices_list_wrapped[i] = new_index;
                }
            }
            
        }
    #endif

}


bool CPlugin::AllocateResources() 
{
	// (...aka OnUserResizeWindow) 
	// (...aka OnToggleFullscreen)

    m_szShaderIncludeText      = LoadShaderCode("include.fx");
    m_szDefaultWarpVShaderText = LoadShaderCode("warp_vs.fx");
    m_szDefaultWarpPShaderText = LoadShaderCode("warp_ps.fx");
    m_szDefaultCompVShaderText = LoadShaderCode("comp_vs.fx");
    m_szDefaultCompPShaderText = LoadShaderCode("comp_ps.fx");

    // load fixed function shaders
    m_shader_fixed = LoadShaderFromFile("fixed.fx");
    m_shader_custom_shapes = LoadShaderFromFile("custom_shapes.fx");
    m_shader_custom_waves = LoadShaderFromFile("custom_waves.fx");
    m_shader_warp_fixed = LoadShaderFromFile("warp_fixed.fx");
    m_shader_output = LoadShaderFromFile("output.fx");


    m_shader_blur1 = LoadShaderFromFile("blur1.fx");
    m_shader_blur2 = LoadShaderFromFile("blur2.fx");

	// GENERATED TEXTURES FOR SHADERS
	//-------------------------------------
	{
		// Generate noise textures
		AddNoiseTex("noise_lq",      256, 1);
		AddNoiseTex("noise_lq_lite",  32, 1);
		AddNoiseTex("noise_mq",      256, 4);
		AddNoiseTex("noise_hq",      256, 8);

#if 0
        AddNoiseVol("noisevol_lq", 32, 1);
        AddNoiseVol("noisevol_hq", 32, 4);
#else
        AddNoiseTex("noisevol_lq", 256, 1);
        AddNoiseTex("noisevol_hq", 256, 4);
#endif
	}
    
    Randomize();

	return true;
}


static float fCubicInterpolate(float y0, float y1, float y2, float y3, float t)
{
   float a0,a1,a2,a3,t2;

   t2 = t*t;
   a0 = y3 - y2 - y0 + y1;
   a1 = y0 - y1 - a0;
   a2 = y2 - y0;
   a3 = y1;

   return(a0*t*t2+a1*t2+a2*t+a3);
}

static uint32_t dwCubicInterpolate(uint32_t y0, uint32_t y1, uint32_t y2, uint32_t y3, float t)
{
	// performs cubic interpolation on a D3DCOLOR value.
	uint32_t ret = 0;
	uint32_t shift = 0;
	for (int i=0; i<4; i++) 
	{
		float f = fCubicInterpolate(  
			((y0 >> shift) & 0xFF)/255.0f, 
			((y1 >> shift) & 0xFF)/255.0f, 
			((y2 >> shift) & 0xFF)/255.0f, 
			((y3 >> shift) & 0xFF)/255.0f, 
			t 
		);
		if (f<0)
			f = 0;
		if (f>1)
			f = 1;
		ret |= ((uint32_t)(f*255)) << shift;
		shift += 8;
	}
	return ret;
}

void CPlugin::AddNoiseTex(const char* szTexName, int size, int zoom_factor)
{
	// size = width & height of the texture; 
	// zoom_factor = how zoomed-in the texture features should be.
	//           1 = random noise
	//           2 = smoothed (interp)
	//           4/8/16... = cubic interp.

	// Synthesize noise texture(s)
	uint32_t *data = new uint32_t[size * size];

	// write to the bits...
	uint32_t* dst = (uint32_t*)data;
	int uint32_ts_per_line = size;
	int RANGE = (zoom_factor > 1) ? 216 : 256;
	for (int y=0; y<size; y++) {
//		LARGE_INTEGER q;
//		QueryPerformanceCounter(&q);
//		srand(q.LowPart ^ q.HighPart ^ warand());
		for (int x=0; x<size; x++) {
			dst[x] = (((uint32_t)(warand(RANGE) )+RANGE/2) << 24) |
					 (((uint32_t)(warand(RANGE) )+RANGE/2) << 16) |
					 (((uint32_t)(warand(RANGE) )+RANGE/2) <<  8) |
					 (((uint32_t)(warand(RANGE) )+RANGE/2)      );
		}
		// swap some pixels randomly, to improve 'randomness'
		for (int x=0; x<size; x++)
		{
			int x1 = (warand(size));
			int x2 = (warand(size));
			uint32_t temp = dst[x2];
			dst[x2] = dst[x1];
			dst[x1] = temp;
		}
		dst += uint32_ts_per_line;
	}

	// smoothing
	if (zoom_factor > 1) 
	{
		// first go ACROSS, blending cubically on X, but only on the main lines.
		uint32_t* dst = (uint32_t*)data;
		for (int y=0; y<size; y+=zoom_factor)
			for (int x=0; x<size; x++) 
				if (x % zoom_factor)
				{
					int base_x = (x/zoom_factor)*zoom_factor + size;
					int base_y = y*uint32_ts_per_line;
					uint32_t y0 = dst[ base_y + ((base_x - zoom_factor  ) % size) ];
					uint32_t y1 = dst[ base_y + ((base_x                ) % size) ];
					uint32_t y2 = dst[ base_y + ((base_x + zoom_factor  ) % size) ];
					uint32_t y3 = dst[ base_y + ((base_x + zoom_factor*2) % size) ];

					float t = (x % zoom_factor)/(float)zoom_factor;

					uint32_t result = dwCubicInterpolate(y0, y1, y2, y3, t);
					
					dst[ y*uint32_ts_per_line + x ] = result;        
				}
		
		// next go down, doing cubic interp along Y, on every line.
		for (int x=0; x<size; x++) 
			for (int y=0; y<size; y++)
				if (y % zoom_factor)
				{
					int base_y = (y/zoom_factor)*zoom_factor + size;
					uint32_t y0 = dst[ ((base_y - zoom_factor  ) % size)*uint32_ts_per_line + x ];
					uint32_t y1 = dst[ ((base_y                ) % size)*uint32_ts_per_line + x ];
					uint32_t y2 = dst[ ((base_y + zoom_factor  ) % size)*uint32_ts_per_line + x ];
					uint32_t y3 = dst[ ((base_y + zoom_factor*2) % size)*uint32_ts_per_line + x ];

					float t = (y % zoom_factor)/(float)zoom_factor;

					uint32_t result = dwCubicInterpolate(y0, y1, y2, y3, t);
					
					dst[ y*uint32_ts_per_line + x ] = result;        
				}

	}


	// add it to m_textures[].  
	TexturePtr texptr  = m_context->CreateTexture(szTexName, size, size, m_FormatRGBA8, data);
    m_texture_map->AddTexture(szTexName, texptr);
	delete[] data;
//	LogPrint("Created noise texture %s (%dx%d)\n", szTexName, size, size);
}

#if 0
bool CPlugin::AddNoiseVol(const char* szTexName, int size, int zoom_factor)
{
	// size = width & height & depth of the texture; 
	// zoom_factor = how zoomed-in the texture features should be.
	//           1 = random noise
	//           2 = smoothed (interp)
	//           4/8/16... = cubic interp.

	// Synthesize noise texture(s)
	uint32_t *data = new uint32_t[size * size * size];

	// write to the bits...
	int uint32_ts_per_slice = size * size;
	int uint32_ts_per_line = size;
	int RANGE = (zoom_factor > 1) ? 216 : 256;
	for (int z=0; z<size; z++) {
		uint32_t* dst = data + z*uint32_ts_per_slice;
		for (int y=0; y<size; y++) {
//			LARGE_INTEGER q;
//			QueryPerformanceCounter(&q);
//			srand(q.LowPart ^ q.HighPart ^ warand());
			for (int x=0; x<size; x++) {
				dst[x] = (((uint32_t)(warand(RANGE))+RANGE/2) << 24) |
						 (((uint32_t)(warand(RANGE))+RANGE/2) << 16) |
						 (((uint32_t)(warand(RANGE))+RANGE/2) <<  8) |
						 (((uint32_t)(warand(RANGE))+RANGE/2)      );
			}
			// swap some pixels randomly, to improve 'randomness'
			for (int x=0; x<size; x++)
			{
				int x1 = warand(size);
				int x2 = warand(size);
				uint32_t temp = dst[x2];
				dst[x2] = dst[x1];
				dst[x1] = temp;
			}
			dst += uint32_ts_per_line;
		}
	}

	// smoothing
	if (zoom_factor > 1) 
	{
		// first go ACROSS, blending cubically on X, but only on the main lines.
		uint32_t* dst = (uint32_t*)data;
		for (int z=0; z<size; z+=zoom_factor)
			for (int y=0; y<size; y+=zoom_factor)
				for (int x=0; x<size; x++) 
					if (x % zoom_factor)
					{
						int base_x = (x/zoom_factor)*zoom_factor + size;
						int base_y = z*uint32_ts_per_slice + y*uint32_ts_per_line;
						uint32_t y0 = dst[ base_y + ((base_x - zoom_factor  ) % size) ];
						uint32_t y1 = dst[ base_y + ((base_x                ) % size) ];
						uint32_t y2 = dst[ base_y + ((base_x + zoom_factor  ) % size) ];
						uint32_t y3 = dst[ base_y + ((base_x + zoom_factor*2) % size) ];

						float t = (x % zoom_factor)/(float)zoom_factor;

						uint32_t result = dwCubicInterpolate(y0, y1, y2, y3, t);
					
						dst[ z*uint32_ts_per_slice + y*uint32_ts_per_line + x ] = result;        
					}
		
		// next go down, doing cubic interp along Y, on the main slices.
		for (int z=0; z<size; z+=zoom_factor)
			for (int x=0; x<size; x++) 
				for (int y=0; y<size; y++)
					if (y % zoom_factor)
					{
						int base_y = (y/zoom_factor)*zoom_factor + size;
						int base_z = z*uint32_ts_per_slice;
						uint32_t y0 = dst[ ((base_y - zoom_factor  ) % size)*uint32_ts_per_line + base_z + x ];
						uint32_t y1 = dst[ ((base_y                ) % size)*uint32_ts_per_line + base_z + x ];
						uint32_t y2 = dst[ ((base_y + zoom_factor  ) % size)*uint32_ts_per_line + base_z + x ];
						uint32_t y3 = dst[ ((base_y + zoom_factor*2) % size)*uint32_ts_per_line + base_z + x ];

						float t = (y % zoom_factor)/(float)zoom_factor;

						uint32_t result = dwCubicInterpolate(y0, y1, y2, y3, t);
					
						dst[ y*uint32_ts_per_line + base_z + x ] = result;        
					}

		// next go through, doing cubic interp along Z, everywhere.
		for (int x=0; x<size; x++) 
			for (int y=0; y<size; y++)
				for (int z=0; z<size; z++)
					if (z % zoom_factor)
					{
						int base_y = y*uint32_ts_per_line;
						int base_z = (z/zoom_factor)*zoom_factor + size;
						uint32_t y0 = dst[ ((base_z - zoom_factor  ) % size)*uint32_ts_per_slice + base_y + x ];
						uint32_t y1 = dst[ ((base_z                ) % size)*uint32_ts_per_slice + base_y + x ];
						uint32_t y2 = dst[ ((base_z + zoom_factor  ) % size)*uint32_ts_per_slice + base_y + x ];
						uint32_t y3 = dst[ ((base_z + zoom_factor*2) % size)*uint32_ts_per_slice + base_y + x ];

						float t = (z % zoom_factor)/(float)zoom_factor;

						uint32_t result = dwCubicInterpolate(y0, y1, y2, y3, t);
					
						dst[ z*uint32_ts_per_slice + base_y + x ] = result;        
					}

	}


	// add it to m_textures[].
	TexturePtr texptr  = m_context->CreateVolumeTexture(szTexName, size, size, size, data);
	m_textures.push_back(texptr);
	delete[] data;

	LogPrint("Created noise volume texture %s (%dx%dx%d)\n", szTexName, size, size, size);
	return true;
}
#endif

bool CPlugin::PickRandomTexture(const std::string &prefix, std::string &szRetTextureFilename)
{
    std::vector<render::TexturePtr> list;
    m_texture_map->GetTextureListWithPrefix(prefix, list);
    if (list.empty())
        return false;

    // pick random texture
    int i = warand((int)list.size());
    szRetTextureFilename = list[i]->GetName();
    return true;
}

render::TexturePtr CPlugin::LookupTexture(const std::string &name)
{
    return m_texture_map->LookupTexture(name);
}

TexturePtr CPlugin::LoadDiskTexture(const std::string &name)
{
    return m_texture_map->LookupTexture(name);
}

static bool StartsWith(const std::string &str, const std::string &prefix)
{
    return (str.find(prefix) == 0);
}


void ShaderInfo::CacheParams(CPlugin *plugin)
{
	if (!shader) return;

	#define MAX_RAND_TEX 16
	std::string RandTexName[MAX_RAND_TEX];

    
    _texture_bindings.clear();

	// pass 1: find all the samplers (and texture bindings).
	for (int i=0; i< shader->GetSamplerCount(); i++)
	{
		ShaderSamplerPtr shader_sampler = shader->GetSampler(i);
        const std::string &name = shader_sampler->GetName();

        SamplerInfo tb;
        tb.shader_sampler = shader_sampler;

			// remove "sampler_" prefix to create root file name.  could still have "FW_" prefix or something like that.
            std::string szRootName;
			if ( StartsWith(name, "sampler_"))
				szRootName = name.substr(8);
			else
				szRootName = name;

			// also peel off "XY_" prefix, if it's there, to specify filtering & wrap mode.
			bool bBilinear = true;
			bool bWrap     = true;
			bool bWrapFilterSpecified = false;
			if (szRootName.size() > 3 && szRootName[2]=='_')
			{
                std::string temp = szRootName.substr(0, 2);

                // convert to uppercase
                temp[0] = toupper(temp[0]);
                temp[1] = toupper(temp[1]);

				if      (temp == "FW") { bWrapFilterSpecified = true; bBilinear = true;  bWrap = true; }
				else if (temp == "FC") { bWrapFilterSpecified = true; bBilinear = true;  bWrap = false; }
				else if (temp == "PW") { bWrapFilterSpecified = true; bBilinear = false; bWrap = true; }
				else if (temp == "PC") { bWrapFilterSpecified = true; bBilinear = false; bWrap = false; }
				// also allow reverses:
				else if (temp == "WF") { bWrapFilterSpecified = true; bBilinear = true;  bWrap = true; }
				else if (temp == "CF") { bWrapFilterSpecified = true; bBilinear = true;  bWrap = false; }
				else if (temp == "WP") { bWrapFilterSpecified = true; bBilinear = false; bWrap = true; }
				else if (temp == "CP") { bWrapFilterSpecified = true; bBilinear = false; bWrap = false; }

				// peel off the prefix
                szRootName = szRootName.substr(3);
			}
			tb.bWrap     = bWrap;
			tb.bBilinear = bBilinear;

			// if <szFileName> is "main", map it to the VS...
			if (szRootName == "main")
			{
				tb.texptr    = NULL;
				tb.texcode = TEX_VS;
			}
			else if (szRootName == "blur1")
			{
                tb.texptr    = NULL;
				tb.texcode     = TEX_BLUR1;
				if (!bWrapFilterSpecified) { // when sampling blur textures, default is CLAMP
					tb.bWrap = false;
					tb.bBilinear = true;
				}
			}
            else if (szRootName == "blur2")
            {
                tb.texptr    = NULL;
                tb.texcode     = TEX_BLUR2;
                if (!bWrapFilterSpecified) { // when sampling blur textures, default is CLAMP
                    tb.bWrap = false;
                    tb.bBilinear = true;
                }
            }
            else if (szRootName == "blur3")
            {
                tb.texptr    = NULL;
                tb.texcode    = TEX_BLUR3;
                if (!bWrapFilterSpecified) { // when sampling blur textures, default is CLAMP
                    tb.bWrap = false;
                    tb.bBilinear = true;
                }
            }
			else
			{
				tb.texcode = TEX_DISK;

                std::string tex_path = szRootName;

				// check for request for random texture.
				if (StartsWith(szRootName, "rand") &&
                    szRootName.size() >= 6 &&
					IsNumericChar(szRootName[4]) &&
					IsNumericChar(szRootName[5]) && 
					(szRootName.size() == 6  || szRootName[6]=='_')
                    )
				{
					int rand_slot = -1;
					
					// peel off filename prefix ("rand13_smalltiled", for example)
                    std::string prefix;
					if (szRootName.size() >= 7 && szRootName[6]=='_')
						prefix = szRootName.substr(7);
					szRootName = szRootName.substr(0, 6);
                    
                    std::string rand_slot_str = szRootName.substr(4, 2);
					sscanf(rand_slot_str.c_str(), "%d", &rand_slot);
                    
					if (rand_slot >= 0 && rand_slot <= 15)      // otherwise, not a special filename - ignore it
					{
						if (!plugin->PickRandomTexture(prefix, tex_path))
						{
							if (prefix[0])
								tex_path = StringFormat("[rand%02d] %s*", rand_slot, prefix.c_str());
							else
								tex_path = StringFormat("[rand%02d] *", rand_slot);
						}
						else 
						{
                            tex_path = PathRemoveExtension(tex_path);
						}

						assert(RandTexName[rand_slot].size() == 0);
						RandTexName[rand_slot] = tex_path; // we'll need to remember this for texsize_ params!
					}
				}
                
                tb.texptr = plugin->LoadDiskTexture(tex_path);
			}
		
        _texture_bindings.push_back(tb);
	}

	// pass 2: bind all the float4's.  "texsize_XYZ" params will be filled out via knowledge of loaded texture sizes.
	for (int i=0; i< shader->GetConstantCount(); i++)
	{
		ShaderConstantPtr h = shader->GetConstant(i);
        const std::string &name = h->GetName();

//        if (cd.RegisterSet == D3DXRS_FLOAT4)
		{
  //          if (cd.Class == D3DXPC_MATRIX_COLUMNS) 
			{
				if      (name == "rot_s1" ) rot_mat[0]  = h;
				else if (name == "rot_s2" ) rot_mat[1]  = h;
				else if (name == "rot_s3" ) rot_mat[2]  = h;
				else if (name == "rot_s4" ) rot_mat[3]  = h;
				else if (name == "rot_d1" ) rot_mat[4]  = h;
				else if (name == "rot_d2" ) rot_mat[5]  = h;
				else if (name == "rot_d3" ) rot_mat[6]  = h;
				else if (name == "rot_d4" ) rot_mat[7]  = h;
				else if (name == "rot_f1" ) rot_mat[8]  = h;
				else if (name == "rot_f2" ) rot_mat[9]  = h;
				else if (name == "rot_f3" ) rot_mat[10] = h;
				else if (name == "rot_f4" ) rot_mat[11] = h;
				else if (name == "rot_vf1") rot_mat[12] = h;
				else if (name == "rot_vf2") rot_mat[13] = h;
				else if (name == "rot_vf3") rot_mat[14] = h;
				else if (name == "rot_vf4") rot_mat[15] = h;
				else if (name == "rot_uf1") rot_mat[16] = h;
				else if (name == "rot_uf2") rot_mat[17] = h;
				else if (name == "rot_uf3") rot_mat[18] = h;
				else if (name == "rot_uf4") rot_mat[19] = h;
				else if (name == "rot_rand1") rot_mat[20] = h;
				else if (name == "rot_rand2") rot_mat[21] = h;
				else if (name == "rot_rand3") rot_mat[22] = h;
				else if (name == "rot_rand4") rot_mat[23] = h;
			}
	//        else if (cd.Class == D3DXPC_VECTOR)
			{
				if      (name == "rand_frame")  rand_frame  = h;
				else if (name == "rand_preset") rand_preset = h;
				else if (StartsWith(name, "texsize_"))
				{
					// remove "texsize_" prefix to find root file name.
                    std::string szRootName  = name.substr(8);

					// check for request for random texture.
					// it should be a previously-seen random index - just fetch/reuse the name.
                    if (szRootName.size() >= 6 &&
                        StartsWith(szRootName, "rand") &&
                        IsNumericChar(szRootName[4]) &&
                        IsNumericChar(szRootName[5]) &&
                        (szRootName.size() == 6  || szRootName[6]=='_')
                        )
					{
						int rand_slot = -1;

						// ditch filename prefix ("rand13_smalltiled", for example)
						// and just go by the slot

                        std::string rand_slot_str = szRootName.substr(4, 2);
                        sscanf(rand_slot_str.c_str(), "%d", &rand_slot);
                        
						if (rand_slot >= 0 && rand_slot <= 15)      // otherwise, not a special filename - ignore it
							if (!RandTexName[rand_slot].empty())
								szRootName = RandTexName[rand_slot];
					}

					// see if <szRootName>.tga or .jpg has already been loaded.
                    auto texture = plugin->LookupTexture(szRootName);
                    if (texture)
                    {
                        // found a match - texture was loaded
                        TexSizeParamInfo y;
                        y.texname       = szRootName; //for debugging
                        y.texsize_param = h;
                        y.w             = texture->GetWidth();
                        y.h             = texture->GetHeight();
                        texsize_params.push_back(y);
					} else
					{
						LogError("UNABLE_TO_RESOLVE_TEXSIZE_FOR_A_TEXTURE_NOT_IN_USE %s\n", name.c_str() );
					}
				}
				else if (name[0] == '_' && name[1] == 'c') 
				{
					int z;
					if (sscanf(&name[2], "%d", &z)==1) 
						if (z >= 0 && z < sizeof(const_handles)/sizeof(const_handles[0]))
							const_handles[z] = h;
				}
				else if (name[0] == '_' && name[1] == 'q') 
				{
					int z = name[2] - 'a';
					if (z >= 0 && z < sizeof(q_const_handles)/sizeof(q_const_handles[0]))
						q_const_handles[z] = h;
				}
			}
		}
	}
    
    

     // find highest blur used
     highest_blur_used = 0;
     for (const auto &tb : _texture_bindings)
     {
         if (!tb.shader_sampler)
             continue;
         tex_code texcode = tb.texcode;
         // finally, if it was a blur texture, note that
         if (texcode >= TEX_BLUR1 && texcode <= TEX_BLUR3)
         {
             int blur = ((int)texcode - (int)TEX_BLUR1) + 1;
             highest_blur_used = std::max(highest_blur_used, blur);
         }
     }

}

//----------------------------------------------------------------------

//bool CPlugin::RecompileShader(const std::string &vs_text, const std::string &ps_text, ShaderInfoPtr *psi, int shaderType)
ShaderInfoPtr       CPlugin::RecompileShader(const std::string &name, const std::string &vs_text, const std::string &ps_text, int shaderType, std::string &errors)
{
	if (vs_text.empty() || ps_text.empty())
        return nullptr;
    
    PROFILE_FUNCTION_CAT("load")

    
//    LogPrint("RecompileShader %s\n", name.c_str());

    
    StopWatch sw;

	// compile shaders
    std::string rootDir = PathCombine(m_assetDir, "data");
    std::string path = PathCombine(rootDir, name);
    if (shaderType == SHADER_WARP) path += "+warp";
    if (shaderType == SHADER_COMP) path += "+comp";
    
    
	std::string vs_code = ComposeShader(vs_text, "VS", "vs_1_1", shaderType);
    std::string ps_code = ComposeShader(ps_text, "PS", "ps_3_0", shaderType);

    auto shader = m_context->CreateShader( PathGetFileName(path).c_str());
    if (!shader->CompileAndLink({
        ShaderSource{ShaderType::Vertex,    path, vs_code, "VS", "vs_1_1", "hlsl"},
        ShaderSource{ShaderType::Fragment,  path, ps_code, "PS", "ps_3_0", "hlsl"}
    }))
	{
		//assert(0);
        errors = shader->GetErrors();
        
        path += "+Error";
        
        // create error shader
        ps_code = ComposeShader("shader_body\n{\nret=float3(1,0,0);\n}\n", "PS", "ps_3_0", shaderType);
        shader = m_context->CreateShader(PathGetFileName(path).c_str());
        if (!shader->CompileAndLink({
            ShaderSource{ShaderType::Vertex,     path, vs_code, "VS", "vs_1_1", "hlsl"},
            ShaderSource{ShaderType::Fragment,   path, ps_code, "PS", "ps_3_0", "hlsl"}
        }))
        {
            assert(0);
            return nullptr;
        }
            
	}


    // create shader info
    ShaderInfoPtr si = std::make_shared<ShaderInfo>();
    si->shader = shader;
	si->CacheParams(this);
    si->compileTime = sw.GetElapsedMilliSeconds();
	return si;
}

std::string CPlugin::ComposeShader( const std::string &szOrigShaderText, const char* szFn, const char* szProfile, int shaderType)
{
    const char szWarpParams[] = "VS_OUTPUT_WARP _v";
    const char szCompParams[] = "VS_OUTPUT_COMP _v";

    
    const char szLastLine[] = "    return float4(ret.xyz, _v.color.w);";
    
    std::string szShaderText;
//    szShaderText.append(m_szShaderIncludeText);
//    szShaderText.append("\n");
    
    // paste in any custom #defines for this shader type
    std::string szFirstLine = "\n    float3 ret = 0;\n";
    if (shaderType == SHADER_WARP && szProfile[0]=='p')
    {
        const char szWarpDefines[] =
        "float rad = _v.rad_ang.x;\n"
        "float ang = _v.rad_ang.y;\n"
        "float2 uv = _v.uv.xy;\n"
        "float2 uv_orig = _v.uv.zw;\n"
        "float4 _vDiffuse = _v.color;\n";

        szFirstLine.append(szWarpDefines);
        szFirstLine.append("\n");
    }
    else if (shaderType == SHADER_COMP && szProfile[0]=='p')
    {
        const char szCompDefines[] =
        "float rad = _v.rad_ang.x;\n"
        "float ang = _v.rad_ang.y;\n"
        "float2 uv = _v.uv.xy;\n"
        "float2 uv_orig = _v.uv.xy;\n" // [sic]
        "float4 _vDiffuse = _v.color;\n"
        "float3 hue_shader = _v.color.xyz;\n";

        szFirstLine.append(szCompDefines);
        szFirstLine.append("\n");
    }
    
    szShaderText.append(szOrigShaderText);
    szShaderText.append("\n");
    
    // strip out all comments
    szShaderText = StripComments(szShaderText);
    
    /*
     1. paste warp or comp #defines
     2. search for "void" + whitespace + szFn + [whitespace] + '('
     3. insert params
     4. search for [whitespace] + ')'.
     5. search for final '}' (strrchr)
     6. back up one char, insert the Last Line, and add '}' and that's it.
     */
    if ((shaderType == SHADER_WARP || shaderType == SHADER_COMP) && szProfile[0]=='p')
    {
        std::string find_str = "shader_body";
        size_t body = szShaderText.find(find_str);
        if (body != std::string::npos)
        {
            const char *params = (shaderType == SHADER_WARP) ? szWarpParams : szCompParams;
            char fstr[4096];
            sprintf(fstr, "float4 %s( %s ) : COLOR0\n", szFn, params);
            szShaderText.replace(body, find_str.size(), fstr);

            
            size_t start = szShaderText.find('{', body);
            if (start != std::string::npos)
            {
                size_t end = szShaderText.rfind("}");
                if (end != std::string::npos)
                {
                    // truncate everything after the last } in the shader, some people leave comment text after the last }
                    szShaderText.resize(end + 1);
                    
                    szShaderText.insert(end, szLastLine);
                    szShaderText.insert(start + 1, szFirstLine);
                }
                else
                {
                    assert(0);
                }
            }
            else
            {
                assert(0);
            }
        }
    }

    szShaderText.insert(0, "\n// END include.fx \n");
    szShaderText.insert(0, m_szShaderIncludeText);
    szShaderText.insert(0, "\n// BEGIN include.fx\n");

    
    return szShaderText;
    //return m_context->CompileShader(szShaderText.c_str(), szFn, szProfile);
}

//----------------------------------------------------------------------

void CPlugin::ReleaseResources()
{
	// Clean up all your DX9 and D3DX textures, fonts, buffers, etc. here.
	// EVERYTHING CREATED IN ALLOCATEMYDX9STUFF() SHOULD BE CLEANED UP HERE.
	// The input parameter, 'final_cleanup', will be 0 if this is 
	//   a routine cleanup (part of a window resize or switch between
	//   fullscr/windowed modes), or 1 if this is the final cleanup
	//   and the plugin is exiting.  Note that even if it is a routine
	//   cleanup, *you still have to release ALL your DirectX stuff,
	//   because the DirectX device is being destroyed and recreated!*
	// Also set all the pointers back to NULL;
	//   this is important because if we go to reallocate the DX9
	//   stuff later, and something fails, then CleanUp will get called,
	//   but it will then be trying to clean up invalid pointers.)
	// The SafeRelease() and SafeDelete() macros make your code prettier;
	//   they are defined here in utility.h as follows:
	//       #define SafeRelease(x) if (x) {x->Release(); x=NULL;}
	//       #define SafeDelete(x)  if (x) {delete x; x=NULL;} 
	// IMPORTANT:
	//   This function ISN'T only called when the plugin exits!
	//   It is also called whenever the user toggles between fullscreen and 
	//   windowed modes, or resizes the window.  Basically, on these events, 
	//   the base class calls CleanUpMyDX9Stuff before Reset()ing the DirectX 
	//   device, and then calls AllocateMyDX9Stuff afterwards.



	// One funky thing here: if we're switching between fullscreen and windowed,
	//  or doing any other thing that causes all this stuff to get reloaded in a second,
	//  then if we were blending 2 presets, jump fully to the new preset.
	// Otherwise the old preset wouldn't get all reloaded, and it app would crash
	//  when trying to use its stuff.

	// just force this:
	m_bBlending = false;


    m_blur_textures.clear();
    m_blurtemp_textures.clear();


	// 2. release stuff
	m_lpVS[0].reset();
	m_lpVS[1].reset();

    m_verts.clear();
    m_vertinfo.clear();
    m_indices_list.clear();
}

//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------


void CPlugin::DrawQuad(TexturePtr texture, float x, float y, float w, float h, Color4F color)
{
    Vertex v[4];
    memset(v, 0, sizeof(v));
    
    float x0 = x + 0.0f;
    float x1 = x + w;
    float y0 = y + 0;
    float y1 = y + h;
    //    y0 =  -y0;
    //    y1 =  -y1;
    
    v[0].x = x0;
    v[0].y = y0;
    v[1].x = x1;
    v[1].y = y0;
    v[2].x = x0;
    v[2].y = y1;
    v[3].x = x1;
    v[3].y = y1;
    
    v[0].tu = 0;
    v[0].tv = 0;
    v[1].tu = 1;
    v[1].tv = 0;
    v[2].tu = 0;
    v[2].tv = 1;
    v[3].tu = 1;
    v[3].tv = 1;
    
    auto c32 = color.ToU32();
    for (int i=0; i < 4; i++)
    {
        v[i].Diffuse = c32;
    }
    
    //
    SetFixedShader(texture);
    
    m_context->DrawArrays(PRIMTYPE_TRIANGLESTRIP, 4,  v);
}


void CPlugin::Draw(ContentMode mode, float alpha)
{
    
    PROFILE_FUNCTION()
    
    Matrix44 saved = m_context->GetTransform();
    
    Size2D size = m_context->GetRenderTargetSize();
    float sw = (float)size.width;
    float sh = (float)size.height;
    
    Size2D tex_size = m_outputTexture->GetSize();
    float tw = (float)tex_size.width;
    float th = (float)tex_size.height;

    
    float ratio_w = tw / sw;
    float ratio_h = th / sh;
    
    // setup ortho projections
    Matrix44 m = MatrixOrthoLH(0, sw, sh, 0, 0, 1);
    m_context->SetTransform(m);
    
    
    float ow = sw, oh = sh;

//    mode = ContentMode::ScaleAspectFit;
//        mode = ContentMode::ScaleToFill;
    switch (mode)
    {
        case ContentMode::ScaleAspectFill:
        {
            float ratio = std::min(ratio_w, ratio_h);
            ow = tw / ratio;
            oh = th / ratio;
            break;
        }

        case ContentMode::ScaleAspectFit:
        {
            float ratio = std::max(ratio_w, ratio_h);
            ow = tw / ratio;
            oh = th / ratio;
            break;
        }
            
        case ContentMode::ScaleToFill:
        {
            ow = sw;
            oh = sh;
            break;

        }
            
        default:
            break;
    }
    
    
    float x = -(ow - sw) / 2;
    float y = -(oh - sh) / 2;
    
    

//    m_context->SetBlendDisable();
    m_context->SetDepthEnable(false);
//    DrawQuad(m_outputTexture, x, y, ow, oh, Color4F(1,1,1,alpha));

    Color4F color = Color4F(1,1,1,alpha);
    
     Vertex v[4];
     memset(v, 0, sizeof(v));
     
     float x0 = x + 0.0f;
     float x1 = x + ow;
     float y0 = y + 0;
     float y1 = y + oh;

     
     v[0].x = x0; v[0].y = y0;
     v[1].x = x1; v[1].y = y0;
     v[2].x = x0; v[2].y = y1;
     v[3].x = x1; v[3].y = y1;
     
     v[0].tu = 0; v[0].tv = 0;
     v[1].tu = 1; v[1].tv = 0;
     v[2].tu = 0; v[2].tv = 1;
     v[3].tu = 1; v[3].tv = 1;
     
     auto c32 = color.ToU32();
     for (int i=0; i < 4; i++)
     {
         v[i].Diffuse = c32;
     }
    
    auto sampler = m_shader_output->GetSampler(0);
	if (sampler)
		sampler->SetTexture(m_outputTexture);
    m_context->SetShader(m_shader_output);
     
    m_context->DrawArrays(PRIMTYPE_TRIANGLESTRIP, 4,  v);
    
    m_context->SetTransform(saved);
}


TexturePtr CPlugin::GetInternalTexture()
{
    return m_lpVS[0];
}


TexturePtr CPlugin::GetOutputTexture()
{
    return m_outputTexture;
}

void CPlugin::DrawAudioUI()
{
    //        TableNameValue("SampleRate", "%.fhz", m_audioSource->GetSampleRate());
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::Text("BlockSize");
    ImGui::TableNextColumn();
    ImGui::Text("%d samples\n",  m_audio->GetBlockSize() );
    ImGui::TableNextColumn();

    const char *channels[] = {"Left", "Right", nullptr};
    for (int i=0; channels[i]; i++)
    {

        ImGui::Separator();

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("%s", channels[i]);
        ImGui::TableNextColumn();
        m_audio->DrawChannelUI(i);
        ImGui::TableNextColumn();
    }
}

void CPlugin::DrawDebugUI()
{
    
//    DrawStatusWindow();
   
//    m_presetDebugUI = true;
    if (m_showPresetEditor)
        m_pState->DebugUI(&m_showPresetEditor);

//    m_pluginDebugUI = true;
    if (m_pluginDebugUI)
    {
//        static bool _open = true;;
       // const float DISTANCE = 10.0f;
//        static int corner = 0;
//        ImGuiIO& io = ImGui::GetIO();
//        if (corner != -1)
        {
//            ImVec2 window_pos = ImVec2((corner & 1) ? io.DisplaySize.x - DISTANCE : DISTANCE, (corner & 2) ? io.DisplaySize.y - DISTANCE : DISTANCE);
//            ImVec2 window_pos_pivot = ImVec2((corner & 1) ? 1.0f : 0.0f, (corner & 2) ? 1.0f : 0.0f);

//            ImVec2 window_pos = ImVec2(DISTANCE, DISTANCE);
//            ImVec2 window_pos_pivot = ImVec2(0.0f, 0.0f);
//            ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
//
//            ImGui::SetNextWindowContentWidth(400);

        }
       // ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background

        if (ImGui::Begin("Visualizer", &m_pluginDebugUI))

        
//        if (ImGui::Begin("Visualizer", &_open, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
        {
//            if (!m_bBlending)
//            {
//                ImGui::Text("Preset: %s\n", m_pState->m_desc.c_str() );
//
//                //ImGui::Text("Preset: %s\n", m_pState->m_desc.c_str() );
//            }
//            else
//            {
//
//            }

//            ImGui::TextWrapped("Preset:        %s\n", m_pState->GetDescription().c_str()   );
//            if (m_bBlending)
//                ImGui::TextWrapped("Preset (old):  %s\n", m_pOldState->GetDescription() );

//            ImGui::Separator();
//
//            ImGui::Text("Progress: ");
//            ImGui::SameLine();
//            ImGui::ProgressBar( GetPresetProgress(), ImVec2(100, 10) );
//
//            ImGui::Separator();

//            ImGui::LabelText("RenderTime", "%3.2fms", m_renderFrameTime );

            
            
//            ImGui::Separator();

          //   auto stats = m_scriptEngine->GetStats();

//            ImGui::InvisibleButton("table", ImVec2(300, 1));
//            ImGui::Columns(2);
//
//                ImGui::SetColumnOffset(0, 0);
//                ImGui::SetColumnOffset(1, 100);
//
//                ImGui::Text("Script compile count:");
//                ImGui::NextColumn();
//                ImGui::Text("%d",  stats->compileCount );
//                ImGui::NextColumn();
//
//                ImGui::Text("Script compile time:");
//                ImGui::NextColumn();
//                ImGui::Text("%3.2fms\n", stats->compileTime );
//                ImGui::NextColumn();
//            ImGui::Columns(1);
            
//            ImGui::Text("Debug: %s\n",  PlatformIsDebug() ? "true" : "false" );
//            ImGui::Text("Platform: %s\n", PlatformGetName() );

//            ImGui::Separator();


        

//            ImGui::Text("RenderTime:        %3.2fms\n", m_renderFrameTime );
//            ImGui::Text("Script compile count: %d\n",  stats->compileCount );
//            ImGui::Text("Script compile time:  %3.2fms\n", stats->compileTime );
//            ImGui::Text("Script eval count: %d\n",  stats->evalCount );
//            ImGui::Text("Script eval time:  %3.2fms\n", stats->evalTime );


//            if (m_pState->m_shader_comp)
//            ImGui::Text("Shader comp compile time:  %3.2fms\n", m_pState->m_shader_comp->compileTime );
//            if (m_pState->m_shader_warp)
//            ImGui::Text("Shader warp compile time:  %3.2fms\n", m_pState->m_shader_warp->compileTime );
//
            
            
            ImGui::Separator();

            ImGui::Text("Compute pervertex:    %3.2fms\n", m_pState->m_pervertex_buffer->GetExecuteTime());

            for (auto wave : m_pState->m_waves)
            {
                if (wave->enabled)
                {
                    ImGui::Text("Wave[%d] time_compute_end:  %3.2fms\n", wave->index, wave->time_compute_end );
                }
            }
            
            for (auto shape : m_pState->m_shapes)
            {
                if (shape->enabled)
                {
                    ImGui::Text("Shape[%d] time_compute_end:%3.2fms compute:%3.2fms\n", shape->index,
                                shape->time_compute_end,
                                shape->m_perframe_buffer->GetExecuteTime() );
                }
            }
            

            if (ImGui::CollapsingHeader("Blur Textures"))
            {

                float scale = 1.0f;

                for (int i=0; i < m_blur_textures.size(); i++)
                {
                    if (i != 0 )
                        ImGui::SameLine();

                    auto texture = m_blur_textures[i];
                    ImGui::Image(texture.get(),
                                 ImVec2(texture->GetWidth() * scale,
                                        texture->GetHeight()  * scale)
                                 );
                }
            }
            
            
            
            if (ImGui::CollapsingHeader("Render Targets"))
            {
                
                float scale = 0.25f;
                
                for (int i=0; i < 1; i++)
                {
                    if (i != 0 )
                    ImGui::SameLine();
                    
                    auto texture = m_lpVS[i];
                    ImGui::Image(texture.get(),
                                 ImVec2(texture->GetWidth() * scale,
                                        texture->GetHeight()  * scale)
                                 );
                }
            }
            
            
//            if (ImGui::Button("Show Waveform"))
//            {
//                s_showWaveForm = !s_showWaveForm;
//            }
//            
//            if (ImGui::Button("Show Blur Textures"))
//            {
//                s_showBlurTextures = !s_showBlurTextures;
//            }
            
          
            
            
//            if (ImGui::CollapsingHeader("Scripting"))
//            {
//              //  m_scriptEngine->DrawDebugUI();
//            }
//
//            if (ImGui::CollapsingHeader("Output Texture"))
//            {
//
//                float scale = 0.25f;
//
//                auto texture = m_outputTexture;
//                ImGui::Image(texture.get(),
//                             ImVec2(texture->GetWidth() * scale,
//                                    texture->GetHeight()  * scale)
//                             );
//            }
//
//
//            if (ImGui::CollapsingHeader("Blur Textures"))
//            {
//
//                float scale = 0.25f;
//
//                for (int i=0; i < 3; i++)
//                {
//                    if (i != 0 )
//                        ImGui::SameLine();
//
//                    auto texture = m_lpBlur[i*2+1];
//                    ImGui::Image(texture.get(),
//                                 ImVec2(texture->GetWidth() * scale,
//                                        texture->GetHeight()  * scale)
//                                 );
//                }
//            }

//
            /*
            ImGui::Text("Simple overlay\n" "in the corner of the screen.\n" "(right-click to change position)");
            ImGui::Separator();
            if (ImGui::IsMousePosValid())
                ImGui::Text("Mouse Position: (%.1f,%.1f)", io.MousePos.x, io.MousePos.y);
            else
                ImGui::Text("Mouse Position: <invalid>");
            if (ImGui::BeginPopupContextWindow())
            {
                if (ImGui::MenuItem("Custom",       NULL, corner == -1)) corner = -1;
                if (ImGui::MenuItem("Top-left",     NULL, corner == 0)) corner = 0;
                if (ImGui::MenuItem("Top-right",    NULL, corner == 1)) corner = 1;
                if (ImGui::MenuItem("Bottom-left",  NULL, corner == 2)) corner = 2;
                if (ImGui::MenuItem("Bottom-right", NULL, corner == 3)) corner = 3;
                if (ImGui::MenuItem("Close")) _open = false;
                ImGui::EndPopup();
            }
             */
        }
        //auto &io = ImGui::GetIO();
        
        
        ImGui::End();
    }
    
   

}


//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------



int CPlugin::warand(int modulo)
{
    std::uniform_int_distribution<int>    dist(0, modulo - 1);
    return dist(m_random_generator);
}

float CPlugin::frand()
{
    std::uniform_real_distribution<float>    dist(0.0f, 1.0f);
    return dist(m_random_generator);
}


void CPlugin::SetRandomSeed(uint32_t seed)
{
    m_random_generator.seed(seed);
    
    Randomize();
    
    m_frame = 0;
    m_time  = 0;
}


bool CPlugin::PluginInitialize()
{
	if (!AllocateResources())
	{
		return false;
	}

	return true;
}


//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------


void CPlugin::Render(float dt, Size2D output_size, IAudioSourcePtr audioSource)
{
    SetOutputSize(output_size);
    
    audioSource->ReadAudioFrame(dt, m_samples);
    m_audio->Update(dt, m_samples);
//    m_samples.Modulate(m_audioGain);


	m_fps = 1.0f / dt;
    
    
    RenderFrame();  // see milkdropfs.cpp

	// advance time
	m_frame++;
	m_time += dt;
    
    m_PresetDuration += dt;

}


//----------------------------------------------------------------------

int CPlugin::HandleRegularKey(char key)
{
	// here we handle all the normal keys for milkdrop-
	// these are the hotkeys that are used when you're not
	// in the middle of editing a string, navigating a menu, etc.
	
	// do not make references to virtual keys here; only
	// straight WM_CHAR messages should be sent in.  

	// return 0 if you process/absorb the key; otherwise return 1.

	switch(key)
	{
	// row 1 keys
	case 'q':
		m_pState->m_fVideoEchoZoom /= 1.05f;
		return 0; // we processed (or absorbed) the key
	case 'Q':
		m_pState->m_fVideoEchoZoom *= 1.05f;
		return 0; // we processed (or absorbed) the key
	case 'w':
		m_pState->m_nWaveMode++;
		if (m_pState->m_nWaveMode >= NUM_WAVES) m_pState->m_nWaveMode = 0;
      //  LogPrint("WaveMode: %d\n", m_pState->m_nWaveMode);
		return 0; // we processed (or absorbed) the key
	case 'W':
		m_pState->m_nWaveMode--;
		if (m_pState->m_nWaveMode < 0) m_pState->m_nWaveMode = NUM_WAVES - 1;
      //  LogPrint("WaveMode: %d\n", m_pState->m_nWaveMode);
		return 0; // we processed (or absorbed) the key
	case 'e':
		m_pState->m_fWaveAlpha -= 0.1f; 
		if (m_pState->m_fWaveAlpha.eval(-1) < 0.0f) m_pState->m_fWaveAlpha = 0.0f;
		return 0; // we processed (or absorbed) the key
	case 'E':
		m_pState->m_fWaveAlpha += 0.1f; 
		//if (m_pState->m_fWaveAlpha.eval(-1) > 1.0f) m_pState->m_fWaveAlpha = 1.0f;
		return 0; // we processed (or absorbed) the key

	case 'I':	m_pState->m_fZoom -= 0.01f;		return 0; // we processed (or absorbed) the key
	case 'i':	m_pState->m_fZoom += 0.01f;		return 0; // we processed (or absorbed) the key

	case 'o':	m_pState->m_fWarpAmount /= 1.1f;	return 0; // we processed (or absorbed) the key
	case 'O':	m_pState->m_fWarpAmount *= 1.1f;	return 0; // we processed (or absorbed) the key
	
	
	case 'f':
	case 'F':
		m_pState->m_nVideoEchoOrientation = (m_pState->m_nVideoEchoOrientation + 1) % 4;
		return 0; // we processed (or absorbed) the key
	case 'g':
		m_pState->m_fGammaAdj -= 0.1f;
		if (m_pState->m_fGammaAdj.eval(-1) < 0.0f) m_pState->m_fGammaAdj = 0.0f;
		return 0; // we processed (or absorbed) the key
	case 'G':
		m_pState->m_fGammaAdj += 0.1f;
		//if (m_pState->m_fGammaAdj > 1.0f) m_pState->m_fGammaAdj = 1.0f;
		return 0; // we processed (or absorbed) the key
	case 'j':
		m_pState->m_fWaveScale *= 0.9f;
		return 0; // we processed (or absorbed) the key
	case 'J':
		m_pState->m_fWaveScale /= 0.9f;
		return 0; // we processed (or absorbed) the key

	// row 3/misc. keys

	case '[':
		m_pState->m_fXPush -= 0.005f;
		return 0; // we processed (or absorbed) the key
	case ']':
		m_pState->m_fXPush += 0.005f;
		return 0; // we processed (or absorbed) the key
	case '{':
		m_pState->m_fYPush -= 0.005f;
		return 0; // we processed (or absorbed) the key
	case '}':
		m_pState->m_fYPush += 0.005f;
		return 0; // we processed (or absorbed) the key
	case '<':
		m_pState->m_fRot += 0.02f;
		return 0; // we processed (or absorbed) the key
	case '>':
		m_pState->m_fRot -= 0.02f;
		return 0; // we processed (or absorbed) the key

	case 's':				// SAVE PRESET
	case 'S':
		break;

	case 'm':
	case 'M':
        DumpState();
		return 0; // we processed (or absorbed) the key


    }

	return 1;
}

void CPlugin::DumpState()
{
    m_pState->Dump();

}


//----------------------------------------------------------------------

void CPlugin::Randomize()
{
	//srand((int)(GetTime()*100));
	//m_fAnimTime		= (warand(51234L))*0.01f;
	m_fRandStart[0]		= (warand(64841L))*0.01f;
	m_fRandStart[1]		= (warand(53751L))*0.01f;
	m_fRandStart[2]		= (warand(42661L))*0.01f;
	m_fRandStart[3]		= (warand(31571L))*0.01f;

	//CState temp;
	//temp.Randomize(warand() % NUM_MODES);
	//m_pState->StartBlend(&temp, m_fAnimTime, m_fBlendTimeUser);
}

//----------------------------------------------------------------------


void CPlugin::RandomizeBlendPattern()
{
	if (m_vertinfo.empty())
		return;

	// note: we now avoid constant uniform blend b/c it's half-speed for shader blending. 
	//       (both old & new shaders would have to run on every pixel...)
	int mixtype = 1 + (warand(3));//warand()%4;

	if (mixtype==0)
	{
		// constant, uniform blend
		int nVert = 0;
		for (int y=0; y<=m_nGridY; y++)
		{
			for (int x=0; x<=m_nGridX; x++)
			{
				m_vertinfo[nVert].a = 1;
				m_vertinfo[nVert].c = 0;
				nVert++;
			}
		}
	}
	else if (mixtype==1)
	{
		// directional wipe
		float ang = FRAND*6.28f;
		float vx = cosf(ang);
		float vy = sinf(ang);
		float band = 0.1f + 0.2f*FRAND; // 0.2 is good
		float inv_band = 1.0f/band;
	
		int nVert = 0;
		for (int y=0; y<=m_nGridY; y++)
		{
			float fy = (y/(float)m_nGridY)*m_fAspectY;
			for (int x=0; x<=m_nGridX; x++)
			{
				float fx = (x/(float)m_nGridX)*m_fAspectX;

				// at t==0, mix rangse from -10..0
				// at t==1, mix ranges from   1..11

				float t = (fx-0.5f)*vx + (fy-0.5f)*vy + 0.5f;
				t = (t-0.5f)/sqrtf(2.0f) + 0.5f;

				m_vertinfo[nVert].a = inv_band * (1 + band);
				m_vertinfo[nVert].c = -inv_band + inv_band*t;//(x/(float)m_nGridX - 0.5f)/band;
				nVert++;
			}
		}
	}
	else if (mixtype==2)
	{
		// plasma transition
		float band = 0.12f + 0.13f*FRAND;//0.02f + 0.18f*FRAND;
		float inv_band = 1.0f/band;

		// first generate plasma array of height values
		m_vertinfo[                               0].c = FRAND;
		m_vertinfo[                        m_nGridX].c = FRAND;
		m_vertinfo[m_nGridY*(m_nGridX+1)           ].c = FRAND;
		m_vertinfo[m_nGridY*(m_nGridX+1) + m_nGridX].c = FRAND;
		GenPlasma(0, m_nGridX, 0, m_nGridY, 0.25f);

		// then find min,max so we can normalize to [0..1] range and then to the proper 'constant offset' range.
		float minc = m_vertinfo[0].c;
		float maxc = m_vertinfo[0].c;
		int x,y,nVert;
	
		nVert = 0;
		for (y=0; y<=m_nGridY; y++)
		{
			for (x=0; x<=m_nGridX; x++)
			{
				if (minc > m_vertinfo[nVert].c)
					minc = m_vertinfo[nVert].c;
				if (maxc < m_vertinfo[nVert].c)
					maxc = m_vertinfo[nVert].c;
				nVert++;
			}
		}

		float mult = 1.0f/(maxc-minc);
		nVert = 0;
		for (y=0; y<=m_nGridY; y++)
		{
			for (x=0; x<=m_nGridX; x++)
			{
				float t = (m_vertinfo[nVert].c - minc)*mult;
				m_vertinfo[nVert].a = inv_band * (1 + band);
				m_vertinfo[nVert].c = -inv_band + inv_band*t;
				nVert++;
			}
		}
	}
	else if (mixtype==3)
	{
		// radial blend
		float band = 0.02f + 0.14f*FRAND + 0.34f*FRAND;
		float inv_band = 1.0f/band;
		float dir = (float)((warand(2))*2 - 1);      // 1=outside-in, -1=inside-out

		int nVert = 0;
		for (int y=0; y<=m_nGridY; y++)
		{
			float dy = (y/(float)m_nGridY - 0.5f)*m_fAspectY;
			for (int x=0; x<=m_nGridX; x++)
			{
				float dx = (x/(float)m_nGridX - 0.5f)*m_fAspectX;
				float t = sqrtf(dx*dx + dy*dy)*1.41421f;
				if (dir==-1)
					t = 1-t;

				m_vertinfo[nVert].a = inv_band * (1 + band);
				m_vertinfo[nVert].c = -inv_band + inv_band*t;
				nVert++;
			}
		}
	}
}

void CPlugin::GenPlasma(int x0, int x1, int y0, int y1, float dt)
{
	int midx = (x0+x1)/2;
	int midy = (y0+y1)/2;
	float t00 = m_vertinfo[y0*(m_nGridX+1) + x0].c;
	float t01 = m_vertinfo[y0*(m_nGridX+1) + x1].c;
	float t10 = m_vertinfo[y1*(m_nGridX+1) + x0].c;
	float t11 = m_vertinfo[y1*(m_nGridX+1) + x1].c;

	if (y1-y0 >= 2)
	{
		if (x0==0)
			m_vertinfo[midy*(m_nGridX+1) + x0].c = 0.5f*(t00 + t10) + (FRAND*2-1)*dt*m_fAspectY;
		m_vertinfo[midy*(m_nGridX+1) + x1].c = 0.5f*(t01 + t11) + (FRAND*2-1)*dt*m_fAspectY;
	}
	if (x1-x0 >= 2)
	{
		if (y0==0)
			m_vertinfo[y0*(m_nGridX+1) + midx].c = 0.5f*(t00 + t01) + (FRAND*2-1)*dt*m_fAspectX;
		m_vertinfo[y1*(m_nGridX+1) + midx].c = 0.5f*(t10 + t11) + (FRAND*2-1)*dt*m_fAspectX;
	}

	if (y1-y0 >= 2 && x1-x0 >= 2)
	{
		// do midpoint & recurse:
		t00 = m_vertinfo[midy*(m_nGridX+1) + x0].c;
		t01 = m_vertinfo[midy*(m_nGridX+1) + x1].c;
		t10 = m_vertinfo[y0*(m_nGridX+1) + midx].c;
		t11 = m_vertinfo[y1*(m_nGridX+1) + midx].c;
		m_vertinfo[midy*(m_nGridX+1) + midx].c = 0.25f*(t10 + t11 + t00 + t01) + (FRAND*2-1)*dt;

		GenPlasma(x0, midx, y0, midy, dt*0.5f);
		GenPlasma(midx, x1, y0, midy, dt*0.5f);
		GenPlasma(x0, midx, midy, y1, dt*0.5f);
		GenPlasma(midx, x1, midy, y1, dt*0.5f);
	}
}


PresetPtr CPlugin::LoadPreset(const std::string &text, std::string name, std::string &errors)
{
    PROFILE_FUNCTION_CAT("load")
    StopWatch sw;
    
//    LogPrint("LoadingPreset: '%s' (%fms)\n", name.c_str(), sw.GetElapsedMilliSeconds() );
    
    CStatePtr state = std::make_shared<CState>(this);
    if (!state->ImportFromText(text, name, errors)) {
        // failed to import...
        return nullptr;
    }
    
    state->time_import = sw.GetElapsedMilliSeconds();
    
//    LogPrint("LoadPreset: '%s' (%fms)\n", state->m_desc.c_str(), sw.GetElapsedMilliSeconds() );

    // loaded!
    return state;
}



void CPlugin::LoadEmptyPreset()
{
    CStatePtr state = std::make_shared<CState>(this);

    state->Default();
    state->m_nWaveMode = 9;
    state->m_bWaveThick = false;
    state->m_bWaveDots = false;
    state->m_fDecay = 0.0f;
    state->m_fWarpAmount = 0.0f;
    state->m_fMvR = 0.0f;
    state->m_fMvG = 0.0f;
    state->m_fMvB = 0.0f;
    state->m_fMvA = 0.0f;
//    state->m_bDarken =true;
//    state->m_bDarkenCenter = true;
    state->RecompileExpressions();

//    LogPrint("LoadEmptyPreset %s\n", state->m_desc.c_str());

    m_pOldState = m_pState;
    m_pState = state;
    m_bBlending = false;
    m_fBlendStartTime = GetTime();
    m_fBlendDuration = 0.0f;
}





void CPlugin::SetPreset(CStatePtr preset, PresetLoadArgs args)
{
    PROFILE_FUNCTION()
    
    if (!preset) {
        LoadEmptyPreset();
        return;
    }
    
    if (!m_context->IsThreaded())
    {
        std::string errors;
        preset->RecompileShaders(errors);
        if (!errors.empty())
        {
            LogError("ERROR: Shader compilation '%s'\n%s\n",
                     preset->GetName().c_str(),
                     errors.c_str());
            return;
        }
    }
    
	//LogPrint("LoadPreset %s\n", loaded->m_desc.c_str());

    // if no preset was valid before, make sure there is no blend, because there is nothing valid to blend from.
    if (m_pState->m_name.empty())
        args.blendTime = 0;

    // set as current state, keep old state
    m_pOldState = m_pState;
    m_pState = preset;

	if (!m_pOldState || args.blendTime == 0)
	{
        m_clearTargets = true;
        m_bBlending = false;
        m_fBlendStartTime = GetTime();
        m_fBlendDuration = 0.0f;
	}
	else 
	{
		RandomizeBlendPattern();

        // setup blend parameters
        m_bBlending = true;
        m_fBlendStartTime = GetTime();
        m_fBlendDuration = args.blendTime;
		m_pState->StartBlendFrom(m_pOldState.get(), m_fBlendStartTime, m_fBlendDuration);
	}

    
    // reset preset duration
    m_PresetDuration = 0.0f;
    m_NextPresetDuration = args.duration;
//    return true;
}




float CPlugin::GetImmRelTotal()
{
    return (m_audio->GetImmRel(BAND_BASS) + m_audio->GetImmRel(BAND_MID) + m_audio->GetImmRel(BAND_TREBLE)) / 3.0f;
}

float CPlugin::GetImmRel(AudioBand band)
{
    return m_audio->GetImmRel(band);
}


float CPlugin::GetAvgRelTotal()
{
    return (m_audio->GetAvgRel(BAND_BASS) + m_audio->GetAvgRel(BAND_MID) + m_audio->GetAvgRel(BAND_TREBLE)) / 3.0f;
}

float CPlugin::GetAvgRel(AudioBand band)
{
    return m_audio->GetAvgRel(band);
}


static void Append(std::string &text, const char *format, ...)
{
	va_list arglist;
	va_start(arglist, format);

	text += StringFormatV(format, arglist);
	text.push_back('\n');

	va_end(arglist);
}

void CPlugin::GenWarpPShaderText(std::string &szShaderText, float decay, bool bWrap)
{
	std::string p;
	Append(p, "{");
	Append(p, "    // sample previous frame");
	Append(p, "    ret = tex2D( sampler%s_main, uv ).xyz;", bWrap ? "" : "_fc");
	Append(p, "    // darken (decay) over time");
	Append(p, "    ret *= %.2f; //or try: ret -= 0.004;", decay);
	// Append(p, "    ");
	// Append(p, "    ret.w = vDiffuse.w; // pass alpha along - req'd for preset blending");
	Append(p, "}");

	// find the pixel shader body and replace it with custom code.
	szShaderText = m_szDefaultWarpPShaderText;
	size_t brace = szShaderText.rfind('{');
	if (brace != std::string::npos)
	{
		szShaderText = szShaderText.substr(0, brace) + p;
	}
}

void CPlugin::GenCompPShaderText(std::string &szShaderText, float brightness, float ve_alpha, float ve_zoom, int ve_orient, float hue_shader, bool bBrighten, bool bDarken, bool bSolarize, bool bInvert)
{

	std::string p;
	Append(p, "{");
	if (ve_alpha > 0.001f) 
	{
		int orient_x = (ve_orient %  2) ? -1 : 1;
		int orient_y = (ve_orient >= 2) ? -1 : 1;
		Append(p, "    float2 uv_echo = (uv - 0.5)*%.3f*float2(%d,%d) + 0.5;", 1.0f/ve_zoom, orient_x, orient_y);
		Append(p, "    ret = lerp( tex2D(sampler_main, uv).xyz, ");
		Append(p, "                tex2D(sampler_main, uv_echo).xyz, ");
		Append(p, "                %.2f ", ve_alpha);
		Append(p, "              ); //video echo");
		Append(p, "    ret *= %.2f; //gamma", brightness);
	}
	else 
	{
		Append(p, "    ret = tex2D(sampler_main, uv).xyz;");
		Append(p, "    ret *= %.2f; //gamma", brightness);
	}
	if (hue_shader >= 1.0f)
		Append(p, "    ret *= hue_shader; //old hue shader effect");
	else if (hue_shader > 0.001f)
		Append(p, "    ret *= %.2f + %.2f*hue_shader; //old hue shader effect", 1-hue_shader, hue_shader);

	if (bBrighten)
		Append(p, "    ret = sqrt(ret); //brighten");
	if (bDarken)
		Append(p, "    ret *= ret; //darken");
	if (bSolarize)
		Append(p, "    ret = ret*(1-ret)*4; //solarize");
	if (bInvert)
		Append(p, "    ret = 1 - ret; //invert");
	//Append(p, "    ret.w = vDiffuse.w; // pass alpha along - req'd for preset blending");
	Append(p, "}");


	// find the pixel shader body and replace it with custom code.
	szShaderText = m_szDefaultCompPShaderText;
	size_t brace = szShaderText.rfind('{');
	if (brace != std::string::npos)
	{
		szShaderText = szShaderText.substr(0, brace) + p;
	}
}
