/*
 * unarchiver.cpp
 * --------------
 * Purpose: archive loader
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"

#include "unarchiver.h"
#include "../common/FileReader.h"


OPENMPT_NAMESPACE_BEGIN


CUnarchiver::CUnarchiver(FileReader &file)
	: inFile(file)
	, emptyArchive(inFile)
#if (defined(MPT_WITH_ZLIB) && defined(MPT_WITH_MINIZIP)) || defined(MPT_WITH_MINIZ)
	, zipArchive(inFile)
#endif
#ifdef MPT_WITH_LHASA
	, lhaArchive(inFile)
#endif
#if defined(MPT_WITH_ZLIB) || defined(MPT_WITH_MINIZ)
	, gzipArchive(inFile)
#endif
#ifdef MPT_WITH_UNRAR
	, rarArchive(inFile)
#endif
#ifdef MPT_WITH_ANCIENT
	, ancientArchive(inFile)
#endif
{
	inFile.Rewind();
#if (defined(MPT_WITH_ZLIB) && defined(MPT_WITH_MINIZIP)) || defined(MPT_WITH_MINIZ)
	if(zipArchive.IsArchive()) { impl = &zipArchive; return; }
#endif
#ifdef MPT_WITH_LHASA
	if(lhaArchive.IsArchive()) { impl = &lhaArchive; return; }
#endif
#if defined(MPT_WITH_ZLIB) || defined(MPT_WITH_MINIZ)
	if(gzipArchive.IsArchive()) { impl = &gzipArchive; return; }
#endif
#ifdef MPT_WITH_UNRAR
	if(rarArchive.IsArchive()) { impl = &rarArchive; return; }
#endif
#ifdef MPT_WITH_ANCIENT
	if(ancientArchive.IsArchive()) { impl = &ancientArchive; return; }
#endif
	impl = &emptyArchive;
}


CUnarchiver::~CUnarchiver()
{
	return;
}


static inline std::string GetExtension(const std::string &filename)
{
	if(filename.find_last_of(".") != std::string::npos)
	{
		return mpt::ToLowerCaseAscii(filename.substr(filename.find_last_of(".") + 1));
	}
	return std::string();
}


std::size_t CUnarchiver::FindBestFile(const std::vector<const char *> &extensions)
{
	if(!IsArchive())
	{
		return failIndex;
	}
	uint64 biggestSize = 0;
	std::size_t bestIndex = failIndex;
	for(std::size_t i = 0; i < size(); ++i)
	{
		if(operator[](i).type != ArchiveFileType::Normal)
		{
			continue;
		}
		const std::string ext = GetExtension(operator[](i).name.ToUTF8());

		if(ext == "diz" || ext == "nfo" || ext == "txt")
		{
			// we do not want these
			continue;
		}

		// Compare with list of preferred extensions
		if(mpt::contains(extensions, ext))
		{
			bestIndex = i;
			break;
		}

		if(operator[](i).size >= biggestSize)
		{
			biggestSize = operator[](i).size;
			bestIndex = i;
		}
	}
	return bestIndex;
}


bool CUnarchiver::ExtractBestFile(const std::vector<const char *> &extensions)
{
	std::size_t bestFile = FindBestFile(extensions);
	if(bestFile == failIndex)
	{
		return false;
	}
	return ExtractFile(bestFile);
}


bool CUnarchiver::IsArchive() const
{
	return impl->IsArchive();
}


mpt::ustring CUnarchiver::GetComment() const
{
	return impl->GetComment();
}


bool CUnarchiver::ExtractFile(std::size_t index)
{
	return impl->ExtractFile(index);
}


FileReader CUnarchiver::GetOutputFile() const
{
	return impl->GetOutputFile();
}


std::size_t CUnarchiver::size() const
{
	return impl->size();
}


IArchive::const_iterator CUnarchiver::begin() const
{
	return impl->begin();
}


IArchive::const_iterator CUnarchiver::end() const
{
	return impl->end();
}


const ArchiveFileInfo & CUnarchiver::operator [] (std::size_t index) const
{
	return impl->operator[](index);
}


OPENMPT_NAMESPACE_END
