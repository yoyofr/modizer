/*
 * SSEQ Player - SDAT INFO Entry structures
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2014-10-25
 *
 * Nintendo DS Nitro Composer (SDAT) Specification document found at
 * http://www.feshrine.net/hacking/doc/nds-sdat.html
 */

#pragma once

#include "common.h"

struct INFOEntry
{
	virtual ~INFOEntry()
	{
	}

	virtual void Read(PseudoFile &file) = 0;
};

struct INFOEntrySEQ : INFOEntry
{
	uint16_t fileID;
	uint16_t bank;
	uint8_t vol;
	uint8_t ply;

	INFOEntrySEQ();

	void Read(PseudoFile &file);
};

struct INFOEntryBANK : INFOEntry
{
	uint16_t fileID;
	uint16_t waveArc[4];

	INFOEntryBANK();

	void Read(PseudoFile &file);
};

struct INFOEntryWAVEARC : INFOEntry
{
	uint16_t fileID;

	INFOEntryWAVEARC();

	void Read(PseudoFile &file);
};

struct INFOEntryPLAYER : INFOEntry
{
	uint16_t channelMask;

	INFOEntryPLAYER();

	void Read(PseudoFile &file);
};
