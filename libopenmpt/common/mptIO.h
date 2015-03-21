/*
 * mptIO.h
 * -------
 * Purpose: Basic functions for reading/writing binary and endian safe data to/from files/streams.
 * Notes  : This is work-in-progress.
 *          Some useful functions for reading and writing are still missing.
 * Authors: Joern Heusipp
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once


#include "../common/typedefs.h"
#include "../common/Endianness.h"
#include <algorithm>
#include <iosfwd>
#include <limits>
#if defined(HAS_TYPE_TRAITS)
#include <type_traits>
#endif
#include <cstring>

#if defined(MPT_WITH_FILEIO)
#include <cstdio>
#include <stdio.h>
#endif // MPT_WITH_FILEIO


OPENMPT_NAMESPACE_BEGIN


namespace mpt {

namespace IO {

typedef int64 Offset;



// Returns true iff 'off' fits into 'Toff'.
template < typename Toff >
inline bool OffsetFits(IO::Offset off)
{
	return (static_cast<IO::Offset>(mpt::saturate_cast<Toff>(off)) == off);
}



bool IsValid(std::ostream & f);
bool IsValid(std::istream & f);
bool IsValid(std::iostream & f);
IO::Offset TellRead(std::istream & f);
IO::Offset TellWrite(std::ostream & f);
bool SeekBegin(std::ostream & f);
bool SeekBegin(std::istream & f);
bool SeekBegin(std::iostream & f);
bool SeekEnd(std::ostream & f);
bool SeekEnd(std::istream & f);
bool SeekEnd(std::iostream & f);
bool SeekAbsolute(std::ostream & f, IO::Offset pos);
bool SeekAbsolute(std::istream & f, IO::Offset pos);
bool SeekAbsolute(std::iostream & f, IO::Offset pos);
bool SeekRelative(std::ostream & f, IO::Offset off);
bool SeekRelative(std::istream & f, IO::Offset off);
bool SeekRelative(std::iostream & f, IO::Offset off);
IO::Offset ReadRaw(std::istream & f, uint8 * data, std::size_t size);
IO::Offset ReadRaw(std::istream & f, char * data, std::size_t size);
IO::Offset ReadRaw(std::istream & f, void * data, std::size_t size);
bool WriteRaw(std::ostream & f, const uint8 * data, std::size_t size);
bool WriteRaw(std::ostream & f, const char * data, std::size_t size);
bool WriteRaw(std::ostream & f, const void * data, std::size_t size);
bool IsEof(std::istream & f);
bool Flush(std::ostream & f);



#if defined(MPT_WITH_FILEIO)

// FILE* only makes sense if we support filenames at all.

bool IsValid(FILE* & f);
IO::Offset TellRead(FILE* & f);
IO::Offset TellWrite(FILE* & f);
bool SeekBegin(FILE* & f);
bool SeekEnd(FILE* & f);
bool SeekAbsolute(FILE* & f, IO::Offset pos);
bool SeekRelative(FILE* & f, IO::Offset off);
IO::Offset ReadRaw(FILE * & f, uint8 * data, std::size_t size);
IO::Offset ReadRaw(FILE * & f, char * data, std::size_t size);
IO::Offset ReadRaw(FILE * & f, void * data, std::size_t size);
bool WriteRaw(FILE* & f, const uint8 * data, std::size_t size);
bool WriteRaw(FILE* & f, const char * data, std::size_t size);
bool WriteRaw(FILE* & f, const void * data, std::size_t size);
bool IsEof(FILE * & f);
bool Flush(FILE* & f);

#endif // MPT_WITH_FILEIO



template <typename Tbinary, typename Tfile>
inline bool Read(Tfile & f, Tbinary & v)
{
	return IO::ReadRaw(f, mpt::GetRawBytes(v), sizeof(Tbinary)) == sizeof(Tbinary);
}

template <typename Tbinary, typename Tfile>
inline bool Write(Tfile & f, const Tbinary & v)
{
	return IO::WriteRaw(f, mpt::GetRawBytes(v), sizeof(Tbinary));
}

template <typename T, typename Tfile>
inline bool ReadBinaryTruncatedLE(Tfile & f, T & v, std::size_t size)
{
	bool result = false;
	#ifdef HAS_TYPE_TRAITS
		static_assert(std::is_trivial<T>::value == true, "");
	#endif
	uint8 bytes[sizeof(T)];
	std::memset(bytes, 0, sizeof(T));
	const IO::Offset readResult = IO::ReadRaw(f, bytes, std::min(size, sizeof(T)));
	if(readResult < 0)
	{
		result = false;
	} else
	{
		result = (static_cast<uint64>(readResult) == std::min(size, sizeof(T)));
	}
	#ifdef MPT_PLATFORM_BIG_ENDIAN
		std::reverse(bytes, bytes + sizeof(T));
	#endif
	std::memcpy(&v, bytes, sizeof(T));
	return result;
}

template <typename T, typename Tfile>
inline bool ReadIntLE(Tfile & f, T & v)
{
	bool result = false;
	STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	uint8 bytes[sizeof(T)];
	std::memset(bytes, 0, sizeof(T));
	const IO::Offset readResult = IO::ReadRaw(f, bytes, sizeof(T));
	if(readResult < 0)
	{
		result = false;
	} else
	{
		result = (static_cast<uint64>(readResult) == sizeof(T));
	}
	T val = 0;
	std::memcpy(&val, bytes, sizeof(T));
	v = SwapBytesReturnLE(val);
	return result;
}

template <typename T, typename Tfile>
inline bool ReadIntBE(Tfile & f, T & v)
{
	bool result = false;
	STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	uint8 bytes[sizeof(T)];
	std::memset(bytes, 0, sizeof(T));
	const IO::Offset readResult = IO::ReadRaw(f, bytes, sizeof(T));
	if(readResult < 0)
	{
		result = false;
	} else
	{
		result = (static_cast<uint64>(readResult) == sizeof(T));
	}
	T val = 0;
	std::memcpy(&val, bytes, sizeof(T));
	v = SwapBytesReturnBE(val);
	return result;
}

template <typename Tfile>
inline bool ReadAdaptiveInt16LE(Tfile & f, uint16 & v)
{
	bool result = true;
	uint8 byte = 0;
	std::size_t additionalBytes = 0;
	v = 0;
	byte = 0;
	if(!IO::ReadIntLE<uint8>(f, byte)) result = false;
	additionalBytes = (byte & 0x01);
	v = byte >> 1;
	for(std::size_t i = 0; i < additionalBytes; ++i)
	{
		byte = 0;
		if(!IO::ReadIntLE<uint8>(f, byte)) result = false;
		v |= (static_cast<uint16>(byte) << (((i+1)*8) - 1));
	}
	return result;
}

template <typename Tfile>
inline bool ReadAdaptiveInt32LE(Tfile & f, uint32 & v)
{
	bool result = true;
	uint8 byte = 0;
	std::size_t additionalBytes = 0;
	v = 0;
	byte = 0;
	if(!IO::ReadIntLE<uint8>(f, byte)) result = false;
	additionalBytes = (byte & 0x03);
	v = byte >> 2;
	for(std::size_t i = 0; i < additionalBytes; ++i)
	{
		byte = 0;
		if(!IO::ReadIntLE<uint8>(f, byte)) result = false;
		v |= (static_cast<uint32>(byte) << (((i+1)*8) - 2));
	}
	return result;
}

template <typename Tfile>
inline bool ReadAdaptiveInt64LE(Tfile & f, uint64 & v)
{
	bool result = true;
	uint8 byte = 0;
	std::size_t additionalBytes = 0;
	v = 0;
	byte = 0;
	if(!IO::ReadIntLE<uint8>(f, byte)) result = false;
	additionalBytes = (1 << (byte & 0x03)) - 1;
	v = byte >> 2;
	for(std::size_t i = 0; i < additionalBytes; ++i)
	{
		byte = 0;
		if(!IO::ReadIntLE<uint8>(f, byte)) result = false;
		v |= (static_cast<uint64>(byte) << (((i+1)*8) - 2));
	}
	return result;
}

template <typename Tsize, typename Tfile>
inline bool ReadSizedStringLE(Tfile & f, std::string & str, Tsize maxSize = std::numeric_limits<Tsize>::max())
{
	STATIC_ASSERT(std::numeric_limits<Tsize>::is_integer);
	str.clear();
	Tsize size = 0;
	if(!mpt::IO::ReadIntLE(f, size))
	{
		return false;
	}
	if(size > maxSize)
	{
		return false;
	}
	for(Tsize i = 0; i != size; ++i)
	{
		char c = '\0';
		if(!mpt::IO::ReadIntLE(f, c))
		{
			return false;
		}
		str.push_back(c);
	}
	return true;
}


template <typename T, typename Tfile>
inline bool WriteIntLE(Tfile & f, const T v)
{
	STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	const T val = SwapBytesReturnLE(v);
	uint8 bytes[sizeof(T)];
	std::memcpy(bytes, &val, sizeof(T));
	return IO::WriteRaw(f, bytes, sizeof(T));
}

template <typename T, typename Tfile>
inline bool WriteIntBE(Tfile & f, const T v)
{
	STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	const T val = SwapBytesReturnBE(v);
	uint8 bytes[sizeof(T)];
	std::memcpy(bytes, &val, sizeof(T));
	return IO::WriteRaw(f, bytes, sizeof(T));
}

template <typename Tfile>
inline bool WriteAdaptiveInt16LE(Tfile & f, const uint16 v, std::size_t minSize = 0, std::size_t maxSize = 0)
{
	MPT_ASSERT(minSize == 0 || minSize == 1 || minSize == 2);
	MPT_ASSERT(maxSize == 0 || maxSize == 1 || maxSize == 2);
	MPT_ASSERT(maxSize == 0 || maxSize >= minSize);
	if(v < 0x80 && minSize <= 1 && (1 <= maxSize || maxSize == 0))
	{
		return IO::WriteIntLE<uint8>(f, static_cast<uint8>(v << 1) | 0x00);
	} else if(v < 0x8000 && minSize <= 2 && (2 <= maxSize || maxSize == 0))
	{
		return IO::WriteIntLE<uint16>(f, static_cast<uint16>(v << 1) | 0x01);
	} else
	{
		MPT_ASSERT(false);
		return false;
	}
}

template <typename Tfile>
inline bool WriteAdaptiveInt32LE(Tfile & f, const uint32 v, std::size_t minSize = 0, std::size_t maxSize = 0)
{
	MPT_ASSERT(minSize == 0 || minSize == 1 || minSize == 2 || minSize == 3 || minSize == 4);
	MPT_ASSERT(maxSize == 0 || maxSize == 1 || maxSize == 2 || maxSize == 3 || maxSize == 4);
	MPT_ASSERT(maxSize == 0 || maxSize >= minSize);
	if(v < 0x40 && minSize <= 1 && (1 <= maxSize || maxSize == 0))
	{
		return IO::WriteIntLE<uint8>(f, static_cast<uint8>(v << 2) | 0x00);
	} else if(v < 0x4000 && minSize <= 2 && (2 <= maxSize || maxSize == 0))
	{
		return IO::WriteIntLE<uint16>(f, static_cast<uint16>(v << 2) | 0x01);
	} else if(v < 0x400000 && minSize <= 3 && (3 <= maxSize || maxSize == 0))
	{
		uint32 value = static_cast<uint32>(v << 2) | 0x02;
		uint8 bytes[3];
		bytes[0] = static_cast<uint8>(value >>  0);
		bytes[1] = static_cast<uint8>(value >>  8);
		bytes[2] = static_cast<uint8>(value >> 16);
		return IO::WriteRaw(f, bytes, 3);
	} else if(v < 0x40000000 && minSize <= 4 && (4 <= maxSize || maxSize == 0))
	{
		return IO::WriteIntLE<uint32>(f, static_cast<uint32>(v << 2) | 0x03);
	} else
	{
		MPT_ASSERT(false);
		return false;
	}
}

template <typename Tfile>
inline bool WriteAdaptiveInt64LE(Tfile & f, const uint64 v, std::size_t minSize = 0, std::size_t maxSize = 0)
{
	MPT_ASSERT(minSize == 0 || minSize == 1 || minSize == 2 || minSize == 4 || minSize == 8);
	MPT_ASSERT(maxSize == 0 || maxSize == 1 || maxSize == 2 || maxSize == 4 || maxSize == 8);
	MPT_ASSERT(maxSize == 0 || maxSize >= minSize);
	if(v < 0x40 && minSize <= 1 && (1 <= maxSize || maxSize == 0))
	{
		return IO::WriteIntLE<uint8>(f, static_cast<uint8>(v << 2) | 0x00);
	} else if(v < 0x4000 && minSize <= 2 && (2 <= maxSize || maxSize == 0))
	{
		return IO::WriteIntLE<uint16>(f, static_cast<uint16>(v << 2) | 0x01);
	} else if(v < 0x40000000 && minSize <= 4 && (4 <= maxSize || maxSize == 0))
	{
		return IO::WriteIntLE<uint32>(f, static_cast<uint32>(v << 2) | 0x02);
	} else if(v < 0x4000000000000000ull && minSize <= 8 && (8 <= maxSize || maxSize == 0))
	{
		return IO::WriteIntLE<uint64>(f, static_cast<uint64>(v << 2) | 0x03);
	} else
	{
		MPT_ASSERT(false);
		return false;
	}
}

// Write a variable-length integer, as found in MIDI files. The number of written bytes is placed in the bytesWritten parameter.
template <typename Tfile, typename T>
bool WriteVarInt(Tfile & f, const T v, size_t *bytesWritten = nullptr)
{
	STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	STATIC_ASSERT(!std::numeric_limits<T>::is_signed);

	uint8 out[(sizeof(T) * 8 + 6) / 7];
	size_t numBytes = 0;
	for(uint32 n = (sizeof(T) * 8) / 7; n > 0; n--)
	{
		if(v >= (static_cast<T>(1) << (n * 7u)))
		{
			out[numBytes++] = static_cast<uint8>(((v >> (n * 7u)) & 0x7F) | 0x80);
		}
	}
	out[numBytes++] = static_cast<uint8>(v & 0x7F);
	MPT_ASSERT(numBytes <= CountOf(out));
	if(bytesWritten != nullptr) *bytesWritten = numBytes;
	return mpt::IO::WriteRaw(f, out, numBytes);
}

template <typename Tsize, typename Tfile>
inline bool WriteSizedStringLE(Tfile & f, const std::string & str)
{
	STATIC_ASSERT(std::numeric_limits<Tsize>::is_integer);
	if(str.size() > std::numeric_limits<Tsize>::max())
	{
		return false;
	}
	Tsize size = static_cast<Tsize>(str.size());
	if(!mpt::IO::WriteIntLE(f, size))
	{
		return false;
	}
	if(!mpt::IO::WriteRaw(f, str.data(), str.size()))
	{
		return false;
	}
	return true;
}

template <typename T, typename Tfile>
inline bool WriteConvertEndianness(Tfile & f, T & v)
{
	v.ConvertEndianness();
	bool result = IO::WriteRaw(f, reinterpret_cast<const uint8 *>(&v), sizeof(T));
	v.ConvertEndianness();
	return result;
}

} // namespace IO


} // namespace mpt



#if defined(MPT_FILEREADER_STD_ISTREAM)

class IFileDataContainer {
public:
	typedef std::size_t off_t;
protected:
	IFileDataContainer() { }
public:
	virtual ~IFileDataContainer() { }
public:
	virtual bool IsValid() const = 0;
	virtual const char *GetRawData() const = 0;
	virtual off_t GetLength() const = 0;
	virtual off_t Read(char *dst, off_t pos, off_t count) const = 0;

	virtual const char *GetPartialRawData(off_t pos, off_t length) const // DO NOT USE!!! this is just for ReadMagic ... the pointer returned may be invalid after the next Read()
	{
		if(pos + length > GetLength())
		{
			return nullptr;
		}
		return GetRawData() + pos;
	}

	virtual bool CanRead(off_t pos, off_t length) const
	{
		return pos + length <= GetLength();
	}

	virtual off_t GetReadableLength(off_t pos, off_t length) const
	{
		if(pos >= GetLength())
		{
			return 0;
		}
		return std::min<off_t>(length, GetLength() - pos);
	}
};


class FileDataContainerDummy : public IFileDataContainer {
public:
	FileDataContainerDummy() { }
	virtual ~FileDataContainerDummy() { }
public:
	bool IsValid() const
	{
		return false;
	}

	const char *GetRawData() const
	{
		return nullptr;
	}

	off_t GetLength() const
	{
		return 0;
	}
	off_t Read(char * /*dst*/, off_t /*pos*/, off_t /*count*/) const
	{
		return 0;
	}
};


class FileDataContainerWindow : public IFileDataContainer
{
private:
	MPT_SHARED_PTR<IFileDataContainer> data;
	const off_t dataOffset;
	const off_t dataLength;
public:
	FileDataContainerWindow(MPT_SHARED_PTR<IFileDataContainer> src, off_t off, off_t len) : data(src), dataOffset(off), dataLength(len) { }
	virtual ~FileDataContainerWindow() { }

	bool IsValid() const
	{
		return data->IsValid();
	}
	const char *GetRawData() const {
		return data->GetRawData() + dataOffset;
	}
	off_t GetLength() const {
		return dataLength;
	}
	off_t Read(char *dst, off_t pos, off_t count) const
	{
		if(pos >= dataLength)
		{
			return 0;
		}
		return data->Read(dst, dataOffset + pos, std::min(count, dataLength - pos));
	}
	const char *GetPartialRawData(off_t pos, off_t length) const
	{
		if(pos + length > dataLength)
		{
			return nullptr;
		}
		return data->GetPartialRawData(dataOffset + pos, length);
	}
	bool CanRead(off_t pos, off_t length) const {
		return (pos + length <= dataLength);
	}
	off_t GetReadableLength(off_t pos, off_t length) const
	{
		if(pos >= dataLength)
		{
			return 0;
		}
		return std::min(length, dataLength - pos);
	}
};


class FileDataContainerStdStream : public IFileDataContainer {

private:

	mutable std::vector<char> cache;
	mutable bool streamFullyCached;

	std::istream *stream;

public:

	FileDataContainerStdStream(std::istream *s);
	virtual ~FileDataContainerStdStream();

private:

	static const std::size_t buffer_size = 65536;

	void CacheStream() const;
	void CacheStreamUpTo(std::streampos pos) const;

private:

	void ReadCached(char *dst, off_t pos, off_t count) const;

public:

	bool IsValid() const;
	const char *GetRawData() const;
	off_t GetLength() const;
	off_t Read(char *dst, off_t pos, off_t count) const;
	const char *GetPartialRawData(off_t pos, off_t length) const;
	bool CanRead(off_t pos, off_t length) const;
	off_t GetReadableLength(off_t pos, off_t length) const;

};

#endif


class FileDataContainerMemory
#if defined(MPT_FILEREADER_STD_ISTREAM)
	: public IFileDataContainer
#endif
{

#if !defined(MPT_FILEREADER_STD_ISTREAM)
public:
	typedef std::size_t off_t;
#endif

private:

	const char *streamData;	// Pointer to memory-mapped file
	off_t streamLength;		// Size of memory-mapped file in bytes

public:
	FileDataContainerMemory() : streamData(nullptr), streamLength(0) { }
	FileDataContainerMemory(const char *data, off_t length) : streamData(data), streamLength(length) { }
#if defined(MPT_FILEREADER_STD_ISTREAM)
	virtual
#endif
		~FileDataContainerMemory() { }

public:

	bool IsValid() const
	{
		return streamData != nullptr;
	}

	const char *GetRawData() const
	{
		return streamData;
	}

	off_t GetLength() const
	{
		return streamLength;
	}

	off_t Read(char *dst, off_t pos, off_t count) const
	{
		if(pos >= streamLength)
		{
			return 0;
		}
		off_t avail = std::min<off_t>(streamLength - pos, count);
		std::copy(streamData + pos, streamData + pos + avail, dst);
		return avail;
	}

	const char *GetPartialRawData(off_t pos, off_t length) const
	{
		if(pos + length > streamLength)
		{
			return nullptr;
		}
		return streamData + pos;
	}

	bool CanRead(off_t pos, off_t length) const
	{
		return pos + length <= streamLength;
	}

	off_t GetReadableLength(off_t pos, off_t length) const
	{
		if(pos >= streamLength)
		{
			return 0;
		}
		return std::min<off_t>(length, streamLength - pos);
	}

};



OPENMPT_NAMESPACE_END
