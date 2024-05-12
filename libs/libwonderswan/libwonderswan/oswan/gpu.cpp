////////////////////////////////////////////////////////////////////////////////
// GPU
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
// 7.04.2002: Fixed sprites order
// 7.06.2006: Fixed foreground window
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////

//#define STATISTICS

//#include <windows.h>
#include "wsr_types.h" //YOYOFR

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <malloc.h>
#include <io.h>
#include <fcntl.h>
//#include <conio.h>
#include <time.h>
#include "log.h"
#include "ws.h"
#include "rom.h"
#include "./nec/nec.h"
#include "io.h"
//#include "draw.h"
#include "gpu.h"
#include "audio.h"

#ifdef STATISTICS	
	#include "ticker.h"
#endif

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
#ifdef STATISTICS	
	long	ws_4_shades_tiles_cache_update_time=0;
	long	ws_4_colors_tiles_cache_update_time=0;
	long	ws_16_colors_packed_tiles_cache_update_time=0;
	long	ws_16_colors_layered_tiles_cache_update_time=0;

	long	ws_4_shades_tiles_cache_update_number=0;
	long	ws_4_colors_tiles_cache_update_number=0;
	long	ws_16_colors_packed_tiles_cache_update_number=0;
	long	ws_16_colors_layered_tiles_cache_update_number=0;

	long	ws_background_color_rendering_time=0;
	long	ws_background_rendering_time=0;
	long	ws_foreground_rendering_time=0;
	long	ws_sprites_rendering_time=0;
#endif
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////

BYTE	ws_videoMode;
WORD	ws_palette[16*4];
BYTE	ws_paletteColors[8];
DWORD	wsc_palette[16*16];
DWORD	ws_shades[16];
int		ws_gpu_scanline=0;
int		ws_gpu_forceMonoSystemBool=0;

int		layer1_on=1;
int		layer2_on=1;
int		sprite_on=1;

BYTE	*ws_tile_cache;
BYTE	*ws_hflipped_tile_cache;

BYTE	*wsc_tile_cache;
BYTE	*wsc_hflipped_tile_cache;

BYTE	*ws_modified_tile;
BYTE	*wsc_modified_tile;

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
//void ws_gpu_set_colour_scheme(int scheme)
//{
//// white
//#define SHADE_COLOR_RED		1.00
//#define SHADE_COLOR_GREEN	1.00
//#define SHADE_COLOR_BLUE	1.00
//
//	DWORD	ws_colour_scheme_default[16]={
//						RGB555(SHADE_COLOR_RED*15,SHADE_COLOR_GREEN*15,SHADE_COLOR_BLUE*15),
//						RGB555(SHADE_COLOR_RED*14,SHADE_COLOR_GREEN*14,SHADE_COLOR_BLUE*14),
//						RGB555(SHADE_COLOR_RED*13,SHADE_COLOR_GREEN*13,SHADE_COLOR_BLUE*13),
//						RGB555(SHADE_COLOR_RED*12,SHADE_COLOR_GREEN*12,SHADE_COLOR_BLUE*12),
//						RGB555(SHADE_COLOR_RED*11,SHADE_COLOR_GREEN*11,SHADE_COLOR_BLUE*11),
//						RGB555(SHADE_COLOR_RED*10,SHADE_COLOR_GREEN*10,SHADE_COLOR_BLUE*10),
//						RGB555(SHADE_COLOR_RED*9,SHADE_COLOR_GREEN*9,SHADE_COLOR_BLUE*9),
//						RGB555(SHADE_COLOR_RED*8,SHADE_COLOR_GREEN*8,SHADE_COLOR_BLUE*8),
//						RGB555(SHADE_COLOR_RED*7,SHADE_COLOR_GREEN*7,SHADE_COLOR_BLUE*7),
//						RGB555(SHADE_COLOR_RED*6,SHADE_COLOR_GREEN*6,SHADE_COLOR_BLUE*6),
//						RGB555(SHADE_COLOR_RED*5,SHADE_COLOR_GREEN*5,SHADE_COLOR_BLUE*5),
//						RGB555(SHADE_COLOR_RED*4,SHADE_COLOR_GREEN*4,SHADE_COLOR_BLUE*4),
//						RGB555(SHADE_COLOR_RED*3,SHADE_COLOR_GREEN*3,SHADE_COLOR_BLUE*3),
//						RGB555(SHADE_COLOR_RED*2,SHADE_COLOR_GREEN*2,SHADE_COLOR_BLUE*2),
//						RGB555(SHADE_COLOR_RED*1,SHADE_COLOR_GREEN*1,SHADE_COLOR_BLUE*1),
//						RGB555(SHADE_COLOR_RED*0,SHADE_COLOR_GREEN*0,SHADE_COLOR_BLUE*0)
//					};
//// green
//#undef	SHADE_COLOR_RED
//#undef	SHADE_COLOR_GREEN
//#undef	SHADE_COLOR_BLUE
//#define SHADE_COLOR_RED		0.30
//#define SHADE_COLOR_GREEN	1.00
//#define SHADE_COLOR_BLUE	0.30
//
//	DWORD	ws_colour_scheme_green[16]={
//						RGB555(SHADE_COLOR_RED*15,SHADE_COLOR_GREEN*15,SHADE_COLOR_BLUE*15),
//						RGB555(SHADE_COLOR_RED*14,SHADE_COLOR_GREEN*14,SHADE_COLOR_BLUE*14),
//						RGB555(SHADE_COLOR_RED*13,SHADE_COLOR_GREEN*13,SHADE_COLOR_BLUE*13),
//						RGB555(SHADE_COLOR_RED*12,SHADE_COLOR_GREEN*12,SHADE_COLOR_BLUE*12),
//						RGB555(SHADE_COLOR_RED*11,SHADE_COLOR_GREEN*11,SHADE_COLOR_BLUE*11),
//						RGB555(SHADE_COLOR_RED*10,SHADE_COLOR_GREEN*10,SHADE_COLOR_BLUE*10),
//						RGB555(SHADE_COLOR_RED*9,SHADE_COLOR_GREEN*9,SHADE_COLOR_BLUE*9),
//						RGB555(SHADE_COLOR_RED*8,SHADE_COLOR_GREEN*8,SHADE_COLOR_BLUE*8),
//						RGB555(SHADE_COLOR_RED*7,SHADE_COLOR_GREEN*7,SHADE_COLOR_BLUE*7),
//						RGB555(SHADE_COLOR_RED*6,SHADE_COLOR_GREEN*6,SHADE_COLOR_BLUE*6),
//						RGB555(SHADE_COLOR_RED*5,SHADE_COLOR_GREEN*5,SHADE_COLOR_BLUE*5),
//						RGB555(SHADE_COLOR_RED*4,SHADE_COLOR_GREEN*4,SHADE_COLOR_BLUE*4),
//						RGB555(SHADE_COLOR_RED*3,SHADE_COLOR_GREEN*3,SHADE_COLOR_BLUE*3),
//						RGB555(SHADE_COLOR_RED*2,SHADE_COLOR_GREEN*2,SHADE_COLOR_BLUE*2),
//						RGB555(SHADE_COLOR_RED*1,SHADE_COLOR_GREEN*1,SHADE_COLOR_BLUE*1),
//						RGB555(SHADE_COLOR_RED*0,SHADE_COLOR_GREEN*0,SHADE_COLOR_BLUE*0)
//					};
//// amber
//#undef	SHADE_COLOR_RED
//#undef	SHADE_COLOR_GREEN
//#undef	SHADE_COLOR_BLUE
//#define SHADE_COLOR_RED		1.00
//#define SHADE_COLOR_GREEN	0.71
//#define SHADE_COLOR_BLUE	0.10
//
//	DWORD	ws_colour_scheme_amber[16]={
//						RGB555(SHADE_COLOR_RED*15,SHADE_COLOR_GREEN*15,SHADE_COLOR_BLUE*15),
//						RGB555(SHADE_COLOR_RED*14,SHADE_COLOR_GREEN*14,SHADE_COLOR_BLUE*14),
//						RGB555(SHADE_COLOR_RED*13,SHADE_COLOR_GREEN*13,SHADE_COLOR_BLUE*13),
//						RGB555(SHADE_COLOR_RED*12,SHADE_COLOR_GREEN*12,SHADE_COLOR_BLUE*12),
//						RGB555(SHADE_COLOR_RED*11,SHADE_COLOR_GREEN*11,SHADE_COLOR_BLUE*11),
//						RGB555(SHADE_COLOR_RED*10,SHADE_COLOR_GREEN*10,SHADE_COLOR_BLUE*10),
//						RGB555(SHADE_COLOR_RED*9,SHADE_COLOR_GREEN*9,SHADE_COLOR_BLUE*9),
//						RGB555(SHADE_COLOR_RED*8,SHADE_COLOR_GREEN*8,SHADE_COLOR_BLUE*8),
//						RGB555(SHADE_COLOR_RED*7,SHADE_COLOR_GREEN*7,SHADE_COLOR_BLUE*7),
//						RGB555(SHADE_COLOR_RED*6,SHADE_COLOR_GREEN*6,SHADE_COLOR_BLUE*6),
//						RGB555(SHADE_COLOR_RED*5,SHADE_COLOR_GREEN*5,SHADE_COLOR_BLUE*5),
//						RGB555(SHADE_COLOR_RED*4,SHADE_COLOR_GREEN*4,SHADE_COLOR_BLUE*4),
//						RGB555(SHADE_COLOR_RED*3,SHADE_COLOR_GREEN*3,SHADE_COLOR_BLUE*3),
//						RGB555(SHADE_COLOR_RED*2,SHADE_COLOR_GREEN*2,SHADE_COLOR_BLUE*2),
//						RGB555(SHADE_COLOR_RED*1,SHADE_COLOR_GREEN*1,SHADE_COLOR_BLUE*1),
//						RGB555(SHADE_COLOR_RED*0,SHADE_COLOR_GREEN*0,SHADE_COLOR_BLUE*0)
//					};
//
//	switch (scheme)
//	{
//	case COLOUR_SCHEME_DEFAULT: memcpy(ws_shades,ws_colour_scheme_default,16*sizeof(DWORD)); break;
//	case COLOUR_SCHEME_AMBER  : memcpy(ws_shades,ws_colour_scheme_amber,16*sizeof(DWORD)); break;
//	case COLOUR_SCHEME_GREEN  : memcpy(ws_shades,ws_colour_scheme_green,16*sizeof(DWORD)); break;
//	}
//}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void ws_gpu_forceColorSystem(void)
{
	ws_gpu_forceMonoSystemBool=0;
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void ws_gpu_forceMonoSystem(void)
{
	ws_gpu_forceMonoSystemBool=1;
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void ws_gpu_init(void)
{
	ws_tile_cache				= (BYTE*)malloc(512*8*8);
	wsc_tile_cache				= (BYTE*)malloc(1024*8*8);

	ws_hflipped_tile_cache		= (BYTE*)malloc(512*8*8);
	wsc_hflipped_tile_cache		= (BYTE*)malloc(1024*8*8);

	ws_modified_tile			= (BYTE*)malloc(1024);
	wsc_modified_tile			= (BYTE*)malloc(1024);

	memset(ws_tile_cache,0x00,512*8*8);
	memset(wsc_tile_cache,0x00,1024*8*8);

	memset(ws_hflipped_tile_cache,0x00,512*8*8);
	memset(wsc_hflipped_tile_cache,0x00,1024*8*8);

	memset(ws_modified_tile,0x01,1024);
	memset(wsc_modified_tile,0x01,1024);

	ws_gpu_forceMonoSystemBool=0;
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void ws_gpu_done(void)
{
  if (ws_tile_cache) {
    free(ws_tile_cache);
    ws_tile_cache = NULL;
  }

  if (wsc_tile_cache) {
    free(wsc_tile_cache);
    wsc_tile_cache = NULL;
  }

  if (ws_hflipped_tile_cache) {
    free(ws_hflipped_tile_cache);
    ws_hflipped_tile_cache = NULL;
  }

  if (wsc_hflipped_tile_cache) {
    free(wsc_hflipped_tile_cache);
    wsc_hflipped_tile_cache = NULL;
  }

  if (ws_modified_tile) {
    free(ws_modified_tile);
    ws_modified_tile = NULL;
  }

  if (wsc_modified_tile) {
    free(wsc_modified_tile);
    wsc_modified_tile = NULL;
  }
#ifdef STATISTICS	
	printf("Statistics:\n");
	printf("\tcache:\n");
	if (ws_4_shades_tiles_cache_update_number)
		printf("\t\t4 shades tiles update time         : %i\n",ws_4_shades_tiles_cache_update_time/ws_4_shades_tiles_cache_update_number);
	if (ws_4_colors_tiles_cache_update_number)
		printf("\t\t4 colors tiles update time         : %i\n",ws_4_colors_tiles_cache_update_time/ws_4_colors_tiles_cache_update_number);
	if (ws_16_colors_packed_tiles_cache_update_number)
		printf("\t\t16 colors packed tiles update time : %i\n",ws_16_colors_packed_tiles_cache_update_time/ws_16_colors_packed_tiles_cache_update_number);
	if (ws_16_colors_layered_tiles_cache_update_number)
		printf("\t\t16 colors layered tiles update time: %i\n",ws_16_colors_layered_tiles_cache_update_time/ws_16_colors_layered_tiles_cache_update_number);

	printf("\tscanline rendering:\n");
	long	total=	ws_background_color_rendering_time+ws_background_rendering_time+
					ws_foreground_rendering_time+
					ws_sprites_rendering_time;
	printf("\t\tbackground color   : %4i (%3i %%)\n",ws_background_color_rendering_time,(ws_background_color_rendering_time*100)/total);
	printf("\t\tbackground         : %4i (%3i %%)\n",ws_background_rendering_time,(ws_background_rendering_time*100)/total);
	printf("\t\tforeground         : %4i (%3i %%)\n",ws_foreground_rendering_time,(ws_foreground_rendering_time*100)/total);
	printf("\t\tsprites            : %4i (%3i %%)\n",ws_sprites_rendering_time,(ws_sprites_rendering_time*100)/total);
#endif
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void ws_gpu_changeVideoMode(BYTE value)
{
	// LAST ALIVE, Terrors2 patch
//	ws_audio_set_channel_volume(3,0);

	if(ws_gpu_forceMonoSystemBool)
		value=0;
	if (ws_videoMode!=(value>>5))
	{
		ws_videoMode=value>>5;
		
		// mark all tiles dirty
		memset(wsc_modified_tile,0x01,1024);
		memset(ws_modified_tile,0x01,1024);
	}
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void ws_gpu_reset(void)
{
	memset(wsc_modified_tile,0x01,1024);
	memset(ws_modified_tile,0x01,1024);
	ws_gpu_scanline=0;
	ws_gpu_changeVideoMode(4);
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void ws_gpu_clearCache(void)
{
	memset(wsc_modified_tile,0x01,1024);
	memset(ws_modified_tile,0x01,1024);
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
BYTE *ws_tileCache_getTileRow(DWORD tileIndex, DWORD line, 
							   DWORD vFlip, DWORD hFlip, DWORD bank)
{
	if (ws_videoMode)
	{
		if (bank)
			tileIndex+=512;

		// need to update tile cache ?
		// 4 colors tiles
		if ((ws_videoMode==4)&&(ws_modified_tile[tileIndex]))
		{
#ifdef STATISTICS	
			ws_4_colors_tiles_cache_update_time+=-ticker();
#endif
			BYTE	*tileInCachePtr = &wsc_tile_cache[tileIndex<<6];
			BYTE	*hflippedTileInCachePtr = &wsc_hflipped_tile_cache[tileIndex<<6];
			WORD	*tileInRamPtr	= (WORD*)&internalRam[0x2000+(tileIndex<<4)];
			WORD	tileLine;

			for (int line=0;line<8;line++)
			{
				tileLine=*tileInRamPtr++;

				tileInCachePtr[0]=((tileLine&0x80)>>7)|((tileLine&0x8000)>>14);
				hflippedTileInCachePtr[7]=tileInCachePtr[0];
				tileInCachePtr[1]=((tileLine&0x40)>>6)|((tileLine&0x4000)>>13);
				hflippedTileInCachePtr[6]=tileInCachePtr[1];
				tileInCachePtr[2]=((tileLine&0x20)>>5)|((tileLine&0x2000)>>12);
				hflippedTileInCachePtr[5]=tileInCachePtr[2];
				tileInCachePtr[3]=((tileLine&0x10)>>4)|((tileLine&0x1000)>>11);
				hflippedTileInCachePtr[4]=tileInCachePtr[3];
				tileInCachePtr[4]=((tileLine&0x08)>>3)|((tileLine&0x0800)>>10);
				hflippedTileInCachePtr[3]=tileInCachePtr[4];
				tileInCachePtr[5]=((tileLine&0x04)>>2)|((tileLine&0x0400)>>9);
				hflippedTileInCachePtr[2]=tileInCachePtr[5];
				tileInCachePtr[6]=((tileLine&0x02)>>1)|((tileLine&0x0200)>>8);
				hflippedTileInCachePtr[1]=tileInCachePtr[6];
				tileInCachePtr[7]=((tileLine&0x01)>>0)|((tileLine&0x0100)>>7);
				hflippedTileInCachePtr[0]=tileInCachePtr[7];

				tileInCachePtr+=8;
				hflippedTileInCachePtr+=8;
			}				
			ws_modified_tile[tileIndex]=0;
#ifdef STATISTICS	
			ws_4_colors_tiles_cache_update_time+=ticker();
			ws_4_colors_tiles_cache_update_number++;
#endif
		}
		else
		if (wsc_modified_tile[tileIndex])
		{
			// 16 colors by tile layered mode
			if (ws_videoMode==6)
			{
#ifdef STATISTICS	
				ws_16_colors_layered_tiles_cache_update_time+=-ticker();
#endif
				BYTE	*tileInCachePtr			= &wsc_tile_cache[tileIndex<<6];
				BYTE	*hflippedTileInCachePtr = &wsc_hflipped_tile_cache[tileIndex<<6];
				DWORD	*tileInRamPtr			= (DWORD*)&internalRam[0x4000+(tileIndex<<5)];
				DWORD	tileLine;

				for (int line=0;line<8;line++)
				{
					tileLine=*tileInRamPtr++;
					
					tileInCachePtr[0]=(BYTE)(((tileLine&0x00000080)>>7)|((tileLine&0x00008000)>>14)|
									  ((tileLine&0x00800000)>>21)|((tileLine&0x80000000)>>28));
					hflippedTileInCachePtr[7]=tileInCachePtr[0];

					tileInCachePtr[1]=(BYTE)(((tileLine&0x00000040)>>6)|((tileLine&0x00004000)>>13)|
									  ((tileLine&0x00400000)>>20)|((tileLine&0x40000000)>>27));
					hflippedTileInCachePtr[6]=tileInCachePtr[1];
					
					tileInCachePtr[2]=(BYTE)(((tileLine&0x00000020)>>5)|((tileLine&0x00002000)>>12)|
									  ((tileLine&0x00200000)>>19)|((tileLine&0x20000000)>>26));
					hflippedTileInCachePtr[5]=tileInCachePtr[2];
					
					tileInCachePtr[3]=(BYTE)(((tileLine&0x00000010)>>4)|((tileLine&0x00001000)>>11)|
									  ((tileLine&0x00100000)>>18)|((tileLine&0x10000000)>>25));
					hflippedTileInCachePtr[4]=tileInCachePtr[3];
					
					tileInCachePtr[4]=(BYTE)(((tileLine&0x00000008)>>3)|((tileLine&0x00000800)>>10)|
									  ((tileLine&0x00080000)>>17)|((tileLine&0x08000000)>>24));
					hflippedTileInCachePtr[3]=tileInCachePtr[4];
					
					tileInCachePtr[5]=(BYTE)(((tileLine&0x00000004)>>2)|((tileLine&0x00000400)>>9)|
									  ((tileLine&0x00040000)>>16)|((tileLine&0x04000000)>>23));
					hflippedTileInCachePtr[2]=tileInCachePtr[5];
					
					tileInCachePtr[6]=(BYTE)(((tileLine&0x00000002)>>1)|((tileLine&0x00000200)>>8)|
									  ((tileLine&0x00020000)>>15)|((tileLine&0x02000000)>>22));
					hflippedTileInCachePtr[1]=tileInCachePtr[6];
					
					tileInCachePtr[7]=(BYTE)(((tileLine&0x00000001)>>0)|((tileLine&0x00000100)>>7)|
									  ((tileLine&0x00010000)>>14)|((tileLine&0x01000000)>>21));
					hflippedTileInCachePtr[0]=tileInCachePtr[7];

					tileInCachePtr+=8;
					hflippedTileInCachePtr+=8;
				}			
#ifdef STATISTICS	
				ws_16_colors_layered_tiles_cache_update_time+=ticker();
				ws_16_colors_layered_tiles_cache_update_number++;
#endif
			}
			else
			// 16 colors by tile packed mode
			if (ws_videoMode==7)
			{
#ifdef STATISTICS	
				ws_16_colors_packed_tiles_cache_update_time+=-ticker();
#endif
				BYTE	*tileInCachePtr = &wsc_tile_cache[tileIndex<<6];
				BYTE	*hflippedTileInCachePtr = &wsc_hflipped_tile_cache[tileIndex<<6];
				DWORD	*tileInRamPtr	= (DWORD*)&internalRam[0x4000+(tileIndex<<5)];
				DWORD	tileLine;

				for (int line=0;line<8;line++)
				{
					tileLine=*tileInRamPtr++;
					
					tileInCachePtr[0]=(BYTE)(tileLine>>4)&0x0f;
					hflippedTileInCachePtr[7]=tileInCachePtr[0];
					tileInCachePtr[1]=(BYTE)(tileLine>>0)&0x0f;
					hflippedTileInCachePtr[6]=tileInCachePtr[1];
					tileInCachePtr[2]=(BYTE)(tileLine>>12)&0x0f;
					hflippedTileInCachePtr[5]=tileInCachePtr[2];
					tileInCachePtr[3]=(BYTE)(tileLine>>8)&0x0f;
					hflippedTileInCachePtr[4]=tileInCachePtr[3];
					tileInCachePtr[4]=(BYTE)(tileLine>>20)&0x0f;
					hflippedTileInCachePtr[3]=tileInCachePtr[4];
					tileInCachePtr[5]=(BYTE)(tileLine>>16)&0x0f;
					hflippedTileInCachePtr[2]=tileInCachePtr[5];
					tileInCachePtr[6]=(BYTE)(tileLine>>28)&0x0f;
					hflippedTileInCachePtr[1]=tileInCachePtr[6];
					tileInCachePtr[7]=(BYTE)(tileLine>>24)&0x0f;
					hflippedTileInCachePtr[0]=tileInCachePtr[7];

					tileInCachePtr+=8;
					hflippedTileInCachePtr+=8;
					
				}				
#ifdef STATISTICS	
				ws_16_colors_packed_tiles_cache_update_time+=ticker();
				ws_16_colors_packed_tiles_cache_update_number++;
#endif
			}
			else
			{
				// unknown mode 
			}
			// tile cache updated
			wsc_modified_tile[tileIndex]=0;
		}	
		if (vFlip)
			line=7-line;
		if (hFlip)
			return(&wsc_hflipped_tile_cache[(tileIndex<<6)+(line<<3)]);
		else
			return(&wsc_tile_cache[(tileIndex<<6)+(line<<3)]);
	
	}
	else
	{
		// need to update tile cache ?
		if (ws_modified_tile[tileIndex])
		{
#ifdef STATISTICS	
			ws_4_shades_tiles_cache_update_time+=-ticker();
#endif
			BYTE	*tileInCachePtr			 = &ws_tile_cache[tileIndex<<6];
			BYTE	*hflippedTileInCachePtr	 = &ws_hflipped_tile_cache[(tileIndex<<6)+7];
			DWORD	*tileInRamPtr			 = (DWORD*)&internalRam[0x2000+(tileIndex<<4)];
			DWORD	tileLine;

			for (int line=0;line<4;line++)
			{
				tileLine=*tileInRamPtr++;
								
				*hflippedTileInCachePtr--=*tileInCachePtr++=(BYTE)(((tileLine&0x80)>>7)|((tileLine&0x8000)>>14));
				*hflippedTileInCachePtr--=*tileInCachePtr++=(BYTE)(((tileLine&0x40)>>6)|((tileLine&0x4000)>>13));
				*hflippedTileInCachePtr--=*tileInCachePtr++=(BYTE)(((tileLine&0x20)>>5)|((tileLine&0x2000)>>12));
				*hflippedTileInCachePtr--=*tileInCachePtr++=(BYTE)(((tileLine&0x10)>>4)|((tileLine&0x1000)>>11));
				*hflippedTileInCachePtr--=*tileInCachePtr++=(BYTE)(((tileLine&0x08)>>3)|((tileLine&0x0800)>>10));
				*hflippedTileInCachePtr--=*tileInCachePtr++=(BYTE)(((tileLine&0x04)>>2)|((tileLine&0x0400)>>9));
				*hflippedTileInCachePtr--=*tileInCachePtr++=(BYTE)(((tileLine&0x02)>>1)|((tileLine&0x0200)>>8));
				*hflippedTileInCachePtr--=*tileInCachePtr++=(BYTE)(((tileLine&0x01)>>0)|((tileLine&0x0100)>>7));
				hflippedTileInCachePtr+=16;
				tileLine>>=16;
				*hflippedTileInCachePtr--=*tileInCachePtr++=(BYTE)(((tileLine&0x80)>>7)|((tileLine&0x8000)>>14));
				*hflippedTileInCachePtr--=*tileInCachePtr++=(BYTE)(((tileLine&0x40)>>6)|((tileLine&0x4000)>>13));
				*hflippedTileInCachePtr--=*tileInCachePtr++=(BYTE)(((tileLine&0x20)>>5)|((tileLine&0x2000)>>12));
				*hflippedTileInCachePtr--=*tileInCachePtr++=(BYTE)(((tileLine&0x10)>>4)|((tileLine&0x1000)>>11));
				*hflippedTileInCachePtr--=*tileInCachePtr++=(BYTE)(((tileLine&0x08)>>3)|((tileLine&0x0800)>>10));
				*hflippedTileInCachePtr--=*tileInCachePtr++=(BYTE)(((tileLine&0x04)>>2)|((tileLine&0x0400)>>9));
				*hflippedTileInCachePtr--=*tileInCachePtr++=(BYTE)(((tileLine&0x02)>>1)|((tileLine&0x0200)>>8));
				*hflippedTileInCachePtr--=*tileInCachePtr++=(BYTE)(((tileLine&0x01)>>0)|((tileLine&0x0100)>>7));
				hflippedTileInCachePtr+=16;
			}				
			// tile cache updated
			ws_modified_tile[tileIndex]=0;
#ifdef STATISTICS	
			ws_4_shades_tiles_cache_update_time+=ticker();
			ws_4_shades_tiles_cache_update_number++;
#endif
		}
		if (vFlip)
			line=7-line;
		if (hFlip)
			return(&ws_hflipped_tile_cache[(tileIndex<<6)+(line<<3)]);
		else
			return(&ws_tile_cache[(tileIndex<<6)+(line<<3)]);
	}
	return(NULL);
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
char l_mask[224];
char w_mask[224];
void ws_gpu_renderScanline(DWORD *framebuffer)
{

	if (ws_gpu_scanline>143)	
		return;
#ifdef STATISTICS	
	long startTime=ticker();
#endif
	framebuffer += (224 * ws_gpu_scanline);
	for (int i=0;i<224;i++)
		l_mask[i]=0;

	// fill with border color
	DWORD backgroundColor;
	if (ws_videoMode)
		backgroundColor=wsc_palette[ws_ioRam[0x01]];
	else
		backgroundColor = ws_shades[ws_paletteColors[ws_ioRam[0x01]&0x7]];

	for (int i=0;i<224;i++)
		framebuffer[i] = backgroundColor;
#ifdef STATISTICS	
	ws_background_color_rendering_time=ticker();
#endif
	// render background layer
	if (ws_ioRam[0x00]&0x01&&layer1_on)
	{
		int ws_bgScroll_x=ws_ioRam[0x10];
		int ws_bgScroll_y=ws_ioRam[0x11];
		
		// seek to the first tile
		ws_bgScroll_y=(ws_bgScroll_y+ws_gpu_scanline)&0xff;

		// note: byte ordering assumptions!
		int	ws_currentTile=(ws_bgScroll_x>>3);
		WORD	*ws_bgScrollRamBase=(WORD*)(internalRam+(((DWORD)ws_ioRam[0x07]&0x0f)<<11)+
									((ws_bgScroll_y&0xfff8)<<3));
		
		int	lineInTile	 = ws_bgScroll_y&0x07;
		int columnInTile = ws_bgScroll_x&0x07;
	
		DWORD *scanlinePtr = framebuffer;

		if (ws_videoMode)
		{
			// render the first clipped tile
			if (columnInTile)
			{
				WORD	tileInfo=ws_bgScrollRamBase[ws_currentTile&0x1f];
				ws_currentTile++;
				BYTE	*ws_tileRow=ws_tileCache_getTileRow(tileInfo&0x1ff, lineInTile,
												tileInfo&0x8000, tileInfo&0x4000, tileInfo&0x2000);
				DWORD	*wsc_paletteAlias = &wsc_palette[((tileInfo>>9)&0x0f)<<4];
				ws_tileRow+=columnInTile;
				for (int i=columnInTile;i<8;i++)
				{
					if (ws_ioRam[0x60]&0x40||tileInfo&0x0800) //16 colors || 4 colors and transparent
					{
						if (*ws_tileRow)	
							*scanlinePtr=wsc_paletteAlias[*ws_tileRow];
					}
					else
					{
						*scanlinePtr=wsc_paletteAlias[*ws_tileRow]; //4 colors
					}
					scanlinePtr++; 
					ws_tileRow++;
				}
			}

			// render the tiles between them
			int nbTiles=28;
			if (columnInTile)
				nbTiles=27;
			
			for (int i=0;i<nbTiles;i++)
			{
				WORD	tileInfo=ws_bgScrollRamBase[ws_currentTile&0x1f];
				ws_currentTile++;
				BYTE	*ws_tileRow=ws_tileCache_getTileRow(tileInfo&0x1ff, lineInTile,
													tileInfo&0x8000, tileInfo&0x4000, tileInfo&0x2000);
				DWORD	*wsc_paletteAlias = &wsc_palette[((tileInfo>>9)&0x0f)<<4];

				if (ws_ioRam[0x60]&0x40||tileInfo&0x0800)
				{
					if (*ws_tileRow) *scanlinePtr=wsc_paletteAlias[*ws_tileRow]; scanlinePtr++;	ws_tileRow++;
					if (*ws_tileRow) *scanlinePtr=wsc_paletteAlias[*ws_tileRow]; scanlinePtr++;	ws_tileRow++;
					if (*ws_tileRow) *scanlinePtr=wsc_paletteAlias[*ws_tileRow]; scanlinePtr++;	ws_tileRow++;
					if (*ws_tileRow) *scanlinePtr=wsc_paletteAlias[*ws_tileRow]; scanlinePtr++;	ws_tileRow++;
					if (*ws_tileRow) *scanlinePtr=wsc_paletteAlias[*ws_tileRow]; scanlinePtr++;	ws_tileRow++;
					if (*ws_tileRow) *scanlinePtr=wsc_paletteAlias[*ws_tileRow]; scanlinePtr++;	ws_tileRow++;
					if (*ws_tileRow) *scanlinePtr=wsc_paletteAlias[*ws_tileRow]; scanlinePtr++;	ws_tileRow++;
					if (*ws_tileRow) *scanlinePtr=wsc_paletteAlias[*ws_tileRow]; scanlinePtr++;	ws_tileRow++;					}
				else
				{
					*scanlinePtr=wsc_paletteAlias[*ws_tileRow]; scanlinePtr++;	ws_tileRow++;
					*scanlinePtr=wsc_paletteAlias[*ws_tileRow]; scanlinePtr++;	ws_tileRow++;
					*scanlinePtr=wsc_paletteAlias[*ws_tileRow]; scanlinePtr++;	ws_tileRow++;
					*scanlinePtr=wsc_paletteAlias[*ws_tileRow]; scanlinePtr++;	ws_tileRow++;
					*scanlinePtr=wsc_paletteAlias[*ws_tileRow]; scanlinePtr++;	ws_tileRow++;
					*scanlinePtr=wsc_paletteAlias[*ws_tileRow]; scanlinePtr++;	ws_tileRow++;
					*scanlinePtr=wsc_paletteAlias[*ws_tileRow]; scanlinePtr++;	ws_tileRow++;
					*scanlinePtr=wsc_paletteAlias[*ws_tileRow]; scanlinePtr++;	ws_tileRow++;
				}
			}

			// render the last clipped tile
			if (columnInTile)
			{
				WORD	tileInfo=ws_bgScrollRamBase[ws_currentTile&0x1f];
				ws_currentTile++;
				BYTE	*ws_tileRow=ws_tileCache_getTileRow(tileInfo&0x1ff, lineInTile,
													tileInfo&0x8000, tileInfo&0x4000, tileInfo&0x2000);
				DWORD	*wsc_paletteAlias = &wsc_palette[((tileInfo>>9)&0x0f)<<4];
				

				for (int i=0;i<columnInTile;i++)
				{
					if (ws_ioRam[0x60]&0x40||tileInfo&0x0800)
					{
						if (*ws_tileRow)	
							*scanlinePtr=wsc_paletteAlias[*ws_tileRow];
					}
					else
					{
						*scanlinePtr=wsc_paletteAlias[*ws_tileRow];
					}
					scanlinePtr++; 
					ws_tileRow++;
				}
			}
		}
		else
		{
			// render the first clipped tile
			if (columnInTile)
			{
				WORD	tileInfo=ws_bgScrollRamBase[ws_currentTile&0x1f];
				ws_currentTile++;
				BYTE	*ws_tileRow=ws_tileCache_getTileRow(tileInfo&0x1ff, lineInTile,
													tileInfo&0x8000, tileInfo&0x4000, tileInfo&0x2000);
				
				WORD	*ws_paletteAlias=&ws_palette[((tileInfo>>9)&0x0f)<<2];
				ws_tileRow+=columnInTile;

				if (tileInfo&0x0800)
				{
					for (int i=columnInTile;i<8;i++)
					{
						if (*ws_tileRow)
							*scanlinePtr = ws_shades[ws_paletteColors[ws_paletteAlias[*ws_tileRow]]]; 
						scanlinePtr++; 
						ws_tileRow++;
					}
				}
				else
				{
					for (int i=columnInTile;i<8;i++)
					{
						*scanlinePtr = ws_shades[ws_paletteColors[ws_paletteAlias[*ws_tileRow]]]; 
						scanlinePtr++; 
						ws_tileRow++;
					}
				}
			}
			int nbTiles=28;
			if (columnInTile)
				nbTiles=27;
			
			for (int i=0;i<nbTiles;i++)
			{
				WORD	tileInfo=ws_bgScrollRamBase[ws_currentTile&0x1f];
				ws_currentTile++;
				BYTE	*ws_tileRow=ws_tileCache_getTileRow(tileInfo&0x1ff, lineInTile,
													tileInfo&0x8000, tileInfo&0x4000, tileInfo&0x2000);
				WORD	*ws_paletteAlias=&ws_palette[((tileInfo>>9)&0x0f)<<2];
				
				if (tileInfo&0x0800)
				{
					if (*ws_tileRow) *scanlinePtr=ws_shades[ws_paletteColors[ws_paletteAlias[*ws_tileRow]]]; scanlinePtr++; ws_tileRow++;
					if (*ws_tileRow) *scanlinePtr=ws_shades[ws_paletteColors[ws_paletteAlias[*ws_tileRow]]]; scanlinePtr++; ws_tileRow++;
					if (*ws_tileRow) *scanlinePtr=ws_shades[ws_paletteColors[ws_paletteAlias[*ws_tileRow]]]; scanlinePtr++; ws_tileRow++;
					if (*ws_tileRow) *scanlinePtr=ws_shades[ws_paletteColors[ws_paletteAlias[*ws_tileRow]]]; scanlinePtr++; ws_tileRow++;
					if (*ws_tileRow) *scanlinePtr=ws_shades[ws_paletteColors[ws_paletteAlias[*ws_tileRow]]]; scanlinePtr++; ws_tileRow++;
					if (*ws_tileRow) *scanlinePtr=ws_shades[ws_paletteColors[ws_paletteAlias[*ws_tileRow]]]; scanlinePtr++; ws_tileRow++;
					if (*ws_tileRow) *scanlinePtr=ws_shades[ws_paletteColors[ws_paletteAlias[*ws_tileRow]]]; scanlinePtr++; ws_tileRow++;
					if (*ws_tileRow) *scanlinePtr=ws_shades[ws_paletteColors[ws_paletteAlias[*ws_tileRow]]]; scanlinePtr++; ws_tileRow++;
				}
				else
				{
					*scanlinePtr=ws_shades[ws_paletteColors[ws_paletteAlias[*ws_tileRow]]]; scanlinePtr++; ws_tileRow++;
					*scanlinePtr=ws_shades[ws_paletteColors[ws_paletteAlias[*ws_tileRow]]]; scanlinePtr++; ws_tileRow++;
					*scanlinePtr=ws_shades[ws_paletteColors[ws_paletteAlias[*ws_tileRow]]]; scanlinePtr++; ws_tileRow++;
					*scanlinePtr=ws_shades[ws_paletteColors[ws_paletteAlias[*ws_tileRow]]]; scanlinePtr++; ws_tileRow++;
					*scanlinePtr=ws_shades[ws_paletteColors[ws_paletteAlias[*ws_tileRow]]]; scanlinePtr++; ws_tileRow++;
					*scanlinePtr=ws_shades[ws_paletteColors[ws_paletteAlias[*ws_tileRow]]]; scanlinePtr++; ws_tileRow++;
					*scanlinePtr=ws_shades[ws_paletteColors[ws_paletteAlias[*ws_tileRow]]]; scanlinePtr++; ws_tileRow++;
					*scanlinePtr=ws_shades[ws_paletteColors[ws_paletteAlias[*ws_tileRow]]]; scanlinePtr++; ws_tileRow++;
				}

			}

			// render the last clipped tile
			if (columnInTile)
			{
				WORD	tileInfo=ws_bgScrollRamBase[ws_currentTile&0x1f];
				ws_currentTile++;
				BYTE	*ws_tileRow=ws_tileCache_getTileRow(tileInfo&0x1ff, lineInTile,
													tileInfo&0x8000, tileInfo&0x4000, tileInfo&0x2000);
				WORD	*ws_paletteAlias=&ws_palette[((tileInfo>>9)&0x0f)<<2];
				

				if (tileInfo&0x0800)
				{
					for (int i=0;i<columnInTile;i++)
					{
						if (*ws_tileRow)	
							*scanlinePtr=ws_shades[ws_paletteColors[ws_paletteAlias[*ws_tileRow]]]; 
						scanlinePtr++; 
						ws_tileRow++;
					}
				}
				else
				{
					for (int i=0;i<columnInTile;i++)
					{
						*scanlinePtr=ws_shades[ws_paletteColors[ws_paletteAlias[*ws_tileRow]]]; 
						scanlinePtr++; 
						ws_tileRow++;
					}
				}
			}
		}
	}
#ifdef STATISTICS	
	ws_background_rendering_time=ticker();
#endif
	// render foreground layer
	if (ws_ioRam[0x00]&0x02&&layer2_on)
	{
		int ws_fgWindow_x0=ws_ioRam[0x08];
		int ws_fgWindow_y0=ws_ioRam[0x09];
		int ws_fgWindow_x1=ws_ioRam[0x0a];
		int ws_fgWindow_y1=ws_ioRam[0x0b];
		int ws_fgScroll_x=ws_ioRam[0x12];
		int ws_fgScroll_y=ws_ioRam[0x13];
		if(ws_fgWindow_x0<0)
			ws_fgWindow_x0=0;
		if(ws_fgWindow_x1>223)
			ws_fgWindow_x1=223;

		int windowMode=ws_ioRam[0x00]&0x30;
				
		// seek to the first tile
		ws_fgScroll_y=(ws_fgScroll_y+ws_gpu_scanline)&0xff;

		// note: byte ordering assumptions!
		int	ws_currentTile=(ws_fgScroll_x>>3);
		WORD	*ws_fgScrollRamBase=(WORD*)(internalRam+(((DWORD)ws_ioRam[0x07]&0xf0)<<7)+
									((ws_fgScroll_y&0xfff8)<<3));

		int	lineInTile	 = ws_fgScroll_y&0x07;
		int columnInTile = ws_fgScroll_x&0x07;

		DWORD	*scanlinePtr = framebuffer;
		char *layer_mask = l_mask;

		if (windowMode==0x20) //inside
		{
			memset(layer_mask,0,224);
			if((ws_gpu_scanline>=ws_fgWindow_y0)&&(ws_gpu_scanline<=ws_fgWindow_y1))
			{
				for(int i=ws_fgWindow_x0;i<=ws_fgWindow_x1;i++)
				{
					l_mask[i]=1;
				}
			}
		}
		else
		if (windowMode==0x30) //outside
		{
			memset(layer_mask,1,224);
			if((ws_gpu_scanline>=ws_fgWindow_y0)&&(ws_gpu_scanline<=ws_fgWindow_y1))
			{
				for(int i=ws_fgWindow_x0;i<=ws_fgWindow_x1;i++)
				{
					l_mask[i]=0;
				}
			}
		}
		else
			memset(layer_mask,1,224);

		if (ws_videoMode)
		{
			// render the first clipped tile
			if (columnInTile)
			{
				WORD	tileInfo=ws_fgScrollRamBase[ws_currentTile&0x1f];
				ws_currentTile++;
				BYTE	*ws_tileRow=ws_tileCache_getTileRow(tileInfo&0x1ff, lineInTile,
													tileInfo&0x8000, tileInfo&0x4000, tileInfo&0x2000);
				DWORD	*wsc_paletteAlias = &wsc_palette[((tileInfo>>9)&0x0f)<<4];
				
				ws_tileRow+=columnInTile;

				for (int i=columnInTile;i<8;i++)
				{
					if (ws_ioRam[0x60]&0x40||tileInfo&0x0800)
					{
						if (*ws_tileRow&&*layer_mask)
							*scanlinePtr=wsc_paletteAlias[*ws_tileRow];
						else
							*layer_mask=0;
					}
					else
					if(*layer_mask)
						*scanlinePtr=wsc_paletteAlias[*ws_tileRow];
					scanlinePtr++; 
					ws_tileRow++;
					layer_mask++;
				}
			}

			// render the tiles between them
			int nbTiles=28;
			if (columnInTile)
				nbTiles=27;
			
			for (int i=0;i<nbTiles;i++)
			{
				WORD	tileInfo=ws_fgScrollRamBase[ws_currentTile&0x1f];
				ws_currentTile++;
				BYTE	*ws_tileRow=ws_tileCache_getTileRow(tileInfo&0x1ff, lineInTile,
													tileInfo&0x8000, tileInfo&0x4000, tileInfo&0x2000);
				DWORD	*wsc_paletteAlias = &wsc_palette[((tileInfo>>9)&0x0f)<<4];
				
				if (ws_ioRam[0x60]&0x40||tileInfo&0x0800)
				{
					if (*ws_tileRow&&*layer_mask) *scanlinePtr=wsc_paletteAlias[*ws_tileRow]; else *layer_mask=0; scanlinePtr++; ws_tileRow++; layer_mask++;
					if (*ws_tileRow&&*layer_mask) *scanlinePtr=wsc_paletteAlias[*ws_tileRow]; else *layer_mask=0; scanlinePtr++; ws_tileRow++; layer_mask++;
					if (*ws_tileRow&&*layer_mask) *scanlinePtr=wsc_paletteAlias[*ws_tileRow]; else *layer_mask=0; scanlinePtr++; ws_tileRow++; layer_mask++;
					if (*ws_tileRow&&*layer_mask) *scanlinePtr=wsc_paletteAlias[*ws_tileRow]; else *layer_mask=0; scanlinePtr++; ws_tileRow++; layer_mask++;
					if (*ws_tileRow&&*layer_mask) *scanlinePtr=wsc_paletteAlias[*ws_tileRow]; else *layer_mask=0; scanlinePtr++; ws_tileRow++; layer_mask++;
					if (*ws_tileRow&&*layer_mask) *scanlinePtr=wsc_paletteAlias[*ws_tileRow]; else *layer_mask=0; scanlinePtr++; ws_tileRow++; layer_mask++;
					if (*ws_tileRow&&*layer_mask) *scanlinePtr=wsc_paletteAlias[*ws_tileRow]; else *layer_mask=0; scanlinePtr++; ws_tileRow++; layer_mask++;
					if (*ws_tileRow&&*layer_mask) *scanlinePtr=wsc_paletteAlias[*ws_tileRow]; else *layer_mask=0; scanlinePtr++; ws_tileRow++; layer_mask++;
				}
				else
				{
					if(*layer_mask) *scanlinePtr=wsc_paletteAlias[*ws_tileRow]; scanlinePtr++; ws_tileRow++; layer_mask++;
					if(*layer_mask) *scanlinePtr=wsc_paletteAlias[*ws_tileRow]; scanlinePtr++; ws_tileRow++; layer_mask++;
					if(*layer_mask) *scanlinePtr=wsc_paletteAlias[*ws_tileRow]; scanlinePtr++; ws_tileRow++; layer_mask++;
					if(*layer_mask) *scanlinePtr=wsc_paletteAlias[*ws_tileRow]; scanlinePtr++; ws_tileRow++; layer_mask++;
					if(*layer_mask) *scanlinePtr=wsc_paletteAlias[*ws_tileRow]; scanlinePtr++; ws_tileRow++; layer_mask++;
					if(*layer_mask) *scanlinePtr=wsc_paletteAlias[*ws_tileRow]; scanlinePtr++; ws_tileRow++; layer_mask++;
					if(*layer_mask) *scanlinePtr=wsc_paletteAlias[*ws_tileRow]; scanlinePtr++; ws_tileRow++; layer_mask++;
					if(*layer_mask) *scanlinePtr=wsc_paletteAlias[*ws_tileRow]; scanlinePtr++; ws_tileRow++; layer_mask++;
				}
			}

			// render the last clipped tile
			if (columnInTile)
			{
				WORD	tileInfo=ws_fgScrollRamBase[ws_currentTile&0x1f];
				ws_currentTile++;
				BYTE	*ws_tileRow=ws_tileCache_getTileRow(tileInfo&0x1ff, lineInTile,
													tileInfo&0x8000, tileInfo&0x4000, tileInfo&0x2000);
				DWORD	*wsc_paletteAlias = &wsc_palette[((tileInfo>>9)&0x0f)<<4];
				

				for (int i=0;i<columnInTile;i++)
				{
					if (ws_ioRam[0x60]&0x40||tileInfo&0x0800)
					{
						if (*ws_tileRow&&*layer_mask)
							*scanlinePtr=wsc_paletteAlias[*ws_tileRow];
						else
							*layer_mask=0;
					}
					else
					if(*layer_mask)
						*scanlinePtr=wsc_paletteAlias[*ws_tileRow];
					scanlinePtr++; 
					ws_tileRow++;
					layer_mask++;
				}
			}
		}
		else
		{
			// render the first clipped tile
			if (columnInTile)
			{
				WORD	tileInfo=ws_fgScrollRamBase[ws_currentTile&0x1f];
				ws_currentTile++;
				BYTE	*ws_tileRow=ws_tileCache_getTileRow(tileInfo&0x1ff, lineInTile,
													tileInfo&0x8000, tileInfo&0x4000, tileInfo&0x2000);
				
				WORD	*ws_paletteAlias=&ws_palette[((tileInfo>>9)&0x0f)<<2];
				ws_tileRow+=columnInTile;
				if (tileInfo&0x0800)
				{
					for (int i=columnInTile;i<8;i++)
					{
						if (*ws_tileRow&&*layer_mask)
							*scanlinePtr=ws_shades[ws_paletteColors[ws_paletteAlias[*ws_tileRow]]];
						else
							*layer_mask=0;
						scanlinePtr++; 
						ws_tileRow++;
						layer_mask++;
					}
				}
				else
				{
					for (int i=columnInTile;i<8;i++)
					{
						if(*layer_mask)
							*scanlinePtr=ws_shades[ws_paletteColors[ws_paletteAlias[*ws_tileRow]]]; 
						scanlinePtr++; 
						ws_tileRow++;
						layer_mask++;
					}
				}
			}
			int nbTiles=28;
			if (columnInTile)
				nbTiles=27;
			
			for (int i=0;i<nbTiles;i++)
			{
				WORD	tileInfo=ws_fgScrollRamBase[ws_currentTile&0x1f];
				ws_currentTile++;
				BYTE	*ws_tileRow=ws_tileCache_getTileRow(tileInfo&0x1ff, lineInTile,
													tileInfo&0x8000, tileInfo&0x4000, tileInfo&0x2000);
				WORD	*ws_paletteAlias=&ws_palette[((tileInfo>>9)&0x0f)<<2];
				
				if (tileInfo&0x0800)
				{
					if (*ws_tileRow&&*layer_mask) *scanlinePtr=ws_shades[ws_paletteColors[ws_paletteAlias[*ws_tileRow]]]; else *layer_mask=0; scanlinePtr++; ws_tileRow++; layer_mask++;
					if (*ws_tileRow&&*layer_mask) *scanlinePtr=ws_shades[ws_paletteColors[ws_paletteAlias[*ws_tileRow]]]; else *layer_mask=0; scanlinePtr++; ws_tileRow++; layer_mask++;
					if (*ws_tileRow&&*layer_mask) *scanlinePtr=ws_shades[ws_paletteColors[ws_paletteAlias[*ws_tileRow]]]; else *layer_mask=0; scanlinePtr++; ws_tileRow++; layer_mask++;
					if (*ws_tileRow&&*layer_mask) *scanlinePtr=ws_shades[ws_paletteColors[ws_paletteAlias[*ws_tileRow]]]; else *layer_mask=0; scanlinePtr++; ws_tileRow++; layer_mask++;
					if (*ws_tileRow&&*layer_mask) *scanlinePtr=ws_shades[ws_paletteColors[ws_paletteAlias[*ws_tileRow]]]; else *layer_mask=0; scanlinePtr++; ws_tileRow++; layer_mask++;
					if (*ws_tileRow&&*layer_mask) *scanlinePtr=ws_shades[ws_paletteColors[ws_paletteAlias[*ws_tileRow]]]; else *layer_mask=0; scanlinePtr++; ws_tileRow++; layer_mask++;
					if (*ws_tileRow&&*layer_mask) *scanlinePtr=ws_shades[ws_paletteColors[ws_paletteAlias[*ws_tileRow]]]; else *layer_mask=0; scanlinePtr++; ws_tileRow++; layer_mask++;
					if (*ws_tileRow&&*layer_mask) *scanlinePtr=ws_shades[ws_paletteColors[ws_paletteAlias[*ws_tileRow]]]; else *layer_mask=0; scanlinePtr++; ws_tileRow++; layer_mask++;
				}
				else
				{
					if(*layer_mask) *scanlinePtr=ws_shades[ws_paletteColors[ws_paletteAlias[*ws_tileRow]]]; scanlinePtr++; ws_tileRow++; layer_mask++;
					if(*layer_mask) *scanlinePtr=ws_shades[ws_paletteColors[ws_paletteAlias[*ws_tileRow]]]; scanlinePtr++; ws_tileRow++; layer_mask++;
					if(*layer_mask) *scanlinePtr=ws_shades[ws_paletteColors[ws_paletteAlias[*ws_tileRow]]]; scanlinePtr++; ws_tileRow++; layer_mask++;
					if(*layer_mask) *scanlinePtr=ws_shades[ws_paletteColors[ws_paletteAlias[*ws_tileRow]]]; scanlinePtr++; ws_tileRow++; layer_mask++;
					if(*layer_mask) *scanlinePtr=ws_shades[ws_paletteColors[ws_paletteAlias[*ws_tileRow]]]; scanlinePtr++; ws_tileRow++; layer_mask++;
					if(*layer_mask) *scanlinePtr=ws_shades[ws_paletteColors[ws_paletteAlias[*ws_tileRow]]]; scanlinePtr++; ws_tileRow++; layer_mask++;
					if(*layer_mask) *scanlinePtr=ws_shades[ws_paletteColors[ws_paletteAlias[*ws_tileRow]]]; scanlinePtr++; ws_tileRow++; layer_mask++;
					if(*layer_mask) *scanlinePtr=ws_shades[ws_paletteColors[ws_paletteAlias[*ws_tileRow]]]; scanlinePtr++; ws_tileRow++; layer_mask++;
				}
			}

			// render the last clipped tile
			if (columnInTile)
			{
				WORD	tileInfo=ws_fgScrollRamBase[ws_currentTile&0x1f];
				ws_currentTile++;
				BYTE	*ws_tileRow=ws_tileCache_getTileRow(tileInfo&0x1ff, lineInTile,
													tileInfo&0x8000, tileInfo&0x4000, tileInfo&0x2000);
				WORD	*ws_paletteAlias=&ws_palette[((tileInfo>>9)&0x0f)<<2];
				

				if (tileInfo&0x0800)
				{
					for (int i=0;i<columnInTile;i++)
					{
						if (*ws_tileRow&&*layer_mask)	
							*scanlinePtr=ws_shades[ws_paletteColors[ws_paletteAlias[*ws_tileRow]]];
						else
							*layer_mask=0;
						scanlinePtr++; 
						ws_tileRow++;
						layer_mask++;
					}
				}
				else
				{
					for (int i=0;i<columnInTile;i++)
					{
						if(*layer_mask) 
							*scanlinePtr=ws_shades[ws_paletteColors[ws_paletteAlias[*ws_tileRow]]]; 
						scanlinePtr++; 
						ws_tileRow++;
						layer_mask++;
					}
				}
			}
		}
	}
#ifdef STATISTICS	
	ws_foreground_rendering_time=ticker();
#endif
	// render sprites
	if (ws_ioRam[0x00]&0x04&&sprite_count&&sprite_on)
	{
		int ws_sprWindow_x0=ws_ioRam[0x0c];
		int ws_sprWindow_y0=ws_ioRam[0x0d];
		int ws_sprWindow_x1=ws_ioRam[0x0e];
		int ws_sprWindow_y1=ws_ioRam[0x0f];
		if(ws_sprWindow_x0<0)
			ws_sprWindow_x0=0;
		if(ws_sprWindow_x1>223)
			ws_sprWindow_x1=223;

		// seek to first sprite
		DWORD *ws_sprRamBase=(DWORD*)sprite_table+sprite_count;
		char *window_mask=w_mask;

		for (int i=sprite_count;i>0;i--)
		{
			(DWORD*)ws_sprRamBase--;
			DWORD spr=*ws_sprRamBase;
			short x= (short)((spr&0xff000000)>>24);
			short y= (short)((spr&0x00ff0000)>>16);
			DWORD tileIndex=spr&0x1ff;
			DWORD paletteIndex=((spr&0xe00)>>9)+8;
			DWORD hFlip=spr&0x8000;
			DWORD vFlip=spr&0x4000;
			DWORD layer=spr&0x2000;
			DWORD clip_side=spr&0x1000;

			if (y>248) y-=256;
			if ((ws_gpu_scanline<y)||(ws_gpu_scanline>(y+7)))
				continue;
			if (x>248) x-=256;

			BYTE	*ws_tileRow=ws_tileCache_getTileRow(tileIndex,(ws_gpu_scanline-y)&0x07,hFlip,vFlip, 0);
			BYTE	nbPixels=8;

			if(ws_ioRam[0x00]&0x08)
			{
				if(clip_side)
				{
					memset(window_mask,1,224);
					if((ws_gpu_scanline >= ws_sprWindow_y0)&&(ws_gpu_scanline <= ws_sprWindow_y1))
					{
						for(int c = ws_sprWindow_x0; c <= ws_sprWindow_x1; c++)
							w_mask[c] = 0;
					}
				}
				else
				{
					memset(window_mask, 0, 224);
					if((ws_gpu_scanline >= ws_sprWindow_y0)&&(ws_gpu_scanline <= ws_sprWindow_y1))
					{
						for(int c = ws_sprWindow_x0; c <= ws_sprWindow_x1; c++)
							w_mask[c] = 1;
					}
				}
			}
			else
				memset(window_mask, 1, 224);
		
			if (x<0)
			{
				ws_tileRow -= x;
				nbPixels += x;
				x=0;
			}
			if (x + nbPixels > 224)
				nbPixels = (224 - x);

			DWORD *f_buffer = framebuffer+x;
			char *layer_mask = l_mask+x;
			char *window_mask = w_mask+x;

			if (ws_videoMode)
			{
				DWORD	*wsc_paletteAlias = &wsc_palette[paletteIndex<<4];
				if ((ws_ioRam[0x60]&0x40)||(paletteIndex&0x04))
				{
					while (nbPixels)
					{
						if (*ws_tileRow&&*window_mask&&(layer||!(*layer_mask)))
							*f_buffer = wsc_paletteAlias[*ws_tileRow];
						f_buffer++;
						ws_tileRow++;
						layer_mask++;
						window_mask++;
						nbPixels--;
					}
				}
				else
				{
					while (nbPixels)
					{
						if(*window_mask&&(layer||!(*layer_mask)))
							*f_buffer = wsc_paletteAlias[*ws_tileRow];
						f_buffer++;
						ws_tileRow++;
						layer_mask++;
						window_mask++;
						nbPixels--;
					}
				}
			}
			else
			{
				WORD	*ws_paletteAlias = &ws_palette[paletteIndex<<2];
				if (paletteIndex&0x04)
				{
					while (nbPixels)
					{
						if (*ws_tileRow&&*window_mask&&(layer||!(*layer_mask)))
							*f_buffer = ws_shades[ws_paletteColors[ws_paletteAlias[*ws_tileRow]]]; 
						f_buffer++;
						ws_tileRow++;
						layer_mask++;
						window_mask++;
						nbPixels--;
					}
				}
				else
				{
					while (nbPixels)
					{
						if(*window_mask&&(layer||!(*layer_mask)))
							*f_buffer = ws_shades[ws_paletteColors[ws_paletteAlias[*ws_tileRow]]]; 
						f_buffer++;
						ws_tileRow++;
						layer_mask++;
						window_mask++;
						nbPixels--;
					}
				}
			}
		}
	}
#ifdef STATISTICS	
	ws_sprites_rendering_time=ticker();

	ws_sprites_rendering_time-=ws_foreground_rendering_time;
	ws_foreground_rendering_time-=ws_background_rendering_time;
	ws_background_rendering_time-=ws_background_color_rendering_time;
	ws_background_color_rendering_time-=startTime;
#endif
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//void ws_gpu_write_byte(DWORD offset, BYTE value)
//{
//  if ((offset >= 0x2000) && (offset < 0x4000)) { // ws 4 color tiles bank 0
//    if (internalRam[offset] != value)
//      ws_modified_tile[(offset & 0x1fff) >> 4] = 1;
//  }
//  else if ((offset >= 0x4000) && (offset < 0x6000)) { // ws 4 color tiles bank 1
//    if (internalRam[offset] != value)
//      ws_modified_tile[512 + ((offset & 0x1fff) >> 4)] = 1;
//  }
//
//  if ((offset >= 0x4000) && (offset < 0x8000)) { // wsc 16 color tiles bank 0
//    if (internalRam[offset] != value)
//      wsc_modified_tile[(offset & 0x3fff) >> 5] = 1;
//  }
//  else if ((offset >= 0x8000) && (offset < 0xc000)) { // wsc 16 color tiles bank 1
//    if (internalRam[offset] != value)
//      wsc_modified_tile[512 + ((offset & 0x3fff) >> 5)] = 1;
//  }
//
//  // update the ram
//  internalRam[offset] = value;
//
//  // palette ram
//  if (offset >= 0xfe00) {
//    // RGB444 format
//    WORD color = internalRam[(offset & 0xfffe) + 1];
//    color <<= 8;
//    color  |= internalRam[(offset&0xfffe)];
//
//    DWORD R   = (color >> 8) & 0x0f;
//    DWORD G   = (color >> 4) & 0x0f;
//    DWORD B   =  color       & 0x0f;
//    DWORD PAL = RGB555(R, G, B);
//
//    wsc_palette[(offset & 0x1ff) >> 1] = PAL;
//  }
//}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
unsigned int ws_gpu_unknownPort;
void ws_gpu_port_write(DWORD port,BYTE value)
{
	ws_gpu_unknownPort=0;
	switch (port)
	{
	case 0x60:
//				fprintf(log_get(),"%02X -> port %02X\n",value,port);
				ws_gpu_changeVideoMode(value);
				return;
	case 0x1C:	ws_paletteColors[0]=value&0xf;
				ws_paletteColors[1]=(value>>4)&0xf;
				return;
	case 0x1D:	ws_paletteColors[2]=value&0xf;
				ws_paletteColors[3]=(value>>4)&0xf;
				return;
	case 0x1E:	ws_paletteColors[4]=value&0xf;
				ws_paletteColors[5]=(value>>4)&0xf;
				return;
	case 0x1F:	ws_paletteColors[6]=value&0xf;
				ws_paletteColors[7]=(value>>4)&0xf;
				return;
	default:	ws_gpu_unknownPort=1;
	}
	if ((port>=0x20)&&(port<=0x3f))
	{
		ws_gpu_unknownPort=0;
		port-=0x20;
		int paletteIndex=port>>1;
		if (port&0x01)
		{
			ws_palette[(paletteIndex<<2)+2]=value&0x7; 
			ws_palette[(paletteIndex<<2)+3]=(value>>4)&0x7;
		}
		else
		{
			ws_palette[(paletteIndex<<2)+0]=value&0x7; 
			ws_palette[(paletteIndex<<2)+1]=(value>>4)&0x7;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
BYTE ws_gpu_port_read(BYTE port)
{
  if (port == 0xa0) {
    if (ws_gpu_forceMonoSystemBool)
      return ws_ioRam[0xa0] & (~0x02);
    else
      return ws_ioRam[0xa0] | 2;
  }

  return ws_ioRam[port];
}
