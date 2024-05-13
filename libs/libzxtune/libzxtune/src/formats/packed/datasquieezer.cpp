/**
* 
* @file
*
* @brief  DataSqueezer packer support
*
* @author vitamin.caig@gmail.com
*
* @note   Based on XLook sources by HalfElf
*
**/

//local includes
#include "container.h"
#include "pack_utils.h"
//common includes
#include <byteorder.h>
#include <contract.h>
#include <pointers.h>
//library includes
#include <binary/format_factories.h>
#include <formats/packed.h>
#include <math/numeric.h>
//std includes
#include <algorithm>
#include <iterator>
#include <numeric>
//boost includes
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
//text includes
#include <formats/text/packed.h>

namespace DataSquieezer
{
  const std::size_t MAX_DECODED_SIZE = 0xc000;

  /*
     classic depacker
     bitstream:
     %0,b8,continue
     %11,len=2+b1
     %101,len=4+b2
     %1001,len=8+b4 if len==0x17 copy 0xe+b5 bytes,continue
     %1000,len=0x17+b8 untill data!=0xff
     %1,offset=0x221+bX
     %01,offset=1+b5
     %00,offset=0x21+b9
     backcopy len bytes from offset,continue
  */
  const std::string DEPACKER_PATTERN(
    "11??"      // ld de,xxxx ;buffer
    "21??"      // ld hl,xxxx ;addr+start depacker
    "d5"        // push de
    "01??"      // ld bc,xxxx ;rest depacker size
    "edb0"      // ldir
    "11??"      // ld de,xxxx (data = +0x0d) ;addr
    "21??"      // ld hl,xxxx (data = +0x10) ;packed data start
    "01??"      // ld bc,xxxx (data = +0x13) ;packed size
    "c9"        // ret
    "ed?"       // ldir/lddr  (data = +0x17)
    "010801"    // ld bc,#108 ;b- counter of bits, c=8
    "21??"      // ld hl,xxxx (data = +0x1c) ;last packed byte
    "d9"        // exx
    "e5"        // push hl
//last depacked. word parameter. offset=0x21
    "11??"      // ld de,xxxx ;last depacked byte
/*
                //depcycle:
    "210100"    // ld hl,1    ;l- bytes to copy
    "cd??"      // call getbit
    "3027"      // jr nc,copy_byte
    "45"        // ld b,l     ;%1
    "2c"        // inc l
    "cd??"      // call getbit
    "3830"      // jr c, +62
    "04"        // inc b       ;%10
    "2e04"      // ld l,4
    "cd??"      // call getbit
    "3828"      // jr c, +62
    "cd??"      // call getbit ;%100
    "301f"      // jr nc, +5e
    "45"        // ld b,l      ;%1001
    "cd??"      // call getbits
    "c608"      // ld a,8
    "6f"        // ld l,a
    "fe17"      // cp #17      ;?
    "381f"      // jr c, +69
    "0605"       // ld b,5      ;0xe+b5
    "cd??"      // call getbits
    "c60e"      // ld a,14
    "6f"        // ld l,a
                //copy_byte:
    "0608"      // ld b,8
    "cd??"      // call getbits
    "12"        // ld (de),a
    "1b"        // dec de
    "2d"        // dec l
    "20f6"      // jr nz,copy_byte
    "182f"      // jr  +8d
    "2e17"      // ld l,#17
    "0608"      // ld b,8       ;8bits offset
    "cd??"      // call getbits
    "09"        // add hl,bc
    "3c"        // inc a
    "28f7"      // jr z, +60    ;repeat while offset==0xff
    "e5"        // push hl
    "cd??"      // call getbit
    "212100"    // ld hl,#21
    "380f"      // jr c,xx
    "0609"      // ld b,9
    "19"        // add hl,de
    "cd??"      // call getbit
    "300c"      // jr nc,do_getbits
    "0605"      // ld b,5
    "6b"        // ld l,e
    "62"        // ld h,d
    "23"        // inc hl
    "1805"      // jr do_getbits
//byte parameter. offset=0x82
    "06?"       // ld b,xx
    "2602"      // ld h,2
    "19"        // add hl,de
                //do_getbits:
    "cd??"      // call getbits
    "09"        // add hl,bc
    "c1"        // pop bc
    "edb8"      // lddr
//word parameter. offset=0x8e
    "21??"      // ld hl,xxxx ;depack start-1
    "ed52"      // sbc hl,de
    "388f"      // jr c,depcycle
    "e1"        // pop hl
    "d9"        // exx
    "c3929c"    // jp xxxx  ;jump to addr
//0xbf83 getting bits set, b- count
//getbits to ba(bc)
    "af"        // xor a
    "4f"        // ld c,a
    "cd??"      // call getbit
    "cb11"      // rl c
    "17"        // rla
    "10f8"      // djnz +9b
    "47"        // ld b,a
    "79"        // ld a,c
    "c9"        // ret
//0xbf90 getting bit
//getbit:
    "d9"        // exx
    "1003"      // djnz +ac
    "41"        // ld b,c
    "5e"        // ld e,(hl)
    "2b"        // dec hl
    "cb13"      // rl e
    "d9"        // exx
    "c9"        // ret
*/
  );

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct RawHeader
  {
    //+0
    uint8_t Padding1[0x0d];
    //+0x0d
    ruint16_t PackedTarget;
    //+0x0f
    uint8_t Padding2;
    //+0x10
    ruint16_t PackedSource;
    //+0x12
    uint8_t Padding3;
    //+0x13
    ruint16_t SizeOfPacked;
    //+0x15
    uint8_t Padding4[2];
    //+0x17
    uint8_t PackedDataCopyDirection;
    //+0x18
    uint8_t Padding5[0x04];
    //+0x1c
    ruint16_t LastOfPacked;
    //+0x1e
    uint8_t Padding6[0x3];
    //+0x21
    ruint16_t LastOfDepacked;
    //+0x23
    uint8_t Padding7[0x5f];
    //+0x82
    uint8_t LongOffsetBits;
    //+0x83
    uint8_t Padding8[0xb];
    //+0x8e
    ruint16_t DepackedLimit; //+1 for real first
    //+0x90
    uint8_t Padding9[0x20];
    //+0xb0
    uint8_t Data[1];
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(RawHeader) == 0xb0 + 1);

  const std::size_t MIN_SIZE = sizeof(RawHeader);

  //dsq bitstream decoder
  class Bitstream
  {
  public:
    Bitstream(const uint8_t* data, std::size_t size)
      : Data(data), Pos(Data + size), Bits(), Mask(0)
    {
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

    uint8_t Get8Bits()
    {
      return static_cast<uint8_t>(GetBits(8));
    }
  private:
    uint8_t GetByte()
    {
      Require(Pos > Data);
      return *--Pos;
    }
  private:
    const uint8_t* const Data;
    const uint8_t* Pos;
    uint_t Bits;
    uint_t Mask;
  };

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
      if (!Math::InRange<uint_t>(header.LongOffsetBits, 0x00, 0x10))
      {
        return false;
      }
      if (fromLE(header.LastOfDepacked) < fromLE(header.DepackedLimit))
      {
        return false;
      }
      const DataMovementChecker checker(fromLE(header.PackedSource), fromLE(header.PackedTarget), fromLE(header.SizeOfPacked), header.PackedDataCopyDirection);
      if (!checker.IsValid())
      {
        return false;
      }
      if (checker.LastOfMovedData() != fromLE(header.LastOfPacked))
      {
        return false;
      }
      const uint_t usedSize = GetUsedSize();
      if (Size < usedSize)
      {
        return false;
      }
      return true;
    }

    uint_t GetUsedSize() const
    {
      const RawHeader& header = GetHeader();
      return sizeof(header) + fromLE(header.SizeOfPacked) - sizeof(header.Data);
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

  class DataDecoder
  {
  public:
    explicit DataDecoder(const Container& container)
      : IsValid(container.FastCheck())
      , Header(container.GetHeader())
      , Stream(Header.Data, fromLE(Header.SizeOfPacked))
      , Result(new Dump())
      , Decoded(*Result)
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
  private:
    bool DecodeData()
    {
      const uint_t unpackedSize = fromLE(Header.LastOfDepacked) - fromLE(Header.DepackedLimit);
      Decoded.reserve(unpackedSize);
      try
      {
        while (Decoded.size() < unpackedSize)
        {
          if (!Stream.GetBit())
          {
            Decoded.push_back(Stream.Get8Bits());
          }
          else if (!DecodeCmd())
          {
            return false;
          }
        }
        std::reverse(Decoded.begin(), Decoded.end());
        return true;
      }
      catch (const std::exception&)
      {
        return false;
      }
    }

    bool DecodeCmd()
    {
      const uint_t len = GetLength();
      if (len == uint_t(-1))
      {
        CopySingleBytes();
        return true;
      }
      const uint_t off = GetOffset();
      return CopyFromBack(off, Decoded, len);
    }

    uint_t GetLength()
    {
      //%1
      if (Stream.GetBit())
      {
        //%11
        return 2 + Stream.GetBit();
      }
      //%10
      if (Stream.GetBit())
      {
        //%101
        return 4 + Stream.GetBits(2);
      }
      //%100
      if (Stream.GetBit())
      {
        //%1001
        const uint_t res = 8 + Stream.GetBits(4);
        return res == 0x17 ? uint_t(-1) : res;
      }
      //%1000
      uint_t res = 0x17;
      for (uint_t add = 0xff; add == 0xff;)
      {
        add = Stream.GetBits(8);
        res += add;
      }
      return res;
    }

    uint_t GetOffset()
    {
      if (Stream.GetBit())
      {
        //%1
        return 0x221 + Stream.GetBits(Header.LongOffsetBits);
      }
      //%0
      if (Stream.GetBit())
      {
        //%01
        return 1 + Stream.GetBits(5);
      }
      //%00
      return 0x21 + Stream.GetBits(9);
    }

    void CopySingleBytes()
    {
      const uint_t count = 14 + Stream.GetBits(5);
      std::generate_n(std::back_inserter(Decoded), count, boost::bind(&Bitstream::Get8Bits, &Stream));
    }
  private:
    bool IsValid;
    const RawHeader& Header;
    Bitstream Stream;
    std::auto_ptr<Dump> Result;
    Dump& Decoded;
  };
}

namespace Formats
{
  namespace Packed
  {
    class DataSquieezerDecoder : public Decoder
    {
    public:
      DataSquieezerDecoder()
        : Depacker(Binary::CreateFormat(DataSquieezer::DEPACKER_PATTERN, DataSquieezer::MIN_SIZE))
      {
      }

      virtual String GetDescription() const
      {
        return Text::DSQ_DECODER_DESCRIPTION;
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
        const DataSquieezer::Container container(rawData.Start(), rawData.Size());
        if (!container.FastCheck())
        {
          return Container::Ptr();
        }
        DataSquieezer::DataDecoder decoder(container);
        return CreatePackedContainer(decoder.GetResult(), container.GetUsedSize());
      }
    private:
      const Binary::Format::Ptr Depacker;
    };

    Decoder::Ptr CreateDataSquieezerDecoder()
    {
      return boost::make_shared<DataSquieezerDecoder>();
    }
  }
}
