/*
 * xSF - 2SF Player
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2014-09-24
 *
 * Based on a modified vio2sf v0.22c
 *
 * Partially based on the vio*sf framework
 *
 * Utilizes a modified DeSmuME v0.9.9 SVN for playback
 * http://desmume.org/
 */

#include "XSFPlayer_2SF.h"
#include "XSFCommon.h"

const char *XSFPlayer::WinampDescription = "2SF Decoder";
const char *XSFPlayer::WinampExts = "2sf;mini2sf\0DS Sound Format files (*.2sf;*.mini2sf)\0";

XSFPlayer *XSFPlayer::Create(const std::string &fn)
{
	return new XSFPlayer_2SF(fn);
}

#ifdef _WIN32
XSFPlayer *XSFPlayer::Create(const std::wstring &fn)
{
	return new XSFPlayer_2SF(fn);
}
#endif

volatile bool execute = false;

static struct
{
	std::vector<uint8_t> buf;
	unsigned filled, used;
	uint32_t bufferbytes, cycles;
	int xfs_load, sync_type;
} sndifwork = {std::vector<uint8_t>(), 0, 0, 0, 0, 0, 0};

static void SNDIFDeInit() { }

static int SNDIFInit(int buffersize)
{
	uint32_t bufferbytes = buffersize * sizeof(int16_t);
	SNDIFDeInit();
	sndifwork.buf.resize(bufferbytes + 3);
	sndifwork.bufferbytes = bufferbytes;
	sndifwork.filled = sndifwork.used = 0;
	sndifwork.cycles = 0;
	return 0;
}

static void SNDIFMuteAudio() { }
static void SNDIFUnMuteAudio() { }
static void SNDIFSetVolume(int) { }

static uint32_t SNDIFGetAudioSpace()
{
	return sndifwork.bufferbytes >> 2; // bytes to samples
}

static void SNDIFUpdateAudio(int16_t *buffer, uint32_t num_samples)
{
	uint32_t num_bytes = num_samples << 2;
	if (num_bytes > sndifwork.bufferbytes)
		num_bytes = sndifwork.bufferbytes;
	memcpy(&sndifwork.buf[0], buffer, num_bytes);
	sndifwork.filled = num_bytes;
	sndifwork.used = 0;
}

static const int SNDIFID_2SF = 1;
static SoundInterface_struct SNDIF_2SF =
{
	SNDIFID_2SF,
	"2sf Sound Interface",
	SNDIFInit,
	SNDIFDeInit,
	SNDIFUpdateAudio,
	SNDIFGetAudioSpace,
	SNDIFMuteAudio,
	SNDIFUnMuteAudio,
	SNDIFSetVolume,
	nullptr,
	nullptr,
	nullptr
};

SoundInterface_struct *SNDCoreList[] =
{
	&SNDIF_2SF,
	&SNDDummy,
	nullptr
};

void XSFPlayer_2SF::Map2SFSection(const std::vector<uint8_t> &section)
{
	uint32_t offset = Get32BitsLE(&section[0]), size = Get32BitsLE(&section[4]), finalSize = size + offset;
	finalSize = NextHighestPowerOf2(finalSize);
	if (this->rom.empty())
		this->rom.resize(finalSize + 10, 0);
	else if (this->rom.size() < size + offset)
		this->rom.resize(offset + finalSize + 10);
	memcpy(&this->rom[offset], &section[8], size);
}

bool XSFPlayer_2SF::Map2SF(XSFFile *xSFToLoad)
{
	if (!xSFToLoad->IsValidType(0x24))
		return false;

	const auto &programSection = xSFToLoad->GetProgramSection();

	if (!programSection.empty())
		this->Map2SFSection(programSection);

	return true;
}

bool XSFPlayer_2SF::RecursiveLoad2SF(XSFFile *xSFToLoad, int level)
{
	if (level <= 10 && xSFToLoad->GetTagExists("_lib"))
	{
#ifdef _WIN32
		auto libxSF = std::unique_ptr<XSFFile>(new XSFFile(ConvertFuncs::StringToWString(ExtractDirectoryFromPath(xSFToLoad->GetFilename()) + xSFToLoad->GetTagValue("_lib")), 4, 8));
#else
		auto libxSF = std::unique_ptr<XSFFile>(new XSFFile(ExtractDirectoryFromPath(xSFToLoad->GetFilename()) + xSFToLoad->GetTagValue("_lib"), 4, 8));
#endif
		if (!this->RecursiveLoad2SF(libxSF.get(), level + 1))
			return false;
	}

	if (!this->Map2SF(xSFToLoad))
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
#ifdef _WIN32
			auto libxSF = std::unique_ptr<XSFFile>(new XSFFile(ConvertFuncs::StringToWString(ExtractDirectoryFromPath(xSFToLoad->GetFilename()) + xSFToLoad->GetTagValue(libTag)), 4, 8));
#else
			auto libxSF = std::unique_ptr<XSFFile>(new XSFFile(ExtractDirectoryFromPath(xSFToLoad->GetFilename()) + xSFToLoad->GetTagValue(libTag), 4, 8));
#endif
			if (!this->RecursiveLoad2SF(libxSF.get(), level + 1))
				return false;
		}
	} while (found);

	return true;
}

bool XSFPlayer_2SF::Load2SF(XSFFile *xSFToLoad)
{
	this->rom.clear();

	return this->RecursiveLoad2SF(xSFToLoad, 1);
}

XSFPlayer_2SF::XSFPlayer_2SF(const std::string &filename) : XSFPlayer()
{
	this->xSF.reset(new XSFFile(filename, 4, 8));
}

#ifdef _WIN32
XSFPlayer_2SF::XSFPlayer_2SF(const std::wstring &filename) : XSFPlayer()
{
	this->xSF.reset(new XSFFile(filename, 4, 8));
}
#endif

bool XSFPlayer_2SF::Load()
{
	int frames = this->xSF->GetTagValue("_frames", -1);
	sndifwork.sync_type = this->xSF->GetTagValue("_2sf_sync_type", 0);

	sndifwork.xfs_load = false;
	if (!this->Load2SF(this->xSF.get()))
		return false;

	if (NDS_Init())
		return false;

	SPU_ChangeSoundCore(SNDIFID_2SF, 737);

	execute = false;

	MMU_unsetRom();
	if (!this->rom.empty())
	{
		NDS_SetROM(&this->rom[0], this->rom.size() - 1);
		gameInfo.loadData(reinterpret_cast<char *>(&this->rom[0]), this->rom.size() - 1);
	}

    CommonSettings.use_jit = false;//true;
	NDS_Reset();

	execute = true;

	if (frames > 0)
	{
		/* skip 1 sec */
		for (int i = 0; i < frames; ++i)
			NDS_exec<false>();
	}

	sndifwork.xfs_load = true;
	//CommonSettings.spu_advanced = true;
	//CommonSettings.advanced_timing = false;

	return XSFPlayer::Load();
}

void XSFPlayer_2SF::GenerateSamples(std::vector<uint8_t> &buf, unsigned offset, unsigned samples)
{
	static const double HBASE_CYCLES = 33509300.322234;
	static const int HLINE_CYCLES = 6 * (99 + 256);
	static const uint32_t HSAMPLES = static_cast<uint32_t>(static_cast<double>(this->sampleRate * HLINE_CYCLES) / HBASE_CYCLES);
	static const int VDIVISION = 100;
	static const int VLINES = 263;
	static const double VBASE_CYCLES = HBASE_CYCLES / VDIVISION;
	static const uint32_t VSAMPLES = static_cast<uint32_t>(static_cast<double>(this->sampleRate * HLINE_CYCLES * VLINES) / HBASE_CYCLES);

	if (!sndifwork.xfs_load)
		return;
	unsigned bytes = samples << 2;
	while (bytes)
	{
		unsigned remainbytes = sndifwork.filled - sndifwork.used;
		if (remainbytes > 0)
		{
			if (remainbytes > bytes)
			{
				memcpy(&buf[offset], &sndifwork.buf[sndifwork.used], bytes);
				sndifwork.used += bytes;
				offset += bytes;
				remainbytes -= bytes;
				bytes = 0;
				break;
			}
			else
			{
				memcpy(&buf[offset], &sndifwork.buf[sndifwork.used], remainbytes);
				sndifwork.used += remainbytes;
				offset += remainbytes;
				bytes -= remainbytes;
				remainbytes = 0;
			}
		}
		if (!remainbytes)
		{
			if (sndifwork.sync_type == 1)
			{
				/* vsync */
				sndifwork.cycles += (this->sampleRate / VDIVISION) * HLINE_CYCLES * VLINES;
				if (sndifwork.cycles >= static_cast<uint32_t>(VBASE_CYCLES * (VSAMPLES + 1)))
					sndifwork.cycles -= static_cast<uint32_t>(VBASE_CYCLES * (VSAMPLES + 1));
				else
					sndifwork.cycles -= static_cast<uint32_t>(VBASE_CYCLES * VSAMPLES);
			}
			else
			{
				/* hsync */
				sndifwork.cycles += this->sampleRate * HLINE_CYCLES;
				if (sndifwork.cycles >= static_cast<uint32_t>(HBASE_CYCLES * (HSAMPLES + 1)))
					sndifwork.cycles -= static_cast<uint32_t>(HBASE_CYCLES * (HSAMPLES + 1));
				else
					sndifwork.cycles -= static_cast<uint32_t>(HBASE_CYCLES * HSAMPLES);
			}
			NDS_exec<false>();
			SPU_Emulate_user();
		}
	}
}

void XSFPlayer_2SF::Terminate()
{
	MMU_unsetRom();
	NDS_DeInit();

	this->rom.clear();
}
