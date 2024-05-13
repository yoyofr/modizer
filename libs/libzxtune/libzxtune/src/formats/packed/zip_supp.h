/**
* 
* @file
*
* @brief  ZIP archives structures
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//common includes
#include <byteorder.h>

namespace Formats
{
  namespace Packed
  {
    namespace Zip
    {
#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
      PACK_PRE struct GenericHeader
      {
        static const uint16_t SIGNATURE = 0x4b50;

        //+0
        ruint16_t Signature;
      } PACK_POST;

      PACK_PRE struct FileAttributes
      {
        ruint32_t CRC;
        ruint32_t CompressedSize;
        ruint32_t UncompressedSize;
      } PACK_POST;

      enum
      {
        FILE_CRYPTED = 0x1,
        FILE_COMPRESSION_LEVEL_MASK = 0x6,
        FILE_COMPRESSION_LEVEL_NONE = 0x0,
        FILE_COMPRESSION_LEVEL_NORMAL = 0x2,
        FILE_COMPRESSION_LEVEL_FAST = 0x4,
        FILE_COMPRESSION_LEVEL_EXTRAFAST = 0x6,
        FILE_ATTRIBUTES_IN_FOOTER = 0x8,
        FILE_UTF8 = 0x800
      };

      PACK_PRE struct LocalFileHeader
      {
        static const uint32_t SIGNATURE = 0x04034b50;

        //+0
        ruint32_t Signature;
        //+4
        ruint16_t VersionToExtract;
        //+6
        ruint16_t Flags;
        //+8
        ruint16_t CompressionMethod;
        //+a
        ruint16_t ModificationTime;
        //+c
        ruint16_t ModificationDate;
        //+e
        FileAttributes Attributes;
        //+1a
        ruint16_t NameSize;
        //+1c
        ruint16_t ExtraSize;
        //+1e
        uint8_t Name[1];


        bool IsValid() const;

        std::size_t GetSize() const;

        bool IsSupported() const;
      } PACK_POST;

      PACK_PRE struct LocalFileFooter
      {
        static const uint32_t SIGNATURE = 0x08074b50;

        //+0
        ruint32_t Signature;
        //+4
        FileAttributes Attributes;
      } PACK_POST;

      PACK_PRE struct ExtraDataRecord
      {
        static const uint32_t SIGNATURE = 0x08064b50;

        //+0
        ruint32_t Signature;
        //+4
        ruint32_t DataSize;
        //+8
        uint8_t Data[1];

        std::size_t GetSize() const;
      } PACK_POST;

      PACK_PRE struct CentralDirectoryFileHeader
      {
        static const uint32_t SIGNATURE = 0x02014b50;

        //+0
        ruint32_t Signature;
        //+4
        ruint16_t VersionMadeBy;
        //+6
        ruint16_t VersionToExtract;
        //+8
        ruint16_t Flags;
        //+a
        ruint16_t CompressionMethod;
        //+c
        ruint16_t ModificationTime;
        //+e
        ruint16_t ModificationDate;
        //+10
        FileAttributes Attributes;
        //+1c
        ruint16_t NameSize;
        //+1e
        ruint16_t ExtraSize;
        //+20
        ruint16_t CommentSize;
        //+22
        ruint16_t StartDiskNumber;
        //+24
        ruint16_t InternalFileAttributes;
        //+26
        ruint32_t ExternalFileAttributes;
        //+2a
        ruint32_t LocalHeaderRelOffset;
        //+2e
        uint8_t Name[1];

        std::size_t GetSize() const;
      } PACK_POST;

      PACK_PRE struct CentralDirectoryEnd
      {
        static const uint32_t SIGNATURE = 0x06054b50;

        //+0
        ruint32_t Signature;
        //+4
        ruint16_t ThisDiskNumber;
        //+6
        ruint16_t StartDiskNumber;
        //+8
        ruint16_t ThisDiskCentralDirectoriesCount;
        //+a
        ruint16_t TotalDirectoriesCount;
        //+c
        ruint32_t CentralDirectorySize;
        //+10
        ruint32_t CentralDirectoryOffset;
        //+12
        ruint16_t CommentSize;
        //+14
        //uint8_t Comment[0];

        std::size_t GetSize() const;
      } PACK_POST;

      PACK_PRE struct DigitalSignature
      {
        static const uint32_t SIGNATURE = 0x05054b50;

        //+0
        ruint32_t Signature;
        //+4
        ruint16_t DataSize;
        //+6
        uint8_t Data[1];

        std::size_t GetSize() const;
      } PACK_POST;

      class CompressedFile
      {
      public:
        virtual ~CompressedFile() {}

        virtual std::size_t GetPackedSize() const = 0;
        virtual std::size_t GetUnpackedSize() const = 0;

        static std::auto_ptr<const CompressedFile> Create(const LocalFileHeader& hdr, std::size_t availSize);
      };
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif
    }
  }
}
