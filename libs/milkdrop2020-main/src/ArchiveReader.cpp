
#define NOMINMAX
#include "ArchiveReader.h"


#ifndef WIN32
#include "../external/minizip/MDzip.h"
#include "../external/minizip/MDunzip.h"
#include "../external/minizip/ioapi_mem.h"
#endif

#include "../external/lzma1801/C/7z.h"
#include "../external/lzma1801/C/7zAlloc.h"
#include "../external/lzma1801/C/7zFile.h"
#include "../external/lzma1801/C/7zBuf.h"
#include "../external/lzma1801/C/7zCrc.h"
#include "../external/lzma1801/C/7zFile.h"
#include "../external/lzma1801/C/7zVersion.h"

#include "path.h"
#include "platform.h"

#include <stdint.h>
#include <memory>
#include <algorithm>

namespace Archive {


#ifndef WIN32
class ZipReader : public ArchiveReader
{
    
private:
    std::vector<uint8_t> _archiveData;
    zlib_filefunc_def _filefunc = {};
    ourmemory_t _memory = {};
    zipFile _zf = {};
    unz_file_info64 _file_info = {};
    bool _firstFile = true;
    
public:
    
    ZipReader()
    {
        
    }
    
    virtual ~ZipReader()
    {
        if (_zf)
            MDunzClose(_zf);
    }
    
    
    
    bool OpenArchive(std::string path)
    {
        if (!FileReadAllBytes(path, _archiveData))
        {
            return false;
        }
        
        // setup memory stream
        _memory.base = (char *)_archiveData.data();
        _memory.size = (uLong)_archiveData.size();
        _memory.limit = _memory.size ;
        _memory.cur_offset = 0;
        _memory.grow = 0;
        fill_memory_filefunc(&_filefunc, &_memory);
        
        
        _zf = MDunzOpen2(path.c_str(), &_filefunc);
        if (!_zf)
            return false;
        
        return true;
    }
    
    virtual void Rewind() override
    {
        _firstFile = true;
    }
    
    virtual bool NextFile(ArchiveFileInfo &fi) override
    {
        char buffer[4096]; buffer[0] = 0;
        int result;
        
        if (_firstFile) {
            result = MDunzGoToFirstFile2 (_zf, &_file_info,
                                        buffer, sizeof(buffer),
                                        NULL, 0,
                                        NULL, 0
                                        );
            _firstFile = false;
            
        } else {
            result = unzGoToNextFile2 (_zf, &_file_info,
                                       buffer, sizeof(buffer),
                                       NULL, 0,
                                       NULL, 0
                                       );
        }
        if (result != UNZ_OK)
            return false;
        
        fi.name = buffer;
        fi.compressedSize = _file_info.compressed_size;
        fi.uncompressedSize = _file_info.uncompressed_size;
        fi.is_directory = !fi.name.empty() && fi.name.back() == '/';
        return true;
    }
    
    virtual bool ExtractBinary(std::vector<uint8_t> &buffer) override
    {
        int err = MDunzOpenCurrentFile(_zf);
        if (err != UNZ_OK)
        {
            return false;
        }
        
        buffer.resize( _file_info.uncompressed_size );
        
        int read = MDunzReadCurrentFile(_zf, buffer.data(), (unsigned int)buffer.size());
        if (read != buffer.size())
        {
            return false;
        }
        
        MDMDunzCloseCurrentFile(_zf);
        return true;
    }
    
    
};
#endif

static const ISzAlloc g_Alloc = { SzAlloc, SzFree };

class SevenZipReader : public ArchiveReader
{
public:
    
    
    struct MemoryLookInStream : public ILookInStream
    {
        std::vector<uint8_t> _data;
        size_t _offset = 0;
        size_t _length = 0;
        
        
        MemoryLookInStream()
        {
            Look = MemoryLookInStream::DoLook;
            Skip = MemoryLookInStream::DoSkip;
            Read = MemoryLookInStream::DoRead;
            Seek = MemoryLookInStream::DoSeek;
        }
        
        SRes DoRead(void *buf, size_t *size)
        {
            if (_offset >= _length) {
                return SZ_ERROR_READ;
            }
            
            auto avail = _length - _offset;
            *size = std::min(avail, *size);
            memcpy( ((uint8_t *)buf), &_data[_offset], *size );
            return SZ_OK;
        }
        
        SRes DoSeek(Int64 *pos, ESzSeek origin)
        {
            switch (origin)
            {
                case SZ_SEEK_SET:
                    _offset = (size_t)*pos;
                    break;
                case SZ_SEEK_CUR:
                    _offset += (size_t)*pos;
                    break;
                case SZ_SEEK_END:
                    _offset = (size_t)(_length - *pos);
                    break;
            }
            *pos = _offset;
            return SZ_OK;
        }
        
        SRes DoLook(const void **buf, size_t *size)
        {
            auto avail = _length - _offset;
            
            *buf = &_data[_offset];
            *size = std::min(avail, *size);
            return SZ_OK;
        }
        
        SRes DoSkip(size_t offset)
        {
            _offset += offset;
            return SZ_OK;
        }
        
        static SRes DoLook(const ILookInStream *pp, const void **buf, size_t *size)
        {
            return ((MemoryLookInStream *)(pp))->DoLook(buf, size);
            
        }
        
        static SRes DoSkip(const ILookInStream *pp, size_t offset)
        {
            return ((MemoryLookInStream *)(pp))->DoSkip(offset);
        }
        
        static SRes DoRead(const ILookInStream *pp, void *buf, size_t *size)
        {
            return ((MemoryLookInStream *)(pp))->DoRead(buf, size);
        }
        
        static SRes DoSeek(const ILookInStream *pp, Int64 *pos, ESzSeek origin)
        {
            return ((MemoryLookInStream *)(pp))->DoSeek(pos, origin);
        }
    };
    
    
    SevenZipReader()
    {
        _db.NumFiles = 0;
    }
    
    virtual ~SevenZipReader()
    {
        if (_outBuffer)
        {
            ISzAlloc_Free(&_allocImp, _outBuffer);
        }
        
        SzArEx_Free(&_db, &_allocImp);
    }
    
    bool OpenArchive(std::string path)
    {
        if (!FileReadAllBytes(path, _lookStream._data))
        {
            return false;
        }
        _lookStream._offset = 0;
        _lookStream._length = _lookStream._data.size();
        
        CrcGenerateTable();
        
        SzArEx_Init(&_db);
        
        auto res = SzArEx_Open(&_db, &_lookStream, &_allocImp, &_allocTempImp);
        if (res != SZ_OK)
        {
            return false;
        }
        
        return true;
    }
    
    
    static void ConvertString(std::string &dest, const std::vector<uint16_t> &source)
    {
        dest.clear();
        dest.reserve(source.size());
        for (int j=0; source[j]; j++)
        {
            dest += (char)source[j];
        }
    }
    
    virtual void Rewind() override
    {
        _fileIndex = -1;
    }
    
    virtual bool NextFile(ArchiveFileInfo &fi) override
    {
        ++_fileIndex;
        if (_fileIndex >= (int)_db.NumFiles)
            return false;
        
        size_t len = SzArEx_GetFileNameUtf16(&_db, _fileIndex, NULL);
        _filename.resize(len);
        SzArEx_GetFileNameUtf16(&_db, _fileIndex, _filename.data());
        
        fi.is_directory = SzArEx_IsDir(&_db, _fileIndex);
        fi.compressedSize = 0;
        fi.uncompressedSize = (long)SzArEx_GetFileSize(&_db, _fileIndex);
        ConvertString(fi.name, _filename);
        return true;
    }
    
    virtual bool ExtractBinary(std::vector<uint8_t> &buffer) override
    {
        if (_fileIndex >= (int)_db.NumFiles)
            return false;
        
        size_t offset = 0;
        size_t outSizeProcessed = 0;
        
        auto res = SzArEx_Extract(&_db, &_lookStream, _fileIndex,
                                  &_blockIndex, &_outBuffer, &_outBufferSize,
                                  &offset, &outSizeProcessed,
                                  &_allocImp, &_allocTempImp);
        
        if (res != SZ_OK) {
            buffer.clear();
            return false;
        }
        
        // copy data into buffer
        buffer.resize(outSizeProcessed);
        buffer.assign(_outBuffer + offset, _outBuffer + offset + outSizeProcessed);
        return true;
    }
    
    
private:
    ISzAlloc _allocImp = g_Alloc;
    ISzAlloc _allocTempImp = g_Alloc;
    
    MemoryLookInStream _lookStream = {};
    
    CSzArEx _db = {};
    int     _fileIndex = -1;
    
    std::vector<UInt16> _filename;
    
    Byte *_outBuffer = nullptr;
    UInt32 _blockIndex = 0xFFFFFFFF; /* it can have any value before first call (if outBuffer = 0) */
    size_t _outBufferSize = 0;  /* it can have any value before first call (if outBuffer = 0) */
    
};



ArchiveReaderPtr OpenArchive(std::string path)
{
    std::string ext = PathGetExtensionLower(path);
    

#ifndef WIN32
    if (ext == ".zip")
    {
        auto reader = std::make_shared<ZipReader>();
        if (!reader->OpenArchive(path))
        {
            return nullptr;
        }
        return reader;
    }
#endif
    if (ext == ".7z")
    {
        auto reader = std::make_shared<SevenZipReader>();
        if (!reader->OpenArchive(path))
        {
            return nullptr;
        }
        return reader;
    }
    
    return nullptr;
    
}

} // namespace
