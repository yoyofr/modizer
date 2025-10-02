#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include <memory>

namespace Archive
{


struct ArchiveFileInfo
{
    std::string name;
    long uncompressedSize;
    long compressedSize;
    bool is_directory;
};


class ArchiveReader
{
public:

    ArchiveReader() = default;
    virtual ~ArchiveReader() = default;

    virtual void Rewind() = 0;
    virtual bool NextFile(ArchiveFileInfo &fi) = 0;
    virtual bool ExtractBinary(std::vector<uint8_t> &buffer) = 0;

    virtual bool ExtractText(std::string &text)
    {
        std::vector<uint8_t> buffer;
        if (!ExtractBinary(buffer))
        {
            return false;
        }
        
        text.assign( (const char *)buffer.data(), (const char *)buffer.data() + buffer.size() );
        return true;
    }
};

using ArchiveReaderPtr = std::shared_ptr<ArchiveReader>;


ArchiveReaderPtr OpenArchive(std::string path);


}

