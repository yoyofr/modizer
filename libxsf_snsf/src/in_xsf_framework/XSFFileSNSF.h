/*
 * xSF - File structure
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2014-09-24
 *
 * Partially based on the vio*sf framework
 */

#pragma once

#include <cstdint>
#include "convert.h"
#include "TagListSNSF.h"

enum VolumeTypeSNSF
{
	VOLUMETYPESNSF_NONE,
	VOLUMETYPESNSF_VOLUME,
	VOLUMETYPESNSF_REPLAYGAIN_TRACK,
	VOLUMETYPESNSF_REPLAYGAIN_ALBUM
};

enum PeakTypeSNSF
{
	PEAKTYPESNSF_NONE,
	PEAKTYPESNSF_REPLAYGAIN_TRACK,
	PEAKTYPESNSF_REPLAYGAIN_ALBUM
};

class XSFFileSNSF
{
protected:
	uint8_t xSFType;
	bool hasFile;
	std::vector<uint8_t> rawData, reservedSection, programSection;
	TagListSNSF tags;
	std::string fileName;
	void ReadXSF(const std::string &filename, uint32_t programSizeOffset, uint32_t programHeaderSize, bool readTagsOnly = false);
	void ReadXSF(std::ifstream &xSF, uint32_t programSizeOffset, uint32_t programHeaderSize, bool readTagsOnly = false);
	std::string FormattedTitleOptionalBlock(const std::string &block, bool &hadReplacement, unsigned level) const;
public:
	XSFFileSNSF();
	XSFFileSNSF(const std::string &filename);
	XSFFileSNSF(const std::string &filename, uint32_t programSizeOffset, uint32_t programHeaderSize);
	bool IsValidType(uint8_t type) const;
	void Clear();
	bool HasFile() const;
	std::vector<uint8_t> &GetReservedSection();
	std::vector<uint8_t> GetReservedSection() const;
	std::vector<uint8_t> &GetProgramSection();
	std::vector<uint8_t> GetProgramSection() const;
	const TagListSNSF &GetAllTags() const;
	void SetAllTags(const TagListSNSF &newTags);
	void SetTag(const std::string &name, const std::string &value);
	void SetTag(const std::string &name, const std::wstring &value);
	bool GetTagExists(const std::string &name) const;
	std::string GetTagValue(const std::string &name) const;
	template<typename T> T GetTagValue(const std::string &name, const T &defaultValue) const
	{
		return this->GetTagExists(name) ? convertTo<T>(this->GetTagValue(name), false) : defaultValue;
	}
	unsigned long GetLengthMS(unsigned long defaultLength) const;
	unsigned long GetFadeMS(unsigned long defaultFade) const;
	double GetVolume(VolumeTypeSNSF preferredVolumeType, PeakTypeSNSF preferredPeakType) const;
	std::string GetFormattedTitle(const std::string &format) const;
	std::string GetFilename() const;
	std::string GetFilenameWithoutPath() const;
	void SaveFile() const;
};
