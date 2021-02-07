/*
 * SSEQ Player - Player structure
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2014-10-25
 *
 * Adapted from source code of FeOS Sound System
 * By fincs
 * https://github.com/fincs/FSS
 */

#include "Player.h"
#include "common.h"

Player::Player() : prio(0), nTracks(0), tempo(0), tempoCount(0), tempoRate(0), masterVol(0), sseqVol(0), sseq(nullptr), allowedChannels(0), sampleRate(0),
	interpolation(INTERPOLATION_NONE)
{
	memset(this->trackIds, 0, sizeof(this->trackIds));
	for (size_t i = 0; i < 16; ++i)
	{
		this->channels[i].chnId = i;
		this->channels[i].ply = this;
	}
	memset(this->variables, -1, sizeof(this->variables));
}

// Original FSS Function: Player_Setup
bool Player::Setup(const SSEQ *sseqToPlay)
{
	this->sseq = sseqToPlay;

	int firstTrack = this->TrackAlloc();
	if (firstTrack == -1)
		return false;
	this->tracks[firstTrack].Init(firstTrack, this, nullptr, 0);

	this->nTracks = 1;
	this->trackIds[0] = firstTrack;

	this->tracks[firstTrack].startPos = this->tracks[firstTrack].pos = &this->sseq->data[0];

	this->ClearState();

	return true;
}

// Original FSS Function: Player_ClearState
void Player::ClearState()
{
	this->tempo = 120;
	this->tempoCount = 0;
	this->tempoRate = 0x100;
	this->masterVol = 0; // this is actually the highest level
	memset(this->variables, -1, sizeof(this->variables));
}

// Original FSS Function: Player_FreeTracks
void Player::FreeTracks()
{
	for (uint8_t i = 0; i < this->nTracks; ++i)
		this->tracks[this->trackIds[i]].Free();
	this->nTracks = 0;
}

// Original FSS Function: Player_Stop
void Player::Stop(bool bKillSound)
{
	this->ClearState();
	for (uint8_t i = 0; i < this->nTracks; ++i)
	{
		uint8_t trackId = this->trackIds[i];
		this->tracks[trackId].ClearState();
		for (int j = 0; j < 16; ++j)
		{
			NCSFChannel &chn = this->channels[j];
			if (chn.state != CS_NONE && chn.trackId == trackId)
			{
				if (bKillSound)
					chn.Kill();
				else
					chn.Release();
			}
		}
	}
	this->FreeTracks();
}

// Original FSS Function: Chn_Alloc
int Player::ChannelAlloc(int type, int priority)
{
	static const uint8_t pcmChnArray[] = { 4, 5, 6, 7, 2, 0, 3, 1, 8, 9, 10, 11, 14, 12, 15, 13 };
	static const uint8_t psgChnArray[] = { 8, 9, 10, 11, 12, 13 };
	static const uint8_t noiseChnArray[] = { 14, 15 };
	static const uint8_t arraySizes[] = { sizeof(pcmChnArray), sizeof(psgChnArray), sizeof(noiseChnArray) };
	static const uint8_t *const arrayArray[] = { pcmChnArray, psgChnArray, noiseChnArray };

	auto chnArray = arrayArray[type];
	int arraySize = arraySizes[type];

	int curChnNo = -1;
	for (int i = 0; i < arraySize; ++i)
	{
		int thisChnNo = chnArray[i];
		if (!this->allowedChannels[thisChnNo])
			continue;
		NCSFChannel &thisChn = this->channels[thisChnNo];
		NCSFChannel &curChn = this->channels[curChnNo];
		if (curChnNo != -1 && thisChn.prio >= curChn.prio)
		{
			if (thisChn.prio != curChn.prio)
				continue;
			if (curChn.vol <= thisChn.vol)
				continue;
		}
		curChnNo = thisChnNo;
	}

	if (curChnNo == -1 || priority < this->channels[curChnNo].prio)
		return -1;
	this->channels[curChnNo].noteLength = -1;
	this->channels[curChnNo].vol = 0x7FF;
	return curChnNo;
}

// Original FSS Function: Track_Alloc
int Player::TrackAlloc()
{
	for (int i = 0; i < FSS_MAXTRACKS; ++i)
	{
		Track &thisTrk = this->tracks[i];
		if (!thisTrk.state[TS_ALLOCBIT])
		{
			thisTrk.Zero();
			thisTrk.state.set(TS_ALLOCBIT);
			thisTrk.updateFlags.reset();
			return i;
		}
	}
	return -1;
}

// Original FSS Function: Player_Run
void Player::Run()
{
	while (this->tempoCount > 240)
	{
		this->tempoCount -= 240;
		for (uint8_t i = 0; i < this->nTracks; ++i)
			this->tracks[this->trackIds[i]].Run();
	}
	this->tempoCount += (static_cast<int>(this->tempo) * static_cast<int>(this->tempoRate)) >> 8;
}

void Player::UpdateTracks()
{
	for (int i = 0; i < 16; ++i)
		this->channels[i].UpdateTrack();
	for (int i = 0; i < FSS_MAXTRACKS; ++i)
		this->tracks[i].updateFlags.reset();
}

// Original FSS Function: Snd_Timer
void Player::Timer()
{
	this->UpdateTracks();

	for (int i = 0; i < 16; ++i)
		this->channels[i].Update();

	this->Run();
}
