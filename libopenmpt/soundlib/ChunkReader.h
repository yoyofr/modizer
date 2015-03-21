/*
 * ChunkReader.h
 * -------------
 * Purpose: An extended FileReader to read Iff-like chunk-based file structures.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "../common/FileReader.h"

#include <vector>


OPENMPT_NAMESPACE_BEGIN


//===================================
class ChunkReader : public FileReader
//===================================
{
public:

	ChunkReader(const char *data, size_t length) : FileReader(data, length) { }
	ChunkReader(const FileReader &other) : FileReader(other) { }

	template<typename T>
	//=================
	class ChunkListItem
	//=================
	{
	private:
		T chunkHeader;
		FileReader chunkData;

	public:
		ChunkListItem(const T &header, const FileReader &data) : chunkHeader(header), chunkData(data) { }

		ChunkListItem<T> &operator= (const ChunkListItem<T> &other)
		{ 
			chunkHeader = other.chunkHeader;
			chunkData = other.chunkData;
			return *this;
		}

		const T &GetHeader() const { return chunkHeader; }
		const FileReader &GetData() const { return chunkData; }
	};

	template<typename T>
	//=====================================================
	class ChunkList : public std::vector<ChunkListItem<T> >
	//=====================================================
	{
	public:

		// Check if the list contains a given chunk.
		bool ChunkExists(typename T::id_type id) const
		{
			for(typename std::vector<ChunkListItem<T> >::const_iterator iter = this->begin(); iter != this->end(); iter++)
			{
				if(iter->GetHeader().GetID() == id)
				{
					return true;
				}
			}
			return false;
		}

		// Retrieve the first chunk with a given ID.
		FileReader GetChunk(typename T::id_type id) const
		{
			for(typename std::vector<ChunkListItem<T> >::const_iterator iter = this->begin(); iter != this->end(); iter++)
			{
				if(iter->GetHeader().GetID() == id)
				{
					return iter->GetData();
				}
			}
			return FileReader();
		}

		// Retrieve all chunks with a given ID.
		std::vector<FileReader> GetAllChunks(typename T::id_type id) const
		{
			std::vector<FileReader> result;
			for(typename std::vector<ChunkListItem<T> >::const_iterator iter = this->begin(); iter != this->end(); iter++)
			{
				if(iter->GetHeader().GetID() == id)
				{
					result.push_back(iter->GetData());
				}
			}
			return result;
		}
	};


	// Read a series of "T" chunks until the end of file is reached.
	// T is required to have the methods GetID() and GetLength(), as well as an id_type typedef.
	// GetLength() should return the chunk size in bytes, and GetID() the chunk ID.
	// id_type must reflect the type that is returned by GetID().
	template<typename T>
	ChunkList<T> ReadChunks(size_t padding)
	{
		ChunkList<T> result;
		while(AreBytesLeft())
		{
			T chunkHeader;
			if(!Read(chunkHeader))
			{
				break;
			}

			size_t dataSize = chunkHeader.GetLength();
			ChunkListItem<T> resultItem(chunkHeader, ReadChunk(dataSize));
			result.push_back(resultItem);

			// Skip padding bytes
			if(padding != 0 && dataSize % padding != 0)
			{
				Skip(padding - (dataSize % padding));
			}
		}

		return result;
	}
};


OPENMPT_NAMESPACE_END
