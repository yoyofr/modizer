/**
* 
* @file
*
* @brief  RAR archives support
*
* @author vitamin.caig@gmail.com
*
**/

//library includes
#include <binary/format_factories.h>
#include <binary/typed_container.h>
#include <debug/log.h>
#include <formats/archived.h>
#include <formats/packed/decoders.h>
#include <formats/packed/rar_supp.h>
//std includes
#include <cstring>
#include <deque>
#include <numeric>
//boost includes
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/scoped_ptr.hpp>
//text include
#include <formats/text/packed.h>

namespace Rar
{
  const Debug::Stream Dbg("Formats::Archived::Rar");

  using namespace Formats;

  struct FileBlock
  {
    const Packed::Rar::FileBlockHeader* Header;
    std::size_t Offset;
    std::size_t Size;

    FileBlock()
      : Header()
      , Offset()
      , Size()
    {
    }

    FileBlock(const Packed::Rar::FileBlockHeader* header, std::size_t offset, std::size_t size)
      : Header(header)
      , Offset(offset)
      , Size(size)
    {
    }

    std::size_t GetUnpackedSize() const
    {
      return fromLE(Header->UnpackedSize);
    }

    bool HasParent() const
    {
      return Header->IsSolid();
    }

    bool IsChained() const
    {
      return !Header->IsStored();
    }
  };

  class BlocksIterator
  {
  public:
    explicit BlocksIterator(const Binary::Container& data)
      : Container(data)
      , Limit(data.Size())
      , Offset(0)
    {
    }

    bool IsEof() const
    {
      const uint64_t curBlockSize = GetBlockSize();
      return !curBlockSize || uint64_t(Offset) + curBlockSize > uint64_t(Limit);
    }

    const Packed::Rar::ArchiveBlockHeader* GetArchiveHeader() const
    {
      assert(!IsEof());
      const Packed::Rar::ArchiveBlockHeader& block = *Container.GetField<Packed::Rar::ArchiveBlockHeader>(Offset);
      return !block.IsExtended() && Packed::Rar::ArchiveBlockHeader::TYPE == block.Type
        ? &block
        : 0;
    }
  
    const Packed::Rar::FileBlockHeader* GetFileHeader() const
    {
      assert(!IsEof());
      if (const Packed::Rar::FileBlockHeader* const block = Container.GetField<Packed::Rar::FileBlockHeader>(Offset))
      {
        return block->IsValid()
          ? block
          : 0;
      }
      return 0;
    }

    std::size_t GetOffset() const
    {
      return Offset;
    }

    uint64_t GetBlockSize() const
    {
      if (const Packed::Rar::BlockHeader* const block = Container.GetField<Packed::Rar::BlockHeader>(Offset))
      {
        uint64_t res = fromLE(block->Size);
        if (block->IsExtended())
        {
          if (const Packed::Rar::ExtendedBlockHeader* const extBlock = Container.GetField<Packed::Rar::ExtendedBlockHeader>(Offset))
          {
            res += fromLE(extBlock->AdditionalSize);
            //Even if big files are not supported, we should properly skip them in stream
            if (Packed::Rar::FileBlockHeader::TYPE == extBlock->Type && 
                Packed::Rar::FileBlockHeader::FLAG_BIG_FILE & fromLE(extBlock->Flags))
            {
              if (const Packed::Rar::BigFileBlockHeader* const bigFile = Container.GetField<Packed::Rar::BigFileBlockHeader>(Offset))
              {
                res += uint64_t(fromLE(bigFile->PackedSizeHi)) << (8 * sizeof(uint32_t));
              }
              else
              {
                return sizeof(*bigFile);
              }
            }
          }
          else
          {
            return sizeof(*extBlock);
          }
        }
        return res;
      }
      else
      {
        return sizeof(*block);
      }
    }

    void Next()
    {
      assert(!IsEof());
      Offset += GetBlockSize();
      if (const Packed::Rar::BlockHeader* block = Container.GetField<Packed::Rar::BlockHeader>(Offset))
      {
        //finish block
        if (block->Type == 0x7b && 7 == fromLE(block->Size))
        {
          Offset += sizeof(*block);
        }
      }
    }
  private:
    const Binary::TypedContainer Container;
    const std::size_t Limit;
    std::size_t Offset;
  };

  class ChainDecoder
  {
  public:
    typedef boost::shared_ptr<ChainDecoder> Ptr;

    explicit ChainDecoder(Binary::Container::Ptr data)
      : Data(data)
      , StatefulDecoder(Packed::CreateRarDecoder())
      , ChainIterator(new BlocksIterator(*Data))
    {
    }

    Binary::Container::Ptr DecodeBlock(const FileBlock& block) const
    {
      if (block.IsChained() && block.HasParent())
      {
        return AdvanceIterator(block.Offset, &ChainDecoder::ProcessBlock)
          ? DecodeSingleBlock(block)
          : Binary::Container::Ptr();
      }
      else
      {
        return AdvanceIterator(block.Offset, &ChainDecoder::SkipBlock)
          ? DecodeSingleBlock(block)
          : Binary::Container::Ptr();
      }
    }
  private:
    bool AdvanceIterator(std::size_t offset, void (ChainDecoder::*BlockOp)(const FileBlock&) const) const
    {
      if (ChainIterator->GetOffset() > offset)
      {
        Dbg(" Reset caching iterator to beginning");
        ChainIterator.reset(new BlocksIterator(*Data));
      }
      while (ChainIterator->GetOffset() <= offset && !ChainIterator->IsEof())
      {
        const FileBlock& curBlock = FileBlock(ChainIterator->GetFileHeader(), ChainIterator->GetOffset(), ChainIterator->GetBlockSize());
        ChainIterator->Next();
        if (curBlock.Header)
        {
          if (curBlock.Offset == offset)
          {
            return true;
          }
          else if (curBlock.IsChained())
          {
            (this->*BlockOp)(curBlock);
          }
        }
      }
      return false;
    }

    Binary::Container::Ptr DecodeSingleBlock(const FileBlock& block) const
    {
      Dbg(" Decoding block @%1% (chained=%2%, hasParent=%3%)", block.Offset, block.IsChained(), block.HasParent());
      const Binary::Container::Ptr blockContent = Data->GetSubcontainer(block.Offset, block.Size);
      return StatefulDecoder->Decode(*blockContent);
    }

    void ProcessBlock(const FileBlock& block) const
    {
      Dbg(" Decoding parent block @%1% (chained=%2%, hasParent=%3%)", block.Offset, block.IsChained(), block.HasParent());
      const Binary::Container::Ptr blockContent = Data->GetSubcontainer(block.Offset, block.Size);
      StatefulDecoder->Decode(*blockContent);
    }

    void SkipBlock(const FileBlock& block) const
    {
      Dbg(" Skip block @%1% (chained=%2%, hasParent=%3%)", block.Offset, block.IsChained(), block.HasParent());
    }
  private:
    const Binary::Container::Ptr Data;
    const Formats::Packed::Decoder::Ptr StatefulDecoder;
    mutable boost::scoped_ptr<BlocksIterator> ChainIterator;
  };

  class File : public Archived::File
  {
  public:
    File(ChainDecoder::Ptr decoder, const FileBlock& block, const String& name)
      : Decoder(decoder)
      , Block(block)
      , Name(name)
    {
    }

    virtual String GetName() const
    {
      return Name;
    }

    virtual std::size_t GetSize() const
    {
      return Block.GetUnpackedSize();
    }

    virtual Binary::Container::Ptr GetData() const
    {
      Dbg("Decompressing '%1%' started at %2%", Name, Block.Offset);
      return Decoder->DecodeBlock(Block);
    }
  private:
    const ChainDecoder::Ptr Decoder;
    const FileBlock Block;
    const String Name;
  };

  class FileIterator
  {
  public:
    FileIterator(ChainDecoder::Ptr decoder, const Binary::Container& data)
      : Decoder(decoder)
      , Data(data)
      , Blocks(data)
    {
      SkipNonFileBlocks();
    }

    bool IsEof() const
    {
      return Blocks.IsEof();
    }

    bool IsValid() const
    {
      assert(!IsEof());
      const Formats::Packed::Rar::FileBlockHeader& file = *Blocks.GetFileHeader();
      return file.IsSupported();
    }

    String GetName() const
    {
      const Formats::Packed::Rar::FileBlockHeader& file = *Blocks.GetFileHeader();
      String name = file.GetName();
      std::replace(name.begin(), name.end(), '\\', '/');
      return name;
    }

    Archived::File::Ptr GetFile() const
    {
      const Formats::Packed::Rar::FileBlockHeader& file = *Blocks.GetFileHeader();
      if (file.IsSupported() && !Current)
      {
        const FileBlock block(&file, Blocks.GetOffset(), Blocks.GetBlockSize());
        Current = boost::make_shared<File>(Decoder, block, GetName());
      }
      return Current;
    }

    void Next()
    {
      assert(!IsEof());
      Current.reset();
      Blocks.Next();
      SkipNonFileBlocks();
    }

    std::size_t GetOffset() const
    {
      return Blocks.GetOffset();
    }
  private:
    void SkipNonFileBlocks()
    {
      while (!Blocks.IsEof() && !Blocks.GetFileHeader())
      {
        Blocks.Next();
      }
    }
  private:
    const ChainDecoder::Ptr Decoder;
    const Binary::Container& Data;
    BlocksIterator Blocks;
    mutable Archived::File::Ptr Current;
  };

  class Container : public Archived::Container
  {
  public:
    Container(Binary::Container::Ptr data, uint_t filesCount)
      : Decoder(boost::make_shared<ChainDecoder>(data))
      , Delegate(data)
      , FilesCount(filesCount)
    {
      Dbg("Found %1% files. Size is %2%", filesCount, Delegate->Size());
    }

    //Binary::Container
    virtual const void* Start() const
    {
      return Delegate->Start();
    }

    virtual std::size_t Size() const
    {
      return Delegate->Size();
    }

    virtual Binary::Container::Ptr GetSubcontainer(std::size_t offset, std::size_t size) const
    {
      return Delegate->GetSubcontainer(offset, size);
    }

    //Archive::Container
    virtual void ExploreFiles(const Archived::Container::Walker& walker) const
    {
      for (FileIterator iter(Decoder, *Delegate); !iter.IsEof(); iter.Next())
      {
        if (!iter.IsValid())
        {
          continue;
        }
        const Archived::File::Ptr fileObject = iter.GetFile();
        walker.OnFile(*fileObject);
      }
    }

    virtual Archived::File::Ptr FindFile(const String& name) const
    {
      for (FileIterator iter(Decoder, *Delegate); !iter.IsEof(); iter.Next())
      {
        if (iter.IsValid() && iter.GetName() == name)
        {
          return iter.GetFile();
        }
      }
      return Archived::File::Ptr();
    }

    virtual uint_t CountFiles() const
    {
      return FilesCount;
    }
  private:
    const ChainDecoder::Ptr Decoder;
    const Binary::Container::Ptr Delegate;
    const uint_t FilesCount;
  };

  const std::string FORMAT(
    //file marker
    "5261"       //uint16_t CRC;   "Ra"
    "72"         //uint8_t Type;   "r"
    "211a"       //uint16_t Flags; "!ESC^"
    "0700"       //uint16_t Size;  "BELL^,0"
    //archive header
    "??"         //uint16_t CRC;
    "73"         //uint8_t Type;
    "??"         //uint16_t Flags;
    "0d00"       //uint16_t Size
  );
}

namespace Formats
{
  namespace Archived
  {
    class RarDecoder : public Decoder
    {
    public:
      RarDecoder()
        : Format(Binary::CreateFormat(Rar::FORMAT))
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

      virtual Container::Ptr Decode(const Binary::Container& data) const
      {
        if (!Format->Match(data))
        {
          return Container::Ptr();
        }

        uint_t filesCount = 0;
        Rar::BlocksIterator iter(data);
        for (; !iter.IsEof(); iter.Next())
        {
          if (const Formats::Packed::Rar::FileBlockHeader* file = iter.GetFileHeader())
          {
            filesCount += file->IsSupported();
          }
        }
        if (const std::size_t totalSize = iter.GetOffset())
        {
          const Binary::Container::Ptr archive = data.GetSubcontainer(0, totalSize);
          return boost::make_shared<Rar::Container>(archive, filesCount);
        }
        else
        {
          return Container::Ptr();
        }
      }
    private:
      const Binary::Format::Ptr Format;
    };

    Decoder::Ptr CreateRarDecoder()
    {
      return boost::make_shared<RarDecoder>();
    }
  }
}

