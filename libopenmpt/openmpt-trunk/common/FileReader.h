/*
 * FileReader.h
 * ------------
 * Purpose: A basic class for transparent reading of memory-based files.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once


#include "typedefs.h"
#include "mptTypeTraits.h"
#include "StringFixer.h"
#include "misc_util.h"
#include "Endianness.h"
#include "mptIO.h"
#include <algorithm>
#include <limits>
#include <vector>
#include <cstring>


OPENMPT_NAMESPACE_BEGIN


// change to show warnings for functions which trigger pre-caching the whole file for unseekable streams
//#define FILEREADER_DEPRECATED MPT_DEPRECATED
#define FILEREADER_DEPRECATED


//==============
class FileReader
//==============
{

public:

#if defined(MPT_FILEREADER_STD_ISTREAM)
	typedef IFileDataContainer::off_t off_t;
#else
	typedef FileDataContainerMemory::off_t off_t;
#endif

private:

#if defined(MPT_FILEREADER_STD_ISTREAM)
	const IFileDataContainer & DataContainer() const { return *data; }
	IFileDataContainer & DataContainer() { return *data; }
	std::shared_ptr<IFileDataContainer> data;
#else
	const FileDataContainerMemory & DataContainer() const { return data; }
	FileDataContainerMemory & DataContainer() { return data; }
	FileDataContainerMemory data;
#endif

	off_t streamPos;		// Cursor location in the file

#if defined(MPT_ENABLE_FILEIO)
	const mpt::PathString *fileName;  // Filename that corresponds to this FileReader. It is only set if this FileReader represents the whole contents of fileName. May be nullptr.
	#define MPT_FILEREADER_INIT_FILENAME ,fileName(nullptr)
#else // !MPT_ENABLE_FILEIO
	#define MPT_FILEREADER_INIT_FILENAME
#endif // MPT_ENABLE_FILEIO

public:

#if defined(MPT_FILEREADER_STD_ISTREAM)

	// Initialize invalid file reader object.
	FileReader() : data(std::make_shared<FileDataContainerDummy>()), streamPos(0) MPT_FILEREADER_INIT_FILENAME { }

	// Initialize file reader object with pointer to data and data length.
	FileReader(mpt::span<const mpt::byte> bytedata) : data(std::make_shared<FileDataContainerMemory>(bytedata)), streamPos(0) MPT_FILEREADER_INIT_FILENAME { }
#ifdef MODPLUG_TRACKER
	FileReader(const char *chardata, off_t length) : data(std::make_shared<FileDataContainerMemory>(mpt::as_span(mpt::byte_cast<const mpt::byte *>(chardata), length))), streamPos(0) MPT_FILEREADER_INIT_FILENAME { }
	FileReader(const uint8 *uint8data, off_t length) : data(std::make_shared<FileDataContainerMemory>(mpt::as_span(mpt::byte_cast<const mpt::byte *>(uint8data), length))), streamPos(0) MPT_FILEREADER_INIT_FILENAME { }
#endif // MODPLUG_TRACKER
#if defined(MPT_ENABLE_FILEIO)
	FileReader(mpt::span<const mpt::byte> bytedata, const mpt::PathString *filename) : data(std::make_shared<FileDataContainerMemory>(bytedata)), streamPos(0), fileName(filename) { }
#ifdef MODPLUG_TRACKER
	FileReader(const char *chardata, off_t length, const mpt::PathString *filename) : data(std::make_shared<FileDataContainerMemory>(mpt::as_span(mpt::byte_cast<const mpt::byte *>(chardata), length))), streamPos(0), fileName(filename) { }
	FileReader(const uint8 *uint8data, off_t length, const mpt::PathString *filename) : data(std::make_shared<FileDataContainerMemory>(mpt::as_span(mpt::byte_cast<const mpt::byte *>(uint8data), length))), streamPos(0), fileName(filename) { }
#endif // MODPLUG_TRACKER
#endif // MPT_ENABLE_FILEIO

#if defined(MPT_FILEREADER_CALLBACK_STREAM)
		// Initialize file reader object with a CallbackStream.
	FileReader(CallbackStream s)
		: data(
#if defined(MPT_FILEREADER_STD_ISTREAM_SEEKABLE)
				FileDataContainerCallbackStreamSeekable::IsSeekable(s) ?
					std::static_pointer_cast<IFileDataContainer>(std::make_shared<FileDataContainerCallbackStreamSeekable>(s))
				:
#endif // MPT_FILEREADER_STD_ISTREAM_SEEKABLE
					std::static_pointer_cast<IFileDataContainer>(std::make_shared<FileDataContainerCallbackStream>(s))
			)
		, streamPos(0)
		MPT_FILEREADER_INIT_FILENAME
	{
		return;
	}
#if defined(MPT_ENABLE_FILEIO)
	FileReader(CallbackStream s, const mpt::PathString *filename)
		: data(
#if defined(MPT_FILEREADER_STD_ISTREAM_SEEKABLE)
				FileDataContainerCallbackStreamSeekable::IsSeekable(s) ?
					std::static_pointer_cast<IFileDataContainer>(std::make_shared<FileDataContainerCallbackStreamSeekable>(s))
				:
#endif // MPT_FILEREADER_STD_ISTREAM_SEEKABLE
					std::static_pointer_cast<IFileDataContainer>(std::make_shared<FileDataContainerCallbackStream>(s))
			)
		, streamPos(0)
		, fileName(filename)
	{
		return;
	}
#endif // MPT_ENABLE_FILEIO
#endif // MPT_FILEREADER_CALLBACK_STREAM


	// Initialize file reader object with a std::istream.
	FileReader(std::istream *s)
		: data(
#if defined(MPT_FILEREADER_STD_ISTREAM_SEEKABLE)
				FileDataContainerStdStreamSeekable::IsSeekable(s) ?
					std::static_pointer_cast<IFileDataContainer>(std::make_shared<FileDataContainerStdStreamSeekable>(s))
				:
#endif // MPT_FILEREADER_STD_ISTREAM_SEEKABLE
					std::static_pointer_cast<IFileDataContainer>(std::make_shared<FileDataContainerStdStream>(s))
			)
		, streamPos(0)
		MPT_FILEREADER_INIT_FILENAME
	{
		return;
	}
#if defined(MPT_ENABLE_FILEIO)
	FileReader(std::istream *s, const mpt::PathString *filename)
		: data(
#if defined(MPT_FILEREADER_STD_ISTREAM_SEEKABLE)
				FileDataContainerStdStreamSeekable::IsSeekable(s) ?
					std::static_pointer_cast<IFileDataContainer>(std::make_shared<FileDataContainerStdStreamSeekable>(s))
				:
#endif // MPT_FILEREADER_STD_ISTREAM_SEEKABLE
					std::static_pointer_cast<IFileDataContainer>(std::make_shared<FileDataContainerStdStream>(s))
			)
		, streamPos(0)
		, fileName(filename)
	{
		return;
	}
#endif // MPT_ENABLE_FILEIO

	// Initialize file reader object based on an existing file reader object window.
	FileReader(std::shared_ptr<IFileDataContainer> other) : data(other), streamPos(0) MPT_FILEREADER_INIT_FILENAME { }

	// Initialize file reader object based on an existing file reader object. The other object's stream position is copied.
	FileReader(const FileReader &other) : data(other.data), streamPos(other.streamPos)
#if defined(MPT_ENABLE_FILEIO)
		, fileName(other.fileName)
#endif // MPT_ENABLE_FILEIO
	{ }

#else // !MPT_FILEREADER_STD_ISTREAM

	// Initialize invalid file reader object.
	FileReader() : data(mpt::span<const mpt::byte>()), streamPos(0) MPT_FILEREADER_INIT_FILENAME { }

	// Initialize file reader object with pointer to data and data length.
	FileReader(mpt::span<const mpt::byte> bytedata) : data(bytedata), streamPos(0) MPT_FILEREADER_INIT_FILENAME { }
#ifdef MODPLUG_TRACKER
	FileReader(const char *chardata, off_t length) : data(mpt::as_span(mpt::byte_cast<const mpt::byte *>(chardata), length)), streamPos(0) MPT_FILEREADER_INIT_FILENAME { }
	FileReader(const uint8 *uint8data, off_t length) : data(mpt::as_span(mpt::byte_cast<const mpt::byte *>(uint8data), length)), streamPos(0) MPT_FILEREADER_INIT_FILENAME { }
#endif // MODPLUG_TRACKER
#if defined(MPT_ENABLE_FILEIO)
	FileReader(mpt::span<const mpt::byte> bytedata, const mpt::PathString *filename) : data(bytedata), streamPos(0), fileName(filename) { }
#ifdef MODPLUG_TRACKER
	FileReader(const char *chardata, off_t length, const mpt::PathString *filename) : data(mpt::as_span(mpt::byte_cast<const mpt::byte *>(chardata), length)), streamPos(0), fileName(filename) { }
	FileReader(const uint8 *uint8data, off_t length, const mpt::PathString *filename) : data(mpt::as_span(mpt::byte_cast<const mpt::byte *>(uint8data), length)), streamPos(0), fileName(filename) { }
#endif // MODPLUG_TRACKER
#endif // MPT_ENABLE_FILEIO

	// Initialize file reader object based on an existing file reader object. The other object's stream position is copied.
	FileReader(const FileReader &other) : data(other.data), streamPos(other.streamPos)
#if defined(MPT_ENABLE_FILEIO)
		, fileName(other.fileName)
#endif // MPT_ENABLE_FILEIO
	{ }

#endif // MPT_FILEREADER_STD_ISTREAM


#if defined(MPT_ENABLE_FILEIO)
	mpt::PathString GetFileName() const
	{
		if(!fileName)
		{
			return mpt::PathString();
		}
		return *fileName;
	}
#endif // MPT_ENABLE_FILEIO

	// Returns true if the object points to a valid stream.
	bool IsValid() const
	{
		return DataContainer().IsValid();
	}

	// Reset cursor to first byte in file.
	void Rewind()
	{
		streamPos = 0;
	}

	// Seek to a position in the mapped file.
	// Returns false if position is invalid.
	bool Seek(off_t position)
	{
		if(position <= streamPos)
		{
			streamPos = position;
			return true;
		}
		if(position <= DataContainer().GetLength())
		{
			streamPos = position;
			return true;
		} else
		{
			return false;
		}
	}

	// Increases position by skipBytes.
	// Returns true if skipBytes could be skipped or false if the file end was reached earlier.
	bool Skip(off_t skipBytes)
	{
		if(CanRead(skipBytes))
		{
			streamPos += skipBytes;
			return true;
		} else
		{
			streamPos = DataContainer().GetLength();
			return false;
		}
	}

	// Decreases position by skipBytes.
	// Returns true if skipBytes could be skipped or false if the file start was reached earlier.
	bool SkipBack(off_t skipBytes)
	{
		if(streamPos >= skipBytes)
		{
			streamPos -= skipBytes;
			return true;
		} else
		{
			streamPos = 0;
			return false;
		}
	}

	// Returns cursor position in the mapped file.
	off_t GetPosition() const
	{
		return streamPos;
	}

	// Returns size of the mapped file in bytes.
	FILEREADER_DEPRECATED off_t GetLength() const
	{
		// deprecated because in case of an unseekable std::istream, this triggers caching of the whole file
		return DataContainer().GetLength();
	}

	// Return byte count between cursor position and end of file, i.e. how many bytes can still be read.
	FILEREADER_DEPRECATED off_t BytesLeft() const
	{
		// deprecated because in case of an unseekable std::istream, this triggers caching of the whole file
		return DataContainer().GetLength() - streamPos;
	}

	bool EndOfFile() const
	{
		return !CanRead(1);
	}

	bool NoBytesLeft() const
	{
		return !CanRead(1);
	}

	// Check if "amount" bytes can be read from the current position in the stream.
	bool CanRead(off_t amount) const
	{
		return DataContainer().CanRead(streamPos, amount);
	}

	// Check if file size is at least size, without potentially caching the whole file to query the exact file length.
	bool LengthIsAtLeast(off_t size) const
	{
		return DataContainer().CanRead(0, size);
	}

	// Check if file size is exactly size, without potentially caching the whole file if it is larger.
	bool LengthIs(off_t size) const
	{
		return DataContainer().CanRead(0, size) && !DataContainer().CanRead(size, 1);
	}

protected:

	FileReader CreateChunk(off_t position, off_t length) const
	{
		off_t readableLength = DataContainer().GetReadableLength(position, length);
		if(readableLength == 0)
		{
			return FileReader();
		}
		#if defined(MPT_FILEREADER_STD_ISTREAM)
			return FileReader(std::static_pointer_cast<IFileDataContainer>(std::make_shared<FileDataContainerWindow>(data, position, std::min(length, DataContainer().GetLength() - position))));
		#else
			return FileReader(mpt::as_span(DataContainer().GetRawData() + position, std::min(length, DataContainer().GetLength() - position)));
		#endif
	}

public:

	// Create a new FileReader object for parsing a sub chunk at a given position with a given length.
	// The file cursor is not modified.
	FileReader GetChunkAt(off_t position, off_t length) const
	{
		return CreateChunk(position, length);
	}

	// Create a new FileReader object for parsing a sub chunk at the current position with a given length.
	// The file cursor is not advanced.
	FileReader GetChunk(off_t length)
	{
		return CreateChunk(streamPos, length);
	}
	// Create a new FileReader object for parsing a sub chunk at the current position with a given length.
	// The file cursor is advanced by "length" bytes.
	FileReader ReadChunk(off_t length)
	{
		off_t position = streamPos;
		Skip(length);
		return CreateChunk(position, length);
	}

	//=====================
	class PinnedRawDataView
	//=====================
	{
	private:
		std::size_t size_;
		const mpt::byte *pinnedData;
		std::vector<mpt::byte> cache;
	private:
		void Init(const FileReader &file, std::size_t size)
		{
			size_ = 0;
			pinnedData = nullptr;
			if(!file.CanRead(size))
			{
				size = file.BytesLeft();
			}
			size_ = size;
			if(file.DataContainer().HasPinnedView())
			{
				pinnedData = file.DataContainer().GetRawData() + file.GetPosition();
			} else
			{
				cache.resize(size_);
				if(!cache.empty())
				{
					file.GetRaw(&(cache[0]), size);
				}
			}
		}
	public:
		PinnedRawDataView()
		{
			return;
		}
		PinnedRawDataView(const FileReader &file)
		{
			Init(file, file.BytesLeft());
		}
		PinnedRawDataView(const FileReader &file, std::size_t size)
		{
			Init(file, size);
		}
		PinnedRawDataView(FileReader &file, bool advance)
		{
			Init(file, file.BytesLeft());
			if(advance)
			{
				file.Skip(size_);
			}
		}
		PinnedRawDataView(FileReader &file, std::size_t size, bool advance)
		{
			Init(file, size);
			if(advance)
			{
				file.Skip(size_);
			}
		}
	public:
		mpt::span<const mpt::byte> GetSpan() const
		{
			if(pinnedData)
			{
				return mpt::as_span(pinnedData, size_);
			} else if(!cache.empty())
			{
				return mpt::as_span(cache);
			} else
			{
				return mpt::span<const mpt::byte>();
			}
		}
		mpt::span<const mpt::byte> span() const { return GetSpan(); }
		void invalidate() { size_ = 0; pinnedData = nullptr; cache = std::vector<mpt::byte>(); }
		const mpt::byte *data() const { return span().data(); }
		std::size_t size() const { return size_; }
		mpt::span<const mpt::byte>::iterator begin() const { return span().begin(); }
		mpt::span<const mpt::byte>::iterator end() const { return span().end(); }
		mpt::span<const mpt::byte>::const_iterator cbegin() const { return span().cbegin(); }
		mpt::span<const mpt::byte>::const_iterator cend() const { return span().cend(); }
	};

	// Returns a pinned view into the remaining raw data from cursor position.
	PinnedRawDataView GetPinnedRawDataView() const
	{
		return PinnedRawDataView(*this);
	}
	// Returns a pinned view into the remeining raw data from cursor position, clamped at size.
	PinnedRawDataView GetPinnedRawDataView(std::size_t size) const
	{
		return PinnedRawDataView(*this, size);
	}

	// Returns a pinned view into the remeining raw data from cursor position.
	// File cursor is advaned by the size of the returned pinned view.
	PinnedRawDataView ReadPinnedRawDataView()
	{
		return PinnedRawDataView(*this, true);
	}
	// Returns a pinned view into the remeining raw data from cursor position, clamped at size.
	// File cursor is advaned by the size of the returned pinned view.
	PinnedRawDataView ReadPinnedRawDataView(std::size_t size)
	{
		return PinnedRawDataView(*this, size, true);
	}

	// Returns raw stream data at cursor position.
	// Should only be used if absolutely necessary, for example for sample reading, or when used with a small chunk of the file retrieved by ReadChunk().
	// Use GetPinnedRawDataView(size) whenever possible.
	FILEREADER_DEPRECATED const mpt::byte *GetRawData() const
	{
		// deprecated because in case of an unseekable std::istream, this triggers caching of the whole file
		return DataContainer().GetRawData() + streamPos;
	}
	template <typename T>
	FILEREADER_DEPRECATED const T *GetRawData() const
	{
		// deprecated because in case of an unseekable std::istream, this triggers caching of the whole file
		return mpt::byte_cast<const T*>(DataContainer().GetRawData() + streamPos);
	}

	template <typename T>
	std::size_t GetRaw(T *dst, std::size_t count) const
	{
		return static_cast<std::size_t>(DataContainer().Read(mpt::byte_cast<mpt::byte*>(dst), streamPos, count));
	}

	template <typename T>
	std::size_t ReadRaw(T *dst, std::size_t count)
	{
		std::size_t result = static_cast<std::size_t>(DataContainer().Read(mpt::byte_cast<mpt::byte*>(dst), streamPos, count));
		streamPos += result;
		return result;
	}
	
	std::vector<mpt::byte> GetRawDataAsByteVector() const
	{
		PinnedRawDataView view = GetPinnedRawDataView();
		return std::vector<mpt::byte>(view.span().begin(), view.span().end());
	}
	std::vector<mpt::byte> ReadRawDataAsByteVector()
	{
		PinnedRawDataView view = ReadPinnedRawDataView();
		return std::vector<mpt::byte>(view.span().begin(), view.span().end());
	}
	std::vector<mpt::byte> GetRawDataAsByteVector(std::size_t size) const
	{
		PinnedRawDataView view = GetPinnedRawDataView(size);
		return std::vector<mpt::byte>(view.span().begin(), view.span().end());
	}
	std::vector<mpt::byte> ReadRawDataAsByteVector(std::size_t size)
	{
		PinnedRawDataView view = ReadPinnedRawDataView(size);
		return std::vector<mpt::byte>(view.span().begin(), view.span().end());
	}

	std::string GetRawDataAsString() const
	{
		PinnedRawDataView view = GetPinnedRawDataView();
		return std::string(view.span().begin(), view.span().end());
	}
	std::string ReadRawDataAsString()
	{
		PinnedRawDataView view = ReadPinnedRawDataView();
		return std::string(view.span().begin(), view.span().end());
	}
	std::string GetRawDataAsString(std::size_t size) const
	{
		PinnedRawDataView view = GetPinnedRawDataView(size);
		return std::string(view.span().begin(), view.span().end());
	}
	std::string ReadRawDataAsString(std::size_t size)
	{
		PinnedRawDataView view = ReadPinnedRawDataView(size);
		return std::string(view.span().begin(), view.span().end());
	}

protected:

	// Read a "T" object from the stream.
	// If not enough bytes can be read, false is returned.
	// If successful, the file cursor is advanced by the size of "T".
	template <typename T>
	bool Read(T &target)
	{
		if(sizeof(T) != DataContainer().Read(reinterpret_cast<mpt::byte*>(&target), streamPos, sizeof(T)))
		{
			return false;
		}
		streamPos += sizeof(T);
		return true;
	}

public:

	// Read some kind of integer in little-endian format.
	// If successful, the file cursor is advanced by the size of the integer.
	template <typename T>
	T ReadIntLE()
	{
		static_assert(std::numeric_limits<T>::is_integer == true, "Target type is a not an integer");
		T target;
		if(Read(target))
		{
			return SwapBytesLE(target);
		} else
		{
			return 0;
		}
	}

	// Read some kind of integer in big-endian format.
	// If successful, the file cursor is advanced by the size of the integer.
	template <typename T>
	T ReadIntBE()
	{
		static_assert(std::numeric_limits<T>::is_integer == true, "Target type is a not an integer");
		T target;
		if(Read(target))
		{
			return SwapBytesBE(target);
		} else
		{
			return 0;
		}
	}

	// Read a integer in little-endian format which has some of its higher bytes not stored in file.
	// If successful, the file cursor is advanced by the given size.
	template <typename T>
	T ReadTruncatedIntLE(off_t size)
	{
		static_assert(std::numeric_limits<T>::is_integer == true, "Target type is a not an integer");
		MPT_ASSERT(sizeof(T) >= size);
		if(size == 0)
		{
			return 0;
		}
		if(!CanRead(size))
		{
			return 0;
		}
		uint8 buf[sizeof(T)];
		bool negative = false;
		for(std::size_t i = 0; i < sizeof(T); ++i)
		{
			uint8 byte = 0;
			if(i < size)
			{
				Read(byte);
				negative = std::numeric_limits<T>::is_signed && ((byte & 0x80) != 0x00);
			} else
			{
				// sign or zero extend
				byte = negative ? 0xff : 0x00;
			}
			buf[i] = byte;
		}
		T target;
		std::memcpy(&target, buf, sizeof(T));
		return SwapBytesLE(target);
	}

	// Read a supplied-size little endian integer to a fixed size variable.
	// The data is properly sign-extended when fewer bytes are stored.
	// If more bytes are stored, higher order bytes are silently ignored.
	// If successful, the file cursor is advanced by the given size.
	template <typename T>
	T ReadSizedIntLE(off_t size)
	{
		static_assert(std::numeric_limits<T>::is_integer == true, "Target type is a not an integer");
		if(size == 0)
		{
			return 0;
		}
		if(!CanRead(size))
		{
			return 0;
		}
		if(size < sizeof(T))
		{
			return ReadTruncatedIntLE<T>(size);
		}
		T retval = ReadIntLE<T>();
		Skip(size - sizeof(T));
		return retval;
	}

	// Read unsigned 32-Bit integer in little-endian format.
	// If successful, the file cursor is advanced by the size of the integer.
	uint32 ReadUint32LE()
	{
		return ReadIntLE<uint32>();
	}

	// Read unsigned 32-Bit integer in big-endian format.
	// If successful, the file cursor is advanced by the size of the integer.
	uint32 ReadUint32BE()
	{
		return ReadIntBE<uint32>();
	}

	// Read signed 32-Bit integer in little-endian format.
	// If successful, the file cursor is advanced by the size of the integer.
	int32 ReadInt32LE()
	{
		return ReadIntLE<int32>();
	}

	// Read signed 32-Bit integer in big-endian format.
	// If successful, the file cursor is advanced by the size of the integer.
	int32 ReadInt32BE()
	{
		return ReadIntBE<int32>();
	}

	// Read unsigned 16-Bit integer in little-endian format.
	// If successful, the file cursor is advanced by the size of the integer.
	uint16 ReadUint16LE()
	{
		return ReadIntLE<uint16>();
	}

	// Read unsigned 16-Bit integer in big-endian format.
	// If successful, the file cursor is advanced by the size of the integer.
	uint16 ReadUint16BE()
	{
		return ReadIntBE<uint16>();
	}

	// Read signed 16-Bit integer in little-endian format.
	// If successful, the file cursor is advanced by the size of the integer.
	int16 ReadInt16LE()
	{
		return ReadIntLE<int16>();
	}

	// Read signed 16-Bit integer in big-endian format.
	// If successful, the file cursor is advanced by the size of the integer.
	int16 ReadInt16BE()
	{
		return ReadIntBE<int16>();
	}

	// Read unsigned 8-Bit integer.
	// If successful, the file cursor is advanced by the size of the integer.
	uint8 ReadUint8()
	{
		uint8 target;
		if(Read(target))
		{
			return target;
		} else
		{
			return 0;
		}
	}

	// Read signed 8-Bit integer. If successful, the file cursor is advanced by the size of the integer.
	int8 ReadInt8()
	{
		int8 target;
		if(Read(target))
		{
			return target;
		} else
		{
			return 0;
		}
	}

	// Read 32-Bit float in little-endian format.
	// If successful, the file cursor is advanced by the size of the float.
	float ReadFloatLE()
	{
		IEEE754binary32LE target;
		if(Read(target))
		{
			return target;
		} else
		{
			return 0.0f;
		}
	}

	// Read 32-Bit float in big-endian format.
	// If successful, the file cursor is advanced by the size of the float.
	float ReadFloatBE()
	{
		IEEE754binary32BE target;
		if(Read(target))
		{
			return target;
		} else
		{
			return 0.0f;
		}
	}

	// Read 64-Bit float in little-endian format.
	// If successful, the file cursor is advanced by the size of the float.
	double ReadDoubleLE()
	{
		IEEE754binary64LE target;
		if(Read(target))
		{
			return target;
		} else
		{
			return 0.0f;
		}
	}

	// Read 64-Bit float in big-endian format.
	// If successful, the file cursor is advanced by the size of the float.
	double ReadDoubleBE()
	{
		IEEE754binary64BE target;
		if(Read(target))
		{
			return target;
		} else
		{
			return 0.0f;
		}
	}

	// Read a struct.
	// If successful, the file cursor is advanced by the size of the struct. Otherwise, the target is zeroed.
	template <typename T>
	bool ReadStruct(T &target)
	{
		STATIC_ASSERT(mpt::is_binary_safe<T>::value);
		if(Read(target))
		{
			return true;
		} else
		{
			MemsetZero(target);
			return false;
		}
	}

	// Allow to read a struct partially (if there's less memory available than the struct's size, fill it up with zeros).
	// The file cursor is advanced by "partialSize" bytes.
	template <typename T>
	bool ReadStructPartial(T &target, off_t partialSize = sizeof(T))
	{
		STATIC_ASSERT(mpt::is_binary_safe<T>::value);
		off_t copyBytes = std::min(partialSize, sizeof(T));
		if(!CanRead(copyBytes))
		{
			copyBytes = BytesLeft();
		}
		DataContainer().Read(reinterpret_cast<mpt::byte *>(&target), streamPos, copyBytes);
		std::memset(reinterpret_cast<mpt::byte *>(&target) + copyBytes, 0, sizeof(target) - copyBytes);
		Skip(partialSize);
		return true;
	}

	// Read a string of length srcSize into fixed-length char array destBuffer using a given read mode.
	// The file cursor is advanced by "srcSize" bytes.
	// Returns true if at least one byte could be read or 0 bytes were requested.
	template<mpt::String::ReadWriteMode mode, size_t destSize>
	bool ReadString(char (&destBuffer)[destSize], const off_t srcSize)
	{
		FileReader::PinnedRawDataView source = ReadPinnedRawDataView(srcSize); // Make sure the string is cached properly.
		off_t realSrcSize = source.size();	// In case fewer bytes are available
		mpt::String::Read<mode, destSize>(destBuffer, mpt::byte_cast<const char*>(source.data()), realSrcSize);
		return (realSrcSize > 0 || srcSize == 0);
	}

	// Read a string of length srcSize into a std::string dest using a given read mode.
	// The file cursor is advanced by "srcSize" bytes.
	// Returns true if at least one character could be read or 0 characters were requested.
	template<mpt::String::ReadWriteMode mode>
	bool ReadString(std::string &dest, const off_t srcSize)
	{
		FileReader::PinnedRawDataView source = ReadPinnedRawDataView(srcSize);	// Make sure the string is cached properly.
		off_t realSrcSize = source.size();	// In case fewer bytes are available
		mpt::String::Read<mode>(dest, mpt::byte_cast<const char*>(source.data()), realSrcSize);
		return (realSrcSize > 0 || srcSize == 0);
	}

	// Read a string with a preprended length field of type Tsize (must be a packed<*,*> type) into a std::string dest using a given read mode.
	// The file cursor is advanced by the string length.
	// Returns true if the size field could be read and at least one character could be read or 0 characters were requested.
	template<typename Tsize, mpt::String::ReadWriteMode mode, size_t destSize>
	bool ReadSizedString(char (&destBuffer)[destSize], const off_t maxLength = std::numeric_limits<off_t>::max())
	{
		packed<typename Tsize::base_type, typename Tsize::endian_type> srcSize;	// Enforce usage of a packed type by ensuring that the passed type has the required typedefs
		if(!Read(srcSize))
			return false;
		return ReadString<mode>(destBuffer, std::min<off_t>(srcSize, maxLength));
	}

	// Read a string with a preprended length field of type Tsize (must be a packed<*,*> type) into a std::string dest using a given read mode.
	// The file cursor is advanced by the string length.
	// Returns true if the size field could be read and at least one character could be read or 0 characters were requested.
	template<typename Tsize, mpt::String::ReadWriteMode mode>
	bool ReadSizedString(std::string &dest, const off_t maxLength = std::numeric_limits<off_t>::max())
	{
		packed<typename Tsize::base_type, typename Tsize::endian_type> srcSize;	// Enforce usage of a packed type by ensuring that the passed type has the required typedefs
		if(!Read(srcSize))
			return false;
		return ReadString<mode>(dest, std::min<off_t>(srcSize, maxLength));
	}

	// Read a null-terminated string into a std::string
	bool ReadNullString(std::string &dest, const off_t maxLength = std::numeric_limits<off_t>::max())
	{
		dest.clear();
		if(!CanRead(1))
			return false;
		try
		{
			char buffer[64];
			off_t avail = 0;
			while((avail = std::min(DataContainer().Read(reinterpret_cast<mpt::byte*>(buffer), streamPos, sizeof(buffer)), maxLength - dest.length())) != 0)
			{
				auto end = std::find(buffer, buffer + avail, '\0');
				dest.insert(dest.end(), buffer, end);
				streamPos += (end - buffer);
				if(end < buffer + avail)
				{
					// Found null char
					streamPos++;
					break;
				}
			}
		} MPT_EXCEPTION_CATCH_OUT_OF_MEMORY(e)
		{
			MPT_EXCEPTION_DELETE_OUT_OF_MEMORY(e);
		}
		return dest.length() != 0;
	}

private:
	static bool IsLineEnding(char c) { return c == '\r' || c == '\n'; }
public:
	// Read a string up to the next line terminator into a std::string
	bool ReadLine(std::string &dest, const off_t maxLength = std::numeric_limits<off_t>::max())
	{
		dest.clear();
		if(!CanRead(1))
			return false;
		try
		{
			char buffer[64], c = '\0';
			off_t avail = 0;
			while((avail = std::min(DataContainer().Read(reinterpret_cast<mpt::byte*>(buffer), streamPos, sizeof(buffer)), maxLength - dest.length())) != 0)
			{
				auto end = std::find_if(buffer, buffer + avail, IsLineEnding);
				dest.insert(dest.end(), buffer, end);
				streamPos += (end - buffer);
				if(end < buffer + avail)
				{
					// Found line ending
					streamPos++;
					// Handle CRLF line ending
					if(*end == '\r')
					{
						if(Read(c) && c != '\n')
							SkipBack(1);
					}
					break;
				}
			}
		} MPT_EXCEPTION_CATCH_OUT_OF_MEMORY(e)
		{
			MPT_EXCEPTION_DELETE_OUT_OF_MEMORY(e);
		}
		return true;
	}

	// Read an array of byte-sized values.
	// If successful, the file cursor is advanced by the size of the array.
	// Otherwise, the target is zeroed.
	template<typename T, off_t destSize>
	bool ReadArray(T (&destArray)[destSize])
	{
		STATIC_ASSERT(sizeof(T) == 1);
		if(CanRead(sizeof(destArray)))
		{
			for(std::size_t i = 0; i < destSize; ++i)
			{
				Read(destArray[i]);
			}
			return true;
		} else
		{
			MemsetZero(destArray);
			return false;
		}
	}

	// Read an array.
	// If successful, the file cursor is advanced by the size of the array.
	// Otherwise, the target is zeroed.
	template<typename T, off_t destSize>
	bool ReadArrayLE(T (&destArray)[destSize])
	{
		if(CanRead(sizeof(destArray)))
		{
			for(std::size_t i = 0; i < destSize; ++i)
			{
				destArray[i] = ReadIntLE<T>();
			}
			return true;
		} else
		{
			MemsetZero(destArray);
			return false;
		}
	}

	// Read destSize elements of byte-sized type T into a vector.
	// If successful, the file cursor is advanced by the size of the vector.
	// Otherwise, the vector is resized to destSize, but possibly existing contents are not cleared.
	template<typename T>
	bool ReadVector(std::vector<T> &destVector, size_t destSize)
	{
		STATIC_ASSERT(sizeof(T) == 1);
		const off_t readSize = sizeof(T) * destSize;
		destVector.resize(destSize);
		if(CanRead(readSize))
		{
			for(std::size_t i = 0; i < destSize; ++i)
			{
				Read(destVector[i]);
			}
			return true;
		} else
		{
			return false;
		}
	}

	// Read destSize elements of type T into a vector.
	// If successful, the file cursor is advanced by the size of the vector.
	// Otherwise, the vector is resized to destSize, but possibly existing contents are not cleared.
	template<typename T>
	bool ReadVectorLE(std::vector<T> &destVector, size_t destSize)
	{
		const off_t readSize = sizeof(T) * destSize;
		try
		{
			destVector.resize(destSize);
		} MPT_EXCEPTION_CATCH_OUT_OF_MEMORY(e)
		{
			MPT_EXCEPTION_DELETE_OUT_OF_MEMORY(e);
			return false;
		}
		if(CanRead(readSize))
		{
			for(std::size_t i = 0; i < destSize; ++i)
			{
				destVector[i] = ReadIntLE<T>();
			}
			return true;
		} else
		{
			return false;
		}
	}

	// Compare a magic string with the current stream position.
	// Returns true if they are identical and advances the file cursor by the the length of the "magic" string.
	// Returns false if the string could not be found. The file cursor is not advanced in this case.
	template<size_t N>
	bool ReadMagic(const char (&magic)[N])
	{
		MPT_ASSERT(magic[N - 1] == '\0');
		for(std::size_t i = 0; i < N - 1; ++i)
		{
			MPT_ASSERT(magic[i] != '\0');
		}
		if(CanRead(N - 1))
		{
			mpt::byte bytes[N - 1];
			STATIC_ASSERT(sizeof(bytes) == sizeof(magic) - 1);
			DataContainer().Read(bytes, streamPos, N - 1);
			if(!std::memcmp(bytes, magic, N - 1))
			{
				streamPos += (N - 1);
				return true;
			}
		}
		return false;
	}

	bool ReadMagic(const char *const magic, off_t magicLength)
	{
		if(CanRead(magicLength))
		{
			bool identical = true;
			for(std::size_t i = 0; i < magicLength; ++i)
			{
				mpt::byte c = 0;
				DataContainer().Read(&c, streamPos + i, 1);
				if(c != mpt::byte_cast<mpt::byte>(magic[i]))
				{
					identical = false;
					break;
				}
			}
			if(identical)
			{
				streamPos += magicLength;
				return true;
			} else
			{
				return false;
			}
		} else
		{
			return false;
		}
	}

	// Read variable-length integer (as found in MIDI files).
	// If successful, the file cursor is advanced by the size of the integer and true is returned.
	// False is returned if not enough bytes were left to finish reading of the integer or if an overflow happened (source doesn't fit into target integer).
	// In case of an overflow, the target is also set to the maximum value supported by its data type.
	template<typename T>
	bool ReadVarInt(T &target)
	{
		static_assert(std::numeric_limits<T>::is_integer == true
			&& std::numeric_limits<T>::is_signed == false,
			"Target type is a not an unsigned integer");

		if(NoBytesLeft())
		{
			target = 0;
			return false;
		}

		size_t writtenBits = 0;
		uint8 b = ReadUint8();
		target = (b & 0x7F);

		// Count actual bits used in most significant byte (i.e. this one)
		for(size_t bit = 0; bit < 7; bit++)
		{
			if((b & (1 << bit)) != 0)
			{
				writtenBits = bit + 1;
			}
		}

		while(CanRead(1) && (b & 0x80) != 0)
		{
			b = ReadUint8();
			target <<= 7;
			target |= (b & 0x7F);
			writtenBits += 7;
		};

		if(writtenBits > sizeof(target) * 8)
		{
			// Overflow
			target = Util::MaxValueOfType<T>(target);
			return false;
		} else if((b & 0x80) != 0)
		{
			// Reached EOF
			return false;
		}
		return true;
	}

};


#if defined(MPT_ENABLE_FILEIO)
// templated in order to reduce header inter-depoendencies
template <typename TInputFile>
FileReader GetFileReader(TInputFile &file)
{
	#if defined(MPT_FILEREADER_STD_ISTREAM)
		typename TInputFile::ContentsRef tmp = file.Get();
		if(!tmp.first)
		{
			return FileReader();
		}
		if(!tmp.first->good())
		{
			return FileReader();
		}
		#ifdef MPT_ENABLE_FILEIO
			return FileReader(tmp.first, tmp.second);
		#else
			return FileReader(tmp.first);
		#endif
	#else
		typename TInputFile::ContentsRef tmp = file.Get();
		#ifdef MPT_ENABLE_FILEIO
			return FileReader(mpt::as_span(tmp.first.data, tmp.first.size), tmp.second);
		#else
			return FileReader(mpt::as_span(tmp.first.data, tmp.first.size));
		#endif
	#endif
}
#endif // MPT_ENABLE_FILEIO


#if defined(MPT_ENABLE_TEMPFILE) && MPT_OS_WINDOWS

class OnDiskFileWrapper
{

private:

	mpt::PathString m_Filename;
	bool m_IsTempFile;

public:

	OnDiskFileWrapper(FileReader &file, const mpt::PathString &fileNameExtension = MPT_PATHSTRING("tmp"));

	~OnDiskFileWrapper();

public:

	bool IsValid() const;

	mpt::PathString GetFilename() const;

}; // class OnDiskFileWrapper

#endif // MPT_ENABLE_TEMPFILE && MPT_OS_WINDOWS


OPENMPT_NAMESPACE_END
