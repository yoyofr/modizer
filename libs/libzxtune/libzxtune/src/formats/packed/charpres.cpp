/**
* 
* @file
*
* @brief  CharPres packer support
*
* @author vitamin.caig@gmail.com
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
#include <algorithm>
#include <iterator>
//boost includes
#include <boost/make_shared.hpp>
//text include
#include <formats/text/packed.h>

namespace CharPres
{
  const std::size_t MIN_SIZE = 0x20;//TODO
  const std::size_t MAX_DECODED_SIZE = 0xc000;

  const std::string DEPACKER_PATTERN(
    "21??"          // ld hl,xxxx depacker body src
    "11??"          // ld de,xxxx depacker body dst
    "01??"          // ld bc,xxxx depacker body size
    "d5"            // push de
    "edb0"          // ldir
    "c9"            // ret
    //+0x0d
    "11??"          // ld de,xxxx packed dst
    "01??"          // ld bc,xxxx packed size
    "ed?"           // ldir/lddr (ldir really)
    "eb"            // ex de,hl
    "11??"          // ld de,depack dst (end)
    "2b"            // dec hl
    "7e"            // ld a,(hl)
    "1b"            // dec de
    "12"            // ld (de),a
    "d6?"           // sub 0x1d
    "20?"           // jr nz,xx
    "2b"            // dec hl
  );

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct RawHeader
  {
    //+0
    char Padding1;
    //+1
    ruint16_t DepackerBodySrc;
    //+3
    char Padding2;
    //+4
    ruint16_t DepackerBodyDst;
    //+6
    char Padding3;
    //+7
    ruint16_t DepackerBodySize;
    //+0x9
    char Padding4[4];
    //+0xd
    char DepackerBody[1];
    //+0xe
    ruint16_t PackedDataDst;
    //+0x10
    char Padding5;
    //+0x11
    ruint16_t PackedDataSize;
    //+0x13
    char Padding6;
    //+0x14
    uint8_t PackedDataCopyDirection;
    //+0x15
    char Padding7;
    //+0x16
    ruint16_t EndOfDepacked;
    //+0x18
    char Padding8[6];
    //+0x1e
    uint8_t Marker;
    //+0x1f
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(RawHeader) == 0x1f);

  class Container
  {
  public:
    Container(const void* data, std::size_t size)
      : Data(static_cast<const uint8_t*>(data))
      , Size(size)
    {
    }

    bool FastCheck() const
    {
      if (Size < sizeof(RawHeader))
      {
        return false;
      }
      const RawHeader& header = GetHeader();
      const DataMovementChecker checker(fromLE(header.DepackerBodySrc) + fromLE(header.DepackerBodySize), 
        fromLE(header.PackedDataDst), fromLE(header.PackedDataSize), header.PackedDataCopyDirection);
      if (!checker.IsValid())
      {
        return false;
      }
      const uint_t usedSize = GetUsedSize();
      if (usedSize > Size)
      {
        return false;
      }
      return true;
    }

    uint_t GetUsedSize() const
    {
      const RawHeader& header = GetHeader();
      return offsetof(RawHeader, DepackerBody) + fromLE(header.DepackerBodySize) + fromLE(header.PackedDataSize);
    }

    const uint8_t* GetPackedData() const
    {
      const RawHeader& header = GetHeader();
      const std::size_t fixedSize = offsetof(RawHeader, DepackerBody) + fromLE(header.DepackerBodySize);
      return Data + fixedSize;
    }

    std::size_t GetPackedSize() const
    {
      const RawHeader& header = GetHeader();
      return fromLE(header.PackedDataSize);
    }

    const RawHeader& GetHeader() const
    {
      assert(Size >= sizeof(RawHeader));
      return *safe_ptr_cast<const RawHeader*>(Data);
    }
  private:
    const uint8_t* const Data;
    const std::size_t Size;
  };

  class ReverseByteStream
  {
  public:
    ReverseByteStream(const uint8_t* data, std::size_t size)
      : Data(data), Pos(Data + size)
    {
    }

    bool Eof() const
    {
      return Pos < Data;
    }

    uint8_t GetByte()
    {
      return Eof() ? 0 : *--Pos;
    }
  private:
    const uint8_t* const Data;
    const uint8_t* Pos;
  };

  class DataDecoder
  {
  public:
    explicit DataDecoder(const Container& container)
      : IsValid(container.FastCheck())
      , Header(container.GetHeader())
      , Stream(container.GetPackedData(), container.GetPackedSize())
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
  private:
    bool DecodeData()
    {
      //assume that first byte always exists due to header format
      while (!Stream.Eof() && Decoded.size() < MAX_DECODED_SIZE)
      {
        const uint_t data = Stream.GetByte();
        if (data != Header.Marker)
        {
          Decoded.push_back(data);
        }
        else
        {
          if (const uint_t nextDat = Stream.GetByte())
          {
            const uint_t offset = 256 * (nextDat & 7) + Stream.GetByte() + 1;
            const std::size_t len = 3 + (nextDat >> 3);
            if (!CopyFromBack(offset, Decoded, len))
            {
              return false;
            }
          }
          else
          {
            Decoded.push_back(data);
          }
        }
      }
      std::reverse(Decoded.begin(), Decoded.end());
      return true;
    }
  private:
    bool IsValid;
    const RawHeader& Header;
    ReverseByteStream Stream;
    std::auto_ptr<Dump> Result;
    Dump& Decoded;
  };
}

namespace Formats
{
  namespace Packed
  {
    class CharPresDecoder : public Decoder
    {
    public:
      CharPresDecoder()
        : Depacker(Binary::CreateFormat(CharPres::DEPACKER_PATTERN, CharPres::MIN_SIZE))
      {
      }

      virtual String GetDescription() const
      {
        return Text::CHARPRES_DECODER_DESCRIPTION;
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
        const CharPres::Container container(rawData.Start(), rawData.Size());
        if (!container.FastCheck())
        {
          return Container::Ptr();
        }
        CharPres::DataDecoder decoder(container);
        return CreatePackedContainer(decoder.GetResult(), container.GetUsedSize());
      }
    private:
      const Binary::Format::Ptr Depacker;
    };

    Decoder::Ptr CreateCharPresDecoder()
    {
      return boost::make_shared<CharPresDecoder>();
    }
  }
}
