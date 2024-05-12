#pragma once

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct
	{
		int(*pLoadWSR)(const void*, unsigned);
		int(*pGetFirstSong)(void);
		unsigned(*pSetFrequency)(unsigned int);
		void(*pSetChannelMuting)(unsigned int);
		unsigned int(*pGetChannelMuting)(void);
		void(*pResetWSR)(unsigned);
		void(*pCloseWSR)(void);
		int(*pUpdateWSR)(void*, unsigned, unsigned);

	} WSRPlayerApi;

#ifdef __cplusplus
}
#endif