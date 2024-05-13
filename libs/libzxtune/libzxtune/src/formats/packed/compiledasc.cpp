/**
* 
* @file
*
* @brief  ASCSoundMaster compiled modules support
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "container.h"
#include "formats/chiptune/aym/ascsoundmaster.h"
//common includes
#include <byteorder.h>
//library includes
#include <binary/format_factories.h>
#include <binary/typed_container.h>
#include <debug/log.h>
//boost includes
#include <boost/array.hpp>
//text includes
#include <formats/text/chiptune.h>
#include <formats/text/packed.h>

namespace
{
  const Debug::Stream Dbg("Formats::Packed::CompiledASC");
}

namespace CompiledASC
{
  const std::size_t MAX_MODULE_SIZE = 0x3a00;
  const std::size_t MAX_PLAYER_SIZE = 1700;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  typedef boost::array<uint8_t, 63> InfoData;

  struct Version0
  {
    static const String DESCRIPTION;
    static const std::string FORMAT;

    static Formats::Chiptune::ASCSoundMaster::Decoder::Ptr CreateDecoder()
    {
      return Formats::Chiptune::ASCSoundMaster::Ver0::CreateDecoder();
    }

    PACK_PRE struct Player
    {
      uint8_t Padding1[12];
      ruint16_t InitAddr;
      uint8_t Padding2;
      ruint16_t PlayAddr;
      uint8_t Padding3;
      ruint16_t ShutAddr;
      //+20
      InfoData Information;
      //+83
      uint8_t Initialization;
      uint8_t Padding4[29];
      ruint16_t DataAddr;

      std::size_t GetSize() const
      {
        const uint_t initAddr = fromLE(InitAddr);
        const uint_t compileAddr = initAddr - offsetof(Player, Initialization);
        return fromLE(DataAddr) - compileAddr;
      }
    } PACK_POST;
  };

  struct Version1 : Version0
  {
    static const String DESCRIPTION;
    static const std::string FORMAT;

    static Formats::Chiptune::ASCSoundMaster::Decoder::Ptr CreateDecoder()
    {
      return Formats::Chiptune::ASCSoundMaster::Ver1::CreateDecoder();
    }
  };

  struct Version2 : Version1
  {
    static const String DESCRIPTION;
    static const std::string FORMAT;

    PACK_PRE struct Player
    {
      uint8_t Padding1[20];
      //+20
      InfoData Information;
      //+83
      uint8_t Initialization;
      uint8_t Padding2[40];
      //+124
      ruint16_t DataOffset;

      std::size_t GetSize() const
      {
        return DataOffset;
      }
    } PACK_POST;
  };
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(offsetof(Version1::Player, Information) == 20);
  BOOST_STATIC_ASSERT(offsetof(Version1::Player, Initialization) == 83);
  BOOST_STATIC_ASSERT(offsetof(Version2::Player, Initialization) == 83);
  BOOST_STATIC_ASSERT(offsetof(Version2::Player, DataOffset) == 124);

  const String Version0::DESCRIPTION = String(Text::ASCSOUNDMASTER0_DECODER_DESCRIPTION) + Text::PLAYER_SUFFIX;
  const String Version1::DESCRIPTION = String(Text::ASCSOUNDMASTER1_DECODER_DESCRIPTION) + Text::PLAYER_SUFFIX;
  const String Version2::DESCRIPTION = String(Text::ASCSOUNDMASTER2_DECODER_DESCRIPTION) + Text::PLAYER_SUFFIX;

  const std::string ID_FORMAT(
    "'A'S'M' 'C'O'M'P'I'L'A'T'I'O'N' 'O'F' "
    "?{20}" //title
    "?{4}"  //any text
    "?{20}" //author
  );

  const std::string BASE_FORMAT = 
    "?{11}" //unknown
    "c3??"  //init
    "c3??"  //play
    "c3??"  //silent
    + ID_FORMAT +
    //+0x53    init
    "af"       //xor a
    "?{28}"
  ;

  const std::string Version0::FORMAT = BASE_FORMAT +
    //+0x70
    "11??"     //ld de,ModuleAddr
    "42"       //ld b,d
    "4b"       //ld c,e
    "1a"       //ld a,(de)
    "13"       //inc de
    "32??"     //ld (xxx),a
    "cd??"     //call xxxx
  ;  

  const std::string Version1::FORMAT = BASE_FORMAT +
    //+0x70
    "11??"     //ld de,ModuleAddr
    "42"       //ld b,d
    "4b"       //ld c,e
    "1a"       //ld a,(de)
    "13"       //inc de
    "32??"     //ld (xxx),a
    "1a"       //ld a,(de)
    "13"       //inc de
    "32??"     //ld (xxx),a
  ;

  const std::string Version2::FORMAT =
    "?{11}"     //padding
    "184600"
    "c3??"
    "c3??"
    + ID_FORMAT +
    //+0x53 init
    "cd??"
    "3b3b"
    "?{35}"
    //+123
    "11??" //data offset
  ;

  bool IsInfoEmpty(const InfoData& info)
  {
    //19 - fixed
    //20 - author
    //4  - ignore
    //20 - title
    const uint8_t* const authorStart = info.begin() + 19;
    const uint8_t* const ignoreStart = authorStart + 20;
    const uint8_t* const titleStart = ignoreStart + 4;
    return ignoreStart == std::find_if(authorStart, ignoreStart, std::bind2nd(std::greater<Char>(), Char(' ')))
        && info.end() == std::find_if(titleStart, info.end(), std::bind2nd(std::greater<Char>(), Char(' ')));
  }
}//CompiledASC

namespace Formats
{
  namespace Packed
  {
    template<class Version>
    class CompiledASCDecoder : public Decoder
    {
    public:
      CompiledASCDecoder()
        : Player(Binary::CreateFormat(Version::FORMAT, sizeof(typename Version::Player)))
        , Decoder(Version::CreateDecoder())
      {
      }

      virtual String GetDescription() const
      {
        return Version::DESCRIPTION;
      }

      virtual Binary::Format::Ptr GetFormat() const
      {
        return Player;
      }

      virtual Container::Ptr Decode(const Binary::Container& rawData) const
      {
        if (!Player->Match(rawData))
        {
          return Container::Ptr();
        }
        const Binary::TypedContainer typedData(rawData);
        const std::size_t availSize = rawData.Size();
        const typename Version::Player& rawPlayer = *typedData.GetField<typename Version::Player>(0);
        const std::size_t playerSize = rawPlayer.GetSize();
        if (playerSize >= std::min(availSize, CompiledASC::MAX_PLAYER_SIZE))
        {
          Dbg("Invalid player");
          return Container::Ptr();
        }
        Dbg("Detected player in first %1% bytes", playerSize);
        const std::size_t modDataSize = std::min(CompiledASC::MAX_MODULE_SIZE, availSize - playerSize);
        const Binary::Container::Ptr modData = rawData.GetSubcontainer(playerSize, modDataSize);
        const Dump metainfo(rawPlayer.Information.begin(), rawPlayer.Information.end());
        if (CompiledASC::IsInfoEmpty(rawPlayer.Information))
        {
          Dbg("Player has empty metainfo");
          if (const Binary::Container::Ptr originalModule = Decoder->Decode(*modData))
          {
            const std::size_t originalSize = originalModule->Size();
            return CreatePackedContainer(originalModule, playerSize + originalSize);
          }
        }
        else if (const Binary::Container::Ptr fixedModule = Decoder->InsertMetainformation(*modData, metainfo))
        {
          if (Decoder->Decode(*fixedModule))
          {
            const std::size_t originalSize = fixedModule->Size() - metainfo.size();
            return CreatePackedContainer(fixedModule, playerSize + originalSize);
          }
          Dbg("Failed to parse fixed module");
        }
        Dbg("Failed to find module after player");
        return Container::Ptr();
      }
    private:
      const Binary::Format::Ptr Player;
      const Formats::Chiptune::ASCSoundMaster::Decoder::Ptr Decoder;
    };

    Decoder::Ptr CreateCompiledASC0Decoder()
    {
      return boost::make_shared<CompiledASCDecoder<CompiledASC::Version0> >();
    }

    Decoder::Ptr CreateCompiledASC1Decoder()
    {
      return boost::make_shared<CompiledASCDecoder<CompiledASC::Version1> >();
    }

    Decoder::Ptr CreateCompiledASC2Decoder()
    {
      return boost::make_shared<CompiledASCDecoder<CompiledASC::Version2> >();
    }
  }//namespace Packed
}//namespace Formats
