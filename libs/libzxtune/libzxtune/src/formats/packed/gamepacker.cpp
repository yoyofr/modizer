/**
* 
* @file
*
* @brief  GamePacker packer support
*
* @author vitamin.caig@gmail.com
*
* @note   Based on Pusher sources by Himik/ZxZ
*
**/

//local includes
#include "container.h"
#include "pack_utils.h"
//common includes
#include <byteorder.h>
#include <pointers.h>
//library includes
#include <binary/format_factories.h>
#include <formats/packed.h>
//std includes
#include <cstring>
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <formats/text/packed.h>

namespace GamePacker
{
  const std::size_t MAX_DECODED_SIZE = 0xc000;

  struct Version1
  {
    static const String DESCRIPTION;
    static const std::size_t MIN_SIZE = 0x20;//TODO
    static const std::string DEPACKER_PATTERN;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
    PACK_PRE struct RawHeader
    {
      //+0
      char Padding1;
      //+1
      ruint16_t DepackerBodyAddr;
      //+3
      char Padding2[5];
      //+8
      ruint16_t DepackerBodySize;
      //+a
      char Padding3[3];
      //+d
      ruint16_t EndOfPackedSource;
      //+f
      char Padding4[4];
      //+13
      ruint16_t PackedSize;
      //+15

      uint_t GetPackedDataOffset() const
      {
        const uint_t selfAddr = fromLE(DepackerBodyAddr) - 0x1d;
        const uint_t packedDataStart = fromLE(EndOfPackedSource) - fromLE(PackedSize) + 1;
        return packedDataStart - selfAddr;
      }

      uint_t GetPackedDataSize() const
      {
        return fromLE(PackedSize);
      }
    } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif
  };

  struct Version2
  {
    static const String DESCRIPTION;
    static const std::size_t MIN_SIZE = 0x20;//TODO
    static const std::string DEPACKER_PATTERN;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
    PACK_PRE struct RawHeader
    {
      //+0
      char Padding1;
      //+1
      ruint16_t DepackerBodyAddr;
      //+3
      char Padding2[4];
      //+7
      ruint16_t DepackerBodySize;
      //+9
      char Padding3[5];
      //+e
      ruint16_t PackedSource;
      //+10

      uint_t GetPackedDataOffset() const
      {
        const uint_t selfAddr = fromLE(DepackerBodyAddr) - 0x0d;
        return fromLE(PackedSource) - selfAddr;
      }

      uint_t GetPackedDataSize() const
      {
        return 0;
      }
    } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif
  };

  const String Version1::DESCRIPTION = Text::GAM_DECODER_DESCRIPTION; 
  const std::string Version1::DEPACKER_PATTERN =
    "21??"   // ld hl,xxxx depacker body src
    "11??"   // ld de,xxxx depacker body dst
    "d5"     // push de
    "01??"   // ld bc,xxxx depacker body size
    "edb0"   // ldir
    "21??"   // ld hl,xxxx PackedSource
    "11ffff" // ld de,ffff EndOfPacked
    "01??"   // ld bc,xxxx PackedSize
    "edb8"   // lddr
    "13"     // inc de
    "eb"     // ex de,hl
    "11??"   // ld de,depack target
    "c9"     // ret
    //+29 (0x1d) DepackerBody starts here
    "7c"     // ld a,h
    "b5"     // or l
  ;

  const String Version2::DESCRIPTION = Text::GAMPLUS_DECODER_DESCRIPTION;
  const std::string Version2::DEPACKER_PATTERN =
    "21??"   // ld hl,xxxx depacker body src
    "11??"   // ld de,xxxx depacker body dst
    "01??"   // ld bc,xxxx depacker body size
    "d5"     // push de
    "edb0"   // ldir
    "c9"     // ret
    //+13 (0x0d)
    "21??"   // ld hl,xxxx PackedSource
    "11??"   // ld de,xxxx PackedTarget
    "7e"
    "cb7f"
    "20?"
    "e60f"
    "47"
    "ed6f"
    "23"
    "c603"
    "4f"
    "7b"
    "96"
    //23 e5 6f 7a 98 67 0600 edb0 e1 18e3 e67f ca7181 23 cb77 2007 4f 0600 edb0 18d2 e6 3f c603 48 7e 23 12 1310fc18c5
  ;

  BOOST_STATIC_ASSERT(sizeof(Version1::RawHeader) == 0x15);
  BOOST_STATIC_ASSERT(sizeof(Version2::RawHeader) == 0x10);

  template<class Version>
  class Container
  {
  public:
    Container(const void* data, std::size_t size)
      : Data(static_cast<const uint8_t*>(data))
      , Size(size)
    {
    }

    bool Check() const
    {
      if (Size <= sizeof(typename Version::RawHeader))
      {
        return false;
      }
      const typename Version::RawHeader& header = GetHeader();
      return header.GetPackedDataOffset() + header.GetPackedDataSize() <= Size;
    }

    const uint8_t* GetPackedData() const
    {
      const typename Version::RawHeader& header = GetHeader();
      return Data + header.GetPackedDataOffset();
    }

    std::size_t GetPackedDataSize() const
    {
      const typename Version::RawHeader& header = GetHeader();
      if (uint_t packed = header.GetPackedDataSize())
      {
        return packed;
      }
      return Size - header.GetPackedDataOffset();
    }

    const typename Version::RawHeader& GetHeader() const
    {
      assert(Size >= sizeof(typename Version::RawHeader));
      return *safe_ptr_cast<const typename Version::RawHeader*>(Data);
    }
  private:
    const uint8_t* const Data;
    const std::size_t Size;
  };

  template<class Version>
  class DataDecoder
  {
  public:
    explicit DataDecoder(const Container<Version>& container)
      : IsValid(container.Check())
      , Header(container.GetHeader())
      , Stream(container.GetPackedData(), container.GetPackedDataSize())
      , Result(new Dump())
      , Decoded(*Result)
    {
      if (IsValid && !Stream.Eof())
      {
        IsValid = DecodeData();
      }
    }

    std::auto_ptr<Dump> GetResult()
    {
      return IsValid
        ? Result
        : std::auto_ptr<Dump>();
    }

    std::size_t GetUsedSize() const
    {
      return IsValid
        ? Header.GetPackedDataOffset() + Stream.GetProcessedBytes()
        : 0;
    }
  private:
    bool DecodeData()
    {
      Decoded.reserve(Header.GetPackedDataSize() * 2);

      while (!Stream.Eof() && Decoded.size() < MAX_DECODED_SIZE)
      {
        const uint8_t byt = Stream.GetByte();
        if (0 == (byt & 128))
        {
          const uint_t offset = 256 * (byt & 0x0f) + Stream.GetByte();
          const uint_t count = (byt >> 4) + 3;
          if (!CopyFromBack(offset, Decoded, count))
          {
            return false;
          }
        }
        else if (0 == (byt & 64))
        {
          uint_t count = byt & 63;
          if (!count)
          {
            return true;
          }
          for (; count && !Stream.Eof(); --count)
          {
            Decoded.push_back(Stream.GetByte());
          }
          if (count)
          {
            return false;
          }
        }
        else
        {
          const uint_t data = Stream.GetByte();
          const uint_t len = (byt & 63) + 3;
          std::fill_n(std::back_inserter(Decoded), len, data);
        }
      }
      return true;
    }
  private:
    bool IsValid;
    const typename Version::RawHeader& Header;
    ByteStream Stream;
    std::auto_ptr<Dump> Result;
    Dump& Decoded;
  };
}


namespace Formats
{
  namespace Packed
  {
    template<class Version>
    class GamePackerDecoder : public Decoder
    {
    public:
      GamePackerDecoder()
        : Depacker(Binary::CreateFormat(Version::DEPACKER_PATTERN, Version::MIN_SIZE))
      {
      }

      virtual String GetDescription() const
      {
        return Version::DESCRIPTION;
      }

      virtual Binary::Format::Ptr GetFormat() const
      {
        return Depacker;
      }

      virtual Container::Ptr Decode(const Binary::Container& rawData) const
      {
        if (!Depacker->Match(rawData))
        {
          return Container::Ptr();
        }

        const GamePacker::Container<Version> container(rawData.Start(), rawData.Size());
        if (!container.Check())
        {
          return Container::Ptr();
        }
        GamePacker::DataDecoder<Version> decoder(container);
        return CreatePackedContainer(decoder.GetResult(), decoder.GetUsedSize());
      }
    private:
      const Binary::Format::Ptr Depacker;
    };

    Decoder::Ptr CreateGamePackerDecoder()
    {
      return boost::make_shared<GamePackerDecoder<GamePacker::Version1> >();
    }

    Decoder::Ptr CreateGamePackerPlusDecoder()
    {
      return boost::make_shared<GamePackerDecoder<GamePacker::Version2> >();
    }
  }
}
