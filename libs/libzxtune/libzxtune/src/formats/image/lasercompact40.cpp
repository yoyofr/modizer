/**
*
* @file
*
* @brief  LaserCompact v4.0 support implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "container.h"
//common includes
#include <contract.h>
//library includes
#include <binary/format_factories.h>
#include <binary/typed_container.h>
#include <formats/image.h>
//text includes
#include <formats/text/image.h>

namespace LaserCompact40
{
  const std::string DEPACKER_PATTERN(

   "0ef9"   //ld c,#f9
   "0d"     //dec c
   "cdc61f" //call #1fc6
   "119000" //ld de,#0090
   "19"     //add hl,de
   "1100?"  //ld de,xxxx
   "7e"     //ld a,(hl)
   "d6c0"   //sub #c0
   "381a"   //jr c,xx
   "23"     //inc hl
   "c8"     //ret z
   "1f"     //rra
   "47"     //ld b,a
   "7e"     //ld a,(hl)
   "3045"   //jr nc,xx
  );

  /*
    @1fc6 48ROM

    e1 pop hl
    c8 ret z
    e9 jp (hl)
  */

  const std::size_t DEPACKER_SIZE = 0x95;

  const std::size_t MIN_SIZE = DEPACKER_SIZE + 16;

  const std::size_t PIXELS_SIZE = 6144;
  const std::size_t ATTRS_SIZE = 768;

  class ByteStream
  {
  public:
    ByteStream(const uint8_t* data, std::size_t size, std::size_t offset)
      : Start(data)
      , Cursor(Start + offset)
      , End(Start + size)
    {
    }

    uint8_t GetByte()
    {
      Require(Cursor != End);
      return *Cursor++;
    }

    std::size_t GetProcessedBytes() const
    {
      return Cursor - Start;
    }
  private:
    const uint8_t* const Start;
    const uint8_t* Cursor;
    const uint8_t* const End;
  };

  class Container
  {
  public:
    explicit Container(const Binary::Data& data)
      : Data(data)
    {
    }

    bool FastCheck() const
    {
      const uint_t sizeCode = *Data.GetField<uint8_t>(DEPACKER_SIZE);
      return sizeCode == 0
          || sizeCode == 1
          || sizeCode == 2
          || sizeCode == 8
          || sizeCode == 9
          || sizeCode == 16
      ;
    }

    ByteStream GetStream() const
    {
      return ByteStream(Data.GetField<uint8_t>(0), Data.GetSize(), DEPACKER_SIZE);
    }
  private:
    const Binary::TypedContainer Data;
  };

  class AddrTranslator
  {
  public:
    AddrTranslator(uint_t sizeCode)
      : AttrBase(256 * sizeCode)
      , ScrStart((AttrBase & 0x0300) << 3)
      , ScrLimit((AttrBase ^ 0x1800) & 0xfc00)
    {
    }

    std::size_t GetStart() const
    {
      return ScrStart;
    }

    std::size_t operator ()(std::size_t virtAddr) const
    {
      if (virtAddr < ScrLimit)
      {
        const std::size_t line = (virtAddr & 0x0007) << 8;
        const std::size_t row =  (virtAddr & 0x0038) << 2;
        const std::size_t col =  (virtAddr & 0x07c0) >> 6;
        return (virtAddr & 0x1800) | line | row | col;
      }
      else
      {
        return AttrBase + virtAddr;
      }
    }
  private:
    const std::size_t AttrBase;
    const std::size_t ScrStart;
    const std::size_t ScrLimit;
  };

  class DataDecoder
  {
  public:
    explicit DataDecoder(const Container& container)
      : IsValid(container.FastCheck())
      , Stream(container.GetStream())
    {
      if (IsValid)
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
      return Stream.GetProcessedBytes();
    }
  private:
    bool DecodeData()
    {
      try
      {
        Dump decoded(PIXELS_SIZE + ATTRS_SIZE);
        std::fill_n(&decoded[PIXELS_SIZE], ATTRS_SIZE, 7);

        const AddrTranslator translate(Stream.GetByte());

        std::size_t target = translate.GetStart();
        for (;;)
        {
          const uint8_t val = Stream.GetByte();
          if (val == 0xc0)
          {
            break;
          }
          if ((val & 0xc1) == 0xc1)
          {
            uint8_t len = (val & 0x3c) >> 2;
            if (0 == len)
            {
              len = Stream.GetByte();
            }
            else if ((val & 0x02) == 0x00)
            {
              ++len;
            }
            const uint8_t fill = (val & 0x02) == 0x00 ? Stream.GetByte() : 0;
            do
            {
              decoded.at(translate(target++)) = fill;
            }
            while (--len);
          }
          else if ((val & 0xc1) == 0xc0)
          {
            uint8_t len = (val & 0x3e) >> 1;
            do
            {
              decoded.at(translate(target++)) = Stream.GetByte();
            }
            while (--len);
          }
          else
          {
            uint8_t len = (val & 0x78) >> 3;
            uint8_t hi = (val & 0x07) | 0xf8;

            if (0 == len)
            {
              const uint8_t next = Stream.GetByte();
              hi = (hi << 1) | (next & 1);
              len = next >> 1;
            }
            ++len;
            uint16_t source = target + (uint_t(hi << 8) | Stream.GetByte());
            const int_t step = (val & 0x80) ? -1 : +1;
            do
            {
              decoded.at(translate(target++)) = decoded.at(translate(source));
              source += step;
            }
            while (--len);
          }
        }
        Result.reset(new Dump());
        if (target <= PIXELS_SIZE)
        {
          decoded.resize(PIXELS_SIZE);
        }
        Result->swap(decoded);
        return true;
      }
      catch (const std::exception&)
      {
        return false;
      }
    }
  private:
    bool IsValid;
    ByteStream Stream;
    std::auto_ptr<Dump> Result;
  };
}

namespace Formats
{
  namespace Image
  {
    class LaserCompact40Decoder : public Decoder
    {
    public:
      LaserCompact40Decoder()
        : Depacker(Binary::CreateFormat(LaserCompact40::DEPACKER_PATTERN, LaserCompact40::MIN_SIZE))
      {
      }

      virtual String GetDescription() const
      {
        return Text::LASERCOMPACT40_DECODER_DESCRIPTION;
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
        const LaserCompact40::Container container(rawData);
        if (!container.FastCheck())
        {
          return Container::Ptr();
        }
        LaserCompact40::DataDecoder decoder(container);
        return CreateImageContainer(decoder.GetResult(), decoder.GetUsedSize());
      }
    private:
      const Binary::Format::Ptr Depacker;
    };

    Decoder::Ptr CreateLaserCompact40Decoder()
    {
      return boost::make_shared<LaserCompact40Decoder>();
    }
  }
}
