#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <Windows.h>
#include "wsr_types.h" //YOYOFR

//TODO:  MODIZER changes start / YOYOFR
#include "../../../../src/ModizerVoicesData.h"
//TODO:  MODIZER changes end / YOYOFR


#include "../wsr_player_common/wsr_player_intf.h"
#include "../wsr_player_common/wsr_player_common.h"
namespace oswan
{
	//#include "wsr_player.h"
	#include "io.h"
	#include "ws.h"
	#include "nec/necintrf.h"
	#include "audio.h"


	static uint32_t g_channel_mute = 0;
	static uint8_t  g_first_song_number = 0;
	static WSRPlayerSoundBuf g_buffer = { 0, 0, 0, 0 };


	void InitWSR(void)
	{
		ws_audio_init();
		ws_io_init();
	}

	int LoadWSR(const void* pFile, unsigned Size)
	{
		if (Size <= 0x20 || !pFile) return 0;
		if (ReadLE32((const uint8_t*)pFile + Size - 0x20) != WSR_HEADER_MAGIC)
			return 0;

		uint32_t buflen = (4096 << 2) + 16;
		g_buffer.ptr = (uint8_t*)malloc(buflen);
		if (!g_buffer.ptr)
		{
			return 0;
		}
		g_buffer.len = buflen;
		g_buffer.fil = 0;
		g_buffer.cur = 0;

		if (ws_init(pFile, Size) == 0)	//error
			return 0;

		g_first_song_number = *((const uint8_t*)pFile + Size - 0x20 + 0x05);
		InitWSR();

		return 1;
	}


	void CloseWSR()
	{
		ws_done();
		ws_io_done();
		ws_audio_done();

		if (g_buffer.ptr)
		{
			free(g_buffer.ptr);
			g_buffer.ptr = 0;
		}
		g_buffer.len = 0;
		g_buffer.fil = 0;
		g_buffer.cur = 0;
	}


	int GetFirstSong(void)
	{
		return g_first_song_number;
	}

	unsigned SetFrequency(unsigned int Freq)
	{
		ws_audio_set_bps(Freq);
		return ws_audio_get_bps();
	}

	void SetChannelMuting(unsigned int Mute)
	{
		g_channel_mute = Mute;
//		for (unsigned i = 0; i < 4; i++)
//		{
//			ws_audio_set_channel_enabled(i, (Mute & (1 << i)) ? 0 : 1);
//		}
//
//		ws_audio_set_channel_enabled(4, (Mute & (1 << 1)) ? 0 : 1);	//voice
//		ws_audio_set_channel_enabled(5, (Mute & (1 << 3)) ? 0 : 1);	//noise
//		ws_audio_set_channel_enabled(6, (Mute & (1 << 2)) ? 0 : 1);	//sweep
//		ws_audio_set_channel_enabled(7, (Mute & (1 << 1)) ? 0 : 1);	//hyper voice
        
        for (unsigned i = 0; i < 4; i++) {
            ws_audio_set_channel_enabled(i, (Mute & (1 << i)) ? 0 : 1);
        }
        ws_audio_set_channel_enabled(4, (Mute & (1 << 4)) ? 0 : 1);    //voice
        ws_audio_set_channel_enabled(5, (Mute & (1 << 5)) ? 0 : 1);    //noise
        ws_audio_set_channel_enabled(6, (Mute & (1 << 2)) ? 0 : 1);    //sweep
        ws_audio_set_channel_enabled(7, (Mute & (1 << 4)) ? 0 : 1);    //hyper voice

		return;
	}

	unsigned int GetChannelMuting(void)
	{
		return g_channel_mute;
	}

	void ResetWSR(unsigned SongNo)
	{
		ws_reset();
		ws_io_reset();
		ws_audio_reset();
		nec_reset(NULL);
		nec_set_reg(NEC_SP, 0x2000);
		nec_set_reg(NEC_AW, SongNo);
		ws_audio_play();
		g_buffer.cur = 0;
		g_buffer.fil = 0;
	}

	void FlushBufferWSR(const short* finalWave, unsigned long length)
	{
		if (length > g_buffer.len - g_buffer.fil)
		{
			length = g_buffer.len - g_buffer.fil;
		}
		if (length > 0)
		{
			memcpy(g_buffer.ptr + g_buffer.fil, finalWave, length);
			g_buffer.fil += length;
		}
	}

	//refferd to viogsf's drvimpl.cpp. thanks the author.
	int UpdateWSR(void* pBuf, unsigned Buflen, unsigned Samples)
	{
        int vgm_cleaned=0;//YOYOFR
		int ret = 0;
		uint32_t bytes = Samples << 2;
		uint8_t* des = (uint8_t*)pBuf;

		if (g_buffer.ptr == 0 || pBuf == 0)
			return 0;

		while (bytes > 0)
		{
			uint32_t remain = g_buffer.fil - g_buffer.cur;
			while (remain == 0)
			{
				g_buffer.cur = 0;
				g_buffer.fil = 0;
                
                if (!vgm_cleaned) {
                    memset(vgm_last_note,0,sizeof(vgm_last_note));
                    memset(vgm_last_vol,0,sizeof(vgm_last_vol));
                    vgm_cleaned=1;
                }

				WsExecuteLine(NULL, FALSE);

				remain = g_buffer.fil - g_buffer.cur;
			}
			uint32_t len = remain;
			if (len > bytes)
			{
				len = bytes;
			}
			memcpy(des, g_buffer.ptr + g_buffer.cur, len);
			bytes -= len;
			des += len;
			g_buffer.cur += len;
			ret += len;
		}
		return ret;

	}


	WSRPlayerApi g_wsr_player_api =
	{
		LoadWSR,
		GetFirstSong,
		SetFrequency,
		SetChannelMuting,
		GetChannelMuting,
		ResetWSR,
		CloseWSR,
		UpdateWSR
	};
}
// namespace oswan
