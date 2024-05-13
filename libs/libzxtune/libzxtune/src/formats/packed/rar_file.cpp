/**
* 
* @file
*
* @brief  RAR compressor support
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "container.h"
#include "rar_supp.h"
#include "pack_utils.h"
//library includes
#include <binary/format_factories.h>
#include <binary/typed_container.h>
#include <debug/log.h>
#include <formats/packed.h>
#include <math/numeric.h>
//std includes
#include <cassert>
#include <limits>
#include <memory>
//boost includes
#include <boost/array.hpp>
#include <boost/make_shared.hpp>
//thirdparty
#include <3rdparty/unrar/rar.hpp>
//text includes
#include <formats/text/packed.h>

#undef min
#undef max

namespace RarFile
{
  const Debug::Stream Dbg("Formats::Packed::Rar");

  const std::string HEADER_PATTERN =
    "??"          // uint16_t CRC;
    "74"          // uint8_t Type;
    "?%1xxxxxxx"  // uint16_t Flags;
  ;

  class Container : public Binary::TypedContainer
  {
  public:
    explicit Container(const Binary::Container& data)
      : Binary::TypedContainer(data)
      , Data(data)
    {
    }

    bool FastCheck() const
    {
      if (std::size_t usedSize = GetUsedSize())
      {
        return usedSize <= Data.Size();
      }
      return false;
    }

    std::size_t GetUsedSize() const
    {
      if (const Formats::Packed::Rar::FileBlockHeader* header = GetField<Formats::Packed::Rar::FileBlockHeader>(0))
      {
        if (!header->IsValid() || !header->IsSupported())
        {
          return 0;
        }
        uint64_t res = fromLE(header->Size);
        res += fromLE(header->AdditionalSize);
        if (header->IsBigFile())
        {
          if (const Formats::Packed::Rar::BigFileBlockHeader* bigHeader = GetField<Formats::Packed::Rar::BigFileBlockHeader>(0))
          {
            res += uint64_t(fromLE(bigHeader->PackedSizeHi)) << (8 * sizeof(uint32_t));
          }
          else
          {
            return 0;
          }
        }
        const std::size_t maximum = std::numeric_limits<std::size_t>::max();
        return res > maximum
          ? maximum
          : static_cast<std::size_t>(res);
      }
      return false;
    }

    const Formats::Packed::Rar::FileBlockHeader& GetHeader() const
    {
      return *GetField<Formats::Packed::Rar::FileBlockHeader>(0);
    }

    const Binary::Container& GetData() const
    {
      return Data;
    }
  private:
    const Binary::Container& Data;
  };

  class CompressedFile
  {
  public:
    typedef std::auto_ptr<const CompressedFile> Ptr;
    virtual ~CompressedFile() {}

    virtual Binary::Container::Ptr Decompress(const Container& container) const = 0;
  };

  class StoredFile : public CompressedFile
  {
  public:
    virtual Binary::Container::Ptr Decompress(const Container& container) const
    {
      const Formats::Packed::Rar::FileBlockHeader& header = container.GetHeader();
      const std::size_t offset = fromLE(header.Size);
      const std::size_t size = fromLE(header.AdditionalSize);
      const std::size_t outSize = fromLE(header.UnpackedSize);
      assert(0x30 == header.Method);
      if (size != outSize)
      {
        Dbg("Stored file sizes mismatch");
        return Binary::Container::Ptr();
      }
      else
      {
        Dbg("Restore");
        return container.GetData().GetSubcontainer(offset, size);
      }
    }
  };


  class PackedFile : public CompressedFile
  {
  public:
    PackedFile()
      : Decoder(&Stream)
    {
      Stream.EnableShowProgress(false);
      Decoder.Init();
    }

    virtual Binary::Container::Ptr Decompress(const Container& container) const
    {
      const Formats::Packed::Rar::FileBlockHeader& header = container.GetHeader();
      assert(0x30 != header.Method);
      const std::size_t offset = fromLE(header.Size);
      const std::size_t size = fromLE(header.AdditionalSize);
      const std::size_t outSize = fromLE(header.UnpackedSize);
      const bool isSolid = header.IsSolid();
      Dbg("Depack %1% -> %2% (solid %3%)", size, outSize, isSolid);
      //Old format starts from 52 45 7e 5e
      const bool oldFormat = false;
      Stream.SetUnpackFromMemory(container.GetField<uint8_t>(offset), size, oldFormat);
      Stream.SetPackedSizeToRead(size);
      const int method = std::max<int>(header.DepackerVersion, 15);
      return Decompress(method, outSize, isSolid, fromLE(header.UnpackedCRC));
    }
  private:
    Binary::Container::Ptr Decompress(int method, std::size_t outSize, bool isSolid, uint32_t crc) const
    {
      try
      {
        std::auto_ptr<Dump> result(new Dump(outSize));
        Stream.SetUnpackToMemory(&result->front(), outSize);
        Decoder.SetDestSize(outSize);
        Decoder.DoUnpack(method, isSolid);
        if (crc != Stream.GetUnpackedCrc())
        {
          Dbg("Crc mismatch: stored 0x%1$08x, calculated 0x%2$08x", crc, Stream.GetUnpackedCrc());
        }
        return Binary::CreateContainer(result);
      }
      catch (const std::exception& e)
      {
        Dbg("Failed to decode: %1%", e.what());
        return Binary::Container::Ptr();
      }
    }
  private:
    mutable ComprDataIO Stream;
    mutable Unpack Decoder;
  };

  class DispatchedCompressedFile : public CompressedFile
  {
  public:
    DispatchedCompressedFile()
      : Packed(new PackedFile())
      , Stored(new StoredFile())
    {
    }

    virtual Binary::Container::Ptr Decompress(const Container& container) const
    {
      const Formats::Packed::Rar::FileBlockHeader& header = container.GetHeader();
      if (header.IsStored())
      {
        return Stored->Decompress(container);
      }
      else
      {
        return Packed->Decompress(container);
      }
    }
  private:
    const std::auto_ptr<CompressedFile> Packed;
    const std::auto_ptr<CompressedFile> Stored;
  };
}

namespace Formats
{
  namespace Packed
  {
    namespace Rar
    {
      String FileBlockHeader::GetName() const
      {
        const uint8_t* const self = safe_ptr_cast<const uint8_t*>(this);
        const uint8_t* const filename = self + (IsBigFile() ? sizeof(BigFileBlockHeader) : sizeof(FileBlockHeader));
        return String(filename, filename + fromLE(NameSize));
      }

      bool FileBlockHeader::IsValid() const
      {
        return IsExtended() && Type == TYPE;
      }

      bool FileBlockHeader::IsSupported() const
      {
        const uint_t flags = fromLE(Flags);
        //multivolume files are not suported
        if (0 != (flags & (FileBlockHeader::FLAG_SPLIT_BEFORE | FileBlockHeader::FLAG_SPLIT_AFTER)))
        {
          return false;
        }
        //encrypted files are not supported
        if (0 != (flags & FileBlockHeader::FLAG_ENCRYPTED))
        {
          return false;
        }
        //big files are not supported
        if (IsBigFile())
        {
          return false;
        }
        //skip directory
        if (FileBlockHeader::FLAG_DIRECTORY == (flags & FileBlockHeader::FLAG_DIRECTORY))
        {
          return false;
        }
        //skip empty files
        if (0 == UnpackedSize)
        {
          return false;
        }
        //skip invalid version
        if (!Math::InRange<uint_t>(DepackerVersion, MIN_VERSION, MAX_VERSION))
        {
          return false;
        }
        return true;
      }
    }

    class RarDecoder : public Decoder
    {
    public:
      RarDecoder()
        : Format(Binary::CreateFormat(RarFile::HEADER_PATTERN))
        , Decoder(new RarFile::DispatchedCompressedFile())
      {
      }

      virtual String GetDescription() const
      {
        return Text::RAR_DECODER_DESCRIPTION;
      }

      virtual Binary::Format::Ptr GetFormat() const
      {
        return Format;
      }

      virtual Container::Ptr Decode(const Binary::Container& rawData) const
      {
        const RarFile::Container container(rawData);
        if (!container.FastCheck())
        {
          return Container::Ptr();
        }
        return CreatePackedContainer(Decoder->Decompress(container), container.GetUsedSize());
      }
    private:
      const Binary::Format::Ptr Format;
      const RarFile::CompressedFile::Ptr Decoder;
    };

    Decoder::Ptr CreateRarDecoder()
    {
      return boost::make_shared<RarDecoder>();
    }
  }
}
