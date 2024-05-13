/**
* 
* @file
*
* @brief  Hobeta image support
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "container.h"
//common includes
#include <byteorder.h>
#include <pointers.h>
//library includes
#include <binary/format_factories.h>
#include <formats/packed.h>
#include <math/numeric.h>
//std includes
#include <numeric>
//boost includes
#include <boost/array.hpp>
//text includes
#include <formats/text/packed.h>

namespace Hobeta
{
#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct Header
  {
    boost::array<uint8_t, 9> Filename;
    ruint16_t Start;
    ruint16_t Length;
    ruint16_t FullLength;
    ruint16_t CRC;
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(Header) == 17);
  const std::size_t MIN_SIZE = 0x100;
  const std::size_t MAX_SIZE = 0xff00;

  bool Check(const void* rawData, std::size_t limit)
  {
    if (limit < sizeof(Header))
    {
      return false;
    }
    const uint8_t* const data = static_cast<const uint8_t*>(rawData);
    const Header* const header = static_cast<const Header*>(rawData);
    const std::size_t dataSize = fromLE(header->Length);
    const std::size_t fullSize = fromLE(header->FullLength);
    if (!Math::InRange(dataSize, MIN_SIZE, MAX_SIZE) ||
        dataSize + sizeof(*header) > limit ||
        fullSize != Math::Align<std::size_t>(dataSize, 256) ||
        //check for valid name
        header->Filename.end() != std::find_if(header->Filename.begin(), header->Filename.end(),
          std::bind2nd(std::less<uint8_t>(), uint8_t(' ')))
        )
    {
      return false;
    }
    //check for crc
    if (fromLE(header->CRC) == ((105 + 257 * std::accumulate(data, data + 15, 0u)) & 0xffff))
    {
      return true;
    }
    return false;
  }

  const std::string FORMAT(
    //Filename
    "20-7a 20-7a 20-7a 20-7a 20-7a 20-7a 20-7a 20-7a 20-7a"
    //Start
    "??"
    //Length
    "?01-ff"
    //FullLength
    "0001-ff"
  );
}

namespace Formats
{
  namespace Packed
  {
    class HobetaDecoder : public Decoder
    {
    public:
      HobetaDecoder()
        : Format(Binary::CreateFormat(Hobeta::FORMAT, Hobeta::MIN_SIZE))
      {
      }

      virtual String GetDescription() const
      {
        return Text::HOBETA_DECODER_DESCRIPTION;
      }

      virtual Binary::Format::Ptr GetFormat() const
      {
        return Format;
      }

      virtual Formats::Packed::Container::Ptr Decode(const Binary::Container& rawData) const
      {
        if (!Format->Match(rawData))
        {
          return Formats::Packed::Container::Ptr();
        }
        const uint8_t* const data = static_cast<const uint8_t*>(rawData.Start());
        const std::size_t availSize = rawData.Size();
        if (!Hobeta::Check(data, availSize))
        {
          return Formats::Packed::Container::Ptr();
        }
        const Hobeta::Header* const header = safe_ptr_cast<const Hobeta::Header*>(data);
        const std::size_t dataSize = fromLE(header->Length);
        const std::size_t fullSize = fromLE(header->FullLength);
        const Binary::Container::Ptr subdata = rawData.GetSubcontainer(sizeof(*header), dataSize);
        return CreatePackedContainer(subdata, fullSize + sizeof(*header));
      }
    private:
      const Binary::Format::Ptr Format;
    };

    Decoder::Ptr CreateHobetaDecoder()
    {
      return boost::make_shared<HobetaDecoder>();
    }
  }
}
