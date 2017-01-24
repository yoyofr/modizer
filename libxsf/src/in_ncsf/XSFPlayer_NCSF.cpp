/*
 * xSF - NCSF Player
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2014-10-25
 *
 * Partially based on the vio*sf framework
 *
 * Utilizes a modified FeOS Sound System for playback
 * https://github.com/fincs/FSS
 */

#include <memory>
#include <cstdlib>
#include <ctime>
#include <zlib.h>
#include "convert.h"
#include "XSFPlayer_NCSF.h"
#include "XSFConfig_NCSF.h"
#include "XSFCommon.h"
#include "SSEQPlayer/SDAT.h"
#include "SSEQPlayer/Player.h"

const char *XSFPlayer::WinampDescription = "NCSF Decoder";
const char *XSFPlayer::WinampExts = "ncsf;minincsf\0DS Nitro Composer Sound Format files (*.ncsf;*.minincsf)\0";

extern XSFConfig *xSFConfig;

XSFPlayer *XSFPlayer::Create(const std::string &fn)
{
	return new XSFPlayer_NCSF(fn);
}


void XSFPlayer_NCSF::MapNCSFSection(const std::vector<uint8_t> &section)
{
	uint32_t size = Get32BitsLE(&section[8]), finalSize = size;
	if (this->sdatData.empty())
		this->sdatData.resize(finalSize, 0);
	else if (this->sdatData.size() < size)
		this->sdatData.resize(finalSize);
	memcpy(&this->sdatData[0], &section[0], size);
}

bool XSFPlayer_NCSF::MapNCSF(XSFFile *xSFToLoad)
{
	if (!xSFToLoad->IsValidType(0x25))
		return false;

	auto &reservedSection = xSFToLoad->GetReservedSection(), &programSection = xSFToLoad->GetProgramSection();

	if (!reservedSection.empty())
		this->sseq = Get32BitsLE(&reservedSection[0]);

	if (!programSection.empty())
		this->MapNCSFSection(programSection);

	return true;
}

bool XSFPlayer_NCSF::RecursiveLoadNCSF(XSFFile *xSFToLoad, int level)
{
	if (level <= 10 && xSFToLoad->GetTagExists("_lib"))
	{
		auto libxSF = std::unique_ptr<XSFFile>(new XSFFile(ExtractDirectoryFromPath(xSFToLoad->GetFilename()) + xSFToLoad->GetTagValue("_lib"), 8, 12));
		if (!this->RecursiveLoadNCSF(libxSF.get(), level + 1))
			return false;
	}

	if (!this->MapNCSF(xSFToLoad))
		return false;

	unsigned n = 2;
	bool found;
	do
	{
		found = false;
		std::string libTag = "_lib" + stringify(n++);
		if (xSFToLoad->GetTagExists(libTag))
		{
			found = true;
			auto libxSF = std::unique_ptr<XSFFile>(new XSFFile(ExtractDirectoryFromPath(xSFToLoad->GetFilename()) + xSFToLoad->GetTagValue(libTag), 8, 12));
			if (!this->RecursiveLoadNCSF(libxSF.get(), level + 1))
				return false;
		}
	} while (found);

	return true;
}

bool XSFPlayer_NCSF::LoadNCSF()
{
	return this->RecursiveLoadNCSF(this->xSF.get(), 1);
}

XSFPlayer_NCSF::XSFPlayer_NCSF(const std::string &filename) : XSFPlayer()
{
	this->uses32BitSamplesClampedTo16Bit = true;
	this->xSF.reset(new XSFFile(filename, 8, 12));
}


XSFPlayer_NCSF::~XSFPlayer_NCSF()
{
}

bool XSFPlayer_NCSF::Load()
{
	if (!this->LoadNCSF())
		return false;

	std::srand(static_cast<unsigned>(std::time(nullptr)));

	PseudoFile file;
	file.data = &this->sdatData;
	this->sdat.reset(new SDAT(file, this->sseq));
	auto *sseqToPlay = this->sdat->sseq.get();
	this->player.allowedChannels = std::bitset<16>(this->sdat->player.channelMask);
	this->player.sseqVol = Cnv_Scale(sseqToPlay->info.vol);
	this->player.sampleRate = this->sampleRate;
	this->player.Setup(sseqToPlay);
	this->player.Timer();
	this->secondsPerSample = 1.0 / this->sampleRate;
	this->secondsIntoPlayback = 0;
	this->secondsUntilNextClock = SecondsPerClockCycle;

	return XSFPlayer::Load();
}

static inline int32_t muldiv7(int32_t val, uint8_t mul)
{
	return mul == 127 ? val : ((val * mul) >> 7);
}

void XSFPlayer_NCSF::GenerateSamples(std::vector<uint8_t> &buf, unsigned offset, unsigned samples)
{
	unsigned long mute = this->mutes.to_ulong();

	for (unsigned smpl = 0; smpl < samples; ++smpl)
	{
		this->secondsIntoPlayback += this->secondsPerSample;

		int32_t leftChannel = 0, rightChannel = 0;

		// I need to advance the sound channels here
		for (int i = 0; i < 16; ++i)
		{
			NCSFChannel &chn = this->player.channels[i];

			if (chn.state > CS_NONE)
			{
				int32_t sample = chn.GenerateSample();
				chn.IncrementSample();

				if (mute & SBIT(i))
					continue;

				uint8_t datashift = chn.reg.volumeDiv;
				if (datashift == 3)
					datashift = 4;
				sample = muldiv7(sample, chn.reg.volumeMul) >> datashift;

				leftChannel += muldiv7(sample, 127 - chn.reg.panning);
				rightChannel += muldiv7(sample, chn.reg.panning);
			}
		}

		buf[offset++] = leftChannel & 0xFF;
		buf[offset++] = (leftChannel >> 8) & 0xFF;
		buf[offset++] = (leftChannel >> 16) & 0xFF;
		buf[offset++] = (leftChannel >> 24) & 0xFF;
		buf[offset++] = rightChannel & 0xFF;
		buf[offset++] = (rightChannel >> 8) & 0xFF;
		buf[offset++] = (rightChannel >> 16) & 0xFF;
		buf[offset++] = (rightChannel >> 24) & 0xFF;

		if (this->secondsIntoPlayback > this->secondsUntilNextClock)
		{
			this->player.Timer();
			this->secondsUntilNextClock += SecondsPerClockCycle;
		}
	}
}

void XSFPlayer_NCSF::Terminate()
{
	this->player.Stop(true);
}

void XSFPlayer_NCSF::SetInterpolation(unsigned interpolation)
{
	this->player.interpolation = static_cast<Interpolation>(interpolation);
}

void XSFPlayer_NCSF::SetMutes(const std::bitset<16> &newMutes)
{
	this->mutes = newMutes;
}

