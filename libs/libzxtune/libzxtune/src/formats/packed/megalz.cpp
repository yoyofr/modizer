/**
* 
* @file
*
* @brief  MegaLZ packer support
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "container.h"
#include "pack_utils.h"
//common includes
#include <byteorder.h>
#include <contract.h>
//library includes
#include <binary/format_factories.h>
#include <formats/packed.h>
//std includes
#include <numeric>
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <formats/text/packed.h>

namespace MegaLZ
{
  const std::size_t MIN_DECODED_SIZE = 0x100;
  const std::size_t DEPACKER_SIZE = 110;
  const std::size_t MIN_SIZE = 256;
  const std::size_t MIN_PACKED_SIZE = MIN_SIZE - DEPACKER_SIZE;
  const std::size_t MAX_DECODED_SIZE = 0xc000;
  //assume that packed data are located right after depacked
  //prologue is ignored due to standard absense
  const std::string DEPACKER_PATTERN =
    "3e80"   //ld a,#80
    "08"     //ex af,af'
    "eda0"   //ldi
    "01ff02" //ld bc,#02ff
    "08"     //ex af,af'
    "87"     //add a,a
    "2003"   //jr nz,xxx
    "7e"     //ld a,(hl)
    "23"     //inc hl
    "17"     //rla
    "cb11"   //rl c
    "30f6"   //jr nc,xxx
    "08"     //ex af,af'
    "100f"   //djnz xxxx
    "3e02"   //ld a,2
    "cb29"   //sra c
    "3818"   //jr c,xxxx
    "3c"     //inc a
    "0c"     //inc c
    "280f"   //jr z,xxxx
    "013f03" //ld bc,#033f
  ;

  class Bitstream : private ByteStream
  {
  public:
    Bitstream(const uint8_t* data, std::size_t size)
      : ByteStream(data, size)
      , Bits(), Mask(0)
    {
    }

    uint8_t GetByte()
    {
      Require(!Eof());
      return ByteStream::GetByte();
    }

    uint_t GetBit()
    {
      if (!(Mask >>= 1))
      {
        Bits = GetByte();
        Mask = 0x80;
      }
      return Bits & Mask ? 1 : 0;
    }

    uint_t GetBits(uint_t count)
    {
      uint_t result = 0;
      while (count--)
      {
        result = 2 * result | GetBit();
      }
      return result;
    }

    uint_t GetLen()
    {
      uint_t len = 1;
      while (!GetBit())
      {
        ++len;
      }
      return len;
    }

    int_t GetDist()
    {
      if (GetBit())
      {
        const uint_t bits = GetBits(4) << 8;
        return static_cast<int16_t>(((0xf000 | bits) - 0x100) | GetByte());
      }
      else
      {
        return static_cast<int16_t>(0xff00 + GetByte());
      }
    }

    using ByteStream::GetProcessedBytes;
  private:
    uint_t Bits;
    uint_t Mask;
  };

  class DataDecoder
  {
  public:
    explicit DataDecoder(const uint8_t* data, std::size_t size)
      : IsValid(size >= MIN_SIZE)
      , Stream(data + DEPACKER_SIZE, size - DEPACKER_SIZE)
      , Result(new Dump())
      , Decoded(*Result)
    {
      if (IsValid)
      {
        Decoded.reserve(size * 2);
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
      return DEPACKER_SIZE + Stream.GetProcessedBytes();
    }
  private:
    bool DecodeData()
    {
      try
      {
        Decoded.push_back(Stream.GetByte());
        while (Decoded.size() < MAX_DECODED_SIZE)
        {
          if (Stream.GetBit())
          {
            Decoded.push_back(Stream.GetByte());
            continue;
          }
          const uint_t code = Stream.GetBits(2);
          if (code == 0)
          {
            //%0 00
            Require(CopyFromBack(-static_cast<int16_t>(0xfff8 | Stream.GetBits(3)), Decoded, 1));
          }
          else if (code == 1)
          {
            //%0 01
            Require(CopyFromBack(-static_cast<int16_t>(0xff00 | Stream.GetByte()), Decoded, 2));
          }
          else if (code == 2)
          {
            //%0 10
            Require(CopyFromBack(-Stream.GetDist(), Decoded, 3));
          }
          else
          {
            const uint_t len = Stream.GetLen();
            if (len == 9)
            {
              Require(Decoded.size() >= MIN_DECODED_SIZE);
              return true;
            }
            else
            {
              Require(len <= 7);
              const uint_t bits = Stream.GetBits(len);
              Require(CopyFromBack(-Stream.GetDist(), Decoded, 2 + (1 << len) + bits));
            }
          }
        }
      }
      catch (const std::exception&)
      {
      }
      return false;
    }
  private:
    bool IsValid;
    Bitstream Stream;
    std::auto_ptr<Dump> Result;
    Dump& Decoded;
  };
}

namespace Formats
{
  namespace Packed
  {
    class MegaLZDecoder : public Decoder
    {
    public:
      MegaLZDecoder()
        : Depacker(Binary::CreateFormat(MegaLZ::DEPACKER_PATTERN, MegaLZ::MIN_SIZE))
      {
      }

      virtual String GetDescription() const
      {
        return Text::MEGALZ_DECODER_DESCRIPTION;
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
        const std::size_t size = std::min(MegaLZ::MAX_DECODED_SIZE, rawData.Size());
        MegaLZ::DataDecoder decoder(static_cast<const uint8_t*>(rawData.Start()), size);
        return CreatePackedContainer(decoder.GetResult(), decoder.GetUsedSize());
      }
    private:
      const Binary::Format::Ptr Depacker;
    };

    Decoder::Ptr CreateMegaLZDecoder()
    {
      return boost::make_shared<MegaLZDecoder>();
    }
  }
}
