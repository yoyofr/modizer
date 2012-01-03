/*-----------------------------------------------------------------------------

	ST-Sound ( YM files player library )

	Copyright (C) 1995-1999 Arnaud Carre ( http://leonard.oxg.free.fr )

	This is a sample program: it's a real-time YM player using windows WaveOut API.

-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------

	This file is part of ST-Sound

	ST-Sound is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	ST-Sound is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with ST-Sound; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

-----------------------------------------------------------------------------*/

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include "SoundServer.h"
#include "../StSoundLibrary/StSoundLibrary.h"


// Globals
static	volatile	YMMUSIC			*	s_pMusic = NULL;
static	CSoundServer					soundServer;



static	void	soundServerCallback(void *pBuffer,long size)
{

	if (s_pMusic)
	{
		int nbSample = size / sizeof(ymsample);
		ymMusicCompute((void*)s_pMusic,(ymsample*)pBuffer,nbSample);
	}
}





int main(int argc, char* argv[])
{

	//--------------------------------------------------------------------------
	// Checks args.
	//--------------------------------------------------------------------------
	printf(	"SmallYmPlayer.\n"
			"Using ST-Sound Library, under GPL license\n"
			"Copyright (C) 1995-1999 Arnaud Carre ( http://leonard.oxg.free.fr )\n");
	
	if (argc!=2)
	{
		printf("Usage: SmallYmPlayer <ym music file>\n\n");
		return -1;
	}


	//--------------------------------------------------------------------------
	// Load YM music and creates WAV file
	//--------------------------------------------------------------------------
	printf("Loading music \"%s\"...\n",argv[1]);
	YMMUSIC *pMusic = ymMusicCreate();

	if (ymMusicLoad(pMusic,argv[1]))
	{
		if (soundServer.open(soundServerCallback,500))
		{

			ymMusicInfo_t info;
			ymMusicGetInfo(pMusic,&info);
			printf("Name.....: %s\n",info.pSongName);
			printf("Author...: %s\n",info.pSongAuthor);
			printf("Comment..: %s\n",info.pSongComment);
			printf("Duration.: %d:%02d\n",info.musicTimeInSec/60,info.musicTimeInSec%60);

			printf("\nPlaying music...(press a key to abort)\n");

			ymMusicSetLoopMode(pMusic,YMTRUE);
			ymMusicPlay(pMusic);
			s_pMusic = pMusic;			// global instance for soundserver callback

			int oldSec = -1;
			for (;;)
			{
				if (_kbhit())
					break;

				int sec = ymMusicGetPos(pMusic) / 1000;
				if (sec != oldSec)
				{
					printf("Time: %d:%02d\r",sec/60,sec%60);
					oldSec = sec;
				}

				Sleep(20);
			}
			_getch();		// flush the key pressed
			printf("\n");

			// Switch off replayer
			s_pMusic = NULL;
			ymMusicStop(pMusic);
			soundServer.close();

		}
		else
		{
			printf("ERROR: Unable to initialize sound card hardware\n");
		}

	}
	else
	{
		printf("Error in loading file %s:\n%s\n",argv[1],ymMusicGetLastError(pMusic));
	}

	ymMusicDestroy(pMusic);

	return 0;
}

