/*
 * xSF - GSF Player
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2014-09-24
 *
 * Based on a modified viogsf v0.08
 *
 * Partially based on the vio*sf framework
 *
 * Utilizes a modified VBA-M, SVN revision 1102, for playback
 * http://vba-m.com/
 */

#include <memory>
#include <zlib.h>
#include "convert.h"
#include "XSFPlayer.h"
#include "XSFCommon.h"
#include "vbam/gba/Globals.h"
#include "vbam/gba/Sound.h"
#include "vbam/common/SoundDriver.h"

class XSFPlayer_GSF : public XSFPlayer
{
public:
	XSFPlayer_GSF(const std::string &filename);
#ifdef _WIN32
	XSFPlayer_GSF(const std::wstring &filename);
#endif
	~XSFPlayer_GSF() { this->Terminate(); }
	bool Load();
	void GenerateSamples(std::vector<uint8_t> &buf, unsigned offset, unsigned samples);
	void Terminate();
};

const char *XSFPlayer::WinampDescription = "GSF Decoder";
const char *XSFPlayer::WinampExts = "gsf;minigsf\0Game Boy Advance Sound Format files (*.gsf;*.minigsf)\0";

XSFPlayer *XSFPlayer::Create(const std::string &fn)
{
	return new XSFPlayer_GSF(fn);
}

#ifdef _WIN32
XSFPlayer *XSFPlayer::Create(const std::wstring &fn)
{
	return new XSFPlayer_GSF(fn);
}
#endif

static struct
{
	std::vector<uint8_t> rom;
	unsigned entry;
} loaderwork;

int mapgsf(uint8_t *d, int l, int &s)
{
	if (static_cast<size_t>(l) > loaderwork.rom.size())
		l = loaderwork.rom.size();
	if (l)
		memcpy(d, &loaderwork.rom[0], l);
	s = l;
	return l;
}

static struct
{
	std::vector<uint8_t> buf;
	uint32_t len, fil, cur;
} buffer = { std::vector<uint8_t>(), 0, 0, 0 };

class GSFSoundDriver : public SoundDriver
{
	void freebuffer()
	{
		buffer.buf.clear();
		buffer.len = buffer.fil = buffer.cur = 0;
	}
public:
	bool init(long sampleRate)
	{
		freebuffer();
		uint32_t len = (sampleRate / 10) << 2;
		buffer.buf.resize(len);
		buffer.len = len;
		return true;
	}

	void pause()
	{
	}

	void reset()
	{
	}

	void resume()
	{
	}

	void write(uint16_t *finalWave, int length)
	{
		if (static_cast<uint32_t>(length) > buffer.len - buffer.fil)
			length = buffer.len - buffer.fil;
		if (length > 0)
		{
			memcpy(&buffer.buf[buffer.fil], finalWave, length);
			buffer.fil += length;
		}
	}

	void setThrottle(unsigned short)
	{
	}
};

SoundDriver *systemSoundInit()
{
	return new GSFSoundDriver();
}

static void Map2SFSection(const std::vector<uint8_t> &section, int level)
{
	auto &data = loaderwork.rom;

	uint32_t entry = Get32BitsLE(&section[0]), offset = Get32BitsLE(&section[4]) & 0x1FFFFFF, size = Get32BitsLE(&section[8]), finalSize = size + offset;
	if (level == 1)
		loaderwork.entry = entry;
	finalSize = NextHighestPowerOf2(finalSize);
	if (data.empty())
		data.resize(finalSize + 10, 0);
	else if (data.size() < size + offset)
		data.resize(offset + finalSize + 10);
	memcpy(&data[offset], &section[12], size);
}

static bool Map2SF(XSFFile *xSF, int level)
{
	if (!xSF->IsValidType(0x22))
		return false;

	auto &programSection = xSF->GetProgramSection();

	if (!programSection.empty())
		Map2SFSection(programSection, level);

	return true;
}

static bool RecursiveLoad2SF(XSFFile *xSF, int level)
{
	if (level <= 10 && xSF->GetTagExists("_lib"))
	{
#ifdef _WIN32
		auto libxSF = std::unique_ptr<XSFFile>(new XSFFile(ConvertFuncs::StringToWString(ExtractDirectoryFromPath(xSF->GetFilename()) + xSF->GetTagValue("_lib")), 8, 12));
#else
		auto libxSF = std::unique_ptr<XSFFile>(new XSFFile(ExtractDirectoryFromPath(xSF->GetFilename()) + xSF->GetTagValue("_lib"), 8, 12));
#endif
		if (!RecursiveLoad2SF(libxSF.get(), level + 1))
			return false;
	}

	if (!Map2SF(xSF, level))
		return false;

	unsigned n = 2;
	bool found;
	do
	{
		found = false;
		std::string libTag = "_lib" + stringify(n++);
		if (xSF->GetTagExists(libTag))
		{
			found = true;
#ifdef _WIN32
			auto libxSF = std::unique_ptr<XSFFile>(new XSFFile(ConvertFuncs::StringToWString(ExtractDirectoryFromPath(xSF->GetFilename()) + xSF->GetTagValue(libTag)), 8, 12));
#else
			auto libxSF = std::unique_ptr<XSFFile>(new XSFFile(ExtractDirectoryFromPath(xSF->GetFilename()) + xSF->GetTagValue(libTag), 8, 12));
#endif
			if (!RecursiveLoad2SF(libxSF.get(), level + 1))
				return false;
		}
	} while (found);

	return true;
}

static bool Load2SF(XSFFile *xSF)
{
	loaderwork.rom.clear();
	loaderwork.entry = 0;

	return RecursiveLoad2SF(xSF, 1);
}

XSFPlayer_GSF::XSFPlayer_GSF(const std::string &filename) : XSFPlayer()
{
	this->xSF.reset(new XSFFile(filename, 8, 12));
}

#ifdef _WIN32
XSFPlayer_GSF::XSFPlayer_GSF(const std::wstring &filename) : XSFPlayer()
{
	this->xSF.reset(new XSFFile(filename, 8, 12));
}
#endif

bool XSFPlayer_GSF::Load()
{
	if (!Load2SF(this->xSF.get()))
		return false;

	cpuIsMultiBoot = (loaderwork.entry >> 24) == 2;

	CPULoadRom();

	soundSetSampleRate(this->sampleRate);
	soundInit();
	soundReset();
	soundSetEnable(0x30F);

	CPUInit();
	CPUReset();

	return XSFPlayer::Load();
}

void XSFPlayer_GSF::GenerateSamples(std::vector<uint8_t> &buf, unsigned offset, unsigned samples)
{
	unsigned bytes = samples << 2;
	while (bytes)
	{
		unsigned remainbytes = buffer.fil - buffer.cur;
		while (!remainbytes)
		{
			buffer.cur = buffer.fil = 0;
			CPULoop(250000);

			remainbytes = buffer.fil - buffer.cur;
		}
		unsigned len = remainbytes;
		if (len > bytes)
			len = bytes;
		memcpy(&buf[offset], &buffer.buf[buffer.cur], len);
		bytes -= len;
		offset += len;
		buffer.cur += len;
	}
}

void XSFPlayer_GSF::Terminate()
{
	soundShutdown();

	loaderwork.rom.clear();
	loaderwork.entry = 0;
}
