/*
 * SSEQ Player - SDAT structure
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2014-10-25
 *
 * Nintendo DS Nitro Composer (SDAT) Specification document found at
 * http://www.feshrine.net/hacking/doc/nds-sdat.html
 */

#pragma once

#include <memory>
#include "SSEQ.h"
#include "SBNK.h"
#include "SWAR.h"
#include "common.h"

struct SDAT
{
	std::unique_ptr<SSEQ> sseq;
	std::unique_ptr<SBNK> sbnk;
	std::unique_ptr<SWAR> swar[4];
	INFOEntryPLAYER player;

	SDAT(PseudoFile &file, uint32_t sseqToLoad);
private:
	SDAT(const SDAT &);
	SDAT &operator=(const SDAT &);
};
