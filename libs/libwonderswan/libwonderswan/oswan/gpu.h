//////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//////////////////////////////////////////////////////////////////////////////

#ifndef __GPU_H__
#define __GPU_H__

#define COLOUR_SCHEME_DEFAULT	0
#define COLOUR_SCHEME_AMBER		1
#define COLOUR_SCHEME_GREEN		2

#define RGB555(R,G,B) (DWORD)((((int)(R))<<(28 - SftR))|(((int)(G))<<(28 - SftG))|(((int)(B))<<(28 - SftB)))

extern	int		ws_gpu_scanline;
//extern	BYTE	ws_gpu_operatingInColor;
extern	BYTE	ws_videoMode;
extern	WORD	ws_palette[16*4];
extern	BYTE	ws_paletteColors[8];
extern	DWORD	wsc_palette[16*16];
extern  unsigned int ws_gpu_unknownPort;
extern	int		ws_gpu_forceMonoSystemBool;

extern	int		layer1_on;
extern	int		layer2_on;
extern	int		sprite_on;

void ws_gpu_init(void);
void ws_gpu_done(void);
void ws_gpu_reset(void);
void ws_gpu_renderScanline(DWORD *framebuffer);
void ws_gpu_changeVideoMode(BYTE value);
//void ws_gpu_write_byte(DWORD offset, BYTE value);
void ws_gpu_port_write(DWORD port,BYTE value);
BYTE ws_gpu_port_read(BYTE port);
//void ws_gpu_set_colour_scheme(int scheme);
void ws_gpu_forceColorSystem(void);
void ws_gpu_forceMonoSystem(void);
void ws_gpu_clearCache(void);

#endif

