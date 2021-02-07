/*
 * SSEQ Player - Player structure
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2014-10-25
 *
 * Adapted from source code of FeOS Sound System
 * By fincs
 * https://github.com/fincs/FSS
 */

#pragma once

#include <bitset>
#include "SSEQ.h"
#include "Track.h"
#include "NCSFChannel.h"
#include "consts.h"

struct Player
{
	uint8_t prio, nTracks;
	uint16_t tempo, tempoCount, tempoRate /* 8.8 fixed point */;
	int16_t masterVol, sseqVol;

	const SSEQ *sseq;

	uint8_t trackIds[FSS_TRACKCOUNT];
	Track tracks[FSS_MAXTRACKS];
	NCSFChannel channels[16];
	std::bitset<16> allowedChannels;
	int16_t variables[32];

	uint32_t sampleRate;
	Interpolation interpolation;

	Player();

	bool Setup(const SSEQ *sseq);
	void ClearState();
	void FreeTracks();
	void Stop(bool bKillSound);
	int ChannelAlloc(int type, int prio);
	int TrackAlloc();
	void Run();
	void UpdateTracks();
	void Timer();
};
